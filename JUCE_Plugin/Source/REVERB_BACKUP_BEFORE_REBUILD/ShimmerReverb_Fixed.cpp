// ShimmerReverb - Complete rebuild with working pitch-shift and reverb
#include "ShimmerReverb.h"
#include "SMBPitchShiftFixed.h"
#include <cmath>
#include <algorithm>
#include <memory>

// Simple but effective Freeverb implementation
class FreeverbCore {
public:
    static constexpr int NUM_COMBS = 8;
    static constexpr int NUM_ALLPASS = 4;
    
    struct Comb {
        std::vector<float> buffer;
        int writePos = 0;
        float feedback = 0.84f;
        float damp = 0.2f;
        float filterstore = 0.0f;
        
        void init(int size) {
            buffer.resize(size, 0.0f);
            writePos = 0;
            filterstore = 0.0f;
        }
        
        float process(float input) {
            float output = buffer[writePos];
            filterstore = (output * (1.0f - damp)) + (filterstore * damp);
            buffer[writePos] = input + (filterstore * feedback);
            writePos = (writePos + 1) % buffer.size();
            return output;
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            filterstore = 0.0f;
        }
        
        void setDamp(float d) { damp = d; }
        void setFeedback(float f) { feedback = f; }
    };
    
    struct Allpass {
        std::vector<float> buffer;
        int writePos = 0;
        static constexpr float feedback = 0.5f;
        
        void init(int size) {
            buffer.resize(size, 0.0f);
            writePos = 0;
        }
        
        float process(float input) {
            float bufout = buffer[writePos];
            float output = -input + bufout;
            buffer[writePos] = input + (bufout * feedback);
            writePos = (writePos + 1) % buffer.size();
            return output;
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
        }
    };
    
    Comb combs[NUM_COMBS];
    Allpass allpasses[NUM_ALLPASS];
    float gain = 0.015f;  // Input gain scaling
    
    void init(double sampleRate) {
        // Freeverb tuning (scaled for sample rate)
        const int combTuning[NUM_COMBS] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
        const int allpassTuning[NUM_ALLPASS] = {556, 441, 341, 225};
        
        float scale = sampleRate / 44100.0f;
        
        for (int i = 0; i < NUM_COMBS; i++) {
            combs[i].init(static_cast<int>(combTuning[i] * scale));
        }
        
        for (int i = 0; i < NUM_ALLPASS; i++) {
            allpasses[i].init(static_cast<int>(allpassTuning[i] * scale));
        }
    }
    
    void setParameters(float roomSize, float damping) {
        float feedback = roomSize * 0.28f + 0.7f;  // 0.7 to 0.98
        damping = damping * 0.4f;  // 0 to 0.4
        
        for (int i = 0; i < NUM_COMBS; i++) {
            combs[i].setFeedback(feedback);
            combs[i].setDamp(damping);
        }
    }
    
    float process(float input) {
        float output = 0.0f;
        input *= gain;
        
        // Parallel comb filters
        for (int i = 0; i < NUM_COMBS; i++) {
            output += combs[i].process(input);
        }
        
        // Series allpass filters
        for (int i = 0; i < NUM_ALLPASS; i++) {
            output = allpasses[i].process(output);
        }
        
        return output;
    }
    
    void reset() {
        for (int i = 0; i < NUM_COMBS; i++) {
            combs[i].reset();
        }
        for (int i = 0; i < NUM_ALLPASS; i++) {
            allpasses[i].reset();
        }
    }
};

class ShimmerReverb::Impl {
public:
    // Core reverb engines
    FreeverbCore reverbL, reverbR;
    
    // Pitch shifters
    std::unique_ptr<SMBPitchShiftFixed> pitchShifterL;
    std::unique_ptr<SMBPitchShiftFixed> pitchShifterR;
    
    // Buffers for pitch shifting (SMBPitchShift needs block processing)
    static constexpr int PITCH_BUFFER_SIZE = 512;
    float pitchInputL[PITCH_BUFFER_SIZE];
    float pitchInputR[PITCH_BUFFER_SIZE];
    float pitchOutputL[PITCH_BUFFER_SIZE];
    float pitchOutputR[PITCH_BUFFER_SIZE];
    int pitchBufferPos = 0;
    
    // Shimmer feedback delay
    std::vector<float> feedbackDelayL;
    std::vector<float> feedbackDelayR;
    int feedbackWritePos = 0;
    int feedbackDelaySize = 0;
    
    // Parameters
    float pitchShift = 0.5f;      // 0=down octave, 0.5=+octave, 1=+2 octaves
    float shimmerAmount = 0.3f;    // 0-1 shimmer mix
    float roomSize = 0.7f;         // 0-1 reverb size
    float damping = 0.3f;          // 0-1 damping
    float mix = 0.5f;              // 0-1 wet/dry mix
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Initialize reverbs
        reverbL.init(sr);
        reverbR.init(sr);
        
        // Initialize pitch shifters
        pitchShifterL = std::make_unique<SMBPitchShiftFixed>(1024, 4, sr);
        pitchShifterR = std::make_unique<SMBPitchShiftFixed>(1024, 4, sr);
        
