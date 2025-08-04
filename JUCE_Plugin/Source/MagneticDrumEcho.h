// ==================== MagneticDrumEcho.h ====================
#pragma once
#include "EngineBase.h"
#include <array>
#include <memory>
#include <atomic>
#include <cmath>
#include <random>
#include <vector>

class MagneticDrumEcho : public EngineBase {
public:
    MagneticDrumEcho();
    ~MagneticDrumEcho() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Magnetic Drum Echo"; }
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    
    // Optional: Configure max delay time before prepareToPlay()
    void setMaxDelayTime(double seconds) { 
        m_maxDelaySeconds = std::clamp(seconds, 0.1, 5.0); 
    }
    
    // Memory usage info
    size_t getMemoryUsage() const;
    
private:
    // Professional constants
    static constexpr double DENORMAL_PREVENTION = 1e-30;
    static constexpr int OVERSAMPLE_FACTOR = 2;
    static constexpr int MAX_BLOCK_SIZE = 2048;
    static constexpr int NUM_CHANNELS = 2;
    static constexpr int NUM_HEADS = 4;  // 1 record + 3 playback
    
    // Authentic Binson Echorec head positions (in degrees on drum)
    static constexpr std::array<double, NUM_HEADS> HEAD_POSITIONS = {
        0.0,    // Record head
        90.0,   // Playback head 1
        180.0,  // Playback head 2  
        270.0   // Playback head 3
    };
    
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
    
    // Efficient shared circular buffer for drum
    class CircularDrumBuffer {
        std::vector<float> buffer;
        size_t bufferSize = 0;
        size_t writePos = 0;
        
    public:
        void prepare(double sampleRate, double maxDelaySeconds) {
            bufferSize = static_cast<size_t>(sampleRate * maxDelaySeconds) + 1;
            buffer.resize(bufferSize);
            reset();
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }
        
        void write(float sample) {
            buffer[writePos] = sample;
            writePos = (writePos + 1) % bufferSize;
        }
        
        float read(double delaySamples) const {
            // Calculate read position
            double readPos = static_cast<double>(writePos) - delaySamples;
            while (readPos < 0) readPos += bufferSize;
            while (readPos >= bufferSize) readPos -= bufferSize;
            
            // Cubic interpolation
            int idx0 = static_cast<int>(readPos);
            double frac = readPos - idx0;
            
            int idx1 = (idx0 + 1) % bufferSize;
            int idx2 = (idx0 + 2) % bufferSize;
            int idx3 = (idx0 + 3) % bufferSize;
            
            float y0 = buffer[idx0];
            float y1 = buffer[idx1];
            float y2 = buffer[idx2];
            float y3 = buffer[idx3];
            
            float c0 = y1;
            float c1 = 0.5f * (y2 - y0);
            float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            
            return ((c3 * frac + c2) * frac + c1) * frac + c0;
        }
        
        size_t getBufferSize() const { return bufferSize; }
    };
    
    // Magnetic head (processes signal, doesn't store audio)
    class MagneticHead {
        // Tape characteristics
        double magnetization = 0.0;
        double previousInput = 0.0;
        
        // Head gap resonance filter
        double bumpX1 = 0, bumpY1 = 0;
        double bumpFreq = 100.0;
        double bumpQ = 2.0;
        double bumpGain = 3.0;  // dB
        
    public:
        void reset();
        float processMagneticSaturation(float input);
        float processHeadBump(float input, double sampleRate);
        void setHeadBump(double freq, double q, double gainDb);
    };
    
    // Professional tube saturation model
    class TubeSaturation {
        double inputCouplingState = 0.0;
        double outputCouplingState = 0.0;
        double inputCouplingCoeff = 0.0;
        double outputCouplingCoeff = 0.0;
        
        // Tube parameters (12AX7 characteristics)
        double mu = 100.0;
        double plateResistance = 62.5e3;
        double transconductance = 1.6e-3;
        double gridBias = -2.0;
        double plateVoltage = 250.0;
        
    public:
        void setSampleRate(double sr);
        double process(double input, double drive);
        void reset();
        
    private:
        double processInputCoupling(double input);
        double processOutputCoupling(double input);
        double processTubeStage(double input, double drive);
    };
    
    // Authentic wow & flutter simulation
    class WowFlutterSimulator {
        // Multiple LFO sources
        double wowPhase = 0.0;
        double flutterPhase = 0.0;
        double scrapePhase = 0.0;
        
        // Random drift
        std::mt19937 rng;
        std::normal_distribution<double> distribution{0.0, 1.0};
        double driftValue = 0.0;
        double driftTarget = 0.0;
        int driftCounter = 0;
        
        // Amount controls
        double wowAmount = 0.002;
        double flutterAmount = 0.001;
        double scrapeAmount = 0.0002;
        
    public:
        WowFlutterSimulator() : rng(std::random_device{}()) {}
        
