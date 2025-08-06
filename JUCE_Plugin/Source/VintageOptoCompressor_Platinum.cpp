#include "VintageOptoCompressor_Platinum.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>
#include <functional>
#include <chrono>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

// Constants
static constexpr float kDenormalThreshold = 1.0e-30f;
static constexpr float kDCBlockerFreq = 5.0f;  // Hz
static constexpr int kOversampleFactor = 4;
static constexpr int kMaxBlockSize = 2048;
static constexpr float kSilenceThreshold = 1.0e-10f;

// Fast math approximations
namespace FastMath {
    // Fast tanh approximation (Padé approximant, accuracy ±0.0001)
    inline float tanh(float x) noexcept {
        const float x2 = x * x;
        const float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
        const float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
        return a / b;
    }
    
    // Fast exp approximation for envelope followers
    inline float exp(float x) noexcept {
        x = 1.0f + x * 0.00390625f;  // x/256
        x *= x; x *= x; x *= x; x *= x;
        x *= x; x *= x; x *= x; x *= x;
        return x;
    }
    
    // Denormal flush
    template<typename T>
    inline T flushDenormal(T x) noexcept {
        return (std::fabs(x) < kDenormalThreshold) ? T(0) : x;
    }
}

// Thread-safe PRNG for analog noise
class XorShift32 {
    uint32_t state;
public:
    explicit XorShift32(uint32_t seed = 2463534242u) : state(seed) {}
    
    float next() noexcept {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (state & 0xFFFFFF) * (1.0f / 16777216.0f) - 0.5f;
    }
};

// Atomic smoothed parameter with denormal protection
class AtomicSmoothParam {
    std::atomic<float> target{0.5f};
    float current{0.5f};
    float coeff{0.995f};
    
public:
    void set(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
    }
    
    void reset(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
        current = value;
    }
    
    void setSmoothingTime(float ms, double sampleRate) noexcept {
        const float samples = ms * 0.001f * static_cast<float>(sampleRate);
        coeff = FastMath::exp(-1.0f / samples);
    }
    
    float tick() noexcept {
        const float t = target.load(std::memory_order_relaxed);
        current += (t - current) * (1.0f - coeff);
        current = FastMath::flushDenormal(current);
        return current;
    }
    
    float getValue() const noexcept { return current; }
};

// High-quality oversampling with polyphase IIR
class PlatinumOversampler {
    static constexpr int kOrder = 8;  // 8th order Butterworth
    
    struct BiquadCascade {
        // Transposed Direct Form II for numerical stability
        struct Biquad {
            double b0{1}, b1{0}, b2{0};
            double a1{0}, a2{0};
            double s1{0}, s2{0};
            
            double process(double x) noexcept {
                double y = b0 * x + s1;
                s1 = b1 * x - a1 * y + s2;
                s2 = b2 * x - a2 * y;
                s1 = FastMath::flushDenormal(s1);
                s2 = FastMath::flushDenormal(s2);
                return y;
            }
            
            void reset() noexcept {
                s1 = s2 = 0;
            }
        };
        
        std::array<Biquad, 4> stages;
        
        float process(float x) noexcept {
            double y = x;
            for (auto& stage : stages) {
                y = stage.process(y);
            }
            return static_cast<float>(y);
        }
        
        void reset() noexcept {
            for (auto& stage : stages) {
                stage.reset();
            }
        }
    };
    
