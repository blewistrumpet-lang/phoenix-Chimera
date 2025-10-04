// MonoMaker_Platinum.cpp - Frequency-Selective Mono Conversion Implementation
#include "MonoMaker_Platinum.h"
#include <cmath>
#include <algorithm>
#include <numeric>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

namespace {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float EPSILON = 1e-10f;
    constexpr float DB_TO_LINEAR = 0.05776226504666210911810267678818f;  // ln(10)/20
}

//==============================================================================
// Implementation Class
//==============================================================================
class MonoMaker_Platinum::Impl {
public:
    //==========================================================================
    // Butterworth Filter for Crossover
    //==========================================================================
    class ButterworthFilter {
        struct Biquad {
            float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
            float a1 = 0.0f, a2 = 0.0f;
            float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
            
            float process(float input) {
                const float y = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                x2 = x1; x1 = input;
                y2 = y1; y1 = y;
                
                // Denormal prevention
                if (std::abs(y1) < EPSILON) y1 = 0.0f;
                if (std::abs(y2) < EPSILON) y2 = 0.0f;
                
                return y;
            }
            
            void reset() {
                x1 = x2 = y1 = y2 = 0.0f;
            }
        };
        
        static constexpr int MAX_ORDER = 8;  // Up to 48dB/oct (8*6)
        Biquad stages[2][MAX_ORDER/2];      // [channel][stage]
        int numStages = 2;
        bool isHighpass = false;
        
    public:
        void setFrequency(float freq, float sampleRate, int order, bool hp = false) {
            isHighpass = hp;
            numStages = (order + 1) / 2;  // Each biquad is 2nd order
            
            const float omega = TWO_PI * freq / sampleRate;
            
            for (int stage = 0; stage < numStages; ++stage) {
                // Butterworth pole angles
                const float poleAngle = PI * (2.0f * stage + 1.0f) / (2.0f * order);
                const float q = 1.0f / (2.0f * std::sin(poleAngle));
                
                // Calculate coefficients
                const float cosw = std::cos(omega);
                const float sinw = std::sin(omega);
                const float alpha = sinw / (2.0f * q);
                
                const float norm = 1.0f / (1.0f + alpha);
                
                if (hp) {
                    // Highpass
                    const float b0 = (1.0f + cosw) * 0.5f * norm;
                    const float b1 = -(1.0f + cosw) * norm;
                    const float b2 = b0;
                    const float a1 = -2.0f * cosw * norm;
                    const float a2 = (1.0f - alpha) * norm;
                    
                    for (int ch = 0; ch < 2; ++ch) {
                        stages[ch][stage].b0 = b0;
                        stages[ch][stage].b1 = b1;
                        stages[ch][stage].b2 = b2;
                        stages[ch][stage].a1 = a1;
                        stages[ch][stage].a2 = a2;
                    }
                } else {
                    // Lowpass
                    const float b0 = (1.0f - cosw) * 0.5f * norm;
                    const float b1 = (1.0f - cosw) * norm;
                    const float b2 = b0;
                    const float a1 = -2.0f * cosw * norm;
                    const float a2 = (1.0f - alpha) * norm;
                    
                    for (int ch = 0; ch < 2; ++ch) {
                        stages[ch][stage].b0 = b0;
                        stages[ch][stage].b1 = b1;
                        stages[ch][stage].b2 = b2;
                        stages[ch][stage].a1 = a1;
                        stages[ch][stage].a2 = a2;
                    }
                }
            }
        }
        
        float process(float input, int channel) {
            float output = input;
            for (int i = 0; i < numStages; ++i) {
                output = stages[channel][i].process(output);
            }
            return output;
        }
        
