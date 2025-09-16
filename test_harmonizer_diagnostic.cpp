// Comprehensive diagnostic test for IntelligentHarmonizer
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>
#include <fstream>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 8192;
const float INPUT_FREQ = 440.0f; // A4

// Generate a sine wave
void generateSineWave(float* buffer, int numSamples, float frequency, float sampleRate) {
    for (int i = 0; i < numSamples; ++i) {
        float phase = 2.0f * M_PI * frequency * i / sampleRate;
        buffer[i] = std::sin(phase) * 0.8f;
    }
}

// FFT-based frequency analysis using JUCE
float analyzeFrequencyJUCE(const float* buffer, int numSamples, float sampleRate) {
    const int fftOrder = 12; // 2^12 = 4096
    const int fftSize = 1 << fftOrder;
    
    if (numSamples < fftSize) {
        std::cout << "  [WARNING] Buffer too small for FFT analysis" << std::endl;
        return 0.0f;
    }
    
    juce::dsp::FFT fft(fftOrder);
    
    // Prepare data for real-only FFT (needs 2*fftSize floats)
    std::vector<float> fftData(fftSize * 2, 0.0f);
    
    // Copy input and apply window (only to first fftSize elements)
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / (fftSize - 1)); // Hann window
        fftData[i] = buffer[i] * window;
    }
    
    // Perform FFT
    fft.performRealOnlyForwardTransform(fftData.data(), true);
    
    // Now fftData contains interleaved real/imaginary pairs
    // Find peak in the magnitude spectrum
    float maxMag = 0.0f;
    int peakBin = 0;
    
    // Debug: print spectrum
    std::cout << "  [DEBUG] Top 5 FFT bins:" << std::endl;
    std::vector<std::pair<int, float>> binMags;
    
    // Only process positive frequencies (up to fftSize/2 + 1)
    for (int bin = 1; bin <= fftSize/2; ++bin) {
        float real = fftData[bin * 2];
        float imag = fftData[bin * 2 + 1];
        float mag = std::sqrt(real * real + imag * imag);
        binMags.push_back({bin, mag});
        if (mag > maxMag) {
            maxMag = mag;
            peakBin = bin;
        }
    }
    
    // Sort and show top 5
    std::sort(binMags.begin(), binMags.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    for (int i = 0; i < 5 && i < binMags.size(); ++i) {
        float freq = binMags[i].first * sampleRate / (float)fftSize;
        std::cout << "    Bin " << binMags[i].first << ": " << freq << " Hz (mag=" 
                  << binMags[i].second << ")" << std::endl;
    }
    
    // Quadratic interpolation for better precision
    if (peakBin > 0 && peakBin < fftSize/2 - 1) {
        // Get magnitudes of surrounding bins
        float real1 = fftData[(peakBin - 1) * 2];
        float imag1 = fftData[(peakBin - 1) * 2 + 1];
        float y1 = std::sqrt(real1 * real1 + imag1 * imag1);
        
        float real2 = fftData[peakBin * 2];
        float imag2 = fftData[peakBin * 2 + 1];
        float y2 = std::sqrt(real2 * real2 + imag2 * imag2);
        
        float real3 = fftData[(peakBin + 1) * 2];
        float imag3 = fftData[(peakBin + 1) * 2 + 1];
        float y3 = std::sqrt(real3 * real3 + imag3 * imag3);
        
        if (y1 > 0 && y2 > 0 && y3 > 0 && (y1 - 2.0f * y2 + y3) != 0) {
            float delta = 0.5f * (y1 - y3) / (y1 - 2.0f * y2 + y3);
            float interpolatedBin = peakBin + delta;
            
            return interpolatedBin * sampleRate / fftSize;
        }
    }
    
    return peakBin * sampleRate / fftSize;
}

// Zero-crossing analysis for comparison
float analyzeFrequencyZeroCrossing(const float* buffer, int numSamples, float sampleRate) {
    std::vector<float> crossings;
    
    // Find positive-going zero crossings
    for (int i = 1; i < numSamples - 1; ++i) {
        if (buffer[i-1] <= 0 && buffer[i] > 0) {
            // Linear interpolation for precise crossing
            float frac = -buffer[i-1] / (buffer[i] - buffer[i-1]);
            crossings.push_back(i - 1 + frac);
        }
    }
    
    if (crossings.size() < 2) return 0.0f;
    
    // Calculate average period
    float totalPeriod = 0;
    for (size_t i = 1; i < crossings.size(); ++i) {
        totalPeriod += (crossings[i] - crossings[i-1]);
    }
    
    float avgPeriod = totalPeriod / (crossings.size() - 1);
    return sampleRate / avgPeriod;
}

// Write audio to file for manual inspection
void writeAudioToFile(const float* buffer, int numSamples, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "  [ERROR] Could not open file: " << filename << std::endl;
        return;
    }
    
    for (int i = 0; i < numSamples; ++i) {
        file << i << "," << buffer[i] << std::endl;
    }
    
    file.close();
    std::cout << "  [INFO] Wrote " << numSamples << " samples to " << filename << std::endl;
}

