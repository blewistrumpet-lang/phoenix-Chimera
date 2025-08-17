#include "VintageConsoleEQ_Platinum.h"
#include "DspEngineUtilities.h"
#include <cmath>
#include <algorithm>
#include <cstring>

// Platform-specific SIMD headers with detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Professional denormal prevention macros
#if HAS_SSE2
    #define ENABLE_DENORMAL_PREVENTION() \
        _mm_setcsr(_mm_getcsr() | 0x8040)
    #define FLUSH_DENORMAL(x) \
        _mm_store_ss(&(x), _mm_set_ss(x))
#else
    #define ENABLE_DENORMAL_PREVENTION() ((void)0)
    #define FLUSH_DENORMAL(x) \
        do { if (std::abs(x) < 1e-30f) (x) = 0.0f; } while(0)
#endif

//==============================================================================
// Private Implementation
//==============================================================================
struct VintageConsoleEQ_Platinum::Impl {
    // Sample rate and processing info
    float sampleRate = 0.0f;
    int samplesPerBlock = 512;
    std::atomic<float> cpuLoad{0.0f};
    
    // Thread-safe parameters with smoothing
    struct SmoothParam {
        std::atomic<float> target{0.5f};
        float current = 0.5f;
        float smooth = 0.995f;
        
        void setSmoothingTime(float milliseconds, float sr) {
            float freq = 1000.0f / (2.0f * M_PI * milliseconds);
            smooth = std::exp(-2.0f * M_PI * freq / sr);
        }
        
        float getNext() {
            float t = target.load(std::memory_order_relaxed);
            current = t + (current - t) * smooth;
            FLUSH_DENORMAL(current);
            return current;
        }
        
        void reset(float value) {
            target.store(value, std::memory_order_relaxed);
            current = value;
        }
    };
    
    // Parameters
    SmoothParam lowGain;
    SmoothParam lowFreq;
    SmoothParam midGain;
    SmoothParam midFreq;
    SmoothParam midQ;
    SmoothParam highGain;
    SmoothParam highFreq;
    SmoothParam drive;
    SmoothParam consoleType;
    SmoothParam vintage;
    SmoothParam mix;
    
    // Console models with authentic characteristics
    struct ConsoleCharacteristics {
        float saturationKnee;
        float saturationAmount;
        float harmonicProfile[5]; // 2nd through 6th harmonics
        float lowShelfQ;
        float highShelfQ;
        float midQScale;
        float transformerResponse;
        float noiseFloor;
    };
    
    static constexpr ConsoleCharacteristics consoleModels[4] = {
        // SSL 4000 - Clean, surgical
        {0.8f, 0.02f, {0.001f, 0.0005f, 0.0002f, 0.0001f, 0.00005f}, 0.71f, 0.71f, 1.0f, 0.0f, -110.0f},
        // API 550 - Punchy, musical
        {0.6f, 0.05f, {0.003f, 0.002f, 0.001f, 0.0005f, 0.0002f}, 0.85f, 0.85f, 1.2f, 0.3f, -105.0f},
        // Neve 1073 - Warm, transformer-coupled
        {0.5f, 0.08f, {0.005f, 0.003f, 0.002f, 0.001f, 0.0005f}, 0.9f, 0.9f, 0.8f, 0.5f, -100.0f},
        // Pultec - Smooth, passive curves
        {0.7f, 0.04f, {0.002f, 0.001f, 0.0005f, 0.0002f, 0.0001f}, 0.6f, 0.6f, 0.7f, 0.2f, -108.0f}
    };
    
    // Biquad filter with SIMD optimization
    struct alignas(16) BiquadFilter {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        
        // State variables (stereo)
        alignas(16) float x1[2] = {0.0f, 0.0f};
        alignas(16) float x2[2] = {0.0f, 0.0f};
        alignas(16) float y1[2] = {0.0f, 0.0f};
        alignas(16) float y2[2] = {0.0f, 0.0f};
        
