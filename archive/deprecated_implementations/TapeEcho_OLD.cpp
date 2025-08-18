#include "TapeEcho.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// Constructor
TapeEcho::TapeEcho() {
    // Initialize with musical defaults
    m_time.reset(0.375f);      // 375ms - dotted eighth at 120 BPM
    m_feedback.reset(0.35f);   // Moderate feedback
    m_wowFlutter.reset(0.25f); // Vintage character
    m_saturation.reset(0.3f);  // Warm tape sound
    m_mix.reset(0.35f);        // Balanced mix
}

//==============================================================================
// Prepare for playback
void TapeEcho::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_maxBlockSize = samplesPerBlock;
    
    // Configure parameter smoothing with different times
    m_time.setSmoothingTime(30.0f, sampleRate);        // Slower for time changes
    m_feedback.setSmoothingTime(20.0f, sampleRate);    // Medium speed
    m_wowFlutter.setSmoothingTime(50.0f, sampleRate);  // Very slow for modulation
    m_saturation.setSmoothingTime(25.0f, sampleRate);  // Medium-slow
    m_mix.setSmoothingTime(15.0f, sampleRate);         // Faster for mix
    
    // Prepare channel states
    for (auto& state : m_channelStates) {
        state.prepare(sampleRate);
    }
    
    // Reset filter update tracking
    m_lastSaturation = -1.0f;
}

//==============================================================================
// Reset all state
void TapeEcho::reset() {
    for (auto& state : m_channelStates) {
        state.reset();
    }
    m_lastSaturation = -1.0f;
}

//==============================================================================
// Main processing
void TapeEcho::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Safety check
    if (numSamples > m_maxBlockSize) {
        jassertfalse;
        DBG("TapeEcho: Block size " << numSamples << " exceeds maximum " << m_maxBlockSize);
        return;
    }
    
    // Update smoothed parameters once per block
    m_time.update();
    m_feedback.update();
    m_wowFlutter.update();
    m_saturation.update();
    m_mix.update();
    
    // Update filters if saturation changed significantly
    if (std::abs(m_saturation.current - m_lastSaturation) > 0.001f) {
        for (auto& state : m_channelStates) {
            state.filter.updateCoefficients(m_saturation.current, m_sampleRate);
        }
        m_lastSaturation = m_saturation.current;
    }
    
    // Update random targets once per block
    for (auto& state : m_channelStates) {
        state.modulation.updateRandomTarget();
    }
    
    // Process each channel
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        // Process samples with safety checks
        for (int sample = 0; sample < numSamples; ++sample) {
            float result = processSample(channelData[sample], channel);
            
            // Final safety net
            if (!std::isfinite(result)) {
                result = 0.0f;
                // Reset channel state to prevent continued corruption
                m_channelStates[channel].reset();
            }
            
            channelData[sample] = result;
        }
    }
}

//==============================================================================
// Per-sample processing
float TapeEcho::processSample(float input, int channel) {
    auto& state = m_channelStates[channel];
    
    // Calculate modulated delay time
    float baseDelayMs = MIN_DELAY_MS + m_time.current * (MAX_DELAY_MS - MIN_DELAY_MS);
    float modulation = state.modulation.process(m_wowFlutter.current);
    float modulatedDelayMs = baseDelayMs * (1.0f + modulation);
    float delaySamples = modulatedDelayMs * m_sampleRate * 0.001f;
    
    // Clamp using public accessor
    float maxDelaySamples = static_cast<float>(state.delayLine.getMaxDelaySamples());
    delaySamples = std::max(1.0f, std::min(delaySamples, maxDelaySamples));
    
    // Read delayed signal with interpolation
    float delayed = state.delayLine.readInterpolated(delaySamples);
    
    // Apply tape character to delayed signal
    delayed = applyTapeCharacter(delayed, state);
    
    // Process feedback path
    float feedbackSignal = processFeedback(delayed, state);
    
    // Soft saturation on feedback to control self-oscillation
    if (m_feedback.current > 0.7f) {
        float threshold = 0.7f;
        if (std::abs(feedbackSignal) > threshold) {
            float excess = std::abs(feedbackSignal) - threshold;
            float limited = threshold + std::tanh(excess * 3.0f) * 0.3f;
            feedbackSignal = limited * (feedbackSignal < 0 ? -1.0f : 1.0f);
        }
    }
    
    // Process input through record path
    float recordSignal = state.filter.processRecord(input);
    
    // Apply subtle compression to input
    recordSignal = state.compressor.process(recordSignal, m_saturation.current * 0.3f);
    
    // Write to delay line (with safety check)
    float writeSignal = recordSignal + feedbackSignal;
    if (!std::isfinite(writeSignal)) {
        writeSignal = 0.0f;
    }
    state.delayLine.write(writeSignal);
    
    // Mix dry and wet signals
    float output = input * (1.0f - m_mix.current) + delayed * m_mix.current;
    
    // Soft output limiting
    if (std::abs(output) > 0.95f) {
        output = 0.95f * std::tanh(output / 0.95f);
    }
    
    // Final NaN/Inf check before returning
    if (!std::isfinite(output)) {
        output = 0.0f;
    }
    
    return output;
}