    // Polyphase decomposition for efficiency
    std::array<BiquadCascade, kOversampleFactor> upsamplers;
    std::array<BiquadCascade, kOversampleFactor> downsamplers;
    std::array<float, kOversampleFactor * kMaxBlockSize> workBuffer;
    
public:
    void init(double sampleRate) noexcept {
        // Calculate Butterworth coefficients for fc = 0.45 * Nyquist
        const double wc = M_PI * 0.45;
        const double wc2 = wc * wc;
        
        for (int i = 0; i < kOversampleFactor; ++i) {
            for (int j = 0; j < 4; ++j) {
                auto& up = upsamplers[i].stages[j];
                auto& down = downsamplers[i].stages[j];
                
                // Butterworth prototype
                const double theta = M_PI * (2.0 * j + 1) / (2.0 * kOrder);
                const double s_re = -std::sin(theta);
                const double s_im = std::cos(theta);
                
                // Bilinear transform
                const double a0 = wc2 + 2.0 * wc * s_re + 1.0;
                up.b0 = down.b0 = wc2 / a0;
                up.b1 = down.b1 = 2.0 * wc2 / a0;
                up.b2 = down.b2 = wc2 / a0;
                up.a1 = down.a1 = (2.0 * wc2 - 2.0) / a0;
                up.a2 = down.a2 = (wc2 - 2.0 * wc * s_re + 1.0) / a0;
            }
        }
    }
    
    void reset() noexcept {
        for (auto& up : upsamplers) up.reset();
        for (auto& down : downsamplers) down.reset();
        std::memset(workBuffer.data(), 0, workBuffer.size() * sizeof(float));
    }
    
    void processBlock(const float* input, float* output, int numSamples,
                     std::function<void(float*, int)> processor) noexcept {
        // Upsample
        for (int i = 0; i < numSamples; ++i) {
            for (int j = 0; j < kOversampleFactor; ++j) {
                float x = (j == 0) ? input[i] * kOversampleFactor : 0.0f;
                workBuffer[i * kOversampleFactor + j] = upsamplers[j].process(x);
            }
        }
        
        // Process at higher rate
        processor(workBuffer.data(), numSamples * kOversampleFactor);
        
        // Downsample
        for (int i = 0; i < numSamples; ++i) {
            float sum = 0.0f;
            for (int j = 0; j < kOversampleFactor; ++j) {
                sum += downsamplers[j].process(workBuffer[i * kOversampleFactor + j]);
            }
            output[i] = sum;
        }
    }
};

// Platinum-spec T4 opto cell model
class T4OptoCell {
    // Dual time constant model (fast and slow photoresistors)
    double fastBrightness{0};
    double slowBrightness{0};
    double thermalFactor{1.0};
    double memoryEffect{0};
    
    static constexpr double kFastAttack = 0.001;   // ~1ms
    static constexpr double kSlowAttack = 0.010;   // ~10ms
    static constexpr double kFastRelease = 0.060;  // ~60ms
    static constexpr double kSlowRelease = 1.000;  // ~1s
    static constexpr double kMemoryDecay = 0.9995;
    
public:
    void setThermalFactor(double factor) noexcept {
        thermalFactor = factor;
    }
    
    void update(double targetBrightness, double sampleRate) noexcept {
        const double attackCoeff = 1.0 / (sampleRate * 0.001);
        const double releaseCoeff = 1.0 / (sampleRate * 0.001);
        
        // Fast cell
        if (targetBrightness > fastBrightness) {
            const double rate = 1.0 - FastMath::exp(-attackCoeff * kFastAttack * thermalFactor);
            fastBrightness += (targetBrightness - fastBrightness) * rate;
        } else {
            const double programRelease = kFastRelease * (1.0 + memoryEffect * 4.0);
            const double rate = 1.0 - FastMath::exp(-releaseCoeff * programRelease * thermalFactor);
            fastBrightness += (targetBrightness - fastBrightness) * rate;
        }
        
        // Slow cell
        if (targetBrightness > slowBrightness) {
            const double rate = 1.0 - FastMath::exp(-attackCoeff * kSlowAttack * thermalFactor);
            slowBrightness += (targetBrightness - slowBrightness) * rate;
        } else {
            const double programRelease = kSlowRelease * (1.0 + memoryEffect * 2.0);
            const double rate = 1.0 - FastMath::exp(-releaseCoeff * programRelease * thermalFactor);
            slowBrightness += (targetBrightness - slowBrightness) * rate;
        }
        
        // Update memory effect
        if (targetBrightness > 0.5) {
            memoryEffect = targetBrightness;
        } else {
            memoryEffect *= kMemoryDecay;
        }
        
        // Denormal protection
        fastBrightness = FastMath::flushDenormal(fastBrightness);
        slowBrightness = FastMath::flushDenormal(slowBrightness);
        memoryEffect = FastMath::flushDenormal(memoryEffect);
    }
    