        void reset() {
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < MAX_ORDER/2; ++i) {
                    stages[ch][i].reset();
                }
            }
        }
    };
    
    //==========================================================================
    // DC Blocking Filter
    //==========================================================================
    class DCBlocker {
        float x1 = 0.0f;
        float y1 = 0.0f;
        const float R = 0.995f;  // Cutoff around 8Hz at 48kHz
        
    public:
        float process(float input) {
            const float y = input - x1 + R * y1;
            x1 = input;
            y1 = y;
            
            // Denormal prevention
            if (std::abs(y1) < EPSILON) y1 = 0.0f;
            
            return y;
        }
        
        void reset() {
            x1 = y1 = 0.0f;
        }
    };
    
    //==========================================================================
    // Phase Correlation Meter
    //==========================================================================
    class CorrelationMeter {
        static constexpr size_t BUFFER_SIZE = 512;
        float bufferL[BUFFER_SIZE] = {};
        float bufferR[BUFFER_SIZE] = {};
        size_t writePos = 0;
        float correlation = 0.0f;
        float monoCompatibility = 1.0f;
        
    public:
        void process(float left, float right) {
            bufferL[writePos] = left;
            bufferR[writePos] = right;
            writePos = (writePos + 1) % BUFFER_SIZE;
            
            // Update every 32 samples for efficiency
            if ((writePos % 32) == 0) {
                updateMeters();
            }
        }
        
        float getCorrelation() const { return correlation; }
        float getMonoCompatibility() const { return monoCompatibility; }
        
    private:
        void updateMeters() {
            // Calculate correlation
            float sumL = 0.0f, sumR = 0.0f;
            float sumL2 = 0.0f, sumR2 = 0.0f, sumLR = 0.0f;
            
            for (size_t i = 0; i < BUFFER_SIZE; ++i) {
                const float l = bufferL[i];
                const float r = bufferR[i];
                sumL += l;
                sumR += r;
                sumL2 += l * l;
                sumR2 += r * r;
                sumLR += l * r;
            }
            
            const float invSize = 1.0f / BUFFER_SIZE;
            const float meanL = sumL * invSize;
            const float meanR = sumR * invSize;
            const float varL = sumL2 * invSize - meanL * meanL;
            const float varR = sumR2 * invSize - meanR * meanR;
            const float covar = sumLR * invSize - meanL * meanR;
            
            const float denom = std::sqrt(varL * varR);
            correlation = (denom > EPSILON) ? (covar / denom) : 0.0f;
            
            // Calculate mono compatibility (RMS of sum vs difference)
            float sumMono = 0.0f, sumSide = 0.0f;
            for (size_t i = 0; i < BUFFER_SIZE; ++i) {
                const float m = bufferL[i] + bufferR[i];
                const float s = bufferL[i] - bufferR[i];
                sumMono += m * m;
                sumSide += s * s;
            }
            
            const float rmsMonoMid = std::sqrt(sumMono * invSize);
            const float rmsSide = std::sqrt(sumSide * invSize);
            
            // Compatibility score: 1.0 = perfect mono, 0.0 = out of phase
            if (rmsMonoMid + rmsSide > EPSILON) {
                monoCompatibility = rmsMonoMid / (rmsMonoMid + rmsSide);
            } else {
                monoCompatibility = 1.0f;
            }
        }
    };
    
    //==========================================================================
    // Stereo Width Processor
    //==========================================================================
    class StereoWidthProcessor {
    public:
        void process(float& left, float& right, float width) {
            const float mid = (left + right) * 0.5f;
            const float side = (left - right) * 0.5f;
            
            // Width: 0 = mono, 1 = normal, 2 = extra wide
            const float processedSide = side * width;
            
            left = mid + processedSide;
            right = mid - processedSide;
        }
    };
    
    //==========================================================================
    // Main Implementation
    //==========================================================================
    
    // Processing components
    ButterworthFilter lowpassL, lowpassR;   // Extract low frequencies
    ButterworthFilter highpassL, highpassR; // Extract high frequencies
    DCBlocker dcBlockerL, dcBlockerR;
    CorrelationMeter correlationMeter;
    StereoWidthProcessor widthProcessor;
    
    // Parameters
    struct Parameters {
        std::atomic<float> frequency{0.3f};      // 20Hz-1kHz log scale
        std::atomic<float> slope{0.5f};          // 6-48 dB/oct
        std::atomic<float> mode{0.0f};           // Processing mode
        std::atomic<float> bassMono{1.0f};       // Bass mono amount
        std::atomic<float> preservePhase{0.0f};  // Phase preservation
        std::atomic<float> dcFilter{1.0f};       // DC filter on/off
        std::atomic<float> widthAbove{1.0f};     // Width above cutoff
        std::atomic<float> outputGain{0.5f};     // -6 to +6 dB
    } params;
    
    // Parameter smoothing
    struct Smoother {
        float current = 0.0f;
        float target = 0.0f;
        float coeff = 0.0f;
        
        void setCoeff(double sampleRate, float timeMs) {
            coeff = std::exp(-1.0f / (sampleRate * timeMs * 0.001f));
        }
        
        void setTarget(float t) { target = t; }
        void reset(float value) { current = target = value; }
        
        float tick() {
            current += (target - current) * (1.0f - coeff);
            return current;
        }
    };
    
    struct {
        Smoother frequency;
        Smoother bassMono;
        Smoother widthAbove;
        Smoother outputGain;
    } smoothers;
    
    // State
    float sampleRate = 48000.0f;
    ProcessingMode currentMode = ProcessingMode::STANDARD;
    std::atomic<float> currentCorrelation{0.0f};
    std::atomic<float> currentMonoCompatibility{1.0f};
    std::atomic<float> currentCutoff{100.0f};
    std::atomic<bool> isProcessing{false};
    
    //==========================================================================
    // Constructor
    //==========================================================================
    Impl() {
        #if HAS_SSE2
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
        #endif
    }
    
    //==========================================================================
    // Processing
    //==========================================================================
    void process(juce::AudioBuffer<float>& buffer) {
        DenormalGuard guard;
        
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        if (numChannels < 2 || numSamples == 0) {
            isProcessing.store(false);
            return;
        }
        
        isProcessing.store(true);
        
        // Update mode
        const float modeValue = params.mode.load();
        if (modeValue < 0.33f) {
            currentMode = ProcessingMode::STANDARD;
        } else if (modeValue < 0.67f) {
            currentMode = ProcessingMode::ELLIPTICAL;
        } else {
            currentMode = ProcessingMode::MID_SIDE;
        }
        
        // Calculate frequency and slope
        const float freqNorm = params.frequency.load();
        const float frequency = 20.0f * std::pow(50.0f, freqNorm);  // 20Hz to 1kHz
        currentCutoff.store(frequency);
        
        const float slopeNorm = params.slope.load();
        const int filterOrder = 1 + static_cast<int>(slopeNorm * 7.0f);  // 1-8 (6-48 dB/oct)
        
        // Update filters
        lowpassL.setFrequency(frequency, sampleRate, filterOrder, false);
        lowpassR.setFrequency(frequency, sampleRate, filterOrder, false);
        highpassL.setFrequency(frequency, sampleRate, filterOrder, true);
        highpassR.setFrequency(frequency, sampleRate, filterOrder, true);
        
        // Update smoothers
        smoothers.frequency.setTarget(frequency);
        smoothers.bassMono.setTarget(params.bassMono.load());
        smoothers.widthAbove.setTarget(params.widthAbove.load());
        smoothers.outputGain.setTarget((params.outputGain.load() - 0.5f) * 12.0f);  // -6 to +6 dB
        
        // Process audio
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        
        const bool useDCFilter = params.dcFilter.load() > 0.5f;
        
        for (int i = 0; i < numSamples; ++i) {
            float inL = left[i];
            float inR = right[i];
            
            // DC filtering if enabled
            if (useDCFilter) {
                inL = dcBlockerL.process(inL);
                inR = dcBlockerR.process(inR);
            }
            
            // Split into frequency bands
            const float lowL = lowpassL.process(inL, 0);
            const float lowR = lowpassR.process(inR, 1);
            const float highL = highpassL.process(inL, 0);
            const float highR = highpassR.process(inR, 1);
            
            // Process based on mode
            float processedLowL = lowL;
            float processedLowR = lowR;
            float processedHighL = highL;
            float processedHighR = highR;
            
            const float monoAmount = smoothers.bassMono.tick();
            
            switch (currentMode) {
                case ProcessingMode::STANDARD: {
                    // Simple mono conversion below frequency
                    const float lowMono = (lowL + lowR) * 0.5f;
                    processedLowL = lowL * (1.0f - monoAmount) + lowMono * monoAmount;
                    processedLowR = lowR * (1.0f - monoAmount) + lowMono * monoAmount;
                    break;
                }
                
                case ProcessingMode::ELLIPTICAL: {
                    // Elliptical EQ - reduce side information below cutoff
                    const float lowMid = (lowL + lowR) * 0.5f;
                    const float lowSide = (lowL - lowR) * 0.5f;
                    const float ellipticalSide = lowSide * (1.0f - monoAmount);
                    processedLowL = lowMid + ellipticalSide;
                    processedLowR = lowMid - ellipticalSide;
                    break;
                }
                
                case ProcessingMode::MID_SIDE: {
                    // Full M/S processing
                    const float lowMid = (lowL + lowR) * 0.5f;
                    const float lowSide = (lowL - lowR) * 0.5f;
                    const float highMid = (highL + highR) * 0.5f;
                    const float highSide = (highL - highR) * 0.5f;
                    
                    // Process low frequencies
                    const float processedLowSide = lowSide * (1.0f - monoAmount);
                    processedLowL = lowMid + processedLowSide;
                    processedLowR = lowMid - processedLowSide;
                    
                    // Process high frequencies with width control
                    const float width = smoothers.widthAbove.tick();
                    const float processedHighSide = highSide * width;
                    processedHighL = highMid + processedHighSide;
                    processedHighR = highMid - processedHighSide;
                    break;
                }
            }
            
            // Apply width to high frequencies (except in M/S mode where it's already done)
            if (currentMode != ProcessingMode::MID_SIDE) {
                const float width = smoothers.widthAbove.tick();
                widthProcessor.process(processedHighL, processedHighR, width);
            }
            
            // Combine bands
            float outL = processedLowL + processedHighL;
            float outR = processedLowR + processedHighR;
            
            // Apply output gain
            const float gainDb = smoothers.outputGain.tick();
            const float gain = std::exp(gainDb * DB_TO_LINEAR);
            outL *= gain;
            outR *= gain;
            
            // Write output
            left[i] = outL;
            right[i] = outR;
            
            // Update meters
            correlationMeter.process(outL, outR);
        }
        
        // Store meter values
        currentCorrelation.store(correlationMeter.getCorrelation());
        currentMonoCompatibility.store(correlationMeter.getMonoCompatibility());
        
        // Final safety scrub
        scrubBuffer(buffer);
    }
    
    void prepareToPlay(double sr, int samplesPerBlock) {
        sampleRate = static_cast<float>(sr);
        
        // Setup smoothers
        smoothers.frequency.setCoeff(sr, 50.0f);
        smoothers.bassMono.setCoeff(sr, 20.0f);
        smoothers.widthAbove.setCoeff(sr, 20.0f);
        smoothers.outputGain.setCoeff(sr, 20.0f);
        
        // Initialize smoothers
        smoothers.frequency.reset(100.0f);
        smoothers.bassMono.reset(1.0f);
        smoothers.widthAbove.reset(1.0f);
        smoothers.outputGain.reset(0.0f);
        
        reset();
    }
    
    void reset() {
        lowpassL.reset();
        lowpassR.reset();
        highpassL.reset();
        highpassR.reset();
        dcBlockerL.reset();
        dcBlockerR.reset();
    }
};

