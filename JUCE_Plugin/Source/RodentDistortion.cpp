#include "RodentDistortion.h"
#include "DspEngineUtilities.h"
#include <cmath>
#include <algorithm>

// ==================== CONSTRUCTOR ====================

RodentDistortion::RodentDistortion() {
    // Initialize parameter smoothers
    m_gain = std::make_unique<ParameterSmoother>();
    m_filter = std::make_unique<ParameterSmoother>();
    m_clipping = std::make_unique<ParameterSmoother>();
    m_tone = std::make_unique<ParameterSmoother>();
    m_output = std::make_unique<ParameterSmoother>();
    m_mix = std::make_unique<ParameterSmoother>();
    m_distortionType = std::make_unique<ParameterSmoother>();
    m_presence = std::make_unique<ParameterSmoother>();
    
    // Set default values
    m_gain->reset(0.5);           // Mid gain
    m_filter->reset(0.4);         // 2kHz-ish
    m_clipping->reset(0.5);       // Moderate clipping
    m_tone->reset(0.5);           // Neutral tone
    m_output->reset(0.5);         // Unity gain
    m_mix->reset(1.0);            // 100% wet
    m_distortionType->reset(0.0); // RAT mode
    m_presence->reset(0.3);       // Subtle presence
    
    // Initialize oversamplers
    for (int i = 0; i < 2; ++i) {
        m_oversamplers[i] = std::make_unique<Oversampler>();
    }
    
    // Initialize feedback arrays and processing buffers
    m_fuzzFaceFeedback.fill(0.0);
}

// ==================== ELLIPTIC FILTER DESIGN ====================

void RodentDistortion::EllipticFilter::design(double normalizedFreq, double passbandRipple, double stopbandAtten) {
    // 8th-order elliptic filter design
    // This is a simplified version - in production, use a proper filter design library
    
    // Pre-warped frequency
    double wp = 2.0 * std::tan(M_PI * normalizedFreq);
    
    // For an 8th order elliptic with 0.1dB ripple and 80dB stopband
    // These are pre-calculated coefficients for common case
    if (std::abs(normalizedFreq - 0.1125) < 0.001) { // 0.45/4 for 4x oversampling
        // Stage 1
        coeffs[0] = {
            0.0009441, 0.0018881, 0.0009441,
            -1.911198, 0.914974
        };
        
        // Stage 2
        coeffs[1] = {
            0.003789, 0.007578, 0.003789,
            -1.815893, 0.831049
        };
        
        // Stage 3
        coeffs[2] = {
            0.013657, 0.027314, 0.013657,
            -1.632993, 0.687621
        };
        
        // Stage 4
        coeffs[3] = {
            0.042659, 0.085318, 0.042659,
            -1.378091, 0.548728
        };
    } else {
        // Generic butterworth as fallback
        double Q[4] = {0.5412, 1.3065, 0.5412, 1.3065};
        
        for (int i = 0; i < 4; ++i) {
            double w0 = wp;
            double cosw0 = std::cos(w0);
            double sinw0 = std::sin(w0);
            double alpha = sinw0 / (2.0 * Q[i]);
            
            double b0 = (1.0 - cosw0) / 2.0;
            double b1 = 1.0 - cosw0;
            double b2 = (1.0 - cosw0) / 2.0;
            double a0 = 1.0 + alpha;
            double a1 = -2.0 * cosw0;
            double a2 = 1.0 - alpha;
            
            // Normalize
            coeffs[i] = {
                b0/a0, b1/a0, b2/a0,
                a1/a0, a2/a0
            };
        }
    }
}

// ==================== OVERSAMPLER ====================

void RodentDistortion::Oversampler::upsample(const double* input, double* output, int numSamples) {
    // Zero-stuff and filter
    int outputIdx = 0;
    
    for (int i = 0; i < numSamples; ++i) {
        // Insert input sample
        output[outputIdx] = upsampleFilter.process(input[i] * FACTOR);
        outputIdx++;
        
        // Insert zeros
        for (int j = 1; j < FACTOR; ++j) {
            output[outputIdx] = upsampleFilter.process(0.0);
            outputIdx++;
        }
    }
}