    float getGainReduction() const noexcept {
        // Weighted combination of fast and slow cells
        const double brightness = fastBrightness * 0.3 + slowBrightness * 0.7;
        
        // Convert to resistance (LA-2A T4 characteristic)
        const double resistance = 10000.0 + (990000.0 * (1.0 - brightness));
        
        // Voltage divider gain reduction
        const double ratio = 100000.0 / (100000.0 + resistance);
        return static_cast<float>(1.0 - ratio);
    }
    
    void reset() noexcept {
        fastBrightness = slowBrightness = memoryEffect = 0;
    }
};

// DC blocker with subsonic filtering
class PlatinumDCBlocker {
    double x1{0}, y1{0};
    double R{0.995};
    
public:
    void setSampleRate(double sampleRate) noexcept {
        const double fc = kDCBlockerFreq / sampleRate;
        R = 1.0 - (2.0 * M_PI * fc);
    }
    
    float process(float input) noexcept {
        double output = input - x1 + R * y1;
        x1 = input;
        y1 = FastMath::flushDenormal(output);
        return static_cast<float>(output);
    }
    
    void reset() noexcept {
        x1 = y1 = 0;
    }
};

// Peak/RMS hybrid detector
class HybridDetector {
    double rmsState{0};
    double peakState{0};
    double attackCoeff{0};
    double releaseCoeff{0};
    
public:
    void setSampleRate(double sampleRate) noexcept {
        attackCoeff = 1.0 - FastMath::exp(-1.0 / (0.001 * sampleRate));   // 1ms
        releaseCoeff = 1.0 - FastMath::exp(-1.0 / (0.100 * sampleRate));  // 100ms
    }
    
    float detect(float input) noexcept {
        const double inputAbs = std::fabs(input);
        const double inputSquared = input * input;
        
        // RMS tracking
        const double rmsCoeff = (inputSquared > rmsState) ? attackCoeff : releaseCoeff;
        rmsState += (inputSquared - rmsState) * rmsCoeff;
        
        // Peak tracking
        const double peakCoeff = (inputAbs > peakState) ? attackCoeff : releaseCoeff;
        peakState += (inputAbs - peakState) * peakCoeff;
        
        // Denormal protection
        rmsState = FastMath::flushDenormal(rmsState);
        peakState = FastMath::flushDenormal(peakState);
        
        // Hybrid: 70% RMS, 30% peak
        return static_cast<float>(std::sqrt(rmsState) * 0.7 + peakState * 0.3);
    }
    
    void reset() noexcept {
        rmsState = peakState = 0;
    }
};

// Harmonic tube saturation
class TubeSaturation {
    XorShift32 noiseGen;
    
public:
    float process(float input, float drive, float age) noexcept {
        if (drive < 0.001f) return input;
        
        // Asymmetric saturation with age factor
        const float ageBoost = 1.0f + age * 0.1f;
        const float driveAmount = drive * ageBoost;
        
        // Generate harmonics
        float x = input * (1.0f + driveAmount);
        
        // Asymmetric clipping
        float output;
        if (x > 0) {
            output = FastMath::tanh(x * 0.7f) / 0.7f;
        } else {
            output = FastMath::tanh(x * 0.9f) / 0.9f;
        }
        
        // Add even harmonics
        const float h2 = output * output * (output > 0 ? 1.0f : -1.0f);
        output += h2 * driveAmount * 0.03f;
        
        // Vintage noise floor
        const float noise = noiseGen.next() * 0.00001f * (1.0f + age);
        output += noise;
        
        return output;
    }
};

