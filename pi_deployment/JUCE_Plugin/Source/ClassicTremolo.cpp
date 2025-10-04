#include "ClassicTremolo.h"
#include "DspEngineUtilities.h"
#include <algorithm>

ClassicTremolo::ClassicTremolo() {
    // Initialize parameter smoothers
    m_rate = std::make_unique<ParameterSmoother>();
    m_depth = std::make_unique<ParameterSmoother>();
    m_shape = std::make_unique<ParameterSmoother>();
    m_stereoPhase = std::make_unique<ParameterSmoother>();
    m_type = std::make_unique<ParameterSmoother>();
    m_symmetry = std::make_unique<ParameterSmoother>();
    m_volume = std::make_unique<ParameterSmoother>();
    m_mix = std::make_unique<ParameterSmoother>();
    
    // Set default values
    m_rate->reset(5.0);          // 5 Hz
    m_depth->reset(0.5);         // 50% depth
    m_shape->reset(0.0);         // Sine wave
    m_stereoPhase->reset(0.0);   // No stereo offset
    m_type->reset(0.0);          // Sine amplitude tremolo
    m_symmetry->reset(0.5);      // Symmetric waveform
    m_volume->reset(1.0);        // Unity gain
    m_mix->reset(1.0);           // 100% wet
    
    // Create processing components
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        m_harmonicTremolos[ch] = std::make_unique<HarmonicTremolo>();
        m_tubeTremolos[ch] = std::make_unique<TubeBiasTremoloV2>();
        m_rotarySpeakers[ch] = std::make_unique<ProfessionalRotarySpkr>();
        m_oversamplers[ch] = std::make_unique<OptimizedOversampler>();
    }
}

void ClassicTremolo::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize parameter smoothers
    m_rate->setSampleRate(sampleRate, 20.0);       // 20ms smoothing
    m_depth->setSampleRate(sampleRate, 30.0);      // 30ms
    m_shape->setSampleRate(sampleRate, 50.0);      // 50ms
    m_stereoPhase->setSampleRate(sampleRate, 50.0);
    m_type->setSampleRate(sampleRate, 100.0);      // 100ms for type changes
    m_symmetry->setSampleRate(sampleRate, 30.0);
    m_volume->setSampleRate(sampleRate, 20.0);
    m_mix->setSampleRate(sampleRate, 30.0);
    
    // Initialize LFOs
    for (auto& lfo : m_lfos) {
        lfo.setSampleRate(sampleRate);
        lfo.setFrequency(5.0);
    }
    
    // Initialize processing models
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        m_opticalModels[ch].setSampleRate(sampleRate);
        m_harmonicTremolos[ch]->setSampleRate(sampleRate);
        m_tubeTremolos[ch]->setSampleRate(sampleRate);
        m_rotarySpeakers[ch]->setSampleRate(sampleRate);
        m_oversamplers[ch]->prepare(sampleRate);
        m_inputDCBlockers[ch].setCutoff(20.0, sampleRate);
        m_outputDCBlockers[ch].setCutoff(20.0, sampleRate);
    }
    
    reset();
}

void ClassicTremolo::reset() {
    // Reset LFOs with phase offset for stereo
    m_lfos[0].reset(0.0);
    if (NUM_CHANNELS > 1) {
        m_lfos[1].reset(0.25);  // 90 degree offset default
    }
    
    // Reset all processing models
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        m_opticalModels[ch].reset();
        m_harmonicTremolos[ch]->reset();
        m_tubeTremolos[ch]->reset();
        m_rotarySpeakers[ch]->reset();
        m_oversamplers[ch]->reset();
        m_inputDCBlockers[ch].reset();
        m_outputDCBlockers[ch].reset();
    }
    
    // Clear work buffers
    for (auto& buffer : m_workBuffers) {
        buffer.fill(0.0);
    }
    for (auto& buffer : m_lfoBuffers) {
        buffer.fill(0.0);
    }
    for (auto& buffer : m_oversampledBuffers) {
        buffer.fill(0.0);
    }
}

