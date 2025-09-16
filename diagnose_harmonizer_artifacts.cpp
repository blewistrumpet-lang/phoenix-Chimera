#include <vector>
#include <cmath>
#include <cstdio>
#include <complex>
#include <algorithm>
#include <fstream>
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"

// FFT for spectral analysis
class SimpleFFT {
    static constexpr int kFFTSize = 2048;
    std::vector<std::complex<float>> twiddles;
    
public:
    SimpleFFT() {
        // Precompute twiddle factors
        twiddles.resize(kFFTSize/2);
        for (int i = 0; i < kFFTSize/2; ++i) {
            float angle = -2.0f * M_PI * i / kFFTSize;
            twiddles[i] = std::complex<float>(cos(angle), sin(angle));
        }
    }
    
    std::vector<float> computeMagnitudeSpectrum(const std::vector<float>& signal, int startIdx = 0) {
        std::vector<std::complex<float>> fft(kFFTSize);
        
        // Window and copy
        for (int i = 0; i < kFFTSize && startIdx + i < signal.size(); ++i) {
            float window = 0.5f - 0.5f * cos(2.0f * M_PI * i / (kFFTSize - 1)); // Hann
            fft[i] = signal[startIdx + i] * window;
        }
        
        // Simple DFT (not optimized, but clear)
        std::vector<std::complex<float>> result(kFFTSize);
        for (int k = 0; k < kFFTSize; ++k) {
            std::complex<float> sum(0, 0);
            for (int n = 0; n < kFFTSize; ++n) {
                float angle = -2.0f * M_PI * k * n / kFFTSize;
                sum += fft[n] * std::complex<float>(cos(angle), sin(angle));
            }
            result[k] = sum;
        }
        
        // Convert to magnitude
        std::vector<float> mag(kFFTSize/2);
        for (int i = 0; i < kFFTSize/2; ++i) {
            mag[i] = std::abs(result[i]);
        }
        
        return mag;
    }
};

// Comprehensive diagnostic metrics
struct DiagnosticMetrics {
    // Time domain
    float rms = 0;
    float peak = 0;
    float crestFactor = 0;
    float dcOffset = 0;
    int zerosCrossings = 0;
    int discontinuities = 0;
    
    // Frequency domain
    float spectralCentroid = 0;
    float spectralSpread = 0;
    float spectralFlux = 0;
    float harmonicDistortion = 0;
    std::vector<float> harmonicAmplitudes;
    
    // Artifacts
    int clickCount = 0;
    float maxClickAmplitude = 0;
    int dropoutCount = 0;
    float noiseFloor = 0;
    
    // Pitch tracking
    float detectedF0 = 0;
    float expectedF0 = 0;
    float pitchError = 0;
    float pitchStability = 0;
    
    void print() const {
        printf("\n=== DIAGNOSTIC METRICS ===\n");
        printf("Time Domain:\n");
        printf("  RMS: %.4f (%.2f dB)\n", rms, 20*log10(rms + 1e-12f));
        printf("  Peak: %.4f\n", peak);
        printf("  Crest Factor: %.2f dB\n", crestFactor);
        printf("  DC Offset: %.6f\n", dcOffset);
        printf("  Zero Crossings: %d\n", zerosCrossings);
        printf("  Discontinuities: %d\n", discontinuities);
        
        printf("\nFrequency Domain:\n");
        printf("  Spectral Centroid: %.1f Hz\n", spectralCentroid);
        printf("  Spectral Spread: %.1f Hz\n", spectralSpread);
        printf("  Spectral Flux: %.4f\n", spectralFlux);
        printf("  Harmonic Distortion: %.2f%%\n", harmonicDistortion * 100);
        
        printf("\nArtifacts:\n");
        printf("  Click Count: %d\n", clickCount);
        printf("  Max Click Amplitude: %.4f\n", maxClickAmplitude);
        printf("  Dropout Count: %d\n", dropoutCount);
        printf("  Noise Floor: %.2f dB\n", 20*log10(noiseFloor + 1e-12f));
        
        printf("\nPitch Tracking:\n");
        printf("  Detected F0: %.2f Hz\n", detectedF0);
        printf("  Expected F0: %.2f Hz\n", expectedF0);
        printf("  Pitch Error: %.2f cents\n", pitchError);
        printf("  Pitch Stability: %.2f%%\n", pitchStability * 100);
    }
};

