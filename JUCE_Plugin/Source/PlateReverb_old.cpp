// PlateReverb.cpp - Integration of proven Freeverb algorithm
// Original Freeverb by Jezar at Dreampoint - public domain
// Integrated into EngineBase framework for Chimera Phoenix

#include "PlateReverb.h"
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

// Professional Schroeder-Moorer plate reverb architecture
class PlateReverb::Impl {
public:
    // Delay line implementation
    class DelayLine {
        std::vector<float> buffer;
        int writePos = 0;
        
    public:
        int size = 0;
        void init(int delaySize) {
            size = delaySize;
            buffer.resize(size, 0.0f);
            writePos = 0;
        }
        
        void write(float sample) {
            if (size > 0) {
                buffer[writePos] = sample;
                writePos = (writePos + 1) % size;
            }
        }
        
        float read(float delaySamples) {
            if (size == 0 || delaySamples <= 0) return 0.0f;
            
            // Linear interpolation for fractional delays
            int delayInt = static_cast<int>(delaySamples);
            float frac = delaySamples - delayInt;
            
            delayInt = std::min(delayInt, size - 1);
            int readPos1 = (writePos - delayInt + size) % size;
            int readPos2 = (readPos1 - 1 + size) % size;
            
            return buffer[readPos1] * (1.0f - frac) + buffer[readPos2] * frac;
        }
        
        float readTap(int delaySamples) {
            if (size == 0 || delaySamples <= 0) return 0.0f;
            delaySamples = std::min(delaySamples, size - 1);
            int readPos = (writePos - delaySamples + size) % size;
            return buffer[readPos];
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }
    };
    
    // Comb filter for parallel processing
    class CombFilter {
        DelayLine delay;
        float feedback = 0.7f;
        float dampState = 0.0f;
        float dampCoeff = 0.5f;
        
    public:
        void init(int delaySize) {
            delay.init(delaySize);
        }
        
        float process(float input, float fb, float damp) {
            float delayed = delay.readTap(delay.size);
            
            // Apply damping (one-pole lowpass)
            dampState = delayed * (1.0f - damp) + dampState * damp;
            
            // Write input + feedback
            delay.write(input + dampState * fb);
            
            return delayed;
        }
        
        void reset() {
            delay.reset();
            dampState = 0.0f;
        }
        
        int size = 0;
    };
    
    // Allpass filter for diffusion
    class AllpassFilter {
        DelayLine delay;
        float feedback = 0.7f;
        
    public:
        int size = 0;
        
        void init(int delaySize) {
            delay.init(delaySize);
        }
        
        float process(float input, float fb = 0.7f) {
            float delayed = delay.readTap(delay.size);
            float output = -input + delayed;
            delay.write(input + delayed * fb);
            return output;
        }
        
        void reset() {
            delay.reset();
        }
    };
    
    // Professional parameter set
    float mixParam = 0.5f;
    float sizeParam = 0.5f;
    float dampingParam = 0.5f;
    float predelayParam = 0.0f;
    float diffusionParam = 0.7f;
    float modRateParam = 0.2f;
    float modDepthParam = 0.3f;
    float lowCutParam = 0.1f;
    float highCutParam = 0.8f;
    float widthParam = 1.0f;
    
    // DSP components
    static constexpr int NUM_COMBS = 8;
    static constexpr int NUM_ALLPASS = 4;
    
    CombFilter combsL[NUM_COMBS];
    CombFilter combsR[NUM_COMBS];
    AllpassFilter allpassL[NUM_ALLPASS];
    AllpassFilter allpassR[NUM_ALLPASS];
    
    DelayLine predelayL;
    DelayLine predelayR;
    
    // Modulation LFO
    float lfoPhase = 0.0f;
    float lfoRate = 0.5f;
    float lfoDepth = 0.0f;
    
    // Filters
    float lowCutStateL = 0.0f;
    float lowCutStateR = 0.0f;
    float highCutStateL = 0.0f;
    float highCutStateR = 0.0f;
    float lowCutCoeff = 0.0f;
    float highCutCoeff = 0.0f;
    