// Pre/de-emphasis filters
class EmphasisFilter {
    double state{0};
    double cutoff{0.15};
    
public:
    void setSampleRate(double sampleRate) noexcept {
        cutoff = 1000.0 / sampleRate;  // 1kHz emphasis
    }
    
    float processPreEmphasis(float input) noexcept {
        double output = input - state;
        state += output * cutoff;
        state = FastMath::flushDenormal(state);
        return input + static_cast<float>(output * 0.5);
    }
    
    float processDeEmphasis(float input) noexcept {
        state += (input - state) * cutoff;
        state = FastMath::flushDenormal(state);
        return static_cast<float>(state);
    }
    
    void reset() noexcept {
        state = 0;
    }
};

// Main implementation structure
struct VintageOptoCompressor_Platinum::Impl {
    // Core DSP components
    struct Channel {
        T4OptoCell optoCell;
        HybridDetector detector;
        TubeSaturation tubeSat;
        EmphasisFilter preEmphasis;
        EmphasisFilter deEmphasis;
        PlatinumDCBlocker dcBlocker;
        PlatinumOversampler oversampler;
        
        float lastGain{1.0f};
        
        void prepare(double sampleRate) {
            detector.setSampleRate(sampleRate);
            preEmphasis.setSampleRate(sampleRate);
            deEmphasis.setSampleRate(sampleRate);
            dcBlocker.setSampleRate(sampleRate);
            oversampler.init(sampleRate);
            reset();
        }
        
        void reset() {
            optoCell.reset();
            detector.reset();
            preEmphasis.reset();
            deEmphasis.reset();
            dcBlocker.reset();
            oversampler.reset();
            lastGain = 1.0f;
        }
    };
    
    // Channels
    std::array<Channel, 2> channels;
    
    // Parameters
    std::array<AtomicSmoothParam, 8> params;
    
    // State
    double sampleRate{44100.0};
    float stereoReduction{0.0f};
    float componentAge{0.0f};
    float thermalDrift{1.0f};
    
    // Performance metrics
    PerformanceMetrics metrics;
    
    // Silence detection
    int silenceCounter{0};
    bool isSilent{false};
    
    // Constructor
    Impl() {
        // Initialize parameters with default values
        params[kParamGain].reset(0.5f);
        params[kParamPeakReduction].reset(0.5f);
        params[kParamEmphasis].reset(0.0f);
        params[kParamOutput].reset(0.5f);
        params[kParamMix].reset(1.0f);
        params[kParamKnee].reset(0.7f);
        params[kParamHarmonics].reset(0.3f);
        params[kParamStereoLink].reset(1.0f);
    }
    
