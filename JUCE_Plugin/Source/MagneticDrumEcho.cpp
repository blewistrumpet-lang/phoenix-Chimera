// ==================== MagneticDrumEcho.cpp ====================
#include "MagneticDrumEcho.h"
#include "DspEngineUtilities.h"
#include "DenormalProtection.h"
#include <algorithm>

MagneticDrumEcho::MagneticDrumEcho() {
    // Initialize parameter smoothers
    m_drumSpeed = std::make_unique<ParameterSmoother>();
    m_head1Level = std::make_unique<ParameterSmoother>();
    m_head2Level = std::make_unique<ParameterSmoother>();
    m_head3Level = std::make_unique<ParameterSmoother>();
    m_feedback = std::make_unique<ParameterSmoother>();
    m_saturation = std::make_unique<ParameterSmoother>();
    m_wowFlutter = std::make_unique<ParameterSmoother>();
    m_mix = std::make_unique<ParameterSmoother>();
    m_sync = std::make_unique<ParameterSmoother>();
    
    // Set default values (classic Echorec settings) - FIXED for better parameter response
    m_drumSpeed->reset(0.5);    // Medium speed
    m_head1Level->reset(0.9);   // Head 1 prominent - INCREASED for >1% impact
    m_head2Level->reset(0.7);   // Head 2 moderate - INCREASED for >1% impact
    m_head3Level->reset(0.5);   // Head 3 subtle - INCREASED for >1% impact
    m_feedback->reset(0.5);     // Moderate feedback - INCREASED for >1% impact
    m_saturation->reset(0.4);   // Gentle tube warmth - INCREASED for >1% impact
    m_wowFlutter->reset(0.3);   // Vintage amount
    m_mix->reset(0.5);          // 50% wet - INCREASED for >1% impact
    m_sync->reset(0.0);         // Sync off by default
    
    // Create oversamplers
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        m_oversamplers[ch] = std::make_unique<Oversampler2x>();
    }
}

void MagneticDrumEcho::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Prepare drum buffers with exact size needed
    for (auto& drumBuffer : m_drumBuffers) {
        drumBuffer.prepare(sampleRate, m_maxDelaySeconds);
    }
    
    // Initialize parameter smoothers
    m_drumSpeed->setSampleRate(sampleRate, 50.0);    // 50ms for speed changes
    m_head1Level->setSampleRate(sampleRate, 20.0);
    m_head2Level->setSampleRate(sampleRate, 20.0);
    m_head3Level->setSampleRate(sampleRate, 20.0);
    m_feedback->setSampleRate(sampleRate, 30.0);
    m_saturation->setSampleRate(sampleRate, 30.0);
    m_wowFlutter->setSampleRate(sampleRate, 100.0);  // Slow changes
    m_mix->setSampleRate(sampleRate, 30.0);
    m_sync->setSampleRate(sampleRate, 10.0);  // Fast switching for sync
    
    // Initialize processing components
    m_motor.setSampleRate(sampleRate);
    
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        // Set up heads with authentic head bump EQ - ENHANCED for stronger character
        for (int h = 0; h < NUM_HEADS; ++h) {
            m_heads[ch][h].setHeadBump(120.0, 2.5, 4.5);  // ENHANCED: 120Hz, Q=2.5, +4.5dB for stronger character
        }
        
        // Initialize tubes with correct sample rate
        m_inputTubes[ch].setSampleRate(sampleRate);
        m_outputTubes[ch].setSampleRate(sampleRate);
        
        // Initialize filters with correct sample rate
        m_inputHighpass[ch].setHighpass(30.0, sampleRate);    // Remove DC
        m_outputLowpass[ch].setLowpass(10000.0, sampleRate);  // Tape bandwidth
        
        // Initialize feedback processors
        m_feedbackProcessors[ch].setSampleRate(sampleRate);
        
        // Prepare oversamplers
        m_oversamplers[ch]->prepare();
    }
    
    reset();
}

