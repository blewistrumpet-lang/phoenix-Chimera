// StereoChorus_Reference.cpp
// Reference implementation demonstrating studio-grade DSP practices

#include "StereoChorus_Reference.h"
#include <cmath>
#include <algorithm>

// ========== Implementation Details (PIMPL) ==========
struct StereoChorus_Reference::Impl {
    // Core parameters
    double sampleRate = 44100.0;
    int maxBlockSize = 512;
    int currentBlockSize = 512;
    
    // Latency tracking (for any lookahead/processing delay)
    int latencySamples = 0;
    
    // Transport info for tempo sync
    EngineBase::TransportInfo transport;
    bool tempoSyncEnabled = false;
    
    // ========== Thread-safe Parameters (lock-free atomics) ==========
    AtomicParam rate{0.5f};        // Normalized 0-1
    AtomicParam depth{0.3f};       // Normalized 0-1
    AtomicParam delay{0.3f};       // Normalized 0-1
    AtomicParam feedback{0.3f};    // Normalized 0-1
    AtomicParam width{0.8f};       // Normalized 0-1
    AtomicParam mix{0.5f};         // Normalized 0-1
    AtomicParam sync{0.0f};        // 0 or 1
    
    // ========== Smoothed Parameters (avoid zipper noise) ==========
    MultiRateSmoother rateSmooth;
    MultiRateSmoother depthSmooth;
    MultiRateSmoother delaySmooth;
    MultiRateSmoother feedbackSmooth;
    MultiRateSmoother widthSmooth;
    MultiRateSmoother mixSmooth;
    
    // ========== Bypass Management ==========
    BypassRamp bypassRamp;
    bool isBypassed = false;
    
    // ========== DSP Components per Channel ==========
    static constexpr int kMaxChannels = 2;
    static constexpr int kMaxDelayMs = 100;  // Maximum delay buffer size
    
    struct ChannelState {
        CircularBuffer<float> delayLine;
        OnePoleFilter highpass;  // DC blocking in feedback path
        OnePoleFilter lowpass;   // Darkness control in feedback
        DCBlocker dcBlocker;     // Additional DC protection
        float lfoPhase = 0.0f;
        float feedbackSample = 0.0f;
        
        void prepare(double sr, int maxDelaySamples) {
            delayLine.setSize(maxDelaySamples);
            highpass.setCutoff(20.0f, sr);    // 20Hz highpass
            lowpass.setCutoff(8000.0f, sr);   // 8kHz lowpass
            dcBlocker.prepare(sr);
            reset();
        }
        
        void reset() {
            delayLine.clear();
            highpass.reset();
            lowpass.reset();
            dcBlocker.reset();
            lfoPhase = 0.0f;
            feedbackSample = 0.0f;
        }
    };
    
    std::array<ChannelState, kMaxChannels> channels;
    
    // ========== Level Metering ==========
    LevelMeter inputMeter;
    LevelMeter outputMeter;
    
    // ========== Initialization ==========
    void prepare(double sr, int blockSize) {
        sampleRate = std::max(8000.0, sr);
        currentBlockSize = blockSize;
        
        // Calculate max delay samples with headroom
        const int maxDelaySamples = static_cast<int>(kMaxDelayMs * 0.001 * sampleRate) + 512;
        
        // Prepare all channels
        for (auto& ch : channels) {
            ch.prepare(sampleRate, maxDelaySamples);
        }
        
        // Setup parameter smoothers with appropriate rates
        rateSmooth.prepare(sampleRate, MultiRateSmoother::Medium);    // LFO rate changes
        depthSmooth.prepare(sampleRate, MultiRateSmoother::Fast);     // Modulation depth
        delaySmooth.prepare(sampleRate, MultiRateSmoother::Medium);   // Base delay time
        feedbackSmooth.prepare(sampleRate, MultiRateSmoother::Fast);  // Feedback amount
        widthSmooth.prepare(sampleRate, MultiRateSmoother::Slow);     // Stereo width
        mixSmooth.prepare(sampleRate, MultiRateSmoother::Fast);       // Dry/wet mix
        
        // Setup bypass ramping (5ms ramp)
        bypassRamp.prepare(sampleRate, 5.0);
        
        // Setup meters
        inputMeter.prepare(sampleRate, 0.1, 100.0);
        outputMeter.prepare(sampleRate, 0.1, 100.0);
        
        reset();
    }
    
