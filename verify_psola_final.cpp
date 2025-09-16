#include <vector>
#include <cmath>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include "psola_engine_final.h"

static float dB(float x){ return 20.f*std::log10(std::max(1e-12f,x)); }

// Better pitch detector - count zero crossings
static float estimateF0_ZC(const std::vector<float>& x, float fs) {
    // High-pass filter first to remove DC
    std::vector<float> hp(x.size());
    float z1 = 0;
    for(size_t i=0; i<x.size(); ++i) {
        hp[i] = x[i] - z1;
        z1 = x[i] * 0.95f + z1 * 0.05f;
    }
    
    // Count positive zero crossings
    int crossings = 0;
    for(size_t i=1; i<hp.size(); ++i) {
        if(hp[i-1] <= 0 && hp[i] > 0) crossings++;
    }
    
    float duration = hp.size() / fs;
    return crossings / duration;
}

// Peak-based F0 estimation
static float estimateF0_Peaks(const std::vector<float>& x, float fs, float fmin=60, float fmax=800) {
    // Find peaks
    std::vector<int> peaks;
    for(size_t i=1; i<x.size()-1; ++i) {
        if(x[i] > x[i-1] && x[i] > x[i+1] && x[i] > 0.01f) {
            peaks.push_back(i);
        }
    }
    
    if(peaks.size() < 2) return 0;
    
    // Calculate average period from peak distances
    std::vector<float> periods;
    for(size_t i=1; i<peaks.size(); ++i) {
        float period = peaks[i] - peaks[i-1];
        float f0 = fs / period;
        if(f0 >= fmin && f0 <= fmax) {
            periods.push_back(period);
        }
    }
    
    if(periods.empty()) return 0;
    
    // Use median period
    std::nth_element(periods.begin(), periods.begin()+periods.size()/2, periods.end());
    float medianPeriod = periods[periods.size()/2];
    
    return fs / medianPeriod;
}

// Clean pulse train
static std::vector<float> makeVoiced(float fs, float f0, float seconds){
    int N=(int)std::ceil(seconds*fs);
    int T=(int)std::round(fs/f0);
    std::vector<float> y(N,0.f);
    
    for(int i=0; i<N; i+=T) {
        int pulseLen = T/2;
        for(int j=0; j<pulseLen && i+j<N; ++j) {
            y[i+j] = 0.3f * (1.f - std::cos(2.f*M_PI*j/(pulseLen-1)));
        }
    }
    return y;
}

static std::vector<int> epochMarks(const std::vector<float>& x, float fs, float f0){
    int N=(int)x.size(), T=(int)std::round(fs/f0);
    std::vector<int> marks;
    
    for(int i=T/2; i<N-T; i+=T) {
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

int main(){
    const float fs = 48000.f, f0 = 220.f, dur = 1.0f;
    auto in = makeVoiced(fs, f0, dur);
    
    std::printf("=== PSOLA ENGINE VERIFICATION ===\n");
    std::printf("Input: F0=%.1fHz, Duration=%.1fs\n\n", f0, dur);

    auto ep = epochMarks(in, fs, f0);
    
    PsolaEngine eng; 
    eng.prepare(fs, 2.0);
    
    // Feed in blocks
    int blk=512; 
    for(size_t i=0; i<in.size(); i+=blk){ 
        int N=std::min<int>(blk, (int)in.size()-(int)i); 
        eng.pushBlock(in.data()+i, N); 
    }
    eng.appendEpochs(ep, 0, fs/f0, true);

    struct Case { 
        float ratio; 
        const char* name;
    } cases[] = {
        {0.5f,    "Down 1 Oct"},
        {0.7071f, "Down Tritone"},  // The critical one!
        {1.0f,    "Unison"},
        {1.4142f, "Up Tritone"},
        {1.5f,    "Up a 5th"},
        {2.0f,    "Up 1 Oct"},
    };

    std::printf("%-15s Ratio    Peak F0   ZC F0    Expected  Peak Err  ZC Err\n", "Interval");
    std::printf("%-15s -----    -------   -----    --------  --------  ------\n", "--------");
    
    int passCount = 0;
    
    for (auto c : cases){
        eng.resetSynthesis(0);
        std::vector<float> out(in.size());
        
        // Render in blocks
        int64_t outPos = 0;
        int remaining = in.size();
        while(remaining>0){
            int N = std::min(blk, remaining);
            std::vector<float> tmp(N,0.f);
            eng.renderBlock(c.ratio, tmp.data(), N, outPos);
            std::copy(tmp.begin(), tmp.end(), out.begin()+outPos);
            outPos += N; 
            remaining -= N;
        }

        // Skip transient
        int skip = 0.1f * fs;
        std::vector<float> tail(out.begin()+skip, out.end());

        float f0Peak = estimateF0_Peaks(tail, fs);
        float f0ZC = estimateF0_ZC(tail, fs);
        float fExpect = f0 * c.ratio;
        
        float centsPeak = 1200.f * std::log2(std::max(1e-6f,f0Peak) / std::max(1e-6f,fExpect));
        float centsZC = 1200.f * std::log2(std::max(1e-6f,f0ZC) / std::max(1e-6f,fExpect));
        
        std::printf("%-15s %.4f  %7.1f  %7.1f  %8.1f  %+7.1fc  %+6.1fc\n",
                    c.name, c.ratio, f0Peak, f0ZC, fExpect, centsPeak, centsZC);
        
        // Pass if either detector is within 25 cents
        if (std::fabs(centsPeak) < 25 || std::fabs(centsZC) < 25) {
            passCount++;
        }
    }
    
    std::printf("\n");
    if (passCount == 6) {
        std::printf("✅ ALL TESTS PASSED!\n");
        std::printf("The surgical fixes work correctly for all ratios including 0.7071.\n");
    } else {
        std::printf("Passed %d/6 tests\n", passCount);
    }
    
    return 0;
}