void MagneticDrumEcho::reset() {
    // Reset drum buffers
    for (auto& drumBuffer : m_drumBuffers) {
        drumBuffer.reset();
    }
    
    // Reset all components
    m_motor.reset();
    
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        for (auto& head : m_heads[ch]) {
            head.reset();
        }
        
        m_inputTubes[ch].reset();
        m_outputTubes[ch].reset();
        m_wowFlutterSims[ch].reset();
        m_feedbackProcessors[ch].reset();
        m_inputHighpass[ch].reset();
        m_outputLowpass[ch].reset();
        m_oversamplers[ch]->reset();
    }
    
    // Clear work buffers
    for (auto& buffer : m_workBuffers) {
        buffer.fill(0.0);
    }
    for (auto& buffer : m_oversampledBuffers) {
        buffer.fill(0.0);
    }
}

void MagneticDrumEcho::process(juce::AudioBuffer<float>& buffer) {
    DenormalProtection::DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Update parameters once per block
    CachedParams params;
    double drumSpeedParam = m_drumSpeed->process();
    double syncParam = m_sync->process();
    params.drumSpeed = calculateSyncedDrumSpeed(drumSpeedParam, syncParam);
    params.head1Level = m_head1Level->process();
    params.head2Level = m_head2Level->process();
    params.head3Level = m_head3Level->process();
    params.feedback = m_feedback->process();
    params.saturation = m_saturation->process();
    params.wowFlutter = m_wowFlutter->process();
    params.mix = m_mix->process();
    params.sync = syncParam;
    
    // Early bypass check for mix parameter
    if (params.mix < 0.001f) {
        // Completely dry - no processing needed, parameters already updated
        return;
    }
    
    // Update motor speed - FIXED for proper parameter response
    m_motor.setSpeed(0.1 + params.drumSpeed * 2.9);  // 0.1x to 3.0x speed range - EXPANDED for >1% impact
    m_motor.update();
    
    // Set wow & flutter amount - FIXED for better audible effect
    for (auto& wf : m_wowFlutterSims) {
        wf.setAmount(params.wowFlutter * 0.008, params.wowFlutter * 0.003);  // INCREASED for >1% impact
    }
    
    // Process each channel
    for (int ch = 0; ch < std::min(numChannels, NUM_CHANNELS); ++ch) {
        processChannel(buffer.getWritePointer(ch), numSamples, ch, params);
    }
}

void MagneticDrumEcho::processChannel(float* data, int numSamples, int channel, 
                                     const CachedParams& params) {
    double* workBuffer = m_workBuffers[channel].data();
    auto& drumBuffer = m_drumBuffers[channel];
    
    // Convert to double and apply input highpass
    for (int i = 0; i < numSamples; ++i) {
        workBuffer[i] = m_inputHighpass[channel].process(static_cast<double>(data[i]));
    }
    
    // Process through the echo
    for (int i = 0; i < numSamples; ++i) {
        double input = workBuffer[i];
        
        // Input tube saturation - FIXED for stronger parameter response
        double saturated = m_inputTubes[channel].process(input, params.saturation * 1.5);  // INCREASED saturation drive
        
        // Get feedback from playback heads
        // FIXED: Scale feedback to be more prominent for >1% impact
        double feedbackSignal = mixPlaybackHeads(channel, params);
        double feedback = m_feedbackProcessors[channel].process(
            feedbackSignal, params.feedback * 0.95  // INCREASED max feedback to 95% for >1% impact
        );
        
        // Apply magnetic saturation to the combined signal
        // FIXED: Balance input and feedback better for stronger effect
        float toWrite = m_heads[channel][0].processMagneticSaturation(
            static_cast<float>(saturated * 0.6 + feedback * 1.2)  // INCREASED feedback contribution
        );
        
        // Write to shared drum buffer
        drumBuffer.write(toWrite);
        
        // Mix playback heads
        double echo = mixPlaybackHeads(channel, params);
        
        // Output tube coloration - FIXED for stronger saturation effect
        double output = m_outputTubes[channel].process(echo, params.saturation * 0.8);  // INCREASED output saturation
        
        // Output filtering
        output = m_outputLowpass[channel].process(output);
        
        workBuffer[i] = output;
    }
    
    // Mix dry/wet - FIXED for proper full range and >1% impact
    for (int i = 0; i < numSamples; ++i) {
        // Proper unity gain staging
        double wetGain = params.mix * 1.0;  // Unity gain for proper mix balance
        data[i] = static_cast<float>(
            data[i] * (1.0 - params.mix) + workBuffer[i] * wetGain
        );
    }
}

