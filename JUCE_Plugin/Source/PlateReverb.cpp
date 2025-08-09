#include "PlateReverb.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <numeric>

// ParameterSmoother implementation
class PlateReverb::ParameterSmoother {
public:
    ParameterSmoother(double smoothingTime = 0.02) : m_smoothingTime(smoothingTime) {}
    
    void reset(float value) {
        m_current = value;
        m_target.store(value);
        m_coefficient = 1.0f;
    }
    
    void setSampleRate(double sampleRate) {
        m_sampleRate = sampleRate;
        updateCoefficient();
    }
    
    void setSmoothingTime(double seconds) {
        m_smoothingTime = seconds;
        updateCoefficient();
    }
    
    void setTarget(float value) {
        m_target.store(std::clamp(value, 0.0f, 1.0f));
    }
    
    float getTarget() const {
        return m_target.load();
    }
    
    float process() {
        float target = m_target.load();
        m_current += (target - m_current) * m_coefficient;
        
        // Snap to target when very close
        if (std::abs(m_current - target) < 0.0001f) {
            m_current = target;
        }
        
        return m_current;
    }
    
    float getCurrentValue() const { return m_current; }

private:
    void updateCoefficient() {
        if (m_sampleRate > 0 && m_smoothingTime > 0) {
            m_coefficient = 1.0f - std::exp(-1.0 / (m_smoothingTime * m_sampleRate));
        } else {
            m_coefficient = 1.0f;
        }
    }
    
    std::atomic<float> m_target{0.5f};
    float m_current = 0.5f;
    float m_coefficient = 1.0f;
    double m_smoothingTime = 0.02;
    double m_sampleRate = 44100.0;
};

// SoftKneeLimiter implementation
class PlateReverb::SoftKneeLimiter {
public:
    void reset() {
        m_envelope = 0.0f;
    }
    
    void setSampleRate(double sampleRate) {
        m_attackCoeff = std::exp(-1.0 / (0.001 * sampleRate));  // 1ms attack
        m_releaseCoeff = std::exp(-1.0 / (0.010 * sampleRate)); // 10ms release
    }
    
    float process(float input) {
        float inputAbs = std::abs(input);
        
        // Envelope follower
        float targetEnv = inputAbs;
        if (targetEnv > m_envelope) {
            m_envelope = targetEnv + (m_envelope - targetEnv) * m_attackCoeff;
        } else {
            m_envelope = targetEnv + (m_envelope - targetEnv) * m_releaseCoeff;
        }
        
        // Soft knee compression (starts at -6dB)
        const float threshold = 0.5f;
        const float knee = 0.1f;
        
        if (m_envelope < threshold - knee) {
            return input;
        }
        
        float overAmount = 0.0f;
        if (m_envelope > threshold + knee) {
            overAmount = m_envelope - threshold;
        } else {
            // Soft knee region
            float kneeAmount = (m_envelope - threshold + knee) / (2.0f * knee);
            overAmount = knee * kneeAmount * kneeAmount;
        }
        
        // Apply 4:1 compression
        float reduction = 1.0f - (overAmount * 0.75f / std::max(m_envelope, 0.001f));
        
        return input * reduction;
    }

private:
    float m_envelope = 0.0f;
    float m_attackCoeff = 0.0f;
    float m_releaseCoeff = 0.0f;
};

// ButterworthHighpass implementation
class PlateReverb::ButterworthHighpass {
public:
    void reset() {
        m_x1 = m_x2 = 0.0f;
        m_y1 = m_y2 = 0.0f;
    }
    
    void setFrequency(float freq, double sampleRate) {
        float w = 2.0f * M_PI * freq / sampleRate;
        float cosw = std::cos(w);
        float sinw = std::sin(w);
        float alpha = sinw / std::sqrt(2.0f);
        
        float a0 = 1.0f + alpha;
        m_b0 = (1.0f + cosw) / (2.0f * a0);
        m_b1 = -(1.0f + cosw) / a0;
        m_b2 = m_b0;
        m_a1 = -2.0f * cosw / a0;
        m_a2 = (1.0f - alpha) / a0;
    }
    