void RodentDistortion::Oversampler::downsample(const double* input, double* output, int numSamples) {
    // Filter and decimate
    int inputIdx = 0;
    
    for (int i = 0; i < numSamples; ++i) {
        // Apply anti-aliasing filter to all samples
        double filtered = 0.0;
        for (int j = 0; j < FACTOR; ++j) {
            filtered = downsampleFilter.process(input[inputIdx++]);
        }
        
        // Take only the last filtered sample (after the zeros have been processed)
        output[i] = filtered;
    }
}

// ==================== MAIN PROCESSING ====================

void RodentDistortion::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize parameter smoothers
    m_gain->setSampleRate(sampleRate);
    m_filter->setSampleRate(sampleRate);
    m_clipping->setSampleRate(sampleRate);
    m_tone->setSampleRate(sampleRate);
    m_output->setSampleRate(sampleRate);
    m_mix->setSampleRate(sampleRate);
    m_distortionType->setSampleRate(sampleRate);
    m_presence->setSampleRate(sampleRate);
    
    // Set smoothing times
    m_gain->setSmoothingTime(0.01);          // 10ms - fast for responsiveness
    m_filter->setSmoothingTime(0.02);        // 20ms
    m_clipping->setSmoothingTime(0.01);      // 10ms
    m_tone->setSmoothingTime(0.02);          // 20ms
    m_output->setSmoothingTime(0.01);        // 10ms
    m_mix->setSmoothingTime(0.02);           // 20ms
    m_distortionType->setSmoothingTime(0.05); // 50ms - slower for mode switching
    m_presence->setSmoothingTime(0.02);       // 20ms
    
    // Initialize filters
    // Pre-allocate processing buffers to avoid dynamic allocation in audio thread
    const int oversampledSize = samplesPerBlock * DistortionConstants::OVERSAMPLE_FACTOR;
    m_inputDouble.resize(samplesPerBlock);
    m_oversampledInput.resize(oversampledSize);
    m_oversampledOutput.resize(oversampledSize);
    m_outputDouble.resize(samplesPerBlock);
    
    for (int ch = 0; ch < 2; ++ch) {
        m_inputFilters[ch].updateCoefficients(2000.0, 0.7, sampleRate);
        m_toneFilters[ch].updateCoefficients(5000.0, 0.5, sampleRate);
        
        // Prepare oversampling
        m_oversamplers[ch]->prepare(samplesPerBlock, sampleRate);
        
        // DC blockers - 20Hz highpass
        m_inputDCBlockers[ch].setCutoff(20.0, sampleRate);
        m_outputDCBlockers[ch].setCutoff(20.0, sampleRate);
    }
}

void RodentDistortion::reset() {
    // Reset all filters and states
    for (int ch = 0; ch < 2; ++ch) {
        m_inputFilters[ch].reset();
        m_toneFilters[ch].reset();
        m_oversamplers[ch]->reset();
        m_inputDCBlockers[ch].reset();
        m_outputDCBlockers[ch].reset();
        m_opAmps[ch].reset();
    }
    
    // Clear fuzz face feedback
    m_fuzzFaceFeedback.fill(0.0);
    
    // Clear processing buffers
    std::fill(m_inputDouble.begin(), m_inputDouble.end(), 0.0);
    std::fill(m_oversampledInput.begin(), m_oversampledInput.end(), 0.0);
    std::fill(m_oversampledOutput.begin(), m_oversampledOutput.end(), 0.0);
    std::fill(m_outputDouble.begin(), m_outputDouble.end(), 0.0);
}