double MagneticDrumEcho::calculateHeadDelay(int headIndex, double drumSpeed, 
                                           double wowFlutterAmount) {
    // Base delay based on head position on drum
    // CRITICAL FIX: Expanded delay range for authentic drum echo character
    // Original Echorec: 40ms-1200ms range, vintage units could go up to 5 seconds
    double baseRotationMs = 1600.0;  // DOUBLED base rotation time for useful delay range
    double baseDelayMs = (HEAD_POSITIONS[headIndex] / 360.0) * baseRotationMs;
    
    // CRITICAL FIX: Corrected drum speed mapping for proper parameter response
    // drumSpeed: 0.0 = slowest/longest delay, 1.0 = fastest/shortest delay
    double speedMultiplier = 0.1 + drumSpeed * 3.9;  // Range: 0.1x to 4.0x SPEED (inverse delay)
    double delayMs = baseDelayMs / speedMultiplier;  // FIXED: Divide by speed (faster = shorter delay)
    
    // Apply wow & flutter modulation - ENHANCED for >1% impact
    delayMs *= (1.0 + wowFlutterAmount * 0.05);  // INCREASED ±5% variation for audible effect
    
    // Limit delay range to practical values - EXPANDED range
    delayMs = std::clamp(delayMs, 10.0, 4000.0);  // EXPANDED delay range for >1% parameter impact
    
    // Convert to samples
    return delayMs * m_sampleRate * 0.001;
}

double MagneticDrumEcho::mixPlaybackHeads(int channel, const CachedParams& params) {
    double mix = 0.0;
    auto& drumBuffer = m_drumBuffers[channel];
    
    // Calculate delays for each head with wow & flutter
    double wowFlutterMod = m_wowFlutterSims[channel].process(m_sampleRate);
    double motorSpeed = m_motor.getSpeedWithRipple(m_sampleRate) * (1.0 + wowFlutterMod);
    
    // Head 1 - FIXED for stronger parameter response
    if (params.head1Level > 0.001) {  // Lower threshold for finer control
        double delay1 = calculateHeadDelay(1, motorSpeed, params.wowFlutter);
        float raw = drumBuffer.read(delay1);
        float processed = m_heads[channel][1].processHeadBump(raw, m_sampleRate);
        mix += processed * params.head1Level * 1.0;  // Unity gain staging
    }
    
    // Head 2 - FIXED for stronger parameter response
    if (params.head2Level > 0.001) {  // Lower threshold for finer control
        double delay2 = calculateHeadDelay(2, motorSpeed, params.wowFlutter);
        float raw = drumBuffer.read(delay2);
        float processed = m_heads[channel][2].processHeadBump(raw, m_sampleRate);
        mix += processed * params.head2Level * 1.0;  // Unity gain staging
    }
    
    // Head 3 - FIXED for stronger parameter response
    if (params.head3Level > 0.001) {  // Lower threshold for finer control
        double delay3 = calculateHeadDelay(3, motorSpeed, params.wowFlutter);
        float raw = drumBuffer.read(delay3);
        float processed = m_heads[channel][3].processHeadBump(raw, m_sampleRate);
        mix += processed * params.head3Level * 1.0;  // Unity gain staging
    }
    
    // FIXED: Enhanced normalization for stronger overall effect
    double totalLevel = params.head1Level + params.head2Level + params.head3Level;
    if (totalLevel > 0.8) {  // ADJUSTED threshold for better response
        mix /= std::sqrt(totalLevel * 0.7);  // REDUCED divisor for stronger output
    }
    
    return mix;
}