void ClassicTremolo::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Cache all parameters once per block
    CachedParams params;
    params.rate = m_rate->process();
    params.depth = m_depth->process();
    params.shape = m_shape->process();
    params.stereoPhase = m_stereoPhase->process();
    params.symmetry = m_symmetry->process();
    params.volume = m_volume->process();
    params.mix = m_mix->process();
    
    // Determine tremolo type once
    double typeValue = m_type->process();
    params.type = static_cast<TremoloType>(
        std::clamp(static_cast<int>(typeValue * 6.99), 0, 6)
    );
    
    // Update LFO frequencies once per block
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        m_lfos[ch].setFrequency(params.rate);
        m_lfos[ch].setPulseWidth(params.symmetry);
        
        // Apply stereo phase offset
        if (ch == 1) {
            double phaseOffset = params.stereoPhase / 360.0;
            m_lfos[1].reset(m_lfos[0].getPhase() + phaseOffset);
        }
    }
    
    // Check if we need oversampling for this block
    bool needsOversampling = (params.type == TremoloType::BIAS_TREMOLO ||
                             params.type == TremoloType::HARMONIC_TREMOLO);
    
    // Process channels with cached parameters
    for (int ch = 0; ch < std::min(numChannels, NUM_CHANNELS); ++ch) {
        processChannelOptimized(buffer.getWritePointer(ch), numSamples, ch, 
                               params, needsOversampling);
    }
    
    scrubBuffer(buffer);
}

void ClassicTremolo::processChannelOptimized(float* data, int numSamples, int channel,
                                            const CachedParams& params, bool needsOversampling) {
    double* workBuffer = m_workBuffers[channel].data();
    double* lfoBuffer = m_lfoBuffers[channel].data();
    
    // Convert to double and apply input DC blocking
    for (int i = 0; i < numSamples; ++i) {
        workBuffer[i] = static_cast<double>(data[i]);
    }
    m_inputDCBlockers[channel].processBlock(workBuffer, numSamples);
    
    // Generate LFO values for the entire block
    m_lfos[channel].generateBlock(lfoBuffer, numSamples, params.shape);
    
    // Process based on tremolo type
    switch (params.type) {
        case TremoloType::SINE_AMPLITUDE:
        case TremoloType::TRIANGLE_AMPLITUDE:
        case TremoloType::SQUARE_AMPLITUDE:
            // Use SIMD-optimized processing for simple amplitude modulation
            std::copy(workBuffer, workBuffer + numSamples, data);
            processSimpleTremoloSIMD(data, numSamples, lfoBuffer, params.depth);
            std::copy(data, data + numSamples, workBuffer);
            break;
            
        case TremoloType::OPTICAL_TREMOLO:
            // Process optical tremolo
            m_opticalModels[channel].processBlock(lfoBuffer, lfoBuffer, numSamples);
            for (int i = 0; i < numSamples; ++i) {
                double gain = 1.0 - params.depth * (1.0 - lfoBuffer[i]);
                workBuffer[i] *= gain;
            }
            break;
            
        case TremoloType::HARMONIC_TREMOLO:
            if (needsOversampling && OVERSAMPLE_FACTOR > 1) {
                double* oversampledBuffer = m_oversampledBuffers[channel].data();
                m_oversamplers[channel]->processUpsample(workBuffer, oversampledBuffer, 
                                                        numSamples, OVERSAMPLE_FACTOR);
                // Process at higher rate
                m_harmonicTremolos[channel]->processBlock(oversampledBuffer, oversampledBuffer,
                                                         lfoBuffer, numSamples * OVERSAMPLE_FACTOR,
                                                         params.depth);
                m_oversamplers[channel]->processDownsample(oversampledBuffer, numSamples, 
                                                          OVERSAMPLE_FACTOR);
                std::copy(oversampledBuffer, oversampledBuffer + numSamples, workBuffer);
            } else {
                m_harmonicTremolos[channel]->processBlock(workBuffer, workBuffer,
                                                         lfoBuffer, numSamples, params.depth);
            }
            break;
            
        case TremoloType::BIAS_TREMOLO:
            if (needsOversampling && OVERSAMPLE_FACTOR > 1) {
                double* oversampledBuffer = m_oversampledBuffers[channel].data();
                m_oversamplers[channel]->processUpsample(workBuffer, oversampledBuffer,
                                                        numSamples, OVERSAMPLE_FACTOR);
                m_tubeTremolos[channel]->processBlock(oversampledBuffer, oversampledBuffer,
                                                     lfoBuffer, numSamples * OVERSAMPLE_FACTOR,
                                                     params.depth);
                m_oversamplers[channel]->processDownsample(oversampledBuffer, numSamples,
                                                          OVERSAMPLE_FACTOR);
                std::copy(oversampledBuffer, oversampledBuffer + numSamples, workBuffer);
            } else {
                m_tubeTremolos[channel]->processBlock(workBuffer, workBuffer,
                                                     lfoBuffer, numSamples, params.depth);
            }
            break;
            
        case TremoloType::ROTARY_SPEAKER:
            m_rotarySpeakers[channel]->processBlock(workBuffer, workBuffer, numSamples, params.depth);
            break;
    }
    
    // Apply output DC blocking and volume
    m_outputDCBlockers[channel].processBlock(workBuffer, numSamples);
    
    // Apply volume and mix
    for (int i = 0; i < numSamples; ++i) {
        workBuffer[i] *= params.volume;
        data[i] = static_cast<float>(data[i] * (1.0 - params.mix) + workBuffer[i] * params.mix);
    }
}