    float process(float input) {
        float output = m_b0 * input + m_b1 * m_x1 + m_b2 * m_x2 
                      - m_a1 * m_y1 - m_a2 * m_y2;
        
        m_x2 = m_x1;
        m_x1 = input;
        m_y2 = m_y1;
        m_y1 = output;
        
        return output;
    }

private:
    float m_b0 = 1.0f, m_b1 = 0.0f, m_b2 = 0.0f;
    float m_a1 = 0.0f, m_a2 = 0.0f;
    float m_x1 = 0.0f, m_x2 = 0.0f;
    float m_y1 = 0.0f, m_y2 = 0.0f;
};

// OnePoleFilter implementation  
class PlateReverb::OnePoleFilter {
public:
    void reset() {
        m_state = 0.0f;
    }
    
    void setCoefficient(float coeff) {
        m_coefficient = std::clamp(coeff, 0.0f, 0.999f);
    }
    
    float processLowpass(float input) {
        m_state = input * (1.0f - m_coefficient) + m_state * m_coefficient;
        return m_state;
    }
    
    float processHighpass(float input) {
        float lowpass = processLowpass(input);
        return input - lowpass;
    }

private:
    float m_state = 0.0f;
    float m_coefficient = 0.0f;
};

// InterpolatedDelayLine implementation
class PlateReverb::InterpolatedDelayLine {
public:
    InterpolatedDelayLine() = default;
    
    void init(int maxDelaySamples) {
        m_buffer.resize(maxDelaySamples + 4); // Extra samples for interpolation
        reset();
    }
    
    void reset() {
        std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
        m_writePos = 0;
    }
    
    float read(float delaySamples, PerformanceMode mode) const {
        if (m_buffer.empty()) return 0.0f;
        
        float readPos = m_writePos - delaySamples;
        while (readPos < 0) readPos += m_buffer.size();
        
        switch (mode) {
            case PerformanceMode::LOW_CPU:
                return readLinear(readPos);
            case PerformanceMode::BALANCED:
                return readHermite(readPos);
            case PerformanceMode::HIGH_QUALITY:
            default:
                return readCubic(readPos);
        }
    }
    
    void write(float sample) {
        if (!m_buffer.empty()) {
            m_buffer[m_writePos] = sample;
            m_writePos = (m_writePos + 1) % m_buffer.size();
        }
    }
    
    float processTap(float input, float delaySamples, PerformanceMode mode) {
        float output = read(delaySamples, mode);
        write(input);
        return output;
    }

private:
    float readLinear(float pos) const {
        int idx0 = static_cast<int>(pos);
        int idx1 = (idx0 + 1) % m_buffer.size();
        float frac = pos - idx0;
        
        return m_buffer[idx0] * (1.0f - frac) + m_buffer[idx1] * frac;
    }
    
    float readHermite(float pos) const {
        int idx1 = static_cast<int>(pos);
        int idx0 = (idx1 - 1 + m_buffer.size()) % m_buffer.size();
        int idx2 = (idx1 + 1) % m_buffer.size();
        int idx3 = (idx1 + 2) % m_buffer.size();
        
        float y0 = m_buffer[idx0];
        float y1 = m_buffer[idx1];
        float y2 = m_buffer[idx2];
        float y3 = m_buffer[idx3];
        
        float frac = pos - idx1;
        float frac2 = frac * frac;
        float frac3 = frac2 * frac;
        
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        
        return c0 + c1 * frac + c2 * frac2 + c3 * frac3;
    }
    
    float readCubic(float pos) const {
        int idx1 = static_cast<int>(pos);
        int idx0 = (idx1 - 1 + m_buffer.size()) % m_buffer.size();
        int idx2 = (idx1 + 1) % m_buffer.size();
        int idx3 = (idx1 + 2) % m_buffer.size();
        
        float y0 = m_buffer[idx0];
        float y1 = m_buffer[idx1];
        float y2 = m_buffer[idx2];
        float y3 = m_buffer[idx3];
        
        float x = pos - idx1;
        float x2 = x * x;
        float x3 = x2 * x;
        
        float a0 = y3 - y2 - y0 + y1;
        float a1 = y0 - y1 - a0;
        float a2 = y2 - y0;
        float a3 = y1;
        
        return a0 * x3 + a1 * x2 + a2 * x + a3;
    }
    
