#include <iostream>
#include <cmath>
#include <vector>
#include <complex>

// Debug version to trace signal flow issues

class DebugPitchShifter {
public:
    static constexpr int FFT_SIZE = 4096;
    static constexpr int OVERLAP_FACTOR = 4;
    static constexpr int HOP_SIZE = FFT_SIZE / OVERLAP_FACTOR;  // 1024
    
    std::vector<float> inputRing;
    std::vector<float> outputRing;
    int inputWriteIdx = 0;
    int outputReadIdx = 0;
    int outputWriteIdx = 0;
    int hopCounter = 0;
    
    float outputScale;
    
    DebugPitchShifter() : 
        inputRing(FFT_SIZE * 2, 0.0f),
        outputRing(FFT_SIZE * 2, 0.0f) {
        
        // TEST DIFFERENT OUTPUT SCALING VALUES
        std::cout << "\n=== OUTPUT SCALING TESTS ===\n";
        
        // Original (broken)
        float scale1 = 1.0f / (FFT_SIZE * OVERLAP_FACTOR * 2.0f);
        std::cout << "Original: 1/(4096*4*2) = " << scale1 << " = " << scale1 << "\n";
        
        // First fix attempt
        float scale2 = 1.0f / OVERLAP_FACTOR;
        std::cout << "Fix 1: 1/4 = " << scale2 << "\n";
        
        // Alternative scalings
        float scale3 = 1.0f / FFT_SIZE;
        std::cout << "Fix 2: 1/4096 = " << scale3 << "\n";
        
        float scale4 = 1.0f / std::sqrt(FFT_SIZE);
        std::cout << "Fix 3: 1/sqrt(4096) = " << scale4 << "\n";
        
        float scale5 = 2.0f / FFT_SIZE;  // Common FFT normalization
        std::cout << "Fix 4: 2/4096 = " << scale5 << "\n";
        
        outputScale = scale2;  // Use 1/4 for now
    }
    
    void testSignalFlow() {
        std::cout << "\n=== SIGNAL FLOW TEST ===\n";
        
        // Simulate 2048 samples (2 frames worth)
        std::vector<float> testSignal(2048);
        for (int i = 0; i < 2048; ++i) {
            testSignal[i] = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        }
        
        float inputRMS = 0;
        float outputRMS = 0;
        
        // Process samples
        for (int i = 0; i < 2048; ++i) {
            float input = testSignal[i];
            inputRMS += input * input;
            
            // Write to input ring
            inputRing[inputWriteIdx] = input;
            inputWriteIdx = (inputWriteIdx + 1) & (FFT_SIZE * 2 - 1);
            hopCounter++;
            
            // Process frame when ready
            if (hopCounter >= HOP_SIZE) {
                hopCounter = 0;
                processFrame();
            }
            
            // Read output
            float output = outputRing[outputReadIdx];
            outputRing[outputReadIdx] = 0.0f;
            outputReadIdx = (outputReadIdx + 1) & (FFT_SIZE * 2 - 1);
            
            outputRMS += output * output;
            
            // First few samples will be zero until buffer fills
            if (i == HOP_SIZE) {
                std::cout << "After first frame (sample " << i << "):\n";
                std::cout << "  Output = " << output << "\n";
            }
        }
        
        inputRMS = std::sqrt(inputRMS / 2048);
        outputRMS = std::sqrt(outputRMS / 2048);
        
        std::cout << "\nRMS Levels:\n";
        std::cout << "  Input RMS: " << inputRMS << "\n";
        std::cout << "  Output RMS: " << outputRMS << "\n";
        std::cout << "  Ratio: " << (outputRMS / inputRMS) << "\n";
        
        if (outputRMS < 0.001f) {
            std::cout << "\n❌ OUTPUT IS ESSENTIALLY ZERO!\n";
        } else if (outputRMS < inputRMS * 0.1f) {
            std::cout << "\n⚠️ OUTPUT IS VERY QUIET (< 10% of input)\n";
        } else {
            std::cout << "\n✅ OUTPUT LEVEL IS REASONABLE\n";
        }
    }
    
    void processFrame() {
        // Simulate FFT -> IFFT passthrough (no pitch shift)
        // This should preserve the signal if scaling is correct
        
        // In real code: gather frame, apply window, FFT, process, IFFT
        // Here we'll simulate the overlap-add directly
        
        // Read FFT_SIZE samples from input ring (simulating gather)
        int readIdx = (inputWriteIdx - FFT_SIZE + inputRing.size()) & (FFT_SIZE * 2 - 1);
        
        // Simulate window (Hann)
        std::vector<float> windowed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            float window = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / (FFT_SIZE - 1));
            windowed[i] = inputRing[readIdx] * window;
            readIdx = (readIdx + 1) & (FFT_SIZE * 2 - 1);
        }
        
        // Simulate FFT -> IFFT (passthrough for testing)
        // Real FFT would modify spectrum here
        
        // Overlap-add into output ring
        int writeIdx = outputWriteIdx;
        for (int i = 0; i < FFT_SIZE; ++i) {
            // Apply synthesis window and scaling
            float window = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / (FFT_SIZE - 1));
            outputRing[writeIdx] += windowed[i] * window * outputScale;
            writeIdx = (writeIdx + 1) & (FFT_SIZE * 2 - 1);
        }
        
        // Update write position by hop size
        outputWriteIdx = (outputWriteIdx + HOP_SIZE) & (FFT_SIZE * 2 - 1);
    }
    
    void testFFTScaling() {
        std::cout << "\n=== FFT SCALING TEST ===\n";
        
        // Test what JUCE FFT does to a unit impulse
        std::vector<std::complex<float>> fftData(FFT_SIZE);
        fftData[0] = std::complex<float>(1.0f, 0.0f);  // Unit impulse
        
        // Simulate forward FFT
        float forwardSum = 0;
        for (const auto& c : fftData) {
            forwardSum += std::abs(c);
        }
        std::cout << "Sum after forward FFT of unit impulse: " << forwardSum << "\n";
        
        // Simulate inverse FFT
        // JUCE typically scales by 1/N on inverse
        float inverseScale = 1.0f / FFT_SIZE;
        std::cout << "Typical IFFT scaling: " << inverseScale << "\n";
        
        // With overlap-add of 4
        float totalScale = inverseScale / OVERLAP_FACTOR;
        std::cout << "With overlap factor: " << totalScale << "\n";
        
        // Window compensation (Hann window)
        float windowSum = 0;
        for (int i = 0; i < OVERLAP_FACTOR; ++i) {
            float window = 0.5f;  // Average value of Hann window squared
            windowSum += window;
        }
        std::cout << "Window overlap sum: " << windowSum << "\n";
        
        float finalScale = totalScale / windowSum;
        std::cout << "Final recommended scale: " << finalScale << "\n";
    }
};

int main() {
    std::cout << "=== PITCHSHIFTER DEBUG ANALYSIS ===\n";
    
    DebugPitchShifter debugger;
    debugger.testSignalFlow();
    debugger.testFFTScaling();
    
    std::cout << "\n=== RECOMMENDATIONS ===\n";
    std::cout << "1. The outputScale value is critical\n";
    std::cout << "2. Current setting (1/4) may still be wrong\n";
    std::cout << "3. Need to account for FFT normalization\n";
    std::cout << "4. Window overlap compensation needed\n";
    std::cout << "5. Try outputScale = 2.0f / FFT_SIZE = " << (2.0f / 4096) << "\n";
    
    return 0;
}