class HarmonizerDiagnostics {
    SimpleFFT fft;
    float sampleRate = 48000.0f;
    
public:
    DiagnosticMetrics analyze(const std::vector<float>& signal, float expectedF0 = 0) {
        DiagnosticMetrics metrics;
        metrics.expectedF0 = expectedF0;
        
        // Time domain analysis
        analyzeTimeDomain(signal, metrics);
        
        // Frequency domain analysis
        analyzeFrequencyDomain(signal, metrics);
        
        // Artifact detection
        detectArtifacts(signal, metrics);
        
        // Pitch analysis
        if (expectedF0 > 0) {
            analyzePitch(signal, metrics);
        }
        
        return metrics;
    }
    
    // Generate spectrogram data for visualization
    std::vector<std::vector<float>> generateSpectrogram(const std::vector<float>& signal, 
                                                         int hopSize = 512) {
        std::vector<std::vector<float>> spectrogram;
        
        for (int i = 0; i < signal.size() - 2048; i += hopSize) {
            auto spectrum = fft.computeMagnitudeSpectrum(signal, i);
            
            // Convert to dB
            for (auto& bin : spectrum) {
                bin = 20 * log10(bin + 1e-12f);
            }
            
            spectrogram.push_back(spectrum);
        }
        
        return spectrogram;
    }
    
    // Save spectrogram to CSV for plotting
    void saveSpectrogram(const std::vector<std::vector<float>>& spec, const std::string& filename) {
        std::ofstream file(filename);
        
        for (const auto& frame : spec) {
            for (size_t i = 0; i < frame.size(); ++i) {
                file << frame[i];
                if (i < frame.size() - 1) file << ",";
            }
            file << "\n";
        }
        
        file.close();
        printf("Saved spectrogram to %s\n", filename.c_str());
    }
    
private:
    void analyzeTimeDomain(const std::vector<float>& signal, DiagnosticMetrics& m) {
        if (signal.empty()) return;
        
        // RMS and peak
        double sum = 0, sum2 = 0;
        m.peak = 0;
        
        for (float s : signal) {
            sum += s;
            sum2 += s * s;
            m.peak = std::max(m.peak, std::abs(s));
        }
        
        m.dcOffset = sum / signal.size();
        m.rms = sqrt(sum2 / signal.size());
        m.crestFactor = 20 * log10(m.peak / (m.rms + 1e-12f));
        
        // Zero crossings
        for (size_t i = 1; i < signal.size(); ++i) {
            if ((signal[i-1] <= 0 && signal[i] > 0) || 
                (signal[i-1] >= 0 && signal[i] < 0)) {
                m.zerosCrossings++;
            }
        }
        
        // Discontinuities (sudden jumps)
        float threshold = m.rms * 3.0f;
        for (size_t i = 1; i < signal.size(); ++i) {
            if (std::abs(signal[i] - signal[i-1]) > threshold) {
                m.discontinuities++;
            }
        }
    }
    
    void analyzeFrequencyDomain(const std::vector<float>& signal, DiagnosticMetrics& m) {
        if (signal.size() < 2048) return;
        
        // Get spectrum from middle of signal
        int midPoint = signal.size() / 2;
        auto spectrum = fft.computeMagnitudeSpectrum(signal, midPoint - 1024);
        
        // Spectral centroid and spread
        float sumMag = 0, sumFreqMag = 0, sumFreq2Mag = 0;
        float binHz = sampleRate / 2048.0f;
        
        for (size_t i = 0; i < spectrum.size(); ++i) {
            float freq = i * binHz;
            float mag = spectrum[i];
            
            sumMag += mag;
            sumFreqMag += freq * mag;
            sumFreq2Mag += freq * freq * mag;
        }
        
        if (sumMag > 0) {
            m.spectralCentroid = sumFreqMag / sumMag;
            float variance = (sumFreq2Mag / sumMag) - (m.spectralCentroid * m.spectralCentroid);
            m.spectralSpread = sqrt(std::max(0.0f, variance));
        }
        
        // Spectral flux (change between frames)
        if (signal.size() >= 4096) {
            auto spectrum1 = fft.computeMagnitudeSpectrum(signal, 0);
            auto spectrum2 = fft.computeMagnitudeSpectrum(signal, 2048);
            
            float flux = 0;
            for (size_t i = 0; i < spectrum1.size(); ++i) {
                float diff = spectrum2[i] - spectrum1[i];
                if (diff > 0) flux += diff;
            }
            m.spectralFlux = flux / spectrum1.size();
        }
    }
    
