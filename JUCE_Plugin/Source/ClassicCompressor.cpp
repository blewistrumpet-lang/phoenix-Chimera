#include "ClassicCompressor.h"

// Platform-specific SIMD support (should match header)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define HAS_SIMD 1
#else
    #define HAS_SIMD 0
#endif

ClassicCompressor::ClassicCompressor() {
    // Initialize with professional defaults
    m_threshold.reset(-12.0f);
    m_ratio.reset(4.0f);
    m_attack.reset(10.0f);
    m_release.reset(100.0f);
    m_knee.reset(2.0f);
    m_makeupGain.reset(0.0f);
    m_mix.reset(1.0f);
    m_lookahead.reset(0.0f);
    m_autoRelease.reset(0.5f);
    m_sidechain.reset(0.0f);
}

void ClassicCompressor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Enable denormal prevention
    enableDenormalPrevention();
    
    // Configure smoothers with proper time constants
    m_threshold.setSampleRate(sampleRate, 10.0f);
    m_ratio.setSampleRate(sampleRate, 20.0f);
    m_attack.setSampleRate(sampleRate, 5.0f);
    m_release.setSampleRate(sampleRate, 10.0f);
    m_knee.setSampleRate(sampleRate, 20.0f);
    m_makeupGain.setSampleRate(sampleRate, 10.0f);
    m_mix.setSampleRate(sampleRate, 5.0f);
    m_lookahead.setSampleRate(sampleRate, 20.0f);
    m_autoRelease.setSampleRate(sampleRate, 30.0f);
    m_sidechain.setSampleRate(sampleRate, 20.0f);
    
    // Initialize DSP components
    for (int ch = 0; ch < 2; ++ch) {
        m_envelopes[ch].reset();
        m_sidechains[ch].prepare(sampleRate);
        m_gainSmoothers[ch].reset();
        m_dcBlockers[ch].reset();
    }
    
    reset();
}

void ClassicCompressor::reset() {
    for (int ch = 0; ch < 2; ++ch) {
        m_envelopes[ch].reset();
        m_sidechains[ch].reset();
        m_gainSmoothers[ch].reset();
        m_dcBlockers[ch].reset();
    }
    
    m_currentGainReduction.store(0.0f, std::memory_order_relaxed);
    m_peakGainReduction.store(0.0f, std::memory_order_relaxed);
}

void ClassicCompressor::enableDenormalPrevention() {
#if HAS_SIMD && defined(__SSE__)
    // Enable flush-to-zero and denormals-are-zero
    _mm_setcsr(_mm_getcsr() | 0x8040);
#elif defined(__ARM_NEON) && defined(__ARM_FP)
    // ARM NEON equivalent - use proper ARM64 syntax
    uint64_t fpcr;
    __asm__ __volatile__ ("mrs %0, fpcr" : "=r" (fpcr));
    fpcr |= (1ULL << 24); // FZ bit
    __asm__ __volatile__ ("msr fpcr, %0" : : "r" (fpcr));
#endif
    // For other architectures, rely on compiler flags or runtime denormal handling
}

void ClassicCompressor::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Get channel pointers
    float* channelData[2] = { nullptr, nullptr };
    channelData[0] = buffer.getWritePointer(0);
    if (numChannels > 1) {
        channelData[1] = buffer.getWritePointer(1);
    } else {
        channelData[1] = channelData[0]; // Mono
    }
    
    // Process in sub-blocks for efficiency
    int samplesRemaining = numSamples;
    int currentSample = 0;
    
    while (samplesRemaining > 0) {
        int subBlockSize = std::min(samplesRemaining, SUBBLOCK_SIZE);
        processSubBlock(channelData[0] + currentSample, 
                       channelData[1] + currentSample, 
                       currentSample, subBlockSize);
        
        currentSample += subBlockSize;
        samplesRemaining -= subBlockSize;
    }
    
    scrubBuffer(buffer);
}