    // Process implementation
    void processBlockInternal(juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        // Update parameters once per block
        std::array<float, 8> paramValues;
        for (int i = 0; i < 8; ++i) {
            paramValues[i] = params[i].tick();
        }
        
        // Convert to processing values
        const float inputGain = std::pow(10.0f, paramValues[kParamGain] * 40.0f / 20.0f);
        const float outputGain = std::pow(10.0f, ((paramValues[kParamOutput] - 0.5f) * 40.0f) / 20.0f);
        const float wetMix = paramValues[kParamMix];
        const float dryMix = 1.0f - wetMix;
        const float compression = paramValues[kParamPeakReduction];
        const float threshold = 1.0f - compression * 0.8f;
        const float kneeWidth = paramValues[kParamKnee] * 0.3f;
        const float harmonics = paramValues[kParamHarmonics];
        const bool useEmphasis = paramValues[kParamEmphasis] > 0.5f;
        const bool stereoLink = paramValues[kParamStereoLink] > 0.5f && numChannels > 1;
        
        // Silence detection
        float blockRMS = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                blockRMS += data[i] * data[i];
            }
        }
        blockRMS = std::sqrt(blockRMS / (numChannels * numSamples));
        
        if (blockRMS < kSilenceThreshold) {
            silenceCounter++;
            if (silenceCounter > 10) {
                isSilent = true;
                // Fast-path for silence
                buffer.clear();
                return;
            }
        } else {
            silenceCounter = 0;
            isSilent = false;
        }
        
        // Thermal modeling (once per block)
        static float thermalPhase = 0.0f;
        thermalPhase += 0.0001f;
        thermalDrift = 1.0f + 0.002f * std::sin(thermalPhase);
        
        // Update component aging
        componentAge += numSamples / (sampleRate * 3600.0f);
        
        // Process stereo-linked detection if needed
        if (stereoLink) {
            float maxReduction = 0.0f;
            
            for (int ch = 0; ch < std::min(numChannels, 2); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                auto& channel = channels[ch];
                
                for (int i = 0; i < numSamples; ++i) {
                    float input = data[i] * inputGain;
                    float level = channel.detector.detect(input);
                    
                    if (level > threshold) {
                        float excess = level - threshold;
                        float reduction = softKnee(excess, kneeWidth);
                        maxReduction = std::max(maxReduction, reduction);
                    }
                }
            }
            
            stereoReduction = stereoReduction * 0.9f + maxReduction * 0.1f;
        }
        
        // Main processing loop
        for (int ch = 0; ch < numChannels; ++ch) {
            if (ch >= 2) break;  // Only process stereo
            
            float* data = buffer.getWritePointer(ch);
            auto& channel = channels[ch];
            
            // Set thermal factor
            channel.optoCell.setThermalFactor(thermalDrift);
            
            // Lambda for oversampled processing
            auto processOversampled = [&](float* osData, int osNumSamples) {
                for (int i = 0; i < osNumSamples; ++i) {
                    float sample = osData[i];
                    
                    // Input gain
                    sample *= inputGain;
                    
                    // Input tube stage
                    if (harmonics > 0.01f) {
                        sample = channel.tubeSat.process(sample, harmonics * 0.3f, componentAge);
                    }
                    
                    // Pre-emphasis
                    if (useEmphasis) {
                        sample = channel.preEmphasis.processPreEmphasis(sample);
                    }
                    
                    // Detection
                    float level = channel.detector.detect(sample);
                    float targetBrightness = 0.0f;
                    
                    if (stereoLink) {
                        targetBrightness = stereoReduction;
                    } else {
                        if (level > threshold) {
                            float excess = level - threshold;
                            targetBrightness = softKnee(excess, kneeWidth) * 2.0f;
                            targetBrightness = std::min(1.0f, targetBrightness);
                        }
                    }
                    
                    // Update opto cell
                    channel.optoCell.update(targetBrightness * compression, sampleRate * kOversampleFactor);
                    
                    // Get gain reduction
                    float gainReduction = channel.optoCell.getGainReduction();
                    float gain = 1.0f - (gainReduction * compression);
                    
                    // Smooth gain changes
                    gain = channel.lastGain + (gain - channel.lastGain) * 0.05f;
                    channel.lastGain = gain;
                    
                    // Apply compression
                    sample *= gain;
                    
                    // De-emphasis
                    if (useEmphasis) {
                        sample = channel.deEmphasis.processDeEmphasis(sample);
                    }
                    
                    // Output tube stage
                    if (harmonics > 0.01f) {
                        sample = channel.tubeSat.process(sample, harmonics * 0.5f, componentAge);
                    }
                    
                    // Output gain
                    sample *= outputGain;
                    
                    osData[i] = sample;
                }
            };
            
            // Create temporary buffers for processing
            std::vector<float> tempIn(numSamples);
            std::vector<float> tempOut(numSamples);
            
            // Copy and store dry signal
            std::memcpy(tempIn.data(), data, numSamples * sizeof(float));
            
            // Process with oversampling
            if (metrics.oversamplingActive.load()) {
                channel.oversampler.processBlock(tempIn.data(), tempOut.data(), 
                                                numSamples, processOversampled);
            } else {
                std::memcpy(tempOut.data(), tempIn.data(), numSamples * sizeof(float));
                processOversampled(tempOut.data(), numSamples);
            }
            
            // Apply DC blocking and mix
            for (int i = 0; i < numSamples; ++i) {
                float processed = channel.dcBlocker.process(tempOut[i]);
                data[i] = processed * wetMix + tempIn[i] * dryMix;
                
                // Final soft clipping
                if (std::fabs(data[i]) > 0.95f) {
                    data[i] = FastMath::tanh(data[i] * 0.9f) * 1.05f;
                }
            }
        }
    }
    
    // Soft knee calculation
    float softKnee(float excess, float kneeWidth) const noexcept {
        if (kneeWidth <= 0.0f || excess <= 0.0f) {
            return excess;
        }
        
        if (excess <= kneeWidth) {
            float t = excess / kneeWidth;
            return kneeWidth * t * t * 0.5f;
        }
        
        return excess - kneeWidth * 0.5f;
    }
};

