// DigitalDelay.cpp
#include "DigitalDelay.h"
#include "DspEngineUtilities.h"
#include <cstring>
#include <algorithm>

namespace AudioDSP {

// ==================== DigitalDelay Implementation ====================

DigitalDelay::DigitalDelay() {
    // Initialize DSP components
    for (int i = 0; i < 2; ++i) {
        m_delayLines[i] = std::make_unique<DigitalDelayImpl::DelayLine>();
        m_filters[i] = std::make_unique<DigitalDelayImpl::BiquadFilter>();
        m_dcBlockers[i] = std::make_unique<DigitalDelayImpl::DCBlocker>();
    }
    
    m_clipper = std::make_unique<DigitalDelayImpl::SoftClipper>();
    m_modulator = std::make_unique<DigitalDelayImpl::ModulationProcessor>();
    
    // Initialize parameter smoothers
    m_delayTime = std::make_unique<DigitalDelayImpl::ParameterSmoother>();
    m_feedback = std::make_unique<DigitalDelayImpl::ParameterSmoother>();
    m_mix = std::make_unique<DigitalDelayImpl::ParameterSmoother>();
    m_highCut = std::make_unique<DigitalDelayImpl::ParameterSmoother>();
    m_sync = std::make_unique<DigitalDelayImpl::ParameterSmoother>();
    
    // Set default values
    m_delayTime->reset(0.4f);
    m_feedback->reset(0.3f);
    m_mix->reset(0.3f);
    m_highCut->reset(0.8f);
    m_sync->reset(0.0f); // sync off by default
}

DigitalDelay::~DigitalDelay() = default;

void DigitalDelay::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_sampleRateChanged = true;
    
    // Configure smoothers
    m_delayTime->setSampleRate(sampleRate);
    m_delayTime->setSmoothingTime(50.0f);
    
    m_feedback->setSampleRate(sampleRate);
    m_feedback->setSmoothingTime(30.0f);
    
    m_mix->setSampleRate(sampleRate);
    m_mix->setSmoothingTime(30.0f);
    
    m_highCut->setSampleRate(sampleRate);
    m_highCut->setSmoothingTime(20.0f);
    
    m_sync->setSampleRate(sampleRate);
    m_sync->setSmoothingTime(10.0f); // Fast switching for sync
    
    // Configure DSP components
    m_modulator->setSampleRate(sampleRate);
    m_clipper->reset();
    
    for (int i = 0; i < 2; ++i) {
        m_filters[i]->reset();
        m_dcBlockers[i]->reset();
        m_delayLines[i]->reset();
    }
    
    reset();
}

void DigitalDelay::reset() {
    for (int i = 0; i < 2; ++i) {
        if (m_delayLines[i]) m_delayLines[i]->reset();
        if (m_filters[i]) m_filters[i]->reset();
        if (m_dcBlockers[i]) m_dcBlockers[i]->reset();
    }
    
    if (m_clipper) m_clipper->reset();
    if (m_modulator) m_modulator->reset();
    
    m_crossfeed.leftToRight = 0.0f;
    m_crossfeed.rightToLeft = 0.0f;
}

void DigitalDelay::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Process based on channel count
    if (numChannels == 1) {
        processMono(buffer.getWritePointer(0), numSamples);
    } else {
        float* left = buffer.getWritePointer(0);
        float* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        if (right) {
            processStereo(left, right, numSamples);
        } else {
            processMono(left, numSamples);
        }
    }
}

void DigitalDelay::processStereo(float* left, float* right, int numSamples) {
    // Process in optimized blocks
    int samplesRemaining = numSamples;
    int samplePos = 0;
    
    while (samplesRemaining > 0) {
        int blockSize = std::min(samplesRemaining, PROCESS_BLOCK_SIZE);
        
        for (int i = 0; i < blockSize; ++i) {
            left[samplePos + i] = processSample(left[samplePos + i], 0);
            right[samplePos + i] = processSample(right[samplePos + i], 1);
            
            // Apply ping-pong crossfeed
            float crossL = m_crossfeed.leftToRight;
            float crossR = m_crossfeed.rightToLeft;
            
            m_crossfeed.leftToRight = right[samplePos + i] * m_crossfeed.amount;
            m_crossfeed.rightToLeft = left[samplePos + i] * m_crossfeed.amount;
            
            left[samplePos + i] += crossR * 0.3f;
            right[samplePos + i] += crossL * 0.3f;
        }
        
        samplesRemaining -= blockSize;
        samplePos += blockSize;
    }
}

