// ==================== BucketBrigadeDelay.h ====================
#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <array>
#include <memory>
#include <atomic>
#include <mutex>
#include <cmath>
#include <random>
#include <vector>

class BucketBrigadeDelay : public EngineBase {
public:
    BucketBrigadeDelay();
    ~BucketBrigadeDelay() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Bucket Brigade Delay"; }
    int getNumParameters() const override { return 7; }
    juce::String getParameterName(int index) const override;
    
    // Extended EngineBase API
    void setTransportInfo(const TransportInfo& info) override;
    bool supportsFeature(Feature f) const noexcept override;
    
private:
    // Professional constants
    static constexpr double DENORMAL_PREVENTION = 1e-30;
    static constexpr int MAX_BLOCK_SIZE = 2048;
    static constexpr int NUM_CHANNELS = 2;
    
    // BBD chip specifications
    static constexpr int BBD_STAGES_3005 = 4096;  // MN3005
    static constexpr int BBD_STAGES_3007 = 1024;  // MN3007
    static constexpr int BBD_STAGES_3008 = 2048;  // MN3008
    static constexpr double MAX_CLOCK_RATE = 100000.0;  // 100kHz max
    static constexpr double MIN_CLOCK_RATE = 5000.0;    // 5kHz min
    
    // Thread safety
    mutable std::mutex parameterMutex;
    std::atomic<bool> parametersChanged{false};
    std::atomic<int> m_chipTypeAtomic{1};  // Default to MN3007
    
    // Professional parameter smoothing
    class ParameterSmoother {
        std::atomic<double> targetValue{0.0};
        double currentValue = 0.0;
        double smoothingCoeff = 0.0;
        
    public:
        void setSampleRate(double sr, double smoothingTimeMs = 20.0) {
            double fc = 1000.0 / (2.0 * M_PI * smoothingTimeMs);
            smoothingCoeff = std::exp(-2.0 * M_PI * fc / sr);
        }
        
        void setTarget(double value) {
            targetValue.store(value, std::memory_order_relaxed);
        }
        
        double process() {
            double target = targetValue.load(std::memory_order_relaxed);
            currentValue = target + (currentValue - target) * smoothingCoeff;
            currentValue += DENORMAL_PREVENTION;
            currentValue -= DENORMAL_PREVENTION;
            return currentValue;
        }
        
        void reset(double value) {
            targetValue.store(value, std::memory_order_relaxed);
            currentValue = value;
        }
        
        double getCurrent() const { return currentValue; }
    };
    
    // Authentic BBD stage modeling with dynamic allocation
    class BBDChain {
        std::vector<double> buckets;  // Dynamic allocation
        int numStages = 1024;
        
        // Two-phase non-overlapping clock
        double clockPhase = 0.0;
        enum ClockState { IDLE, PHASE1, DEAD_TIME, PHASE2 } clockState = IDLE;
        static constexpr double DEAD_TIME_RATIO = 0.05;  // 5% dead time
        
        // Charge transfer characteristics
        double transferEfficiency = 0.997;
        double chargeLeakage = 0.00001;
        double inputCapacitance = 0.1;
        double clockFeedthrough = 0.002;
        
    public:
        void setNumStages(int stages);
        void reset();
        double process(double input, double clockRate, double sampleRate);
        void setCharacteristics(double efficiency, double leakage, double feedthrough);
        
    private:
        void transferCharges(double input, bool oddPhase);
    };
    
    // Professional companding system
    class CompandingSystem {
        double compressorEnvelope = 0.0;
        double compressorGain = 1.0;
        double preEmphasisState = 0.0;
        
        double expanderEnvelope = 0.0;
        double expanderGain = 1.0;
        double deEmphasisState = 0.0;
        
        double attackTime = 0.0001;
        double releaseTime = 0.001;
        double attackCoeff = 0.0;
        double releaseCoeff = 0.0;
        
        double emphasisFreq = 3180.0;
        double sampleRate = 48000.0;
        
    public:
        void setSampleRate(double sr);
        double compress(double input);
        double expand(double input);
        void reset();
        
    private:
        double updateEnvelope(double input, double& envelope);
        static double dBtoLinear(double dB) { return std::pow(10.0, dB / 20.0); }
        static double linearTodB(double linear) { return 20.0 * std::log10(std::max(linear, 1e-10)); }
    };
    
    // Anti-aliasing and reconstruction filters
    class BBDFilters {
        double sampleRate = 48000.0;
        
        class EllipticFilter {
            struct Biquad {
                double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
                double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
                
                double process(double input);
                void reset() { x1 = x2 = y1 = y2 = 0; }
            };
            
            std::array<Biquad, 2> stages;
            
        public:
            void designLowpass(double freq, double sampleRate, double ripple = 0.1);
            double process(double input);
            void reset();
        };
        
        EllipticFilter antiAliasingFilter;
        EllipticFilter reconstructionFilter;
        