    std::vector<float> m_buffer;
    int m_writePos = 0;
};

// ModulatedCombFilter implementation
struct PlateReverb::ModulatedCombFilter {
    InterpolatedDelayLine delay;
    OnePoleFilter damping;
    float modPhase = 0.0f;
    float modRate = 0.0f;
    float modDepth = 0.0f;
    int baseDelay = 0;
    
    void init(int delaySamples, float rate, float depth) {
        baseDelay = delaySamples;
        modRate = rate;
        modDepth = depth * 3.0f; // Scale for samples
        delay.init(delaySamples + static_cast<int>(modDepth) + 4);
        damping.reset();
        modPhase = 0.0f;
    }
    
    void reset() {
        delay.reset();
        damping.reset();
        modPhase = 0.0f;
    }
    
    float process(float input, float feedback, float dampingAmount, PerformanceMode mode, double sampleRate) {
        // Update modulation
        modPhase += modRate / sampleRate;
        if (modPhase >= 1.0f) modPhase -= 1.0f;
        
        // Calculate modulated delay
        float modulation = std::sin(2.0f * M_PI * modPhase) * modDepth;
        float currentDelay = baseDelay + modulation;
        
        // Read delayed signal
        float delayed = delay.read(currentDelay, mode);
        
        // Apply damping
        damping.setCoefficient(dampingAmount * 0.8f);
        float filtered = damping.processLowpass(delayed);
        
        // Feedback with soft limiting
        float fedback = std::tanh(filtered * feedback);
        
        // Write to delay line
        delay.write(input + fedback);
        
        return delayed;
    }
};

// AllpassFilter implementation
struct PlateReverb::AllpassFilter {
    InterpolatedDelayLine delay;
    int delaySamples = 0;
    
    void init(int samples) {
        delaySamples = samples;
        delay.init(samples + 4);
    }
    
    void reset() {
        delay.reset();
    }
    
    float process(float input, float feedback, PerformanceMode mode) {
        float delayed = delay.read(static_cast<float>(delaySamples), mode);
        float output = -input + delayed;
        delay.write(input + delayed * feedback);
        return output;
    }
};

// EarlyReflections implementation
struct PlateReverb::EarlyReflections {
    struct Tap {
        int delay;
        float gain;
        float panL;
        float panR;
    };
    
    std::vector<Tap> taps;
    InterpolatedDelayLine delayLine;
    
    void init(double sampleRate) {
        float srRatio = sampleRate / PlateConstants::REFERENCE_SAMPLE_RATE;
        
        // Create early reflection taps based on plate measurements
        taps.clear();
        for (int i = 0; i < 16; ++i) {
            Tap tap;
            tap.delay = static_cast<int>(PlateConstants::EARLY_TAP_DELAYS[i] * srRatio);
            tap.gain = 1.0f / (1.0f + i * 0.1f); // Natural decay
            
            // Stereo positioning
            float angle = (i * 0.618f) * M_PI; // Golden ratio distribution
            tap.panL = std::cos(angle) * 0.5f + 0.5f;
            tap.panR = std::sin(angle) * 0.5f + 0.5f;
            
            taps.push_back(tap);
        }
        
        // Initialize delay line for maximum delay
        int maxDelay = static_cast<int>(PlateConstants::EARLY_TAP_DELAYS[15] * srRatio);
        delayLine.init(maxDelay + 100);
    }
    
    void reset() {
        delayLine.reset();
    }
    
    std::pair<float, float> process(float input, PerformanceMode mode) {
        delayLine.write(input);
        
        float sumL = 0.0f;
        float sumR = 0.0f;
        
        for (const auto& tap : taps) {
            float sample = delayLine.read(static_cast<float>(tap.delay), mode) * tap.gain;
            sumL += sample * tap.panL;
            sumR += sample * tap.panR;
        }
        
        return {sumL * 0.5f, sumR * 0.5f}; // Scale for mixing
    }
};