void DigitalDelay::processMono(float* data, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        data[i] = processSample(data[i], 0);
    }
}

float DigitalDelay::processSample(float input, int channel) {
    // Sanitize input
    input = sanitizeInput(input);
    
    // Get smoothed parameters
    float delayTime = m_delayTime->getNextValue();
    float feedback = m_feedback->getNextValue() * MAX_FEEDBACK;
    float mix = m_mix->getNextValue();
    float highCut = m_highCut->getNextValue();
    float syncParam = m_sync->getNextValue();
    
    // Calculate delay in samples - use sync if enabled
    double delaySamples = calculateSyncedDelayTime(delayTime, syncParam);
    
    // Add subtle modulation for organic feel
    float modulation = m_modulator->process(0.3f, 0.002f);
    double modulatedDelay = delaySamples * (1.0 + modulation);
    
    // Read from delay line with modulation
    float delayed = m_delayLines[channel]->readModulated(modulatedDelay, modulation);
    
    // Apply highcut filter
    m_filters[channel]->setLowpass(1000.0 + highCut * 19000.0, m_sampleRate);
    delayed = m_filters[channel]->processSample(delayed);
    
    // Apply feedback with soft clipping
    float feedbackSignal = m_clipper->processSample(delayed * feedback);
    
    // DC blocking on feedback path
    feedbackSignal = m_dcBlockers[channel]->processSample(feedbackSignal);
    
    // Write to delay line
    m_delayLines[channel]->write(input + feedbackSignal);
    
    // Mix dry and wet signals
    float output = input * (1.0f - mix) + delayed * mix;
    
    // Final limiting
    return std::max(-MAX_OUTPUT, std::min(MAX_OUTPUT, output));
}

void DigitalDelay::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    m_delayTime->setTargetValue(getParam(0, 0.4f));
    m_feedback->setTargetValue(getParam(1, 0.3f));
    m_mix->setTargetValue(getParam(2, 0.3f));
    m_highCut->setTargetValue(getParam(3, 0.8f));
    m_sync->setTargetValue(getParam(4, 0.0f));
}

juce::String DigitalDelay::getParameterName(int index) const {
    switch (index) {
        case 0: return "Time";
        case 1: return "Feedback";
        case 2: return "Mix";
        case 3: return "High Cut";
        case 4: return "Sync";
        default: return "";
    }
}

// ==================== DelayLine Implementation ====================

DigitalDelayImpl::DelayLine::DelayLine() {
    reset();
}

void DigitalDelayImpl::DelayLine::reset() noexcept {
    std::memset(m_buffer.data(), 0, m_buffer.size() * sizeof(float));
    m_writePos = 0;
}

void DigitalDelayImpl::DelayLine::write(float sample) noexcept {
    m_buffer[m_writePos] = sample + DENORMAL_PREVENTION;
    
    // Update ghost samples for wrap-around interpolation
    if (m_writePos < 4) {
        m_buffer[BUFFER_SIZE + m_writePos] = m_buffer[m_writePos];
    }
    
    m_writePos = (m_writePos + 1) & BUFFER_MASK;
}

float DigitalDelayImpl::DelayLine::read(double delaySamples) noexcept {
    double readPos = static_cast<double>(m_writePos) - delaySamples;
    
    // Wrap negative positions
    while (readPos < 0) {
        readPos += BUFFER_SIZE;
    }
    
    return hermiteInterpolate(readPos);
}

float DigitalDelayImpl::DelayLine::readModulated(double delaySamples, float modulation) noexcept {
    // Apply smooth modulation to delay time
    double modulatedDelay = delaySamples * (1.0 + modulation * 0.01);
    
    // Ensure we don't exceed buffer bounds
    modulatedDelay = std::max(1.0, std::min(static_cast<double>(BUFFER_SIZE - 1), modulatedDelay));
    
    return read(modulatedDelay);
}

