#include "MuffFuzz.h"
#include "DspEngineUtilities.h"
#include <algorithm>
#include <cstring>

MuffFuzz::MuffFuzz() {
    m_sustain = std::make_unique<ParameterSmoother>();
    m_tone = std::make_unique<ParameterSmoother>();
    m_volume = std::make_unique<ParameterSmoother>();
    m_gate = std::make_unique<ParameterSmoother>();
    m_mids = std::make_unique<ParameterSmoother>();
    m_variant = std::make_unique<ParameterSmoother>();
    m_mix = std::make_unique<ParameterSmoother>();
}

void MuffFuzz::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize parameter smoothers
    m_sustain->setSampleRate(sampleRate);
    m_tone->setSampleRate(sampleRate);
    m_volume->setSampleRate(sampleRate);
    m_gate->setSampleRate(sampleRate);
    m_mids->setSampleRate(sampleRate);
    m_variant->setSampleRate(sampleRate);
    m_mix->setSampleRate(sampleRate);
    
    // Fast smoothing for responsive feel
    m_sustain->setSmoothingTime(0.005);
    m_tone->setSmoothingTime(0.005);
    m_volume->setSmoothingTime(0.002);
    m_gate->setSmoothingTime(0.01);
    m_mids->setSmoothingTime(0.005);
    m_variant->setSmoothingTime(0.02);
    m_mix->setSmoothingTime(0.002);
    
    // Prepare circuits for both channels
    for (int ch = 0; ch < 2; ++ch) {
        m_circuits[ch].prepare(sampleRate);
        m_oversamplers[ch].prepare(samplesPerBlock, sampleRate);
        m_gates[ch].setSampleRate(sampleRate);
        m_midScoops[ch].updateCoefficients(750.0, 0.0, sampleRate);
        
        m_inputDCBlockers[ch].setCutoff(20.0, sampleRate);
        m_outputDCBlockers[ch].setCutoff(10.0, sampleRate);
    }
    
    reset();
}

void MuffFuzz::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Allocate oversampled buffers
    std::vector<double> oversampledIn(numSamples * OVERSAMPLE_FACTOR);
    std::vector<double> oversampledOut(numSamples * OVERSAMPLE_FACTOR);
    std::vector<double> channelBuffer(numSamples);
    
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* inputData = buffer.getReadPointer(ch);
        float* outputData = buffer.getWritePointer(ch);
        
        // Convert to double and apply input DC blocking
        for (int i = 0; i < numSamples; ++i) {
            channelBuffer[i] = m_inputDCBlockers[ch].process(static_cast<double>(inputData[i]));
        }
        
        // Upsample
        m_oversamplers[ch].upsample(channelBuffer.data(), oversampledIn.data(), numSamples);
        
        // Process at oversampled rate
        for (int i = 0; i < numSamples * OVERSAMPLE_FACTOR; ++i) {
            // Get smoothed parameters
            double sustain = m_sustain->process();
            double tone = m_tone->process();
            double volume = m_volume->process();
            double gateThresh = m_gate->process();
            double midsDepth = m_mids->process();
            double variantVal = m_variant->process();
            double mixAmt = m_mix->process();
            
            // Apply variant settings
            FuzzVariant currentVariant = static_cast<FuzzVariant>(
                static_cast<int>(variantVal * 5.99f)
            );
            applyVariantSettings(currentVariant);
            
            // Store dry signal
            double dry = oversampledIn[i];
            
            // Process through Big Muff circuit
            double wet = m_circuits[ch].process(dry, sustain, tone, volume);
            
            // Apply gate if threshold > 0
            if (gateThresh > 0.001) {
                wet = m_gates[ch].process(wet, gateThresh * 0.1);
            }
            
            // Apply mid scoop if depth > 0
            if (midsDepth > 0.001) {
                m_midScoops[ch].updateCoefficients(750.0, midsDepth, m_sampleRate * OVERSAMPLE_FACTOR);
                wet = m_midScoops[ch].process(wet);
            }
            
            // Mix dry and wet
            oversampledOut[i] = dry * (1.0 - mixAmt) + wet * mixAmt;
        }
        
        // Downsample
        m_oversamplers[ch].downsample(oversampledOut.data(), channelBuffer.data(), numSamples);
        
        // Apply output DC blocking and convert to float
        for (int i = 0; i < numSamples; ++i) {
            double sample = m_outputDCBlockers[ch].process(channelBuffer[i]);
            
            // Soft clipping for safety
            sample = std::tanh(sample * 0.7) * 1.4286;
            
            outputData[i] = static_cast<float>(sample);
        }
    }
    
    // Update thermal model (once per block)
    double avgPower = 0.1;  // Simplified power calculation
    m_thermalModel.update(avgPower, numSamples / m_sampleRate);
    
    scrubBuffer(buffer);
}