    void detectArtifacts(const std::vector<float>& signal, DiagnosticMetrics& m) {
        if (signal.size() < 100) return;
        
        // Click detection (sudden energy changes)
        int windowSize = 64;
        std::vector<float> energy;
        
        for (int i = 0; i < signal.size() - windowSize; i += windowSize/2) {
            float e = 0;
            for (int j = 0; j < windowSize; ++j) {
                e += signal[i+j] * signal[i+j];
            }
            energy.push_back(e / windowSize);
        }
        
        // Detect sudden energy increases
        for (size_t i = 1; i < energy.size(); ++i) {
            float ratio = energy[i] / (energy[i-1] + 1e-12f);
            if (ratio > 10.0f) { // 20dB sudden increase
                m.clickCount++;
                m.maxClickAmplitude = std::max(m.maxClickAmplitude, sqrt(energy[i]));
            }
        }
        
        // Dropout detection (sudden silence)
        float silenceThreshold = m.rms * 0.01f;
        int silenceCount = 0;
        
        for (size_t i = 0; i < signal.size(); ++i) {
            if (std::abs(signal[i]) < silenceThreshold) {
                silenceCount++;
                if (silenceCount > 48) { // 1ms at 48kHz
                    m.dropoutCount++;
                    silenceCount = 0;
                }
            } else {
                silenceCount = 0;
            }
        }
        
        // Noise floor estimation (quietest 10%)
        std::vector<float> sorted = signal;
        for (auto& s : sorted) s = std::abs(s);
        std::sort(sorted.begin(), sorted.end());
        
        int tenPercent = sorted.size() / 10;
        float sum = 0;
        for (int i = 0; i < tenPercent; ++i) {
            sum += sorted[i];
        }
        m.noiseFloor = sum / tenPercent;
    }
    
    void analyzePitch(const std::vector<float>& signal, DiagnosticMetrics& m) {
        // Autocorrelation-based pitch detection
        int minLag = sampleRate / 800;  // 800Hz max
        int maxLag = std::min((int)signal.size()/2, (int)(sampleRate / 60)); // 60Hz min
        
        float maxCorr = -1;
        int bestLag = 0;
        
        for (int lag = minLag; lag <= maxLag; ++lag) {
            float sum = 0, norm1 = 0, norm2 = 0;
            
            for (int i = 0; i < signal.size() - lag; ++i) {
                sum += signal[i] * signal[i + lag];
                norm1 += signal[i] * signal[i];
                norm2 += signal[i + lag] * signal[i + lag];
            }
            
            float corr = sum / (sqrt(norm1 * norm2) + 1e-12f);
            if (corr > maxCorr) {
                maxCorr = corr;
                bestLag = lag;
            }
        }
        
        m.detectedF0 = sampleRate / bestLag;
        
        if (m.expectedF0 > 0) {
            m.pitchError = 1200 * log2(m.detectedF0 / m.expectedF0);
        }
        
        // Pitch stability (how consistent is the pitch across frames)
        std::vector<float> framePitches;
        int frameSize = 2048;
        int hopSize = 512;
        
        for (int i = 0; i < signal.size() - frameSize; i += hopSize) {
            // Simple zero-crossing rate for quick pitch estimate
            int crossings = 0;
            for (int j = 1; j < frameSize; ++j) {
                if ((signal[i+j-1] <= 0 && signal[i+j] > 0)) {
                    crossings++;
                }
            }
            float framePitch = (crossings * sampleRate) / (2.0f * frameSize);
            framePitches.push_back(framePitch);
        }
        
        // Calculate pitch variance
        if (framePitches.size() > 1) {
            float mean = 0;
            for (float p : framePitches) mean += p;
            mean /= framePitches.size();
            
            float variance = 0;
            for (float p : framePitches) {
                variance += (p - mean) * (p - mean);
            }
            variance /= framePitches.size();
            
            m.pitchStability = 1.0f / (1.0f + sqrt(variance) / mean);
        }
    }
};

