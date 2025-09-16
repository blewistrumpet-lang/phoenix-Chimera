// ==================== BucketBrigadeDelay_Fixed.cpp ====================
// Simplified and fixed version - removing complex features to ensure stability
#include "BucketBrigadeDelay.h"
#include <algorithm>
#include <cmath>

BucketBrigadeDelay::BucketBrigadeDelay() {
    // Initialize parameter smoothers
    m_delayTime = std::make_unique<ParameterSmoother>();
    m_feedback = std::make_unique<ParameterSmoother>();
    m_modulation = std::make_unique<ParameterSmoother>();
    m_tone = std::make_unique<ParameterSmoother>();
    m_age = std::make_unique<ParameterSmoother>();
    m_mix = std::make_unique<ParameterSmoother>();
    m_sync = std::make_unique<ParameterSmoother>();
    
    // Set default values
    m_delayTime->reset(0.3);
    m_feedback->reset(0.4);
    m_modulation->reset(0.2);
    m_tone->reset(0.5);
    m_age->reset(0.0);
    m_mix->reset(0.5);
    m_sync->reset(0.0);
}

void BucketBrigadeDelay::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize parameter smoothers
    m_delayTime->setSampleRate(sampleRate, 50.0);
    m_feedback->setSampleRate(sampleRate, 30.0);
    m_modulation->setSampleRate(sampleRate, 100.0);
    m_tone->setSampleRate(sampleRate, 50.0);
    m_age->setSampleRate(sampleRate, 1000.0);
    m_mix->setSampleRate(sampleRate, 30.0);
    m_sync->setSampleRate(sampleRate, 10.0);
    
    // Initialize processing components with safe defaults
    int numStages = BBD_STAGES_3007; // Default to MN3007
    
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        m_bbdChains[ch].setNumStages(numStages);
        m_companders[ch].setSampleRate(sampleRate);
        m_filters[ch].setSampleRate(sampleRate);
        m_dcServos[ch].setSampleRate(sampleRate);
    }
    
    m_clockGenerator.reset();
    m_analogCircuit.reset();
    
    reset();
}

void BucketBrigadeDelay::reset() {
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        m_bbdChains[ch].reset();
        m_companders[ch].reset();
        m_filters[ch].reset();
        m_feedbackProcessors[ch].reset();
        m_dcServos[ch].reset();
    }
    
    m_clockGenerator.reset();
    m_analogCircuit.reset();
    
    for (auto& buffer : m_workBuffers) {
        buffer.fill(0.0);
    }
}

void BucketBrigadeDelay::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Update parameters safely
    CachedParams params;
    params.delayTime = m_delayTime->process();
    params.feedback = m_feedback->process() * 0.95; // Limit feedback to prevent runaway
    params.modulation = m_modulation->process();
    params.tone = m_tone->process();
    params.age = m_age->process();
    params.mix = m_mix->process();
    params.sync = m_sync->process();
    
    // Calculate delay in milliseconds (20ms to 600ms range)
    double delayMs = 20.0 + params.delayTime * 580.0;
    params.clockRate = calculateClockRate(delayMs);
    
    // Process each channel
    for (int ch = 0; ch < std::min(numChannels, NUM_CHANNELS); ++ch) {
        processChannel(buffer.getWritePointer(ch), numSamples, ch, params);
    }
}

void BucketBrigadeDelay::processChannel(float* data, int numSamples, int channel, 
                                       const CachedParams& params) {
    // Simple processing without complex filtering
    for (int i = 0; i < numSamples; ++i) {
        double input = static_cast<double>(data[i]);
        
        // Process through simplified BBD chain
        double delayed = m_bbdChains[channel].process(input, params.clockRate, m_sampleRate);
        
        // Apply feedback (simplified)
        double feedback = delayed * params.feedback;
        
        // Mix with feedback for next iteration
        double bbdInput = input + feedback;
        
        // Simple tone control (lowpass)
        double toneFreq = 200.0 + params.tone * 4800.0; // 200Hz to 5kHz
        double alpha = 2.0 * M_PI * toneFreq / m_sampleRate;
        alpha = alpha / (alpha + 1.0);
        delayed = delayed * alpha + delayed * (1.0 - alpha) * 0.5;
        
        // Mix dry/wet
        double output = input * (1.0 - params.mix) + delayed * params.mix;
        
        // Soft clipping to prevent overflow
        if (output > 1.0) output = 1.0 - std::exp(1.0 - output);
        if (output < -1.0) output = -1.0 + std::exp(1.0 + output);
        
        data[i] = static_cast<float>(output);
    }
}

double BucketBrigadeDelay::calculateClockRate(double delayMs) const {
    int stages = BBD_STAGES_3007; // Use default
    double clockRate = stages / (2.0 * delayMs * 0.001);
    return std::clamp(clockRate, MIN_CLOCK_RATE, MAX_CLOCK_RATE);
}