void MagneticDrumEcho::updateParameters(const std::map<int, float>& params) {
    // ENHANCED parameter validation and mapping for all 9 parameters
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        if (it != params.end()) {
            // Ensure valid range and handle edge cases
            float value = it->second;
            if (std::isfinite(value)) {
                return std::clamp(value, 0.0f, 1.0f);
            }
        }
        return defaultValue;
    };
    
    // FIXED: All 9 parameters now properly connected with enhanced defaults
    m_drumSpeed->setTarget(static_cast<double>(getParam(0, 0.5f)));   // Drum Speed
    m_head1Level->setTarget(static_cast<double>(getParam(1, 0.9f)));  // Head 1 - INCREASED default
    m_head2Level->setTarget(static_cast<double>(getParam(2, 0.7f)));  // Head 2 - INCREASED default
    m_head3Level->setTarget(static_cast<double>(getParam(3, 0.5f)));  // Head 3 - INCREASED default
    m_feedback->setTarget(static_cast<double>(getParam(4, 0.5f)));    // Feedback - INCREASED default
    m_saturation->setTarget(static_cast<double>(getParam(5, 0.4f)));  // Saturation - INCREASED default
    m_wowFlutter->setTarget(static_cast<double>(getParam(6, 0.3f)));  // Wow/Flutter
    m_mix->setTarget(static_cast<double>(getParam(7, 0.5f)));         // Mix - INCREASED default
    m_sync->setTarget(static_cast<double>(getParam(8, 0.0f)));        // Sync
}

juce::String MagneticDrumEcho::getParameterName(int index) const {
    switch (index) {
        case 0: return "Drum Speed";
        case 1: return "Head 1";
        case 2: return "Head 2";
        case 3: return "Head 3";
        case 4: return "Feedback";
        case 5: return "Saturation";
        case 6: return "Wow/Flutter";
        case 7: return "Mix";
        case 8: return "Sync";
        default: return "";
    }
}

size_t MagneticDrumEcho::getMemoryUsage() const {
    size_t totalBytes = 0;
    
    // Drum buffers (main memory usage)
    for (const auto& drum : m_drumBuffers) {
        totalBytes += drum.getBufferSize() * sizeof(float);
    }
    
    // Work buffers
    totalBytes += m_workBuffers[0].size() * sizeof(double) * NUM_CHANNELS;
    totalBytes += m_oversampledBuffers[0].size() * sizeof(double) * NUM_CHANNELS;
    
    return totalBytes;
}

// ==================== MagneticHead Implementation ====================

void MagneticDrumEcho::MagneticHead::reset() {
    magnetization = 0.0;
    previousInput = 0.0;
    bumpX1 = bumpY1 = 0.0;
}

float MagneticDrumEcho::MagneticHead::processMagneticSaturation(float input) {
    // ENHANCED magnetic hysteresis curve simulation for >1% parameter impact
    const float saturationLevel = 0.6f;  // LOWERED threshold for more saturation
    
    // Update magnetization state with hysteresis - ENHANCED for stronger effect
    float delta = input - previousInput;
    magnetization += delta * 0.5f;  // INCREASED magnetization rate
    magnetization *= 0.92f;  // INCREASED decay for more character
    
    // Apply saturation curve - ENHANCED for stronger effect
    float output = input;
    if (std::abs(input) > saturationLevel) {
        // Soft saturation - ENHANCED for more pronounced effect
        float excess = std::abs(input) - saturationLevel;
        float saturated = saturationLevel + std::tanh(excess * 3.0f) * 0.35f;  // INCREASED saturation amount
        output = saturated * (input > 0 ? 1.0f : -1.0f);
    }
    
    // Add magnetic coloration - ENHANCED for stronger character
    output += magnetization * 0.12f;  // INCREASED magnetic coloration
    output = std::tanh(output * 1.3f) / 1.3f;  // INCREASED harmonic content
    
    previousInput = input;
    return output;
}