void MuffFuzz::reset() {
    for (int ch = 0; ch < 2; ++ch) {
        m_circuits[ch].reset();
        m_oversamplers[ch].reset();
        m_gates[ch].reset();
        m_midScoops[ch].reset();
        m_inputDCBlockers[ch].reset();
        m_outputDCBlockers[ch].reset();
    }
}

void MuffFuzz::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (index) {
            case 0: m_sustain->setTarget(value); break;
            case 1: m_tone->setTarget(value); break;
            case 2: m_volume->setTarget(value); break;
            case 3: m_gate->setTarget(value); break;
            case 4: m_mids->setTarget(value); break;
            case 5: m_variant->setTarget(value); break;
            case 6: m_mix->setTarget(value); break;
        }
    }
}

juce::String MuffFuzz::getParameterName(int index) const {
    switch (index) {
        case 0: return "Sustain";
        case 1: return "Tone";
        case 2: return "Volume";
        case 3: return "Gate";
        case 4: return "Mids";
        case 5: return "Variant";
        case 6: return "Mix";
        default: return "";
    }
}

void MuffFuzz::applyVariantSettings(FuzzVariant variant) {
    double temperature = 298.15;  // Room temperature
    double matching = 1.0;
    
    switch (variant) {
        case FuzzVariant::TRIANGLE_1971:
            temperature = 303.15;  // Slightly warm, vintage components
            matching = 0.85;       // Looser tolerances
            break;
            
        case FuzzVariant::RAMS_HEAD_1973:
            temperature = 300.15;
            matching = 0.9;
            break;
            
        case FuzzVariant::NYC_REISSUE:
            temperature = 298.15;  // Modern, stable
            matching = 0.95;
            break;
            
        case FuzzVariant::RUSSIAN_SOVTEK:
            temperature = 295.15;  // Colder climate components
            matching = 0.8;        // Soviet-era tolerances
            break;
            
        case FuzzVariant::OP_AMP_VERSION:
            temperature = 298.15;
            matching = 0.98;       // Op-amps are more consistent
            break;
            
        case FuzzVariant::MODERN_DELUXE:
            temperature = 298.15;
            matching = 1.0;        // Precision matched
            break;
    }
    
    // Apply thermal settings to both channels
    for (int ch = 0; ch < 2; ++ch) {
        m_circuits[ch].setTemperature(temperature);
        m_circuits[ch].setComponentVariation(matching);
    }
}

// BigMuffToneStack implementation
void MuffFuzz::BigMuffToneStack::updateCoefficients(double tonePosition, double sampleRate) {
    // Tone control varies the balance between two RC networks
    // Position: 0 = full bass, 1 = full treble
    
    // Calculate effective pot resistance
    double Rpot1 = R4 * (1.0 - tonePosition);
    double Rpot2 = R4 * tonePosition;
    
    // Simplified transfer function coefficients
    // This is an approximation of the actual circuit response
    double fc1 = 1.0 / (2.0 * M_PI * (R1 + Rpot1) * C1);
    double fc2 = 1.0 / (2.0 * M_PI * (R2 + Rpot2) * C2);
    
    // Use a shelf filter approximation
    double K1 = std::tan(M_PI * fc1 / sampleRate);
    double K2 = std::tan(M_PI * fc2 / sampleRate);
    
    // Blended response
    double alpha = tonePosition;
    double K = K1 * (1.0 - alpha) + K2 * alpha;
    double K2_val = K * K;
    
    double norm = 1.0 / (K2_val + K * std::sqrt(2.0) + 1.0);
    
    // High-pass influence increases with tone position
    double hpInfluence = tonePosition * 0.7;
    
    b0 = (1.0 - hpInfluence + hpInfluence * K2_val) * norm;
    b1 = 2.0 * (hpInfluence * K2_val - (1.0 - hpInfluence)) * norm;
    b2 = (1.0 - hpInfluence + hpInfluence * K2_val) * norm;
    a1 = 2.0 * (K2_val - 1.0) * norm;
    a2 = (K2_val - K * std::sqrt(2.0) + 1.0) * norm;
}

