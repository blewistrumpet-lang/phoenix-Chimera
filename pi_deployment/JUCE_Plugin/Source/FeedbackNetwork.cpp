#include "FeedbackNetwork.h"
#include <algorithm>

FeedbackNetwork::FeedbackNetwork() {}
FeedbackNetwork::~FeedbackNetwork() {}

void FeedbackNetwork::prepareToPlay(double sampleRate, int samplesPerBlock) {
    fs = sampleRate;
    size_t maxDelay = static_cast<size_t>(fs * 2.0); // 2s max
    delayL.setSize(maxDelay);
    delayR.setSize(maxDelay);
    reset();
}

void FeedbackNetwork::reset() {
    delayL.clear();
    delayR.clear();
    modPhaseL = modPhaseR = 0.0;
}

void FeedbackNetwork::updateParameters(const std::map<int, float>& params) {
    auto get = [&](int id, float def) {
        auto it = params.find(id);
        return it != params.end() ? it->second : def;
    };

    delayTimeSec    = std::max(0.001f, get(kDelayTime, 0.25f));
    // Limited to 0.85 to prevent runaway feedback
    feedback        = clampSafe(get(kFeedback, 0.5f), -0.85f, 0.85f);  // Safer feedback range
    crossFeed       = clampSafe(get(kCrossFeed, 0.0f), -0.85f, 0.85f); // Safer crossfeed range
    diffusion       = std::clamp(get(kDiffusion, 0.0f), 0.0f, 1.0f);
    modulationDepth = std::clamp(get(kModulation, 0.0f), 0.0f, 0.05f);
    freeze          = std::clamp(get(kFreeze, 0.0f), 0.0f, 1.0f);
    shimmer         = std::clamp(get(kShimmer, 0.0f), 0.0f, 1.0f);
    mix             = std::clamp(get(kMix, 0.5f), 0.0f, 1.0f);  // Default to 50% mix instead of 100%
}

void FeedbackNetwork::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;  // RAII denormal protection for entire process block
    
    auto* left  = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    size_t delaySamples = static_cast<size_t>(delayTimeSec * fs);

    for (int n = 0; n < buffer.getNumSamples(); ++n) {
        // Simple sine LFO modulation
        float modOffsetL = std::sin(modPhaseL) * modulationDepth * fs;
        float modOffsetR = std::sin(modPhaseR) * modulationDepth * fs;
        modPhaseL += 2.0 * M_PI * modRate / fs;
        modPhaseR += 2.0 * M_PI * (modRate * 1.1) / fs;

        // Read delay
        float dl = delayL.read(std::clamp<size_t>(delaySamples + (size_t)modOffsetL, 1, delayL.buffer.size()-1));
        float dr = delayR.read(std::clamp<size_t>(delaySamples + (size_t)modOffsetR, 1, delayR.buffer.size()-1));

        // Apply freeze
        if (freeze > 0.5f) {
            left[n]  = sanitize(dl);
            if (right) right[n] = sanitize(dr);
        } else {
            // Feedback with crossfeed
            float inL = sanitize(left[n] + dr * crossFeed);
            float inR = sanitize((right ? right[n] : left[n]) + dl * crossFeed);

            // Diffusion (simple 1st order)
            inL = inL + diffusion * (dr - inL);
            inR = inR + diffusion * (dl - inR);

            // Write to delay lines - include both input and feedback
            delayL.write(sanitize(inL + dl * feedback));
            delayR.write(sanitize(inR + dr * feedback));

            // Mix output
            left[n]  = DSPUtils::flushDenorm((1.0f - mix) * left[n]  + mix * dl);
            if (right) right[n] = DSPUtils::flushDenorm((1.0f - mix) * right[n] + mix * dr);
        }
    }
    
    // Scrub buffer for NaN/Inf protection at end of processing
    scrubBuffer(buffer);
}

juce::String FeedbackNetwork::getParameterName(int index) const {
    switch (index) {
        case kDelayTime:   return "Delay Time";
        case kFeedback:    return "Feedback";
        case kCrossFeed:   return "Crossfeed";
        case kDiffusion:   return "Diffusion";
        case kModulation:  return "Modulation";
        case kFreeze:      return "Freeze";
        case kShimmer:     return "Shimmer";
        case kMix:         return "Mix";
        default:           return {};
    }
}