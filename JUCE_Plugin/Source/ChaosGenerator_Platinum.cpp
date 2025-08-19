#include "ChaosGenerator_Platinum.h"
#include <algorithm>
#include <array>
#include <cstring>

// SIMD FTZ/DAZ (optional)
#if defined(__SSE2__)
  #include <immintrin.h>
#endif

namespace {
    constexpr double kPi      = 3.14159265358979323846;
    constexpr double kTwoPi   = 2.0 * kPi;
    constexpr float  kTinyF   = 1e-30f;
    constexpr double kTinyD   = 1e-300;
    constexpr float  kSilenceThresh = 1e-6f;

    // Denormal + NaN/Inf guards
    template <typename T>
    inline T flushDenorm(T x) noexcept {
    #if defined(__SSE2__)
        if constexpr (std::is_same_v<T,float>) {
            // force FTZ/DAZ through an add
            return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(x), _mm_set_ss(0.0f)));
        }
    #endif
        const T ax = std::abs(x);
        if constexpr (std::is_same_v<T,float>) {
            return ax < kTinyF ? T(0) : x;
        } else {
            return ax < kTinyD ? T(0) : x;
        }
    }

    template <typename T>
    inline bool isBad(T v) noexcept {
        return !std::isfinite(v);
    }

    inline float fastTanh(float x) noexcept {
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    // Smooth parameter (lock-free target, local state)
    class ParamSmoother {
    public:
        void prepare(double sr, float ms) noexcept {
            sampleRate = sr;
            setTime(ms);
            current = target.load(std::memory_order_relaxed);
        }
        void setTime(float ms) noexcept {
            timeMs = std::max(0.02f, ms);
            const double t = timeMs * 0.001;
            coeff = static_cast<float>(1.0 - std::exp(-1.0 / (t * std::max(1.0, sampleRate))));
        }
        void setTarget(float v) noexcept { target.store(v, std::memory_order_relaxed); }
        void snap(float v) noexcept { current = v; target.store(v, std::memory_order_relaxed); }
        inline float tick() noexcept {
            const float t = target.load(std::memory_order_relaxed);
            current += coeff * (t - current);
            return flushDenorm(current);
        }
        float value() const noexcept { return current; }
    private:
        std::atomic<float> target{0.0f};
        float current{0.0f};
        float coeff{0.01f};
        float timeMs{10.0f};
        double sampleRate{44100.0};
    };

    // Chaos type enum (internal)
    enum class ChaosType : int {
        Lorenz = 0, Rossler, Henon, Logistic, Ikeda, Duffing, Count
    };

    // Base chaos system
    struct ChaosSystem {
        virtual ~ChaosSystem() = default;
        virtual void reset(double seed) = 0;
        virtual void step(double dt) = 0;       // must be bounded & safe
        virtual double out() const = 0;         // [-1..1] preferred
        virtual void harden() noexcept {}       // scrub bad state if any
    protected:
        static inline double clamp(double x, double lim=100.0) noexcept {
            return std::max(-lim, std::min(lim, x));
        }
        static inline double safed(double v) noexcept {
            return isBad(v) ? 0.0 : v;
        }
    };

    struct Lorenz : ChaosSystem {
        // x' = σ(y-x), y' = x(ρ-z) - y, z' = xy - βz
        double x{0.1}, y{0.0}, z{0.0};
        void reset(double seed) override {
            x = 0.1 + 0.01 * seed; y = 0.2 + 0.02 * seed; z = 0.3 + 0.03 * seed;
        }
        void harden() noexcept override {
            if (isBad(x) || isBad(y) || isBad(z)) { x=0.1; y=0.0; z=0.0; }
            x = flushDenorm(x); y = flushDenorm(y); z = flushDenorm(z);
        }
        void step(double dt) override {
            const double sigma=10.0, rho=28.0, beta=8.0/3.0;
            // Sub-step RK4 with caps
            const int maxSub = 8;
            double h = std::clamp(dt, 1.0/192000.0, 0.01);
            int n = std::min(maxSub, std::max(1, int(std::ceil(dt / (1.0/44100.0)))));
            h = dt / n;

            for (int i=0;i<n;i++) {
                // k1
                double k1x = sigma * (y - x);
                double k1y = x * (rho - z) - y;
                double k1z = x * y - beta * z;

                // k2
                double x2 = x + 0.5*h*k1x;
                double y2 = y + 0.5*h*k1y;
                double z2 = z + 0.5*h*k1z;
                double k2x = sigma * (y2 - x2);
                double k2y = x2 * (rho - z2) - y2;
                double k2z = x2 * y2 - beta * z2;

                // k3
                double x3 = x + 0.5*h*k2x;
                double y3 = y + 0.5*h*k2y;
                double z3 = z + 0.5*h*k2z;
                double k3x = sigma * (y3 - x3);
                double k3y = x3 * (rho - z3) - y3;
                double k3z = x3 * y3 - beta * z3;

                // k4
                double x4 = x + h*k3x;
                double y4 = y + h*k3y;
                double z4 = z + h*k3z;
                double k4x = sigma * (y4 - x4);
                double k4y = x4 * (rho - z4) - y4;
                double k4z = x4 * y4 - beta * z4;

                x = clamp(x + (h/6.0)*(k1x + 2*k2x + 2*k3x + k4x), 200.0);
                y = clamp(y + (h/6.0)*(k1y + 2*k2y + 2*k3y + k4y), 200.0);
                z = clamp(z + (h/6.0)*(k1z + 2*k2z + 2*k3z + k4z), 200.0);
                harden();
            }
        }
        double out() const override {
            return std::tanh(x * 0.05); // gentle normalization
        }
    };

    struct Rossler : ChaosSystem {
        double x{1.0}, y{0.0}, z{0.0};
        void reset(double seed) override { x = 0.1 + 0.01*seed; y = 0.1; z = 0.1; }
        void harden() noexcept override {
            if (isBad(x)||isBad(y)||isBad(z)) { x=0.1; y=0.1; z=0.1; }
            x=flushDenorm(x); y=flushDenorm(y); z=flushDenorm(z);
        }
        void step(double dt) override {
            // Simple semi-implicit Euler with caps
            const double a=0.2, b=0.2, c=5.7;
            const int maxIter = 16;
            int n = std::clamp(int(std::ceil(dt / (1.0/88200.0))), 1, maxIter);
            double h = dt / n;
            for (int i=0;i<n;i++) {
                double dx = -y - z;
                double dy = x + a*y;
                double dz = b + z*(x - c);
                x = clamp(x + h*dx, 200.0);
                y = clamp(y + h*dy, 200.0);
                z = clamp(z + h*dz, 200.0);
                harden();
            }
        }
        double out() const override { return std::tanh(x * 0.1); }
    };

    struct Henon : ChaosSystem {
        double x{0.0}, y{0.0};
        double tAcc{0.0};
        void reset(double seed) override { x = 0.1*seed; y = 0.1*seed; tAcc=0.0; }
        void harden() noexcept override {
            if (isBad(x)||isBad(y)) { x=0.0; y=0.0; }
            x=flushDenorm(x); y=flushDenorm(y);
        }
        void step(double dt) override {
            tAcc += dt;
            // iterate at fixed ~1 kHz; limit loop count
            const double period = 0.001;
            int iter = 0; const int kMax = 8;
            while (tAcc >= period && iter < kMax) {
                tAcc -= period; iter++;
                const double a=1.4, b=0.3;
                const double xn = 1.0 - a*x*x + y;
                const double yn = b*x;
                x = clamp(xn, 20.0); y = clamp(yn, 20.0);
                harden();
            }
        }
        double out() const override { return std::tanh(x); }
    };

    struct Logistic : ChaosSystem {
        double x{0.5};
        double tAcc{0.0};
        void reset(double seed) override { x = std::clamp(0.1 + 0.8*std::fmod(std::abs(seed),1.0), 0.001, 0.999); tAcc=0.0; }
        void harden() noexcept override {
            if (isBad(x)) x = 0.5;
            x = std::clamp(x, 0.001, 0.999);
            x = flushDenorm(x);
        }
        void step(double dt) override {
            tAcc += dt;
            const double period = 0.0005; // 2 kHz
            int iter = 0; const int kMax=16;
            while (tAcc >= period && iter < kMax) {
                tAcc -= period; iter++;
                const double r = 3.9;
                x = r * x * (1.0 - x);
                harden();
            }
        }
        double out() const override { return 2.0*x - 1.0; }
    };

    struct Ikeda : ChaosSystem {
        double x{0.1}, y{0.1};
        double tAcc{0.0};
        void reset(double seed) override { x=0.1*seed; y=0.1*seed; tAcc=0.0; }
        void harden() noexcept override {
            if (isBad(x)||isBad(y)) { x=0.1; y=0.1; }
            x=flushDenorm(x); y=flushDenorm(y);
            x = clamp(x, 200.0); y = clamp(y, 200.0);
        }
        void step(double dt) override {
            tAcc += dt;
            const double period = 0.001; // 1kHz
            int iter=0; const int kMax=8;
            while (tAcc >= period && iter < kMax) {
                tAcc -= period; iter++;
                const double u=0.918;
                const double t = 0.4 - 6.0 / std::max(1.0, 1.0 + x*x + y*y);
                const double ct = std::cos(t), st = std::sin(t);
                const double xn = 1.0 + u*(x*ct - y*st);
                const double yn = u*(x*st + y*ct);
                x = xn; y = yn;
                harden();
            }
        }
        double out() const override { return std::tanh(0.25*(x+y)); }
    };

    struct Duffing : ChaosSystem {
        double x{0.0}, v{0.0};
        double phase{0.0};
        void reset(double seed) override { x = 0.1*seed; v=0.0; phase=0.0; }
        void harden() noexcept override {
            if (isBad(x)||isBad(v)||isBad(phase)) { x=0.0; v=0.0; phase=0.0; }
            x=flushDenorm(x); v=flushDenorm(v);
            x=clamp(x, 200.0); v=clamp(v, 200.0);
            if (phase > 1e6) phase = std::fmod(phase, kTwoPi);
        }
        void step(double dt) override {
            // x'' + δ x' + α x + β x^3 = γ cos(ω t)
            const double delta=0.3, alpha=-1.0, beta=1.0, gamma=0.35, omega=1.2;
            const int maxSub = 8;
            int n = std::clamp(int(std::ceil(dt / (1.0/88200.0))), 1, maxSub);
            const double h = dt / n;
            for (int i=0;i<n;i++) {
                const double drive = gamma * std::cos(omega * phase);

                // RK2 (midpoint)
                const double k1x = v;
                const double k1v = -delta*v - alpha*x - beta*x*x*x + drive;

                const double xm = x + 0.5*h*k1x;
                const double vm = v + 0.5*h*k1v;
                const double k2x = vm;
                const double k2v = -delta*vm - alpha*xm - beta*xm*xm*xm + drive;

                x = clamp(x + h * k2x, 200.0);
                v = clamp(v + h * k2v, 200.0);
                phase += h;
                if (phase > kTwoPi) phase -= kTwoPi;
                harden();
            }
        }
        double out() const override { return std::tanh(0.5 * x); }
    };

    // Simple one-pole per channel (used for filter mod)
    struct OnePole {
        float z{0.0f};
        void reset() noexcept { z = 0.0f; }
        inline float process(float x, float a) noexcept {
            // y[n] = a*x + (1-a)*y[n-1]
            z = a*x + (1.0f - a)*z;
            return flushDenorm(z);
        }
    };

    // Safe mix helper
    inline float mix(float dry, float wet, float m) noexcept {
        m = std::clamp(m, 0.0f, 1.0f);
        return flushDenorm(dry*(1.0f - m) + wet*m);
    }

    // Global FTZ/DAZ enable
    struct FTZGuard {
        FTZGuard() {
        #if defined(__SSE2__)
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
        #endif
        }
    } static g_ftzGuard;
} // namespace