double MuffFuzz::BigMuffToneStack::process(double input) {
    double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    
    // Update state
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    // Denormal prevention
    y1 += DENORMAL_PREVENTION;
    y1 -= DENORMAL_PREVENTION;
    
    return output;
}

// TransistorClippingStage implementation
double MuffFuzz::TransistorClippingStage::process(double input, double gain, double bias) {
    // Temperature-dependent parameters
    double vt = 8.617333e-5 * temperature;  // Thermal voltage
    double adjustedVbe = vbe * (1.0 - (temperature - 298.15) * 0.002);
    
    // Apply gain and bias
    double biasedInput = input * gain + bias;
    
    // AC coupling (simplified)
    double coupled = biasedInput - collectorCurrent * 0.1;
    
    // Transistor transfer function
    double vbeClamped = std::max(coupled, -adjustedVbe);
    double ic = (vbeClamped / adjustedVbe) * std::exp(vbeClamped / vt);
    
    // Beta limiting (current gain)
    ic = std::tanh(ic / beta) * beta;
    
    // Update collector current with filtering
    collectorCurrent += (ic - collectorCurrent) * c1;
    
    // Output with soft saturation
    double output = std::tanh(collectorCurrent * 0.5) * 2.0;
    
    return output;
}

// DiodeClipper implementation  
double MuffFuzz::DiodeClipper::process(double voltage) {
    // Temperature-adjusted thermal voltage
    double vt = VT * (temperature / 298.15);
    
    // Silicon diode exponential characteristic
    // Back-to-back diodes create symmetric clipping
    double threshold = DIODE_THRESHOLD * (1.0 - (temperature - 298.15) * 0.002);
    
    if (std::abs(voltage) < threshold * 0.5) {
        // Linear region (small signal)
        return voltage;
    }
    
    // Exponential diode curve
    double sign = (voltage > 0) ? 1.0 : -1.0;
    double absV = std::abs(voltage);
    
    // Shockley diode equation approximation
    double current = IS * (std::exp(absV / (N * vt)) - 1.0);
    
    // Voltage across diode with series resistance
    double vDiode = N * vt * std::log(1.0 + current / IS);
    vDiode = std::min(vDiode, threshold);
    
    return sign * vDiode;
}

// Oversampler implementation
void MuffFuzz::Oversampler::prepare(int blockSize, double sampleRate) {
    oversampledBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
    
    // Design anti-aliasing filters
    double cutoff = 0.45 / OVERSAMPLE_FACTOR;  // Below Nyquist
    
    for (auto& filter : upsampleFilters) {
        filter.design(cutoff);
    }
    
    for (auto& filter : downsampleFilters) {
        filter.design(cutoff);
    }
    
    reset();
}

void MuffFuzz::Oversampler::upsample(const double* input, double* output, int numSamples) {
    // Zero-stuff and filter
    for (int i = 0; i < numSamples; ++i) {
        for (int j = 0; j < OVERSAMPLE_FACTOR; ++j) {
            double sample = (j == 0) ? input[i] * OVERSAMPLE_FACTOR : 0.0;
            
            // Apply cascaded filters
            for (auto& filter : upsampleFilters) {
                sample = filter.process(sample);
            }
            
            output[i * OVERSAMPLE_FACTOR + j] = sample;
        }
    }
}

void MuffFuzz::Oversampler::downsample(const double* input, double* output, int numSamples) {
    // Filter and decimate
    std::vector<double> filtered(numSamples * OVERSAMPLE_FACTOR);
    
    // Apply anti-aliasing filters
    for (int i = 0; i < numSamples * OVERSAMPLE_FACTOR; ++i) {
        double sample = input[i];
        
        for (auto& filter : downsampleFilters) {
            sample = filter.process(sample);
        }
        
        filtered[i] = sample;
    }
    
    // Decimate
    for (int i = 0; i < numSamples; ++i) {
        output[i] = filtered[i * OVERSAMPLE_FACTOR];
    }
}

void MuffFuzz::Oversampler::reset() {
    for (auto& filter : upsampleFilters) {
        filter.reset();
    }
    for (auto& filter : downsampleFilters) {
        filter.reset();
    }
}

