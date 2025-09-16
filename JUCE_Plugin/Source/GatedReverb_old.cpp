// GatedReverb.cpp - Dynamic gated reverb with envelope control
#include "GatedReverb.h"
#include <cmath>
#include <algorithm>
#include <memory>

class GatedReverb::Impl {
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
    
    // Comb filter for reverb
    class CombFilter {
        DelayLine delay;
        float dampState = 0.0f;
        
    public:
        void init(int delaySize) {
            delay.init(delaySize);
            delay.size = delaySize;
        }
        
        float process(float input, float feedback, float damp) {
            float delayed = delay.readTap(delay.size);
            dampState = delayed * (1.0f - damp) + dampState * damp;
            delay.write(input + dampState * feedback);
            return delayed;
        }
        
        void reset() {
            delay.reset();
            dampState = 0.0f;
        }
    };
    
    // Allpass filter
    class AllpassFilter {
        DelayLine delay;
        
    public:
        void init(int delaySize) {
            delay.init(delaySize);
            delay.size = delaySize;
        }
        
        float process(float input, float feedback = 0.7f) {
            float delayed = delay.readTap(delay.size);
            float output = -input + delayed;
            delay.write(input + delayed * feedback);
            return output;
        }
        
        void reset() {
            delay.reset();
        }
    };
    
    // Gate envelope
    enum GateState {
        CLOSED,
        ATTACKING,
        OPEN,
        HOLDING,
        RELEASING
    };
    
    // Parameters
    float mixParam = 0.5f;
    float thresholdParam = 0.3f;
    float holdParam = 0.3f;
    float releaseParam = 0.5f;
    float attackParam = 0.1f;
    float sizeParam = 0.5f;
    float dampingParam = 0.5f;
    float predelayParam = 0.0f;
    float lowCutParam = 0.1f;
    float highCutParam = 0.8f;
    
    // DSP components
    static constexpr int NUM_COMBS = 8;
    static constexpr int NUM_ALLPASS = 4;
    
    CombFilter combsL[NUM_COMBS];
    CombFilter combsR[NUM_COMBS];
    AllpassFilter allpassL[NUM_ALLPASS];
    AllpassFilter allpassR[NUM_ALLPASS];
    
    DelayLine predelayL;
    DelayLine predelayR;
    
    // Gate state
    GateState gateState = CLOSED;
    float gateLevel = 0.0f;
    float inputLevel = 0.0f;
    float levelDetector = 0.0f;
    int holdCounter = 0;
    int holdSamples = 0;
    
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
    float threshold = 0.01f;
    float attackRate = 0.001f;
    float releaseRate = 0.001f;
    int predelaySamples = 0;
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Initialize comb filters
        const float combDelaysMs[NUM_COMBS] = {
            29.7f, 37.1f, 41.1f, 43.7f,
            31.3f, 39.7f, 42.9f, 46.3f
        };
        
        for (int i = 0; i < NUM_COMBS; i++) {
            int samplesL = static_cast<int>(combDelaysMs[i] * sr / 1000.0f);
            int samplesR = static_cast<int>(combDelaysMs[(i + 4) % NUM_COMBS] * sr / 1000.0f * 1.1f);
            
            combsL[i].init(samplesL);
            combsR[i].init(samplesR);
        }
        
        // Initialize allpass filters
        const float allpassDelaysMs[NUM_ALLPASS] = {
            5.0f, 7.9f, 11.3f, 13.7f
        };
        
        for (int i = 0; i < NUM_ALLPASS; i++) {
            int samplesL = static_cast<int>(allpassDelaysMs[i] * sr / 1000.0f);
            int samplesR = static_cast<int>(allpassDelaysMs[i] * sr / 1000.0f * 1.05f);
            
            allpassL[i].init(samplesL);
            allpassR[i].init(samplesR);
        }
        
        // Initialize predelay
        int maxPredelay = static_cast<int>(0.2f * sr); // 200ms
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
        
        gateState = CLOSED;
        gateLevel = 0.0f;
        inputLevel = 0.0f;
        levelDetector = 0.0f;
        holdCounter = 0;
        
