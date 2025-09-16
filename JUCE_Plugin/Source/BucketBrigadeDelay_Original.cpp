// ==================== BucketBrigadeDelay.cpp ====================
#include "BucketBrigadeDelay.h"
#include "DspEngineUtilities.h"
#include <algorithm>

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
    m_sync->reset(0.0); // sync off by default
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
    m_sync->setSampleRate(sampleRate, 10.0); // Fast switching for sync
    
    // Get current chip type
    ChipType currentType = getCurrentChipType();
    int numStages = BBD_STAGES_3007;
    switch (currentType) {
        case ChipType::MN3005: numStages = BBD_STAGES_3005; break;
        case ChipType::MN3007: numStages = BBD_STAGES_3007; break;
        case ChipType::MN3008: numStages = BBD_STAGES_3008; break;
    }
    
    // Initialize processing components
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
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Check for chip type changes
    if (parametersChanged.load()) {
        std::lock_guard<std::mutex> lock(parameterMutex);
        
        ChipType currentType = getCurrentChipType();
        int numStages = BBD_STAGES_3007;
        switch (currentType) {
            case ChipType::MN3005: numStages = BBD_STAGES_3005; break;
            case ChipType::MN3007: numStages = BBD_STAGES_3007; break;
            case ChipType::MN3008: numStages = BBD_STAGES_3008; break;
        }
        
        for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
            m_bbdChains[ch].setNumStages(numStages);
        }
        
        parametersChanged.store(false);
    }
    
    // Update parameters once per block
    CachedParams params;
    double delayTimeParam = m_delayTime->process();
    double syncParam = m_sync->process();
    params.delayTime = delayTimeParam; // Store original for other uses
    params.feedback = m_feedback->process();
    params.modulation = m_modulation->process();
    params.tone = m_tone->process();
    params.age = m_age->process();
    params.mix = m_mix->process();
    params.sync = syncParam;
    
    // Calculate delay time (synced or manual)
    double delayMs = calculateSyncedDelayTime(delayTimeParam, syncParam);
    params.clockRate = calculateClockRate(delayMs);
    
    // Update analog circuit modeling
    m_analogCircuit.setAging(params.age);
    m_analogCircuit.update(m_sampleRate);
    
    // Set clock modulation
    m_clockGenerator.setLFO(0.5, params.modulation * 0.02);
    m_clockGenerator.setJitter(params.age * 0.001);
    
    // Update filter characteristics
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        m_filters[ch].updateFilters(params.clockRate * (0.5 + params.tone));
        
        double efficiency = 0.997 - params.age * 0.002;
        double leakage = 0.00001 + params.age * 0.00005;
        double feedthrough = 0.002 + params.age * 0.003;
        m_bbdChains[ch].setCharacteristics(efficiency, leakage, feedthrough);
    }
    
    // Process each channel
    for (int ch = 0; ch < std::min(numChannels, NUM_CHANNELS); ++ch) {
        processChannel(buffer.getWritePointer(ch), numSamples, ch, params);
    }
}

