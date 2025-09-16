#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <complex>
#include <numeric>
#include "JUCE_Plugin/Source/ShimmerReverb.h"

// FFT for frequency analysis
class SimpleFFT {
public:
    static std::vector<float> getMagnitudeSpectrum(const std::vector<float>& signal) {
        int N = signal.size();
        std::vector<std::complex<float>> fft(N);
        
        // Simple DFT (not efficient but accurate for testing)
        for (int k = 0; k < N/2; ++k) {
            std::complex<float> sum(0, 0);
            for (int n = 0; n < N; ++n) {
                float angle = -2.0f * M_PI * k * n / N;
                sum += signal[n] * std::complex<float>(cos(angle), sin(angle));
            }
            fft[k] = sum;
        }
        
        std::vector<float> magnitude(N/2);
        for (int i = 0; i < N/2; ++i) {
            magnitude[i] = std::abs(fft[i]) / N;
        }
        return magnitude;
    }
    
    static float findPeakFrequency(const std::vector<float>& signal, float sampleRate) {
        auto spectrum = getMagnitudeSpectrum(signal);
        int peakBin = std::max_element(spectrum.begin() + 10, spectrum.end()) - spectrum.begin();
        return peakBin * sampleRate / signal.size();
    }
};

void testShimmerPitch() {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     SHIMMER REVERB PITCH SHIFT VALIDATION TEST          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";
    
    ShimmerReverb shimmer;
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    
    shimmer.prepareToPlay(sampleRate, blockSize);
    shimmer.reset();
    
    // Test different pitch shift amounts
    struct PitchTest {
        float pitchParam;  // 0.0 to 1.0
        float expectedRatio;
        std::string description;
    };
    
    std::vector<PitchTest> tests = {
        {0.0f,  0.5f,   "Down 1 octave (-12 semitones)"},
        {0.25f, 0.707f, "Down 6 semitones (tritone)"},
        {0.5f,  1.0f,   "No shift (unison)"},
        {0.75f, 1.414f, "Up 6 semitones (tritone)"},
        {1.0f,  2.0f,   "Up 1 octave (+12 semitones)"},
        {0.583f, 1.122f, "Up 2 semitones (major second)"},
        {0.417f, 0.891f, "Down 2 semitones"}
    };
    
    for (const auto& test : tests) {
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "Test: " << test.description << "\n";
        std::cout << "  Pitch parameter: " << test.pitchParam << "\n";
        std::cout << "  Expected ratio: " << test.expectedRatio << "\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        
        // Reset reverb for each test
        shimmer.reset();
        
        // Set parameters: shimmer ON, specific pitch, full wet
        std::map<int, float> params;
        params[0] = 0.7f;  // Size/Room
        params[1] = 1.0f;  // Shimmer amount (full)
        params[2] = test.pitchParam;  // Pitch
        params[3] = 0.3f;  // Damping
        params[4] = 0.5f;  // Diffusion
        params[5] = 0.3f;  // Modulation
        params[6] = 0.0f;  // Predelay
        params[7] = 0.5f;  // Width
        params[8] = 0.0f;  // Freeze
        params[9] = 1.0f;  // Mix (full wet)
        shimmer.updateParameters(params);
        
        // Test with pure sine wave at 440Hz
        float testFreq = 440.0f;
        juce::AudioBuffer<float> buffer(2, blockSize * 20); // Longer buffer for analysis
        
        // Generate test signal
        std::cout << "  1. Testing with " << testFreq << "Hz sine wave\n";
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = 0.5f * std::sin(2.0f * M_PI * testFreq * i / sampleRate);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        
        // Process in blocks
        for (int block = 0; block < 20; ++block) {
            juce::AudioBuffer<float> blockBuffer(2, blockSize);
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    blockBuffer.setSample(ch, i, buffer.getSample(ch, block * blockSize + i));
                }
            }
            shimmer.process(blockBuffer);
            
            // Copy back
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    buffer.setSample(ch, block * blockSize + i, blockBuffer.getSample(ch, i));
                }
            }
        }
        
        // Analyze output frequency content
        std::cout << "  2. Analyzing output frequency content\n";
        
        // Extract middle portion for analysis (avoid transients)
        std::vector<float> analysisBuffer;
        int startSample = blockSize * 5;  // Skip first 5 blocks
        int endSample = blockSize * 15;   // Use middle 10 blocks
        
        for (int i = startSample; i < endSample; ++i) {
            analysisBuffer.push_back(buffer.getSample(0, i));
        }
        
        // Find peak frequencies
        auto spectrum = SimpleFFT::getMagnitudeSpectrum(analysisBuffer);
        float binResolution = sampleRate / analysisBuffer.size();
        
        std::cout << "  3. Frequency analysis results:\n";
        
        // Find peaks in spectrum
        std::vector<std::pair<float, float>> peaks; // frequency, magnitude
        for (size_t i = 10; i < spectrum.size() - 1; ++i) {
            if (spectrum[i] > spectrum[i-1] && spectrum[i] > spectrum[i+1] && spectrum[i] > 0.01f) {
                float freq = i * binResolution;
                peaks.push_back({freq, spectrum[i]});
            }
        }
        
        // Sort by magnitude
        std::sort(peaks.begin(), peaks.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Display top frequencies
        std::cout << "     Top frequencies detected:\n";
        float expectedFreq = testFreq * test.expectedRatio;
        bool foundExpected = false;
        
        for (size_t i = 0; i < std::min(size_t(5), peaks.size()); ++i) {
            std::cout << "       " << std::fixed << std::setprecision(1) 
                     << peaks[i].first << " Hz (magnitude: " 
                     << std::scientific << peaks[i].second << ")";
            
            // Check if this is close to expected
            float error = std::abs(peaks[i].first - expectedFreq) / expectedFreq;
            if (error < 0.05f) {  // Within 5% of expected
                std::cout << " ← EXPECTED PITCH ✓";
                foundExpected = true;
            } else if (std::abs(peaks[i].first - testFreq) < 20.0f) {
                std::cout << " ← Original pitch (reverb)";
            }
            std::cout << "\n";
        }
        
        std::cout << "\n     Expected pitch-shifted frequency: " 
                 << std::fixed << std::setprecision(1) << expectedFreq << " Hz\n";
        
        // Test amplitude stability
        std::cout << "\n  4. Checking amplitude stability:\n";
        float maxSample = 0.0f;
        float avgEnergy = 0.0f;
        int clippedSamples = 0;
        
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = std::abs(buffer.getSample(0, i));
            maxSample = std::max(maxSample, sample);
            avgEnergy += sample * sample;
            if (sample > 0.95f) clippedSamples++;
        }
        avgEnergy = std::sqrt(avgEnergy / buffer.getNumSamples());
        
        std::cout << "     Max amplitude: " << maxSample;
        if (maxSample > 1.0f) {
            std::cout << " ✗ (CLIPPING!)";
        } else if (maxSample > 0.9f) {
            std::cout << " ⚠ (near clipping)";
        } else {
            std::cout << " ✓";
        }
        std::cout << "\n";
        
        std::cout << "     RMS level: " << avgEnergy;
        if (avgEnergy < 0.01f) {
            std::cout << " ✗ (too quiet)";
        } else if (avgEnergy > 0.5f) {
            std::cout << " ⚠ (very loud)";
        } else {
            std::cout << " ✓";
        }
        std::cout << "\n";
        
        // Overall test result
        std::cout << "\n  5. Test Result: ";
        if (foundExpected && maxSample < 1.0f && avgEnergy > 0.01f) {
            std::cout << "✅ PASS - Pitch shift working correctly\n";
        } else if (foundExpected) {
            std::cout << "⚠️ PARTIAL - Pitch detected but amplitude issues\n";
        } else {
            std::cout << "❌ FAIL - Expected pitch not detected\n";
        }
        std::cout << "\n";
    }
    
    // Test shimmer with musical content
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "MUSICAL CONTENT TEST\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    shimmer.reset();
    
    // Set shimmer to octave up with moderate mix
    std::map<int, float> params;
    params[0] = 0.6f;  // Size
    params[1] = 0.7f;  // Shimmer amount
    params[2] = 1.0f;  // Pitch (octave up)
    params[3] = 0.4f;  // Damping
    params[9] = 0.4f;  // Mix (40% wet)
    shimmer.updateParameters(params);
    
    // Create C major chord
    std::cout << "Testing with C major chord (C4, E4, G4):\n";
    float chordFreqs[] = {261.63f, 329.63f, 392.0f};
    juce::AudioBuffer<float> chordBuffer(2, blockSize * 10);
    
    for (int i = 0; i < chordBuffer.getNumSamples(); ++i) {
        float sample = 0.0f;
        for (float freq : chordFreqs) {
            sample += 0.15f * std::sin(2.0f * M_PI * freq * i / sampleRate);
        }
        // Apply envelope
        float envelope = 1.0f;
        if (i < 1000) envelope = i / 1000.0f;  // Fade in
        if (i > chordBuffer.getNumSamples() - 2000) {
            envelope = (chordBuffer.getNumSamples() - i) / 2000.0f;  // Fade out
        }
        sample *= envelope;
        chordBuffer.setSample(0, i, sample);
        chordBuffer.setSample(1, i, sample);
    }
    
    // Process chord
    for (int block = 0; block < 10; ++block) {
        juce::AudioBuffer<float> blockBuffer(2, blockSize);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                blockBuffer.setSample(ch, i, chordBuffer.getSample(ch, block * blockSize + i));
            }
        }
        shimmer.process(blockBuffer);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                chordBuffer.setSample(ch, block * blockSize + i, blockBuffer.getSample(ch, i));
            }
        }
    }
    
    // Check output
    float maxChord = 0.0f;
    float avgChord = 0.0f;
    for (int i = 0; i < chordBuffer.getNumSamples(); ++i) {
        float sample = std::abs(chordBuffer.getSample(0, i));
        maxChord = std::max(maxChord, sample);
        avgChord += sample;
    }
    avgChord /= chordBuffer.getNumSamples();
    
    std::cout << "  Max output: " << maxChord;
    if (maxChord > 1.0f) std::cout << " ✗ (clipping)";
    else if (maxChord < 0.1f) std::cout << " ✗ (too quiet)";
    else std::cout << " ✓";
    std::cout << "\n";
    
    std::cout << "  Average level: " << avgChord;
    if (avgChord > 0.01f && avgChord < 0.3f) std::cout << " ✓";
    else std::cout << " ⚠";
    std::cout << "\n\n";
    
    std::cout << "══════════════════════════════════════════════════════════\n";
    std::cout << "SHIMMER REVERB PITCH SHIFT TEST COMPLETE\n";
    std::cout << "══════════════════════════════════════════════════════════\n";
    std::cout << "\nThe SMBPitchShiftFixed algorithm should:\n";
    std::cout << "  • Accurately shift pitch by the specified ratio\n";
    std::cout << "  • Maintain stable amplitude without clipping\n";
    std::cout << "  • Preserve audio quality with minimal artifacts\n";
    std::cout << "  • Handle both single tones and complex musical content\n\n";
}

int main() {
    testShimmerPitch();
    return 0;
}