#include "ChaosGenerator_Platinum.h"
#include <cmath>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <array>
#include <immintrin.h>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

// Platform-specific denormal prevention
#if HAS_SSE2
static struct DenormGuard {
    DenormGuard() {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    }
} s_denormGuard;
#endif

namespace {
    // Constants for chaos generation
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kTwoPi = 2.0 * kPi;
    constexpr float kSmoothingMin = 0.0001f;
    constexpr double kMinTimeStep = 0.0001;
    constexpr double kMaxTimeStep = 0.1;
    constexpr float kSilenceThreshold = 1e-6f;
    
    // Chaos type enumeration
    enum ChaosType {
        kLorenz = 0,
        kRossler,
        kHenon,
        kLogistic,
        kIkeda,
        kDuffing,
        kNumTypes
    };
    
    // Inline denormal flushing
    template<typename T>
    inline T flushDenorm(T v) noexcept {
        #if HAS_SSE2
            if constexpr (std::is_same_v<T, float>) {
                return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(v), _mm_set_ss(0.0f)));
            }
        #endif
        constexpr T tiny = static_cast<T>(1.0e-38);
        return std::fabs(v) < tiny ? static_cast<T>(0) : v;
    }
    
    // Fast approximations
    inline float fastTanh(float x) noexcept {
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }
    
    inline float fastExp(float x) noexcept {
        x = 1.0f + x / 256.0f;
        x *= x; x *= x; x *= x; x *= x;
        x *= x; x *= x; x *= x; x *= x;
        return x;
    }
}

// Lock-free parameter smoother with denormal protection
class ParamSmoother {
public:
    void setSampleRate(double sr) noexcept {
        m_sampleRate = sr;
        updateCoeff();
    }
    
    void setTime(float ms) noexcept {
        m_timeMs = std::max(0.01f, ms);
        updateCoeff();
    }
    
    void setValue(float v) noexcept {
        m_target.store(v, std::memory_order_relaxed);
    }
    
    void snap(float v) noexcept {
        m_current = m_target = v;
    }
    
    inline float tick() noexcept {
        float t = m_target.load(std::memory_order_relaxed);
        m_current += m_coeff * (t - m_current);
        return flushDenorm(m_current);
    }
    
private:
    void updateCoeff() noexcept {
        if (m_sampleRate > 0 && m_timeMs > 0) {
            m_coeff = 1.0f - fastExp(-1000.0f / (m_timeMs * m_sampleRate));
            m_coeff = std::min(1.0f, std::max(0.0f, m_coeff));
        }
    }
    
    std::atomic<float> m_target{0.0f};
    float m_current{0.0f};
    float m_coeff{0.01f};
    float m_timeMs{5.0f};
    double m_sampleRate{44100.0};
};

// Base class for chaos systems with double precision state
class ChaosSystem {
public:
    virtual ~ChaosSystem() = default;
    
    // Initialize system with a seed value
    virtual void reset(double seed) = 0;
    
    // Advance the system by dt seconds
    virtual void step(double dt) = 0;
    
    // Get current output in [-1, 1] range
    virtual double getOutput() const = 0;
    
    // Get system stability metric
    virtual double getStability() const { return 1.0; }
    
protected:
    // Clamp to prevent numerical explosion
    inline double clamp(double x, double limit = 100.0) const noexcept {
        return std::max(-limit, std::min(limit, x));
    }
};

// Lorenz attractor - classic butterfly
class LorenzSystem : public ChaosSystem {
public:
    void reset(double seed) override {
        x = seed * 0.1;
        y = seed * 0.2;
        z = seed * 0.3;
    }
    