// Static parameter names
const std::array<juce::String, 8> 
    VintageOptoCompressor_Platinum::kParameterNames = {
    "Gain",           // kParamGain
    "Peak Reduction", // kParamPeakReduction
    "HF Emphasis",    // kParamEmphasis
    "Output",         // kParamOutput
    "Mix",            // kParamMix
    "Knee",           // kParamKnee
    "Harmonics",      // kParamHarmonics
    "Stereo Link"     // kParamStereoLink
};

// Constructor/Destructor
VintageOptoCompressor_Platinum::VintageOptoCompressor_Platinum() 
    : pImpl(std::make_unique<Impl>()) {
    
    // Enable denormal prevention globally
    #if HAS_SSE2
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    #endif
}

VintageOptoCompressor_Platinum::~VintageOptoCompressor_Platinum() = default;

// Core interface implementation
void VintageOptoCompressor_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->sampleRate = sampleRate;
    
    // Set parameter smoothing
    for (auto& param : pImpl->params) {
        param.setSmoothingTime(50.0f, sampleRate);
    }
    
    // Prepare channels
    for (auto& channel : pImpl->channels) {
        channel.prepare(sampleRate);
    }
}

void VintageOptoCompressor_Platinum::process(juce::AudioBuffer<float>& buffer) {
    // CPU usage tracking
    const auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process audio
    pImpl->processBlockInternal(buffer);
    
    // Update metrics
    const auto endTime = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration<float>(endTime - startTime).count();
    const float blockTime = buffer.getNumSamples() / static_cast<float>(pImpl->sampleRate);
    const float cpuUsage = (duration / blockTime) * 100.0f;
    
    pImpl->metrics.cpuUsage.store(cpuUsage);
    float peak = pImpl->metrics.peakCpuUsage.load();
    if (cpuUsage > peak) {
        pImpl->metrics.peakCpuUsage.store(cpuUsage);
    }
}

void VintageOptoCompressor_Platinum::reset() {
    for (auto& channel : pImpl->channels) {
        channel.reset();
    }
    pImpl->stereoReduction = 0.0f;
    pImpl->silenceCounter = 0;
    pImpl->isSilent = false;
}

void VintageOptoCompressor_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index >= 0 && index < 8) {
            pImpl->params[index].set(value);
        }
    }
}

juce::String VintageOptoCompressor_Platinum::getParameterName(int index) const {
    if (index >= 0 && index < 8) {
        return kParameterNames[index];
    }
    return juce::String();
}

const VintageOptoCompressor_Platinum::PerformanceMetrics& 
VintageOptoCompressor_Platinum::getMetrics() const noexcept {
    return pImpl->metrics;
}