        void calculateLowShelf(float freq, float gain, float q, float sr) {
            float omega = 2.0f * M_PI * freq / sr;
            float sinw = std::sin(omega);
            float cosw = std::cos(omega);
            float A = std::pow(10.0f, gain / 40.0f);
            float alpha = sinw / (2.0f * q);
            float beta = std::sqrt(A) / q;
            
            b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw + beta * sinw);
            b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw);
            b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw - beta * sinw);
            float a0 = (A + 1.0f) + (A - 1.0f) * cosw + beta * sinw;
            a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw) / a0;
            a2 = ((A + 1.0f) + (A - 1.0f) * cosw - beta * sinw) / a0;
            
            // Normalize
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
        }
        
        void calculateBell(float freq, float gain, float q, float sr) {
            float omega = 2.0f * M_PI * freq / sr;
            float sinw = std::sin(omega);
            float cosw = std::cos(omega);
            float A = std::pow(10.0f, gain / 40.0f);
            float alpha = sinw / (2.0f * q);
            
            b0 = 1.0f + alpha * A;
            b1 = -2.0f * cosw;
            b2 = 1.0f - alpha * A;
            float a0 = 1.0f + alpha / A;
            a1 = -2.0f * cosw / a0;
            a2 = (1.0f - alpha / A) / a0;
            
            // Normalize
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
        }
        
        void calculateHighShelf(float freq, float gain, float q, float sr) {
            float omega = 2.0f * M_PI * freq / sr;
            float sinw = std::sin(omega);
            float cosw = std::cos(omega);
            float A = std::pow(10.0f, gain / 40.0f);
            float alpha = sinw / (2.0f * q);
            float beta = std::sqrt(A) / q;
            
            b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw + beta * sinw);
            b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw);
            b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw - beta * sinw);
            float a0 = (A + 1.0f) - (A - 1.0f) * cosw + beta * sinw;
            a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw) / a0;
            a2 = ((A + 1.0f) - (A - 1.0f) * cosw - beta * sinw) / a0;
            
            // Normalize
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
        }
        
        void processStereo(float* left, float* right) {
            #if HAS_SSE2
            // SIMD optimized processing for stereo
            __m128 in = _mm_set_ps(0, 0, *right, *left);
            __m128 x1v = _mm_load_ps((float*)&x1[0]);
            __m128 x2v = _mm_load_ps((float*)&x2[0]);
            __m128 y1v = _mm_load_ps((float*)&y1[0]);
            __m128 y2v = _mm_load_ps((float*)&y2[0]);
            
            __m128 b0v = _mm_set1_ps(b0);
            __m128 b1v = _mm_set1_ps(b1);
            __m128 b2v = _mm_set1_ps(b2);
            __m128 a1v = _mm_set1_ps(a1);
            __m128 a2v = _mm_set1_ps(a2);
            
            __m128 out = _mm_mul_ps(b0v, in);
            out = _mm_add_ps(out, _mm_mul_ps(b1v, x1v));
            out = _mm_add_ps(out, _mm_mul_ps(b2v, x2v));
            out = _mm_sub_ps(out, _mm_mul_ps(a1v, y1v));
            out = _mm_sub_ps(out, _mm_mul_ps(a2v, y2v));
            
            // Update state
            _mm_store_ps((float*)&x2[0], x1v);
            _mm_store_ps((float*)&x1[0], in);
            _mm_store_ps((float*)&y2[0], y1v);
            _mm_store_ps((float*)&y1[0], out);
            
            // Extract results
            alignas(16) float result[4];
            _mm_store_ps(result, out);
            *left = result[0];
            *right = result[1];
            #else
            // Scalar fallback
            float outL = b0 * (*left) + b1 * x1[0] + b2 * x2[0] - a1 * y1[0] - a2 * y2[0];
            float outR = b0 * (*right) + b1 * x1[1] + b2 * x2[1] - a1 * y1[1] - a2 * y2[1];
            
            x2[0] = x1[0]; x1[0] = *left; y2[0] = y1[0]; y1[0] = outL;
            x2[1] = x1[1]; x1[1] = *right; y2[1] = y1[1]; y1[1] = outR;
            
            *left = outL;
            *right = outR;
            #endif
            
            FLUSH_DENORMAL(*left);
            FLUSH_DENORMAL(*right);
        }
        
        void reset() {
            std::memset(x1, 0, sizeof(x1));
            std::memset(x2, 0, sizeof(x2));
            std::memset(y1, 0, sizeof(y1));
            std::memset(y2, 0, sizeof(y2));
        }
    };
    
    // EQ bands
    BiquadFilter lowShelf;
    BiquadFilter midBell;
    BiquadFilter highShelf;
    
    // Saturation processors
    struct ConsoleStage {
        float lastSample[2] = {0.0f, 0.0f};
        float dcBlock[2] = {0.0f, 0.0f};
        
        void processSaturation(float& left, float& right, 
                             const ConsoleCharacteristics& console, 
                             float driveAmount, float vintageAmount) {
            // DC blocking (6Hz highpass)
            const float dcAlpha = 0.9992f;
            float tempL = left - lastSample[0] + dcBlock[0] * dcAlpha;
            float tempR = right - lastSample[1] + dcBlock[1] * dcAlpha;
            lastSample[0] = left;
            lastSample[1] = right;
            dcBlock[0] = tempL;
            dcBlock[1] = tempR;
            left = tempL;
            right = tempR;
            
            // Apply drive
            float gain = 1.0f + driveAmount * 5.0f;
            left *= gain;
            right *= gain;
            
            // Console-specific saturation
            float knee = console.saturationKnee;
            float amount = console.saturationAmount * (1.0f + vintageAmount);
            
            // Soft clipping with knee
            auto softClip = [knee, amount](float x) {
                float absX = std::abs(x);
                if (absX < knee) {
                    return x;
                } else {
                    float over = absX - knee;
                    float saturated = knee + std::tanh(over * amount) / amount;
                    return x < 0 ? -saturated : saturated;
                }
            };
            
            left = softClip(left);
            right = softClip(right);
            
            // Harmonic generation (simplified)
            if (console.transformerResponse > 0.0f) {
                float harm2 = console.harmonicProfile[0] * console.transformerResponse;
                float harm3 = console.harmonicProfile[1] * console.transformerResponse;
                
                left += harm2 * left * left * (left < 0 ? -1.0f : 1.0f);
                left += harm3 * left * left * left;
                
                right += harm2 * right * right * (right < 0 ? -1.0f : 1.0f);
                right += harm3 * right * right * right;
            }
            
            // Vintage noise floor
            if (vintageAmount > 0.0f) {
                float noiseLevel = std::pow(10.0f, console.noiseFloor / 20.0f) * vintageAmount;
                // Thread-safe noise generation
                thread_local juce::Random random;
                left += noiseLevel * (random.nextFloat() - 0.5f) * 0.001f;
                right += noiseLevel * (random.nextFloat() - 0.5f) * 0.001f;
            }
            
            FLUSH_DENORMAL(left);
            FLUSH_DENORMAL(right);
        }
        
        void reset() {
            std::memset(lastSample, 0, sizeof(lastSample));
            std::memset(dcBlock, 0, sizeof(dcBlock));
        }
    };
    
    ConsoleStage saturationStage;
    
    // Constructor
    Impl() {
        // Set default smoothing
        const float smoothTime = 20.0f; // milliseconds
        lowGain.setSmoothingTime(smoothTime, sampleRate);
        lowFreq.setSmoothingTime(smoothTime, sampleRate);
        midGain.setSmoothingTime(smoothTime, sampleRate);
        midFreq.setSmoothingTime(smoothTime, sampleRate);
        midQ.setSmoothingTime(smoothTime, sampleRate);
        highGain.setSmoothingTime(smoothTime, sampleRate);
        highFreq.setSmoothingTime(smoothTime, sampleRate);
        drive.setSmoothingTime(smoothTime, sampleRate);
        consoleType.setSmoothingTime(smoothTime, sampleRate);
        vintage.setSmoothingTime(smoothTime, sampleRate);
        mix.setSmoothingTime(smoothTime, sampleRate);
        
        reset();
    }
    
    void reset() {
        lowShelf.reset();
        midBell.reset();
        highShelf.reset();
        saturationStage.reset();
        
        // Reset parameters to center
        lowGain.reset(0.5f);
        lowFreq.reset(0.3f);
        midGain.reset(0.5f);
        midFreq.reset(0.5f);
        midQ.reset(0.5f);
        highGain.reset(0.5f);
        highFreq.reset(0.5f);
        drive.reset(0.3f);
        consoleType.reset(0.5f); // Default to Neve
        vintage.reset(0.3f);
        mix.reset(1.0f);
    }
    
    ConsoleModel getCurrentConsoleModel() const {
        float type = consoleType.current;
        if (type < 0.25f) return ConsoleModel::SSL_4000;
        else if (type < 0.5f) return ConsoleModel::API_550;
        else if (type < 0.75f) return ConsoleModel::NEVE_1073;
        else return ConsoleModel::PULTEC;
    }
    
    FilterResponse getFrequencyResponse(float testFreq) const {
        // Calculate combined magnitude/phase response
        // Simplified - would implement full frequency response calculation
        FilterResponse response;
        response.magnitude = 1.0f;
        response.phase = 0.0f;
        return response;
    }
};