//==============================================================================
// Apply tape characteristics to delayed signal
float TapeEcho::applyTapeCharacter(float signal, ChannelState& state) {
    // Update tape age based on saturation
    state.tapeAge = m_saturation.current;
    
    // Playback filtering
    signal = state.filter.processPlayback(signal, state.tapeAge);
    
    // Tape compression
    signal = state.compressor.process(signal, m_saturation.current * 0.5f);
    
    // Tape saturation with bias
    signal = state.saturation.process(signal, m_saturation.current, ChannelState::TAPE_BIAS);
    
    return signal;
}

//==============================================================================
// Process feedback path with filtering
float TapeEcho::processFeedback(float signal, ChannelState& state) {
    // Check for NaN/Inf in input
    if (!std::isfinite(signal)) signal = 0.0f;
    
    float feedback = signal * m_feedback.current;
    
    // ALWAYS limit feedback, regardless of feedback amount
    const float MAX_FEEDBACK = 10.0f; // Absolute maximum
    feedback = std::max(-MAX_FEEDBACK, std::min(feedback, MAX_FEEDBACK));
    
    // Check for NaN/Inf
    if (!std::isfinite(feedback)) feedback = 0.0f;
    
    // Use pre-computed filter coefficients
    float hpOut = feedback - state.feedback.highpassState;
    state.feedback.highpassState += state.feedback.hpAlpha * hpOut;
    feedback = hpOut;
    
    // Lowpass with dynamic cutoff based on feedback amount
    // Update alpha only when feedback changes significantly
    if (std::abs(m_feedback.current - state.feedback.lastFeedback) > 0.01f) {
        const float lpFreq = 6000.0f * (1.0f - m_feedback.current * 0.3f);
        state.feedback.lpAlpha = 1.0f - std::exp(-2.0f * M_PI * lpFreq / m_sampleRate);
        state.feedback.lastFeedback = m_feedback.current;
    }
    
    state.feedback.lowpassState += state.feedback.lpAlpha * (feedback - state.feedback.lowpassState);
    feedback = state.feedback.lowpassState;
    
    // Final safety check
    if (!std::isfinite(feedback)) feedback = 0.0f;
    
    return feedback;
}

//==============================================================================
// Parameter updates
void TapeEcho::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_time.target = params.at(0);
    if (params.count(1)) m_feedback.target = params.at(1);
    if (params.count(2)) m_wowFlutter.target = params.at(2);
    if (params.count(3)) m_saturation.target = params.at(3);
    if (params.count(4)) m_mix.target = params.at(4);
}

//==============================================================================
// Parameter names
juce::String TapeEcho::getParameterName(int index) const {
    switch (index) {
        case 0: return "Time";
        case 1: return "Feedback";
        case 2: return "Wow & Flutter";
        case 3: return "Saturation";
        case 4: return "Mix";
        default: return "";
    }
}

//==============================================================================
// DelayLine implementation
void TapeEcho::DelayLine::prepare(double sampleRate, float maxDelayMs) {
    m_size = static_cast<int>(maxDelayMs * 0.001 * sampleRate) + 4; // Extra for interpolation
    m_buffer.resize(m_size, 0.0f);
    m_writePos = 0;
}

