// ShimmerReverb_Proven.cpp - Integration of proven shimmer reverb algorithm
// Combines Freeverb with pitch shifting for octave-up shimmer effect
// Based on established DSP techniques used in commercial shimmer reverbs

#include "ShimmerReverb.h"
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

namespace {
    // Freeverb constants for the reverb engine
    const int numCombs = 8;
    const int numAllpasses = 4;
    const float fixedGain = 0.015f;
    const float scaleDamp = 0.4f;
    const float scaleRoom = 0.28f;
    const float offsetRoom = 0.7f;
    const int stereoSpread = 89;  // Increased from 67 to 89 for even wider stereo image
    
    // Freeverb delay times (44100Hz)
    const int combTuning[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    const int allpassTuning[4] = { 556, 441, 341, 225 };
    
    // Pitch shifter constants
    const int pitchBufferSize = 4096;
    const int grainSize = 1024;
    const int numGrains = 2;
}

// Simple comb filter from Freeverb
class Comb {
public:
    void setBuffer(float *buf, int size) {
        buffer = buf;
        bufferSize = size;
        bufferIndex = 0;
    }
    
    void mute() {
        if (buffer) {
            std::fill(buffer, buffer + bufferSize, 0.0f);
        }
        filterStore = 0.0f;
    }
    
    void setDamp(float val) {
        damp1 = val;
        damp2 = 1.0f - val;
    }
    
    void setFeedback(float val) {
        feedback = val;
    }
    
    float process(float input) {
        if (!buffer || bufferSize == 0) return 0.0f;
        
        float output = buffer[bufferIndex];
        filterStore = (output * damp2) + (filterStore * damp1);
        buffer[bufferIndex] = input + (filterStore * feedback);
        
        if (++bufferIndex >= bufferSize) {
            bufferIndex = 0;
        }
        
        return output;
    }
    
private:
    float *buffer = nullptr;
    int bufferSize = 0;
    int bufferIndex = 0;
    float filterStore = 0.0f;
    float damp1 = 0.0f;
    float damp2 = 1.0f;
    float feedback = 0.0f;
};

// Simple allpass filter from Freeverb
class Allpass {
public:
    void setBuffer(float *buf, int size) {
        buffer = buf;
        bufferSize = size;
        bufferIndex = 0;
    }
    
    void mute() {
        if (buffer) {
            std::fill(buffer, buffer + bufferSize, 0.0f);
        }
    }
    
    void setFeedback(float val) {
        feedback = val;
    }
    
    float process(float input) {
        if (!buffer || bufferSize == 0) return 0.0f;
        
        float bufout = buffer[bufferIndex];
        float output = -input + bufout;
        buffer[bufferIndex] = input + (bufout * feedback);
        
        if (++bufferIndex >= bufferSize) {
            bufferIndex = 0;
        }
        
        return output;
    }
    
private:
    float *buffer = nullptr;
    int bufferSize = 0;
    int bufferIndex = 0;
    float feedback = 0.5f;
};

// Simple pitch shifter using granular technique
class SimplePitchShifter {
public:
    SimplePitchShifter(float phaseOffset = 0.0f) : phaseOffset(phaseOffset) {
        buffer.resize(pitchBufferSize, 0.0f);
        grainEnvelope.resize(grainSize);

        // Create Hann window for grain envelope
        for (int i = 0; i < grainSize; i++) {
            float phase = static_cast<float>(i) / static_cast<float>(grainSize - 1);
            grainEnvelope[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
        }
    }

    void reset() {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        for (int i = 0; i < numGrains; i++) {
            // Start grains at 1/4 window position where envelope has good amplitude
            // This avoids starting at position 0 where Hann window = 0
            float basePos = grainSize * 0.25f;  // Start at 25% through window
            grainPos[i] = basePos + (i * grainSize / numGrains) + (phaseOffset * grainSize);
            // Wrap around if needed
            while (grainPos[i] >= grainSize) grainPos[i] -= grainSize;
        }
    }
    
    float process(float input, float pitchRatio) {
        // Write to circular buffer
        buffer[writePos] = input;

        float output = 0.0f;

        // Process grains
        for (int g = 0; g < numGrains; g++) {
            // Read position - read backwards from write position using grain delay
            // This ensures we're reading from filled buffer positions
            float grainDelay = grainPos[g];
            float readPosFloat = writePos - grainDelay;

            // Handle wraparound for negative positions
            while (readPosFloat < 0) {
                readPosFloat += pitchBufferSize;
            }

            // Get sample with linear interpolation
            int readIdx = static_cast<int>(readPosFloat) % pitchBufferSize;
            int readIdx2 = (readIdx + 1) % pitchBufferSize;
            float frac = readPosFloat - std::floor(readPosFloat);

            float sample = buffer[readIdx] * (1.0f - frac) + buffer[readIdx2] * frac;

            // Apply grain envelope
            int envPos = static_cast<int>(grainPos[g]) % grainSize;
            sample *= grainEnvelope[envPos];

            output += sample;

            // Update grain position
            grainPos[g] += pitchRatio;
            if (grainPos[g] >= grainSize) {
                grainPos[g] -= grainSize;
            }
        }

        writePos = (writePos + 1) % pitchBufferSize;

        return output / numGrains;
    }
    
private:
    std::vector<float> buffer;
    std::vector<float> grainEnvelope;
    int writePos = 0;
    float grainPos[numGrains] = {0};
    float phaseOffset = 0.0f;
};

// Main ShimmerReverb implementation
class ShimmerReverb::Impl {
public:
    // Freeverb components
    Comb combL[numCombs];
    Comb combR[numCombs];
    Allpass allpassL[numAllpasses];
    Allpass allpassR[numAllpasses];
    