    void reset() {
        for (auto& ch : channels) {
            ch.reset();
        }
        
        // Reset smoothers to current target values
        rateSmooth.snap(rate.get());
        depthSmooth.snap(depth.get());
        delaySmooth.snap(delay.get());
        feedbackSmooth.snap(feedback.get());
        widthSmooth.snap(width.get());
        mixSmooth.snap(mix.get());
        
        inputMeter.reset();
        outputMeter.reset();
    }
    
    // ========== Main Processing ==========
    void processBlock(juce::AudioBuffer<float>& buffer) {
        // RAII denormal protection for this entire block
        DenormalGuard denormGuard;
        
        const int numChannels = std::min(buffer.getNumChannels(), kMaxChannels);
        const int numSamples = buffer.getNumSamples();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        // Process bypass state
        const float bypassGain = bypassRamp.processSample();
        
        // Quick bypass: skip processing if fully bypassed
        if (bypassRamp.isBypassed()) {
            return;  // Audio passes through unchanged
        }
        
        // ========== Per-sample processing ==========
        for (int sample = 0; sample < numSamples; ++sample) {
            
            // Smooth parameters once per sample
            const float currentRate = rateSmooth.process(rate.get());
            const float currentDepth = depthSmooth.process(depth.get());
            const float currentDelay = delaySmooth.process(delay.get());
            const float currentFeedback = feedbackSmooth.process(feedback.get());
            const float currentWidth = widthSmooth.process(width.get());
            const float currentMix = mixSmooth.process(mix.get());
            const bool syncEnabled = sync.get() > 0.5f;
            
            // Convert normalized parameters to actual values
            float lfoRateHz = 0.1f + currentRate * 9.9f;  // 0.1 to 10 Hz
            
            // Tempo sync if enabled and transport available
            if (syncEnabled && transport.bpm > 0) {
                // Sync to musical divisions (1/4, 1/8, 1/16, etc.)
                const float beatsPerSecond = transport.bpm / 60.0f;
                const int division = static_cast<int>(currentRate * 7.0f);  // 0-7 = whole to 1/32
                const float divisor = std::pow(2.0f, division);
                lfoRateHz = beatsPerSecond * divisor;
            }
            
            const float depthMs = currentDepth * 20.0f;         // 0 to 20ms
            const float delayMs = 5.0f + currentDelay * 45.0f;  // 5 to 50ms
            const float fbAmount = (currentFeedback - 0.5f) * 1.9f;  // -0.95 to 0.95
            
            // Process each channel
            for (int ch = 0; ch < numChannels; ++ch) {
                float* channelData = buffer.getWritePointer(ch);
                const float drySample = channelData[sample];
                
                // Update metering
                if (ch == 0) {
                    inputMeter.processSample(drySample);
                }
                
                // Calculate LFO (sine wave with stereo offset)
                const float stereoOffset = (ch == 1) ? (0.25f + 0.25f * currentWidth) : 0.0f;
                const float lfoValue = std::sin(2.0f * M_PI * (channels[ch].lfoPhase + stereoOffset));
                
                // Update LFO phase
                channels[ch].lfoPhase += lfoRateHz / sampleRate;
                if (channels[ch].lfoPhase >= 1.0f) {
                    channels[ch].lfoPhase -= 1.0f;
                }
                
                // Calculate modulated delay time
                const float modulatedDelayMs = delayMs + depthMs * lfoValue;
                const float delaySamples = (modulatedDelayMs * 0.001f * sampleRate);
                
                // Read from delay line with interpolation
                const float delayedSample = channels[ch].delayLine.readInterpolated(delaySamples);
                
                // Process feedback path with filtering
                float feedbackIn = delayedSample * fbAmount;
                feedbackIn = channels[ch].highpass.process(feedbackIn);  // Remove DC
                feedbackIn = channels[ch].lowpass.process(feedbackIn);   // Tame highs
                feedbackIn = clampSafe(feedbackIn, -0.95f, 0.95f);      // Prevent runaway
                
                // Write to delay line
                const float delayInput = drySample + channels[ch].feedbackSample;
                channels[ch].delayLine.write(delayInput);
                channels[ch].feedbackSample = feedbackIn;
                
                // Apply DC blocking to wet signal
                float wetSample = channels[ch].dcBlocker.process(delayedSample);
                
                // Equal-power crossfade between dry and wet
                const float mixedSample = equalPowerMix(drySample, wetSample, currentMix);
                
                // Apply bypass ramping
                channelData[sample] = drySample + (mixedSample - drySample) * bypassGain;
                
                // Update output metering
                if (ch == 0) {
                    outputMeter.processSample(channelData[sample]);
                }
                
                // Flush denormals
                channelData[sample] = flushDenorm(channelData[sample]);
            }
        }
        
        // Final safety scrub (catches any NaN/Inf that slipped through)
        scrubBuffer(buffer);
    }
    