        lowCutStateL = 0.0f;
        lowCutStateR = 0.0f;
        highCutStateL = 0.0f;
        highCutStateR = 0.0f;
    }
    
    void updateCoefficients() {
        // Mix
        wetGain = mixParam;
        dryGain = 1.0f - mixParam;
        
        // Gate threshold (logarithmic scale for better control)
        threshold = 0.001f * std::pow(1000.0f, thresholdParam); // 0.001 to 1.0
        
        // Gate times
        float attackMs = 0.1f + attackParam * 99.9f; // 0.1ms to 100ms
        attackRate = 1.0f / (attackMs * sampleRate / 1000.0f);
        
        float releaseMs = 10.0f + releaseParam * 990.0f; // 10ms to 1000ms
        releaseRate = 1.0f / (releaseMs * sampleRate / 1000.0f);
        
        float holdMs = 10.0f + holdParam * 490.0f; // 10ms to 500ms
        holdSamples = static_cast<int>(holdMs * sampleRate / 1000.0f);
        
        // Reverb parameters
        float decayTime = 0.2f + sizeParam * 4.8f; // 0.2s to 5s
        feedback = std::pow(0.001f, 37.1f / (decayTime * 1000.0f));
        feedback = std::clamp(feedback, 0.0f, 0.95f);
        
        dampCoeff = dampingParam * 0.8f;
        
        // Predelay
        predelaySamples = static_cast<int>(predelayParam * 0.1f * sampleRate); // 0 to 100ms
        
        // Filters
        float lowCutFreq = 20.0f * std::pow(50.0f, lowCutParam); // 20Hz to 1kHz
        lowCutCoeff = 1.0f - std::exp(-2.0f * M_PI * lowCutFreq / sampleRate);
        
        float highCutFreq = 1000.0f * std::pow(20.0f, highCutParam); // 1kHz to 20kHz
        highCutCoeff = std::exp(-2.0f * M_PI * highCutFreq / sampleRate);
    }
    
    float detectLevel(float inputL, float inputR) {
        // Peak detection with fast attack, slow release
        float peak = std::max(std::abs(inputL), std::abs(inputR));
        
        float attackCoeff = 0.01f; // Fast attack
        float releaseCoeff = 0.0001f; // Slow release
        
        if (peak > levelDetector) {
            levelDetector += (peak - levelDetector) * attackCoeff;
        } else {
            levelDetector += (peak - levelDetector) * releaseCoeff;
        }
        
        return levelDetector;
    }
    
    void updateGate(float level) {
        switch (gateState) {
            case CLOSED:
                if (level > threshold) {
                    gateState = ATTACKING;
                }
                break;
                
            case ATTACKING:
                gateLevel += attackRate;
                if (gateLevel >= 1.0f) {
                    gateLevel = 1.0f;
                    gateState = OPEN;
                }
                break;
                
            case OPEN:
                if (level < threshold * 0.9f) { // Hysteresis
                    gateState = HOLDING;
                    holdCounter = 0;
                }
                break;
                
            case HOLDING:
                holdCounter++;
                if (holdCounter >= holdSamples) {
                    gateState = RELEASING;
                } else if (level > threshold) {
                    gateState = OPEN; // Retrigger
                }
                break;
                
            case RELEASING:
                gateLevel -= releaseRate;
                if (gateLevel <= 0.0f) {
                    gateLevel = 0.0f;
                    gateState = CLOSED;
                } else if (level > threshold) {
                    gateState = ATTACKING; // Retrigger
                }
                break;
        }
        
        gateLevel = std::clamp(gateLevel, 0.0f, 1.0f);
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
            
            // Detect input level for gate
            float level = detectLevel(inputL, inputR);
            updateGate(level);
            
            // Apply predelay
            if (predelaySamples > 0) {
                float predelayedL = predelayL.readTap(predelaySamples);
                float predelayedR = predelayR.readTap(predelaySamples);
                predelayL.write(inputL);
                predelayR.write(inputR);
                inputL = predelayedL;
                inputR = predelayedR;
            }
            
            // Process through comb filters
            float combOutL = 0.0f;
            float combOutR = 0.0f;
            
            for (int i = 0; i < NUM_COMBS; i++) {
                combOutL += combsL[i].process(inputL, feedback, dampCoeff) / NUM_COMBS;
                combOutR += combsR[i].process(inputR, feedback, dampCoeff) / NUM_COMBS;
            }
            
            // Process through allpass filters
            float allpassOutL = combOutL;
            float allpassOutR = combOutR;
            
            for (int i = 0; i < NUM_ALLPASS; i++) {
                allpassOutL = allpassL[i].process(allpassOutL, 0.7f);
                allpassOutR = allpassR[i].process(allpassOutR, 0.7f);
            }
            
            // Apply filters
            float filteredL = processLowCut(allpassOutL, lowCutStateL);
            filteredL = processHighCut(filteredL, highCutStateL);
            
            float filteredR = processLowCut(allpassOutR, lowCutStateR);
            filteredR = processHighCut(filteredR, highCutStateR);
            
            // Apply gate envelope to wet signal
            float gatedL = filteredL * gateLevel;
            float gatedR = filteredR * gateLevel;
            
            // Mix wet and dry signals
            leftData[sample] = dryL * dryGain + gatedL * wetGain;
            if (rightData) {
                rightData[sample] = dryR * dryGain + gatedR * wetGain;
            }
        }
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: mixParam = value; break;
            case 1: thresholdParam = value; break;
            case 2: holdParam = value; break;
            case 3: releaseParam = value; break;
            case 4: attackParam = value; break;
            case 5: sizeParam = value; break;
            case 6: dampingParam = value; break;
            case 7: predelayParam = value; break;
            case 8: lowCutParam = value; break;
            case 9: highCutParam = value; break;
        }
        
        updateCoefficients();
    }
};

// Public interface implementation
GatedReverb::GatedReverb() : pImpl(std::make_unique<Impl>()) {}
GatedReverb::~GatedReverb() = default;

void GatedReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->init(sampleRate);
}

void GatedReverb::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void GatedReverb::reset() {
    pImpl->reset();
}

void GatedReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index < 10) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String GatedReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Mix";
        case 1: return "Threshold";
        case 2: return "Hold";
        case 3: return "Release";
        case 4: return "Attack";
        case 5: return "Size";
        case 6: return "Damping";
        case 7: return "Pre-Delay";
        case 8: return "Low Cut";
        case 9: return "High Cut";
        default: return "";
    }
}

int GatedReverb::getNumParameters() const {
    return 10;
}

juce::String GatedReverb::getName() const {
    return "Gated Reverb";
}