//==============================================================================
// Public Implementation
//==============================================================================
VintageConsoleEQ_Platinum::VintageConsoleEQ_Platinum()
    : pimpl(std::make_unique<Impl>()) {
    ENABLE_DENORMAL_PREVENTION();
}

VintageConsoleEQ_Platinum::~VintageConsoleEQ_Platinum() = default;

void VintageConsoleEQ_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->sampleRate = static_cast<float>(sampleRate);
    pimpl->samplesPerBlock = samplesPerBlock;
    
    // Update smoothing times
    const float smoothTime = 20.0f;
    pimpl->lowGain.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->lowFreq.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->midGain.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->midFreq.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->midQ.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->highGain.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->highFreq.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->drive.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->consoleType.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->vintage.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->mix.setSmoothingTime(smoothTime, pimpl->sampleRate);
    
    reset();
}

void VintageConsoleEQ_Platinum::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Get current console model
    int consoleIndex = static_cast<int>(pimpl->getCurrentConsoleModel());
    const auto& console = Impl::consoleModels[consoleIndex];
    
    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample) {
        // Update parameters (sample-accurate automation)
        float lowGainValue = (pimpl->lowGain.getNext() - 0.5f) * 30.0f; // ±15dB
        float lowFreqValue = 30.0f + pimpl->lowFreq.getNext() * 270.0f; // 30-300Hz
        float midGainValue = (pimpl->midGain.getNext() - 0.5f) * 30.0f; // ±15dB
        float midFreqValue = 200.0f * std::pow(40.0f, pimpl->midFreq.getNext()); // 200Hz-8kHz
        float midQValue = 0.3f + pimpl->midQ.getNext() * 2.7f; // 0.3-3.0
        float highGainValue = (pimpl->highGain.getNext() - 0.5f) * 30.0f; // ±15dB
        float highFreqValue = 3000.0f + pimpl->highFreq.getNext() * 13000.0f; // 3-16kHz
        float driveValue = pimpl->drive.getNext();
        float vintageValue = pimpl->vintage.getNext();
        float mixValue = pimpl->mix.getNext();
        
        // Update filter coefficients (only when changed significantly)
        static float lastLowGain = -999.0f, lastLowFreq = -999.0f;
        static float lastMidGain = -999.0f, lastMidFreq = -999.0f, lastMidQ = -999.0f;
        static float lastHighGain = -999.0f, lastHighFreq = -999.0f;
        
        const float threshold = 0.01f;
        
        if (std::abs(lowGainValue - lastLowGain) > threshold || 
            std::abs(lowFreqValue - lastLowFreq) > threshold) {
            pimpl->lowShelf.calculateLowShelf(lowFreqValue, lowGainValue, 
                                             console.lowShelfQ * (1.0f + vintageValue * 0.3f), 
                                             pimpl->sampleRate);
            lastLowGain = lowGainValue;
            lastLowFreq = lowFreqValue;
        }
        
        if (std::abs(midGainValue - lastMidGain) > threshold || 
            std::abs(midFreqValue - lastMidFreq) > threshold ||
            std::abs(midQValue - lastMidQ) > threshold) {
            pimpl->midBell.calculateBell(midFreqValue, midGainValue, 
                                        midQValue * console.midQScale, 
                                        pimpl->sampleRate);
            lastMidGain = midGainValue;
            lastMidFreq = midFreqValue;
            lastMidQ = midQValue;
        }
        
        if (std::abs(highGainValue - lastHighGain) > threshold || 
            std::abs(highFreqValue - lastHighFreq) > threshold) {
            pimpl->highShelf.calculateHighShelf(highFreqValue, highGainValue, 
                                               console.highShelfQ * (1.0f + vintageValue * 0.3f), 
                                               pimpl->sampleRate);
            lastHighGain = highGainValue;
            lastHighFreq = highFreqValue;
        }
        
        // Process audio
        if (numChannels == 1) {
            // Mono processing
            float* channelData = buffer.getWritePointer(0);
            float drySignal = channelData[sample];
            float left = drySignal;
            float right = drySignal;
            
            // Apply EQ
            pimpl->lowShelf.processStereo(&left, &right);
            pimpl->midBell.processStereo(&left, &right);
            pimpl->highShelf.processStereo(&left, &right);
            
            // Apply console saturation
            pimpl->saturationStage.processSaturation(left, right, console, 
                                                    driveValue, vintageValue);
            
            // Mix control
            channelData[sample] = drySignal * (1.0f - mixValue) + left * mixValue;
            
        } else {
            // Stereo processing
            float* leftData = buffer.getWritePointer(0);
            float* rightData = buffer.getWritePointer(1);
            
            float dryLeft = leftData[sample];
            float dryRight = rightData[sample];
            float left = dryLeft;
            float right = dryRight;
            
            // Apply EQ
            pimpl->lowShelf.processStereo(&left, &right);
            pimpl->midBell.processStereo(&left, &right);
            pimpl->highShelf.processStereo(&left, &right);
            
            // Apply console saturation
            pimpl->saturationStage.processSaturation(left, right, console, 
                                                    driveValue, vintageValue);
            
            // Mix control
            leftData[sample] = dryLeft * (1.0f - mixValue) + left * mixValue;
            rightData[sample] = dryRight * (1.0f - mixValue) + right * mixValue;
        }
    }
    
    // Update CPU load estimate (simplified)
    float load = static_cast<float>(numSamples) / static_cast<float>(pimpl->samplesPerBlock);
    pimpl->cpuLoad.store(load * 0.1f, std::memory_order_relaxed); // Rough estimate
}