// Test direct SMBPitchShiftFixed
void testDirectPitchShifter() {
    std::cout << "\n=== Testing SMBPitchShiftFixed Directly ===" << std::endl;
    
    SMBPitchShiftFixed shifter;
    // No setSampleRate needed - it processes at any sample rate
    
    // Generate input
    std::vector<float> input(BUFFER_SIZE);
    generateSineWave(input.data(), BUFFER_SIZE, INPUT_FREQ, SAMPLE_RATE);
    
    // Test unison (ratio = 1.0)
    std::cout << "\n[Unison Test]" << std::endl;
    std::vector<float> output(BUFFER_SIZE);
    shifter.process(input.data(), output.data(), BUFFER_SIZE, 1.0f);
    
    float freqFFT = analyzeFrequencyJUCE(output.data() + 2048, 4096, SAMPLE_RATE);
    float freqZC = analyzeFrequencyZeroCrossing(output.data() + 2048, 4096, SAMPLE_RATE);
    
    std::cout << "  Input: " << INPUT_FREQ << " Hz" << std::endl;
    std::cout << "  Output (FFT): " << freqFFT << " Hz (Error: " 
              << std::abs(freqFFT - INPUT_FREQ) / INPUT_FREQ * 100.0f << "%)" << std::endl;
    std::cout << "  Output (ZC): " << freqZC << " Hz (Error: " 
              << std::abs(freqZC - INPUT_FREQ) / INPUT_FREQ * 100.0f << "%)" << std::endl;
    
    // Test octave up (ratio = 2.0)
    std::cout << "\n[Octave Up Test]" << std::endl;
    shifter.process(input.data(), output.data(), BUFFER_SIZE, 2.0f);
    
    float expectedOctave = INPUT_FREQ * 2.0f;
    freqFFT = analyzeFrequencyJUCE(output.data() + 2048, 4096, SAMPLE_RATE);
    freqZC = analyzeFrequencyZeroCrossing(output.data() + 2048, 4096, SAMPLE_RATE);
    
    std::cout << "  Expected: " << expectedOctave << " Hz" << std::endl;
    std::cout << "  Output (FFT): " << freqFFT << " Hz (Error: " 
              << std::abs(freqFFT - expectedOctave) / expectedOctave * 100.0f << "%)" << std::endl;
    std::cout << "  Output (ZC): " << freqZC << " Hz (Error: " 
              << std::abs(freqZC - expectedOctave) / expectedOctave * 100.0f << "%)" << std::endl;
    
    // Test perfect fifth (ratio = 1.5)
    std::cout << "\n[Perfect Fifth Test]" << std::endl;
    shifter.process(input.data(), output.data(), BUFFER_SIZE, 1.5f);
    
    float expectedFifth = INPUT_FREQ * 1.5f;
    freqFFT = analyzeFrequencyJUCE(output.data() + 2048, 4096, SAMPLE_RATE);
    freqZC = analyzeFrequencyZeroCrossing(output.data() + 2048, 4096, SAMPLE_RATE);
    
    std::cout << "  Expected: " << expectedFifth << " Hz" << std::endl;
    std::cout << "  Output (FFT): " << freqFFT << " Hz (Error: " 
              << std::abs(freqFFT - expectedFifth) / expectedFifth * 100.0f << "%)" << std::endl;
    std::cout << "  Output (ZC): " << freqZC << " Hz (Error: " 
              << std::abs(freqZC - expectedFifth) / expectedFifth * 100.0f << "%)" << std::endl;
}