    // ========== Helpers ==========
    void setMaxBlockSize(int maxSize) {
        maxBlockSize = std::max(1, maxSize);
        // Could pre-allocate temp buffers here if needed
    }
    
    int getLatency() const {
        // Chorus typically has no inherent latency, but if we added
        // lookahead or linear-phase filtering, we'd report it here
        return latencySamples;
    }
};

// ========== Public Interface Implementation ==========

StereoChorus_Reference::StereoChorus_Reference()
    : pImpl(std::make_unique<Impl>()) {
}

StereoChorus_Reference::~StereoChorus_Reference() = default;

void StereoChorus_Reference::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->prepare(sampleRate, samplesPerBlock);
}

void StereoChorus_Reference::process(juce::AudioBuffer<float>& buffer) {
    pImpl->processBlock(buffer);
}

void StereoChorus_Reference::reset() {
    pImpl->reset();
}

void StereoChorus_Reference::updateParameters(const std::map<int, float>& params) {
    // Thread-safe parameter updates using atomics
    for (const auto& [id, value] : params) {
        const float clampedValue = clampSafe(value, 0.0f, 1.0f);
        
        switch (id) {
            case kRate:     pImpl->rate.set(clampedValue); break;
            case kDepth:    pImpl->depth.set(clampedValue); break;
            case kDelay:    pImpl->delay.set(clampedValue); break;
            case kFeedback: pImpl->feedback.set(clampedValue); break;
            case kWidth:    pImpl->width.set(clampedValue); break;
            case kMix:      pImpl->mix.set(clampedValue); break;
            case kSync:     pImpl->sync.set(clampedValue); break;
        }
    }
}

juce::String StereoChorus_Reference::getParameterName(int index) const {
    switch (index) {
        case kRate:     return "Rate";
        case kDepth:    return "Depth";
        case kDelay:    return "Delay";
        case kFeedback: return "Feedback";
        case kWidth:    return "Width";
        case kMix:      return "Mix";
        case kSync:     return "Sync";
        default:        return "";
    }
}

// ========== Extended API ==========

int StereoChorus_Reference::getLatencySamples() const noexcept {
    return pImpl->getLatency();
}

void StereoChorus_Reference::setMaxBlockSizeHint(int maxBlockSize) {
    pImpl->setMaxBlockSize(maxBlockSize);
}

void StereoChorus_Reference::setTransportInfo(const TransportInfo& info) {
    pImpl->transport = info;
}

void StereoChorus_Reference::setBypassed(bool shouldBypass) {
    pImpl->bypassRamp.setBypass(shouldBypass);
    pImpl->isBypassed = shouldBypass;
}

bool StereoChorus_Reference::supportsFeature(Feature f) const noexcept {
    switch (f) {
        case Feature::TempoSync:    return true;
        case Feature::Bypass:       return true;
        default:                    return false;
    }
}