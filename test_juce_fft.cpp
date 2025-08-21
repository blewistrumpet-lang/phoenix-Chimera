#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <vector>
#include <complex>
#include <cmath>

// Test JUCE FFT scaling to understand the correct normalization

int main() {
    const int FFT_ORDER = 12;  // 2^12 = 4096
    const int FFT_SIZE = 1 << FFT_ORDER;
    
    juce::dsp::FFT fft(FFT_ORDER);
    
    std::cout << "=== JUCE FFT SCALING TEST ===\n";
    std::cout << "FFT Size: " << FFT_SIZE << "\n\n";
    
    // Test 1: Unit impulse
    {
        std::vector<std::complex<float>> data(FFT_SIZE);
        data[0] = std::complex<float>(1.0f, 0.0f);
        
        std::cout << "Test 1: Unit Impulse\n";
        std::cout << "Input[0] = " << data[0].real() << "\n";
        
        // Forward FFT
        fft.perform(data.data(), data.data(), false);
        
        float sumAfterFFT = 0;
        for (const auto& c : data) {
            sumAfterFFT += std::abs(c);
        }
        std::cout << "After forward FFT, sum of magnitudes = " << sumAfterFFT << "\n";
        
        // Inverse FFT
        fft.perform(data.data(), data.data(), true);
        
        std::cout << "After inverse FFT, data[0] = " << data[0].real() << "\n";
        std::cout << "Scaling factor needed = " << (1.0f / data[0].real()) << "\n\n";
    }
    
    // Test 2: Sine wave
    {
        std::vector<std::complex<float>> data(FFT_SIZE);
        std::vector<float> original(FFT_SIZE);
        
        // Generate sine wave
        for (int i = 0; i < FFT_SIZE; ++i) {
            original[i] = std::sin(2.0f * M_PI * 10.0f * i / FFT_SIZE);
            data[i] = std::complex<float>(original[i], 0.0f);
        }
        
        float inputRMS = 0;
        for (float s : original) {
            inputRMS += s * s;
        }
        inputRMS = std::sqrt(inputRMS / FFT_SIZE);
        
        std::cout << "Test 2: Sine Wave\n";
        std::cout << "Input RMS = " << inputRMS << "\n";
        
        // Forward FFT
        fft.perform(data.data(), data.data(), false);
        
        // Immediately inverse FFT (passthrough)
        fft.perform(data.data(), data.data(), true);
        
        float outputRMS = 0;
        for (const auto& c : data) {
            outputRMS += c.real() * c.real();
        }
        outputRMS = std::sqrt(outputRMS / FFT_SIZE);
        
        std::cout << "Output RMS (no scaling) = " << outputRMS << "\n";
        std::cout << "Gain = " << (outputRMS / inputRMS) << "\n";
        std::cout << "Scaling needed = " << (inputRMS / outputRMS) << "\n\n";
    }
    
    // Test 3: Window and overlap-add
    {
        const int OVERLAP = 4;
        const int HOP = FFT_SIZE / OVERLAP;
        
        std::cout << "Test 3: Overlap-Add with Hann Window\n";
        std::cout << "Overlap factor = " << OVERLAP << "\n";
        std::cout << "Hop size = " << HOP << "\n";
        
        // Create Hann window
        std::vector<float> window(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            window[i] = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / (FFT_SIZE - 1));
        }
        
        // Calculate window sum for COLA
        std::vector<float> windowSum(FFT_SIZE, 0.0f);
        for (int frame = 0; frame < OVERLAP; ++frame) {
            int offset = frame * HOP;
            for (int i = 0; i < FFT_SIZE; ++i) {
                int idx = (i + offset) % FFT_SIZE;
                windowSum[idx] += window[i] * window[i];
            }
        }
        
        float avgWindowSum = 0;
        for (float s : windowSum) {
            avgWindowSum += s;
        }
        avgWindowSum /= FFT_SIZE;
        
        std::cout << "Average window overlap sum = " << avgWindowSum << "\n";
        std::cout << "COLA compensation = " << (1.0f / avgWindowSum) << "\n";
        
        // Final scaling recommendation
        float fftScale = 1.0f / FFT_SIZE;  // JUCE inverse FFT scaling
        float overlapScale = 1.0f / OVERLAP;  // Overlap compensation  
        float windowScale = 1.0f / avgWindowSum;  // Window compensation
        
        float totalScale = fftScale * overlapScale * windowScale;
        
        std::cout << "\n=== FINAL SCALING CALCULATION ===\n";
        std::cout << "FFT scaling: " << fftScale << "\n";
        std::cout << "Overlap scaling: " << overlapScale << "\n";
        std::cout << "Window scaling: " << windowScale << "\n";
        std::cout << "Total scaling: " << totalScale << "\n";
        
        // Simpler approach
        float simpleScale = 1.0f / FFT_SIZE;
        std::cout << "\nSimpler approach: just 1/FFT_SIZE = " << simpleScale << "\n";
    }
    
    return 0;
}