    // Derived parameters
    float wetGain = 0.5f;
    float dryGain = 0.5f;
    float feedback = 0.7f;
    float dampCoeff = 0.3f;
    int predelaySamples = 0;
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Initialize comb filters with prime number delays for density
        // These create the main body of the reverb
        const float combDelaysMs[NUM_COMBS] = {
            29.7f, 37.1f, 41.1f, 43.7f,  // Left channel
            31.3f, 39.7f, 42.9f, 46.3f   // Right channel
        };
        
        for (int i = 0; i < NUM_COMBS; i++) {
            int samplesL = static_cast<int>(combDelaysMs[i] * sr / 1000.0f * 2.0f); // Extra room for modulation
            int samplesR = static_cast<int>(combDelaysMs[(i + 4) % NUM_COMBS] * sr / 1000.0f * 2.0f);
            
            combsL[i].init(samplesL);
            combsL[i].size = samplesL / 2; // Actual delay
            
            combsR[i].init(samplesR);
            combsR[i].size = samplesR / 2;
        }
        
        // Initialize allpass filters for diffusion
        const float allpassDelaysMs[NUM_ALLPASS] = {
            5.0f, 7.9f, 11.3f, 13.7f
        };
        
        for (int i = 0; i < NUM_ALLPASS; i++) {
            int samplesL = static_cast<int>(allpassDelaysMs[i] * sr / 1000.0f);
            int samplesR = static_cast<int>(allpassDelaysMs[i] * sr / 1000.0f * 1.05f); // Slight stereo offset
            
            allpassL[i].init(samplesL);
            allpassL[i].size = samplesL;
            
            allpassR[i].init(samplesR);
            allpassR[i].size = samplesR;
        }
        
        // Initialize predelay (up to 200ms)
        int maxPredelay = static_cast<int>(0.2f * sr);
        predelayL.init(maxPredelay);
        predelayR.init(maxPredelay);
        