// FDN (Feedback Delay Network) implementation
struct PlateReverb::FDN {
    static constexpr int MAX_TAPS = 8;
    std::array<ModulatedCombFilter, MAX_TAPS> delays;
    std::array<OnePoleFilter, MAX_TAPS> filters;
    std::array<float, MAX_TAPS> lastOutputs;
    int numTaps = 8;
    
    void init(double sampleRate, int taps) {
        numTaps = std::min(taps, MAX_TAPS);
        float srRatio = sampleRate / PlateConstants::REFERENCE_SAMPLE_RATE;
        
        for (int i = 0; i < numTaps; ++i) {
            int delaySamples = static_cast<int>(PlateConstants::FDN_DELAY_BASE[i] * srRatio);
            float modRate = PlateConstants::MOD_RATES[i];
            float modDepth = 0.2f + i * 0.05f;
            
            delays[i].init(delaySamples, modRate, modDepth);
            filters[i].reset();
            lastOutputs[i] = 0.0f;
        }
    }
    
    void reset() {
        for (int i = 0; i < numTaps; ++i) {
            delays[i].reset();
            filters[i].reset();
            lastOutputs[i] = 0.0f;
        }
    }
    
    float process(float input, float feedback, float damping, PerformanceMode mode, double sampleRate) {
        // Hadamard matrix mixing for FDN
        std::array<float, MAX_TAPS> mixed;
        hadamardMix(lastOutputs, mixed);
        
        // Process each delay line
        float sum = 0.0f;
        for (int i = 0; i < numTaps; ++i) {
            float delayIn = input * 0.25f + mixed[i];
            lastOutputs[i] = delays[i].process(delayIn, feedback, damping, mode, sampleRate);
            
            // Additional filtering for frequency shaping
            filters[i].setCoefficient(0.2f + damping * 0.6f);
            lastOutputs[i] = filters[i].processLowpass(lastOutputs[i]);
            
            sum += lastOutputs[i];
        }
        
        return sum / std::sqrt(static_cast<float>(numTaps));
    }
    
private:
    void hadamardMix(const std::array<float, MAX_TAPS>& in, std::array<float, MAX_TAPS>& out) {
        // Simplified Hadamard matrix multiplication for mixing
        // This creates maximum diffusion in the FDN
        if (numTaps == 8) {
            // 8x8 Hadamard
            out[0] = (in[0] + in[1] + in[2] + in[3] + in[4] + in[5] + in[6] + in[7]) * 0.353553f;
            out[1] = (in[0] - in[1] + in[2] - in[3] + in[4] - in[5] + in[6] - in[7]) * 0.353553f;
            out[2] = (in[0] + in[1] - in[2] - in[3] + in[4] + in[5] - in[6] - in[7]) * 0.353553f;
            out[3] = (in[0] - in[1] - in[2] + in[3] + in[4] - in[5] - in[6] + in[7]) * 0.353553f;
            out[4] = (in[0] + in[1] + in[2] + in[3] - in[4] - in[5] - in[6] - in[7]) * 0.353553f;
            out[5] = (in[0] - in[1] + in[2] - in[3] - in[4] + in[5] - in[6] + in[7]) * 0.353553f;
            out[6] = (in[0] + in[1] - in[2] - in[3] - in[4] - in[5] + in[6] + in[7]) * 0.353553f;
            out[7] = (in[0] - in[1] - in[2] + in[3] - in[4] + in[5] + in[6] - in[7]) * 0.353553f;
        } else if (numTaps == 6) {
            // Simplified 6-tap mixing
            float scale = 0.408248f; // 1/sqrt(6)
            out[0] = (in[0] + in[1] + in[2] + in[3] + in[4] + in[5]) * scale;
            out[1] = (in[0] - in[1] + in[2] - in[3] + in[4] - in[5]) * scale;
            out[2] = (in[0] + in[1] - in[2] - in[3] + in[4] + in[5]) * scale;
            out[3] = (in[0] - in[1] - in[2] + in[3] + in[4] - in[5]) * scale;
            out[4] = (in[0] + in[1] + in[2] - in[3] - in[4] - in[5]) * scale;
            out[5] = (in[0] - in[1] + in[2] + in[3] - in[4] + in[5]) * scale;
        } else {
            // Fallback for other tap counts
            out = in;
        }
    }
};