// =========================== Impl ===========================================
struct ChaosGenerator_Platinum::Impl {
    // Core
    double sampleRate{44100.0};
    int blockSize{512};

    // Chaos systems
    std::array<std::unique_ptr<ChaosSystem>, static_cast<int>(ChaosType::Count)> systems;
    ChaosType current{ChaosType::Lorenz};

    // Parameter smoothing (match original param roles)
    ParamSmoother pRate;      // 0..1
    ParamSmoother pDepth;     // 0..1
    ParamSmoother pSmooth;    // 0..1 (output smoothing)
    ParamSmoother pMix;       // 0..1
    std::atomic<int> pType{0};
    std::atomic<int> pTarget{0};
    std::atomic<uint32_t> pSeed{12345};
    std::atomic<bool> pSync{false};

    // Per-channel helpers
    static constexpr int kMaxChannels = 2;
    std::array<OnePole, kMaxChannels> onePoleLP;

    // Internal state
    double chaosSmoothed{0.0};
    float  lastDepth{0.0f};

    // Mod target enum mirrors original switch cases
    enum ModTarget { ModPitch=0, ModFilter, ModAmp, ModPan, ModTargetCount };

    void prepare(double sr, int bs) {
        sampleRate = std::max(8000.0, sr);
        blockSize = std::max(1, bs);

        // Build chaos systems
        systems[0] = std::make_unique<Lorenz>();
        systems[1] = std::make_unique<Rossler>();
        systems[2] = std::make_unique<Henon>();
        systems[3] = std::make_unique<Logistic>();
        systems[4] = std::make_unique<Ikeda>();
        systems[5] = std::make_unique<Duffing>();

        // Smoothers
        pRate.prepare(sampleRate, 10.0f);
        pDepth.prepare(sampleRate, 15.0f);
        pSmooth.prepare(sampleRate, 50.0f);
        pMix.prepare(sampleRate, 5.0f);

        reset();
    }

