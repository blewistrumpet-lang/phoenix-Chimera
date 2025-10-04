// SpringReverb_Proven.cpp - Integration of proven spring reverb algorithm
// Based on established DSP techniques for spring reverb simulation
// Uses allpass filters with modulation to simulate spring characteristics

#include "SpringReverb.h"
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

namespace {
    // Spring reverb characteristics
    const int numSprings = 3;
    const int allpassesPerSpring = 4;
    const float springDecay = 0.95f;
    const float springDiffusion = 0.7f;
    
    // Delay times for spring simulation (in ms at 44100Hz)
    const float springDelays[numSprings] = { 37.0f, 43.0f, 51.0f };
    const float allpassDelays[allpassesPerSpring] = { 4.3f, 7.7f, 11.3f, 13.7f };
}

// Simple allpass filter for spring simulation
class SpringAllpass {
public:
    void init(int size) {
        buffer.resize(size, 0.0f);
        bufferSize = size;
        writePos = 0;
    }
    
    void reset() {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }
    
    float process(float input, float feedback) {
        if (bufferSize == 0) return input;
        
        float delayed = buffer[writePos];
        float output = -input + delayed;
        buffer[writePos] = input + delayed * feedback;
        
        writePos = (writePos + 1) % bufferSize;
        return output;
    }
    
private:
    std::vector<float> buffer;
    int bufferSize = 0;
    int writePos = 0;
};

// Spring tank simulation
class SpringTank {
public:
    SpringAllpass allpasses[allpassesPerSpring];
    std::vector<float> delayBuffer;
    int delaySize = 0;
    int writePos = 0;
    float feedback = 0.9f;
    float damping = 0.3f;
    float dampState = 0.0f;
    
    void init(float delayMs, double sampleRate) {
        delaySize = static_cast<int>(delayMs * sampleRate / 1000.0f);
        delayBuffer.resize(delaySize, 0.0f);
        writePos = 0;
        
        // Initialize allpass chain
        for (int i = 0; i < allpassesPerSpring; i++) {
            int apSize = static_cast<int>(allpassDelays[i] * sampleRate / 1000.0f);
            allpasses[i].init(apSize);
        }
    }
    
    void reset() {
        std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
        writePos = 0;
        dampState = 0.0f;
        
        for (int i = 0; i < allpassesPerSpring; i++) {
            allpasses[i].reset();
        }
    }
    
    float process(float input, float chirp = 0.0f) {
        if (delaySize == 0) return input;
        
        // Read from delay with chirp modulation
        int readPos = writePos - delaySize + 1;
        if (chirp != 0.0f) {
            readPos += static_cast<int>(chirp * 10.0f); // Small modulation
        }
        while (readPos < 0) readPos += delaySize;
        readPos = readPos % delaySize;
        
        float delayed = delayBuffer[readPos];
        
        // Apply damping
        dampState = delayed * (1.0f - damping) + dampState * damping;
        
        // Write to delay with feedback
        delayBuffer[writePos] = input + dampState * feedback;
        writePos = (writePos + 1) % delaySize;
        
        // Process through allpass chain for dispersion
        float output = delayed;
        for (int i = 0; i < allpassesPerSpring; i++) {
            output = allpasses[i].process(output, springDiffusion);
        }
        
        return output;
    }
    
    void setFeedback(float fb) {
        feedback = std::clamp(fb, 0.0f, 0.99f);
    }
    
    void setDamping(float damp) {
        damping = std::clamp(damp, 0.0f, 0.9f);
    }
};

// Main SpringReverb implementation
class SpringReverb::Impl {
public:
    // Spring tanks
    SpringTank springs[numSprings];
    
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
    
    // Soft saturation for drive
    float drive = 1.0f;
    
    // LFO for chirp modulation
    float lfoPhase = 0.0f;
    float lfoRate = 0.5f;
    float chirpAmount = 0.0f;
    
    // Parameters
    float mixParam = 0.5f;
    float tensionParam = 0.5f;
    float dampingParam = 0.5f;
    float decayParam = 0.5f;
    float predelayParam = 0.0f;
    float driveParam = 0.0f;
    float chirpParam = 0.0f;
    float lowCutParam = 0.0f;
    float highCutParam = 1.0f;
    float widthParam = 1.0f;
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Initialize spring tanks with different delays
        for (int i = 0; i < numSprings; i++) {
            springs[i].init(springDelays[i] * (1.0f + i * 0.1f), sr);
        }
        
        // Initialize predelay
        int maxPredelay = static_cast<int>(0.2f * sr); // 200ms max
        predelayBufferL.resize(maxPredelay);
        predelayBufferR.resize(maxPredelay);
        