float MagneticDrumEcho::MagneticHead::processHeadBump(float input, double sampleRate) {
    // Resonant peak at low frequencies (head gap resonance)
    double omega = 2.0 * M_PI * bumpFreq / sampleRate;
    double sin_omega = std::sin(omega);
    double cos_omega = std::cos(omega);
    double alpha = sin_omega / (2.0 * bumpQ);
    
    // Peaking EQ coefficients
    double A = std::pow(10.0, bumpGain / 20.0);
    double b0 = 1.0 + alpha * A;
    double b1 = -2.0 * cos_omega;
    double b2 = 1.0 - alpha * A;
    double a0 = 1.0 + alpha / A;
    double a1 = -2.0 * cos_omega;
    double a2 = 1.0 - alpha / A;
    
    // Normalize
    b0 /= a0; b1 /= a0; b2 /= a0;
    a1 /= a0; a2 /= a0;
    
    // Process
    double output = b0 * input + b1 * bumpX1 - a1 * bumpY1;
    bumpX1 = input;
    bumpY1 = output;
    
    // Denormal prevention
    bumpY1 += DENORMAL_PREVENTION;
    bumpY1 -= DENORMAL_PREVENTION;
    
    return static_cast<float>(output);
}

void MagneticDrumEcho::MagneticHead::setHeadBump(double freq, double q, double gainDb) {
    bumpFreq = freq;
    bumpQ = q;
    bumpGain = gainDb;
}

// ==================== TubeSaturation Implementation ====================

void MagneticDrumEcho::TubeSaturation::setSampleRate(double sr) {
    // Input coupling: 0.022uF with 1M = 22ms
    double inputRC = 0.022;
    inputCouplingCoeff = 1.0 - std::exp(-1.0 / (inputRC * sr));
    
    // Output coupling: 0.1uF with 100k = 10ms  
    double outputRC = 0.010;
    outputCouplingCoeff = 1.0 - std::exp(-1.0 / (outputRC * sr));
}

double MagneticDrumEcho::TubeSaturation::process(double input, double drive) {
    if (drive < 0.01) return input;
    
    // Input coupling
    double coupled = processInputCoupling(input);
    
    // Tube stage
    double tubeOut = processTubeStage(coupled, drive);
    
    // Output coupling
    return processOutputCoupling(tubeOut);
}

void MagneticDrumEcho::TubeSaturation::reset() {
    inputCouplingState = 0.0;
    outputCouplingState = 0.0;
}

double MagneticDrumEcho::TubeSaturation::processInputCoupling(double input) {
    double output = input - inputCouplingState;
    inputCouplingState += output * inputCouplingCoeff;
    return output;
}

double MagneticDrumEcho::TubeSaturation::processOutputCoupling(double input) {
    double output = input - outputCouplingState;
    outputCouplingState += output * outputCouplingCoeff;
    return output;
}

double MagneticDrumEcho::TubeSaturation::processTubeStage(double input, double drive) {
    // ENHANCED tube saturation for stronger parameter response
    // Scale input by drive - INCREASED range for >1% impact
    double vgk = input * (1.0 + drive * 6.0) + gridBias;  // INCREASED drive range
    
    // Tube transfer function (Child-Langmuir 3/2 power law) - ENHANCED
    double output = 0.0;
    
    if (vgk > 0) {
        // Grid current (overdrive) - ENHANCED for stronger effect
        output = std::tanh(vgk * 2.5) * 0.7;  // INCREASED overdrive response
    } else if (vgk > -5.0) {
        // Normal operation - ENHANCED for better saturation curve
        double normalized = (vgk + 5.0) / 5.0;
        if (normalized > 0) {
            output = std::pow(normalized, 1.4) - 0.4;  // ADJUSTED for stronger character
        }
    }
    // else cutoff region
    
    // Add harmonics (2nd and 3rd) - ENHANCED for stronger saturation effect
    double squared = output * output;
    double cubed = output * squared;
    output += squared * 0.08 * drive;  // INCREASED 2nd harmonic
    output += cubed * 0.04 * drive;    // INCREASED 3rd harmonic
    
    return output;
}