        // Initialize feedback delay (100ms)
        feedbackDelaySize = static_cast<int>(sr * 0.1);
        feedbackDelayL.resize(feedbackDelaySize, 0.0f);
        feedbackDelayR.resize(feedbackDelaySize, 0.0f);
        
        reset();
    }
    
    void reset() {
        reverbL.reset();
        reverbR.reset();
        
        if (pitchShifterL) pitchShifterL->reset();
        if (pitchShifterR) pitchShifterR->reset();
        
        std::fill(feedbackDelayL.begin(), feedbackDelayL.end(), 0.0f);
        std::fill(feedbackDelayR.begin(), feedbackDelayR.end(), 0.0f);
        std::fill(pitchInputL, pitchInputL + PITCH_BUFFER_SIZE, 0.0f);
        std::fill(pitchInputR, pitchInputR + PITCH_BUFFER_SIZE, 0.0f);
        std::fill(pitchOutputL, pitchOutputL + PITCH_BUFFER_SIZE, 0.0f);
        std::fill(pitchOutputR, pitchOutputR + PITCH_BUFFER_SIZE, 0.0f);
        
        feedbackWritePos = 0;
        pitchBufferPos = 0;
    }
    
    void updateParameters() {
        reverbL.setParameters(roomSize, damping);
        reverbR.setParameters(roomSize, damping);
    }
    
    float calculatePitchRatio() {
        // Map 0-1 to pitch ratios:
        // 0.0 = 0.5 (down octave)
        // 0.5 = 2.0 (up octave)  
        // 1.0 = 4.0 (up 2 octaves)
        if (pitchShift < 0.5f) {
            return 0.5f + pitchShift;  // 0.5 to 1.0
        } else {
            return 1.0f + (pitchShift - 0.5f) * 6.0f;  // 1.0 to 4.0
        }
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        if (!pitchShifterL || !pitchShifterR) return;
        
        updateParameters();
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        float pitchRatio = calculatePitchRatio();
        
        for (int sample = 0; sample < numSamples; sample++) {
            // Get input
            float inputL = leftData[sample];
            float inputR = rightData ? rightData[sample] : inputL;
            
            // Store dry signal
            float dryL = inputL;
            float dryR = inputR;
            
            // Read from feedback delay
            float feedbackL = feedbackDelayL[feedbackWritePos] * shimmerAmount * 0.5f;
            float feedbackR = feedbackDelayR[feedbackWritePos] * shimmerAmount * 0.5f;
            
            // Mix input with shimmer feedback
            float reverbInputL = inputL + feedbackL;
            float reverbInputR = inputR + feedbackR;
            
            // Process through reverb
            float wetL = reverbL.process(reverbInputL);
            float wetR = reverbR.process(reverbInputR);
            
            // Accumulate samples for pitch shifting
            if (shimmerAmount > 0.01f) {
                pitchInputL[pitchBufferPos] = wetL;
                pitchInputR[pitchBufferPos] = wetR;
                
                pitchBufferPos++;
                
                // When buffer is full, process pitch shift
                if (pitchBufferPos >= PITCH_BUFFER_SIZE) {
                    // Process pitch shift in blocks
                    pitchShifterL->process(pitchInputL, pitchOutputL, PITCH_BUFFER_SIZE, pitchRatio);
                    pitchShifterR->process(pitchInputR, pitchOutputR, PITCH_BUFFER_SIZE, pitchRatio);
                    
                    // Write pitched output to feedback delay
                    for (int i = 0; i < PITCH_BUFFER_SIZE; i++) {
                        int writeIdx = (feedbackWritePos - PITCH_BUFFER_SIZE + i + feedbackDelaySize) % feedbackDelaySize;
                        feedbackDelayL[writeIdx] = pitchOutputL[i];
                        feedbackDelayR[writeIdx] = pitchOutputR[i];
                    }
                    
                    pitchBufferPos = 0;
                }
            }
            
            // Update feedback write position
            feedbackWritePos = (feedbackWritePos + 1) % feedbackDelaySize;
            
            // Mix wet and dry
            leftData[sample] = dryL * (1.0f - mix) + wetL * mix;
            if (rightData) {
                rightData[sample] = dryR * (1.0f - mix) + wetR * mix;
            }
        }
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: pitchShift = value; break;
            case 1: shimmerAmount = value; break;
            case 2: roomSize = value; break;
            case 3: damping = value; break;
            case 4: mix = value; break;
        }
    }
};

// Public interface
ShimmerReverb::ShimmerReverb() : pImpl(std::make_unique<Impl>()) {}
ShimmerReverb::~ShimmerReverb() = default;

void ShimmerReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->init(sampleRate);
}

void ShimmerReverb::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void ShimmerReverb::reset() {
    pImpl->reset();
}

void ShimmerReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index < 5) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String ShimmerReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Pitch Shift";
        case 1: return "Shimmer";
        case 2: return "Room Size";
        case 3: return "Damping";
        case 4: return "Mix";
        default: return "";
    }
}

int ShimmerReverb::getNumParameters() const {
    return 5;
}

juce::String ShimmerReverb::getName() const {
    return "Shimmer Reverb";
}