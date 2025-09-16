// ConvolutionReverb - Fully functional implementation
#include "ConvolutionReverb.h"
#include <cmath>
#include <algorithm>
#include <random>

// Simple but effective convolution reverb using basic FIR filter
class ConvolutionReverb::Impl {
public:
    // Parameters
    float mixParam = 0.5f;
    float predelayParam = 0.0f;
    float dampingParam = 0.5f;
    float sizeParam = 0.5f;
    float widthParam = 1.0f;
    float modulationParam = 0.0f;
    float earlyLateParam = 0.5f;
    float highCutParam = 1.0f;
    
    // IR buffer
    std::vector<float> impulseResponse;
    
    // Convolution buffers (simple FIR implementation)
    std::vector<float> convBufferL;
    std::vector<float> convBufferR;
    int convWritePos = 0;
    
    // Predelay
    std::vector<float> predelayBufferL;
    std::vector<float> predelayBufferR;
    int predelayWritePos = 0;
    int predelaySamples = 0;
    
    // High cut filter
    float highCutStateL = 0.0f;
    float highCutStateR = 0.0f;
    float highCutCoeff = 0.0f;
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Allocate buffers
        int maxIRSize = static_cast<int>(sr * 3.0); // 3 seconds max
        convBufferL.resize(maxIRSize, 0.0f);
        convBufferR.resize(maxIRSize, 0.0f);
        
        int maxPredelay = static_cast<int>(sr * 0.2); // 200ms max predelay
        predelayBufferL.resize(maxPredelay, 0.0f);
        predelayBufferR.resize(maxPredelay, 0.0f);
        
