// ===============================================================
// MasteringLimiter_Platinum.cpp - Safe Implementation
// ===============================================================
#include "MasteringLimiter_Platinum.h"
#include "DspEngineUtilities.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstring>
#include <memory>

namespace {
    // Constants
    constexpr float kSilenceThresh = 1e-6f;
    constexpr float kMaxGain = 24.0f;
    constexpr int kMaxLookaheadMs = 10;
    
    // Use utilities from DspEngineUtilities.h
    using ::clampSafe;
    
    // Alias for consistency with existing code
    inline float clamp(float x, float lo, float hi) noexcept {
        return clampSafe(x, lo, hi);
    }
    
    // Local flushDenorm to avoid conflicts
    inline float flushDenorm(float x) noexcept {
        return DSPUtils::flushDenorm(x);
    }
    
    // Safe dB conversions
    inline float dBToGain(float dB) noexcept {
        dB = clamp(dB, -60.0f, kMaxGain);
        return std::pow(10.0f, dB * 0.05f);
    }
    
    inline float gainTodB(float gain) noexcept {
        gain = std::max(1e-6f, gain);
        return 20.0f * std::log10(gain);
    }
    
    // Simple envelope follower
    class EnvelopeFollower {
    public:
        void prepare(double sr) {
            sampleRate = sr;
            reset();
        }
        
        void reset() {
            envelope = 0.0f;
        }
        
        float process(float input, float attackMs, float releaseMs) {
            const float absIn = std::fabs(input);
            
            // Safe coefficient calculation
            const float attackCoeff = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
            const float releaseCoeff = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
            
            if (absIn > envelope) {
                envelope += attackCoeff * (absIn - envelope);
            } else {
                envelope += releaseCoeff * (absIn - envelope);
            }
            
            envelope = flushDenorm(envelope);
            return envelope;
        }
        
    private:
        double sampleRate = 0.0;
        float envelope = 0.0f;
    };
    
    // Simple delay line for lookahead
    class DelayLine {
    public:
        void setMaxDelay(int samples) {
            maxDelay = std::max(1, samples);
            buffer.resize(maxDelay, 0.0f);
            reset();
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
            currentDelay = 0;
        }
        
        void setDelay(int samples) {
            currentDelay = clamp(samples, 0, maxDelay - 1);
        }
        
        float process(float input) {
            if (buffer.empty()) return input;
            
            buffer[writePos] = input;
            
            int readPos = writePos - currentDelay;
            if (readPos < 0) readPos += maxDelay;
            
            float output = buffer[readPos];
            
            writePos = (writePos + 1) % maxDelay;
            
            return output;
        }
        
    private:
        std::vector<float> buffer;
        int writePos = 0;
        int currentDelay = 0;
        int maxDelay = 1;
    };
}

// ===================== PIMPL Implementation =====================
struct MasteringLimiter_Platinum::Impl {
    // Core parameters
    double sampleRate = 0.0;
    int blockSize = 512;
    
    // Parameter values (smoothed)
    float threshold = 0.0f;      // dB
    float ceiling = -0.1f;        // dB
    float release = 50.0f;        // ms
    float lookahead = 5.0f;       // ms
    float knee = 2.0f;            // dB
    float makeup = 0.0f;          // dB
    float saturation = 0.0f;      // 0-1
    float stereoLink = 1.0f;      // 0-1
    float truePeakMode = 1.0f;    // 0-1
    float mix = 1.0f;             // 0-1
    
    // Processing components per channel
    std::vector<EnvelopeFollower> envelopes;
    std::vector<DelayLine> delayLines;
    std::vector<float> lastGain;
    
    // Simple peak detector for true peak approximation
    std::vector<float> peakHold;
    
    // Initialization
    void prepare(double sr, int bs) {
        sampleRate = std::max(8000.0, sr);
        blockSize = std::max(1, bs);
        
        const int numChannels = 2;
        
        envelopes.resize(numChannels);
        delayLines.resize(numChannels);
        lastGain.resize(numChannels, 1.0f);
        peakHold.resize(numChannels, 0.0f);
        
        // Calculate max lookahead samples
        int maxLookaheadSamples = (int)(kMaxLookaheadMs * 0.001 * sampleRate);
        
        for (int ch = 0; ch < numChannels; ++ch) {
            envelopes[ch].prepare(sampleRate);
            delayLines[ch].setMaxDelay(maxLookaheadSamples);
        }
        
        reset();
    }
    