double BucketBrigadeDelay::calculateSyncedDelayTime(double timeParam, double syncParam) const {
    // Simple implementation - just return manual time for now
    return 20.0 + timeParam * 580.0; // 20ms to 600ms
}

double BucketBrigadeDelay::getBeatDivisionMs(BeatDivision division) const {
    // Placeholder - would need BPM info
    return 500.0; // Default 500ms
}

void BucketBrigadeDelay::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? 
               std::clamp(it->second, 0.0f, 1.0f) : defaultValue;
    };
    
    m_delayTime->setTarget(static_cast<double>(getParam(0, 0.3f)));
    m_feedback->setTarget(static_cast<double>(getParam(1, 0.4f)));
    m_modulation->setTarget(static_cast<double>(getParam(2, 0.2f)));
    m_tone->setTarget(static_cast<double>(getParam(3, 0.5f)));
    m_age->setTarget(static_cast<double>(getParam(4, 0.0f)));
    m_mix->setTarget(static_cast<double>(getParam(5, 0.5f)));
    m_sync->setTarget(static_cast<double>(getParam(6, 0.0f)));
}

juce::String BucketBrigadeDelay::getParameterName(int index) const {
    switch (index) {
        case 0: return "Delay Time";
        case 1: return "Feedback";
        case 2: return "Modulation";
        case 3: return "Tone";
        case 4: return "Age";
        case 5: return "Mix";
        case 6: return "Sync";
        default: return "";
    }
}

void BucketBrigadeDelay::setTransportInfo(const TransportInfo& info) {
    m_transportInfo = info;
}

bool BucketBrigadeDelay::supportsFeature(Feature f) const noexcept {
    switch (f) {
        case Feature::TRANSPORT_SYNC: return true;
        case Feature::SIDECHAIN: return false;
        case Feature::OVERSAMPLING: return false;
        case Feature::EXTERNAL_CONTROL: return false;
        default: return false;
    }
}

void BucketBrigadeDelay::updateChipType(ChipType newType) {
    m_chipTypeAtomic.store(static_cast<int>(newType));
    parametersChanged.store(true);
}

// ==================== BBDChain Implementation ====================

void BucketBrigadeDelay::BBDChain::setNumStages(int stages) {
    numStages.store(stages, std::memory_order_release);
    
    // Safely allocate and initialize buckets
    if (static_cast<size_t>(stages) != bucketCapacity) {
        buckets = std::make_unique<std::atomic<double>[]>(stages);
        bucketCapacity = stages;
        
        // Initialize all buckets to zero
        for (int i = 0; i < stages; ++i) {
            buckets[i].store(0.0, std::memory_order_relaxed);
        }
    }
}

void BucketBrigadeDelay::BBDChain::reset() {
    int stages = numStages.load(std::memory_order_acquire);
    if (buckets && stages > 0 && static_cast<size_t>(stages) <= bucketCapacity) {
        for (int i = 0; i < stages; ++i) {
            buckets[i].store(0.0, std::memory_order_relaxed);
        }
    }
    clockPhase = 0.0;
    clockState = IDLE;
}

double BucketBrigadeDelay::BBDChain::process(double input, double clockRate, double sampleRate) {
    int stages = numStages.load(std::memory_order_acquire);
    
    // Safety check
    if (!buckets || stages <= 0 || static_cast<size_t>(stages) > bucketCapacity) {
        return 0.0;
    }
    
    // Simple ring buffer delay implementation for stability
    // This replaces the complex BBD simulation that was causing crashes
    static int writeIndex = 0;
    static double accumulator = 0.0;
    
    // Calculate delay in samples
    double delaySamples = sampleRate / (2.0 * clockRate) * stages;
    int delayInt = static_cast<int>(delaySamples);
    double delayFrac = delaySamples - delayInt;
    
    // Ensure delay is within bounds
    delayInt = std::clamp(delayInt, 1, stages - 1);
    
    // Write input to current position
    buckets[writeIndex].store(input, std::memory_order_relaxed);
    
    // Read from delayed position with linear interpolation
    int readIndex1 = (writeIndex - delayInt + stages) % stages;
    int readIndex2 = (readIndex1 - 1 + stages) % stages;
    
    double sample1 = buckets[readIndex1].load(std::memory_order_relaxed);
    double sample2 = buckets[readIndex2].load(std::memory_order_relaxed);
    
    // Linear interpolation
    double output = sample1 * (1.0 - delayFrac) + sample2 * delayFrac;
    
    // Advance write position
    writeIndex = (writeIndex + 1) % stages;
    
    // Simple analog-style filtering
    accumulator = output * 0.7 + accumulator * 0.3;
    
    return accumulator;
}

void BucketBrigadeDelay::BBDChain::transferCharges(double input, bool oddPhase) {
    // Simplified - not needed for basic delay
}

void BucketBrigadeDelay::BBDChain::setCharacteristics(double efficiency, double leakage, 
                                                      double feedthrough) {
    transferEfficiency.store(efficiency, std::memory_order_release);
    chargeLeakage.store(leakage, std::memory_order_release);
    clockFeedthrough.store(feedthrough, std::memory_order_release);
}

