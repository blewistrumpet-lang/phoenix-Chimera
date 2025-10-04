#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h" // DenormalGuard, scrubBuffer, DCBlocker
#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <cmath>
#include <algorithm>

/**
 * VintageConsoleEQ_Studio — Project Chimera v3.0 Phoenix
 * Dr. Sarah Chen — character EQ inspired by Neve/SSL/API topologies
 *
 * Design goals:
 * - Console-voiced curves (Orfanidis bell/shelf), stepped musical centers per console
 * - Proportional-Q (boost narrows, cut broadens per-console law)
 * - Transformer/inductor coloration (frequency-dependent saturation & phase)
 * - Inter-band coupling matrix to mimic analog interactions
 * - 2× oversampling around nonlinear stage at 44.1/48k (auto-bypass ≥96k)
 * - RT-safe: no allocations in process(), FTZ/DAZ guard, scrub/ DCBlocker
 */

class VintageConsoleEQ_Studio : public EngineBase {
public:
    VintageConsoleEQ_Studio();
    ~VintageConsoleEQ_Studio() override;

    // EngineBase
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Vintage Console EQ Studio"; }
    int getNumParameters() const override { return 13; }
    juce::String getParameterName(int index) const override;

    // Console selection
    enum class ConsoleType { NEVE_1073, SSL_4000E, API_550A, CUSTOM };
    void selectConsole(ConsoleType type);

    // Parameters (IDs must align with your plugin parameter table)
    enum ParamID : int {
        // Frequency indices per band
        kLow_Index     = 0,   // Low shelf frequency index
        kLM_Index      = 2,   // Low-mid bell frequency index
        kHM_Index      = 4,   // High-mid bell frequency index
        kHigh_Index    = 6,   // High shelf frequency index

        // Band gains (±15 dB range mapped from 0-1)
        kLow_Gain_dB   = 1,   // Low shelf gain
        kLM_Gain_dB    = 3,   // Low-mid bell gain
        kHM_Gain_dB    = 5,   // High-mid bell gain
        kHigh_Gain_dB  = 7,   // High shelf gain

        // Global controls
        kDrive         = 8,   // 0..1 input drive into transformer stage
        kConsoleType   = 9,   // 0=Neve,0.33=SSL,0.66=API,1=CUSTOM
        kQBias         = 10,  // Q character (0..1 -> min..max in console law)
        kNoiseOn       = 11,  // vintage hiss
        kOutputTrim_dB = 12   // output trim (±24 dB range mapped from 0-1)
    };

private:
    // -------- Config --------
    static constexpr int kMaxChannels   = 2;
    static constexpr int kCtrlInterval  = 32;         // samples per control tick
    static constexpr int kXfadeSamples  = 64;         // clickless swap
    static constexpr int kAnalyzerDecim = 8;          // if you tap later
    static constexpr float kMaxGain_dB  = 16.0f;

    // -------- Proportional-Q laws (per-console) --------
    static inline float propQ(float gainDB, ConsoleType c, float qBias) noexcept {
        const float ag = std::min(std::abs(gainDB), kMaxGain_dB) / kMaxGain_dB; // 0..1
        switch (c) {
            case ConsoleType::NEVE_1073: { // 0.7 → 2.0
                const float qMin = 0.7f, qMax = 2.0f;
                const float curve = 0.85f + 0.3f*qBias;
                return qMin + (qMax - qMin) * std::pow(ag, curve);
            }
            case ConsoleType::SSL_4000E: { // 0.5 → 3.0
                const float qMin = 0.5f, qMax = 3.0f;
                const float curve = 1.0f + 0.6f*qBias;
                return qMin + (qMax - qMin) * std::pow(ag, curve);
            }
            case ConsoleType::API_550A: { // reciprocal feel; cuts slightly narrower
                const float qBoostMin = 0.9f, qBoostMax = 2.2f;
                const float qCutMin   = 1.2f, qCutMax   = 2.8f;
                if (gainDB >= 0.0f) {
                    return qBoostMin + (qBoostMax - qBoostMin) * std::pow(ag, 0.9f + 0.5f*qBias);
                } else {
                    return qCutMin   + (qCutMax   - qCutMin)   * std::pow(ag, 0.8f + 0.5f*qBias);
                }
            }
            default: {
                const float qMin = 0.7f, qMax = 2.5f;
                return qMin + (qMax - qMin) * ag;
            }
        }
    }