float DigitalDelayImpl::DelayLine::hermiteInterpolate(double position) const noexcept {
    // Get integer and fractional parts
    int intPos = static_cast<int>(position);
    float frac = static_cast<float>(position - intPos);
    
    // Get the four sample points for interpolation
    int idx0 = (intPos - 1) & BUFFER_MASK;
    int idx1 = intPos & BUFFER_MASK;
    int idx2 = (intPos + 1) & BUFFER_MASK;
    int idx3 = (intPos + 2) & BUFFER_MASK;
    
    float y0 = m_buffer[idx0];
    float y1 = m_buffer[idx1];
    float y2 = m_buffer[idx2];
    float y3 = m_buffer[idx3];
    
    // Hermite interpolation coefficients
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    
    // Evaluate polynomial
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

// ==================== BiquadFilter Implementation ====================

void DigitalDelayImpl::BiquadFilter::reset() noexcept {
    m_x1 = m_x2 = 0.0;
    m_y1 = m_y2 = 0.0;
    
    #ifdef __SSE2__
    // Reset SIMD state
    for (int i = 0; i < 4; ++i) {
        m_state[i] = _mm_setzero_pd();
    }
    #endif
}

void DigitalDelayImpl::BiquadFilter::setLowpass(double frequency, double sampleRate, double q) noexcept {
    double omega = 2.0 * M_PI * frequency / sampleRate;
    double sinOmega = std::sin(omega);
    double cosOmega = std::cos(omega);
    double alpha = sinOmega / (2.0 * q);
    
    double b0 = (1.0 - cosOmega) / 2.0;
    double b1 = 1.0 - cosOmega;
    double b2 = (1.0 - cosOmega) / 2.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosOmega;
    double a2 = 1.0 - alpha;
    
    // Normalize coefficients
    m_a0 = b0 / a0;
    m_a1 = b1 / a0;
    m_a2 = b2 / a0;
    m_b1 = a1 / a0;
    m_b2 = a2 / a0;
    
    #ifdef __SSE2__
    // Prepare SIMD coefficients
    m_coeffs[0] = _mm_set1_pd(m_a0);
    m_coeffs[1] = _mm_set1_pd(m_a1);
    m_coeffs[2] = _mm_set1_pd(m_a2);
    m_coeffs[3] = _mm_set1_pd(m_b1);
    m_coeffs[4] = _mm_set1_pd(m_b2);
    #endif
}

float DigitalDelayImpl::BiquadFilter::processSample(float input) noexcept {
    double in = static_cast<double>(input);
    double out = m_a0 * in + m_a1 * m_x1 + m_a2 * m_x2 - m_b1 * m_y1 - m_b2 * m_y2;
    
    m_x2 = m_x1;
    m_x1 = in;
    m_y2 = m_y1;
    m_y1 = out;
    
    // Denormal prevention
    m_y1 += 1e-30;
    m_y1 -= 1e-30;
    
    return static_cast<float>(out);
}

void DigitalDelayImpl::BiquadFilter::processBlock(const float* input, float* output, int numSamples) noexcept {
    #ifdef __SSE2__
    // Process in groups of 4 for SIMD optimization
    int simdSamples = numSamples & ~3;
    
    if (simdSamples > 0) {
        processBlockSIMD(input, output, simdSamples);
    }
    
    // Process remaining samples
    for (int i = simdSamples; i < numSamples; ++i) {
        output[i] = processSample(input[i]);
    }
    #else
    // Non-SIMD fallback
    for (int i = 0; i < numSamples; ++i) {
        output[i] = processSample(input[i]);
    }
    #endif
}

#ifdef __SSE2__
void DigitalDelayImpl::BiquadFilter::processBlockSIMD(const float* input, float* output, int numSamples) noexcept {
    // Process 4 samples at a time using SSE2
    for (int i = 0; i < numSamples; i += 4) {
        __m128 in = _mm_loadu_ps(&input[i]);
        __m128d in_lo = _mm_cvtps_pd(in);
        __m128d in_hi = _mm_cvtps_pd(_mm_movehl_ps(in, in));
        
        // Process low pair
        __m128d out_lo = _mm_mul_pd(m_coeffs[0], in_lo);
        out_lo = _mm_add_pd(out_lo, _mm_mul_pd(m_coeffs[1], m_state[0]));
        out_lo = _mm_add_pd(out_lo, _mm_mul_pd(m_coeffs[2], m_state[1]));
        out_lo = _mm_sub_pd(out_lo, _mm_mul_pd(m_coeffs[3], m_state[2]));
        out_lo = _mm_sub_pd(out_lo, _mm_mul_pd(m_coeffs[4], m_state[3]));
        
        // Update state
        m_state[1] = m_state[0];
        m_state[0] = in_lo;
        m_state[3] = m_state[2];
        m_state[2] = out_lo;
        
        // Process high pair (similar logic)
        __m128d out_hi = _mm_mul_pd(m_coeffs[0], in_hi);
        out_hi = _mm_add_pd(out_hi, _mm_mul_pd(m_coeffs[1], m_state[0]));
        out_hi = _mm_add_pd(out_hi, _mm_mul_pd(m_coeffs[2], m_state[1]));
        out_hi = _mm_sub_pd(out_hi, _mm_mul_pd(m_coeffs[3], m_state[2]));
        out_hi = _mm_sub_pd(out_hi, _mm_mul_pd(m_coeffs[4], m_state[3]));
        
        // Convert back to float and store
        __m128 out = _mm_movelh_ps(_mm_cvtpd_ps(out_lo), _mm_cvtpd_ps(out_hi));
        _mm_storeu_ps(&output[i], out);
    }
}
#else
void DigitalDelayImpl::BiquadFilter::processBlockSIMD(const float* input, float* output, int numSamples) noexcept {
    // Non-SIMD fallback
    processBlock(input, output, numSamples);
}
#endif

// ==================== SoftClipper Implementation ====================

DigitalDelayImpl::SoftClipper::SoftClipper() {
    m_oversampler = std::make_unique<Oversampler>();
    reset();
}

void DigitalDelayImpl::SoftClipper::reset() noexcept {
    if (m_oversampler) {
        m_oversampler->reset();
    }
}

float DigitalDelayImpl::SoftClipper::processSample(float input) noexcept {
    // 4x oversampling for anti-aliased clipping
    float upsampled[4];
    m_oversampler->upsample(input, upsampled);
    
    // Apply soft clipping to each oversampled sample
    for (int i = 0; i < 4; ++i) {
        upsampled[i] = softClip(upsampled[i]);
    }
    
    // Downsample back to original rate
    return m_oversampler->downsample(upsampled);
}

void DigitalDelayImpl::SoftClipper::processBlock(float* data, int numSamples) noexcept {
    for (int i = 0; i < numSamples; ++i) {
        data[i] = processSample(data[i]);
    }
}

float DigitalDelayImpl::SoftClipper::softClip(float x) const noexcept {
    const float threshold = 0.7f;
    float absX = std::abs(x);
    
    if (absX < threshold) {
        return x;
    }
    
    float sign = x > 0 ? 1.0f : -1.0f;
    
    // Smooth knee compression
    if (absX < 0.95f) {
        float knee = (absX - threshold) / (0.95f - threshold);
        float gain = 1.0f - knee * knee * 0.3f;
        return sign * (threshold + (absX - threshold) * gain);
    }
    
    // Asymptotic limiting with tanh
    float excess = absX - 0.95f;
    return sign * (0.95f + std::tanh(excess * 3.0f) * 0.05f);
}

// ==================== Oversampler Implementation ====================

DigitalDelayImpl::SoftClipper::Oversampler::Oversampler() {
    reset();
    
    // Design polyphase FIR filter coefficients (Kaiser window)
    const float beta = 8.0f;
    const float cutoff = 0.45f;
    
    for (int i = 0; i < FILTER_SIZE; ++i) {
        float x = static_cast<float>(i - FILTER_SIZE / 2);
        
        if (std::abs(x) < 1e-6f) {
            m_coeffs[i] = 2.0f * cutoff;
        } else {
            m_coeffs[i] = std::sin(2.0f * M_PI * cutoff * x) / (M_PI * x);
            
            // Kaiser window
            float window = 0.0f;
            float alpha = beta * std::sqrt(1.0f - (2.0f * x / FILTER_SIZE) * (2.0f * x / FILTER_SIZE));
            
            // Modified Bessel function approximation
            float sum = 1.0f;
            float term = 1.0f;
            for (int k = 1; k < 20; ++k) {
                term *= (alpha / k) * (alpha / k) / 4.0f;
                sum += term;
            }
            window = sum;
            
            m_coeffs[i] *= window;
        }
    }
    
    // Normalize
    float sum = 0.0f;
    for (float coeff : m_coeffs) {
        sum += coeff;
    }
    
    for (float& coeff : m_coeffs) {
        coeff /= sum;
    }
}

void DigitalDelayImpl::SoftClipper::Oversampler::reset() noexcept {
    std::memset(m_state, 0, sizeof(m_state));
    m_stateIndex = 0;
}

void DigitalDelayImpl::SoftClipper::Oversampler::upsample(float input, float* output) noexcept {
    // Insert input into delay line
    m_state[m_stateIndex] = input;
    
    // Generate 4 upsampled outputs
    for (int phase = 0; phase < OVERSAMPLE_FACTOR; ++phase) {
        float sum = 0.0f;
        
        // Polyphase filtering
        for (int tap = phase; tap < FILTER_SIZE; tap += OVERSAMPLE_FACTOR) {
            int idx = (m_stateIndex - tap / OVERSAMPLE_FACTOR + FILTER_SIZE) % FILTER_SIZE;
            sum += m_state[idx] * m_coeffs[tap];
        }
        
        output[phase] = sum * OVERSAMPLE_FACTOR;
    }
    
    m_stateIndex = (m_stateIndex + 1) % FILTER_SIZE;
}

float DigitalDelayImpl::SoftClipper::Oversampler::downsample(const float* input) noexcept {
    // Simple averaging for downsampling (after upsampled processing)
    float sum = 0.0f;
    for (int i = 0; i < OVERSAMPLE_FACTOR; ++i) {
        sum += input[i];
    }
    return sum / OVERSAMPLE_FACTOR;
}

// ==================== ModulationProcessor Implementation ====================

DigitalDelayImpl::ModulationProcessor::ModulationProcessor() {
    m_smoothingFilter = std::make_unique<DigitalDelayImpl::BiquadFilter>();
    reset();
}

void DigitalDelayImpl::ModulationProcessor::setSampleRate(double sampleRate) noexcept {
    m_sampleRate = sampleRate;
    
    // Configure smoothing filter
    m_smoothingFilter->setLowpass(2.0, sampleRate, 0.7);
}

void DigitalDelayImpl::ModulationProcessor::reset() noexcept {
    m_phase = 0.0f;
    if (m_smoothingFilter) {
        m_smoothingFilter->reset();
    }
}

float DigitalDelayImpl::ModulationProcessor::process(float rate, float depth) noexcept {
    // Generate LFO
    m_phase += rate / static_cast<float>(m_sampleRate);
    if (m_phase >= 1.0f) {
        m_phase -= 1.0f;
    }
    
    // Sine wave LFO
    float lfo = std::sin(2.0f * M_PI * m_phase);
    
    // Smooth the modulation for more organic feel
    lfo = m_smoothingFilter->processSample(lfo);
    
    return lfo * depth;
}

// ==================== Transport Sync Implementation ====================

void DigitalDelay::setTransportInfo(const TransportInfo& info) {
    m_transportInfo = info;
}

bool DigitalDelay::supportsFeature(Feature f) const noexcept {
    switch (f) {
        case Feature::TempoSync: return true;
        default: return false;
    }
}

float DigitalDelay::calculateSyncedDelayTime(float timeParam, float syncParam) const {
    // Sync is off if syncParam < 0.5, use manual time in samples
    if (syncParam < 0.5f) {
        double delayMs = 1.0 + timeParam * 1999.0; // 1ms to 2000ms range
        return static_cast<float>((delayMs * m_sampleRate) / 1000.0);
    }
    
    // Sync is on, map timeParam to beat divisions
    const int divisionIndex = static_cast<int>(timeParam * 8.999f); // 0-8 range
    const BeatDivision division = static_cast<BeatDivision>(divisionIndex);
    
    return getBeatDivisionSamples(division);
}

float DigitalDelay::getBeatDivisionSamples(BeatDivision division) const {
    const double bpm = std::max(20.0, std::min(999.0, m_transportInfo.bpm));
    const double quarterNoteSamples = (60.0 / bpm) * m_sampleRate; // samples per quarter note
    
    switch (division) {
        case BeatDivision::DIV_1_64: return static_cast<float>(quarterNoteSamples / 16.0);
        case BeatDivision::DIV_1_32: return static_cast<float>(quarterNoteSamples / 8.0);
        case BeatDivision::DIV_1_16: return static_cast<float>(quarterNoteSamples / 4.0);
        case BeatDivision::DIV_1_8:  return static_cast<float>(quarterNoteSamples / 2.0);
        case BeatDivision::DIV_1_4:  return static_cast<float>(quarterNoteSamples);
        case BeatDivision::DIV_1_2:  return static_cast<float>(quarterNoteSamples * 2.0);
        case BeatDivision::DIV_1_1:  return static_cast<float>(quarterNoteSamples * 4.0);
        case BeatDivision::DIV_2_1:  return static_cast<float>(quarterNoteSamples * 8.0);
        case BeatDivision::DIV_4_1:  return static_cast<float>(quarterNoteSamples * 16.0);
        default: return static_cast<float>(quarterNoteSamples);
    }
}

} // namespace AudioDSP