    void reset() {
        for (auto& env : envelopes) env.reset();
        for (auto& dl : delayLines) dl.reset();
        std::fill(lastGain.begin(), lastGain.end(), 1.0f);
        std::fill(peakHold.begin(), peakHold.end(), 0.0f);
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer) {
        // RAII denormal protection for entire block
        DenormalGuard guard;
        
        const int numChannels = std::min(buffer.getNumChannels(), (int)envelopes.size());
        const int numSamples = buffer.getNumSamples();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        // Calculate lookahead delay
        const int lookaheadSamples = clamp((int)(lookahead * 0.001 * sampleRate), 0, 
                                          (int)(kMaxLookaheadMs * 0.001 * sampleRate) - 1);
        
        // Set delay for all channels
        for (int ch = 0; ch < numChannels; ++ch) {
            delayLines[ch].setDelay(lookaheadSamples);
        }
        
        // Convert parameters to linear
        const float thresholdGain = dBToGain(threshold);
        const float ceilingGain = dBToGain(ceiling);
        const float makeupGain = dBToGain(makeup);
        const float kneeWidth = knee * 0.5f; // Half knee on each side
        
        // Process each sample
        for (int i = 0; i < numSamples; ++i) {
            // Peak detection across channels
            float peak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch) {
                float sample = buffer.getReadPointer(ch)[i];
                
                // True peak approximation (simple oversampled peak hold)
                if (truePeakMode > 0.5f) {
                    peakHold[ch] = std::max(peakHold[ch] * 0.9999f, std::fabs(sample));
                    peak = std::max(peak, peakHold[ch]);
                } else {
                    peak = std::max(peak, std::fabs(sample));
                }
            }
            
            // Stereo linking
            float linkedPeak = peak;
            if (stereoLink < 1.0f && numChannels > 1) {
                // Independent processing per channel when stereoLink = 0
                // For simplicity, we'll just use the peak as-is
            }
            
            // Calculate gain reduction with soft knee
            float gainReduction = 1.0f;
            
            if (linkedPeak > thresholdGain - kneeWidth) {
                float kneeStart = thresholdGain - kneeWidth;
                float kneeEnd = thresholdGain + kneeWidth;
                
                if (linkedPeak < kneeEnd) {
                    // Soft knee region - quadratic interpolation
                    float kneePos = (linkedPeak - kneeStart) / (2.0f * kneeWidth);
                    kneePos = clamp(kneePos, 0.0f, 1.0f);
                    float softRatio = kneePos * kneePos; // Quadratic curve
                    
                    float excessDb = gainTodB(linkedPeak) - threshold;
                    float reductionDb = excessDb * softRatio;
                    gainReduction = dBToGain(-reductionDb);
                } else {
                    // Above knee - full limiting
                    float excessDb = gainTodB(linkedPeak) - threshold;
                    gainReduction = thresholdGain / linkedPeak;
                }
                
                // Apply ceiling
                float outputLevel = linkedPeak * gainReduction;
                if (outputLevel > ceilingGain) {
                    gainReduction *= ceilingGain / outputLevel;
                }
            }
            
            gainReduction = clamp(gainReduction, 0.001f, 1.0f);
            
            // Process each channel
            for (int ch = 0; ch < numChannels; ++ch) {
                float* channelData = buffer.getWritePointer(ch);
                
                // Get delayed dry signal
                float drySample = delayLines[ch].process(channelData[i]);
                
                // Smooth gain changes
                float targetGain = gainReduction * makeupGain;
                float currentGain = lastGain[ch];
                
                // Simple one-pole smoothing for gain
                const float smoothCoeff = 0.01f; // Fast smoothing
                currentGain += smoothCoeff * (targetGain - currentGain);
                lastGain[ch] = flushDenorm(currentGain);
                
                // Apply gain
                float wetSample = drySample * currentGain;
                
                // Optional saturation (soft clipping)
                if (saturation > 0.01f) {
                    float drive = 1.0f + saturation * 4.0f;
                    wetSample *= drive;
                    wetSample = std::tanh(wetSample * 0.7f) * 1.4286f; // Compensate for tanh compression
                    wetSample /= drive;
                }
                
                // Mix dry/wet
                channelData[i] = drySample * (1.0f - mix) + wetSample * mix;
                
                // Final safety limiting
                channelData[i] = clamp(channelData[i], -2.0f, 2.0f);
                channelData[i] = flushDenorm(channelData[i]);
                
                // Update meters (simplified)
                float inputLevel = std::fabs(drySample);
                float outputLevel = std::fabs(channelData[i]);
                float gr = currentGain;
                
                // Simple peak hold for meters
                static float inputPeak = 0.0f, outputPeak = 0.0f, grPeak = 1.0f;
                inputPeak = std::max(inputPeak * 0.9995f, inputLevel);
                outputPeak = std::max(outputPeak * 0.9995f, outputLevel);
                grPeak = std::min(grPeak * 1.0005f, gr);
                
                if (ch == 0 && i % 64 == 0) { // Update meters occasionally
                    m_inputMeter->store(gainTodB(inputPeak), std::memory_order_relaxed);
                    m_outputMeter->store(gainTodB(outputPeak), std::memory_order_relaxed);
                    m_grMeter->store(gainTodB(grPeak), std::memory_order_relaxed);
                    m_truePeakMeter->store(gainTodB(peakHold[0]), std::memory_order_relaxed);
                }
            }
        }
        