    // -------- Filter coeffs (Orfanidis-matched bell/shelf) --------
    struct BiquadCoeffs { float b0=1.f,b1=0.f,b2=0.f,a1=0.f,a2=0.f; };
    struct BiquadTDF2 {
        double z1=0.0, z2=0.0;
        BiquadCoeffs c;
        inline float process(float x) noexcept {
            const double y = (double)c.b0 * x + z1;
            z1 = (double)c.b1 * x - (double)c.a1 * y + z2;
            z2 = (double)c.b2 * x - (double)c.a2 * y;
            return (float)y;
        }
        inline void reset() noexcept { z1=z2=0.0; }
    };
    static void bellOrfanidis(BiquadCoeffs& out, double fs, double f0, double Q, double gainDB);
    static void shelfOrfanidis(BiquadCoeffs& out, double fs, double f0, double slope, double gainDB, bool highShelf);

    struct XfadeGain { float gA=1.f,gB=1.f; inline void set(float mA,float mB){ gA = (mA>1e-9f)? 1.f/mA:1.f; gB=(mB>1e-9f)?1.f/mB:1.f; } };
    struct FilterAB {
        BiquadTDF2 A,B; bool useA=true; int xfadeCtr=0; XfadeGain g;
        inline float process(float x) noexcept {
            if (xfadeCtr>0) {
                const float ya=A.process(x), yb=B.process(x);
                const float t = 1.f - (float)xfadeCtr / (float)kXfadeSamples; --xfadeCtr;
                return ya*g.gA*(1.f-t) + yb*g.gB*t;
            }
            return (useA?A:B).process(x);
        }
        inline void reset(){ A.reset(); B.reset(); xfadeCtr=0; g={}; }
    };

    // -------- Bands --------
    enum Band : int { LOW=0, LM=1, HM=2, HIGH=3, NBANDS=4 };
    struct BandState {
        // steppers: index -> frequency per console
        int   idx = 0;
        float gainDB = 0.f; // ±16 dB
        float Qbias  = 0.5f;
        BiquadCoeffs last; bool haveLast=false;
        FilterAB filt[kMaxChannels];
        bool isShelf = false; // LOW/HIGH shelves; LM/HM bells
    };

    // -------- Nonlinear stages (transformer/inductor) --------
    struct XformStage {
        // Simple frequency-dependent drive shaping + soft clip; LF "iron" emphasis
        float drive = 0.0f; // 0..1
        inline void reset() {}
        inline float process(float x, float instHz, float fs) noexcept {
            // frequency-weighted drive: more LF saturation
            const float lfEmph = 1.0f + 1.5f * (1.0f / (1.0f + (instHz/200.0f)*(instHz/200.0f)));
            const float d = std::clamp(drive * lfEmph, 0.0f, 2.0f);
            // Soft clip with slight asymmetry for even/odd mix
            const float k = 0.8f + 0.4f*d;
            float y = std::tanh(k*(x + 0.03f*x*x)); // asymmetry -> even harmonics
            // HF loss sim (eddy currents): 1st-order LP tilt toward HF loss when driven
            const float hfLoss = 1.0f / (1.0f + 0.0008f * (float)fs / std::max(10.0f, instHz));
            return y * (0.9f + 0.1f*hfLoss);
        }
    };
    struct InductorInteraction {
        // Subtle resonant bias around band centers -> phase bend + slight compression
        float state[kMaxChannels]{0.f,0.f};
        inline void reset(){ state[0]=state[1]=0.f; }
        inline float process(int ch, float x, float centerHz, float fs, float strength) noexcept {
            // Under-damped 1st-order resonator-ish one-pole around center
            const float w = 2.f * float(M_PI) * (float)centerHz / (float)fs;
            const float a = std::exp(-w * 0.15f);        // damping
            state[ch] = a*state[ch] + (1.f-a)*x;
            const float sat = std::tanh(state[ch] * (0.5f + 3.0f*strength));
            return 0.98f*x + strength*(sat - state[ch]*0.02f); // gentle "iron" cushion
        }
    };

    // -------- Inter-band coupling --------
    struct Coupling {
        // Low  LM   HM   High
        float M[NBANDS][NBANDS] = {
            {1.00f, 0.05f, 0.00f, 0.00f},
            {0.05f, 1.00f, 0.08f, 0.00f},
            {0.00f, 0.08f, 1.00f, 0.05f},
            {0.00f, 0.00f, 0.05f, 1.00f}
        };
        inline void apply(float gIn[NBANDS], float gOut[NBANDS]) const noexcept {
            for (int i=0;i<NBANDS;++i){ float s=0.f; for(int j=0;j<NBANDS;++j) s += M[i][j]*gIn[j]; gOut[i]=s; }
        }
    };