// Test IntelligentHarmonizer parameter flow
void testHarmonizerParameterFlow() {
    std::cout << "\n=== Testing IntelligentHarmonizer Parameter Flow ===" << std::endl;
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test 1: Custom mode with explicit intervals
    std::cout << "\n[Test 1: Custom Mode - Unison]" << std::endl;
    
    std::map<int, float> params;
    params[0] = 0.0f;      // 1 voice
    params[1] = 1.0f;      // Custom chord mode
    params[2] = 0.0f;      // Root Key C
    params[3] = 1.0f;      // Chromatic scale
    params[4] = 1.0f;      // 100% wet
    
    // Check what parameters are being set
    for (const auto& [key, value] : params) {
        std::cout << "  Param[" << key << "] = " << value << std::endl;
    }
    
    harmonizer.updateParameters(params);
    harmonizer.reset();
    
    // Generate and process
    std::vector<float> input(BUFFER_SIZE);
    generateSineWave(input.data(), BUFFER_SIZE, INPUT_FREQ, SAMPLE_RATE);
    
    juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
    
    // Process multiple times to stabilize
    for (int pass = 0; pass < 5; ++pass) {
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, input[i]);
        }
        harmonizer.process(buffer);
    }
    
    // Extract output
    std::vector<float> output(BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        output[i] = buffer.getSample(0, i);
    }
    
    // Analyze
    float freqFFT = analyzeFrequencyJUCE(output.data() + 2048, 4096, SAMPLE_RATE);
    float freqZC = analyzeFrequencyZeroCrossing(output.data() + 2048, 4096, SAMPLE_RATE);
    
    std::cout << "  Input: " << INPUT_FREQ << " Hz" << std::endl;
    std::cout << "  Output (FFT): " << freqFFT << " Hz" << std::endl;
    std::cout << "  Output (ZC): " << freqZC << " Hz" << std::endl;
    
    // Check output amplitude
    float rms = 0.0f;
    float peak = 0.0f;
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        rms += output[i] * output[i];
        peak = std::max(peak, std::abs(output[i]));
    }
    rms = std::sqrt(rms / BUFFER_SIZE);
    
    std::cout << "  Output RMS: " << rms << std::endl;
    std::cout << "  Output Peak: " << peak << std::endl;
    
    // Write to file for inspection
    writeAudioToFile(input.data(), 1000, "harmonizer_input.csv");
    writeAudioToFile(output.data(), 1000, "harmonizer_output.csv");
    
    // Test 2: Major triad
    std::cout << "\n[Test 2: Major Triad]" << std::endl;
    
    params[0] = 1.0f;      // 3 voices
    params[1] = 0.0f;      // Major triad
    
    harmonizer.updateParameters(params);
    harmonizer.reset();
    
    // Process
    for (int pass = 0; pass < 5; ++pass) {
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, input[i]);
        }
        harmonizer.process(buffer);
    }
    
    // Extract and analyze
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        output[i] = buffer.getSample(0, i);
    }
    
    std::cout << "  Expected intervals: 0, 4, 7 semitones" << std::endl;
    std::cout << "  Expected frequencies: 440, 554.4, 659.3 Hz" << std::endl;
    
    // Perform spectral analysis
    const int fftOrder = 12;
    const int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft(fftOrder);
    std::vector<float> fftData(fftSize * 2, 0.0f);
    
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / (fftSize - 1));
        fftData[i] = output[i + 2048] * window;
    }
    
    fft.performRealOnlyForwardTransform(fftData.data(), true);
    
    // Find top peaks
    std::vector<std::pair<float, float>> peaks; // frequency, magnitude
    
    for (int bin = 10; bin < fftSize/2 - 10; ++bin) {
        float real = fftData[bin * 2];
        float imag = fftData[bin * 2 + 1];
        float mag = std::sqrt(real * real + imag * imag);
        
        // Check if local maximum
        bool isPeak = true;
        for (int j = -5; j <= 5; ++j) {
            if (j != 0) {
                float r = fftData[(bin + j) * 2];
                float i = fftData[(bin + j) * 2 + 1];
                float m = std::sqrt(r * r + i * i);
                if (m > mag) {
                    isPeak = false;
                    break;
                }
            }
        }
        
        if (isPeak) {
            float freq = bin * SAMPLE_RATE / (float)fftSize;
            peaks.push_back({freq, mag});
        }
    }
    
    // Sort by magnitude
    std::sort(peaks.begin(), peaks.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::cout << "  Top frequency peaks:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), peaks.size()); ++i) {
        std::cout << "    " << peaks[i].first << " Hz (mag: " << peaks[i].second << ")" << std::endl;
    }
}

// Test parameter mapping
void testParameterMapping() {
    std::cout << "\n=== Testing Parameter Value Mapping ===" << std::endl;
    
    IntelligentHarmonizer harmonizer;
    
    // Test chord parameter values
    std::vector<std::pair<float, std::string>> chordTests = {
        {0.0f, "Major Triad"},
        {0.083f, "Minor Triad"},
        {0.167f, "Diminished Triad"},
        {0.25f, "Augmented Triad"},
        {0.333f, "Major 7th"},
        {0.417f, "Minor 7th"},
        {0.5f, "Dominant 7th"},
        {0.583f, "Half-Diminished 7th"},
        {0.667f, "Diminished 7th"},
        {0.75f, "Sus2"},
        {0.833f, "Sus4"},
        {0.917f, "Add9"},
        {1.0f, "Custom"}
    };
    
    for (const auto& [value, name] : chordTests) {
        std::cout << "  Chord param " << std::fixed << std::setprecision(3) << value 
                  << " -> " << name << std::endl;
        
        // Get display string
        std::string display = harmonizer.getParameterDisplayString(1, value).toStdString();
        std::cout << "    Display: " << display << std::endl;
    }
}

int main() {
    std::cout << "=== COMPREHENSIVE HARMONIZER DIAGNOSTIC ===" << std::endl;
    std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz" << std::endl;
    std::cout << "Buffer Size: " << BUFFER_SIZE << " samples" << std::endl;
    std::cout << "Input Frequency: " << INPUT_FREQ << " Hz" << std::endl;
    
    // Test 1: Direct pitch shifter
    testDirectPitchShifter();
    
    // Test 2: Harmonizer parameter flow
    testHarmonizerParameterFlow();
    
    // Test 3: Parameter mapping
    testParameterMapping();
    
    std::cout << "\n=== DIAGNOSTIC COMPLETE ===" << std::endl;
    
    return 0;
}