// Test harness
int main() {
    printf("=== INTELLIGENT HARMONIZER ARTIFACT DIAGNOSIS ===\n\n");
    
    const float fs = 48000.0f;
    const int blockSize = 512;
    const float testDuration = 2.0f;
    const int totalSamples = fs * testDuration;
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(fs, blockSize);
    
    HarmonizerDiagnostics diagnostics;
    
    // Test 1: Clean sine wave
    printf("TEST 1: Clean 220Hz Sine Wave\n");
    printf("--------------------------------\n");
    {
        std::vector<float> input(totalSamples);
        std::vector<float> output(totalSamples);
        
        // Generate input
        for (int i = 0; i < totalSamples; ++i) {
            input[i] = 0.3f * sin(2.0f * M_PI * 220.0f * i / fs);
        }
        
        // Process through harmonizer with different intervals
        float testIntervals[] = {0.5f, 0.25f, 0.75f}; // Unison, octave down, octave up
        const char* intervalNames[] = {"Unison", "Octave Down", "Octave Up"};
        float expectedPitches[] = {220.0f, 110.0f, 440.0f};
        
        for (int test = 0; test < 3; ++test) {
            printf("\n--- %s ---\n", intervalNames[test]);
            
            // Reset harmonizer
            harmonizer.reset();
            
            // Set parameters
            std::map<int, float> params;
            params[0] = testIntervals[test]; // Interval
            params[7] = 1.0f; // Mix = 100% wet
            harmonizer.updateParameters(params);
            
            // Process in blocks
            for (int i = 0; i < totalSamples; i += blockSize) {
                int samplesThisBlock = std::min(blockSize, totalSamples - i);
                juce::AudioBuffer<float> buffer(1, samplesThisBlock);
                
                // Copy input
                for (int j = 0; j < samplesThisBlock; ++j) {
                    buffer.setSample(0, j, input[i + j]);
                }
                
                // Process
                harmonizer.process(buffer);
                
                // Copy output
                for (int j = 0; j < samplesThisBlock; ++j) {
                    output[i + j] = buffer.getSample(0, j);
                }
            }
            
            // Analyze output
            auto metrics = diagnostics.analyze(output, expectedPitches[test]);
            metrics.print();
            
            // Save spectrogram
            auto spectrogram = diagnostics.generateSpectrogram(output);
            char filename[100];
            sprintf(filename, "spectrogram_sine_%s.csv", intervalNames[test]);
            diagnostics.saveSpectrogram(spectrogram, filename);
        }
    }
    
    // Test 2: Complex harmonic signal
    printf("\n\nTEST 2: Complex Harmonic Signal (Sawtooth)\n");
    printf("--------------------------------------------\n");
    {
        std::vector<float> input(totalSamples);
        std::vector<float> output(totalSamples);
        
        // Generate sawtooth
        float phase = 0;
        float phaseInc = 220.0f / fs;
        for (int i = 0; i < totalSamples; ++i) {
            input[i] = 0.3f * (2.0f * phase - 1.0f);
            phase += phaseInc;
            if (phase >= 1.0f) phase -= 1.0f;
        }
        
        // Test problematic interval (tritone)
        printf("\n--- Tritone (0.7071 ratio) ---\n");
        
        harmonizer.reset();
        std::map<int, float> params;
        params[0] = 0.354f; // Approximately -6 semitones normalized
        params[7] = 1.0f;
        harmonizer.updateParameters(params);
        
        // Process
        for (int i = 0; i < totalSamples; i += blockSize) {
            int samplesThisBlock = std::min(blockSize, totalSamples - i);
            juce::AudioBuffer<float> buffer(1, samplesThisBlock);
            
            for (int j = 0; j < samplesThisBlock; ++j) {
                buffer.setSample(0, j, input[i + j]);
            }
            
            harmonizer.process(buffer);
            
            for (int j = 0; j < samplesThisBlock; ++j) {
                output[i + j] = buffer.getSample(0, j);
            }
        }
        
        auto metrics = diagnostics.analyze(output, 155.56f); // 220 * 0.7071
        metrics.print();
        
        auto spectrogram = diagnostics.generateSpectrogram(output);
        diagnostics.saveSpectrogram(spectrogram, "spectrogram_sawtooth_tritone.csv");
    }
    
    // Test 3: Real-world audio (speech-like envelope)
    printf("\n\nTEST 3: Speech-like Signal with Envelope\n");
    printf("-----------------------------------------\n");
    {
        std::vector<float> input(totalSamples);
        std::vector<float> output(totalSamples);
        
        // Generate modulated signal
        for (int i = 0; i < totalSamples; ++i) {
            float envelope = 0.5f * (1.0f + sin(2.0f * M_PI * 3.0f * i / fs)); // 3Hz envelope
            float carrier = sin(2.0f * M_PI * 220.0f * i / fs);
            input[i] = 0.3f * envelope * carrier;
        }
        
        harmonizer.reset();
        std::map<int, float> params;
        params[0] = 0.583f; // +7 semitones (perfect fifth)
        params[7] = 0.7f;   // 70% wet mix
        harmonizer.updateParameters(params);
        
        // Process
        for (int i = 0; i < totalSamples; i += blockSize) {
            int samplesThisBlock = std::min(blockSize, totalSamples - i);
            juce::AudioBuffer<float> buffer(1, samplesThisBlock);
            
            for (int j = 0; j < samplesThisBlock; ++j) {
                buffer.setSample(0, j, input[i + j]);
            }
            
            harmonizer.process(buffer);
            
            for (int j = 0; j < samplesThisBlock; ++j) {
                output[i + j] = buffer.getSample(0, j);
            }
        }
        
        auto metrics = diagnostics.analyze(output, 329.63f); // 220 * 1.5
        metrics.print();
    }
    
    // Test 4: Silence and noise handling
    printf("\n\nTEST 4: Silence and Low-level Noise\n");
    printf("------------------------------------\n");
    {
        std::vector<float> input(totalSamples);
        std::vector<float> output(totalSamples);
        
        // Generate signal with silent sections
        for (int i = 0; i < totalSamples; ++i) {
            if (i < totalSamples/3 || i > 2*totalSamples/3) {
                // Silence with tiny noise
                input[i] = 0.0001f * ((rand() / (float)RAND_MAX) - 0.5f);
            } else {
                // Signal
                input[i] = 0.3f * sin(2.0f * M_PI * 220.0f * i / fs);
            }
        }
        
        harmonizer.reset();
        std::map<int, float> params;
        params[0] = 0.5f; // Unison
        params[7] = 1.0f;
        harmonizer.updateParameters(params);
        
        // Process
        for (int i = 0; i < totalSamples; i += blockSize) {
            int samplesThisBlock = std::min(blockSize, totalSamples - i);
            juce::AudioBuffer<float> buffer(1, samplesThisBlock);
            
            for (int j = 0; j < samplesThisBlock; ++j) {
                buffer.setSample(0, j, input[i + j]);
            }
            
            harmonizer.process(buffer);
            
            for (int j = 0; j < samplesThisBlock; ++j) {
                output[i + j] = buffer.getSample(0, j);
            }
        }
        
        auto metrics = diagnostics.analyze(output);
        metrics.print();
    }
    
    printf("\n\n=== DIAGNOSIS COMPLETE ===\n");
    printf("Check the generated CSV files for detailed spectrograms.\n");
    printf("Look for:\n");
    printf("- High click counts -> Discontinuity issues\n");
    printf("- High spectral flux -> Unstable processing\n");
    printf("- Poor pitch stability -> Epoch detection problems\n");
    printf("- High noise floor -> Numerical issues\n");
    
    return 0;
}