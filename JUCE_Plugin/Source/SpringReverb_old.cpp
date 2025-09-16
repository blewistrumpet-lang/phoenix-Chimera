// SpringReverb.cpp - Authentic spring reverb with physical modeling
#include "SpringReverb.h"
#include <cmath>
#include <algorithm>
#include <memory>

class SpringReverb::Impl {
public:
    // Delay line with interpolation
    class DelayLine {
        std::vector<float> buffer;
        int writePos = 0;
        int size = 0;
        
    public:
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
            
            // Linear interpolation
            int delayInt = static_cast<int>(delaySamples);
            float frac = delaySamples - delayInt;
            
            delayInt = std::min(delayInt, size - 1);
            int readPos1 = (writePos - delayInt + size) % size;
            int readPos2 = (readPos1 - 1 + size) % size;
            
            return buffer[readPos1] * (1.0f - frac) + buffer[readPos2] * frac;
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }
    };
    
    // Chirped allpass filter for spring dispersion
    class ChirpedAllpass {
        DelayLine delay;
        float feedback = 0.7f;
        float chirpPhase = 0.0f;
        float chirpRate = 0.001f;
        float chirpDepth = 1.0f;
        
    public:
        void init(int maxDelay) {
            delay.init(maxDelay);
        }
        
        float process(float input, float baseDelay, float chirp) {
            // Modulate delay time for chirp effect
            float chirpMod = std::sin(chirpPhase) * chirp * 2.0f;
            chirpPhase += chirpRate;
            if (chirpPhase >= 2.0f * M_PI) chirpPhase -= 2.0f * M_PI;
            
            float currentDelay = baseDelay + chirpMod;
            currentDelay = std::max(1.0f, currentDelay);
            
            float delayed = delay.read(currentDelay);
            float output = -input + delayed;
            delay.write(input + delayed * feedback);
            
            return output;
        }
        
        void reset() {
            delay.reset();
            chirpPhase = 0.0f;
        }
        
        void setChirpRate(float rate) {
            chirpRate = rate;
        }
    };
    
    // Spring tank simulation
    class SpringTank {
        DelayLine mainDelay;
        ChirpedAllpass allpass1;
        ChirpedAllpass allpass2;
        ChirpedAllpass allpass3;
        float dampState = 0.0f;
        
    public:
        void init(double sr, float baseDelayMs) {
            int delaySamples = static_cast<int>(baseDelayMs * sr / 1000.0f * 2.0f); // Extra room
            mainDelay.init(delaySamples);
            
            // Allpass filters for dispersion
            allpass1.init(static_cast<int>(5.0f * sr / 1000.0f));
            allpass2.init(static_cast<int>(7.0f * sr / 1000.0f));
            allpass3.init(static_cast<int>(11.0f * sr / 1000.0f));
            
            // Set different chirp rates for each allpass
            allpass1.setChirpRate(0.001f);
            allpass2.setChirpRate(0.0013f);
            allpass3.setChirpRate(0.0017f);
        }
        
        float process(float input, float delayTime, float feedback, float damping, float chirp) {
            // Read delayed signal
            float delayed = mainDelay.read(delayTime);
            
            // Apply damping
            dampState = delayed * (1.0f - damping) + dampState * damping;
            
            // Process through chirped allpass filters for spring character
            float dispersed = dampState;
            dispersed = allpass1.process(dispersed, 3.0f, chirp);
            dispersed = allpass2.process(dispersed, 5.0f, chirp * 0.7f);
            dispersed = allpass3.process(dispersed, 7.0f, chirp * 0.5f);
            
            // Write to delay with feedback
            mainDelay.write(input + dispersed * feedback);
            
            return dispersed;
        }
        
        void reset() {
            mainDelay.reset();
            allpass1.reset();
            allpass2.reset();
            allpass3.reset();
            dampState = 0.0f;
        }
    };
    
    // Parameters
    float mixParam = 0.5f;
    float tensionParam = 0.5f;
    float dampingParam = 0.5f;
    float decayParam = 0.5f;
    float predelayParam = 0.0f;
    float driveParam = 0.3f;
    float chirpParam = 0.5f;
    float lowCutParam = 0.2f;
    float highCutParam = 0.7f;
    float widthParam = 0.8f;
    
    // DSP components - 3 spring tanks for richer sound
    static constexpr int NUM_SPRINGS = 3;
    SpringTank springsL[NUM_SPRINGS];
    SpringTank springsR[NUM_SPRINGS];
    
    DelayLine predelayL;
    DelayLine predelayR;
    
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
    float delayTime = 30.0f;
    int predelaySamples = 0;
    float driveAmount = 1.0f;
    float chirpAmount = 0.5f;
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Initialize spring tanks with different delay times
        // These create the characteristic spring sound
        const float springDelays[NUM_SPRINGS] = {37.0f, 41.0f, 43.0f};
        
        for (int i = 0; i < NUM_SPRINGS; i++) {
            springsL[i].init(sr, springDelays[i]);
            springsR[i].init(sr, springDelays[i] * 1.1f); // Slight stereo offset
        }
        
        // Initialize predelay (up to 100ms)
        int maxPredelay = static_cast<int>(0.1f * sr);
        predelayL.init(maxPredelay);
        predelayR.init(maxPredelay);
        
        updateCoefficients();
    }
    
    void reset() {
        for (int i = 0; i < NUM_SPRINGS; i++) {
            springsL[i].reset();
            springsR[i].reset();
        }
        
        predelayL.reset();
        predelayR.reset();
        
        lowCutStateL = 0.0f;
        lowCutStateR = 0.0f;
        highCutStateL = 0.0f;
        highCutStateR = 0.0f;
    }
    
    void updateCoefficients() {
        // Mix
        wetGain = mixParam;
        dryGain = 1.0f - mixParam;
        
        // Tension affects delay time and character
        delayTime = 20.0f + (1.0f - tensionParam) * 60.0f; // 20ms to 80ms
        
        // Decay affects feedback
        float decayTime = 0.5f + decayParam * 4.5f; // 0.5s to 5s
        feedback = std::pow(0.001f, delayTime / (decayTime * 1000.0f));
        feedback = std::clamp(feedback, 0.0f, 0.95f);
        
        // Damping
        dampCoeff = dampingParam * 0.7f;
        
        // Predelay
        predelaySamples = static_cast<int>(predelayParam * 0.1f * sampleRate); // 0 to 100ms
        
        // Drive amount (1.0 to 5.0)
        driveAmount = 1.0f + driveParam * 4.0f;
        
        // Chirp amount
        chirpAmount = chirpParam * 3.0f; // 0 to 3 samples of chirp
        
        // Filters
        // Low cut: 20Hz to 500Hz
        float lowCutFreq = 20.0f * std::pow(25.0f, lowCutParam);
        lowCutCoeff = 1.0f - std::exp(-2.0f * M_PI * lowCutFreq / sampleRate);
        
        // High cut: 2kHz to 10kHz
        float highCutFreq = 2000.0f * std::pow(5.0f, highCutParam);
        highCutCoeff = std::exp(-2.0f * M_PI * highCutFreq / sampleRate);
    }
    
    float softClip(float input) {
        // Soft saturation for authentic spring drive
        float abs = std::abs(input);
        if (abs < 0.5f) {
            return input;
        } else if (abs < 1.0f) {
            float x = (abs - 0.5f) * 2.0f;
            float curve = 0.5f + 0.5f * (x - x * x * x / 3.0f);
            return curve * (input > 0 ? 1.0f : -1.0f);
        } else {
            return (input > 0 ? 1.0f : -1.0f) * (2.0f / 3.0f);
        }
    }
    
    float processLowCut(float input, float& state) {
        state += (input - state) * lowCutCoeff;
        return input - state;
    }
    
    float processHighCut(float input, float& state) {
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
            float inputL = leftData[sample];
            float inputR = rightData ? rightData[sample] : inputL;
            
            // Store dry signal
            float dryL = inputL;
            float dryR = inputR;
            
            // Apply drive (soft saturation)
            inputL = softClip(inputL * driveAmount) / driveAmount;
            inputR = softClip(inputR * driveAmount) / driveAmount;
            
            // Apply predelay
            if (predelaySamples > 0) {
                float predelayedL = predelayL.read(predelaySamples);
                float predelayedR = predelayR.read(predelaySamples);
                predelayL.write(inputL);
                predelayR.write(inputR);
                inputL = predelayedL;
                inputR = predelayedR;
            }
            
            // Process through multiple spring tanks
            float springOutL = 0.0f;
            float springOutR = 0.0f;
            
            for (int i = 0; i < NUM_SPRINGS; i++) {
                // Each spring has slightly different parameters for richness
                float tensionOffset = delayTime + i * 3.0f;
                float feedbackScale = feedback * (1.0f - i * 0.05f);
                
                springOutL += springsL[i].process(inputL, tensionOffset, feedbackScale, 
                                                  dampCoeff, chirpAmount) / NUM_SPRINGS;
                springOutR += springsR[i].process(inputR, tensionOffset * 1.05f, feedbackScale, 
                                                  dampCoeff, chirpAmount) / NUM_SPRINGS;
            }
            
            // Apply filters
            float filteredL = processLowCut(springOutL, lowCutStateL);
            filteredL = processHighCut(filteredL, highCutStateL);
            
            float filteredR = processLowCut(springOutR, lowCutStateR);
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
            case 1: tensionParam = value; break;
            case 2: dampingParam = value; break;
            case 3: decayParam = value; break;
            case 4: predelayParam = value; break;
            case 5: driveParam = value; break;
            case 6: chirpParam = value; break;
            case 7: lowCutParam = value; break;
            case 8: highCutParam = value; break;
            case 9: widthParam = value; break;
        }
        
        updateCoefficients();
    }
};

// Public interface implementation
SpringReverb::SpringReverb() : pImpl(std::make_unique<Impl>()) {}
SpringReverb::~SpringReverb() = default;

void SpringReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->init(sampleRate);
}

void SpringReverb::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void SpringReverb::reset() {
    pImpl->reset();
}

void SpringReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index < 10) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String SpringReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Mix";
        case 1: return "Tension";
        case 2: return "Damping";
        case 3: return "Decay";
        case 4: return "Pre-Delay";
        case 5: return "Drive";
        case 6: return "Chirp";
        case 7: return "Low Cut";
        case 8: return "High Cut";
        case 9: return "Width";
        default: return "";
    }
}

int SpringReverb::getNumParameters() const {
    return 10;
}

juce::String SpringReverb::getName() const {
    return "Spring Reverb";
}