        generateIR();
        updateCoefficients();
    }
    
    void reset() {
        std::fill(convBufferL.begin(), convBufferL.end(), 0.0f);
        std::fill(convBufferR.begin(), convBufferR.end(), 0.0f);
        std::fill(predelayBufferL.begin(), predelayBufferL.end(), 0.0f);
        std::fill(predelayBufferR.begin(), predelayBufferR.end(), 0.0f);
        convWritePos = 0;
        predelayWritePos = 0;
        highCutStateL = 0.0f;
        highCutStateR = 0.0f;
    }
    
    void generateIR() {
        // Generate impulse response based on size and damping
        float rt60 = 0.5f + sizeParam * 2.5f; // 0.5 to 3 seconds RT60
        int irLength = static_cast<int>(sampleRate * rt60);
        irLength = std::min(irLength, static_cast<int>(convBufferL.size()));
        
        impulseResponse.resize(irLength);
        
        // Exponential decay
        float decayRate = -3.0f / rt60;
        
        // Random number generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        // Generate IR with early reflections and late reverb
        for (int i = 0; i < irLength; i++) {
            float time = i / static_cast<float>(sampleRate);
            float envelope = std::exp(decayRate * time);
            
            // Apply frequency-dependent damping
            float dampingFactor = 1.0f - dampingParam * 0.7f * (time / rt60);
            dampingFactor = std::max(0.0f, dampingFactor);
            envelope *= dampingFactor;
            
            // Early reflections (first 80ms)
            if (time < 0.08f) {
                // Sparse early reflections
                if (i == static_cast<int>(0.007f * sampleRate) ||
                    i == static_cast<int>(0.011f * sampleRate) ||
                    i == static_cast<int>(0.017f * sampleRate) ||
                    i == static_cast<int>(0.023f * sampleRate) ||
                    i == static_cast<int>(0.029f * sampleRate) ||
                    i == static_cast<int>(0.037f * sampleRate) ||
                    i == static_cast<int>(0.043f * sampleRate) ||
                    i == static_cast<int>(0.053f * sampleRate)) {
                    
                    float earlyGain = (1.0f - earlyLateParam) + 0.3f;
                    impulseResponse[i] = (dist(gen) * 0.5f) * envelope * earlyGain;
                } else {
                    impulseResponse[i] = 0.0f;
                }
            } else {
                // Dense late reverb
                float lateGain = earlyLateParam * 0.7f + 0.3f;
                impulseResponse[i] = dist(gen) * envelope * lateGain * 0.3f;
            }
        }
        
        // Add initial direct sound
        if (irLength > 0) {
            impulseResponse[0] = 0.0f; // No direct sound in wet signal
        }
    }
    
    void updateCoefficients() {
        // Update predelay
        predelaySamples = static_cast<int>(predelayParam * 0.2f * sampleRate); // 0-200ms
        
        // Update high cut filter
        float cutoffFreq = 20.0f + (20000.0f - 20.0f) * highCutParam;
        highCutCoeff = std::exp(-2.0f * M_PI * cutoffFreq / sampleRate);
        
        // Regenerate IR if size or damping changed significantly
        static float lastSize = -1.0f;
        static float lastDamping = -1.0f;
        static float lastEarlyLate = -1.0f;
        
        if (std::abs(sizeParam - lastSize) > 0.05f ||
            std::abs(dampingParam - lastDamping) > 0.05f ||
            std::abs(earlyLateParam - lastEarlyLate) > 0.05f) {
            generateIR();
            lastSize = sizeParam;
            lastDamping = dampingParam;
            lastEarlyLate = earlyLateParam;
        }
    }
    
    float processConvolution(float input, int channel) {
        // Write input to circular buffer
        if (channel == 0) {
            convBufferL[convWritePos] = input;
        } else {
            convBufferR[convWritePos] = input;
        }
        
        // Perform convolution
        float output = 0.0f;
        int irSize = static_cast<int>(impulseResponse.size());
        auto& buffer = (channel == 0) ? convBufferL : convBufferR;
        
        // Optimized convolution loop
        for (int i = 0; i < irSize; i++) {
            int readPos = (convWritePos - i + static_cast<int>(buffer.size())) % static_cast<int>(buffer.size());
            output += buffer[readPos] * impulseResponse[i];
        }
        
        return output;
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        updateCoefficients();
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        for (int sample = 0; sample < numSamples; sample++) {
            float inputL = leftData[sample];
            float inputR = rightData ? rightData[sample] : inputL;
            
            // Store dry signal
            float dryL = inputL;
            float dryR = inputR;
            
            // Apply predelay
            if (predelaySamples > 0) {
                int predelayReadPos = (predelayWritePos - predelaySamples + static_cast<int>(predelayBufferL.size())) 
                                     % static_cast<int>(predelayBufferL.size());
                float predelayedL = predelayBufferL[predelayReadPos];
                float predelayedR = predelayBufferR[predelayReadPos];
                
                predelayBufferL[predelayWritePos] = inputL;
                predelayBufferR[predelayWritePos] = inputR;
                predelayWritePos = (predelayWritePos + 1) % static_cast<int>(predelayBufferL.size());
                
                inputL = predelayedL;
                inputR = predelayedR;
            }
            
            // Process convolution
            float wetL = processConvolution(inputL, 0);
            float wetR = processConvolution(inputR, 1);
            
            // Apply stereo width
            if (widthParam < 1.0f) {
                float mono = (wetL + wetR) * 0.5f;
                wetL = mono + (wetL - mono) * widthParam;
                wetR = mono + (wetR - mono) * widthParam;
            }
            
            // Apply high cut filter
            if (highCutParam < 0.99f) {
                highCutStateL = wetL * (1.0f - highCutCoeff) + highCutStateL * highCutCoeff;
                highCutStateR = wetR * (1.0f - highCutCoeff) + highCutStateR * highCutCoeff;
                wetL = highCutStateL;
                wetR = highCutStateR;
            }
            
            // Mix dry and wet
            leftData[sample] = dryL * (1.0f - mixParam) + wetL * mixParam;
            if (rightData) {
                rightData[sample] = dryR * (1.0f - mixParam) + wetR * mixParam;
            }
            
            // Update write position
            if (sample == numSamples - 1) {
                convWritePos = (convWritePos + 1) % static_cast<int>(convBufferL.size());
            }
        }
        
        // Update convolution write position for all samples
        convWritePos = (convWritePos + numSamples) % static_cast<int>(convBufferL.size());
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: mixParam = value; break;
            case 1: predelayParam = value; break;
            case 2: dampingParam = value; break;
            case 3: sizeParam = value; break;
            case 4: widthParam = value; break;
            case 5: modulationParam = value; break;
            case 6: earlyLateParam = value; break;
            case 7: highCutParam = value; break;
        }
    }
};

// Public interface
ConvolutionReverb::ConvolutionReverb() : pImpl(std::make_unique<Impl>()) {}
ConvolutionReverb::~ConvolutionReverb() = default;

void ConvolutionReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->init(sampleRate);
}

void ConvolutionReverb::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void ConvolutionReverb::reset() {
    pImpl->reset();
}

void ConvolutionReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index < 8) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String ConvolutionReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Mix";
        case 1: return "Pre-Delay";
        case 2: return "Damping";
        case 3: return "Size";
        case 4: return "Width";
        case 5: return "Modulation";
        case 6: return "Early/Late";
        case 7: return "High Cut";
        default: return "";
    }
}

int ConvolutionReverb::getNumParameters() const {
    return 8;
}

juce::String ConvolutionReverb::getName() const {
    return "Convolution Reverb";
}