#include "TapeEcho.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// ctor with safe defaults that make sound
TapeEcho::TapeEcho()
{
    pTime_.target.store(0.375f);       // ~375 ms at default mapping
    pFeedback_.target.store(0.35f);
    pWowFlutter_.target.store(0.25f);
    pSaturation_.target.store(0.3f);
    pMix_.target.store(0.35f);

    pTime_.snap(); pFeedback_.snap(); pWowFlutter_.snap(); pSaturation_.snap(); pMix_.snap();
}

//==============================================================================
// prepare
void TapeEcho::prepareToPlay(double sr, int /*blockSize*/)
{
    sampleRate_ = std::max(8000.0, sr);

    // smoothing τ
    const float fs = (float) sampleRate_;
    pTime_.setTimeConst(0.03f, fs);
    pFeedback_.setTimeConst(0.02f, fs);
    pWowFlutter_.setTimeConst(0.05f, fs);
    pSaturation_.setTimeConst(0.025f, fs);
    pMix_.setTimeConst(0.015f, fs);

    pTime_.snap(); pFeedback_.snap(); pWowFlutter_.snap(); pSaturation_.snap(); pMix_.snap();

    for (auto& c : ch_) c.prepare(sampleRate_);
}

//==============================================================================
// reset
void TapeEcho::reset()
{
    for (auto& c : ch_) c.reset();
}

//==============================================================================
// parameter updates (lock-free)
void TapeEcho::updateParameters(const std::map<int, float>& params)
{
    auto set = [&](int idx, SmoothParam& p, float def, float lo, float hi){
        auto it = params.find(idx);
        float v = (it != params.end()) ? it->second : def;
        p.target.store(juce::jlimit(lo, hi, v), std::memory_order_relaxed);
    };

    set(0, pTime_,       0.375f, 0.0f, 1.0f);
    set(1, pFeedback_,   0.35f,  0.0f, 1.0f);
    set(2, pWowFlutter_, 0.25f,  0.0f, 1.0f);
    set(3, pSaturation_, 0.30f,  0.0f, 1.0f);
    set(4, pMix_,        0.35f,  0.0f, 1.0f);
}

juce::String TapeEcho::getParameterName(int index) const
{
    switch (index) {
        case 0: return "Time";
        case 1: return "Feedback";
        case 2: return "Wow & Flutter";
        case 3: return "Saturation";
        case 4: return "Mix";
        default: return {};
    }
}

//==============================================================================
// DelayLine
void TapeEcho::DelayLine::prepare(double sampleRate, float maxDelayMs)
{
    // Clamp sample rate to prevent overflow
    sampleRate = std::min(sampleRate, 384000.0);
    // Ensure maxDelayMs is valid
    maxDelayMs = std::max(1.0f, std::min(maxDelayMs, 5000.0f));
    
    const int needed = (int) std::ceil(sampleRate * (maxDelayMs * 0.001f)) + kExtraGuard;
    const int targetSize = std::max(needed, 128); // Minimum 128 samples
    
    // Safe buffer allocation with proper error handling
    try {
        buf_.assign((size_t) targetSize, 0.0f);
        size_ = static_cast<int>(buf_.size()); // Keep size_ in sync with actual buffer
    } catch (...) {
        // Fallback to minimum safe size on allocation failure
        try {
            buf_.assign(128, 0.0f);  // Minimum for about 3ms at 44.1kHz
            size_ = static_cast<int>(buf_.size());
        } catch (...) {
            // Critical failure - ensure consistent state
            buf_.clear();
            size_ = 0;
        }
    }
    w_ = 0;
}

void TapeEcho::DelayLine::clear()
{
    std::fill(buf_.begin(), buf_.end(), 0.0f);
    w_ = 0;
}

// Safe wrapping function that handles negative indices correctly
static inline int wrapi(int i, int n) noexcept { 
    if (n <= 1) return 0;  // Safety check - need at least 2 samples
    // Proper negative modulo handling
    i = ((i % n) + n) % n;
    return i; 
}

