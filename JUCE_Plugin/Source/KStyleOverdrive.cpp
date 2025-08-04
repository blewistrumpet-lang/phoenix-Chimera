#include "KStyleOverdrive.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// Constructor
KStyleOverdrive::KStyleOverdrive()
    : m_sampleRate(44100.0),
      m_oversampledRate(44100.0 * OVERSAMPLE_FACTOR),
      m_maxBlockSize(DEFAULT_BLOCK_SIZE),
      m_lastTone(-1.0f)
{
    // Initialize parameters
    m_drive.reset(0.3f);
    m_tone.reset(0.5f);
    m_level.reset(0.5f);
    m_mix.reset(1.0f);
}

//==============================================================================
// Prepare to play
void KStyleOverdrive::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_sampleRate       = sampleRate;
    m_oversampledRate  = sampleRate * OVERSAMPLE_FACTOR;
    m_maxBlockSize     = samplesPerBlock;

    // 10 ms smoothing for all params
    constexpr float smoothingTimeMs = 10.0f;
    m_drive.setSmoothingTime(smoothingTimeMs, sampleRate);
    m_tone.setSmoothingTime(smoothingTimeMs, sampleRate);
    m_level.setSmoothingTime(smoothingTimeMs, sampleRate);
    m_mix.setSmoothingTime(smoothingTimeMs, sampleRate);

    // Initialize filter stages
    for (auto& stage : m_filterStages)
    {
        stage.prepare(sampleRate);
        stage.reset();
    }

    // Reset tube model and tone-tracker
    m_tubeStage.reset();
    m_lastTone = -1.0f;
}

//==============================================================================
// Reset everything
void KStyleOverdrive::reset()
{
    for (auto& stage : m_filterStages)
        stage.reset();

    m_tubeStage.reset();
    m_lastTone = -1.0f;
}

//==============================================================================
// Main audio callback
void KStyleOverdrive::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    if (numSamples > m_maxBlockSize)
    {
        jassertfalse; // Debug alert
        DBG("KStyleOverdrive: block size " << numSamples
            << " exceeds max " << m_maxBlockSize);
        return;
    }

    // Update smoothed params
    m_drive.update();
    m_tone.update();
    m_level.update();
    m_mix.update();

    // Recompute tone-stack filters if needed
    if (std::abs(m_tone.current - m_lastTone) > 0.001f)
    {
        updateFilterCoefficients();
        m_lastTone = m_tone.current;
    }

    // Process left & right (or mono)
    for (int ch = 0; ch < numChannels && ch < 2; ++ch)
    {
        processBlock(buffer.getWritePointer(ch),
                     numSamples,
                     m_filterStages[ch]);
    }
}