    void step(double dt) override {
        // Lorenz equations with stability modifications
        const double sigma = 10.0;
        const double rho = 28.0;
        const double beta = 8.0 / 3.0;
        
        // 4th order Runge-Kutta integration
        double k1x = sigma * (y - x);
        double k1y = x * (rho - z) - y;
        double k1z = x * y - beta * z;
        
        double x2 = x + 0.5 * dt * k1x;
        double y2 = y + 0.5 * dt * k1y;
        double z2 = z + 0.5 * dt * k1z;
        
        double k2x = sigma * (y2 - x2);
        double k2y = x2 * (rho - z2) - y2;
        double k2z = x2 * y2 - beta * z2;
        
        double x3 = x + 0.5 * dt * k2x;
        double y3 = y + 0.5 * dt * k2y;
        double z3 = z + 0.5 * dt * k2z;
        
        double k3x = sigma * (y3 - x3);
        double k3y = x3 * (rho - z3) - y3;
        double k3z = x3 * y3 - beta * z3;
        
        double x4 = x + dt * k3x;
        double y4 = y + dt * k3y;
        double z4 = z + dt * k3z;
        
        double k4x = sigma * (y4 - x4);
        double k4y = x4 * (rho - z4) - y4;
        double k4z = x4 * y4 - beta * z4;
        
        // Update with clamping for stability
        x = clamp(x + dt * (k1x + 2*k2x + 2*k3x + k4x) / 6.0);
        y = clamp(y + dt * (k1y + 2*k2y + 2*k3y + k4y) / 6.0);
        z = clamp(z + dt * (k1z + 2*k2z + 2*k3z + k4z) / 6.0);
        
        // Apply denormal protection
        x = flushDenorm(x);
        y = flushDenorm(y);
        z = flushDenorm(z);
    }
    
    double getOutput() const override {
        return fastTanh(x / 20.0);  // Normalized to [-1, 1]
    }
    
    double getStability() const override {
        double energy = x*x + y*y + z*z;
        return 1.0 / (1.0 + energy * 0.001);
    }
    
private:
    double x = 0.1, y = 0.0, z = 0.0;
};

// Rossler attractor - spiral chaos
class RosslerSystem : public ChaosSystem {
public:
    void reset(double seed) override {
        x = seed * 0.1;
        y = seed * 0.1;
        z = seed * 0.1;
    }
    
    void step(double dt) override {
        const double a = 0.2;
        const double b = 0.2;
        const double c = 5.7;
        
        // Rossler equations
        double dx = -y - z;
        double dy = x + a * y;
        double dz = b + z * (x - c);
        
        // Euler integration with stability
        x = clamp(x + dt * dx, 50.0);
        y = clamp(y + dt * dy, 50.0);
        z = clamp(z + dt * dz, 50.0);
        
        x = flushDenorm(x);
        y = flushDenorm(y);
        z = flushDenorm(z);
    }
    
    double getOutput() const override {
        return fastTanh(x / 10.0);
    }
    
private:
    double x = 1.0, y = 0.0, z = 0.0;
};

// Henon map - discrete chaos
class HenonSystem : public ChaosSystem {
public:
    void reset(double seed) override {
        x = seed * 0.1;
        y = seed * 0.1;
        n = 0;
    }
    
    void step(double dt) override {
        // Accumulate time and iterate when crossing threshold
        accumTime += dt;
        while (accumTime >= iterPeriod) {
            accumTime -= iterPeriod;
            
            // Henon map iteration
            const double a = 1.4;
            const double b = 0.3;
            
            double xNew = 1.0 - a * x * x + y;
            double yNew = b * x;
            
            x = clamp(xNew, 10.0);
            y = clamp(yNew, 10.0);
            
            x = flushDenorm(x);
            y = flushDenorm(y);
            
            n++;
        }
    }
    
    double getOutput() const override {
        return fastTanh(x);
    }
    
private:
    double x = 0.0, y = 0.0;
    double accumTime = 0.0;
    const double iterPeriod = 0.001;  // 1kHz iteration rate
    int n = 0;
};

// Logistic map - population dynamics
class LogisticSystem : public ChaosSystem {
public:
    void reset(double seed) override {
        x = 0.1 + seed * 0.8;  // Keep in (0,1)
        accumTime = 0.0;
    }
    
    void step(double dt) override {
        accumTime += dt;
        while (accumTime >= iterPeriod) {
            accumTime -= iterPeriod;
            
            // Logistic map with chaos at r=3.9
            const double r = 3.9;
            x = r * x * (1.0 - x);
            
            // Keep in valid range
            x = std::max(0.001, std::min(0.999, x));
            x = flushDenorm(x);
        }
    }
    