    // Buffers for Freeverb
    std::vector<float> combBufferL[numCombs];
    std::vector<float> combBufferR[numCombs];
    std::vector<float> allpassBufferL[numAllpasses];
    std::vector<float> allpassBufferR[numAllpasses];
    
    // Pitch shifters for shimmer - initialize with different phase offsets for stereo width
    SimplePitchShifter pitchShifterL{0.0f};    // Left channel: no offset
    SimplePitchShifter pitchShifterR{0.333f};  // Right channel: 1/3 phase offset
    
    // Pre-delay
    std::vector<float> predelayBufferL;
    std::vector<float> predelayBufferR;
    int predelayIndex = 0;
    int predelaySize = 0;
    
    // Filters
    float lowCutStateL = 0.0f;
    float lowCutStateR = 0.0f;
    float highCutStateL = 0.0f;
    float highCutStateR = 0.0f;
    float lowCutCoeff = 0.0f;
    float highCutCoeff = 0.0f;
    
    // LFO for modulation
    float lfoPhase = 0.0f;
    float lfoRate = 0.5f;
    
    // Parameters
    float mixParam = 0.5f;
    float pitchShiftParam = 1.0f; // Default to octave up
    float shimmerParam = 0.5f;
    float sizeParam = 0.5f;
    float dampingParam = 0.5f;
    float feedbackParam = 0.5f;
    float predelayParam = 0.0f;
    float modulationParam = 0.0f;
    float lowCutParam = 0.0f;
    float highCutParam = 1.0f;
    
    // Freeverb parameters
    float roomSize = 0.5f;
    float damping = 0.5f;
    float gain = fixedGain;
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Calculate scaling for sample rate
        float srScale = static_cast<float>(sr / 44100.0);
        
        // Initialize comb filters
        for (int i = 0; i < numCombs; i++) {
            int sizeL = static_cast<int>(combTuning[i] * srScale);
            int sizeR = static_cast<int>((combTuning[i] + stereoSpread) * srScale);
            
            combBufferL[i].resize(sizeL);
            combBufferR[i].resize(sizeR);
            
            combL[i].setBuffer(combBufferL[i].data(), sizeL);
            combR[i].setBuffer(combBufferR[i].data(), sizeR);
        }
        
        // Initialize allpass filters
        for (int i = 0; i < numAllpasses; i++) {
            int sizeL = static_cast<int>(allpassTuning[i] * srScale);
            int sizeR = static_cast<int>((allpassTuning[i] + stereoSpread) * srScale);
            
            allpassBufferL[i].resize(sizeL);
            allpassBufferR[i].resize(sizeR);
            
            allpassL[i].setBuffer(allpassBufferL[i].data(), sizeL);
            allpassR[i].setBuffer(allpassBufferR[i].data(), sizeR);
            allpassL[i].setFeedback(0.5f);
            allpassR[i].setFeedback(0.5f);
        }
        