    void reset() {
        // Reset chaos output smoothing
        chaosSmoothed = 0.0;
        
        // Reset all one-pole filters
        for (auto& op : onePoleLP) op.reset();

        // Reset all chaos systems with current seed
        const double seed = (pSeed.load(std::memory_order_relaxed) / 4294967295.0);
        for (auto& s : systems) {
            if (s) {
                s->reset(seed);
                s->harden(); // Ensure clean state after reset
            }
        }

        // Reset parameter smoothers to current targets for deterministic start
        pRate.snap(pRate.value());
        pDepth.snap(pDepth.value());
        pSmooth.snap(pSmooth.value());
        pMix.snap(pMix.value());

        // Reset internal tracking state
        lastDepth = pDepth.value();
        
        // Additional safety: ensure atomic parameters are in valid ranges
        const int currentType = pType.load(std::memory_order_relaxed);
        if (currentType < 0 || currentType >= static_cast<int>(ChaosType::Count)) {
            pType.store(0, std::memory_order_relaxed); // Default to Lorenz
        }
        
        const int currentTarget = pTarget.load(std::memory_order_relaxed);
        if (currentTarget < 0 || currentTarget >= static_cast<int>(ModTargetCount)) {
            pTarget.store(0, std::memory_order_relaxed); // Default to ModPitch
        }
    }