float TapeEcho::DelayLine::readCubic(float delaySamples) const noexcept
{
    // Safety check for valid buffer - use buf_.size() consistently
    if (buf_.empty() || buf_.size() <= kExtraGuard) return 0.0f;
    
    const int bufSize = static_cast<int>(buf_.size());
    
    // clamp delay to safe region using actual buffer size
    delaySamples = juce::jlimit(1.0f, (float)(bufSize-kExtraGuard), delaySamples);

    float rp = (float) w_ - delaySamples;
    // Safe wrap-around without potential infinite loop
    if (rp < 0.0f) {
        rp = fmodf(rp, (float)bufSize);
        if (rp < 0.0f) rp += (float)bufSize;
    }

    // Safe float to int conversion
    const int i0 = (int) std::floor(std::max(0.0f, std::min(rp, (float)(bufSize-1))));
    const float frac = rp - (float) i0;

    // Additional safety check
    if (bufSize < 4) return 0.0f;
    
    // neighborhood: [-1,0,1,2] wrapped with bounds checking
    const int idx_1 = wrapi(i0 - 1, bufSize);
    const int idx0  = wrapi(i0,     bufSize);
    const int idx1  = wrapi(i0 + 1, bufSize);
    const int idx2  = wrapi(i0 + 2, bufSize);
    
    // Safe bounds-checked access - no unsafe size_t casts
    const float y_1 = (idx_1 >= 0 && idx_1 < bufSize) ? buf_[idx_1] : 0.0f;
    const float y0  = (idx0  >= 0 && idx0  < bufSize) ? buf_[idx0]  : 0.0f;
    const float y1  = (idx1  >= 0 && idx1  < bufSize) ? buf_[idx1]  : 0.0f;
    const float y2  = (idx2  >= 0 && idx2  < bufSize) ? buf_[idx2]  : 0.0f;

    // Catmull-Rom cubic (Hermite)
    const float c0 = y0;
    const float c1 = 0.5f * (y1 - y_1);
    const float c2 = y_1 - 2.5f*y0 + 2.0f*y1 - 0.5f*y2;
    const float c3 = 0.5f * (y2 - y_1) + 1.5f * (y0 - y1);

    float out = ((c3*frac + c2)*frac + c1)*frac + c0;
    if (!std::isfinite(out)) out = 0.0f;
    return flushDenorm(out);
}

//==============================================================================
// main processing
void TapeEcho::process(juce::AudioBuffer<float>& buffer)
{
    const int nCh = std::min(buffer.getNumChannels(), kMaxChannels);
    const int n   = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0) return;

    // Update block-smoothed params (coeffs are per-sample stable anyway)
    const float t      = pTime_.next();
    const float fbAmt  = pFeedback_.next();
    const float modAmt = pWowFlutter_.next();
    const float satAmt = pSaturation_.next();
    const float mix    = pMix_.next();

    // one random target per block (cheap)
    for (int ch = 0; ch < nCh; ++ch) ch_[ch].mod.updateRandomOncePerBlock();

    // process per channel independently
    for (int ch = 0; ch < nCh; ++ch)
    {
        auto* wr = buffer.getWritePointer(ch);
        auto* rd = buffer.getReadPointer(ch);

        auto& cs = ch_[ch];

        // Map time [0..1] to 10..2000ms outside the loop
        const float baseDelayMs = kMinDelayMs + t * (kMaxDelayMs - kMinDelayMs);

        // dynamic LP cutoff for feedback based on fbAmt, precompute alpha
        const float lpHz = 6000.0f * (1.0f - 0.3f * fbAmt);
        cs.lpAlpha = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * lpHz / (float)sampleRate_);

        for (int i = 0; i < n; ++i)
        {
            // safe input
            float in = rd[i];
            if (!std::isfinite(in)) in = 0.0f;

            // per-sample modulation of delay (speed-based mapping)
            const float speedMod = cs.mod.process(modAmt); // ~[-small..small]
            const float modDelayMs = baseDelayMs * (1.0f + speedMod); // simple & musical
            float delaySamples = juce::jlimit(1.0f,
                (float) cs.delay.capacity(),
                modDelayMs * (float)sampleRate_ * 0.001f);

            // read delayed
            float delayed = cs.delay.readCubic(delaySamples);

            // playback path "tape" tone
            // head bump (BP) add, then HF loss LP; simple pre-emphasis on record side
            float playTone = delayed;
            playTone += cs.headBumpBP.processBP(delayed) * 0.18f;
            playTone  = cs.gapLossLP.processLP(playTone);

            // "tape" compression/saturation on wet
            playTone = saturateTape(playTone, satAmt * 0.6f);

            // feedback conditioning: HP -> LP -> soft limit
            float fb = playTone * fbAmt;

            // 1st-order HP @ ~100 Hz
            const float hpOut = fb - cs.hpState;
            cs.hpState += cs.hpAlpha * hpOut;
            fb = hpOut;

            // dynamic LP then mild limiter
            cs.lpState += cs.lpAlpha * (fb - cs.lpState);
            fb = cs.lpState;
            fb = softSaturate(fb);

            // record path pre-emphasis & gentle comp to write into tape
            float rec = in;
            rec = rec + cs.preEmphHP.processHP(in) * 0.25f;
            rec = saturateTape(rec, satAmt * 0.25f);

            // write
            const float writeSig = std::isfinite(rec + fb) ? (rec + fb) : 0.0f;
            cs.delay.write(writeSig);

            // dry/wet mix (linear is fine here; feel free to switch to equal-power)
            const float out = flushDenorm(in * (1.0f - mix) + playTone * mix);

            wr[i] = std::isfinite(out) ? out : 0.0f;
        }
    }

    // mono safety: if host gave 1 channel, nothing else to do; if >2, we ignore extras
}

//==============================================================================
// per-sample helper (unused in this refactor but kept for clarity)
float TapeEcho::processSample(float in, ChannelState& cs) noexcept
{
    // Not used — we integrated sample loop directly in process() for fewer derefs.
    return in;
}