void RodentDistortion::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Use pre-allocated buffers (no dynamic allocation in audio thread)
    const int oversampledSize = numSamples * DistortionConstants::OVERSAMPLE_FACTOR;
    
    // Ensure buffers are large enough (shouldn't need to resize in normal operation)
    if (m_inputDouble.size() < static_cast<size_t>(numSamples)) {
        m_inputDouble.resize(numSamples);
    }
    if (m_oversampledInput.size() < static_cast<size_t>(oversampledSize)) {
        m_oversampledInput.resize(oversampledSize);
    }
    if (m_oversampledOutput.size() < static_cast<size_t>(oversampledSize)) {
        m_oversampledOutput.resize(oversampledSize);
    }
    if (m_outputDouble.size() < static_cast<size_t>(numSamples)) {
        m_outputDouble.resize(numSamples);
    }
    
    // Process each channel
    for (int ch = 0; ch < std::min(numChannels, 2); ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        
        // Convert to double precision
        for (int i = 0; i < numSamples; ++i) {
            m_inputDouble[i] = static_cast<double>(channelData[i]);
        }
        
        // DC blocking on input
        for (int i = 0; i < numSamples; ++i) {
            m_inputDouble[i] = m_inputDCBlockers[ch].process(m_inputDouble[i]);
        }
        
        // Upsample
        m_oversamplers[ch]->upsample(m_inputDouble.data(), m_oversampledInput.data(), numSamples);
        
        // Process at oversampled rate
        for (int i = 0; i < oversampledSize; ++i) {
            // Update parameters (at oversampled rate for smooth operation)
            double gain = m_gain->process();
            double filterFreq = DistortionConstants::MIN_FILTER_HZ + 
                               m_filter->process() * (DistortionConstants::MAX_FILTER_HZ - 
                                                     DistortionConstants::MIN_FILTER_HZ);
            double clipping = m_clipping->process();
            double toneFreq = DistortionConstants::MIN_TONE_HZ + 
                             m_tone->process() * (DistortionConstants::MAX_TONE_HZ - 
                                                 DistortionConstants::MIN_TONE_HZ);
            double outputGain = m_output->process();
            double presence = m_presence->process();
            double distMode = m_distortionType->process();
            
            // Update filter frequencies (only when changed significantly)
            if (i % 16 == 0) { // Update every 16 samples to save CPU
                m_inputFilters[ch].updateCoefficients(filterFreq, 0.7, m_sampleRate * DistortionConstants::OVERSAMPLE_FACTOR);
                m_toneFilters[ch].updateCoefficients(toneFreq, 0.5, m_sampleRate * DistortionConstants::OVERSAMPLE_FACTOR);
            }
            
            // Get sample
            double sample = m_oversampledInput[i];
            
            // Input filter (highpass to remove mud)
            auto filterOut = m_inputFilters[ch].process(sample);
            sample = filterOut.highpass;
            
            // Apply gain with safety limits
            double gainDB = DistortionConstants::MIN_GAIN_DB + 
                           gain * (DistortionConstants::MAX_GAIN_DB - DistortionConstants::MIN_GAIN_DB);
            // Clamp gain to safe range to prevent std::pow overflow
            gainDB = std::clamp(gainDB, -60.0, 60.0);
            double gainLinear = std::pow(10.0, gainDB / 20.0);
            // Additional safety clamp on linear gain
            gainLinear = std::clamp(gainLinear, 0.001, 1000.0);
            sample *= gainLinear;
            
            // Safety check for NaN/Inf
            if (!std::isfinite(sample)) {
                sample = 0.0;
            }
            
            // Distortion based on mode
            int mode = static_cast<int>(distMode * 3.99);
            switch (static_cast<VintageMode>(mode)) {
                case VintageMode::RAT:
                    sample = processRATCircuit(sample, ch);
                    break;
                case VintageMode::TUBE_SCREAMER:
                    sample = processTubeScreamerCircuit(sample, ch);
                    break;
                case VintageMode::BIG_MUFF:
                    sample = processBigMuffCircuit(sample, ch);
                    break;
                case VintageMode::FUZZ_FACE:
                    sample = processFuzzFaceCircuit(sample, ch);
                    break;
            }
            
            // Presence control (high frequency emphasis)
            if (presence > 0.01) {
                auto toneOut = m_toneFilters[ch].process(sample);
                double highFreq = toneOut.highpass;
                sample += highFreq * presence * 2.0;
            }
            
            // Tone control (lowpass)
            auto toneOut = m_toneFilters[ch].process(sample);
            sample = toneOut.lowpass;
            
            // Output gain
            sample *= outputGain * 2.0; // 0-2 range
            
            // Final safety check before output
            if (!std::isfinite(sample)) {
                sample = 0.0;
            }
            
            // Soft limiting for safety
            sample = tanhApproximation(sample * 0.5) * 2.0;
            
            // Final NaN/Inf check
            if (!std::isfinite(sample)) {
                sample = 0.0;
            }
            
            m_oversampledOutput[i] = sample;
        }
        
        // Downsample
        m_oversamplers[ch]->downsample(m_oversampledOutput.data(), m_outputDouble.data(), numSamples);
        
        // DC blocking on output
        for (int i = 0; i < numSamples; ++i) {
            m_outputDouble[i] = m_outputDCBlockers[ch].process(m_outputDouble[i]);
        }
        
        // Mix dry/wet and convert back to float
        double mix = m_mix->getCurrent();
        for (int i = 0; i < numSamples; ++i) {
            double dry = m_inputDouble[i];
            double wet = m_outputDouble[i];
            channelData[i] = static_cast<float>(wet * mix + dry * (1.0 - mix));
        }
    }
    
    // Update thermal model
    double avgPower = 0.1; // Simplified - in reality, calculate from circuit current/voltage
    m_thermalModel.update(avgPower, numSamples / m_sampleRate);
    
    scrubBuffer(buffer);
}

