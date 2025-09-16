// PlateReverb_Freeverb.cpp - Integration of proven Freeverb algorithm
// Original Freeverb by Jezar at Dreampoint - public domain
// Integrated into EngineBase framework for Chimera Phoenix

#include "PlateReverb.h"
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

// Freeverb constants - these are the magic numbers that make it work
namespace {
    const int numCombs = 8;
    const int numAllpasses = 4;
    const float muted = 0.0f;
    const float fixedGain = 0.015f;
    const float scaleDamp = 0.4f;
    const float scaleRoom = 0.28f;
    const float offsetRoom = 0.7f;
    const float initialRoom = 0.5f;
    const float initialDamp = 0.5f;
    const float initialWet = 1.0f / 3.0f;
    const float initialDry = 0.0f;
    const float initialWidth = 1.0f;
    const float initialMode = 0.0f;
    const float freezeMode = 0.5f;
    const int stereoSpread = 23;
    
    // These values are tuned for 44100Hz - will be scaled for other rates
    const int combTuningL[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    const int combTuningR[8] = { 1116+stereoSpread, 1188+stereoSpread, 1277+stereoSpread, 
                                  1356+stereoSpread, 1422+stereoSpread, 1491+stereoSpread, 
                                  1557+stereoSpread, 1617+stereoSpread };
    const int allpassTuningL[4] = { 556, 441, 341, 225 };
    const int allpassTuningR[4] = { 556+stereoSpread, 441+stereoSpread, 
                                     341+stereoSpread, 225+stereoSpread };
}

// Freeverb Comb filter implementation
class Comb {
public:
    Comb() : bufferSize(0), bufferIndex(0), filterStore(0.0f) {}
    
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
    int bufferSize;
    int bufferIndex;
    float filterStore;
    float damp1 = 0.0f;
    float damp2 = 1.0f;
    float feedback = 0.0f;
};

// Freeverb Allpass filter implementation
class Allpass {
public:
    Allpass() : bufferSize(0), bufferIndex(0) {}
    
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
        
        float output;
        float bufout = buffer[bufferIndex];
        
        output = -input + bufout;
        buffer[bufferIndex] = input + (bufout * feedback);
        
        if (++bufferIndex >= bufferSize) {
            bufferIndex = 0;
        }
        
        return output;
    }
    
private:
    float *buffer = nullptr;
    int bufferSize;
    int bufferIndex;
    float feedback = 0.5f;
};

// Main implementation class encapsulating Freeverb
class PlateReverb::Impl {
public:
    // Freeverb components
    Comb combL[numCombs];
    Comb combR[numCombs];
    Allpass allpassL[numAllpasses];
    Allpass allpassR[numAllpasses];
    
    // Buffers for delay lines
    std::vector<float> combBufferL[numCombs];
    std::vector<float> combBufferR[numCombs];
    std::vector<float> allpassBufferL[numAllpasses];
    std::vector<float> allpassBufferR[numAllpasses];
    
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
    
    // Freeverb parameters
    float gain = fixedGain;
    float roomSize = initialRoom;
    float damping = initialDamp;
    float wetLevel = initialWet;
    float dryLevel = initialDry;
    float width = initialWidth;
    float mode = initialMode;
    
    // Our parameter values
    float mixParam = 0.5f;
    float sizeParam = 0.5f;
    float dampParam = 0.5f;
    float predelayParam = 0.0f;
    float widthParam = 1.0f;
    float freezeParam = 0.0f;
    float lowCutParam = 0.0f;
    float highCutParam = 1.0f;
    float earlyParam = 0.5f;
    float diffusionParam = 0.5f;
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Calculate scaling factor for sample rate
        float srScale = static_cast<float>(sr / 44100.0);
        
        // Initialize comb filters
        for (int i = 0; i < numCombs; i++) {
            int sizeL = static_cast<int>(combTuningL[i] * srScale);
            int sizeR = static_cast<int>(combTuningR[i] * srScale);
            
            combBufferL[i].resize(sizeL);
            combBufferR[i].resize(sizeR);
            
            combL[i].setBuffer(combBufferL[i].data(), sizeL);
            combR[i].setBuffer(combBufferR[i].data(), sizeR);
        }
        
        // Initialize allpass filters
        for (int i = 0; i < numAllpasses; i++) {
            int sizeL = static_cast<int>(allpassTuningL[i] * srScale);
            int sizeR = static_cast<int>(allpassTuningR[i] * srScale);
            
            allpassBufferL[i].resize(sizeL);
            allpassBufferR[i].resize(sizeR);
            
            allpassL[i].setBuffer(allpassBufferL[i].data(), sizeL);
            allpassR[i].setBuffer(allpassBufferR[i].data(), sizeR);
            allpassL[i].setFeedback(0.5f);
            allpassR[i].setFeedback(0.5f);
        }
        
        // Initialize predelay
        int maxPredelay = static_cast<int>(0.2f * sr); // 200ms max
        predelayBufferL.resize(maxPredelay);
        predelayBufferR.resize(maxPredelay);
        