    inline double stepChaos() noexcept {
        // Convert normalized rate -> speed scaling
        const float rateN = pRate.tick(); // 0..1
        // Map to ~0.1x..10x speed factor
        const double speed = 0.1 + 9.9 * std::clamp(double(rateN), 0.0, 1.0);

        // If sync enabled we could map to host tempo externally; here we just apply speed
        const double dt = std::clamp(speed / sampleRate, 1.0/192000.0, 0.01);

        // Step active system safely
        const int idx = std::clamp(pType.load(std::memory_order_relaxed), 0, (int)ChaosType::Count-1);
        auto& sys = systems[(size_t)idx];
        sys->step(dt);

        double y = sys->out();
        if (isBad(y)) { sys->harden(); y = 0.0; }
        y = std::clamp(y, -1.0, 1.0);

        // Output smoothing (0..1): convert to one-pole coeff
        const float smoothN = pSmooth.tick();
        const double a = std::clamp(1.0 - std::pow(0.001, 1.0 / (std::max(1.0, sampleRate) * (0.002 + 0.998*smoothN))), 0.0, 1.0);
        chaosSmoothed += a * (y - chaosSmoothed);
        chaosSmoothed = flushDenorm(chaosSmoothed);

        return chaosSmoothed;
    }

    void processBlock(juce::AudioBuffer<float>& buffer) {
        const int numCh = std::min(buffer.getNumChannels(), kMaxChannels);
        const int n = buffer.getNumSamples();

        // Fast path: silence in -> still do modulation (it can create sound if Amp or Filter mod?), but keep cheap
        // We'll not bypass to preserve effect behavior. Still safe.

        // Pull smoothers to local per-sample
        for (int i=0;i<n;++i) {
            const float depth = pDepth.tick();
            const float mixAmt = pMix.tick();
            const int  target = std::clamp(pTarget.load(std::memory_order_relaxed), 0, (int)ModTargetCount-1);

            // Generate chaos once per sample (shared across channels)
            const float mod = static_cast<float>(stepChaos()); // in [-1..1]

            for (int ch=0; ch<numCh; ++ch) {
                float* out = buffer.getWritePointer(ch);
                float dry = out[i];
                float wet = dry;

                switch (target) {
                    case ModPitch:
                        // Placeholder lightweight pitch-ish modulation: gentle PM on the sample itself (safe)
                        // NOTE: real pitch would need delayline; we avoid allocations here.
                        wet = dry * (1.0f + 0.05f * depth * mod);
                        break;

                    case ModFilter: {
                        // One-pole brightness mod: map chaos to cutoff coefficient
                        // cutoff = 0.1..0.9 roughly
                        const float cutoff = std::clamp(0.5f + 0.4f * depth * mod, 0.05f, 0.98f);
                        wet = onePoleLP[ch].process(dry, cutoff);
                        break;
                    }

                    case ModAmp:
                        // Tremolo
                        wet = dry * (1.0f + depth * mod);
                        break;

                    case ModPan:
                        // Auto-pan: apply opposite gains per channel
                        if (numCh == 1) {
                            wet = dry; // mono: do nothing fancy
                        } else {
                            const float pan = 0.5f * depth * mod; // -0.5..0.5
                            const float gl = std::clamp(1.0f - pan, 0.0f, 2.0f);
                            const float gr = std::clamp(1.0f + pan, 0.0f, 2.0f);
                            wet = dry * (ch==0 ? gl : gr);
                        }
                        break;
                }

                // Safety limit & mix
                wet = std::clamp(wet, -10.0f, 10.0f);
                out[i] = mix(dry, wet, mixAmt);
            }

            // Periodic sanity scrub if depth changed abruptly (avoids sudden explosions)
            if (i % 256 == 0) {
                if (!std::isfinite(chaosSmoothed)) chaosSmoothed = 0.0;
                for (auto& sys : systems) if (sys) sys->harden();
                lastDepth = depth;
            }
        }
    }
};