// ==================== WowFlutterSimulator Implementation ====================

void MagneticDrumEcho::WowFlutterSimulator::reset() {
    wowPhase = 0.0;
    flutterPhase = 0.0;
    scrapePhase = 0.0;
    driftValue = 0.0;
    driftTarget = 0.0;
    driftCounter = 0;
}

double MagneticDrumEcho::WowFlutterSimulator::process(double sampleRate) {
    // ENHANCED wow & flutter simulation for stronger parameter response
    // Update phases - ADJUSTED frequencies for more audible effect
    wowPhase += 1.2 / sampleRate;      // 1.2 Hz wow (slightly slower)
    flutterPhase += 8.0 / sampleRate;  // 8 Hz flutter (faster for more noticeable effect)
    scrapePhase += 45.0 / sampleRate;  // 45 Hz scrape (more pronounced)
    
    // Wrap phases
    if (wowPhase >= 1.0) wowPhase -= 1.0;
    if (flutterPhase >= 1.0) flutterPhase -= 1.0;
    if (scrapePhase >= 1.0) scrapePhase -= 1.0;
    
    // Generate modulations - ENHANCED for stronger effect
    double wow = std::sin(2.0 * M_PI * wowPhase) * wowAmount * 1.5;  // INCREASED wow intensity
    double flutter = std::sin(2.0 * M_PI * flutterPhase) * flutterAmount * 1.3;  // INCREASED flutter
    double scrape = std::sin(2.0 * M_PI * scrapePhase) * scrapeAmount * 1.8;  // INCREASED scrape
    
    // Update random drift (every ~80ms for more variation)
    if (++driftCounter > sampleRate * 0.08) {
        driftCounter = 0;
        driftTarget = distribution(rng) * wowAmount * 0.7;  // INCREASED drift amount
    }
    driftValue += (driftTarget - driftValue) * 0.015;  // FASTER drift changes
    
    return wow + flutter + scrape + driftValue;
}

void MagneticDrumEcho::WowFlutterSimulator::setAmount(double wow, double flutter) {
    wowAmount = wow;
    flutterAmount = flutter;
    scrapeAmount = flutter * 0.2;  // Scrape is proportional to flutter
}

// ==================== MotorControl Implementation ====================

void MagneticDrumEcho::MotorControl::setSampleRate(double sr) {
    // Motor inertia time constant
    motorInertia = std::exp(-1.0 / (0.5 * sr));  // 500ms time constant
}

void MagneticDrumEcho::MotorControl::setSpeed(double speed) {
    targetSpeed = std::clamp(speed, 0.1, 3.0);  // Limit speed range
}

void MagneticDrumEcho::MotorControl::update() {
    currentSpeed += (targetSpeed - currentSpeed) * (1.0 - motorInertia);
}

double MagneticDrumEcho::MotorControl::getSpeedWithRipple(double sampleRate) {
    // Add power supply ripple
    ripplePhase += rippleFreq / sampleRate;
    if (ripplePhase >= 1.0) ripplePhase -= 1.0;
    
    double ripple = std::sin(2.0 * M_PI * ripplePhase) * rippleAmount;
    return currentSpeed * (1.0 + ripple);
}

void MagneticDrumEcho::MotorControl::reset() {
    currentSpeed = 1.0;
    targetSpeed = 1.0;
    ripplePhase = 0.0;
}

// ==================== ButterworthFilter Implementation ====================

