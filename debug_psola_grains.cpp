#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "psola_engine_final.h"

// Simple voiced signal generator
static std::vector<float> makeVoiced(float fs, float f0, float seconds){
    int N=(int)std::ceil(seconds*fs);
    int T=(int)std::round(fs/f0);
    std::vector<float> y(N,0.f);
    
    // Generate clean periodic pulses
    for(int i=0; i<N; i+=T) {
        // Hann pulse
        int pulseLen = T/2;
        for(int j=0; j<pulseLen && i+j<N; ++j) {
            y[i+j] = 0.3f * (1.f - std::cos(2.f*M_PI*j/(pulseLen-1)));
        }
    }
    return y;
}

// Find epoch marks at peaks
static std::vector<int> epochMarks(const std::vector<float>& x, float fs, float f0){
    int N=(int)x.size(), T=(int)std::round(fs/f0);
    std::vector<int> marks;
    
    for(int i=T/2; i<N-T; i+=T) {
        // Find local max around expected position
        int L=std::max(0,i-T/3), R=std::min(N-1,i+T/3);
        int bestIdx=i; 
        float maxVal=x[i];
        for(int k=L; k<=R; ++k) {
            if(x[k]>maxVal) { maxVal=x[k]; bestIdx=k; }
        }
        marks.push_back(bestIdx);
    }
    return marks;
}

int main() {
    const float fs = 48000.f;
    const float f0 = 220.f;
    const float dur = 0.2f; // Short for debugging
    
    auto signal = makeVoiced(fs, f0, dur);
    auto marks = epochMarks(signal, fs, f0);
    
    printf("=== GRAIN DEBUG ===\n");
    printf("Signal: %zu samples, F0=%.1fHz, Period=%.1f samples\n", 
           signal.size(), f0, fs/f0);
    printf("Found %zu epoch marks\n", marks.size());
    printf("First 5 marks: ");
    for(size_t i=0; i<5 && i<marks.size(); ++i) {
        printf("%d ", marks[i]);
    }
    printf("\n\n");
    
    // Test problematic ratio
    float alpha = 0.7071f;
    
    PsolaEngine eng;
    eng.prepare(fs, 2.0);
    
    // Push all input at once for simplicity
    eng.pushBlock(signal.data(), signal.size());
    eng.appendEpochs(marks, 0, fs/f0, true);
    
    // Render output
    std::vector<float> output(signal.size());
    eng.resetSynthesis(0);
    eng.renderBlock(alpha, output.data(), output.size(), 0);
    
    // Analyze output by finding peaks
    printf("=== OUTPUT ANALYSIS (α=%.4f) ===\n", alpha);
    
    // Find peaks in output
    std::vector<int> outPeaks;
    for(int i=100; i<output.size()-100; ++i) {
        if(output[i] > output[i-1] && output[i] > output[i+1] && output[i] > 0.01f) {
            outPeaks.push_back(i);
        }
    }
    
    printf("Found %zu peaks in output\n", outPeaks.size());
    if(outPeaks.size() >= 2) {
        printf("First 10 peak positions: ");
        for(size_t i=0; i<10 && i<outPeaks.size(); ++i) {
            printf("%d ", outPeaks[i]);
        }
        printf("\n");
        
        // Calculate average period
        double avgPeriod = 0;
        int count = 0;
        for(size_t i=1; i<outPeaks.size(); ++i) {
            int period = outPeaks[i] - outPeaks[i-1];
            avgPeriod += period;
            count++;
            if(i < 6) {
                printf("Period %zu: %d samples\n", i, period);
            }
        }
        avgPeriod /= count;
        
        float detectedF0 = fs / avgPeriod;
        float expectedF0 = f0 * alpha;
        
        printf("\nAverage period: %.1f samples\n", avgPeriod);
        printf("Detected F0: %.1f Hz\n", detectedF0);
        printf("Expected F0: %.1f Hz (%.1f * %.4f)\n", expectedF0, f0, alpha);
        printf("Error: %.1f cents\n", 1200.f * std::log2(detectedF0/expectedF0));
    }
    
    // Save a few samples for inspection
    printf("\n=== SAMPLE VALUES ===\n");
    printf("Input (first pulse): ");
    for(int i=100; i<120; ++i) printf("%.3f ", signal[i]);
    printf("\n");
    
    printf("Output (first pulse): ");
    for(int i=100; i<120; ++i) printf("%.3f ", output[i]);
    printf("\n");
    
    return 0;
}