// =========================== Public API =====================================
ChaosGenerator_Platinum::ChaosGenerator_Platinum()
: pImpl(std::make_unique<Impl>()) {}

ChaosGenerator_Platinum::~ChaosGenerator_Platinum() = default;

void ChaosGenerator_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->prepare(sampleRate, samplesPerBlock);
}

void ChaosGenerator_Platinum::reset() {
    pImpl->reset();
}

void ChaosGenerator_Platinum::process(juce::AudioBuffer<float>& buffer) {
    // DenormalGuard guard; // TODO: Add denormal protection
    
    // Light safety: clamp buffer size assumptions, no allocations here
    if (buffer.getNumSamples() <= 0) return;
    pImpl->processBlock(buffer);
    
    // scrubBuffer(buffer); // TODO: Add buffer scrubbing
}

void ChaosGenerator_Platinum::updateParameters(const std::map<int, float>& params) {
    // Only copy what's present (keep lock-free targets)
    auto get = [&](int id, float def)->float {
        auto it = params.find(id);
        return (it != params.end()) ? it->second : def;
    };

    // kRate (0..1). We use it as speed scaler internally.
    pImpl->pRate.setTarget(std::clamp(get(kRate, 0.5f), 0.0f, 1.0f));

    // kDepth (0..1)
    pImpl->pDepth.setTarget(std::clamp(get(kDepth, 0.5f), 0.0f, 1.0f));

    // kType (0..1 -> 0..5)
    if (params.count(kType)) {
        const float v = std::clamp(get(kType, 0.0f), 0.0f, 1.0f);
        const int idx = std::clamp(int(v * (int)ChaosType::Count - 1e-4f), 0, (int)ChaosType::Count-1);
        pImpl->pType.store(idx, std::memory_order_relaxed);
        // Re-seed current system slightly so a type jump is stable and bounded
        const double seed = (pImpl->pSeed.load(std::memory_order_relaxed) / 4294967295.0);
        if (pImpl->systems[(size_t)idx]) pImpl->systems[(size_t)idx]->reset(seed);
    }

    // kSmoothing (0..1)
    pImpl->pSmooth.setTarget(std::clamp(get(kSmoothing, 0.5f), 0.0f, 1.0f));

    // kModTarget (0..1 -> 0..3)
    if (params.count(kModTarget)) {
        const float v = std::clamp(get(kModTarget, 0.0f), 0.0f, 1.0f);
        const int t = std::clamp(int(v * (int)Impl::ModTargetCount - 1e-4f), 0, (int)Impl::ModTargetCount-1);
        pImpl->pTarget.store(t, std::memory_order_relaxed);
    }

    // kSync (0/1) — available for host sync mapping if you wire transport in
    if (params.count(kSync)) {
        pImpl->pSync.store(get(kSync, 0.0f) > 0.5f, std::memory_order_relaxed);
    }

    // kSeed (0..1 -> uint32_t)
    if (params.count(kSeed)) {
        const float v = std::clamp(get(kSeed, 0.0f), 0.0f, 1.0f);
        const uint32_t s = (uint32_t)std::llround(v * 4294967295.0);
        pImpl->pSeed.store(s, std::memory_order_relaxed);
        const double seed = (s / 4294967295.0);
        for (auto& sys : pImpl->systems) if (sys) sys->reset(seed);
    }

    // kMix (0..1)
    pImpl->pMix.setTarget(std::clamp(get(kMix, 1.0f), 0.0f, 1.0f));
}

juce::String ChaosGenerator_Platinum::getParameterName(int index) const {
    switch (index) {
        case kRate:      return "Rate";
        case kDepth:     return "Depth";
        case kType:      return "Type";
        case kSmoothing: return "Smoothing";
        case kModTarget: return "Target";
        case kSync:      return "Sync";
        case kSeed:      return "Seed";
        case kMix:       return "Mix";
        default:         return {};
    }
}