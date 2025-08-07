#pragma once

// Minimal JUCE compatibility layer for engine testing
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>
#include <chrono>

namespace juce {
    template<typename FloatType>
    struct MathConstants {
        static constexpr FloatType pi = static_cast<FloatType>(3.141592653589793238);
    };
    
    class String {
        std::string str;
    public:
        String() = default;
        String(const char* s) : str(s) {}
        String(const std::string& s) : str(s) {}
        
        const char* toRawUTF8() const { return str.c_str(); }
        std::string toStdString() const { return str; }
    };
    
    template<typename SampleType>
    class AudioBuffer {
        std::vector<std::vector<SampleType>> channels;
        int numChannels_, numSamples_;
        
    public:
        AudioBuffer(int channels, int samples) 
            : channels(channels, std::vector<SampleType>(samples, 0)), 
              numChannels_(channels), numSamples_(samples) {}
        
        int getNumChannels() const { return numChannels_; }
        int getNumSamples() const { return numSamples_; }
        
        SampleType* getWritePointer(int channel) { 
            return channels[channel].data(); 
        }
        const SampleType* getReadPointer(int channel) const { 
            return channels[channel].data(); 
        }
        
        void clear() {
            for (auto& ch : channels) {
                std::fill(ch.begin(), ch.end(), SampleType(0));
            }
        }
        
        void setSample(int channel, int sample, SampleType value) {
            if (channel < numChannels_ && sample < numSamples_) {
                channels[channel][sample] = value;
            }
        }
    };
    
    class Random {
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist;
    public:
        Random() : rng(std::chrono::steady_clock::now().time_since_epoch().count()), 
                   dist(0.0f, 1.0f) {}
        float nextFloat() { return dist(rng); }
    };
    
    class JUCEApplication {
    public:
        virtual ~JUCEApplication() = default;
        virtual const String getApplicationName() = 0;
        virtual const String getApplicationVersion() = 0;
        virtual void initialise(const String&) = 0;
        virtual void shutdown() = 0;
        void quit() { /* stub */ }
    };
}

#define START_JUCE_APPLICATION(AppClass) \
int main(int argc, char* argv[]) { \
    AppClass app; \
    app.initialise(""); \
    return 0; \
}