// Main PlateReverb implementation
PlateReverb::PlateReverb() {
    // Initialize parameter smoothers with appropriate smoothing times
    m_size = std::make_unique<ParameterSmoother>(PlateConstants::SIZE_SMOOTH_TIME);
    m_damping = std::make_unique<ParameterSmoother>(PlateConstants::DAMPING_SMOOTH_TIME);
    m_predelay = std::make_unique<ParameterSmoother>(PlateConstants::PREDELAY_SMOOTH_TIME);
    m_mix = std::make_unique<ParameterSmoother>(PlateConstants::MIX_SMOOTH_TIME);
    
    m_feedbackSmooth = std::make_unique<ParameterSmoother>(PlateConstants::FEEDBACK_SMOOTH_TIME);
    m_fdnDampingSmooth = std::make_unique<ParameterSmoother>(PlateConstants::FDN_DAMPING_SMOOTH_TIME);
    
    // Set initial values
    m_size->reset(0.5f);
    m_damping->reset(0.5f);
    m_predelay->reset(0.0f);
    m_mix->reset(0.3f);
    m_feedbackSmooth->reset(0.9f);
    m_fdnDampingSmooth->reset(0.5f);
}

PlateReverb::~PlateReverb() = default;

void PlateReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_currentBlockSize = PlateConstants::getBlockSize(m_performanceMode.load());
    
    // Update all smoothers
    m_size->setSampleRate(sampleRate);
    m_damping->setSampleRate(sampleRate);
    m_predelay->setSampleRate(sampleRate);
    m_mix->setSampleRate(sampleRate);
    m_feedbackSmooth->setSampleRate(sampleRate);
    m_fdnDampingSmooth->setSampleRate(sampleRate);
    
    initializeFilters();
    m_isInitialized.store(true);
}

void PlateReverb::reset() {
    if (!m_isInitialized.load()) return;
    
    // Reset all DSP components
    for (auto& dcBlocker : m_dcBlockers) {
        if (dcBlocker) dcBlocker->reset();
    }
    
    for (auto& limiter : m_inputLimiters) {
        if (limiter) limiter->reset();
    }
    
    for (auto& predelay : m_predelays) {
        if (predelay) predelay->reset();
    }
    
    if (m_earlyReflections) {
        m_earlyReflections->reset();
    }
    
    for (auto& diffuser : m_inputDiffusion) {
        if (diffuser) diffuser->reset();
    }
    
    if (m_fdnLeft) m_fdnLeft->reset();
    if (m_fdnRight) m_fdnRight->reset();
    
    for (auto& hp : m_outputHighpass) {
        if (hp) hp->reset();
    }
    
    for (auto& limiter : m_outputLimiters) {
        if (limiter) limiter->reset();
    }
}

