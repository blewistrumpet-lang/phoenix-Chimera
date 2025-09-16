#include <vector>
#include <cmath>
#include <cstdio>
#include <complex>
#include <algorithm>
#include <fstream>
#include <cstring>
#include "psola_engine_final.h"

// Simple FFT for spectral analysis
class SimpleFFT {
    static constexpr int kFFTSize = 2048;
    
public:
    std::vector<float> computeMagnitudeSpectrum(const std::vector<float>& signal, int startIdx = 0) {
        std::vector<std::complex<float>> fft(kFFTSize);
        
        // Window and copy
        for (int i = 0; i < kFFTSize && startIdx + i < signal.size(); ++i) {
            float window = 0.5f - 0.5f * cos(2.0f * M_PI * i / (kFFTSize - 1)); // Hann
            fft[i] = signal[startIdx + i] * window;
        }
        
        // Simple DFT (not optimized, but clear)
        std::vector<std::complex<float>> result(kFFTSize);
        for (int k = 0; k < kFFTSize/2; ++k) { // Only compute positive frequencies
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
            mag[i] = std::abs(result[i]) / kFFTSize;
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
    int zeroCrossings = 0;
    int discontinuities = 0;
    
    // Frequency domain
    float spectralCentroid = 0;
    float spectralSpread = 0;
    float harmonicDistortion = 0;
    
    // Artifacts
    int clickCount = 0;
    float maxClickAmplitude = 0;
    int dropoutCount = 0;
    float noiseFloor = 0;
    
    // Pitch tracking
    float detectedF0 = 0;
    float expectedF0 = 0;
    float pitchErrorCents = 0;
    
    // PSOLA specific
    int epochCount = 0;
    float avgEpochSpacing = 0;
    float epochSpacingVariance = 0;
    
    void print() const {
        printf("\n=== DIAGNOSTIC METRICS ===\n");
        printf("Time Domain:\n");
        printf("  RMS: %.4f (%.2f dB)\n", rms, 20*log10(rms + 1e-12f));
        printf("  Peak: %.4f\n", peak);
        printf("  Crest Factor: %.2f dB\n", crestFactor);
        printf("  DC Offset: %.6f\n", dcOffset);
        printf("  Zero Crossings: %d\n", zeroCrossings);
        printf("  Discontinuities: %d\n", discontinuities);
        
        printf("\nFrequency Domain:\n");
        printf("  Spectral Centroid: %.1f Hz\n", spectralCentroid);
        printf("  Spectral Spread: %.1f Hz\n", spectralSpread);
        printf("  THD: %.2f%%\n", harmonicDistortion * 100);
        
        printf("\nArtifacts:\n");
        printf("  Click Count: %d\n", clickCount);
        printf("  Max Click: %.4f\n", maxClickAmplitude);
        printf("  Dropouts: %d\n", dropoutCount);
        printf("  Noise Floor: %.2f dB\n", 20*log10(noiseFloor + 1e-12f));
        
        printf("\nPitch:\n");
        printf("  Detected: %.2f Hz\n", detectedF0);
        printf("  Expected: %.2f Hz\n", expectedF0);
        printf("  Error: %.2f cents\n", pitchErrorCents);
        
        printf("\nPSOLA:\n");
        printf("  Epochs: %d\n", epochCount);
        printf("  Avg Spacing: %.1f samples\n", avgEpochSpacing);
        printf("  Spacing Variance: %.1f\n", epochSpacingVariance);
    }
};

class PSOLADiagnostics {
    SimpleFFT fft;
    float sampleRate = 48000.0f;
    
public:
    DiagnosticMetrics analyze(const std::vector<float>& signal, 
                             const PsolaEngine& engine,
                             float expectedF0 = 0) {
        DiagnosticMetrics m;
        m.expectedF0 = expectedF0;
        
        analyzeTimeDomain(signal, m);
        analyzeFrequencyDomain(signal, m);
        detectArtifacts(signal, m);
        analyzePitch(signal, m);
        analyzeEpochs(engine, m);
        
        return m;
    }
    
    void saveSpectrogram(const std::vector<float>& signal, const char* filename) {
        std::ofstream file(filename);
        int hopSize = 512;
        
        for (int i = 0; i <= signal.size() - 2048; i += hopSize) {
            auto spectrum = fft.computeMagnitudeSpectrum(signal, i);
            
            for (size_t j = 0; j < spectrum.size(); ++j) {
                float dB = 20 * log10(spectrum[j] + 1e-12f);
                file << dB;
                if (j < spectrum.size() - 1) file << ",";
            }
            file << "\n";
        }
        
        file.close();
        printf("Saved spectrogram to %s\n", filename);
    }
    
private:
    void analyzeTimeDomain(const std::vector<float>& signal, DiagnosticMetrics& m) {
        if (signal.empty()) return;
        
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
                m.zeroCrossings++;
            }
        }
        
        // Discontinuities
        float threshold = m.rms * 4.0f;
        for (size_t i = 1; i < signal.size(); ++i) {
            if (std::abs(signal[i] - signal[i-1]) > threshold) {
                m.discontinuities++;
            }
        }
    }
    