void TapeEcho::DelayLine::write(float sample) {
    m_buffer[m_writePos] = sample;
    m_writePos = (m_writePos + 1) % m_size;
}

float TapeEcho::DelayLine::read(float delaySamples) const {
    int readPos = m_writePos - static_cast<int>(delaySamples);
    while (readPos < 0) readPos += m_size;
    return m_buffer[readPos % m_size];
}

float TapeEcho::DelayLine::readInterpolated(float delaySamples) const {
    // Clamp delay samples to safe range
    delaySamples = std::max(1.0f, std::min(delaySamples, float(m_size - 4)));
    
    float readPos = float(m_writePos) - delaySamples;
    while (readPos < 0.0f) readPos += float(m_size);
    
    // Ensure readPos is in valid range
    readPos = fmodf(readPos, float(m_size));
    
    int pos0 = int(readPos);
    // Ensure all positions are within buffer bounds
    pos0 = std::min(pos0, m_size - 4);
    int pos1 = pos0 + 1;
    int pos2 = pos0 + 2;
    int pos3 = pos0 + 3;
    
    float frac = readPos - float(pos0);
    
    // Hermite interpolation for better quality
    return hermiteInterpolate(frac, 
        m_buffer[pos0], m_buffer[pos1], m_buffer[pos2], m_buffer[pos3]);
}

float TapeEcho::DelayLine::hermiteInterpolate(float frac, 
                                              float y0, float y1, float y2, float y3) const {
    // Check for NaN/Inf in input values
    if (!std::isfinite(y0)) y0 = 0.0f;
    if (!std::isfinite(y1)) y1 = 0.0f;
    if (!std::isfinite(y2)) y2 = 0.0f;
    if (!std::isfinite(y3)) y3 = 0.0f;
    if (!std::isfinite(frac)) return y1; // fallback to y1
    
    // Hermite interpolation coefficients
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    
    float result = ((c3 * frac + c2) * frac + c1) * frac + c0;
    
    // Final safety check
    return std::isfinite(result) ? result : 0.0f;
}

void TapeEcho::DelayLine::clear() {
    std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
    m_writePos = 0;
}

//==============================================================================
// TapeModulation implementation
TapeEcho::TapeModulation::TapeModulation() {
    reset();
}

void TapeEcho::TapeModulation::reset() {
    m_wowPhase = 0.0f;
    m_flutterPhase1 = 0.0f;
    m_flutterPhase2 = 0.0f;
    m_driftPhase = 0.0f;
    m_scrapePhase = 0.0f;
    m_randomWalk = 0.0f;
    m_randomTarget = 0.0f;
    m_mechanicalResonance = 0.0f;
    m_rngState = 1;
}

void TapeEcho::TapeModulation::prepare(double sampleRate) {
    // Pre-compute phase increment
    m_phaseIncrement = 2.0f * M_PI / static_cast<float>(sampleRate);
    
    // Pre-compute resonance coefficient
    m_resonanceCoeff = 2.0f * M_PI * MECHANICAL_RES_FREQ / sampleRate;
}

void TapeEcho::TapeModulation::updateRandomTarget() {
    // Update random target once per block
    m_randomTarget = fastRandom() * 0.3f;
}