        updateInternalParameters();
        reset();
    }
    
    void reset() {
        for (int i = 0; i < numSprings; i++) {
            springs[i].reset();
        }
        
        std::fill(predelayBufferL.begin(), predelayBufferL.end(), 0.0f);
        std::fill(predelayBufferR.begin(), predelayBufferR.end(), 0.0f);
        predelayIndex = 0;
        
        lowCutStateL = 0.0f;
        lowCutStateR = 0.0f;
        highCutStateL = 0.0f;
        highCutStateR = 0.0f;
        
        lfoPhase = 0.0f;
    }
    
    void updateInternalParameters() {
        // Map parameters to spring characteristics
        
        // Tension affects the delay times (tighter spring = shorter delays)
        float tensionScale = 0.5f + tensionParam * 1.0f; // 0.5x to 1.5x
        
        // Decay controls feedback
        float feedback = 0.7f + decayParam * 0.28f; // 0.7 to 0.98
        
        // Damping controls high frequency loss
        float damping = dampingParam * 0.8f; // 0 to 0.8
        
        // Update spring parameters
        for (int i = 0; i < numSprings; i++) {
            springs[i].setFeedback(feedback);
            springs[i].setDamping(damping);
        }
        
        // Pre-delay
        predelaySize = static_cast<int>(predelayParam * 0.1f * sampleRate); // 0-100ms
        
        // Drive amount
        drive = 1.0f + driveParam * 4.0f; // 1x to 5x
        
        // Chirp (spring modulation)
        chirpAmount = chirpParam * 0.3f; // 0 to 0.3
        lfoRate = 0.3f + chirpParam * 2.0f; // 0.3 to 2.3 Hz
        
        // Filter coefficients
        float lowCutFreq = 20.0f * std::pow(50.0f, lowCutParam); // 20Hz to 1kHz
        lowCutCoeff = 1.0f - std::exp(-2.0f * M_PI * lowCutFreq / sampleRate);
        
        float highCutFreq = 1000.0f * std::pow(20.0f, highCutParam); // 1kHz to 20kHz
        highCutCoeff = std::exp(-2.0f * M_PI * highCutFreq / sampleRate);
    }
    
    float softClip(float x) {
        // Soft saturation
        if (std::abs(x) < 0.5f) {
            return x;
        } else {
            float sign = x < 0 ? -1.0f : 1.0f;
            float abs_x = std::abs(x);
            return sign * (0.5f + 0.5f * std::tanh(2.0f * (abs_x - 0.5f)));
        }
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        // LFO increment
        float lfoInc = 2.0f * M_PI * lfoRate / sampleRate;
        
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
            
            // Apply drive (before reverb for that spring saturation)
            if (driveParam > 0.01f) {
                delayedL = softClip(delayedL * drive) / drive;
                delayedR = softClip(delayedR * drive) / drive;
            }
            
            // Calculate chirp modulation from LFO
            float chirp = std::sin(lfoPhase) * chirpAmount;
            lfoPhase += lfoInc;
            if (lfoPhase > 2.0f * M_PI) lfoPhase -= 2.0f * M_PI;
            
            // Process through spring tanks
            float springOutL = 0.0f;
            float springOutR = 0.0f;
            
            // Mix input to all springs
            float monoInput = (delayedL + delayedR) * 0.5f;
            
            for (int j = 0; j < numSprings; j++) {
                float springOut = springs[j].process(monoInput * 0.5f, chirp * (1.0f + j * 0.3f));
                
                // Distribute springs across stereo field
                float pan = (j - 1.0f) * 0.3f; // -0.3, 0, 0.3
                springOutL += springOut * (1.0f - pan);
                springOutR += springOut * (1.0f + pan);
            }
            
            // Normalize
            springOutL *= 0.5f;
            springOutR *= 0.5f;
            
            // Apply filters
            if (lowCutParam > 0.001f) {
                lowCutStateL += (springOutL - lowCutStateL) * lowCutCoeff;
                springOutL = springOutL - lowCutStateL;
                
                lowCutStateR += (springOutR - lowCutStateR) * lowCutCoeff;
                springOutR = springOutR - lowCutStateR;
            }
            
            if (highCutParam < 0.999f) {
                highCutStateL = springOutL * (1.0f - highCutCoeff) + highCutStateL * highCutCoeff;
                springOutL = highCutStateL;
                
                highCutStateR = springOutR * (1.0f - highCutCoeff) + highCutStateR * highCutCoeff;
                springOutR = highCutStateR;
            }
            
            // Apply stereo width
            if (widthParam < 0.999f) {
                float mid = (springOutL + springOutR) * 0.5f;
                float side = (springOutL - springOutR) * 0.5f * widthParam;
                springOutL = mid + side;
                springOutR = mid - side;
            }
            
            // Mix dry and wet signals
            float wetGain = mixParam;
            float dryGain = 1.0f - mixParam;
            
            leftData[i] = inputL * dryGain + springOutL * wetGain;
            if (rightData) {
                rightData[i] = inputR * dryGain + springOutR * wetGain;
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
        
        updateInternalParameters();
    }
};

// Public interface
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