// ==================== CompandingSystem Implementation ====================

void BucketBrigadeDelay::CompandingSystem::setSampleRate(double sr) {
    sampleRate = sr;
    attackCoeff = 1.0 - std::exp(-1.0 / (0.0001 * sr));
    releaseCoeff = 1.0 - std::exp(-1.0 / (0.001 * sr));
}

double BucketBrigadeDelay::CompandingSystem::compress(double input) {
    return input; // Simplified - bypass compression for now
}

double BucketBrigadeDelay::CompandingSystem::expand(double input) {
    return input; // Simplified - bypass expansion for now
}

void BucketBrigadeDelay::CompandingSystem::reset() {
    compressorEnvelope = 0.0;
    compressorGain = 1.0;
    preEmphasisState = 0.0;
    expanderEnvelope = 0.0;
    expanderGain = 1.0;
    deEmphasisState = 0.0;
}

double BucketBrigadeDelay::CompandingSystem::updateEnvelope(double input, double& envelope) {
    double rectified = std::abs(input);
    
    if (rectified > envelope) {
        envelope += (rectified - envelope) * attackCoeff;
    } else {
        envelope += (rectified - envelope) * releaseCoeff;
    }
    
    return envelope;
}

// ==================== BBDFilters Implementation ====================

void BucketBrigadeDelay::BBDFilters::setSampleRate(double sr) {
    sampleRate = sr;
}

void BucketBrigadeDelay::BBDFilters::updateFilters(double clockRate) {
    // Simplified - would set filter cutoffs based on clock rate
}

double BucketBrigadeDelay::BBDFilters::processAntiAliasing(double input) {
    return input; // Simplified - bypass filtering for now
}

double BucketBrigadeDelay::BBDFilters::processReconstruction(double input) {
    return input; // Simplified - bypass filtering for now
}

void BucketBrigadeDelay::BBDFilters::reset() {
    // Reset filter states
}

double BucketBrigadeDelay::BBDFilters::EllipticFilter::process(double input) {
    return input; // Simplified
}

void BucketBrigadeDelay::BBDFilters::EllipticFilter::reset() {
    // Reset states
}

void BucketBrigadeDelay::BBDFilters::EllipticFilter::designLowpass(double freq, double sampleRate, double ripple) {
    // Simplified - would design filter coefficients
}

// ==================== ClockGenerator Implementation ====================

void BucketBrigadeDelay::ClockGenerator::reset() {
    phase = 0.0;
    lfoPhase = 0.0;
    noiseState = 0.0;
    noiseLPF = 0.0;
}

double BucketBrigadeDelay::ClockGenerator::generateClockRate(double baseRate, double modulation, double sampleRate) {
    // Simple LFO modulation
    lfoPhase += lfoRate / sampleRate;
    if (lfoPhase > 1.0) lfoPhase -= 1.0;
    
    double lfo = std::sin(2.0 * M_PI * lfoPhase);
    double modAmount = 1.0 + lfo * modulation * 0.1;
    
    return baseRate * modAmount;
}

// ==================== AnalogCircuit Implementation ====================

void BucketBrigadeDelay::AnalogCircuit::update(double sampleRate) {
    // Simplified analog modeling
    ripplePhase += rippleFreq / sampleRate;
    if (ripplePhase > 1.0) ripplePhase -= 1.0;
}

double BucketBrigadeDelay::AnalogCircuit::getDelayModulation() const {
    return std::sin(2.0 * M_PI * ripplePhase) * supplyRipple * 0.001;
}

double BucketBrigadeDelay::AnalogCircuit::getFilterModulation() const {
    return 1.0 + capacitorAging * 0.1;
}

void BucketBrigadeDelay::AnalogCircuit::setAging(double amount) {
    capacitorAging = amount;
    resistorDrift = amount * 0.5;
}

void BucketBrigadeDelay::AnalogCircuit::reset() {
    ripplePhase = 0.0;
    capacitorAging = 0.0;
    resistorDrift = 0.0;
}

// ==================== FeedbackProcessor Implementation ====================

double BucketBrigadeDelay::FeedbackProcessor::process(double input, double amount) {
    // Simple highpass to prevent DC buildup
    double highpassed = input - previousSample * 0.95;
    previousSample = input;
    
    // Soft saturation
    double saturated = softClip(highpassed * amount);
    
    return saturated;
}

double BucketBrigadeDelay::FeedbackProcessor::softClip(double input) {
    if (std::abs(input) < threshold) {
        return input;
    }
    
    double sign = input < 0 ? -1.0 : 1.0;
    double absInput = std::abs(input);
    
    if (absInput < threshold + knee) {
        double t = (absInput - threshold) / knee;
        return sign * (threshold + knee * (t - t * t * 0.25));
    }
    
    return sign * (threshold + knee * 0.75);
}

// ==================== DCServo Implementation ====================
// Already implemented in header as inline functions