    double getOutput() const override {
        return 2.0 * x - 1.0;  // Map [0,1] to [-1,1]
    }
    
private:
    double x = 0.5;
    double accumTime = 0.0;
    const double iterPeriod = 0.0005;  // 2kHz iteration
};

// Ikeda map - optical chaos
class IkedaSystem : public ChaosSystem {
public:
    void reset(double seed) override {
        x = seed * 0.1;
        y = seed * 0.1;
    }
    
    void step(double dt) override {
        accumTime += dt;
        while (accumTime >= iterPeriod) {
            accumTime -= iterPeriod;
            
            const double u = 0.918;  // Chaos parameter
            double t = 0.4 - 6.0 / (1.0 + x*x + y*y);
            
            double xNew = 1.0 + u * (x * std::cos(t) - y * std::sin(t));
            double yNew = u * (x * std::sin(t) + y * std::cos(t));
            
            x = clamp(xNew, 20.0);
            y = clamp(yNew, 20.0);
            
            x = flushDenorm(x);
            y = flushDenorm(y);
        }
    }
    
    double getOutput() const override {
        return fastTanh((x + y) / 4.0);
    }
    
private:
    double x = 0.1, y = 0.1;
    double accumTime = 0.0;
    const double iterPeriod = 0.001;
};

// Duffing oscillator - driven chaos
class DuffingSystem : public ChaosSystem {
public:
    void reset(double seed) override {
        x = seed * 0.1;
        v = 0.0;
        phase = 0.0;
    }
    
    void step(double dt) override {
        // Duffing equation: x'' + δx' + αx + βx³ = γcos(ωt)
        const double delta = 0.3;   // Damping
        const double alpha = -1.0;  // Linear stiffness
        const double beta = 1.0;    // Nonlinear stiffness
        const double gamma = 0.35;  // Drive amplitude
        const double omega = 1.2;   // Drive frequency
        
        // Second order to first order system
        double drive = gamma * std::cos(omega * phase);
        double dx = v;
        double dv = -delta * v - alpha * x - beta * x * x * x + drive;
        
        // RK4 integration
        double k1x = v;
        double k1v = dv;
        
        double x2 = x + 0.5 * dt * k1x;
        double v2 = v + 0.5 * dt * k1v;
        double k2x = v2;
        double k2v = -delta * v2 - alpha * x2 - beta * x2 * x2 * x2 + drive;
        
        x = clamp(x + dt * k2x);
        v = clamp(v + dt * k2v);
        phase += dt;
        
        x = flushDenorm(x);
        v = flushDenorm(v);
        
        // Wrap phase
        if (phase > kTwoPi) phase -= kTwoPi;
    }
    
    double getOutput() const override {
        return fastTanh(x / 2.0);
    }
    
private:
    double x = 0.0, v = 0.0;
    double phase = 0.0;
};

// Implementation structure with all DSP components
struct ChaosGenerator_Platinum::Impl {
    // Core DSP state
    double sampleRate{44100.0};
    int blockSize{512};
    
    // Chaos systems
    std::array<std::unique_ptr<ChaosSystem>, kNumTypes> chaosSystems;
    ChaosType currentType{kLorenz};
    double chaosOutput{0.0};
    double chaosSmoothed{0.0};
    
    // Integration time management
    double timeStep{0.001};
    double timeAccumulator{0.0};
    
    // Host sync
    double hostPhase{0.0};
    double hostBPM{120.0};
    bool isPlaying{false};
    
    // Parameter smoothers
    ParamSmoother rateSmooth;
    ParamSmoother depthSmooth;
    ParamSmoother smoothingSmooth;
    ParamSmoother mixSmooth;
    
    // Modulation routing
    enum ModTarget {
        kModPitch = 0,
        kModFilter,
        kModAmp,
        kModPan,
        kNumModTargets
    };
    ModTarget modTarget{kModPitch};
    
    // State
    double outputSmoothing{0.99};
    uint32_t randomSeed{12345};
    bool syncToHost{false};
    
    // Performance monitoring
    float cpuLoad{0.0f};
    std::atomic<int> blockCounter{0};
    
