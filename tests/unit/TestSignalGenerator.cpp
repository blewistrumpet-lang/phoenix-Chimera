#include "TestSignalGenerator.h"
#include <random>

juce::AudioBuffer<float> TestSignalGenerator::generateSineWave(float frequency, float duration, float sampleRate, float amplitude) {
    int numSamples = static_cast<int>(duration * sampleRate);
    juce::AudioBuffer<float> buffer(2, numSamples);
    
    const float omega = 2.0f * M_PI * frequency / sampleRate;
    
    for (int channel = 0; channel < 2; ++channel) {
        float* data = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i) {
            data[i] = amplitude * std::sin(omega * i);
        }
    }
    
    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generateWhiteNoise(float duration, float sampleRate, float amplitude) {
    int numSamples = static_cast<int>(duration * sampleRate);
    juce::AudioBuffer<float> buffer(2, numSamples);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    for (int channel = 0; channel < 2; ++channel) {
        float* data = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i) {
            data[i] = amplitude * dist(gen);
        }
    }
    
    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generatePinkNoise(float duration, float sampleRate, float amplitude) {
    int numSamples = static_cast<int>(duration * sampleRate);
    juce::AudioBuffer<float> buffer(2, numSamples);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    PinkNoiseFilter filter;
    
    for (int channel = 0; channel < 2; ++channel) {
        float* data = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i) {
            float white = dist(gen);
            data[i] = amplitude * filter.process(white);
        }
    }
    
    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generateImpulse(float sampleRate, float amplitude) {
    int numSamples = static_cast<int>(0.1f * sampleRate); // 100ms buffer
    juce::AudioBuffer<float> buffer(2, numSamples);
    buffer.clear();
    
    // Single sample impulse at the beginning
    for (int channel = 0; channel < 2; ++channel) {
        buffer.setSample(channel, 0, amplitude);
    }
    
    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generateSweep(float startFreq, float endFreq, float duration, float sampleRate, float amplitude) {
    int numSamples = static_cast<int>(duration * sampleRate);
    juce::AudioBuffer<float> buffer(2, numSamples);
    
    // Logarithmic sweep
    const float logStart = std::log(startFreq);
    const float logEnd = std::log(endFreq);
    float phase = 0.0f;
    
    for (int channel = 0; channel < 2; ++channel) {
        float* data = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / numSamples;
            float currentFreq = std::exp(logStart + t * (logEnd - logStart));
            phase += 2.0f * M_PI * currentFreq / sampleRate;
            data[i] = amplitude * std::sin(phase);
        }
        phase = 0.0f; // Reset phase for next channel
    }
    
    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generateSquareWave(float frequency, float duration, float sampleRate, float amplitude) {
    int numSamples = static_cast<int>(duration * sampleRate);
    juce::AudioBuffer<float> buffer(2, numSamples);
    
    const float period = sampleRate / frequency;
    const float halfPeriod = period / 2.0f;
    
    for (int channel = 0; channel < 2; ++channel) {
        float* data = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i) {
            float phase = std::fmod(static_cast<float>(i), period);
            data[i] = amplitude * (phase < halfPeriod ? 1.0f : -1.0f);
        }
    }
    
    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generateSilence(float duration, float sampleRate) {
    int numSamples = static_cast<int>(duration * sampleRate);
    juce::AudioBuffer<float> buffer(2, numSamples);
    buffer.clear();
    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generateDrumHit(float sampleRate) {
    float duration = 0.5f; // 500ms
    int numSamples = static_cast<int>(duration * sampleRate);
    juce::AudioBuffer<float> buffer(2, numSamples);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    for (int channel = 0; channel < 2; ++channel) {
        float* data = buffer.getWritePointer(channel);
        
        // Initial transient (10ms)
        int transientSamples = static_cast<int>(0.01f * sampleRate);
        for (int i = 0; i < transientSamples; ++i) {
            data[i] = dist(gen) * 0.9f;
        }
        
        // Pitched component (kick fundamental around 60Hz)
        float pitch = 60.0f;
        float pitchDecay = 0.998f;
        
        // Envelope
        float env = 1.0f;
        float envDecay = 0.9995f;
        
        for (int i = transientSamples; i < numSamples; ++i) {
            float sine = std::sin(2.0f * M_PI * pitch * i / sampleRate);
            data[i] = sine * env * 0.7f + dist(gen) * env * 0.1f; // Mix tone and noise
            
            env *= envDecay;
            pitch *= pitchDecay; // Pitch drops slightly
            if (pitch < 40.0f) pitch = 40.0f; // Limit pitch drop
        }
    }
    
    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generateChord(float fundamentalFreq, float duration, float sampleRate) {
    int numSamples = static_cast<int>(duration * sampleRate);
    juce::AudioBuffer<float> buffer(2, numSamples);
    buffer.clear();
    
    // Major triad: root, major third, perfect fifth
    float frequencies[3] = {
        fundamentalFreq,           // Root
        fundamentalFreq * 1.25f,   // Major third (5:4 ratio)
        fundamentalFreq * 1.5f     // Perfect fifth (3:2 ratio)
    };
    
    float amplitudes[3] = { 0.4f, 0.3f, 0.3f }; // Balance the chord
    
    for (int channel = 0; channel < 2; ++channel) {
        float* data = buffer.getWritePointer(channel);
        
        for (int freq = 0; freq < 3; ++freq) {
            float omega = 2.0f * M_PI * frequencies[freq] / sampleRate;
            for (int i = 0; i < numSamples; ++i) {
                data[i] += amplitudes[freq] * std::sin(omega * i);
            }
        }
    }
    
    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generateBurst(float onTime, float offTime, float totalDuration, float sampleRate) {
    int numSamples = static_cast<int>(totalDuration * sampleRate);
    juce::AudioBuffer<float> buffer(2, numSamples);
    
    int onSamples = static_cast<int>(onTime * sampleRate);
    int offSamples = static_cast<int>(offTime * sampleRate);
    int periodSamples = onSamples + offSamples;
    
    float frequency = 1000.0f; // 1kHz test tone
    float omega = 2.0f * M_PI * frequency / sampleRate;
    
    for (int channel = 0; channel < 2; ++channel) {
        float* data = buffer.getWritePointer(channel);
        
        for (int i = 0; i < numSamples; ++i) {
            int positionInPeriod = i % periodSamples;
            if (positionInPeriod < onSamples) {
                // Tone is on
                data[i] = 0.5f * std::sin(omega * i);
            } else {
                // Tone is off
                data[i] = 0.0f;
            }
        }
    }
    
    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generateTwoTone(float freq1, float freq2, float duration, float sampleRate) {
    int numSamples = static_cast<int>(duration * sampleRate);
    juce::AudioBuffer<float> buffer(2, numSamples);
    
    float omega1 = 2.0f * M_PI * freq1 / sampleRate;
    float omega2 = 2.0f * M_PI * freq2 / sampleRate;
    
    for (int channel = 0; channel < 2; ++channel) {
        float* data = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i) {
            data[i] = 0.35f * (std::sin(omega1 * i) + std::sin(omega2 * i));
        }
    }
    
    return buffer;
}

void TestSignalGenerator::scaleSignal(juce::AudioBuffer<float>& buffer, float scale) {
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        buffer.applyGain(channel, 0, buffer.getNumSamples(), scale);
    }
}

void TestSignalGenerator::normalizeSignal(juce::AudioBuffer<float>& buffer) {
    float maxValue = 0.0f;
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float channelMax = buffer.findMinMax(channel, 0, buffer.getNumSamples()).getEnd();
        maxValue = std::max(maxValue, std::abs(channelMax));
    }
    
    if (maxValue > 0.0f) {
        float scale = 0.95f / maxValue; // Normalize to -0.95 to avoid clipping
        scaleSignal(buffer, scale);
    }
}