void VintageConsoleEQ_Platinum::reset() {
    pimpl->reset();
}

void VintageConsoleEQ_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (index) {
            case LOW_GAIN:
                pimpl->lowGain.target.store(value, std::memory_order_relaxed);
                break;
            case LOW_FREQ:
                pimpl->lowFreq.target.store(value, std::memory_order_relaxed);
                break;
            case MID_GAIN:
                pimpl->midGain.target.store(value, std::memory_order_relaxed);
                break;
            case MID_FREQ:
                pimpl->midFreq.target.store(value, std::memory_order_relaxed);
                break;
            case MID_Q:
                pimpl->midQ.target.store(value, std::memory_order_relaxed);
                break;
            case HIGH_GAIN:
                pimpl->highGain.target.store(value, std::memory_order_relaxed);
                break;
            case HIGH_FREQ:
                pimpl->highFreq.target.store(value, std::memory_order_relaxed);
                break;
            case DRIVE:
                pimpl->drive.target.store(value, std::memory_order_relaxed);
                break;
            case CONSOLE_TYPE:
                pimpl->consoleType.target.store(value, std::memory_order_relaxed);
                break;
            case VINTAGE:
                pimpl->vintage.target.store(value, std::memory_order_relaxed);
                break;
            case MIX:
                pimpl->mix.target.store(value, std::memory_order_relaxed);
                break;
        }
    }
}

juce::String VintageConsoleEQ_Platinum::getParameterName(int index) const {
    switch (index) {
        case LOW_GAIN: return "Low Gain";
        case LOW_FREQ: return "Low Freq";
        case MID_GAIN: return "Mid Gain";
        case MID_FREQ: return "Mid Freq";
        case MID_Q: return "Mid Q";
        case HIGH_GAIN: return "High Gain";
        case HIGH_FREQ: return "High Freq";
        case DRIVE: return "Drive";
        case CONSOLE_TYPE: return "Console Type";
        case VINTAGE: return "Vintage";
        case MIX: return "Mix";
        default: return "";
    }
}

VintageConsoleEQ_Platinum::ConsoleModel VintageConsoleEQ_Platinum::getCurrentConsoleModel() const noexcept {
    return pimpl->getCurrentConsoleModel();
}

float VintageConsoleEQ_Platinum::getCPULoad() const noexcept {
    return pimpl->cpuLoad.load(std::memory_order_relaxed);
}

VintageConsoleEQ_Platinum::FilterResponse VintageConsoleEQ_Platinum::getFrequencyResponse(float frequency) const noexcept {
    return pimpl->getFrequencyResponse(frequency);
}