void BucketBrigadeDelay::processChannel(float* data, int numSamples, int channel, 
                                       const CachedParams& params) {
    double* workBuffer = m_workBuffers[channel].data();
    
    // Convert to double
    for (int i = 0; i < numSamples; ++i) {
        workBuffer[i] = static_cast<double>(data[i]);
    }
    
    // Process through BBD chain
    for (int i = 0; i < numSamples; ++i) {
        double input = workBuffer[i];
        
        // DC servo to remove offset
        input = m_dcServos[channel].process(input);
        
        // Anti-aliasing filter
        double filtered = m_filters[channel].processAntiAliasing(input);
        
        // Compression stage
        double compressed = m_companders[channel].compress(filtered);
        
        // Generate modulated clock rate
        double clockMod = m_analogCircuit.getDelayModulation();
        double modulatedClock = m_clockGenerator.generateClockRate(
            params.clockRate, params.modulation + clockMod, m_sampleRate
        );
        
        // Get current BBD output for feedback
        double bbdOut = m_bbdChains[channel].process(compressed, modulatedClock, m_sampleRate);
        
        // Process feedback
        double feedback = m_feedbackProcessors[channel].process(bbdOut, params.feedback);
        
        // Mix feedback with compressed input
        double bbdInput = compressed + feedback * 0.7;
        
        // Process through BBD with feedback
        bbdOut = m_bbdChains[channel].process(bbdInput, modulatedClock, m_sampleRate);
        
        // Expansion stage
        double expanded = m_companders[channel].expand(bbdOut);
        
        // Reconstruction filter
        double reconstructed = m_filters[channel].processReconstruction(expanded);
        
        // Mix dry/wet
        workBuffer[i] = input * (1.0 - params.mix) + reconstructed * params.mix;
    }
    
    // Convert back to float
    for (int i = 0; i < numSamples; ++i) {
        data[i] = static_cast<float>(workBuffer[i]);
    }
}

double BucketBrigadeDelay::calculateClockRate(double delayMs) const {
    ChipType currentType = getCurrentChipType();
    int stages = BBD_STAGES_3007;
    switch (currentType) {
        case ChipType::MN3005: stages = BBD_STAGES_3005; break;
        case ChipType::MN3007: stages = BBD_STAGES_3007; break;
        case ChipType::MN3008: stages = BBD_STAGES_3008; break;
    }
    
    double clockRate = stages / (2.0 * delayMs * 0.001);
    return std::clamp(clockRate, MIN_CLOCK_RATE, MAX_CLOCK_RATE);
}