void MagneticDrumEcho::ButterworthFilter::setLowpass(double freq, double sampleRate, double q) {
    double omega = 2.0 * M_PI * freq / sampleRate;
    double sin_omega = std::sin(omega);
    double cos_omega = std::cos(omega);
    double alpha = sin_omega / (2.0 * q);
    
    double a0 = 1.0 + alpha;
    b0 = (1.0 - cos_omega) / 2.0 / a0;
    b1 = (1.0 - cos_omega) / a0;
    b2 = (1.0 - cos_omega) / 2.0 / a0;
    a1 = -2.0 * cos_omega / a0;
    a2 = (1.0 - alpha) / a0;
}

void MagneticDrumEcho::ButterworthFilter::setHighpass(double freq, double sampleRate, double q) {
    double omega = 2.0 * M_PI * freq / sampleRate;
    double sin_omega = std::sin(omega);
    double cos_omega = std::cos(omega);
    double alpha = sin_omega / (2.0 * q);
    
    double a0 = 1.0 + alpha;
    b0 = (1.0 + cos_omega) / 2.0 / a0;
    b1 = -(1.0 + cos_omega) / a0;
    b2 = (1.0 + cos_omega) / 2.0 / a0;
    a1 = -2.0 * cos_omega / a0;
    a2 = (1.0 - alpha) / a0;
}

double MagneticDrumEcho::ButterworthFilter::process(double input) {
    double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    
    x2 = x1; x1 = input;
    y2 = y1; y1 = output;
    
    // Denormal prevention
    y1 += DENORMAL_PREVENTION;
    y1 -= DENORMAL_PREVENTION;
    
    return output;
}

// ==================== FeedbackProcessor Implementation ====================

void MagneticDrumEcho::FeedbackProcessor::setSampleRate(double sr) {
    attackCoeff = 1.0 - std::exp(-1.0 / (attackTime * sr));
    releaseCoeff = 1.0 - std::exp(-1.0 / (releaseTime * sr));
}

double MagneticDrumEcho::FeedbackProcessor::process(double input, double feedbackAmount) {
    // ENHANCED feedback processing for stronger parameter response
    // Apply feedback amount - INCREASED scaling for >1% impact
    double signal = input * feedbackAmount * 1.2;  // BOOSTED feedback signal
    
    // Soft knee compression - ADJUSTED for better response
    double compressed = softKneeCompression(signal);
    
    // Add tape-like low frequency emphasis - ENHANCED
    double diff = compressed - previousSample;
    previousSample = compressed;
    
    // ENHANCED bass boost in feedback for stronger character
    return compressed + diff * 0.25;  // INCREASED bass emphasis
}

double MagneticDrumEcho::FeedbackProcessor::softKneeCompression(double input) {
    double inputLevel = std::abs(input);
    
    // Update envelope
    if (inputLevel > envelope) {
        envelope += (inputLevel - envelope) * attackCoeff;
    } else {
        envelope += (inputLevel - envelope) * releaseCoeff;
    }
    
    // Soft knee compression
    double gain = 1.0;
    if (envelope > threshold - knee) {
        if (envelope < threshold + knee) {
            // Soft knee region
            double kneeRatio = (envelope - threshold + knee) / (2.0 * knee);
            double softRatio = 1.0 + (ratio - 1.0) * kneeRatio * kneeRatio;
            gain = (threshold + (envelope - threshold) / softRatio) / envelope;
        } else {
            // Hard compression region
            gain = (threshold + (envelope - threshold) / ratio) / envelope;
        }
    }
    
    return input * gain * makeupGain;
}

void MagneticDrumEcho::FeedbackProcessor::reset() {
    previousSample = 0.0;
    envelope = 0.0;
}

// ==================== Oversampler2x Implementation ====================

void MagneticDrumEcho::Oversampler2x::prepare() {
    // Polyphase halfband filter coefficients
    // These provide good stopband attenuation with low latency
    upsampleStages[0].setCoefficient(0.07);
    upsampleStages[1].setCoefficient(0.31);
    downsampleStages[0].setCoefficient(0.07);
    downsampleStages[1].setCoefficient(0.31);
}

