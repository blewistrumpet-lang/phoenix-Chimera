/**
 * VintageTubePreamp_Studio — Quality & Character Tests
 *
 * CI-friendly tests (no realtime deps):
 *  - THD vs input/drive across voicings
 *  - Oversampling alias rejection (10 kHz stress)
 *  - Tone stack frequency response sanity at musical centers
 *  - PSU sag timing and magnitude under burst load
 *  - Denormal / NaN safety on silence
 *  - Automation smoothness (no zipper/clicks)
 *
 * Assumptions:
 *  - JUCE headers available for AudioBuffer
 *  - VintageTubePreamp_Studio.{h,cpp} is in the include path
 */

#include "JuceHeader.h"
#include <cmath>
#include <map>
#include <cassert>
#include <cstdio>
#include "VintageTubePreamp_Studio.h"

// ---------- Small helpers ----------
static inline double dbFromLin(double x){ return 20.0 * std::log10(std::max(1e-12, x)); }

static double rmsdB(const juce::AudioBuffer<float>& buf,int ch){
    const int N=buf.getNumSamples(); const float* x=buf.getReadPointer(ch);
    long double acc=0.0L; for(int n=0;n<N;++n) acc += (long double)x[n]*x[n];
    return dbFromLin(std::sqrt((double)(acc/(long double)N)));
}

static void renderSine(VintageTubePreamp_Studio& pre,
                       double fs, double f, double seconds,
                       juce::AudioBuffer<float>& out,
                       float amp = 0.5f)
{
    const int N=(int)std::ceil(seconds*fs);
    out.setSize(2,N); out.clear();
    float ph=0.f, w=(float)(2.0*M_PI*f/fs);
    for(int n=0;n<N;++n){ float s=amp*std::sin(ph); ph+=w; out.setSample(0,n,s); out.setSample(1,n,s); }
    pre.process(out);
}

// Single-bin Goertzel
static double goertzelMag(const float* x,int N,double fs,double f0){
    const double k = std::round((N*f0)/fs);
    const double w = 2.0*M_PI*k/N, cw=std::cos(w);
    double s0=0,s1=0,s2=0; for(int n=0;n<N;++n){ s0 = x[n] + 2*cw*s1 - s2; s2=s1; s1=s0; }
    const double mag2 = s1*s1 + s2*s2 - 2*cw*s1*s2;
    return std::sqrt(std::max(0.0,mag2))/N;
}

static double goertzelMagDB(const float* x,int N,double fs,double f0){
    return dbFromLin(goertzelMag(x,N,fs,f0));
}

static double thdDB(const float* x,int N,double fs,double fFund,int upToHarm=9){
    const double f1 = std::abs(goertzelMag(x,N,fs,fFund));
    double acc2 = 0.0;
    for(int k=2;k<=upToHarm;++k){
        const double hk = goertzelMag(x,N,fs,fFund*k);
        acc2 += hk*hk;
    }
    const double thd = std::sqrt(acc2) / std::max(1e-12, f1);
    return dbFromLin(thd);
}

static void setParams(VintageTubePreamp_Studio& pre,
                      VintageTubePreamp_Studio::Voicing v,
                      float inTrim_dB, float outTrim_dB,
                      float drive, float bright,
                      float bass, float mid, float treble,
                      float presence, float mic, float ghost,
                      float noise, int osMode)
{
    std::map<int,float> p;
    p[VintageTubePreamp_Studio::kBypass]        = 0.f;
    p[VintageTubePreamp_Studio::kVoicing]       = (float)v;
    p[VintageTubePreamp_Studio::kInputTrim_dB]  = inTrim_dB;
    p[VintageTubePreamp_Studio::kOutputTrim_dB] = outTrim_dB;
    p[VintageTubePreamp_Studio::kDrive]         = drive;
    p[VintageTubePreamp_Studio::kBright]        = bright;
    p[VintageTubePreamp_Studio::kBass]          = bass;
    p[VintageTubePreamp_Studio::kMid]           = mid;
    p[VintageTubePreamp_Studio::kTreble]        = treble;
    p[VintageTubePreamp_Studio::kPresence]      = presence;
    p[VintageTubePreamp_Studio::kMicMech]       = mic;
    p[VintageTubePreamp_Studio::kGhost]         = ghost;
    p[VintageTubePreamp_Studio::kNoise]         = noise;
    p[VintageTubePreamp_Studio::kOSMode]        = (float)osMode;
    pre.updateParameters(p);
}