    public:
        void setSampleRate(double sr);
        void updateFilters(double clockRate);
        double processAntiAliasing(double input);
        double processReconstruction(double input);
        void reset();
    };
    
    // Clock generator with realistic BBD clock characteristics
    class ClockGenerator {
        double phase = 0.0;
        double jitterAmount = 0.0;
        
        double lfoPhase = 0.0;
        double lfoRate = 0.5;
        double lfoDepth = 0.0;
        
        std::mt19937 rng;
        std::normal_distribution<double> distribution{0.0, 1.0};
        double noiseState = 0.0;
        double noiseLPF = 0.0;
        
    public:
        ClockGenerator() : rng(std::random_device{}()) {}
        
        void reset();
        double generateClockRate(double baseRate, double modulation, double sampleRate);
        void setLFO(double rate, double depth) { lfoRate = rate; lfoDepth = depth; }
        void setJitter(double amount) { jitterAmount = amount; }
    };
    
    // Analog circuit modeling
    class AnalogCircuit {
        double supplyVoltage = 9.0;
        double supplyRipple = 0.0;
        double rippleFreq = 100.0;
        double ripplePhase = 0.0;
        
        double temperature = 25.0;
        double tempCoefficient = 0.002;
        
        double capacitorAging = 0.0;
        double resistorDrift = 0.0;
        
        std::mt19937 rng;
        std::uniform_real_distribution<double> tempDist{-0.5, 0.5};
        
    public:
        AnalogCircuit() : rng(std::random_device{}()) {}
        
        void update(double sampleRate);
        double getDelayModulation() const;
        double getFilterModulation() const;
        void setAging(double amount);
        void reset();
    };
    
    // Feedback path processing
    class FeedbackProcessor {
        double previousSample = 0.0;
        double highpassState = 0.0;
        double saturationState = 0.0;
        
        double threshold = 0.7;
        double knee = 0.1;
        
    public:
        double process(double input, double amount);
        void reset() { previousSample = highpassState = saturationState = 0.0; }
        
    private:
        double softClip(double input);
    };
    
    // DC servo
    class DCServo {
        double integrator = 0.0;
        double cutoffFreq = 5.0;
        double coefficient = 0.0;
        
    public:
        void setSampleRate(double sr) {
            coefficient = 2.0 * M_PI * cutoffFreq / sr;
        }
        
        double process(double input) {
            integrator += input * coefficient;
            integrator *= 0.9999;
            return input - integrator;
        }
        
        void reset() { integrator = 0.0; }
    };
    
    // Core DSP members
    double m_sampleRate = 48000.0;
    
    // Parameters
    std::unique_ptr<ParameterSmoother> m_delayTime;
    std::unique_ptr<ParameterSmoother> m_feedback;
    std::unique_ptr<ParameterSmoother> m_modulation;
    std::unique_ptr<ParameterSmoother> m_tone;
    std::unique_ptr<ParameterSmoother> m_age;
    std::unique_ptr<ParameterSmoother> m_mix;
    std::unique_ptr<ParameterSmoother> m_sync;
    
    // Processing components
    std::array<BBDChain, NUM_CHANNELS> m_bbdChains;
    std::array<CompandingSystem, NUM_CHANNELS> m_companders;
    std::array<BBDFilters, NUM_CHANNELS> m_filters;
    std::array<FeedbackProcessor, NUM_CHANNELS> m_feedbackProcessors;
    std::array<DCServo, NUM_CHANNELS> m_dcServos;
    
    // Transport sync
    TransportInfo m_transportInfo;
    
    // Beat division mapping
    enum class BeatDivision {
        DIV_1_64, DIV_1_32, DIV_1_16, DIV_1_8, DIV_1_4, 
        DIV_1_2, DIV_1_1, DIV_2_1, DIV_4_1
    };
    
    // Shared components
    ClockGenerator m_clockGenerator;
    AnalogCircuit m_analogCircuit;
    
    // Work buffers
    alignas(16) std::array<double, MAX_BLOCK_SIZE> m_workBuffers[NUM_CHANNELS];
    
    // Cached parameters
    struct CachedParams {
        double delayTime;
        double feedback;
        double modulation;
        double tone;
        double age;
        double mix;
        double sync;
        double clockRate;
    };
    
    // BBD chip selection
    enum class ChipType {
        MN3005,
        MN3007,
        MN3008
    };
    
    // Processing methods
    void processChannel(float* data, int numSamples, int channel, const CachedParams& params);
    double calculateClockRate(double delayMs) const;
    ChipType getCurrentChipType() const { return static_cast<ChipType>(m_chipTypeAtomic.load()); }
    void updateChipType(ChipType newType);
    
    // Transport sync methods
    double calculateSyncedDelayTime(double timeParam, double syncParam) const;
    double getBeatDivisionMs(BeatDivision division) const;
};