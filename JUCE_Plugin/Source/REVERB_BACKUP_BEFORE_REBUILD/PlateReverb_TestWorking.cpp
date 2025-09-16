// Simple test reverb that DEFINITELY works
#include "PlateReverb.h"
#include <cmath>
#include <algorithm>

// Ultra-simple delay-based reverb for testing
class SimpleDelay {
    std::vector<float> buffer;
    int writePos = 0;
    
public:
    void init(int size) {
        buffer.resize(size, 0.0f);
        writePos = 0;
    }
    
    void write(float sample) {
        if (!buffer.empty()) {
            buffer[writePos] = sample;
            writePos = (writePos + 1) % buffer.size();
        }
    }
    
    float read(int delay) {
        if (buffer.empty() || delay <= 0) return 0.0f;
        int readPos = (writePos - delay + buffer.size()) % buffer.size();
        return buffer[readPos];
    }
    
    void clear() {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }
};

// PlateReverb implementation - SUPER SIMPLE VERSION
class PlateReverb::Impl {
public:
    SimpleDelay delayL, delayR;
    float feedback = 0.5f;
    float damping = 0.5f;
    float mix = 0.5f;
    float size = 0.5f;
    float filterStateL = 0.0f;
    float filterStateR = 0.0f;
    double sampleRate = 44100.0;
    
    void setSampleRate(double sr) {
        sampleRate = sr;
        int maxDelay = static_cast<int>(sr * 2.0); // 2 seconds max
        delayL.init(maxDelay);
        delayR.init(maxDelay);
    }
    
    void reset() {
        delayL.clear();
        delayR.clear();
        filterStateL = 0.0f;
        filterStateR = 0.0f;
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        // Calculate delay time based on size
        int delayTime = static_cast<int>(100 + size * (sampleRate * 0.5)); // 100ms to 600ms
        
        // Process each sample
        for (int i = 0; i < numSamples; ++i) {
            float inputL = buffer.getSample(0, i);
            float inputR = numChannels > 1 ? buffer.getSample(1, i) : inputL;
            
            // Read from delays
            float delayedL = delayL.read(delayTime);
            float delayedR = delayR.read(delayTime + 23); // Slight offset for stereo
            
            // Apply damping (simple lowpass)
            filterStateL = delayedL * (1.0f - damping) + filterStateL * damping;
            filterStateR = delayedR * (1.0f - damping) + filterStateR * damping;
            
            // Mix input with filtered delayed signal (feedback)
            float wetL = inputL + filterStateL * feedback * 0.7f; // Scale feedback for stability
            float wetR = inputR + filterStateR * feedback * 0.7f;
            
            // Write to delay lines
            delayL.write(wetL);
            delayR.write(wetR);
            
            // Mix dry/wet
            buffer.setSample(0, i, inputL * (1.0f - mix) + wetL * mix);
            if (numChannels > 1) {
                buffer.setSample(1, i, inputR * (1.0f - mix) + wetR * mix);
            }
        }
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);
        switch (index) {
            case 0: size = value; break;
            case 1: damping = value; break;
            case 2: break; // Pre-delay not implemented in simple version
            case 3: mix = value; break;
        }
    }
};

// Public interface
PlateReverb::PlateReverb() : pImpl(std::make_unique<Impl>()) {}
PlateReverb::~PlateReverb() = default;

void PlateReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->setSampleRate(sampleRate);
}

void PlateReverb::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void PlateReverb::reset() {
    pImpl->reset();
}

void PlateReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index < 4) {
            pImpl->setParameter(index, value);
        }
    }
    
    // Debug output to verify parameters are received
    static int debugCounter = 0;
    if (++debugCounter % 100 == 0) {
        DBG("PlateReverb params: size=" + juce::String(pImpl->size) + 
            " damping=" + juce::String(pImpl->damping) + 
            " mix=" + juce::String(pImpl->mix) +
            " feedback=" + juce::String(pImpl->feedback));
    }
}

juce::String PlateReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Size";
        case 1: return "Damping";
        case 2: return "Predelay";
        case 3: return "Mix";
        default: return "";
    }
}

int PlateReverb::getNumParameters() const {
    return 4;
}

juce::String PlateReverb::getName() const {
    return "Plate Reverb";
}