void ClassicTremolo::processSimpleTremoloSIMD(float* data, int numSamples,
                                              const double* lfoValues, double depth) {
    // Convert depth to gain range
    float minGain = static_cast<float>(1.0 - depth);
    float gainRange = static_cast<float>(depth);
    
    #ifdef __SSE2__
    // Process 4 samples at a time using SIMD
    const int simdWidth = 4;
    int vectorizedSamples = numSamples & ~(simdWidth - 1);
    
    // Vectorized processing
    for (int i = 0; i < vectorizedSamples; i += simdWidth) {
        // Load 4 samples
        __m128 samples = _mm_loadu_ps(&data[i]);
        
        // Calculate gains from LFO
        __m128 gains = _mm_set_ps(
            minGain + gainRange * static_cast<float>((lfoValues[i+3] + 1.0) * 0.5),
            minGain + gainRange * static_cast<float>((lfoValues[i+2] + 1.0) * 0.5),
            minGain + gainRange * static_cast<float>((lfoValues[i+1] + 1.0) * 0.5),
            minGain + gainRange * static_cast<float>((lfoValues[i+0] + 1.0) * 0.5)
        );
        
        // Apply tremolo
        samples = _mm_mul_ps(samples, gains);
        
        // Store results
        _mm_storeu_ps(&data[i], samples);
    }
    
    // Process remaining samples
    for (int i = vectorizedSamples; i < numSamples; ++i) {
        float gain = minGain + gainRange * static_cast<float>((lfoValues[i] + 1.0) * 0.5);
        data[i] *= gain;
    }
    #else
    // Non-SIMD fallback
    for (int i = 0; i < numSamples; ++i) {
        float gain = minGain + gainRange * static_cast<float>((lfoValues[i] + 1.0) * 0.5);
        data[i] *= gain;
    }
    #endif
}

void ClassicTremolo::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? 
               std::clamp(it->second, 0.0f, 1.0f) : defaultValue;
    };
    
    // Map normalized values to actual ranges
    m_rate->setTarget(0.1 + getParam(0, 0.25f) * 19.9);      // 0.1-20 Hz
    m_depth->setTarget(getParam(1, 0.5f));                   // 0-100%
    m_shape->setTarget(getParam(2, 0.0f));                   // Wave shape
    m_stereoPhase->setTarget(getParam(3, 0.0f) * 180.0);     // 0-180 degrees
    m_type->setTarget(getParam(4, 0.0f));                    // Tremolo type
    m_symmetry->setTarget(getParam(5, 0.5f));                // Waveform symmetry
    m_volume->setTarget(getParam(6, 1.0f));                  // Output volume
    m_mix->setTarget(getParam(7, 1.0f));                     // Dry/wet mix
}

juce::String ClassicTremolo::getParameterName(int index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Shape";
        case 3: return "Stereo";
        case 4: return "Type";
        case 5: return "Symmetry";
        case 6: return "Volume";
        case 7: return "Mix";
        default: return "";
    }
}

// ==================== LFO Implementation ====================

void ClassicTremolo::ProfessionalLFO::generateBlock(double* output, int numSamples, double shape) {
    for (int i = 0; i < numSamples; ++i) {
        // Generate waveform based on shape parameter
        if (shape < 0.25) {
            output[i] = sine();
        } else if (shape < 0.5) {
            double blend = (shape - 0.25) * 4.0;
            output[i] = sine() * (1.0 - blend) + triangle() * blend;
        } else if (shape < 0.75) {
            double blend = (shape - 0.5) * 4.0;
            output[i] = triangle() * (1.0 - blend) + square() * blend;
        } else {
            double blend = (shape - 0.75) * 4.0;
            output[i] = square() * (1.0 - blend) + sawUp() * blend;
        }
        
        tick();
    }
}