    // -------- Oversampling (2× halfband, matched polyphase) --------
    struct Halfband2x {
        static constexpr int NTAPS = 31;
        static constexpr int PH    = (NTAPS-1)/2;
        static constexpr float h[NTAPS] = {
            -0.0002346f, 0.0f,  0.0019834f, 0.0f, -0.0077187f, 0.0f, 0.0216015f, 0.0f,
             -0.0508307f, 0.0f, 0.1103840f, 0.0f, -0.2798810f, 0.0f, 0.5000000f, 0.0f,
             -0.2798810f, 0.0f, 0.1103840f, 0.0f, -0.0508307f, 0.0f, 0.0216015f, 0.0f,
             -0.0077187f, 0.0f, 0.0019834f, 0.0f, -0.0002346f
        };
        static constexpr int NE = (NTAPS+1)/2;
        static constexpr int NO = NTAPS/2;
        std::array<float,NE> zLe{}, zRe{};
        std::array<float,NO> zLo{}, zRo{};
        int ie=0, io=0;
        static inline float dotRevE(const float* he, const std::array<float,NE>& z, int idx) {
            float acc=0.f; int i=idx; for(int n=0;n<NE;++n){ acc += he[n]*z[i]; if(--i<0) i=NE-1; } return acc;
        }
        static inline float dotRevO(const float* ho, const std::array<float,NO>& z, int idx) {
            float acc=0.f; int i=idx; for(int n=0;n<NO;++n){ acc += ho[n]*z[i]; if(--i<0) i=NO-1; } return acc;
        }
        void reset(){ zLe.fill(0.f); zRe.fill(0.f); zLo.fill(0.f); zRo.fill(0.f); ie=io=0; }
        inline void upsample(float inL,float inR,float& eL,float& oL,float& eR,float& oR) {
            static float he[NE]; static float ho[NO]; static bool init=false;
            if(!init){ for(int n=0;n<NE;++n) he[n]=h[2*n]; for(int n=0;n<NO;++n) ho[n]=h[2*n+1]; init=true; }
            if(++ie>=NE) ie=0; zLe[ie]=inL; zRe[ie]=inR; eL=dotRevE(he,zLe,ie); eR=dotRevE(he,zRe,ie);
            if(++io>=NO) io=0; zLo[io]=inL; zRo[io]=inR; oL=dotRevO(ho,zLo,io); oR=dotRevO(ho,zRo,io);
        }
        inline void downsample(float eL,float oL,float eR,float oR,float& outL,float& outR) {
            static float he[NE]; static float ho[NO]; static bool init=false;
            if(!init){ for(int n=0;n<NE;++n) he[n]=h[2*n]; for(int n=0;n<NO;++n) ho[n]=h[2*n+1]; init=true; }
            if(++ie>=NE) ie=0; zLe[ie]=eL; zRe[ie]=eR; float lpL=dotRevE(he,zLe,ie); float lpR=dotRevE(he,zRe,ie);
            if(++io>=NO) io=0; zLo[io]=oL; zRo[io]=oR; float hpL=dotRevO(ho,zLo,io); float hpR=dotRevO(ho,zRo,io);
            outL = lpL + hpL; outR = lpR + hpR;
        }
    };

    // -------- Internal helpers --------
    void controlTick(); // runs every kCtrlInterval samples
    void rebuildBandsIfNeeded(int ch, Band b, float centerHz, bool isShelf);
    static float magAtW(const BiquadCoeffs& c, float w);
    static inline float dbToLin(float dB){ return std::pow(10.0f, dB*0.05f); }

    // -------- Musical centers (per-console) --------
    void loadCenters();

    // -------- State --------
    double fs_=48000.0; int blockSize_=0;
    ConsoleType console_=ConsoleType::NEVE_1073;

    // stepped centers per band for current console
    std::array<float,12> lowCenters_{}, lmCenters_{}, hmCenters_{}, highCenters_{};
    int lowCount_=0, lmCount_=0, hmCount_=0, highCount_=0;

    BandState bands_[NBANDS];
    float gainsEffective_[NBANDS] = {0,0,0,0}; // after coupling
    Coupling coupling_;

    XformStage xform_;
    InductorInteraction inductor_;

    float outputTrim_=0.f; float drive_=0.f; bool bypass_=false; bool noiseOn_=false; int osMode_=0; // 0=auto,1=force on,2=force off
    int ctrlPhase_=0;

    DCBlocker dc_[kMaxChannels];
    Halfband2x hb_;
    uint32_t noiseSeed_=0x13579BDFu;
};