void PlateReverb::initializeFilters() {
    // Initialize DC blockers
    for (int i = 0; i < 2; ++i) {
        m_dcBlockers[i] = std::make_unique<ButterworthHighpass>();
        m_dcBlockers[i]->setFrequency(PlateConstants::DC_BLOCK_FREQ, m_sampleRate);
    }
    
    // Initialize limiters
    for (int i = 0; i < 2; ++i) {
        m_inputLimiters[i] = std::make_unique<SoftKneeLimiter>();
        m_inputLimiters[i]->setSampleRate(m_sampleRate);
        
        m_outputLimiters[i] = std::make_unique<SoftKneeLimiter>();
        m_outputLimiters[i]->setSampleRate(m_sampleRate);
    }
    
    // Initialize pre-delays (max 100ms)
    int maxPredelaySamples = static_cast<int>(0.1 * m_sampleRate);
    for (int i = 0; i < 2; ++i) {
        m_predelays[i] = std::make_unique<InterpolatedDelayLine>();
        m_predelays[i]->init(maxPredelaySamples);
    }
    
    // Initialize early reflections
    m_earlyReflections = std::make_unique<EarlyReflections>();
    m_earlyReflections->init(m_sampleRate);
    
    // Initialize input diffusion
    float srRatio = m_sampleRate / PlateConstants::REFERENCE_SAMPLE_RATE;
    for (int i = 0; i < NUM_DIFFUSERS; ++i) {
        m_inputDiffusion[i] = std::make_unique<AllpassFilter>();
        m_inputDiffusion[i]->init(static_cast<int>(PlateConstants::DIFFUSION_DELAYS[i] * srRatio));
    }
    
    // Initialize FDNs
    int fdnSize = getFDNSize();
    m_fdnLeft = std::make_unique<FDN>();
    m_fdnLeft->init(m_sampleRate, fdnSize);
    
    m_fdnRight = std::make_unique<FDN>();
    m_fdnRight->init(m_sampleRate, fdnSize);
    
    // Initialize output highpass filters
    for (int i = 0; i < 2; ++i) {
        m_outputHighpass[i] = std::make_unique<ButterworthHighpass>();
        m_outputHighpass[i]->setFrequency(PlateConstants::OUTPUT_HPF_FREQ, m_sampleRate);
    }
}

void PlateReverb::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    if (!m_isInitialized.load()) return;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Process in blocks for better cache performance
    const int blockSize = m_currentBlockSize;
    const PerformanceMode perfMode = m_performanceMode.load();
    
    for (int offset = 0; offset < numSamples; offset += blockSize) {
        int samplesToProcess = std::min(blockSize, numSamples - offset);
        
        if (numChannels >= 2) {
            // True stereo processing
            float* leftData = buffer.getWritePointer(0) + offset;
            float* rightData = buffer.getWritePointer(1) + offset;
            
            for (int i = 0; i < samplesToProcess; ++i) {
                // Update smoothed parameters
                float size = m_size->process();
                float damping = m_damping->process();
                float predelay = m_predelay->process();
                float mix = m_mix->process();
                
                // Calculate derived parameters
                float feedback = PlateConstants::MIN_FEEDBACK + 
                                size * (PlateConstants::MAX_FEEDBACK - PlateConstants::MIN_FEEDBACK);
                m_feedbackSmooth->setTarget(feedback);
                float smoothedFeedback = m_feedbackSmooth->process();
                
                float fdnDamping = damping * PlateConstants::DAMPING_SCALE;
                m_fdnDampingSmooth->setTarget(fdnDamping);
                float smoothedDamping = m_fdnDampingSmooth->process();
                
                // Get input samples
                float inputL = leftData[i];
                float inputR = rightData[i];
                
                // DC blocking
                inputL = m_dcBlockers[0]->process(inputL);
                inputR = m_dcBlockers[1]->process(inputR);
                
                // Input limiting
                inputL = m_inputLimiters[0]->process(inputL);
                inputR = m_inputLimiters[1]->process(inputR);
                
                // Pre-delay
                float predelaySamples = predelay * 0.1f * m_sampleRate; // Max 100ms
                float delayedL = m_predelays[0]->processTap(inputL, predelaySamples, perfMode);
                float delayedR = m_predelays[1]->processTap(inputR, predelaySamples + 3.7f, perfMode);
                
                // Process reverb
                auto [reverbL, reverbR] = processReverbSample(delayedL, delayedR);
                
                // Apply mix
                leftData[i] = inputL * (1.0f - mix) + reverbL * mix;
                rightData[i] = inputR * (1.0f - mix) + reverbR * mix;
            }
        } else if (numChannels == 1) {
            // Mono processing
            float* data = buffer.getWritePointer(0) + offset;
            
            for (int i = 0; i < samplesToProcess; ++i) {
                float size = m_size->process();
                float damping = m_damping->process();
                float predelay = m_predelay->process();
                float mix = m_mix->process();
                
                float feedback = PlateConstants::MIN_FEEDBACK + 
                                size * (PlateConstants::MAX_FEEDBACK - PlateConstants::MIN_FEEDBACK);
                m_feedbackSmooth->setTarget(feedback);
                float smoothedFeedback = m_feedbackSmooth->process();
                
                float fdnDamping = damping * PlateConstants::DAMPING_SCALE;
                m_fdnDampingSmooth->setTarget(fdnDamping);
                float smoothedDamping = m_fdnDampingSmooth->process();
                
                float input = data[i];
                input = m_dcBlockers[0]->process(input);
                input = m_inputLimiters[0]->process(input);
                
                float predelaySamples = predelay * 0.1f * m_sampleRate;
                float delayed = m_predelays[0]->processTap(input, predelaySamples, perfMode);
                
                auto [reverbL, reverbR] = processReverbSample(delayed, delayed);
                float reverb = (reverbL + reverbR) * 0.5f;
                
                data[i] = input * (1.0f - mix) + reverb * mix;
            }
        }
    }
    
    scrubBuffer(buffer);
}