double ClassicTremolo::ProfessionalLFO::triangle() const {
    double p = phase;
    if (skew != 0.0) {
        // Apply skew for asymmetric triangle
        p = p < 0.5 ? p / (0.5 + skew * 0.5) : (p - 0.5) / (0.5 - skew * 0.5) + 0.5;
        p = std::clamp(p, 0.0, 1.0);
    }
    return p < 0.5 ? 4.0 * p - 1.0 : 3.0 - 4.0 * p;
}

// ==================== Optical Tremolo Implementation ====================

void ClassicTremolo::OpticalTremoloModel::setSampleRate(double sr) {
    // 5ms LED rise time, 25ms photocell decay
    attackCoeff = 1.0 - std::exp(-1.0 / (0.005 * sr));
    decayCoeff = 1.0 - std::exp(-1.0 / (0.025 * sr));
}

double ClassicTremolo::OpticalTremoloModel::process(double lfoValue) {
    // LED brightness follows LFO with attack time
    double targetBrightness = (lfoValue + 1.0) * 0.5;
    
    if (targetBrightness > ledBrightness) {
        ledBrightness += (targetBrightness - ledBrightness) * attackCoeff;
    } else {
        ledBrightness += (targetBrightness - ledBrightness) * decayCoeff;
    }
    
    // Photocell resistance follows brightness non-linearly
    cellResistance = 1.0 / (1.0 + ledBrightness * ledBrightness * 10.0);
    
    return cellResistance;
}

void ClassicTremolo::OpticalTremoloModel::processBlock(const double* lfoValues, double* output, 
                                                       int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process(lfoValues[i]);
    }
}

// ==================== Harmonic Tremolo Implementation ====================

void ClassicTremolo::HarmonicTremolo::AllPassFilter::setFrequency(double freq, double sr) {
    double tan_half_omega = std::tan(M_PI * freq / sr);
    coefficient = (tan_half_omega - 1.0) / (tan_half_omega + 1.0);
}

double ClassicTremolo::HarmonicTremolo::AllPassFilter::process(double input) {
    double output = coefficient * input + x1 - coefficient * y1;
    x1 = input;
    y1 = output + DENORMAL_PREVENTION - DENORMAL_PREVENTION;
    return output;
}

void ClassicTremolo::HarmonicTremolo::setSampleRate(double sr) {
    sampleRate = sr;
    // Set up phase shift network frequencies
    phaseNetwork[0].setFrequency(200.0, sr);
    phaseNetwork[1].setFrequency(500.0, sr);
    phaseNetwork[2].setFrequency(1200.0, sr);
    phaseNetwork[3].setFrequency(3000.0, sr);
}

double ClassicTremolo::HarmonicTremolo::process(double input, double lfoValue, double depth) {
    // Write to delay line
    delayLine[writePos] = input;
    
    // Vibrato via modulated delay
    double delayTime = 2.0 + lfoValue * depth * 1.5;  // 2-3.5ms
    double delaySamples = delayTime * 0.001 * sampleRate;
    
    // Fractional delay with linear interpolation
    int delayInt = static_cast<int>(delaySamples);
    double delayFrac = delaySamples - delayInt;
    
    int readPos1 = (writePos - delayInt + DELAY_SIZE) % DELAY_SIZE;
    int readPos2 = (readPos1 - 1 + DELAY_SIZE) % DELAY_SIZE;
    
    double delayed = delayLine[readPos1] * (1.0 - delayFrac) + 
                   delayLine[readPos2] * delayFrac;
    
    // Phase shift network
    double phaseShifted = delayed;
    for (auto& apf : phaseNetwork) {
        phaseShifted = apf.process(phaseShifted);
    }
    
    // Increment write position
    writePos = (writePos + 1) % DELAY_SIZE;
    
    // Mix original and phase-shifted for harmonic tremolo effect
    return (input + phaseShifted) * 0.5;
}

void ClassicTremolo::HarmonicTremolo::processBlock(const double* input, double* output,
                                                   const double* lfoValues, int numSamples,
                                                   double depth) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process(input[i], lfoValues[i], depth);
    }
}

void ClassicTremolo::HarmonicTremolo::reset() {
    delayLine.fill(0);
    writePos = 0;
    for (auto& apf : phaseNetwork) {
        apf.reset();
    }
}

// ==================== Tube Bias Tremolo V2 Implementation ====================

void ClassicTremolo::TubeBiasTremoloV2::setSampleRate(double sr) {
    sampleRate = sr;
    // RC time constant for coupling cap (0.1uF with 1M grid leak)
    rcTimeConstant = std::exp(-1.0 / (0.1 * sr));
}