// ---------- Tests ----------
int main(){
    constexpr double fs48 = 48000.0;
    constexpr double fs96 = 96000.0;

    // 1) THD vs drive (character check across voicings)
    {
        VintageTubePreamp_Studio pre;
        pre.prepareToPlay(fs48, 512);

        // Moderate drive, Fender voicing
        setParams(pre, VintageTubePreamp_Studio::FENDER_DLUX,
                  0.f, 0.f, 0.45f, 0.1f, 0.5f,0.5f,0.5f, 0.2f, 0.f,0.f, 0.f, /*OS auto*/0);

        juce::AudioBuffer<float> buf;
        renderSine(pre, fs48, 1000.0, 2.0, buf, 0.5f);
        const int N=buf.getNumSamples();
        const float* x = buf.getReadPointer(0);
        double thd = thdDB(x,N,fs48,1000.0,9);
        // Expect tube-ish THD around -35..-20 dB at moderate drive (loose CI bounds)
        assert(thd <= -18.0 && thd >= -45.0);
        std::printf("THD @1k, drive=0.45: %.1f dB [OK]\n", thd);
    }

    // 2) Oversampling alias rejection (10 kHz; compare OS off vs auto/on)
    {
        // Force OS OFF
        VintageTubePreamp_Studio preOff; preOff.prepareToPlay(fs48, 256);
        setParams(preOff, VintageTubePreamp_Studio::VOX_AC30,
                  +6.f, 0.f, 0.75f, 0.2f, 0.5f,0.5f,0.7f, 0.3f, 0.f,0.f, 0.f, /*OS off*/2);
        juce::AudioBuffer<float> bOff; renderSine(preOff, fs48, 10000.0, 2.0, bOff, 0.5f);

        // OS ON (auto will engage at 48k; force on anyway)
        VintageTubePreamp_Studio preOn; preOn.prepareToPlay(fs48, 256);
        setParams(preOn, VintageTubePreamp_Studio::VOX_AC30,
                  +6.f, 0.f, 0.75f, 0.2f, 0.5f,0.5f,0.7f, 0.3f, 0.f,0.f, 0.f, /*OS on*/1);
        juce::AudioBuffer<float> bOn; renderSine(preOn, fs48, 10000.0, 2.0, bOn, 0.5f);

        // At 48k, the 3rd harmonic (30 kHz) aliases to 18 kHz.
        const int N=bOff.getNumSamples();
        double aliasOff = goertzelMagDB(bOff.getReadPointer(0), N, fs48, 18000.0);
        double aliasOn  = goertzelMagDB(bOn .getReadPointer(0), N, fs48, 18000.0);

        // Expect significant reduction with 4× OS (>=10 dB better)
        assert(aliasOn + 10.0 <= aliasOff);
        std::printf("OS alias @18 kHz: off=%.1f dB, on=%.1f dB [OK]\n", aliasOff, aliasOn);
    }

    // 3) Tone stack response sanity (approximate musical centers)
    {
        VintageTubePreamp_Studio pre; pre.prepareToPlay(fs96, 512);
        // Fender: bass +treble up, mid down
        setParams(pre, VintageTubePreamp_Studio::FENDER_DLUX,
                  0.f, 0.f, 0.35f, 0.0f, 0.9f, 0.2f, 0.9f, 0.2f, 0.f,0.f, 0.f, 2); // OS off to speed up

        // Probe with long impulse for steady-state measurement
        const int N = 1<<15;
        juce::AudioBuffer<float> buf; buf.setSize(2,N); buf.clear();
        buf.setSample(0,0, 1.0f); buf.setSample(1,0, 1.0f);
        pre.process(buf);

        // Expect lift at ~80–100 Hz, dip ~400–700, lift ~3–5 kHz
        double dBL = goertzelMagDB(buf.getReadPointer(0),N,fs96,  90.0);
        double dBM = goertzelMagDB(buf.getReadPointer(0),N,fs96, 500.0);
        double dBH = goertzelMagDB(buf.getReadPointer(0),N,fs96,3500.0);

        // Relative to midband (~1 kHz)
        double ref = goertzelMagDB(buf.getReadPointer(0),N,fs96,1000.0);
        double L = dBL - ref;
        double M = dBM - ref;
        double H = dBH - ref;

        // Loose but meaningful expectations:
        // Bass lift > +3 dB, Mid dip < -1 dB, Treble lift > +2 dB
        assert(L > +3.0); assert(M < -1.0); assert(H > +2.0);
        std::printf("Tone stack (Fender): L=%.1f dB, M=%.1f dB, H=%.1f dB [OK]\n", L,M,H);
    }

    // 4) PSU sag timing (burst test)
    {
        VintageTubePreamp_Studio pre; pre.prepareToPlay(fs48, 256);
        setParams(pre, VintageTubePreamp_Studio::MARSHALL_PLEXI,
                  +6.f, 0.f, 0.8f, 0.0f, 0.5f,0.5f,0.5f, 0.3f, 0.f,0.f, 0.f, 1);

        // Build a burst: 80 ms silence, 250 ms loud 100 Hz, 80 ms silence
        const int Nsil = int(0.08*fs48), Nsig = int(0.25*fs48);
        const int N = Nsil + Nsig + Nsil;
        juce::AudioBuffer<float> buf; buf.setSize(2,N); buf.clear();
        float ph=0.f, w=(float)(2.0*M_PI*100.0/fs48);
        for(int n=Nsil; n<Nsil+Nsig; ++n){ float s=0.8f*std::sin(ph); ph+=w; buf.setSample(0,n,s); buf.setSample(1,n,s); }
        pre.process(buf);

        // Measure envelope drop over the first 120 ms of the burst
        auto* x = buf.getReadPointer(0);
        const int W = int(0.02*fs48);
        auto winRMS=[&](int start){ long double acc=0; for(int i=0;i<W;++i){ float v=x[start+i]; acc+=(long double)v*v; } return std::sqrt((double)(acc/W)); };
        double r0 = winRMS(Nsil + int(0.01*fs48));
        double r1 = winRMS(Nsil + int(0.12*fs48));

        // Expect sag: later RMS at least ~10% lower
        assert(r1 <= 0.9 * r0);
        std::printf("Sag: early %.3f -> late %.3f (%.0f%%) [OK]\n", r0, r1, 100.0*(r1/r0));
    }

    // 5) Denormal / NaN safety (silence through process)
    {
        VintageTubePreamp_Studio pre; pre.prepareToPlay(fs96, 1024);
        setParams(pre, VintageTubePreamp_Studio::VOX_AC30,
                  0.f, 0.f, 0.5f, 0.0f, 0.5f,0.5f,0.5f, 0.5f, 0.5f,0.5f, 0.0f, 0);
        juce::AudioBuffer<float> buf; buf.setSize(2, 32768); buf.clear();
        pre.process(buf);
        for(int n=0;n<buf.getNumSamples();++n){
            float v=buf.getSample(0,n);
            assert(std::isfinite(v));
        }
        std::printf("Denormal/NaN safety [OK]\n");
    }

    // 6) Automation smoothness (rapid parameter changes)
    {
        VintageTubePreamp_Studio pre; pre.prepareToPlay(fs48, 256);
        std::map<int,float> p; // start clean
        p[VintageTubePreamp_Studio::kBypass]=0.f; p[VintageTubePreamp_Studio::kOSMode]=1.f;
        pre.updateParameters(p);

        // White noise input
        juce::AudioBuffer<float> noise; const int N=16384; noise.setSize(2,N);
        uint32_t rnd=1u; for(int n=0;n<N;++n){ rnd = rnd*1664525u + 1013904223u; float s=((rnd>>8)*(1.f/8388608.f)-1.f)*0.25f; noise.setSample(0,n,s); noise.setSample(1,n,s); }

        // Process with intermittent, wide parameter slams
        for(int k=0;k<6;++k){
            std::map<int,float> pp;
            pp[VintageTubePreamp_Studio::kDrive]   = (k%2)? 0.85f : 0.25f;
            pp[VintageTubePreamp_Studio::kBass]    = (k%3)/2.0f;
            pp[VintageTubePreamp_Studio::kMid]     = ((k+1)%3)/2.0f;
            pp[VintageTubePreamp_Studio::kTreble]  = ((k+2)%3)/2.0f;
            pp[VintageTubePreamp_Studio::kPresence]= (k%2)? 0.7f : 0.2f;
            pre.updateParameters(pp);
            juce::AudioBuffer<float> tmp; tmp.makeCopyOf(noise);
            pre.process(tmp);

            // No single-sample spikes above sane bounds
            float maxStep=0.f; auto* x=tmp.getReadPointer(0);
            for(int n=1;n<N;++n){ maxStep = std::max(maxStep, std::abs(x[n]-x[n-1])); }
            assert(maxStep < 0.8f);
        }
        std::printf("Automation smoothness [OK]\n");
    }

    std::printf("All VintageTubePreamp_Studio tests passed.\n");
    return 0;
}