std::pair<double, double> PlateReverb::processReverbSample(double inputL, double inputR) {
    const PerformanceMode perfMode = m_performanceMode.load();
    
    // Mix to mono for reverb input
    float monoInput = (inputL + inputR) * 0.5f;
    
    // Early reflections
    auto [earlyL, earlyR] = m_earlyReflections->process(monoInput, perfMode);
    
    // Input diffusion
    float diffused = monoInput;
    for (auto& diffuser : m_inputDiffusion) {
        diffused = diffuser->process(diffused, 0.7f, perfMode);
    }
    
    // Get current parameters
    float feedback = m_feedbackSmooth->getCurrentValue();
    float damping = m_fdnDampingSmooth->getCurrentValue();
    
    // Process through FDNs with slight decorrelation
    float fdnL = m_fdnLeft->process(diffused, feedback, damping, perfMode, m_sampleRate);
    float fdnR = m_fdnRight->process(diffused * 0.95f, feedback, damping, perfMode, m_sampleRate);
    
    // Combine early and late reflections
    float lateL = fdnL * PlateConstants::LATE_MIX;
    float lateR = fdnR * PlateConstants::LATE_MIX;
    
    float outL = earlyL * PlateConstants::EARLY_MIX + lateL;
    float outR = earlyR * PlateConstants::EARLY_MIX + lateR;
    
    // Apply stereo width enhancement
    float mid = (outL + outR) * 0.5f;
    float side = (outL - outR) * 0.5f * PlateConstants::STEREO_SPREAD;
    
    outL = mid + side;
    outR = mid - side;
    
    // Output highpass filtering
    outL = m_outputHighpass[0]->process(outL);
    outR = m_outputHighpass[1]->process(outR);
    
    // Output limiting
    outL = m_outputLimiters[0]->process(outL);
    outR = m_outputLimiters[1]->process(outR);
    
    return {outL, outR};
}

void PlateReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        float clampedValue = clampParameter(value);
        
        switch (index) {
            case 0:
                m_size->setTarget(clampedValue);
                break;
            case 1:
                m_damping->setTarget(clampedValue);
                break;
            case 2:
                m_predelay->setTarget(clampedValue);
                break;
            case 3:
                m_mix->setTarget(clampedValue);
                break;
        }
    }
}

void PlateReverb::setPerformanceMode(PerformanceMode mode) {
    m_performanceMode.store(mode);
    m_currentBlockSize = PlateConstants::getBlockSize(mode);
    
    // Re-initialize FDNs if size changed
    if (m_isInitialized.load()) {
        int newFdnSize = getFDNSize();
        if (m_fdnLeft) m_fdnLeft->init(m_sampleRate, newFdnSize);
        if (m_fdnRight) m_fdnRight->init(m_sampleRate, newFdnSize);
    }
}

juce::String PlateReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Size";
        case 1: return "Damping";
        case 2: return "Predelay";
        case 3: return "Mix";
        default: return "";
    }
}