double ClassicTremolo::TubeBiasTremoloV2::process(double input, double lfoValue, double depth) {
    // AC coupling with correct sample rate
    double coupled = input - couplingState;
    couplingState += coupled * (1.0 - rcTimeConstant);
    
    // Modulate bias voltage
    double biasModulation = 1.0 + lfoValue * depth * 0.3;
    
    // Tube transfer function (more accurate 3/2 power law)
    double vgk = coupled * 10.0 * biasModulation;
    double output;
    
    if (vgk >= -0.5) {
        // Near cutoff - strong compression
        output = std::tanh(vgk * 0.1) * 0.5;
    } else if (vgk >= -3.0) {
        // Normal operation region
        double normalized = (vgk + 3.0) / 2.5;  // Normalize to 0-1
        output = std::pow(normalized, 1.5) * 0.8 - 0.4;
    } else {
        // Cutoff region
        output = -0.4;
    }
    
    // Add some 2nd harmonic (tube characteristic)
    output += std::abs(output) * output * 0.1;
    
    return output;
}

void ClassicTremolo::TubeBiasTremoloV2::processBlock(const double* input, double* output,
                                                     const double* lfoValues, int numSamples,
                                                     double depth) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process(input[i], lfoValues[i], depth);
    }
}

// ==================== Professional Rotary Speaker Implementation ====================

void ClassicTremolo::ProfessionalRotarySpkr::Rotor::update(double speedHz, double sampleRate) {
    // Smooth speed changes (mechanical inertia)
    speed += (targetSpeed - speed) * (1.0 - inertia);
    angle += speed / sampleRate;
    if (angle >= 1.0) angle -= 1.0;
}

void ClassicTremolo::ProfessionalRotarySpkr::LinkwitzRiley::setFrequency(double freq, double sr, 
                                                                         bool highpass) {
    double omega = 2.0 * M_PI * freq / sr;
    double cos_omega = std::cos(omega);
    double sin_omega = std::sin(omega);
    double alpha = sin_omega / std::sqrt(2.0);
    
    if (highpass) {
        b0 = (1.0 + cos_omega) / 2.0;
        b1 = -(1.0 + cos_omega);
        b2 = (1.0 + cos_omega) / 2.0;
    } else {
        b0 = (1.0 - cos_omega) / 2.0;
        b1 = 1.0 - cos_omega;
        b2 = (1.0 - cos_omega) / 2.0;
    }
    
    double a0 = 1.0 + alpha;
    a1 = -2.0 * cos_omega / a0;
    a2 = (1.0 - alpha) / a0;
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
}

double ClassicTremolo::ProfessionalRotarySpkr::LinkwitzRiley::process(double input) {
    double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = input;
    y2 = y1; y1 = output;
    return output;
}

double ClassicTremolo::ProfessionalRotarySpkr::DopplerDelay::process(double input, double delaySamples) {
    buffer[writePos] = input;
    
    // Fractional delay with cubic interpolation
    double readPos = writePos - delaySamples;
    while (readPos < 0) readPos += DELAY_SIZE;
    
    int idx0 = static_cast<int>(readPos);
    double frac = readPos - idx0;
    
    // Cubic interpolation for smooth doppler
    int idx1 = (idx0 + 1) % DELAY_SIZE;
    int idx2 = (idx0 + 2) % DELAY_SIZE;
    int idx3 = (idx0 + 3) % DELAY_SIZE;
    
    double y0 = buffer[idx0];
    double y1 = buffer[idx1];
    double y2 = buffer[idx2];
    double y3 = buffer[idx3];
    
    double c0 = y1;
    double c1 = 0.5 * (y2 - y0);
    double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
    double c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
    
    writePos = (writePos + 1) % DELAY_SIZE;
    
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

void ClassicTremolo::ProfessionalRotarySpkr::setSampleRate(double sr) {
    sampleRate = sr;
    lowpass.setFrequency(800.0, sr, false);
    highpass.setFrequency(800.0, sr, true);
    
    // Different speeds and inertia for horn vs drum
    hornRotor.inertia = 0.96;  // Lighter, faster response
    drumRotor.inertia = 0.98;  // Heavier, slower response
}

void ClassicTremolo::ProfessionalRotarySpkr::setSpeed(bool fast) {
    fastSpeed = fast;
    if (fast) {
        hornRotor.targetSpeed = 7.0;   // 7 Hz fast
        drumRotor.targetSpeed = 6.5;   // Slightly slower
    } else {
        hornRotor.targetSpeed = 0.8;   // 0.8 Hz slow
        drumRotor.targetSpeed = 0.7;
    }
}

double ClassicTremolo::ProfessionalRotarySpkr::process(double input, double depth) {
    // Split into frequency bands
    double low = lowpass.process(input);
    double high = highpass.process(input);
    
    // Update rotors
    hornRotor.update(hornRotor.targetSpeed, sampleRate);
    drumRotor.update(drumRotor.targetSpeed, sampleRate);
    
    // Apply doppler via modulated delays
    double hornDelaySamples = 5.0 * (1.0 + hornRotor.getSine() * 0.3);
    double drumDelaySamples = 8.0 * (1.0 + drumRotor.getSine() * 0.2);
    
    high = hornDelay.process(high, hornDelaySamples);
    low = drumDelay.process(low, drumDelaySamples);
    
    // Amplitude modulation from rotating speakers
    double hornAmp = 1.0 - depth * 0.3 * (1.0 - (hornRotor.getCosine() + 1.0) * 0.5);
    double drumAmp = 1.0 - depth * 0.2 * (1.0 - (drumRotor.getCosine() + 1.0) * 0.5);
    
    return high * hornAmp + low * drumAmp;
}

void ClassicTremolo::ProfessionalRotarySpkr::processBlock(const double* input, double* output,
                                                         int numSamples, double depth) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process(input[i], depth);
    }
}