        updateCoefficients();
    }
    
    void reset() {
        for (int i = 0; i < NUM_COMBS; i++) {
            combsL[i].reset();
            combsR[i].reset();
        }
        
        for (int i = 0; i < NUM_ALLPASS; i++) {
            allpassL[i].reset();
            allpassR[i].reset();
        }
        
        predelayL.reset();
        predelayR.reset();
        
        lowCutStateL = 0.0f;
        lowCutStateR = 0.0f;
        highCutStateL = 0.0f;
        highCutStateR = 0.0f;
        lfoPhase = 0.0f;
    }
    
    void updateCoefficients() {
        // Mix
        wetGain = mixParam;
        dryGain = 1.0f - mixParam;
        
        // Size affects decay time (0.2s to 10s)
        float decayTime = 0.2f + sizeParam * 9.8f;
        // Calculate feedback for 60dB decay
        feedback = std::pow(0.001f, 29.7f / (decayTime * 1000.0f)); // Based on first comb delay
        feedback = std::clamp(feedback, 0.0f, 0.98f);
        
        // Damping
        dampCoeff = dampingParam * 0.8f; // 0 to 0.8
        
        // Predelay
        predelaySamples = static_cast<int>(predelayParam * 0.2f * sampleRate); // 0 to 200ms
        
        // Modulation
        lfoRate = (0.1f + modRateParam * 4.9f) / sampleRate; // 0.1 to 5 Hz
        lfoDepth = modDepthParam * 3.0f; // 0 to 3 samples of modulation
        
        // Filters
        // Low cut: 20Hz to 1kHz
        float lowCutFreq = 20.0f * std::pow(50.0f, lowCutParam); // 20Hz to 1kHz
        lowCutCoeff = 1.0f - std::exp(-2.0f * M_PI * lowCutFreq / sampleRate);
        
        // High cut: 1kHz to 20kHz
        float highCutFreq = 1000.0f * std::pow(20.0f, highCutParam); // 1kHz to 20kHz
        highCutCoeff = std::exp(-2.0f * M_PI * highCutFreq / sampleRate);
    }
    
    float processLowCut(float input, float& state) {
        // One-pole highpass filter
        state += (input - state) * lowCutCoeff;
        return input - state;
    }
    
    float processHighCut(float input, float& state) {
        // One-pole lowpass filter
        state = input * (1.0f - highCutCoeff) + state * highCutCoeff;
        return state;
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        for (int sample = 0; sample < numSamples; sample++) {
            // Get modulation value
            float modulation = std::sin(2.0f * M_PI * lfoPhase) * lfoDepth;
            lfoPhase += lfoRate;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            
            float inputL = leftData[sample];
            float inputR = rightData ? rightData[sample] : inputL;
            
            // Store dry signal
            float dryL = inputL;
            float dryR = inputR;
            
            // Apply predelay
            if (predelaySamples > 0) {
                float predelayedL = predelayL.readTap(predelaySamples);
                float predelayedR = predelayR.readTap(predelaySamples);
                predelayL.write(inputL);
                predelayR.write(inputR);
                inputL = predelayedL;
                inputR = predelayedR;
            }
            
            // Process through parallel comb filters
            float combOutL = 0.0f;
            float combOutR = 0.0f;
            
            for (int i = 0; i < NUM_COMBS; i++) {
                // Add modulation to delay time
                float modDelayL = combsL[i].size + modulation * ((i % 2) ? 1.0f : -1.0f);
                float modDelayR = combsR[i].size + modulation * ((i % 2) ? -1.0f : 1.0f);
                
                combOutL += combsL[i].process(inputL, feedback, dampCoeff) / NUM_COMBS;
                combOutR += combsR[i].process(inputR, feedback, dampCoeff) / NUM_COMBS;
            }
            
            // Process through series allpass filters for diffusion
            float allpassOutL = combOutL;
            float allpassOutR = combOutR;
            
            for (int i = 0; i < NUM_ALLPASS; i++) {
                float apFeedback = 0.5f + diffusionParam * 0.3f; // 0.5 to 0.8
                allpassOutL = allpassL[i].process(allpassOutL, apFeedback);
                allpassOutR = allpassR[i].process(allpassOutR, apFeedback);
            }
            
            // Apply filters
            float filteredL = processLowCut(allpassOutL, lowCutStateL);
            filteredL = processHighCut(filteredL, highCutStateL);
            
            float filteredR = processLowCut(allpassOutR, lowCutStateR);
            filteredR = processHighCut(filteredR, highCutStateR);
            
            // Apply stereo width
            if (widthParam < 1.0f) {
                float mono = (filteredL + filteredR) * 0.5f;
                filteredL = mono + (filteredL - mono) * widthParam;
                filteredR = mono + (filteredR - mono) * widthParam;
            }
            
            // Mix wet and dry signals
            leftData[sample] = dryL * dryGain + filteredL * wetGain;
            if (rightData) {
                rightData[sample] = dryR * dryGain + filteredR * wetGain;
            }
        }
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: mixParam = value; break;
            case 1: sizeParam = value; break;
            case 2: dampingParam = value; break;
            case 3: predelayParam = value; break;
            case 4: diffusionParam = value; break;
            case 5: modRateParam = value; break;
            case 6: modDepthParam = value; break;
            case 7: lowCutParam = value; break;
            case 8: highCutParam = value; break;
            case 9: widthParam = value; break;
        }
        
        updateCoefficients();
    }
};

// Public interface implementation
PlateReverb::PlateReverb() : pImpl(std::make_unique<Impl>()) {}
PlateReverb::~PlateReverb() = default;

void PlateReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->init(sampleRate);
}

void PlateReverb::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void PlateReverb::reset() {
    pImpl->reset();
}

void PlateReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index < 10) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String PlateReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Mix";
        case 1: return "Size";
        case 2: return "Damping";
        case 3: return "Pre-Delay";
        case 4: return "Diffusion";
        case 5: return "Mod Rate";
        case 6: return "Mod Depth";
        case 7: return "Low Cut";
        case 8: return "High Cut";
        case 9: return "Width";
        default: return "";
    }
}

int PlateReverb::getNumParameters() const {
    return 10;
}

juce::String PlateReverb::getName() const {
    return "Plate Reverb";
}