    void analyzeFrequencyDomain(const std::vector<float>& signal, DiagnosticMetrics& m) {
        if (signal.size() < 2048) return;
        
        int midPoint = signal.size() / 2;
        auto spectrum = fft.computeMagnitudeSpectrum(signal, midPoint - 1024);
        
        // Find fundamental and harmonics
        float maxMag = 0;
        int fundBin = 0;
        
        for (int i = 10; i < 100; ++i) { // Look for fundamental between ~100-1000 Hz
            if (spectrum[i] > maxMag) {
                maxMag = spectrum[i];
                fundBin = i;
            }
        }
        
        // Spectral centroid
        float sumMag = 0, sumFreqMag = 0;
        float binHz = sampleRate / 2048.0f;
        
        for (size_t i = 0; i < spectrum.size(); ++i) {
            float freq = i * binHz;
            float mag = spectrum[i];
            sumMag += mag;
            sumFreqMag += freq * mag;
        }
        
        if (sumMag > 0) {
            m.spectralCentroid = sumFreqMag / sumMag;
        }
        
        // THD calculation
        if (fundBin > 0 && fundBin * 5 < spectrum.size()) {
            float fundPower = spectrum[fundBin] * spectrum[fundBin];
            float harmonicPower = 0;
            
            for (int h = 2; h <= 5; ++h) {
                int harmBin = fundBin * h;
                if (harmBin < spectrum.size()) {
                    harmonicPower += spectrum[harmBin] * spectrum[harmBin];
                }
            }
            
            m.harmonicDistortion = sqrt(harmonicPower / (fundPower + 1e-12f));
        }
    }
    
