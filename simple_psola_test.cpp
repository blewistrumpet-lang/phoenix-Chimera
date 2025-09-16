#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

// Simple TD-PSOLA implementation for testing
class SimplePSOLA {
    std::vector<float> buffer;
    std::vector<int> marks;  // pitch marks in samples
    
public:
    void setInput(const std::vector<float>& x, const std::vector<int>& pitchMarks) {
        buffer = x;
        marks = pitchMarks;
    }
    
    std::vector<float> shift(float alpha) {
        if (marks.size() < 2) return buffer;
        
        // Average period
        float avgPeriod = 0;
        for (size_t i = 1; i < marks.size(); ++i) {
            avgPeriod += marks[i] - marks[i-1];
        }
        avgPeriod /= (marks.size() - 1);
        
        int winLen = (int)(2.0f * avgPeriod) | 1;  // odd window, shorter
        int halfWin = winLen / 2;
        
        // Hann window
        std::vector<float> window(winLen);
        for (int i = 0; i < winLen; ++i) {
            window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (winLen - 1)));
        }
        
        // Output buffer
        std::vector<float> output(buffer.size(), 0.0f);
        
        // Synthesis hop
        float synHop = avgPeriod / alpha;
        
        // PSOLA synthesis
        float synPos = marks[0];  // start at first mark
        float anaIdx = 0.0f;       // analysis index (fractional)
        
        int grainCount = 0;
        while (synPos < output.size() - halfWin && anaIdx < marks.size() - 1) {
            // Find analysis grain center (interpolate between marks)
            int k0 = (int)anaIdx;
            int k1 = std::min(k0 + 1, (int)marks.size() - 1);
            float frac = anaIdx - k0;
            
            int center = (int)((1.0f - frac) * marks[k0] + frac * marks[k1]);
            
            // Debug: print grain selection pattern for 0.7071
            if (alpha > 0.7 && alpha < 0.8 && grainCount < 10) {
                printf("Grain %d: anaIdx=%.2f, k0=%d, k1=%d, center=%d, synPos=%.1f\n",
                       grainCount, anaIdx, k0, k1, center, synPos);
            }
            
            // Window and add grain
            for (int i = 0; i < winLen; ++i) {
                int srcIdx = center - halfWin + i;
                int dstIdx = (int)synPos - halfWin + i;
                
                if (srcIdx >= 0 && srcIdx < buffer.size() && 
                    dstIdx >= 0 && dstIdx < output.size()) {
                    output[dstIdx] += window[i] * buffer[srcIdx];
                }
            }
            
            synPos += synHop;
            anaIdx += 1.0f / alpha;  // advance by 1/alpha periods
            grainCount++;
        }
        
        return output;
    }
};

// Simple pitch detector
float detectPitch(const std::vector<float>& x, float fs) {
    int N = x.size();
    int minLag = fs / 800;  // 800 Hz max
    int maxLag = fs / 60;   // 60 Hz min
    
    float maxCorr = -1;
    int bestLag = minLag;
    
    for (int lag = minLag; lag <= maxLag && lag < N; ++lag) {
        float sum = 0, norm1 = 0, norm2 = 0;
        for (int i = 0; i < N - lag; ++i) {
            sum += x[i] * x[i + lag];
            norm1 += x[i] * x[i];
            norm2 += x[i + lag] * x[i + lag];
        }
        float corr = sum / (std::sqrt(norm1 * norm2) + 1e-12f);
        if (corr > maxCorr) {
            maxCorr = corr;
            bestLag = lag;
        }
    }
    
    return fs / bestLag;
}

// Generate test signal
std::vector<float> generatePulses(float fs, float f0, float duration) {
    int N = (int)(fs * duration);
    int period = (int)(fs / f0);
    std::vector<float> signal(N, 0.0f);
    
    for (int i = 0; i < N; i += period) {
        // Add a Hann pulse
        int pulseLen = period / 2;
        for (int j = 0; j < pulseLen && i + j < N; ++j) {
            signal[i + j] = 0.3f * (1.0f - std::cos(2.0f * M_PI * j / (pulseLen - 1)));
        }
    }
    
    return signal;
}

// Find pitch marks
std::vector<int> findMarks(const std::vector<float>& x, int period) {
    std::vector<int> marks;
    int searchWin = period / 3;
    
    for (int i = period; i < x.size() - period; i += period) {
        // Find local max
        int bestIdx = i;
        float maxVal = x[i];
        
        for (int j = i - searchWin; j <= i + searchWin && j < x.size(); ++j) {
            if (x[j] > maxVal) {
                maxVal = x[j];
                bestIdx = j;
            }
        }
        
        marks.push_back(bestIdx);
    }
    
    return marks;
}

int main() {
    float fs = 48000;
    float f0 = 220;
    float duration = 1.0;
    
    // Generate test signal
    auto signal = generatePulses(fs, f0, duration);
    
    // Find pitch marks
    int period = (int)(fs / f0);
    auto marks = findMarks(signal, period);
    
    printf("Input: %zu samples, %zu marks, avg period %.1f\n", 
           signal.size(), marks.size(), fs/f0);
    
    // Test pitch shifts
    SimplePSOLA psola;
    psola.setInput(signal, marks);
    
    float ratios[] = {0.5f, 0.7071f, 1.0f, 1.5f, 2.0f};
    const char* names[] = {"0.5 (oct down)", "0.707 (tritone down)", "1.0 (unison)", 
                           "1.5 (fifth up)", "2.0 (oct up)"};
    
    for (int i = 0; i < 5; ++i) {
        auto output = psola.shift(ratios[i]);
        
        // Analyze last 0.5 seconds
        int start = output.size() / 2;
        std::vector<float> tail(output.begin() + start, output.end());
        
        float detectedF0 = detectPitch(tail, fs);
        float expectedF0 = f0 * ratios[i];
        float cents = 1200.0f * std::log2(detectedF0 / expectedF0);
        
        printf("Ratio %.3f (%s): detected %.1f Hz, expected %.1f Hz, error %.1f cents\n",
               ratios[i], names[i], detectedF0, expectedF0, cents);
    }
    
    return 0;
}