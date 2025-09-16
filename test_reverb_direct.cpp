#include <iostream>
#include <memory>
#include <map>
#include <cmath>
#include "JUCE_Plugin/Source/PlateReverb.cpp"
#include "JUCE_Plugin/Source/SpringReverb.cpp" 
#include "JUCE_Plugin/Source/ShimmerReverb.cpp"
#include "JUCE_Plugin/Source/GatedReverb.cpp"
#include "JUCE_Plugin/Source/ConvolutionReverb.cpp"

// Simple audio buffer
struct SimpleBuffer {
    float** data;
    int channels;
    int samples;
    
    SimpleBuffer(int ch, int samp) : channels(ch), samples(samp) {
        data = new float*[channels];
        for (int i = 0; i < channels; i++) {
            data[i] = new float[samples];
            for (int j = 0; j < samples; j++) {
                data[i][j] = 0.0f;
            }
        }
    }
    
    ~SimpleBuffer() {
        for (int i = 0; i < channels; i++) {
            delete[] data[i];
        }
        delete[] data;
    }
    
    void setImpulse() {
        data[0][0] = 1.0f;
        if (channels > 1) data[1][0] = 1.0f;
    }
    
    float getRMS() {
        float sum = 0;
        for (int ch = 0; ch < channels; ch++) {
            for (int i = 0; i < samples; i++) {
                sum += data[ch][i] * data[ch][i];
            }
        }
        return std::sqrt(sum / (channels * samples));
    }
};

namespace juce {
    template<typename T> class AudioBuffer {
        SimpleBuffer* buffer;
    public:
        AudioBuffer(int ch, int samp) : buffer(new SimpleBuffer(ch, samp)) {}
        ~AudioBuffer() { delete buffer; }
        
        int getNumChannels() const { return buffer->channels; }
        int getNumSamples() const { return buffer->samples; }
        T* getWritePointer(int ch) { return buffer->data[ch]; }
        const T* getReadPointer(int ch) const { return buffer->data[ch]; }
        
        void clear() {
            for (int ch = 0; ch < buffer->channels; ch++) {
                for (int i = 0; i < buffer->samples; i++) {
                    buffer->data[ch][i] = 0;
                }
            }
        }
        
        T getRMSLevel(int ch, int start, int num) const {
            T sum = 0;
            for (int i = start; i < start + num && i < buffer->samples; i++) {
                sum += buffer->data[ch][i] * buffer->data[ch][i];
            }
            return std::sqrt(sum / num);
        }
    };
    
    class String {
        std::string str;
    public:
        String() {}
        String(const char* s) : str(s) {}
        const char* toRawUTF8() const { return str.c_str(); }
    };
}

void testReverb(const char* name, EngineBase* reverb) {
    std::cout << "\n=== Testing " << name << " ===\n";
    
    // Initialize
    reverb->prepareToPlay(44100, 512);
    
    // Set to 100% wet
    std::map<int, float> params;
    params[0] = 1.0f;  // Mix = 100% wet
    params[1] = 0.7f;  // Size/Tension/etc
    reverb->updateParameters(params);
    
    // Process impulse
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    buffer.getWritePointer(0)[0] = 1.0f;
    buffer.getWritePointer(1)[0] = 1.0f;
    
    reverb->process(buffer);
    
    float rms = buffer.getRMSLevel(0, 0, 512);
    std::cout << "RMS after impulse: " << rms << "\n";
    
    // Process tail
    float totalEnergy = 0;
    for (int block = 0; block < 10; block++) {
        buffer.clear();
        reverb->process(buffer);
        float blockRMS = buffer.getRMSLevel(0, 0, 512);
        totalEnergy += blockRMS * blockRMS;
        if (block < 3) {
            std::cout << "Block " << block << " RMS: " << blockRMS << "\n";
        }
    }
    
    std::cout << "Total tail energy: " << totalEnergy << "\n";
    
    if (totalEnergy > 0.001f) {
        std::cout << "✓ Reverb tail present\n";
    } else {
        std::cout << "✗ No reverb tail\n";
    }
}

int main() {
    std::cout << "Direct Reverb Testing\n";
    std::cout << "=====================\n";
    
    PlateReverb plate;
    testReverb("PlateReverb", &plate);
    
    SpringReverb spring;
    testReverb("SpringReverb", &spring);
    
    ShimmerReverb shimmer;
    testReverb("ShimmerReverb", &shimmer);
    
    GatedReverb gated;
    testReverb("GatedReverb", &gated);
    
    ConvolutionReverb conv;
    testReverb("ConvolutionReverb", &conv);
    
    return 0;
}