//==============================================================================
// Per-channel block processing
void KStyleOverdrive::processBlock(float* data,
                                   int numSamples,
                                   FilterStage& state)
{
    const float driveAmt = m_drive.current;
    const float toneAmt  = m_tone.current;
    const float lvlAmt   = m_level.current;
    const float mixAmt   = m_mix.current;

    for (int i = 0; i < numSamples; ++i)
    {
        const float dry = data[i];
        float sample   = dry * INPUT_HEADROOM;

        // 1) Input HPF (80 Hz)
        sample = state.inputHighpass.processHighpass(sample);

        // 2) Pre-emphasis (720 Hz shelf)
        float emph = state.preEmphasis.processHighpass(sample);
        sample += emph * 0.5f * (1.0f + driveAmt);

        // 3) 4× oversampling via cubic interpolation
        float oversampled[OVERSAMPLE_FACTOR];
        oversampled[0] = sample;

        float next  = (i + 1 < numSamples) ? data[i+1] * INPUT_HEADROOM : sample;
        float prev  = state.upsampleHistory;
        float next2 = (i + 2 < numSamples) ? data[i+2] * INPUT_HEADROOM : next;

        for (int j = 1; j < OVERSAMPLE_FACTOR; ++j)
        {
            float f   = float(j) / OVERSAMPLE_FACTOR;
            float c0  = sample;
            float c1  = 0.5f * (next - prev);
            float c2  = prev - 2.5f*sample + 2.0f*next - 0.5f*next2;
            float c3  = 0.5f*(next2 - prev) + 1.5f*(sample - next);
            oversampled[j] = ((c3*f + c2)*f + c1)*f + c0;
        }
        state.upsampleHistory = sample;

        // 4) Per-sample oversampled processing
        for (int j = 0; j < OVERSAMPLE_FACTOR; ++j)
        {
            float x       = state.antiAliasUp.processLowpass(oversampled[j]);
            float driven  = x * (1.0f + driveAmt * 15.0f);
            float tubeOut = processTubeStage(driven, driveAmt);
            oversampled[j] = state.antiAliasDown.processLowpass(tubeOut);
        }

        // 5) Downsample (average; filters applied)
        float sum = 0.0f;
        for (auto v : oversampled) sum += v;
        sample = sum / OVERSAMPLE_FACTOR;

        // 6) Tone stack
        sample = applyToneStack(sample, state, toneAmt);

        // 7) DC blocking
        sample = state.dcBlocker.process(sample);

        // 8) Output gain + soft limit
        float output = sample * lvlAmt * 1.4f;
        output = softLimit(output);

        // 9) Dry/wet mix
        data[i] = dry * (1.0f - mixAmt) + output * mixAmt;
    }
}

//==============================================================================
// Tube stage wrapper
float KStyleOverdrive::processTubeStage(float input, float drive)
{
    return m_tubeStage.process(input, drive);
}

//==============================================================================
// Tone-stack
float KStyleOverdrive::applyToneStack(float input,
                                      FilterStage& state,
                                      float tone)
{
    float bassGain   = 1.0f - tone * 0.5f;
    float midGain    = 1.0f - std::abs(tone - 0.5f) * 0.6f;
    float trebleGain = 0.5f + tone * 0.5f;

    float low  = state.toneStackLow.processLowpass(input);
    float mid  = state.toneStackMid.processBandpass(input);
    float high = state.toneStackHigh.processHighpass(input);

    float combined = low*bassGain + mid*midGain + high*trebleGain;
    float fb       = combined * 0.1f * (1.0f - tone);
    state.toneFeedback = state.toneFeedback*0.8f + fb*0.2f;

    return combined + state.toneFeedback;
}

//==============================================================================
// Soft limiter
float KStyleOverdrive::softLimit(float input)
{
    float x = input;

    if (std::abs(x) > SAFETY_LIMITER_KNEE)
    {
        float over = std::abs(x) - SAFETY_LIMITER_KNEE;
        float lim  = SAFETY_LIMITER_KNEE + std::tanh(over*2.0f)*0.25f;
        x = lim * (x<0 ? -1.0f : 1.0f);
    }

    return std::clamp(x,
                      -SAFETY_LIMITER_THRESHOLD,
                      +SAFETY_LIMITER_THRESHOLD);
}

//==============================================================================
// Update tone-stack filter f & Q
void KStyleOverdrive::updateFilterCoefficients()
{
    float tone = m_tone.current;
    for (auto& s : m_filterStages)
    {
        float bass = 80.0f + tone * 40.0f;
        s.toneStackLow.setFrequency(bass, m_sampleRate);
        s.toneStackLow.setResonance(0.707f);

        float mid = 400.0f + tone * 200.0f;
        float q   = 0.7f + std::abs(tone - 0.5f) * 2.0f;
        s.toneStackMid.setFrequency(mid, m_sampleRate);
        s.toneStackMid.setResonance(q);

        float treb = 2000.0f + tone * 2000.0f;
        s.toneStackHigh.setFrequency(treb, m_sampleRate);
        s.toneStackHigh.setResonance(0.707f);
    }
}