    // Silence detection
    float silenceCounter{0};
    bool isSilent{false};
    
    // Constructor
    Impl() {
        // Create all chaos systems
        chaosSystems[kLorenz] = std::make_unique<LorenzSystem>();
        chaosSystems[kRossler] = std::make_unique<RosslerSystem>();
        chaosSystems[kHenon] = std::make_unique<HenonSystem>();
        chaosSystems[kLogistic] = std::make_unique<LogisticSystem>();
        chaosSystems[kIkeda] = std::make_unique<IkedaSystem>();
        chaosSystems[kDuffing] = std::make_unique<DuffingSystem>();
        
        // Initialize systems
        for (auto& system : chaosSystems) {
            system->reset(randomSeed * 0.0001);
        }
        
        setupSmoothers();
    }
    
    void setupSmoothers() {
        rateSmooth.setTime(10.0f);
        depthSmooth.setTime(20.0f);
        smoothingSmooth.setTime(50.0f);
        mixSmooth.setTime(10.0f);
    }
    
    void prepare(double sr, int bs) {
        sampleRate = sr;
        blockSize = bs;
        
        // Update smoothers
        rateSmooth.setSampleRate(sr);
        depthSmooth.setSampleRate(sr);
        smoothingSmooth.setSampleRate(sr);
        mixSmooth.setSampleRate(sr);
        
        // Calculate time step for integration
        timeStep = 1.0 / sr;
        
        reset();
    }
    
    void reset() {
        for (auto& system : chaosSystems) {
            system->reset(randomSeed * 0.0001);
        }
        
        chaosOutput = 0.0;
        chaosSmoothed = 0.0;
        timeAccumulator = 0.0;
        hostPhase = 0.0;
        silenceCounter = 0;
        isSilent = false;
    }
    
    void setTransportInfo(bool playing, double bpm, double phase) {
        isPlaying = playing;
        hostBPM = bpm;
        hostPhase = phase;
    }
    
    inline double generateChaos() {
        // Get smoothed rate parameter
        float rate = rateSmooth.tick();
        
        // Calculate integration time step based on rate
        double dt = timeStep * (0.1 + rate * 10.0);  // 0.1x to 10.1x speed
        dt = std::min(dt, kMaxTimeStep);
        
        // Sync to host tempo if enabled
        if (syncToHost && isPlaying && hostBPM > 0) {
            double beatsPerSec = hostBPM / 60.0;
            dt = timeStep * beatsPerSec * (0.25 + rate * 4.0);  // 1/16 to 4 bars
        }
        
        // Step the active chaos system
        chaosSystems[currentType]->step(dt);
        
        // Get output
        double output = chaosSystems[currentType]->getOutput();
        
        // Apply smoothing
        float smoothing = smoothingSmooth.tick();
        double smoothCoeff = 1.0 - smoothing * 0.999;
        chaosSmoothed += smoothCoeff * (output - chaosSmoothed);
        
        return flushDenorm(chaosSmoothed);
    }
    
    void processModulation(float* data, int numSamples, int channel) {
        float depth = depthSmooth.tick();
        float mix = mixSmooth.tick();
        
        for (int s = 0; s < numSamples; ++s) {
            // Generate chaos modulation
            double mod = generateChaos() * depth;
            
            // Route modulation based on target
            float modulated = data[s];
            
            switch (modTarget) {
                case kModPitch:
                    // Pitch modulation (vibrato)
                    // This would require a delay line - just scale for now
                    modulated *= (1.0f + static_cast<float>(mod) * 0.1f);
                    break;
                    
                case kModFilter:
                    // Filter modulation (brightness)
                    // Simple one-pole filter
                    {
                        static float z1[2] = {0.0f, 0.0f};
                        float cutoff = 0.5f + static_cast<float>(mod) * 0.4f;
                        modulated = modulated * cutoff + z1[channel] * (1.0f - cutoff);
                        z1[channel] = modulated;
                    }
                    break;
                    
                case kModAmp:
                    // Amplitude modulation (tremolo)
                    modulated *= (1.0f + static_cast<float>(mod));
                    break;
                    
                case kModPan:
                    // Pan modulation (auto-pan)
                    // Apply differently to L/R channels
                    if (channel == 0) {
                        modulated *= (1.0f - static_cast<float>(mod) * 0.5f);
                    } else {
                        modulated *= (1.0f + static_cast<float>(mod) * 0.5f);
                    }
                    break;
            }
            
            // Mix dry/wet
            data[s] = data[s] * (1.0f - mix) + modulated * mix;
            data[s] = flushDenorm(data[s]);
        }
    }
    