        // Initialize predelay
        int maxPredelay = static_cast<int>(0.2f * sr);
        predelayBufferL.resize(maxPredelay);
        predelayBufferR.resize(maxPredelay);
        
        updateInternalParameters();
        reset();
    }
    
    void reset() {
        // Clear Freeverb
        for (int i = 0; i < numCombs; i++) {
            combL[i].mute();
            combR[i].mute();
        }
        
        for (int i = 0; i < numAllpasses; i++) {
            allpassL[i].mute();
            allpassR[i].mute();
        }
        
        // Clear pitch shifters
        pitchShifterL.reset();
        pitchShifterR.reset();
        
        // Clear predelay
        std::fill(predelayBufferL.begin(), predelayBufferL.end(), 0.0f);
        std::fill(predelayBufferR.begin(), predelayBufferR.end(), 0.0f);
        predelayIndex = 0;
        
        // Reset filter states
        lowCutStateL = 0.0f;
        lowCutStateR = 0.0f;
        highCutStateL = 0.0f;
        highCutStateR = 0.0f;
        
        lfoPhase = 0.0f;
    }
    
    void updateInternalParameters() {
        // Map size parameter to room size
        roomSize = (sizeParam * scaleRoom) + offsetRoom;
        
        // Map damping
        damping = dampingParam * scaleDamp;
        
        // Update comb filters
        for (int i = 0; i < numCombs; i++) {
            combL[i].setFeedback(roomSize);
            combR[i].setFeedback(roomSize);
            combL[i].setDamp(damping);
            combR[i].setDamp(damping);
        }
        
        // Pre-delay
        predelaySize = static_cast<int>(predelayParam * 0.1f * sampleRate);
        
        // LFO rate for modulation
        lfoRate = 0.1f + modulationParam * 2.0f; // 0.1 to 2.1 Hz
        
        // Filter coefficients
        float lowCutFreq = 20.0f * std::pow(50.0f, lowCutParam);
        lowCutCoeff = 1.0f - std::exp(-2.0f * M_PI * lowCutFreq / sampleRate);
        
        float highCutFreq = 1000.0f * std::pow(20.0f, highCutParam);
        highCutCoeff = std::exp(-2.0f * M_PI * highCutFreq / sampleRate);
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        // LFO increment
        float lfoInc = 2.0f * M_PI * lfoRate / sampleRate;

        // Calculate pitch ratio (1.0 = no shift, 2.0 = octave up)
        // Use different ratios for L/R to create stereo width
        // Increased detuning from 1% to 3% for better stereo separation
        float pitchRatioBase = 1.0f + pitchShiftParam; // 1.0 to 2.0
        float pitchRatioL = pitchRatioBase * 0.97f;  // 3% lower for left
        float pitchRatioR = pitchRatioBase * 1.03f;  // 3% higher for right

        for (int i = 0; i < numSamples; i++) {
            float inputL = leftData[i];
            float inputR = rightData ? rightData[i] : inputL;
            
            // Apply pre-delay
            float delayedL = inputL;
            float delayedR = inputR;

            if (predelaySize > 0) {
                // Write current input to buffer first
                predelayBufferL[predelayIndex] = inputL;
                predelayBufferR[predelayIndex] = inputR;

                // Calculate read index (predelaySize samples ago, wrapped)
                int readIndex = predelayIndex - predelaySize;
                if (readIndex < 0) {
                    readIndex += static_cast<int>(predelayBufferL.size());
                }

                // Read delayed signal
                delayedL = predelayBufferL[readIndex];
                delayedR = predelayBufferR[readIndex];

                if (++predelayIndex >= static_cast<int>(predelayBufferL.size())) {
                    predelayIndex = 0;
                }
            }
            
            // Process through Freeverb
            float reverbL = 0.0f;
            float reverbR = 0.0f;

            // Add cross-channel mixing for stereo decorrelation
            // Increased from 0.15 to 0.35 for better stereo width
            float crossMix = 0.35f;
            float delayedL_mixed = delayedL * (1.0f - crossMix) + delayedR * crossMix;
            float delayedR_mixed = delayedR * (1.0f - crossMix) + delayedL * crossMix;

            // Accumulate comb filters with mixed inputs
            for (int j = 0; j < numCombs; j++) {
                reverbL += combL[j].process(delayedL_mixed);
                reverbR += combR[j].process(delayedR_mixed);
            }
            
            // Process through allpasses
            for (int j = 0; j < numAllpasses; j++) {
                reverbL = allpassL[j].process(reverbL);
                reverbR = allpassR[j].process(reverbR);
            }
            
            // Apply gain correction
            reverbL *= gain;
            reverbR *= gain;
            
            // Apply pitch shifting for shimmer effect
            float shimmerL = reverbL;
            float shimmerR = reverbR;
            
            if (shimmerParam > 0.01f) {
                // Pitch shift the reverb signal with different ratios for L/R
                float shiftedL = pitchShifterL.process(reverbL, pitchRatioL);
                float shiftedR = pitchShifterR.process(reverbR, pitchRatioR);

                // Mix pitched and unpitched based on shimmer amount
                shimmerL = reverbL * (1.0f - shimmerParam) + shiftedL * shimmerParam;
                shimmerR = reverbR * (1.0f - shimmerParam) + shiftedR * shimmerParam;
                
                // Add feedback for sustained shimmer
                if (feedbackParam > 0.01f) {
                    // Feed shimmer back into the reverb (carefully to avoid runaway)
                    float fbAmount = feedbackParam * 0.3f; // Max 30% feedback
                    delayedL += shimmerL * fbAmount;
                    delayedR += shimmerR * fbAmount;
                }
            }
            
            // Apply modulation if enabled
            if (modulationParam > 0.01f) {
                // Increased modulation depth from 0.002 to 0.005 for better stereo width
                float mod = std::sin(lfoPhase) * modulationParam * 0.005f;
                lfoPhase += lfoInc;
                if (lfoPhase > 2.0f * M_PI) lfoPhase -= 2.0f * M_PI;

                // Enhanced stereo modulation - use opposite phases for L/R
                shimmerL *= (1.0f + mod);
                shimmerR *= (1.0f - mod);

                // Additional phase-offset modulation for wider stereo image
                // Increased from 0.001 to 0.003 for more decorrelation
                float mod2 = std::cos(lfoPhase) * modulationParam * 0.003f;
                shimmerL += shimmerR * mod2;
                shimmerR += shimmerL * (-mod2);
            }
            
            // Apply filters
            if (lowCutParam > 0.001f) {
                lowCutStateL += (shimmerL - lowCutStateL) * lowCutCoeff;
                shimmerL = shimmerL - lowCutStateL;
                
                lowCutStateR += (shimmerR - lowCutStateR) * lowCutCoeff;
                shimmerR = shimmerR - lowCutStateR;
            }
            
            if (highCutParam < 0.999f) {
                highCutStateL = shimmerL * (1.0f - highCutCoeff) + highCutStateL * highCutCoeff;
                shimmerL = highCutStateL;
                
                highCutStateR = shimmerR * (1.0f - highCutCoeff) + highCutStateR * highCutCoeff;
                shimmerR = highCutStateR;
            }
            
            // Mix dry and wet
            float wetGain = mixParam;
            float dryGain = 1.0f - mixParam;
            
            leftData[i] = inputL * dryGain + shimmerL * wetGain;
            if (rightData) {
                rightData[i] = inputR * dryGain + shimmerR * wetGain;
            }
        }
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: mixParam = value; break;
            case 1: pitchShiftParam = value; break;
            case 2: shimmerParam = value; break;
            case 3: sizeParam = value; break;
            case 4: dampingParam = value; break;
            case 5: feedbackParam = value; break;
            case 6: predelayParam = value; break;
            case 7: modulationParam = value; break;
            case 8: lowCutParam = value; break;
            case 9: highCutParam = value; break;
        }
        
        updateInternalParameters();
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
        if (index < 10) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String ShimmerReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Mix";
        case 1: return "Pitch Shift";
        case 2: return "Shimmer";
        case 3: return "Size";
        case 4: return "Damping";
        case 5: return "Feedback";
        case 6: return "Pre-Delay";
        case 7: return "Modulation";
        case 8: return "Low Cut";
        case 9: return "High Cut";
        default: return "";
    }
}

int ShimmerReverb::getNumParameters() const {
    return 10;
}

juce::String ShimmerReverb::getName() const {
    return "Shimmer Reverb";
}