void ClassicCompressor::processSubBlock(float* left, float* right, int startSample, int numSamples) {
    // Update parameters once per sub-block
    double threshold = m_threshold.processSubBlock(numSamples);
    double ratio = m_ratio.processSubBlock(numSamples);
    double attack = m_attack.processSubBlock(numSamples);
    double release = m_release.processSubBlock(numSamples);
    double knee = m_knee.processSubBlock(numSamples);
    double makeupGain = m_makeupGain.processSubBlock(numSamples);
    double mix = m_mix.processSubBlock(numSamples);
    double lookaheadParam = m_lookahead.processSubBlock(numSamples);
    double autoRelease = m_autoRelease.processSubBlock(numSamples);
    double sidechainParam = m_sidechain.processSubBlock(numSamples);
    
    // Convert parameters
    double thresholdDb = -60.0 + threshold * 60.0;
    double ratioValue = 1.0 + ratio * 19.0;
    if (ratio > 0.95) ratioValue = 1000.0; // Infinity
    
    double attackMs = 0.01 + attack * 99.99;
    double releaseMs = 1.0 + release * 4999.0;
    double kneeDb = knee * 12.0;
    double makeupDb = makeupGain * 24.0;
    double lookaheadMs = lookaheadParam * 10.0;
    
    // Pre-compute coefficients for the sub-block
    bool useLookahead = lookaheadMs > 0.1;
    bool useSidechain = sidechainParam > 0.5;
    
    // Update DSP components
    for (int ch = 0; ch < 2; ++ch) {
        m_sidechains[ch].setLookahead(lookaheadMs, m_sampleRate);
        m_envelopes[ch].updateCoefficients(attackMs, releaseMs, m_sampleRate);
        m_gainComputers[ch].updateParameters(thresholdDb, ratioValue, kneeDb);
        m_gainSmoothers[ch].setTimes(attackMs, releaseMs, autoRelease, m_sampleRate);
    }
    
    // Process samples in the sub-block
    double makeupLinear = dbToLinear(makeupDb);
    
    // Copy to work buffers for processing
    std::copy(left, left + numSamples, m_workBuffer1.ptr());
    std::copy(right, right + numSamples, m_workBuffer2.ptr());
    
    for (int i = 0; i < numSamples; ++i) {
        // Sidechain processing
        double scSignals[2];
        float delayedSignals[2];
        
        if (useLookahead) {
            scSignals[0] = m_sidechains[0].processLookahead(m_workBuffer1[i], delayedSignals[0]);
            scSignals[1] = m_sidechains[1].processLookahead(m_workBuffer2[i], delayedSignals[1]);
        } else {
            delayedSignals[0] = m_workBuffer1[i];
            delayedSignals[1] = m_workBuffer2[i];
            scSignals[0] = std::abs(static_cast<double>(m_workBuffer1[i]));
            scSignals[1] = std::abs(static_cast<double>(m_workBuffer2[i]));
        }
        
        if (useSidechain) {
            scSignals[0] = m_sidechains[0].processHighpass(scSignals[0]);
            scSignals[1] = m_sidechains[1].processHighpass(scSignals[1]);
        }
        
        // Stereo linking
        double detectionSignal = 0.0;
        if (m_stereoMode == StereoMode::STEREO_LINK) {
            detectionSignal = std::max(scSignals[0], scSignals[1]);
        } else {
            detectionSignal = scSignals[0];
        }
        
        // Envelope detection (RMS)
        double envelope = m_envelopes[0].processRMS(static_cast<float>(detectionSignal));
        
        // Calculate gain reduction
        double envelopeDb = linearToDb(envelope);
        double gainReductionDb = m_gainComputers[0].computeGainReduction(envelopeDb);
        double targetGain = dbToLinear(-gainReductionDb);
        
        // Smooth gain changes
        double smoothedGain = m_gainSmoothers[0].process(targetGain, envelope);
        
        // Apply compression - simplified without inefficient SIMD for single values
        float gain = static_cast<float>(smoothedGain * makeupLinear);
        float wetMix = static_cast<float>(mix);
        float dryMix = 1.0f - wetMix;
        
        // Apply gain and mix
        float compressedL = delayedSignals[0] * gain;
        float compressedR = delayedSignals[1] * gain;
        
        left[i] = m_dcBlockers[0].process(
            m_workBuffer1[i] * dryMix + compressedL * wetMix
        );
        right[i] = m_dcBlockers[1].process(
            m_workBuffer2[i] * dryMix + compressedR * wetMix
        );
        
        // Update metering (relaxed atomic operations)
        float currentGR = m_currentGainReduction.load(std::memory_order_relaxed);
        currentGR = currentGR * 0.95f + static_cast<float>(gainReductionDb) * 0.05f;
        m_currentGainReduction.store(DSPUtils::flushDenorm(currentGR), std::memory_order_relaxed);
        
        float peakGR = m_peakGainReduction.load(std::memory_order_relaxed);
        peakGR = std::max(peakGR * 0.9999f, static_cast<float>(gainReductionDb));
        m_peakGainReduction.store(DSPUtils::flushDenorm(peakGR), std::memory_order_relaxed);
    }
}

void ClassicCompressor::updateParameters(const std::map<int, float>& params) {
    auto it = params.find(0);
    if (it != params.end()) m_threshold.setTarget(it->second);
    
    it = params.find(1);
    if (it != params.end()) m_ratio.setTarget(it->second);
    
    it = params.find(2);
    if (it != params.end()) m_attack.setTarget(it->second);
    
    it = params.find(3);
    if (it != params.end()) m_release.setTarget(it->second);
    
    it = params.find(4);
    if (it != params.end()) m_knee.setTarget(it->second);
    
    it = params.find(5);
    if (it != params.end()) m_makeupGain.setTarget(it->second);
    
    it = params.find(6);
    if (it != params.end()) m_mix.setTarget(it->second);
    
    it = params.find(7);
    if (it != params.end()) m_lookahead.setTarget(it->second);
    
    it = params.find(8);
    if (it != params.end()) m_autoRelease.setTarget(it->second);
    
    it = params.find(9);
    if (it != params.end()) m_sidechain.setTarget(it->second);
}

juce::String ClassicCompressor::getParameterName(int index) const {
    switch (index) {
        case 0: return "Threshold";
        case 1: return "Ratio";
        case 2: return "Attack";
        case 3: return "Release";
        case 4: return "Knee";
        case 5: return "Makeup";
        case 6: return "Mix";
        case 7: return "Lookahead";
        case 8: return "Auto Release";
        case 9: return "Sidechain";
        default: return "";
    }
}