    bool detectSilence(const juce::AudioBuffer<float>& buffer) {
        float rms = 0.0f;
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();
        
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int s = 0; s < numSamples; ++s) {
                rms += data[s] * data[s];
            }
        }
        
        rms = std::sqrt(rms / (numChannels * numSamples));
        
        if (rms < kSilenceThreshold) {
            silenceCounter += numSamples;
            if (silenceCounter > sampleRate * 0.1) { // 100ms of silence
                return true;
            }
        } else {
            silenceCounter = 0;
        }
        
        return false;
    }
};

// Public interface implementation
ChaosGenerator_Platinum::ChaosGenerator_Platinum() : pImpl(std::make_unique<Impl>()) {}

ChaosGenerator_Platinum::~ChaosGenerator_Platinum() = default;

void ChaosGenerator_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->prepare(sampleRate, samplesPerBlock);
    
    // Initialize default parameters
    pImpl->rateSmooth.snap(0.5f);      // 50% rate
    pImpl->depthSmooth.snap(0.5f);     // 50% depth
    pImpl->smoothingSmooth.snap(0.5f); // 50% smoothing
    pImpl->mixSmooth.snap(1.0f);       // 100% wet
}

void ChaosGenerator_Platinum::process(juce::AudioBuffer<float>& buffer) {
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    // Fast path for silence
    pImpl->isSilent = pImpl->detectSilence(buffer);
    if (pImpl->isSilent) {
        return;  // Bypass processing
    }
    
    // Update block counter for performance monitoring
    pImpl->blockCounter++;
    
    // Process each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        float* data = buffer.getWritePointer(ch);
        pImpl->processModulation(data, numSamples, ch);
    }
}

void ChaosGenerator_Platinum::reset() {
    pImpl->reset();
}

void ChaosGenerator_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [id, value] : params) {
        switch (id) {
            case kRate:
                // 0-1 maps to 0.01Hz to 100Hz (log scale)
                pImpl->rateSmooth.setValue(value);
                break;
                
            case kDepth:
                // Direct 0-1 mapping
                pImpl->depthSmooth.setValue(value);
                break;
                
            case kType:
                // 0-1 maps to chaos types
                {
                    int type = static_cast<int>(value * (kNumTypes - 0.01f));
                    type = std::max(0, std::min(kNumTypes - 1, type));
                    if (type != pImpl->currentType) {
                        pImpl->currentType = static_cast<ChaosType>(type);
                        pImpl->chaosSystems[type]->reset(pImpl->randomSeed * 0.0001);
                    }
                }
                break;
                
            case kSmoothing:
                // 0-1 for chaos output smoothing
                pImpl->smoothingSmooth.setValue(value);
                break;
                
            case kModTarget:
                // 0-1 maps to modulation targets
                {
                    int target = static_cast<int>(value * (Impl::kNumModTargets - 0.01f));
                    target = std::max(0, std::min(Impl::kNumModTargets - 1, target));
                    pImpl->modTarget = static_cast<Impl::ModTarget>(target);
                }
                break;
                
            case kSync:
                // 0-1 for host sync on/off
                pImpl->syncToHost = value > 0.5f;
                break;
                
            case kSeed:
                // 0-1 maps to random seed
                pImpl->randomSeed = static_cast<uint32_t>(value * 4294967295.0f);
                for (auto& system : pImpl->chaosSystems) {
                    system->reset(pImpl->randomSeed * 0.0001);
                }
                break;
                
            case kMix:
                // Direct 0-1 for dry/wet
                pImpl->mixSmooth.setValue(value);
                break;
        }
    }
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
        default: return "Unknown";
    }
}