//==============================================================================
// Public Interface Implementation
//==============================================================================

MonoMaker_Platinum::MonoMaker_Platinum() : pImpl(std::make_unique<Impl>()) {}
MonoMaker_Platinum::~MonoMaker_Platinum() = default;

void MonoMaker_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->prepareToPlay(sampleRate, samplesPerBlock);
}

void MonoMaker_Platinum::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void MonoMaker_Platinum::reset() {
    pImpl->reset();
}

void MonoMaker_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (static_cast<ParamID>(index)) {
            case ParamID::FREQUENCY:      pImpl->params.frequency.store(value); break;
            case ParamID::SLOPE:          pImpl->params.slope.store(value); break;
            case ParamID::MODE:           pImpl->params.mode.store(value); break;
            case ParamID::BASS_MONO:      pImpl->params.bassMono.store(value); break;
            case ParamID::PRESERVE_PHASE: pImpl->params.preservePhase.store(value); break;
            case ParamID::DC_FILTER:      pImpl->params.dcFilter.store(value); break;
            case ParamID::WIDTH_ABOVE:    pImpl->params.widthAbove.store(value); break;
            case ParamID::OUTPUT_GAIN:    pImpl->params.outputGain.store(value); break;
        }
    }
}

juce::String MonoMaker_Platinum::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::FREQUENCY:      return "Frequency";
        case ParamID::SLOPE:          return "Slope";
        case ParamID::MODE:           return "Mode";
        case ParamID::BASS_MONO:      return "Bass Mono";
        case ParamID::PRESERVE_PHASE: return "Preserve Phase";
        case ParamID::DC_FILTER:      return "DC Filter";
        case ParamID::WIDTH_ABOVE:    return "Width Above";
        case ParamID::OUTPUT_GAIN:    return "Output Gain";
        default:                      return "";
    }
}

float MonoMaker_Platinum::getPhaseCorrelation() const {
    return pImpl->currentCorrelation.load();
}

float MonoMaker_Platinum::getMonoCompatibility() const {
    return pImpl->currentMonoCompatibility.load();
}

std::pair<float, float> MonoMaker_Platinum::getStereoWidth() const {
    // Width below cutoff (based on bass mono amount)
    const float widthBelow = 1.0f - pImpl->params.bassMono.load();
    // Width above cutoff
    const float widthAbove = pImpl->params.widthAbove.load();
    return {widthBelow, widthAbove};
}

bool MonoMaker_Platinum::isProcessing() const {
    return pImpl->isProcessing.load();
}

float MonoMaker_Platinum::getCurrentCutoff() const {
    return pImpl->currentCutoff.load();
}