        void reset();
        double process(double sampleRate);
        void setAmount(double wow, double flutter);
    };
    
    // Motor speed control with inertia
    class MotorControl {
        double currentSpeed = 1.0;
        double targetSpeed = 1.0;
        double motorInertia = 0.98;
        
        // Power supply ripple
        double ripplePhase = 0.0;
        double rippleFreq = 100.0;  // 100Hz (50Hz mains * 2)
        double rippleAmount = 0.0005;
        
    public:
        void setSampleRate(double sr);
        void setSpeed(double speed);
        double getCurrentSpeed() const { return currentSpeed; }
        void update();
        double getSpeedWithRipple(double sampleRate);
        void reset();
    };
    
    // Professional Butterworth filter
    class ButterworthFilter {
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        double b0, b1, b2, a1, a2;
        
    public:
        void setLowpass(double freq, double sampleRate, double q = 0.7071);
        void setHighpass(double freq, double sampleRate, double q = 0.7071);
        double process(double input);
        void reset() { x1 = x2 = y1 = y2 = 0; }
    };
    
    // Feedback path with authentic tape compression
    class FeedbackProcessor {
        double previousSample = 0.0;
        
        // Soft knee compressor
        double threshold = 0.7;
        double ratio = 4.0;
        double knee = 0.1;
        double makeupGain = 1.2;
        
        // Envelope follower
        double envelope = 0.0;
        double attackTime = 0.005;
        double releaseTime = 0.050;
        double attackCoeff = 0.0;
        double releaseCoeff = 0.0;
        
    public:
        void setSampleRate(double sr);
        double process(double input, double feedbackAmount);
        void reset();
        
    private:
        double softKneeCompression(double input);
    };
    
    // Optimized oversampling for saturation stages
    class Oversampler2x {
        struct AllPassStage {
            double z1 = 0;
            double coefficient = 0;
            
            void setCoefficient(double c) { coefficient = c; }
            double process(double input) {
                double output = coefficient * (input - z1) + z1;
                z1 = output;
                return output;
            }
            void reset() { z1 = 0; }
        };
        
        std::array<AllPassStage, 2> upsampleStages;
        std::array<AllPassStage, 2> downsampleStages;
        double z1 = 0;
        
    public:
        void prepare();
        void upsample(const double* input, double* output, int numSamples);
        void downsample(const double* input, double* output, int numSamples);
        void reset();
    };
    
    // Core DSP members
    double m_sampleRate = 48000.0;
    double m_maxDelaySeconds = 2.0;  // Configurable max delay
    
    // Parameters
    std::unique_ptr<ParameterSmoother> m_drumSpeed;
    std::unique_ptr<ParameterSmoother> m_head1Level;
    std::unique_ptr<ParameterSmoother> m_head2Level;
    std::unique_ptr<ParameterSmoother> m_head3Level;
    std::unique_ptr<ParameterSmoother> m_feedback;
    std::unique_ptr<ParameterSmoother> m_saturation;
    std::unique_ptr<ParameterSmoother> m_wowFlutter;
    std::unique_ptr<ParameterSmoother> m_mix;
    
    // Shared drum buffers (one per channel)
    std::array<CircularDrumBuffer, NUM_CHANNELS> m_drumBuffers;
    
    // Processing components
    std::array<std::array<MagneticHead, NUM_HEADS>, NUM_CHANNELS> m_heads;
    std::array<TubeSaturation, NUM_CHANNELS> m_inputTubes;
    std::array<TubeSaturation, NUM_CHANNELS> m_outputTubes;
    std::array<WowFlutterSimulator, NUM_CHANNELS> m_wowFlutterSims;
    std::array<FeedbackProcessor, NUM_CHANNELS> m_feedbackProcessors;
    std::array<ButterworthFilter, NUM_CHANNELS> m_inputHighpass;
    std::array<ButterworthFilter, NUM_CHANNELS> m_outputLowpass;
    std::array<std::unique_ptr<Oversampler2x>, NUM_CHANNELS> m_oversamplers;
    
    // Shared motor control
    MotorControl m_motor;
    
    // Work buffers
    alignas(16) std::array<double, MAX_BLOCK_SIZE> m_workBuffers[NUM_CHANNELS];
    alignas(16) std::array<double, MAX_BLOCK_SIZE * OVERSAMPLE_FACTOR> m_oversampledBuffers[NUM_CHANNELS];
    
    // Cached parameters for block processing
    struct CachedParams {
        double drumSpeed;
        double head1Level;
        double head2Level;
        double head3Level;
        double feedback;
        double saturation;
        double wowFlutter;
        double mix;
    };
    
    // Processing methods
    void processChannel(float* data, int numSamples, int channel, const CachedParams& params);
    double calculateHeadDelay(int headIndex, double drumSpeed, double wowFlutter);
    double mixPlaybackHeads(int channel, const CachedParams& params);
};