// ==================== CIRCUIT MODELS ====================

double RodentDistortion::processRATCircuit(double input, int channel) {
    // ProCo RAT circuit emulation
    // Input stage - op-amp with gain (safe limits)
    double clippingAmount = std::clamp(m_clipping->getCurrent(), 0.0, 1.0);
    double opAmpGain = 1.0 + clippingAmount * 100.0; // Reduced from 1000x to 100x for safety
    opAmpGain = std::clamp(opAmpGain, 1.0, 200.0); // Hard limit to prevent extreme values
    double output = m_opAmps[channel].process(input, opAmpGain, 
                                              m_sampleRate * DistortionConstants::OVERSAMPLE_FACTOR);
    
    // Hard clipping diodes (back-to-back silicon)
    const double diodeThreshold = 0.7;
    if (output > diodeThreshold) {
        output = diodeThreshold + (output - diodeThreshold) * 0.05;
    } else if (output < -diodeThreshold) {
        output = -diodeThreshold + (output + diodeThreshold) * 0.05;
    }
    
    // Safety check and output filter (compensate for harsh harmonics)
    if (!std::isfinite(output)) {
        output = 0.0;
    }
    return output * 0.5;
}

double RodentDistortion::processTubeScreamerCircuit(double input, int channel) {
    // Ibanez Tube Screamer circuit emulation
    // Characteristic mid-hump EQ (before clipping)
    auto filtered = m_inputFilters[channel].process(input);
    double midBoosted = filtered.bandpass * 2.0 + input * 0.5;
    
    // Op-amp gain stage (lower gain than RAT)
    double opAmpGain = 1.0 + m_clipping->getCurrent() * 100.0; // Up to 100x
    double amplified = midBoosted * opAmpGain;
    
    // Soft clipping with diodes in feedback loop
    double clipped = m_diodeClippers[channel].process(amplified, false);
    
    // Asymmetric clipping characteristic
    if (clipped > 0) {
        clipped = m_diodeClippers[channel].process(clipped * 0.9, false);
    }
    
    // Safety check
    if (!std::isfinite(clipped)) {
        clipped = 0.0;
    }
    return clipped * 0.3;
}