void BucketBrigadeDelay::updateChipType(ChipType newType) {
    int newTypeInt = static_cast<int>(newType);
    if (m_chipTypeAtomic.load() != newTypeInt) {
        m_chipTypeAtomic.store(newTypeInt);
        parametersChanged.store(true);
    }
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

// ==================== BBDChain Implementation ====================

void BucketBrigadeDelay::BBDChain::setNumStages(int stages) {
    numStages.store(stages, std::memory_order_release);
    
    // Allocate new atomic array if needed
    if (static_cast<size_t>(stages) > bucketCapacity) {
        buckets = std::make_unique<std::atomic<double>[]>(stages);
        bucketCapacity = stages;
    }
    
    // Initialize atomic buckets to zero
    for (int i = 0; i < stages; ++i) {
        buckets[i].store(0.0, std::memory_order_relaxed);
    }
}

void BucketBrigadeDelay::BBDChain::reset() {
    // Thread-safe reset of atomic buckets
    int stages = numStages.load(std::memory_order_acquire);
    if (buckets && stages > 0) {
        for (int i = 0; i < stages; ++i) {
            buckets[i].store(0.0, std::memory_order_relaxed);
        }
    }
    clockPhase = 0.0;
    clockState = IDLE;
}

double BucketBrigadeDelay::BBDChain::process(double input, double clockRate, double sampleRate) {
    // Update clock phase
    double clockIncrement = clockRate / sampleRate;
    clockPhase += clockIncrement;
    
    // Non-overlapping two-phase clock with dead time
    double phasePosition = std::fmod(clockPhase, 1.0);
    
    switch (clockState) {
        case IDLE:
            if (phasePosition >= 0.0 && phasePosition < 0.5 - DEAD_TIME_RATIO) {
                clockState = PHASE1;
            }
            break;
            
        case PHASE1:
            if (phasePosition >= 0.5 - DEAD_TIME_RATIO) {
                clockState = DEAD_TIME;
                transferCharges(input, true);
            }
            break;
            
        case DEAD_TIME:
            if (phasePosition >= 0.5) {
                clockState = PHASE2;
            }
            break;
            
        case PHASE2:
            if (phasePosition >= 1.0 - DEAD_TIME_RATIO || phasePosition < 0.0) {
                clockState = IDLE;
                transferCharges(input, false);
                clockPhase = std::fmod(clockPhase, 1.0);
            }
            break;
    }
    
    // Output is from the last stage (thread-safe atomic read)
    int stages = numStages.load(std::memory_order_acquire);
    if (buckets && stages > 0 && static_cast<size_t>(stages) <= bucketCapacity) {
        return buckets[stages - 1].load(std::memory_order_acquire);
    }
    return 0.0;  // Safety fallback
}

void BucketBrigadeDelay::BBDChain::transferCharges(double input, bool oddPhase) {
    /*
     * THREAD SAFETY IMPLEMENTATION:
     * 
     * This method implements lock-free thread safety for real-time audio processing:
     * 
     * 1. ATOMIC LOADS: All parameters are loaded atomically with acquire semantics
     *    to ensure consistent values throughout the transfer operation.
     * 
     * 2. MEMORY ORDERING: 
     *    - acquire: Ensures no memory operations are reordered before the load
     *    - release: Ensures no memory operations are reordered after the store
     *    - This prevents race conditions and ensures visibility across threads
     * 
     * 3. BUCKET ACCESS: Each bucket is an atomic<double> with proper memory ordering
     *    to prevent data races during concurrent read/write operations.
     * 
     * 4. PROCESSING ORDER: Odd/even stages are processed separately in two-phase
     *    clocking to maintain BBD authenticity while ensuring thread safety.
     * 
     * This approach eliminates clicking artifacts and inconsistent delay behavior
     * while maintaining real-time performance without locks or mutexes.
     */
    
    // Load characteristics atomically once for consistency during transfer
    const double efficiency = transferEfficiency.load(std::memory_order_acquire);
    const double leakage = chargeLeakage.load(std::memory_order_acquire);
    const double feedthrough = clockFeedthrough.load(std::memory_order_acquire);
    const double inputCap = inputCapacitance.load(std::memory_order_acquire);
    const int stages = numStages.load(std::memory_order_acquire);
    
    // THREAD SAFETY: Bounds checking to prevent array access issues
    if (!buckets || stages <= 0 || static_cast<size_t>(stages) > bucketCapacity) {
        return;  // Safety exit if inconsistent state
    }
    
    if (oddPhase) {
        // Transfer odd stages - process from end to beginning to avoid dependency issues
        for (int i = stages - 1; i > 0; i -= 2) {
            // THREAD SAFETY: Atomic read operations with acquire semantics
            // This ensures that we see the most recent writes from other threads
            double prevValue = buckets[i-1].load(std::memory_order_acquire);
            double currentValue = buckets[i].load(std::memory_order_acquire);
            
            // Calculate new value using consistent parameter values
            double newValue = prevValue * efficiency;
            newValue *= (1.0 - leakage);
            newValue += feedthrough * (input - currentValue);
            
            // THREAD SAFETY: Atomic write with release semantics
            // This ensures our write is visible to other threads immediately
            buckets[i].store(newValue, std::memory_order_release);
        }
    } else {
        // Transfer even stages - process from end to beginning 
        for (int i = stages - 2; i >= 0; i -= 2) {
            if (i > 0) {
                // THREAD SAFETY: Atomic read operations with acquire semantics
                double prevValue = buckets[i-1].load(std::memory_order_acquire);
                double currentValue = buckets[i].load(std::memory_order_acquire);
                
                // Calculate new value using consistent parameter values
                double newValue = prevValue * efficiency;
                newValue *= (1.0 - leakage);
                newValue += feedthrough * (input - currentValue);
                
                // THREAD SAFETY: Atomic write with release semantics
                buckets[i].store(newValue, std::memory_order_release);
            }
        }
        
        // THREAD SAFETY: Input stage atomic update with proper memory ordering
        // This ensures thread-safe access to the first bucket stage
        double currentInput = buckets[0].load(std::memory_order_acquire);
        double newInput = input * inputCap + currentInput * (1.0 - inputCap);
        buckets[0].store(newInput, std::memory_order_release);
    }
}

void BucketBrigadeDelay::BBDChain::setCharacteristics(double efficiency, double leakage, 
                                                      double feedthrough) {
    // Thread-safe atomic updates with memory ordering for parameter changes
    transferEfficiency.store(std::clamp(efficiency, 0.9, 0.999), std::memory_order_release);
    chargeLeakage.store(std::clamp(leakage, 0.0, 0.001), std::memory_order_release);
    clockFeedthrough.store(std::clamp(feedthrough, 0.0, 0.01), std::memory_order_release);
}

// ==================== CompandingSystem Implementation ====================

void BucketBrigadeDelay::CompandingSystem::setSampleRate(double sr) {
    sampleRate = sr;
    attackCoeff = 1.0 - std::exp(-1.0 / (attackTime * sr));
    releaseCoeff = 1.0 - std::exp(-1.0 / (releaseTime * sr));
}

double BucketBrigadeDelay::CompandingSystem::compress(double input) {
    // Pre-emphasis with correct sample rate
    double emphasisCutoff = 2.0 * M_PI * emphasisFreq / sampleRate;
    double alpha = emphasisCutoff / (emphasisCutoff + 1.0);
    
    // First-order highpass for pre-emphasis
    double highpassed = input - preEmphasisState;
    preEmphasisState += highpassed * alpha;
    double emphasized = input + highpassed * 0.5;  // Boost high frequencies
    
    // Update envelope
    double envelope = updateEnvelope(emphasized, compressorEnvelope);
    
    // Calculate 2:1 compression
    double inputdB = linearTodB(envelope);
    double outputdB = inputdB;
    
    if (inputdB > -20.0) {
        outputdB = -20.0 + (inputdB + 20.0) * 0.5;
    }
    
    double gainReduction = dBtoLinear(outputdB - inputdB);
    compressorGain = gainReduction;
    
    return emphasized * compressorGain;
}

double BucketBrigadeDelay::CompandingSystem::expand(double input) {
    // Update envelope
    double envelope = updateEnvelope(input, expanderEnvelope);
    
    // Calculate 1:2 expansion
    double inputdB = linearTodB(envelope);
    double outputdB = inputdB;
    
    if (inputdB > -20.0) {
        outputdB = -20.0 + (inputdB + 20.0) * 2.0;
    }
    
    double gainIncrease = dBtoLinear(outputdB - inputdB);
    expanderGain = gainIncrease;
    
    double expanded = input * expanderGain;
    
    // De-emphasis with correct sample rate
    double deemphasisCutoff = 2.0 * M_PI * emphasisFreq / sampleRate;
    double beta = deemphasisCutoff / (deemphasisCutoff + 1.0);
    
    // First-order lowpass for de-emphasis
    deEmphasisState += beta * (expanded - deEmphasisState);
    
    return deEmphasisState;
}

double BucketBrigadeDelay::CompandingSystem::updateEnvelope(double input, double& envelope) {
    double rectified = std::abs(input);
    
    if (rectified > envelope) {
        envelope += (rectified - envelope) * attackCoeff;
    } else {
        envelope += (rectified - envelope) * releaseCoeff;
    }
    
    envelope += DENORMAL_PREVENTION;
    envelope -= DENORMAL_PREVENTION;
    
    return envelope;
}

void BucketBrigadeDelay::CompandingSystem::reset() {
    compressorEnvelope = 0.0;
    compressorGain = 1.0;
    preEmphasisState = 0.0;
    expanderEnvelope = 0.0;
    expanderGain = 1.0;
    deEmphasisState = 0.0;
}

// ==================== BBDFilters Implementation ====================

double BucketBrigadeDelay::BBDFilters::EllipticFilter::Biquad::process(double input) {
    double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = input;
    y2 = y1; y1 = output;
    return output;
}

void BucketBrigadeDelay::BBDFilters::EllipticFilter::designLowpass(double freq, 
                                                                   double sampleRate, 
                                                                   double ripple) {
    double omega = 2.0 * M_PI * freq / sampleRate;
    double Q = 2.0;
    
    for (auto& stage : stages) {
        double cos_omega = std::cos(omega);
        double sin_omega = std::sin(omega);
        double alpha = sin_omega / (2.0 * Q);
        
        double a0 = 1.0 + alpha;
        stage.b0 = (1.0 - cos_omega) / 2.0 / a0;
        stage.b1 = (1.0 - cos_omega) / a0;
        stage.b2 = (1.0 - cos_omega) / 2.0 / a0;
        stage.a1 = -2.0 * cos_omega / a0;
        stage.a2 = (1.0 - alpha) / a0;
        
        Q *= 0.7;
    }
}

double BucketBrigadeDelay::BBDFilters::EllipticFilter::process(double input) {
    double output = input;
    for (auto& stage : stages) {
        output = stage.process(output);
    }
    return output;
}

void BucketBrigadeDelay::BBDFilters::EllipticFilter::reset() {
    for (auto& stage : stages) {
        stage.reset();
    }
}

void BucketBrigadeDelay::BBDFilters::setSampleRate(double sr) {
    sampleRate = sr;
    updateFilters(50000.0);
}

void BucketBrigadeDelay::BBDFilters::updateFilters(double clockRate) {
    double bbdSampleRate = clockRate / 2.0;
    double cutoffFreq = bbdSampleRate * 0.4;
    
    cutoffFreq = std::min(cutoffFreq, 15000.0);
    cutoffFreq = std::max(cutoffFreq, 1000.0);
    
    antiAliasingFilter.designLowpass(cutoffFreq, sampleRate);
    reconstructionFilter.designLowpass(cutoffFreq * 0.9, sampleRate);
}

double BucketBrigadeDelay::BBDFilters::processAntiAliasing(double input) {
    return antiAliasingFilter.process(input);
}

double BucketBrigadeDelay::BBDFilters::processReconstruction(double input) {
    return reconstructionFilter.process(input);
}

void BucketBrigadeDelay::BBDFilters::reset() {
    antiAliasingFilter.reset();
    reconstructionFilter.reset();
}

// ==================== ClockGenerator Implementation ====================

void BucketBrigadeDelay::ClockGenerator::reset() {
    phase = 0.0;
    lfoPhase = 0.0;
    noiseState = 0.0;
    noiseLPF = 0.0;
}

double BucketBrigadeDelay::ClockGenerator::generateClockRate(double baseRate, 
                                                            double modulation, 
                                                            double sampleRate) {
    // LFO modulation
    lfoPhase += lfoRate / sampleRate;
    if (lfoPhase >= 1.0) lfoPhase -= 1.0;
    
    double lfoValue = std::sin(2.0 * M_PI * lfoPhase) * lfoDepth;
    
    // Clock jitter
    if (jitterAmount > 0.0) {
        double noise = distribution(rng) * jitterAmount;
        noiseLPF += (noise - noiseLPF) * 0.1;
        noiseState = noiseLPF;
    }
    
    double totalModulation = lfoValue + noiseState * 0.001;
    double modulatedRate = baseRate * (1.0 + totalModulation);
    
    return std::clamp(modulatedRate, MIN_CLOCK_RATE, MAX_CLOCK_RATE);
}

// ==================== AnalogCircuit Implementation ====================

void BucketBrigadeDelay::AnalogCircuit::update(double sampleRate) {
    // Supply voltage ripple
    ripplePhase += rippleFreq / sampleRate;
    if (ripplePhase >= 1.0) ripplePhase -= 1.0;
    supplyRipple = std::sin(2.0 * M_PI * ripplePhase) * 0.01;
    
    // Temperature drift
    temperature += tempDist(rng) * 0.0001;
    temperature = std::clamp(temperature, 15.0, 35.0);
}

double BucketBrigadeDelay::AnalogCircuit::getDelayModulation() const {
    double tempEffect = (temperature - 25.0) * tempCoefficient;
    double supplyEffect = supplyRipple * 0.5;
    double agingEffect = capacitorAging * 0.01;
    
    return tempEffect + supplyEffect + agingEffect;
}

double BucketBrigadeDelay::AnalogCircuit::getFilterModulation() const {
    return resistorDrift * 0.1 + capacitorAging * 0.05;
}

void BucketBrigadeDelay::AnalogCircuit::setAging(double amount) {
    capacitorAging = amount;
    resistorDrift = amount * 0.5;
}

void BucketBrigadeDelay::AnalogCircuit::reset() {
    supplyVoltage = 9.0;
    supplyRipple = 0.0;
    ripplePhase = 0.0;
    temperature = 25.0;
    capacitorAging = 0.0;
    resistorDrift = 0.0;
}

// ==================== FeedbackProcessor Implementation ====================

double BucketBrigadeDelay::FeedbackProcessor::process(double input, double amount) {
    // Highpass filter to prevent low frequency buildup
    double hpCutoff = 0.01;
    highpassState += hpCutoff * (input - highpassState);
    double filtered = input - highpassState;
    
    double feedback = filtered * amount;
    feedback = softClip(feedback);
    
    previousSample = feedback;
    
    return feedback;
}

double BucketBrigadeDelay::FeedbackProcessor::softClip(double input) {
    double absInput = std::abs(input);
    
    if (absInput < threshold - knee) {
        return input;
    } else if (absInput < threshold + knee) {
        double kneePosition = (absInput - threshold + knee) / (2.0 * knee);
        double gain = 1.0 - kneePosition * kneePosition * 0.25;
        return input * gain;
    } else {
        double sign = input > 0 ? 1.0 : -1.0;
        return sign * (threshold + knee * 0.75 + std::tanh((absInput - threshold - knee) * 2.0) * 0.1);
    }
}

// ==================== Transport Sync Implementation ====================

void BucketBrigadeDelay::setTransportInfo(const TransportInfo& info) {
    m_transportInfo = info;
}

bool BucketBrigadeDelay::supportsFeature(Feature f) const noexcept {
    switch (f) {
        case Feature::TempoSync: return true;
        default: return false;
    }
}

double BucketBrigadeDelay::calculateSyncedDelayTime(double timeParam, double syncParam) const {
    // Sync is off if syncParam < 0.5, use manual time
    if (syncParam < 0.5) {
        double minDelay = 2.5;  // 2.5ms min
        double maxDelay = 300.0; // 300ms max
        return minDelay + timeParam * (maxDelay - minDelay);
    }
    
    // Sync is on, map timeParam to beat divisions
    const int divisionIndex = static_cast<int>(timeParam * 8.999); // 0-8 range
    const BeatDivision division = static_cast<BeatDivision>(divisionIndex);
    
    return getBeatDivisionMs(division);
}

double BucketBrigadeDelay::getBeatDivisionMs(BeatDivision division) const {
    const double bpm = std::max(20.0, std::min(999.0, m_transportInfo.bpm));
    const double quarterNoteMs = (60.0 / bpm) * 1000.0; // ms per quarter note
    
    switch (division) {
        case BeatDivision::DIV_1_64: return quarterNoteMs / 16.0;
        case BeatDivision::DIV_1_32: return quarterNoteMs / 8.0;
        case BeatDivision::DIV_1_16: return quarterNoteMs / 4.0;
        case BeatDivision::DIV_1_8:  return quarterNoteMs / 2.0;
        case BeatDivision::DIV_1_4:  return quarterNoteMs;
        case BeatDivision::DIV_1_2:  return quarterNoteMs * 2.0;
        case BeatDivision::DIV_1_1:  return quarterNoteMs * 4.0;
        case BeatDivision::DIV_2_1:  return quarterNoteMs * 8.0;
        case BeatDivision::DIV_4_1:  return quarterNoteMs * 16.0;
        default: return quarterNoteMs;
    }
}