        // Set initial parameters
        updateInternalParameters();
        reset();
    }
    
    void reset() {
        // Clear all delay lines
        for (int i = 0; i < numCombs; i++) {
            combL[i].mute();
            combR[i].mute();
        }
        
        for (int i = 0; i < numAllpasses; i++) {
            allpassL[i].mute();
            allpassR[i].mute();
        }
        
        // Clear predelay
        std::fill(predelayBufferL.begin(), predelayBufferL.end(), 0.0f);
        std::fill(predelayBufferR.begin(), predelayBufferR.end(), 0.0f);
        predelayIndex = 0;
        
        // Reset filter states
        lowCutStateL = 0.0f;
        lowCutStateR = 0.0f;
        highCutStateL = 0.0f;
        highCutStateR = 0.0f;
    }
    
    void updateInternalParameters() {
        // Map our parameters to Freeverb parameters
        wetLevel = mixParam;
        dryLevel = 1.0f - mixParam;
        
        // Room size with freeze capability
        if (freezeParam > 0.5f) {
            roomSize = 1.0f;
            damping = 0.0f;
            wetLevel *= 0.5f; // Reduce wet level in freeze mode
        } else {
            roomSize = (sizeParam * scaleRoom) + offsetRoom;
            damping = dampParam * scaleDamp;
        }
        
        // Update comb filters
        for (int i = 0; i < numCombs; i++) {
            combL[i].setFeedback(roomSize);
            combR[i].setFeedback(roomSize);
            combL[i].setDamp(damping);
            combR[i].setDamp(damping);
        }
        
        // Width parameter affects stereo spread
        width = widthParam;
        
        // Pre-delay
        predelaySize = static_cast<int>(predelayParam * 0.1f * sampleRate); // 0-100ms
        
        // Filter coefficients
        float lowCutFreq = 20.0f * std::pow(50.0f, lowCutParam); // 20Hz to 1kHz
        lowCutCoeff = 1.0f - std::exp(-2.0f * M_PI * lowCutFreq / sampleRate);
        
        float highCutFreq = 1000.0f * std::pow(20.0f, highCutParam); // 1kHz to 20kHz
        highCutCoeff = std::exp(-2.0f * M_PI * highCutFreq / sampleRate);
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        for (int i = 0; i < numSamples; i++) {
            float inputL = leftData[i];
            float inputR = rightData ? rightData[i] : inputL;
            
            // Apply pre-delay
            float delayedL = inputL;
            float delayedR = inputR;
            
            if (predelaySize > 0) {
                delayedL = predelayBufferL[predelayIndex];
                delayedR = predelayBufferR[predelayIndex];
                predelayBufferL[predelayIndex] = inputL;
                predelayBufferR[predelayIndex] = inputR;
                
                if (++predelayIndex >= predelaySize) {
                    predelayIndex = 0;
                }
            }
            
            // The Freeverb algorithm - feed input to all comb filters in parallel
            float outL = 0.0f;
            float outR = 0.0f;
            
            // Accumulate comb filters in parallel
            for (int j = 0; j < numCombs; j++) {
                outL += combL[j].process(delayedL);
                outR += combR[j].process(delayedR);
            }
            
            // Feed through allpasses in series
            for (int j = 0; j < numAllpasses; j++) {
                outL = allpassL[j].process(outL);
                outR = allpassR[j].process(outR);
            }
            
            // Apply gain correction
            outL *= gain;
            outR *= gain;
            
            // Apply filters
            if (lowCutParam > 0.001f) {
                lowCutStateL += (outL - lowCutStateL) * lowCutCoeff;
                outL = outL - lowCutStateL;
                
                lowCutStateR += (outR - lowCutStateR) * lowCutCoeff;
                outR = outR - lowCutStateR;
            }
            
            if (highCutParam < 0.999f) {
                highCutStateL = outL * (1.0f - highCutCoeff) + highCutStateL * highCutCoeff;
                outL = highCutStateL;
                
                highCutStateR = outR * (1.0f - highCutCoeff) + highCutStateR * highCutCoeff;
                outR = highCutStateR;
            }
            
            // Apply stereo width
            float wet1 = outL * width;
            float wet2 = outR * width;
            float crossfeed = 1.0f - width;
            outL = wet1 + wet2 * crossfeed;
            outR = wet2 + wet1 * crossfeed;
            
            // Mix dry and wet signals
            leftData[i] = (inputL * dryLevel) + (outL * wetLevel);
            if (rightData) {
                rightData[i] = (inputR * dryLevel) + (outR * wetLevel);
            }
        }
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: mixParam = value; break;
            case 1: sizeParam = value; break;
            case 2: dampParam = value; break;
            case 3: predelayParam = value; break;
            case 4: widthParam = value; break;
            case 5: freezeParam = value; break;
            case 6: lowCutParam = value; break;
            case 7: highCutParam = value; break;
            case 8: earlyParam = value; break; // Not used in basic Freeverb
            case 9: diffusionParam = value; break; // Maps to allpass feedback
        }
        
        // Update allpass feedback for diffusion parameter
        if (index == 9) {
            float feedback = 0.3f + diffusionParam * 0.4f; // 0.3 to 0.7
            for (int i = 0; i < numAllpasses; i++) {
                allpassL[i].setFeedback(feedback);
                allpassR[i].setFeedback(feedback);
            }
        }
        
        updateInternalParameters();
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
        case 4: return "Width";
        case 5: return "Freeze";
        case 6: return "Low Cut";
        case 7: return "High Cut";
        case 8: return "Early Reflections";
        case 9: return "Diffusion";
        default: return "";
    }
}

int PlateReverb::getNumParameters() const {
    return 10;
}

juce::String PlateReverb::getName() const {
    return "Plate Reverb";
}