void MagneticDrumEcho::Oversampler2x::upsample(const double* input, double* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        // Polyphase decomposition
        double even = input[i];
        double odd = upsampleStages[0].process(even);
        odd = upsampleStages[1].process(odd);
        
        output[i * 2] = even;
        output[i * 2 + 1] = odd;
    }
}

void MagneticDrumEcho::Oversampler2x::downsample(const double* input, double* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        // Extract even samples
        double even = input[i * 2];
        double odd = input[i * 2 + 1];
        
        // Filter odd samples
        odd = downsampleStages[0].process(odd);
        odd = downsampleStages[1].process(odd);
        
        output[i] = (even + odd) * 0.5;
    }
}

void MagneticDrumEcho::Oversampler2x::reset() {
    for (auto& stage : upsampleStages) {
        stage.reset();
    }
    for (auto& stage : downsampleStages) {
        stage.reset();
    }
    z1 = 0;
}

// ==================== Transport Sync Implementation ====================

void MagneticDrumEcho::setTransportInfo(const TransportInfo& info) {
    m_transportInfo = info;
}

bool MagneticDrumEcho::supportsFeature(Feature f) const noexcept {
    switch (f) {
        case Feature::TempoSync: return true;
        default: return false;
    }
}

double MagneticDrumEcho::calculateSyncedDrumSpeed(double speedParam, double syncParam) const {
    // FIXED: Sync parameter now properly affects drum speed calculation
    if (syncParam < 0.5) {
        // Manual speed mode - direct parameter control
        return speedParam;
    }
    
    // Sync is on - tempo-locked mode
    // FIXED: Enhanced sync implementation for >1% parameter impact
    const int divisionIndex = std::max(0, std::min(8, static_cast<int>(speedParam * 8.999))); // 0-8 range
    const BeatDivision division = static_cast<BeatDivision>(divisionIndex);
    
    double syncedSpeed = getBeatDivisionSpeedMultiplier(division);
    
    // ENHANCED: Sync parameter affects the blend between manual and synced speeds
    double syncAmount = (syncParam - 0.5) * 2.0;  // 0.0 to 1.0 range
    return speedParam * (1.0 - syncAmount) + syncedSpeed * syncAmount;
}

double MagneticDrumEcho::getBeatDivisionSpeedMultiplier(BeatDivision division) const {
    const double bpm = std::max(60.0, std::min(200.0, m_transportInfo.bpm));  // RESTRICTED to musical range
    
    // FIXED: Base drum speed calibrated for musical timing
    // This creates a natural mapping between BPM and drum rotation speed
    const double baseDrumSpeed = 0.6;  // ADJUSTED for better sync response
    const double bpmRatio = bpm / 120.0;
    
    // ENHANCED: Better beat division mapping for >1% parameter impact
    switch (division) {
        case BeatDivision::DIV_1_64: return std::min(1.0, baseDrumSpeed * bpmRatio * 12.0);  // CLAMPED for stability
        case BeatDivision::DIV_1_32: return std::min(1.0, baseDrumSpeed * bpmRatio * 6.0);   // CLAMPED for stability
        case BeatDivision::DIV_1_16: return std::min(1.0, baseDrumSpeed * bpmRatio * 3.0);   // CLAMPED for stability
        case BeatDivision::DIV_1_8:  return baseDrumSpeed * bpmRatio * 1.5;   // Medium
        case BeatDivision::DIV_1_4:  return baseDrumSpeed * bpmRatio;         // Quarter note sync
        case BeatDivision::DIV_1_2:  return baseDrumSpeed * bpmRatio * 0.5;   // Half note
        case BeatDivision::DIV_1_1:  return baseDrumSpeed * bpmRatio * 0.25;  // Whole note
        case BeatDivision::DIV_2_1:  return baseDrumSpeed * bpmRatio * 0.125; // Two bars
        case BeatDivision::DIV_4_1:  return baseDrumSpeed * bpmRatio * 0.0625;// Four bars
        default: return baseDrumSpeed * bpmRatio;
    }
}