//==============================================================================
// Parameter updates from host
void KStyleOverdrive::updateParameters(const std::map<int, float>& params)
{
    if (params.count(0)) m_drive.target = params.at(0);
    if (params.count(1)) m_tone.target  = params.at(1);
    if (params.count(2)) m_level.target = params.at(2);
    if (params.count(3)) m_mix.target   = params.at(3);
}

//==============================================================================
// Parameter names
juce::String KStyleOverdrive::getParameterName(int index) const
{
    switch (index)
    {
        case 0: return "Drive";
        case 1: return "Tone";
        case 2: return "Level";
        case 3: return "Mix";
        default:return {};
    }
}

//==============================================================================
// FilterStage — prepare
void KStyleOverdrive::FilterStage::prepare(double sampleRate)
{
    inputHighpass.setFrequency(80.0f, sampleRate);
    inputHighpass.setResonance(0.707f);

    preEmphasis.setFrequency(KStyleOverdrive::PRE_EMPHASIS_FC, sampleRate);
    preEmphasis.setResonance(0.707f);

    float aaFreq = sampleRate * 0.48f;
    antiAliasUp.setFrequency(aaFreq, sampleRate * KStyleOverdrive::OVERSAMPLE_FACTOR);
    antiAliasUp.setResonance(0.707f);
    antiAliasDown.setFrequency(aaFreq, sampleRate * KStyleOverdrive::OVERSAMPLE_FACTOR);
    antiAliasDown.setResonance(0.707f);

    toneStackLow.setFrequency(100.0f, sampleRate);
    toneStackLow.setResonance(0.707f);
    toneStackMid.setFrequency(500.0f, sampleRate);
    toneStackMid.setResonance(1.0f);
    toneStackHigh.setFrequency(3000.0f, sampleRate);
    toneStackHigh.setResonance(0.707f);

    dcBlocker.setCutoff(KStyleOverdrive::DC_BLOCK_FC, sampleRate);
}

//==============================================================================
// FilterStage — reset
void KStyleOverdrive::FilterStage::reset()
{
    inputHighpass.reset();
    preEmphasis.reset();
    antiAliasUp.reset();
    antiAliasDown.reset();
    toneStackLow.reset();
    toneStackMid.reset();
    toneStackHigh.reset();
    dcBlocker.reset();

    upsampleHistory = 0.0f;
    toneFeedback    = 0.0f;
}

//==============================================================================
// TubeStage — process
float KStyleOverdrive::TubeStage::process(float input, float drive)
{
    // Grid blocking
    if (input > 0.5f)
        gridCurrent = gridCurrent * 0.999f + (input - 0.5f) * 0.001f;
    else
        gridCurrent *= 0.995f;

    float blocked = input - gridCurrent * 0.1f * drive;

    // Soft-knee compression
    float threshold = 0.5f - drive * 0.2f;
    float ratio     = 1.0f + drive * 3.0f;
    float comp = blocked;
    if (std::abs(blocked) > threshold)
    {
        float over = std::abs(blocked) - threshold;
        comp = (threshold + over/ratio) * (blocked < 0 ? -1.0f : 1.0f);
    }

    // Waveshaping
    float biased = comp + bias * drive;
    float x      = std::tanh(biased * (1.0f + drive));

    float pos = x > 0 ? x : 0;
    float neg = x < 0 ? -x : 0;
    pos = 1.0f - std::exp(-pos*3.0f);
    neg = 1.0f - std::exp(-neg*2.5f);
    x   = pos - neg;

    // Remove DC
    x -= bias * 0.7f;

    // Harmonic reinforcement
    float h2 = x*x * warmth * 0.1f;
    float h3 = x*x*x * warmth * 0.05f;

    // Power sag
    currentSag = currentSag*0.99f + std::abs(x)*0.01f;
    x *= (1.0f - currentSag*0.05f);

    // Microphonics
    x += noise(rng) * 0.00001f;

    return (x + h2 - h3) * 0.7f;
}

//==============================================================================
// TubeStage — reset
void KStyleOverdrive::TubeStage::reset()
{
    currentSag   = 0.0f;
    gridCurrent  = 0.0f;
}