    void detectArtifacts(const std::vector<float>& signal, DiagnosticMetrics& m) {
        if (signal.size() < 100) return;
        
        // Click detection using energy ratio
        int windowSize = 64;
        float prevEnergy = 0;
        
        for (int i = 0; i <= signal.size() - windowSize; i += windowSize/2) {
            float energy = 0;
            for (int j = 0; j < windowSize && i+j < signal.size(); ++j) {
                energy += signal[i+j] * signal[i+j];
            }
            energy /= windowSize;
            
            if (prevEnergy > 0) {
                float ratio = energy / prevEnergy;
                if (ratio > 100.0f) { // 20dB sudden increase
                    m.clickCount++;
                    m.maxClickAmplitude = std::max(m.maxClickAmplitude, sqrt(energy));
                }
            }
            prevEnergy = energy;
        }
        
        // Dropout detection
        float silenceThreshold = m.rms * 0.01f;
        int consecutiveSilence = 0;
        
        for (float s : signal) {
            if (std::abs(s) < silenceThreshold) {
                consecutiveSilence++;
                if (consecutiveSilence > 48) { // 1ms at 48kHz
                    m.dropoutCount++;
                    consecutiveSilence = 0;
                }
            } else {
                consecutiveSilence = 0;
            }
        }
        
        // Noise floor (bottom 10%)
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
        // Autocorrelation pitch detection
        int minLag = sampleRate / 800;
        int maxLag = std::min((int)signal.size()/2, (int)(sampleRate / 60));
        
        float maxCorr = -1;
        int bestLag = 0;
        
        for (int lag = minLag; lag <= maxLag; ++lag) {
            float sum = 0, norm1 = 0, norm2 = 0;
            
            for (size_t i = 0; i < signal.size() - lag; ++i) {
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
            m.pitchErrorCents = 1200 * log2(m.detectedF0 / m.expectedF0);
        }
    }
    
    void analyzeEpochs(const PsolaEngine& engine, DiagnosticMetrics& m) {
        auto& epochs = engine.epochs();
        m.epochCount = epochs.size();
        
        if (epochs.size() >= 2) {
            std::vector<float> spacings;
            for (size_t i = 1; i < epochs.size(); ++i) {
                float spacing = epochs[i].nAbs - epochs[i-1].nAbs;
                spacings.push_back(spacing);
                m.avgEpochSpacing += spacing;
            }
            
            m.avgEpochSpacing /= spacings.size();
            
            // Variance
            for (float s : spacings) {
                float diff = s - m.avgEpochSpacing;
                m.epochSpacingVariance += diff * diff;
            }
            m.epochSpacingVariance = sqrt(m.epochSpacingVariance / spacings.size());
        }
    }
};

// Test signal generators
std::vector<float> generateSine(float fs, float f0, float duration, float amplitude = 0.3f) {
    int N = fs * duration;
    std::vector<float> signal(N);
    for (int i = 0; i < N; ++i) {
        signal[i] = amplitude * sin(2.0f * M_PI * f0 * i / fs);
    }
    return signal;
}

std::vector<float> generatePulseTrain(float fs, float f0, float duration) {
    int N = fs * duration;
    int T = fs / f0;
    std::vector<float> signal(N, 0);
    
    for (int i = 0; i < N; i += T) {
        int pulseLen = T / 2;
        for (int j = 0; j < pulseLen && i+j < N; ++j) {
            signal[i+j] = 0.3f * (1.0f - cos(2.0f * M_PI * j / (pulseLen - 1)));
        }
    }
    return signal;
}

std::vector<int> findEpochs(const std::vector<float>& signal, float fs, float f0) {
    std::vector<int> marks;
    int T = fs / f0;
    
    for (int i = T/2; i < signal.size() - T; i += T) {
        int searchWin = T / 3;
        int bestIdx = i;
        float maxVal = signal[i];
        
        for (int j = i - searchWin; j <= i + searchWin && j < signal.size(); ++j) {
            if (j >= 0 && signal[j] > maxVal) {
                maxVal = signal[j];
                bestIdx = j;
            }
        }
        
        if (maxVal > 0.01f) {
            marks.push_back(bestIdx);
        }
    }
    
    return marks;
}

int main() {
    printf("=== PSOLA ENGINE ARTIFACT DIAGNOSIS ===\n\n");
    
    const float fs = 48000.0f;
    const float f0 = 220.0f;
    const float duration = 1.0f;
    
    PsolaEngine engine;
    engine.prepare(fs, 2.0);
    
    PSOLADiagnostics diagnostics;
    
    // Test cases with different signals and ratios
    struct TestCase {
        const char* name;
        float ratio;
        const char* signalType;
    } testCases[] = {
        {"Unison Sine", 1.0f, "sine"},
        {"Octave Down Sine", 0.5f, "sine"},
        {"Tritone Down Sine", 0.7071f, "sine"},
        {"Fifth Up Sine", 1.5f, "sine"},
        {"Octave Up Sine", 2.0f, "sine"},
        {"Unison Pulse", 1.0f, "pulse"},
        {"Tritone Down Pulse", 0.7071f, "pulse"},
        {"Fifth Up Pulse", 1.5f, "pulse"},
    };
    
    for (const auto& test : testCases) {
        printf("\n========================================\n");
        printf("TEST: %s (ratio=%.4f)\n", test.name, test.ratio);
        printf("========================================\n");
        
        // Generate input signal
        std::vector<float> input;
        if (strcmp(test.signalType, "sine") == 0) {
            input = generateSine(fs, f0, duration);
        } else {
            input = generatePulseTrain(fs, f0, duration);
        }
        
        // Reset engine
        engine.prepare(fs, 2.0);
        engine.resetSynthesis(0);
        
        // Feed input and epochs
        engine.pushBlock(input.data(), input.size());
        auto epochs = findEpochs(input, fs, f0);
        engine.appendEpochs(epochs, 0, fs/f0, true);
        
        // Process
        std::vector<float> output(input.size());
        engine.renderBlock(test.ratio, output.data(), output.size(), 0);
        
        // Analyze
        float expectedF0 = f0 * test.ratio;
        auto metrics = diagnostics.analyze(output, engine, expectedF0);
        metrics.print();
        
        // Quality assessment
        printf("\n--- QUALITY ASSESSMENT ---\n");
        
        bool hasClicks = metrics.clickCount > 5;
        bool hasDropouts = metrics.dropoutCount > 0;
        bool pitchAccurate = std::abs(metrics.pitchErrorCents) < 50;
        bool lowNoise = metrics.noiseFloor < -60;
        bool goodCrest = metrics.crestFactor < 20;
        
        printf("Clicks: %s (%d detected)\n", 
               hasClicks ? "FAIL" : "PASS", metrics.clickCount);
        printf("Dropouts: %s (%d detected)\n", 
               hasDropouts ? "FAIL" : "PASS", metrics.dropoutCount);
        printf("Pitch Accuracy: %s (%.1f cents error)\n", 
               pitchAccurate ? "PASS" : "FAIL", metrics.pitchErrorCents);
        printf("Noise Floor: %s (%.1f dB)\n", 
               lowNoise ? "PASS" : "FAIL", 20*log10(metrics.noiseFloor + 1e-12f));
        printf("Crest Factor: %s (%.1f dB)\n", 
               goodCrest ? "PASS" : "FAIL", metrics.crestFactor);
        
        // Save spectrogram for problematic cases
        if (hasClicks || !pitchAccurate || test.ratio == 0.7071f) {
            char filename[256];
            snprintf(filename, sizeof(filename), "spectrogram_%s_%.4f.csv", 
                    test.signalType, test.ratio);
            diagnostics.saveSpectrogram(output, filename);
        }
    }
    
    printf("\n\n=== ARTIFACT DIAGNOSIS COMPLETE ===\n");
    printf("\nKey findings to address:\n");
    printf("1. Click artifacts indicate discontinuities in grain boundaries\n");
    printf("2. Pitch errors suggest epoch selection or timing issues\n");
    printf("3. High noise floor indicates numerical precision problems\n");
    printf("4. Check spectrograms for spectral artifacts\n");
    
    return 0;
}