// BigMuffCircuit implementation
void MuffFuzz::BigMuffCircuit::prepare(double sampleRate) {
    inputBuffer.setSampleRate(sampleRate);
    clippingStage1.setSampleRate(sampleRate);
    clippingStage2.setSampleRate(sampleRate);
    outputBuffer.setSampleRate(sampleRate);
    
    toneStack.updateCoefficients(0.5, sampleRate);
}

double MuffFuzz::BigMuffCircuit::process(double input, double sustain, double tone, double volume) {
    // Input buffer stage (unity gain, high input impedance emulation)
    double signal = inputBuffer.process(input, 1.0, 0.0);
    
    // First clipping stage with sustain control
    double gain1 = 1.0 + sustain * 100.0;  // Sustain controls gain
    signal = clippingStage1.process(signal, gain1, 0.1);
    
    // First diode clipping
    signal = diodeClipper1.process(signal * 0.5) * 2.0;
    
    // Second clipping stage
    double gain2 = 10.0 * (0.5 + sustain * 0.5);  // Additional gain
    signal = clippingStage2.process(signal, gain2, 0.05);
    
    // Second diode clipping  
    signal = diodeClipper2.process(signal * 0.3) * 3.33;
    
    // Tone stack
    toneStack.updateCoefficients(tone, 48000.0);  // Assumes base rate
    signal = toneStack.process(signal);
    
    // Output buffer with volume control
    signal = outputBuffer.process(signal, volume * 2.0, 0.0);
    
    // Component variation affects overall gain slightly
    signal *= (0.9 + transistorMatching * 0.1);
    
    return signal;
}

void MuffFuzz::BigMuffCircuit::setTemperature(double tempK) {
    inputBuffer.setTemperature(tempK);
    clippingStage1.setTemperature(tempK);
    clippingStage2.setTemperature(tempK);
    outputBuffer.setTemperature(tempK);
    diodeClipper1.setTemperature(tempK);
    diodeClipper2.setTemperature(tempK);
}

void MuffFuzz::BigMuffCircuit::setComponentVariation(double matching) {
    transistorMatching = matching;
    diodeMatching = matching;
}

void MuffFuzz::BigMuffCircuit::reset() {
    inputBuffer.reset();
    clippingStage1.reset();
    clippingStage2.reset();
    outputBuffer.reset();
    toneStack.reset();
}

// NoiseGate implementation
double MuffFuzz::NoiseGate::process(double input, double threshold) {
    double absInput = std::abs(input);
    
    // Update envelope
    double targetEnv = absInput;
    if (targetEnv > envelope) {
        envelope += (targetEnv - envelope) * attackTime;
    } else {
        envelope += (targetEnv - envelope) * releaseTime;
    }
    
    // Gate logic with hysteresis
    if (gateState < 0.5) {
        // Gate is closed
        if (envelope > threshold * (1.0 + hysteresis * 0.1)) {
            gateState = 1.0;  // Open gate
        }
    } else {
        // Gate is open
        if (envelope < threshold * (1.0 - hysteresis * 0.1)) {
            gateState = 0.0;  // Close gate
        }
    }
    
    // Smooth gate transitions
    static double smoothGate = 1.0;
    smoothGate += (gateState - smoothGate) * 0.01;
    
    return input * smoothGate;
}

// MidScoopFilter implementation
void MuffFuzz::MidScoopFilter::updateCoefficients(double frequency, double depth, double sampleRate) {
    // Notch filter for mid scoop
    double omega = 2.0 * M_PI * frequency / sampleRate;
    double cos_omega = std::cos(omega);
    double sin_omega = std::sin(omega);
    
    // Q increases with depth for sharper scoop
    double Q = 2.0 + depth * 8.0;
    double alpha = sin_omega / (2.0 * Q);
    
    // Notch filter coefficients
    double a0 = 1.0 + alpha;
    
    b0 = (1.0 - depth * 0.5) / a0;  // Depth controls notch depth
    b1 = -2.0 * cos_omega * (1.0 - depth * 0.5) / a0;
    b2 = (1.0 - depth * 0.5) / a0;
    a1 = -2.0 * cos_omega / a0;
    a2 = (1.0 - alpha) / a0;
}

double MuffFuzz::MidScoopFilter::process(double input) {
    double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    
    // Update state
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    // Denormal prevention
    y1 += DENORMAL_PREVENTION;
    y1 -= DENORMAL_PREVENTION;
    
    return output;
}