void ClassicTremolo::ProfessionalRotarySpkr::reset() {
    hornRotor.angle = 0.0;
    hornRotor.speed = 0.0;
    drumRotor.angle = 0.0;
    drumRotor.speed = 0.0;
    lowpass.reset();
    highpass.reset();
    hornDelay.reset();
    drumDelay.reset();
}

// ==================== Optimized Oversampler Implementation ====================

void ClassicTremolo::OptimizedOversampler::prepare(double sampleRate) {
    // Design 8th order Butterworth at Nyquist/4
    double cutoff = 0.25 / OVERSAMPLE_FACTOR;
    double c = 1.0 / std::tan(M_PI * cutoff);
    double c2 = c * c;
    double sqrt2c = std::sqrt(2.0) * c;
    double a0 = c2 + sqrt2c + 1.0;
    
    // Set coefficients for all stages
    for (auto& stage : upsampleStages) {
        stage.b0 = 1.0 / a0;
        stage.b1 = 2.0 / a0;
        stage.b2 = 1.0 / a0;
        stage.a1 = (2.0 - 2.0 * c2) / a0;
        stage.a2 = (c2 - sqrt2c + 1.0) / a0;
    }
    
    for (auto& stage : downsampleStages) {
        stage.b0 = 1.0 / a0;
        stage.b1 = 2.0 / a0;
        stage.b2 = 1.0 / a0;
        stage.a1 = (2.0 - 2.0 * c2) / a0;
        stage.a2 = (c2 - sqrt2c + 1.0) / a0;
    }
}

void ClassicTremolo::OptimizedOversampler::processUpsample(const double* input, double* output,
                                                          int numSamples, int factor) {
    // Zero-stuff and filter in one pass through first stage
    int outIdx = 0;
    for (int i = 0; i < numSamples; ++i) {
        // Insert input sample
        output[outIdx++] = upsampleStages[0].process(input[i] * factor);
        
        // Insert filtered zeros
        for (int j = 1; j < factor; ++j) {
            output[outIdx++] = upsampleStages[0].process(0.0);
        }
    }
    
    // Apply remaining filter stages
    int totalSamples = numSamples * factor;
    for (int stage = 1; stage < 4; ++stage) {
        for (int i = 0; i < totalSamples; ++i) {
            output[i] = upsampleStages[stage].process(output[i]);
        }
    }
}

void ClassicTremolo::OptimizedOversampler::processDownsample(double* data, int numSamples, int factor) {
    int totalSamples = numSamples * factor;
    
    // Apply all filter stages
    for (auto& stage : downsampleStages) {
        for (int i = 0; i < totalSamples; ++i) {
            data[i] = stage.process(data[i]);
        }
    }
    
    // Decimate in-place
    for (int i = 0; i < numSamples; ++i) {
        data[i] = data[i * factor];
    }
}

void ClassicTremolo::OptimizedOversampler::reset() {
    for (auto& stage : upsampleStages) {
        stage.reset();
    }
    for (auto& stage : downsampleStages) {
        stage.reset();
    }
}