float TapeEcho::TapeModulation::process(float amount) {
    // Update all oscillator phases using pre-computed increment
    m_wowPhase += WOW_RATE * m_phaseIncrement;
    m_flutterPhase1 += FLUTTER_RATE1 * m_phaseIncrement;
    m_flutterPhase2 += FLUTTER_RATE2 * m_phaseIncrement;
    m_driftPhase += DRIFT_RATE * m_phaseIncrement;
    m_scrapePhase += SCRAPE_RATE * m_phaseIncrement;
    
    // Wrap phases
    auto wrapPhase = [](float& phase) {
        while (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    };
    
    wrapPhase(m_wowPhase);
    wrapPhase(m_flutterPhase1);
    wrapPhase(m_flutterPhase2);
    wrapPhase(m_driftPhase);
    wrapPhase(m_scrapePhase);
    
    // Smooth random walk
    m_randomWalk += (m_randomTarget - m_randomWalk) * 0.001f;
    
    // Mechanical resonance using pre-computed coefficient
    float resonanceInput = std::sin(m_flutterPhase1) * 0.1f;
    m_mechanicalResonance = m_mechanicalResonance * 0.98f + 
                           resonanceInput * m_resonanceCoeff;
    
    // Combine all modulation sources with defined depths
    float wow = std::sin(m_wowPhase) * WOW_DEPTH;
    float flutter1 = std::sin(m_flutterPhase1) * FLUTTER_DEPTH1;
    float flutter2 = std::sin(m_flutterPhase2) * FLUTTER_DEPTH2;
    float drift = std::sin(m_driftPhase) * DRIFT_DEPTH;
    float scrape = std::sin(m_scrapePhase) * SCRAPE_DEPTH;
    float random = m_randomWalk * 0.002f;
    float resonance = m_mechanicalResonance * 0.001f;
    
    float total = wow + flutter1 + flutter2 + drift + scrape + random + resonance;
    
    return total * amount;
}

//==============================================================================
// TapeSaturation implementation
TapeEcho::TapeSaturation::TapeSaturation() {
    reset();
}

void TapeEcho::TapeSaturation::reset() {
    m_prevInput = 0.0f;
    m_prevOutput = 0.0f;
    m_hysteresisState = 0.0f;
    m_magnetization = 0.0f;
}

float TapeEcho::TapeSaturation::process(float input, float amount, float bias) {
    // Apply hysteresis decay
    m_magnetization *= HYSTERESIS_DECAY;
    
    // Input gain staging
    float drive = 1.0f + amount * 4.0f;
    float x = input * drive;
    
    // Add bias
    x += bias * amount;
    
    // Apply hysteresis
    float y = softClipWithHysteresis(x, amount);
    
    // Remove DC
    y -= bias * amount * 0.7f;
    
    // Output gain compensation (protect against divide by zero)
    float compensation = std::max(0.1f, drive * 0.9f);
    return y / compensation;
}

float TapeEcho::TapeSaturation::softClipWithHysteresis(float input, float drive) {
    // Simple hysteresis model
    float delta = input - m_prevInput;
    
    // Update magnetization with hysteresis
    float saturation = SATURATION_LEVEL * (1.0f + drive);
    float coercivity = COERCIVITY * (1.0f - drive * 0.3f);
    
    // Langevin function approximation
    float alpha = input / coercivity;
    float targetMag = saturation * (std::tanh(alpha) + alpha * 0.1f);
    
    // Hysteresis: magnetization lags behind input
    float hystFactor = 1.0f - std::exp(-std::abs(delta) * 5.0f);
    m_magnetization += (targetMag - m_magnetization) * hystFactor * 0.5f;
    
    // Add nonlinearity
    float output = m_magnetization;
    
    // Soft clipping
    if (std::abs(output) > 0.7f) {
        float sign = output < 0 ? -1.0f : 1.0f;
        float excess = std::abs(output) - 0.7f;
        output = sign * (0.7f + std::tanh(excess * 2.0f) * 0.3f);
    }
    
    // Update state
    m_prevInput = input;
    m_prevOutput = output;
    m_hysteresisState = output - input;
    
    return output;
}

//==============================================================================
// TapeFilter implementation
void TapeEcho::TapeFilter::prepare(double sampleRate) {
    // Set initial filter frequencies
    m_recordEQ.setFrequency(3000.0f, sampleRate);
    m_recordEQ.setResonance(0.7f);
    
    m_biasFilter.setFrequency(15000.0f, sampleRate);
    m_biasFilter.setResonance(2.0f);
    
    // Initial playback filters (will be updated based on tape age)
    updateCoefficients(0.5f, sampleRate);
}

void TapeEcho::TapeFilter::reset() {
    m_recordEQ.reset();
    m_headBump.reset();
    m_gapLoss.reset();
    m_biasFilter.reset();
    m_dcBlockerX = m_dcBlockerY = 0.0f;
    m_currentTapeAge = -1.0f;
}

float TapeEcho::TapeFilter::processRecord(float input) {
    // Pre-emphasis: boost highs before recording
    float emphasized = input + m_recordEQ.processHighpass(input) * 0.3f;
    
    // Remove bias frequency
    float filtered = emphasized - m_biasFilter.processBandpass(emphasized) * 0.1f;
    
    return filtered;
}

float TapeEcho::TapeFilter::processPlayback(float input, float tapeAge) {
    // Use cached filter settings
    float signal = input + m_headBump.processBandpass(input) * (0.2f + tapeAge * 0.1f);
    
    // Gap loss
    signal = m_gapLoss.processLowpass(signal);
    
    // DC blocking
    const float dcAlpha = 0.995f;
    float dcOut = signal - m_dcBlockerX + dcAlpha * m_dcBlockerY;
    m_dcBlockerX = signal;
    m_dcBlockerY = dcOut;
    
    return dcOut;
}

void TapeEcho::TapeFilter::updateCoefficients(float tapeAge, double sampleRate) {
    // Only update if tape age changed significantly
    if (std::abs(tapeAge - m_currentTapeAge) < 0.001f) {
        return;
    }
    
    // Head bump - resonance in low mids
    float bumpFreq = 100.0f * (1.0f - tapeAge * 0.3f);
    float bumpQ = 2.0f - tapeAge * 1.0f;
    
    m_headBump.setFrequency(bumpFreq, sampleRate);
    m_headBump.setResonance(bumpQ);
    
    // Gap loss - high frequency rolloff
    float gapFreq = 8000.0f - tapeAge * 6000.0f;
    m_gapLoss.setFrequency(gapFreq, sampleRate);
    m_gapLoss.setResonance(0.7f);
    
    m_currentTapeAge = tapeAge;
}

//==============================================================================
// TapeCompressor implementation
void TapeEcho::TapeCompressor::reset() {
    m_envelope = 0.0f;
    m_attackTime = 0.01f;
    m_releaseTime = 0.0005f;
}

float TapeEcho::TapeCompressor::process(float input, float amount) {
    float inputLevel = std::abs(input);
    
    // Program-dependent time constants
    updateTimeConstants(inputLevel);
    
    // Update envelope
    if (inputLevel > m_envelope) {
        m_envelope += (inputLevel - m_envelope) * m_attackTime;
    } else {
        m_envelope += (inputLevel - m_envelope) * m_releaseTime;
    }
    
    // Soft knee compression
    const float threshold = 0.5f;
    const float knee = 0.1f;
    float ratio = 1.0f + amount * 3.0f;
    
    float gain = 1.0f;
    
    if (m_envelope > threshold - knee) {
        if (m_envelope < threshold + knee) {
            // Soft knee region
            float kneePos = (m_envelope - threshold + knee) / (2.0f * knee);
            float kneeFactor = kneePos * kneePos;
            gain = 1.0f - kneeFactor * (1.0f - 1.0f/ratio);
        } else {
            // Above knee
            float excess = m_envelope - threshold;
            float compressedExcess = excess / ratio;
            gain = (threshold + compressedExcess) / m_envelope;
        }
    }
    
    return input * gain;
}

void TapeEcho::TapeCompressor::updateTimeConstants(float inputLevel) {
    // Faster attack for transients
    if (inputLevel > 0.7f) {
        m_attackTime = 0.05f;
        m_releaseTime = 0.002f;
    } else {
        m_attackTime = 0.01f;
        m_releaseTime = 0.0005f;
    }
}

//==============================================================================
// ChannelState implementation
void TapeEcho::ChannelState::prepare(double sampleRate) {
    delayLine.prepare(sampleRate, MAX_DELAY_MS);
    filter.prepare(sampleRate);
    filter.reset();
    compressor.reset();
    saturation.reset();
    modulation.prepare(sampleRate);
    modulation.reset();
    feedback.reset();
    
    // Pre-compute feedback filter coefficients
    const float hpFreq = 100.0f;
    feedback.hpAlpha = 1.0f - std::exp(-2.0f * M_PI * hpFreq / sampleRate);
    
    // LP alpha will be updated based on feedback amount
    const float lpFreq = 6000.0f;
    feedback.lpAlpha = 1.0f - std::exp(-2.0f * M_PI * lpFreq / sampleRate);
}

void TapeEcho::ChannelState::reset() {
    delayLine.clear();
    filter.reset();
    compressor.reset();
    saturation.reset();
    modulation.reset();
    feedback.reset();
    tapeAge = 0.0f;
}