double RodentDistortion::processBigMuffCircuit(double input, int channel) {
    // Electro-Harmonix Big Muff circuit emulation
    // Multiple gain stages with clipping
    double signal = input;
    
    // First gain stage
    signal *= 50.0 * (0.5 + m_clipping->getCurrent());
    signal = softClipAsymmetric(signal, 0.3);
    
    // Second gain stage
    signal *= 20.0;
    signal = softClipAsymmetric(signal, 0.5);
    
    // Tone control (special Big Muff style)
    // This is a simplified version of the actual tone stack
    auto toneOut = m_toneFilters[channel].process(signal);
    double tonePosition = m_tone->getCurrent();
    signal = toneOut.lowpass * (1.0 - tonePosition) + toneOut.highpass * tonePosition;
    
    // Final gain stage
    signal *= 10.0;
    signal = softClipAsymmetric(signal, 0.2);
    
    // Safety check
    if (!std::isfinite(signal)) {
        signal = 0.0;
    }
    return signal * 0.1;
}

double RodentDistortion::processFuzzFaceCircuit(double input, int channel) {
    // Dallas Arbiter Fuzz Face circuit emulation
    // Germanium transistor characteristics
    double temperature = m_thermalModel.getTemperature();
    
    // Temperature-dependent biasing
    double bias = -0.2 + (temperature - 298.15) * 0.001;
    
    // First transistor stage (Q1)
    double q1Out = m_transistors[channel].process(input * 10.0, bias);
    
    // Second transistor stage (Q2) with feedback
    double q2Input = q1Out - m_fuzzFaceFeedback[channel] * 0.5;
    double q2Out = m_transistors[channel].process(q2Input * 50.0, bias * 1.2);
    
    m_fuzzFaceFeedback[channel] = q2Out * 0.1;
    
    // Fuzz control affects the input impedance and gain
    double fuzzAmount = m_clipping->getCurrent();
    q2Out *= (0.1 + fuzzAmount * 0.9);
    
    // Gate effect at low input levels
    if (std::abs(input) < 0.05) {
        q2Out *= std::abs(input) * 20.0;
    }
    
    // Safety check
    if (!std::isfinite(q2Out)) {
        q2Out = 0.0;
    }
    return q2Out * 0.5;
}

// ==================== HELPER FUNCTIONS ====================

double RodentDistortion::tanhApproximation(double x) {
    // Fast tanh approximation for soft clipping
    // More accurate than x/(1+|x|) but faster than std::tanh
    if (x < -3.0) return -1.0;
    if (x > 3.0) return 1.0;
    
    double x2 = x * x;
    return x * (27.0 + x2) / (27.0 + 9.0 * x2);
}

double RodentDistortion::softClipAsymmetric(double x, double amount) {
    // Asymmetric soft clipping
    double positive = x > 0 ? x : 0;
    double negative = x < 0 ? -x : 0;
    
    // Different curves for positive and negative
    positive = tanhApproximation(positive * (1.0 + amount));
    negative = tanhApproximation(negative * (1.0 - amount * 0.3));
    
    return positive - negative;
}

// ==================== PARAMETER HANDLING ====================

void RodentDistortion::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? 
               std::clamp(it->second, 0.0f, 1.0f) : defaultValue;
    };
    
    // Update all parameters with clamping
    m_gain->setTarget(static_cast<double>(getParam(0, 0.5f)));
    m_filter->setTarget(static_cast<double>(getParam(1, 0.4f)));
    m_clipping->setTarget(static_cast<double>(getParam(2, 0.5f)));
    m_tone->setTarget(static_cast<double>(getParam(3, 0.5f)));
    m_output->setTarget(static_cast<double>(getParam(4, 0.5f)));
    m_mix->setTarget(static_cast<double>(getParam(5, 1.0f)));
    m_distortionType->setTarget(static_cast<double>(getParam(6, 0.0f)));
    m_presence->setTarget(static_cast<double>(getParam(7, 0.3f)));
}

juce::String RodentDistortion::getParameterName(int index) const {
    switch (index) {
        case 0: return "Gain";
        case 1: return "Filter";
        case 2: return "Clipping";
        case 3: return "Tone";
        case 4: return "Output";
        case 5: return "Mix";
        case 6: return "Mode";
        case 7: return "Presence";
        default: return "";
    }
}