#include "KStyleOverdrive.h"
#include "DspEngineUtilities.h"  // For DenormalGuard and scrubBuffer
#if defined(__SSE2__) && (defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86))
 #include <immintrin.h>
#endif

KStyleOverdrive::KStyleOverdrive() {
    // musical defaults
    pDrive_.target.store(0.35f); // light grit
    pTone_.target.store(0.5f);   // neutral tilt
    pLevel_.target.store(0.5f);  // 0 dB
    pMix_.target.store(1.0f);    // fully wet

    pDrive_.snap(); pTone_.snap(); pLevel_.snap(); pMix_.snap();

   #if defined(__SSE2__)
    _mm_setcsr(_mm_getcsr() | 0x8040); // FTZ/DAZ
   #endif
}

void KStyleOverdrive::prepareToPlay(double fs, int /*samplesPerBlock*/) {
    sampleRate_ = std::max(8000.0, fs);
    const float ffs = (float) sampleRate_;

    // gentle UI smoothing
    pDrive_.setTimeConst(0.03f, ffs);
    pTone_.setTimeConst(0.03f, ffs);
    pLevel_.setTimeConst(0.02f, ffs);
    pMix_.setTimeConst(0.02f, ffs);

    for (int ch = 0; ch < 2; ++ch) {
        tone_[ch].prepare(sampleRate_);
        tone_[ch].reset();
        oversampler_[ch].prepare(ffs);
        oversampler_[ch].reset();
        dcBlocker_[ch].reset();
    }
}

void KStyleOverdrive::reset() {
    for (int ch = 0; ch < 2; ++ch) {
        tone_[ch].reset();
        oversampler_[ch].reset();
        dcBlocker_[ch].reset();
    }
}

void KStyleOverdrive::updateParameters(const std::map<int, float>& params) {
    // Only update parameters that are actually in the map
    // Don't reset others to defaults!
    auto it = params.find(0);
    if (it != params.end()) pDrive_.target.store(clamp01(it->second), std::memory_order_relaxed);
    
    it = params.find(1);
    if (it != params.end()) pTone_.target.store(clamp01(it->second), std::memory_order_relaxed);
    
    it = params.find(2);
    if (it != params.end()) pLevel_.target.store(clamp01(it->second), std::memory_order_relaxed);
    
    it = params.find(3);
    if (it != params.end()) pMix_.target.store(clamp01(it->second), std::memory_order_relaxed);
}

juce::String KStyleOverdrive::getParameterName(int index) const {
    switch (index) {
        case 0: return "Drive";
        case 1: return "Tone";
        case 2: return "Level";
        case 3: return "Mix";
        default:return {};
    }
}

void KStyleOverdrive::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;  // Enable denormal protection
    
    const int nCh = std::min(buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0) return;

    // block-smooth params
    const float drive = pDrive_.next();
    const float tone  = pTone_.next();
    const float level = fromdB(juce::jmap(pLevel_.next(), 0.0f, 1.0f, -12.0f, +12.0f));
    const float mix   = pMix_.next();

    for (int ch = 0; ch < nCh; ++ch)
        tone_[ch].setMix(tone);

    auto* Lr = buffer.getReadPointer(0);
    auto* Rr = (nCh > 1) ? buffer.getReadPointer(1) : Lr;
    auto* Lw = buffer.getWritePointer(0);
    auto* Rw = (nCh > 1) ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < n; ++i) {
        float inL = Lr[i];
        float inR = Rr[i];
        
        // Store dry signal before any processing
        float dryL = inL;
        float dryR = inR;

        // DC blocking on input
        inL = dcBlocker_[0].process(inL);
        inR = dcBlocker_[1].process(inR);
        
        // Skip pre-EQ high-pass - it's reducing level too much
        // The tone control provides sufficient EQ flexibility
        float preL = inL;
        float preR = inR;

        // 2x oversampling for nonlinearity to prevent aliasing
        float upL[2], upR[2];
        oversampler_[0].upsample(preL, upL);
        oversampler_[1].upsample(preR, upR);
        
        // Process at 2x rate
        for (int j = 0; j < 2; ++j) {
            upL[j] = waveshaper(upL[j], drive);
            upR[j] = waveshaper(upR[j], drive);
        }
        
        // Downsample
        float odL = oversampler_[0].downsample(upL);
        float odR = oversampler_[1].downsample(upR);

        // Post "tone" tilt (musical single knob)
        float postL = tone_[0].process(odL);
        float postR = tone_[1].process(odR);

        // Output level & mix
        float wetL = postL * level;
        float wetR = postR * level;

        float outL = (1.0f - mix) * dryL + mix * wetL;
        float outR = (1.0f - mix) * dryR + mix * wetR;

        // comprehensive NaN/Inf protection
        if (!std::isfinite(outL) || std::isnan(outL)) outL = 0.0f;
        if (!std::isfinite(outR) || std::isnan(outR)) outR = 0.0f;

        Lw[i] = outL;
        if (Rw) Rw[i] = outR;
    }
    
    scrubBuffer(buffer);  // Enable buffer scrubbing
}