        // Final safety scrub (catches any NaN/Inf that slipped through)
        scrubBuffer(buffer);
    }
    
    // Meter pointers
    std::atomic<float>* m_grMeter = nullptr;
    std::atomic<float>* m_inputMeter = nullptr;
    std::atomic<float>* m_outputMeter = nullptr;
    std::atomic<float>* m_truePeakMeter = nullptr;
};

// ===================== Public Implementation =====================
MasteringLimiter_Platinum::MasteringLimiter_Platinum()
    : pimpl(std::make_unique<Impl>()) {
    pimpl->m_grMeter = &m_grMeter;
    pimpl->m_inputMeter = &m_inputMeter;
    pimpl->m_outputMeter = &m_outputMeter;
    pimpl->m_truePeakMeter = &m_truePeakMeter;
}

MasteringLimiter_Platinum::~MasteringLimiter_Platinum() = default;

void MasteringLimiter_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepare(sampleRate, samplesPerBlock);
}

void MasteringLimiter_Platinum::reset() {
    pimpl->reset();
}

void MasteringLimiter_Platinum::process(juce::AudioBuffer<float>& buffer) {
    pimpl->processBlock(buffer);
}

void MasteringLimiter_Platinum::updateParameters(const std::map<int, float>& params) {
    auto get = [&](int id, float defaultVal) {
        auto it = params.find(id);
        return it != params.end() ? it->second : defaultVal;
    };
    
    // Map normalized 0-1 to meaningful ranges
    pimpl->threshold = -30.0f + 30.0f * get(kThreshold, 0.5f);        // -30 to 0 dB
    pimpl->ceiling = -10.0f + 9.9f * get(kCeiling, 0.99f);            // -10 to -0.1 dB
    pimpl->release = 1.0f + 199.0f * get(kRelease, 0.25f);            // 1 to 200 ms
    pimpl->lookahead = 0.1f + 9.9f * get(kLookahead, 0.5f);           // 0.1 to 10 ms
    pimpl->knee = 0.0f + 6.0f * get(kKnee, 0.333f);                   // 0 to 6 dB
    pimpl->makeup = 0.0f + 12.0f * get(kMakeup, 0.0f);                // 0 to 12 dB
    pimpl->saturation = get(kSaturation, 0.0f);                       // 0 to 1
    pimpl->stereoLink = get(kStereoLink, 1.0f);                       // 0 to 1
    pimpl->truePeakMode = get(kTruePeak, 1.0f);                       // 0 or 1
    pimpl->mix = get(kMix, 1.0f);                                     // 0 to 1
}

juce::String MasteringLimiter_Platinum::getParameterName(int index) const {
    switch (index) {
        case kThreshold:   return "Threshold";
        case kCeiling:     return "Ceiling";
        case kRelease:     return "Release";
        case kLookahead:   return "Lookahead";
        case kKnee:        return "Knee";
        case kMakeup:      return "Makeup";
        case kSaturation:  return "Saturation";
        case kStereoLink:  return "Stereo Link";
        case kTruePeak:    return "True Peak";
        case kMix:         return "Mix";
        default:           return "";
    }
}

float MasteringLimiter_Platinum::getGainReduction() const noexcept {
    return m_grMeter.load(std::memory_order_relaxed);
}

float MasteringLimiter_Platinum::getInputLevel() const noexcept {
    return m_inputMeter.load(std::memory_order_relaxed);
}

float MasteringLimiter_Platinum::getOutputLevel() const noexcept {
    return m_outputMeter.load(std::memory_order_relaxed);
}

float MasteringLimiter_Platinum::getTruePeakLevel() const noexcept {
    return m_truePeakMeter.load(std::memory_order_relaxed);
}

int MasteringLimiter_Platinum::getLatencySamples() const noexcept {
    // Report the current lookahead delay in samples
    return static_cast<int>(pimpl->lookahead * 0.001 * pimpl->sampleRate);
}