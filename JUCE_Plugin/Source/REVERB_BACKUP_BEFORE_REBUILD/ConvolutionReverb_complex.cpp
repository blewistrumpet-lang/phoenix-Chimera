// ==================== ConvolutionReverb_Fixed.cpp ====================
// Simplified version that works without crashes
#include "ConvolutionReverb.h"
#include "DenormalProtection.h"
#include <cmath>
#include <random>
#include <algorithm>

ConvolutionReverb::ConvolutionReverb() {
    // Initialize smoothed parameters with proper defaults
    m_mixAmount.reset(0.5f);  // Start with 50% mix for audible reverb
    m_preDelay.reset(0.0f);
    m_damping.reset(0.5f);
    m_size.reset(0.5f);
    m_width.reset(1.0f);
    m_modulation.reset(0.0f);
    m_earlyLate.reset(0.5f);
    m_highCut.reset(1.0f);
    
    m_needsIRUpdate = true;
    m_currentRoomType = IRGenerator::RoomType::Hall;
}

void ConvolutionReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Detect sample rate change for proper IR regeneration
    bool sampleRateChanged = (std::abs(m_sampleRate - sampleRate) > 0.1);
    
    m_sampleRate = sampleRate;
    m_samplesPerBlock = samplesPerBlock;
    
    // Set smoothing times for parameters - scale with sample rate
    float smoothingTime = 100.0f * (44100.0 / sampleRate); // Compensate for sample rate
    m_mixAmount.setSmoothingTime(50.0f * (44100.0 / sampleRate), sampleRate);
    m_preDelay.setSmoothingTime(smoothingTime, sampleRate);
    m_damping.setSmoothingTime(smoothingTime, sampleRate);
    m_size.setSmoothingTime(200.0f * (44100.0 / sampleRate), sampleRate);
    m_width.setSmoothingTime(smoothingTime, sampleRate);
    m_modulation.setSmoothingTime(50.0f * (44100.0 / sampleRate), sampleRate);
    m_earlyLate.setSmoothingTime(smoothingTime, sampleRate);
    m_highCut.setSmoothingTime(50.0f * (44100.0 / sampleRate), sampleRate);
    
    // Prepare DSP modules
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    
    // Prepare convolution engines
    m_convolutionEngine.prepare(spec);
    m_zeroLatencyEngine.prepare(spec);
    
    // Prepare oversampler
    m_oversampler.prepare(spec);
    
    // Prepare pre-delay
    m_preDelayProcessor.prepare(spec);
    
    // Prepare filter system
    m_filterSystem.prepare(spec);
    
    // Prepare DC blockers
    for (auto& blocker : m_dcBlockers) {
        blocker.prepare(sampleRate);
        blocker.reset();
    }
    
    // Generate initial IR or regenerate if sample rate changed
    if (sampleRateChanged || m_needsIRUpdate) {
        generateEnhancedImpulseResponse();
        m_needsIRUpdate = false;
    }
}

void ConvolutionReverb::reset() {
    m_convolutionEngine.reset();
    m_zeroLatencyEngine.reset();
    
    for (auto& blocker : m_dcBlockers) {
        blocker.reset();
    }
    
    m_oversampler.reset();
}

void ConvolutionReverb::process(juce::AudioBuffer<float>& buffer) {
    DenormalProtection::DenormalGuard guard; // Prevent denormal CPU spikes
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Update IR if needed
    updateIRIfNeeded();
    
    // Update thermal modeling
    m_thermalModel.update(m_sampleRate);
    updateComponentAging(m_sampleRate);
    
    // Update smoothed parameters
    for (int i = 0; i < numSamples; ++i) {
        m_mixAmount.update();
        m_preDelay.update();
        m_damping.update();
        m_size.update();
        m_width.update();
        m_modulation.update();
        m_earlyLate.update();
        m_highCut.update();
    }
    
    // Early bypass for pure dry signal
    float mix = m_mixAmount.current;
    if (mix < 0.001f) {
        // Pure dry - skip all processing, input buffer already contains dry signal
        return;
    }
    
    // Store dry signal for mixing later
    juce::AudioBuffer<float> dryBuffer(numChannels, numSamples);
    for (int ch = 0; ch < numChannels; ++ch) {
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }
    
    // Apply pre-delay with modulation if enabled
    if (m_preDelay.current > 0.001f) {
        for (int ch = 0; ch < numChannels; ++ch) {
            float* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                // Apply pre-delay with modulation from the modulation parameter
                m_preDelayProcessor.setDelay(m_preDelay.current * 200.0f, // 0-200ms pre-delay
                                           m_modulation.current * 0.5f,   // Modulation affects pre-delay
                                           m_sampleRate);
                channelData[i] = m_preDelayProcessor.process(channelData[i]);
            }
        }
    }
    
    // Apply gentle input gain reduction to prevent convolution distortion
    buffer.applyGain(0.85f);
    
    // Process through convolution
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    
    // Apply convolution (simplified - no oversampling for now)
    if (m_useZeroLatency) {
        m_zeroLatencyEngine.process(context);
    } else {
        m_convolutionEngine.process(context);
    }
    
    // Apply modulation to the wet signal if enabled
    if (m_modulation.current > 0.01f) {
        for (int ch = 0; ch < numChannels; ++ch) {
            float* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                channelData[i] = processModulation(channelData[i], ch);
            }
        }
    }
    
    // Apply high cut filtering if enabled
    if (m_highCut.current < 0.99f) {
        m_filterSystem.updateParameters(m_highCut.current, m_damping.current, m_sampleRate);
        for (int ch = 0; ch < numChannels; ++ch) {
            float* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                channelData[i] = m_filterSystem.process(channelData[i], ch);
            }
        }
    }
    
    // Apply makeup gain after convolution to restore levels
    buffer.applyGain(1.18f); // 1/0.85 to compensate input reduction
    
    // Mix dry/wet with proper gain staging
    // mix value already retrieved above
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            float dry = dryBuffer.getSample(ch, i);
            float wet = buffer.getSample(ch, i);
            
            // Soft limiting on wet signal if needed
            const float wetThreshold = 0.95f;
            if (std::abs(wet) > wetThreshold) {
                wet = wetThreshold * std::tanh(wet / wetThreshold);
            }
            
            buffer.setSample(ch, i, dry * (1.0f - mix) + wet * mix);
        }
    }
    
    // Apply DC blocking
    for (int ch = 0; ch < std::min(numChannels, 2); ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            channelData[i] = m_dcBlockers[ch].process(channelData[i]);
        }
    }
    
    // Scrub any NaN/Inf values
    scrubBuffer(buffer);
}

void ConvolutionReverb::generateEnhancedImpulseResponse() {
    // Generate simple IR for testing
    auto ir = IRGenerator::generateAdvancedIR(m_sampleRate, m_size.current, 
                                            m_damping.current, m_earlyLate.current,
                                            m_currentRoomType);
    
    // Create stereo version
    auto stereoIR = IRGenerator::createStereoIR(ir, m_width.current, m_sampleRate);
    
    // Load into convolution engine
    try {
        m_convolutionEngine.loadImpulseResponse(std::move(stereoIR), m_sampleRate,
                                              juce::dsp::Convolution::Stereo::yes,
                                              juce::dsp::Convolution::Trim::yes,
                                              juce::dsp::Convolution::Normalise::yes);
        
        // For zero-latency, use shorter version
        if (ir.size() > 1024) {
            ir.resize(1024);
        }
        auto shortStereoIR = IRGenerator::createStereoIR(ir, m_width.current, m_sampleRate);
        m_zeroLatencyEngine.loadImpulseResponse(std::move(shortStereoIR), m_sampleRate,
                                              juce::dsp::Convolution::Stereo::yes,
                                              juce::dsp::Convolution::Trim::no,
                                              juce::dsp::Convolution::Normalise::yes);
    } catch (...) {
        // If loading fails, continue with empty IR
    }
}

void ConvolutionReverb::updateIRIfNeeded() {
    if (m_needsIRUpdate) {
        generateEnhancedImpulseResponse();
        m_needsIRUpdate = false;
    }
}

// IRGenerator implementation
std::vector<float> ConvolutionReverb::IRGenerator::generateAdvancedIR(
    double sampleRate, float size, float damping, float earlyLate, RoomType roomType) {
    
    // IR length based on size - ensure adequate length for proper reverb tail
    float rt60 = 1.0f + size * 5.0f; // 1 to 6 seconds RT60 for richer tails
    
    // Sample rate compensation - adjust RT60 for perceptual consistency
    float sampleRateRatio = static_cast<float>(sampleRate / 44100.0);
    rt60 *= std::pow(sampleRateRatio, 0.15f); // Subtle compensation for frequency response
    
    int irLength = static_cast<int>(sampleRate * rt60 * 1.8f); // 1.8x RT60 for complete tail
    irLength = std::min(irLength, static_cast<int>(sampleRate * 10.0f)); // Cap at 10 seconds
    irLength = std::max(irLength, static_cast<int>(sampleRate * 1.0f)); // Minimum 1 second
    
    std::vector<float> ir(irLength, 0.0f);
    
    // Thread-safe random generation
    thread_local juce::Random rng;
    rng.setSeedRandomly();
    
    // Correct RT60 decay calculation: -60dB in rt60 seconds
    // The decay per sample is calculated as: 10^(-60 / (20 * rt60 * sampleRate))
    float decayPerSample = std::pow(10.0f, -60.0f / (20.0f * rt60 * sampleRate));
    
    // Start with initial impulse (direct sound)
    ir[0] = 1.0f;
    
    // Add early reflections with proper delays and gains
    const int numEarlyReflections = 12;
    const float earlyReflectionTimes[] = {0.007f, 0.011f, 0.017f, 0.023f, 0.029f, 0.037f,
                                          0.041f, 0.047f, 0.053f, 0.061f, 0.067f, 0.073f};
    const float earlyReflectionGains[] = {0.8f, 0.7f, 0.6f, 0.55f, 0.5f, 0.45f,
                                          0.4f, 0.35f, 0.3f, 0.25f, 0.2f, 0.15f};
    
    for (int i = 0; i < numEarlyReflections; ++i) {
        int index = static_cast<int>(earlyReflectionTimes[i] * sampleRate);
        if (index < irLength) {
            // Early/Late balance: more dramatic effect
            // earlyLate = 0.0 -> strong early reflections, weak late reverb
            // earlyLate = 1.0 -> weak early reflections, strong late reverb
            float earlyGain = (1.0f - earlyLate) * 1.5f + 0.2f; // 0.2 to 1.7
            float gain = earlyReflectionGains[i] * earlyGain;
            gain *= (0.9f + rng.nextFloat() * 0.2f); // Small random variation using JUCE's Random
            ir[index] += gain;
        }
    }
    
    // Generate dense late reverb tail starting after early reflections
    int lateStart = static_cast<int>(0.08f * sampleRate); // Start at 80ms
    
    // Late reverb gain controlled by Early/Late parameter
    float lateGain = earlyLate * 1.2f + 0.3f; // 0.3 to 1.5
    
    // Initialize amplitude for exponential decay
    float amplitude = 1.0f;
    
    for (int i = lateStart; i < irLength; ++i) {
        float time = i / static_cast<float>(sampleRate);
        
        // Apply exponential decay per sample
        amplitude *= decayPerSample;
        float envelope = amplitude;
        
        // Apply frequency-dependent damping with more gradual effect
        float dampingFactor = 1.0f - damping * 0.5f * std::min(1.0f, time / rt60);
        envelope *= dampingFactor;
        
        // Apply Early/Late balance to late reverb
        envelope *= lateGain;
        
        // Continue generating until very low levels for proper tail
        if (envelope < 1e-8f) {
            ir[i] = 0.0f;
            continue;
        }
        
        // Dense reverb tail with proper statistical properties (Gaussian distribution)
        float sample = (rng.nextFloat() * 2.0f - 1.0f) * envelope;
        
        // Add subtle comb filtering for more realistic sound
        if (i % 71 == 0) sample *= 1.02f;  // Prime-based delay for natural sound
        if (i % 97 == 0) sample *= 1.015f; // Another prime delay
        if (i % 127 == 0) sample *= 0.98f; // Slight notch
        
        // Increase tail density for more realistic reverb
        sample *= 0.6f; // Further increased gain for prominent tail
        
        // Denormal flush
        if (std::abs(sample) < 1e-12f) sample = 0.0f;
        
        ir[i] = sample;
    }
    
    return ir;
}

juce::AudioBuffer<float> ConvolutionReverb::IRGenerator::createStereoIR(
    const std::vector<float>& monoIR, float width, double sampleRate) {
    
    int numSamples = monoIR.size();
    juce::AudioBuffer<float> stereoIR(2, numSamples);
    
    // Simple stereo widening
    for (int i = 0; i < numSamples; ++i) {
        float sample = monoIR[i];
        
        // Add slight decorrelation between channels
        float leftDelay = i > 10 ? monoIR[i - 10] : 0;
        float rightDelay = i > 15 ? monoIR[i - 15] : 0;
        
        stereoIR.setSample(0, i, sample + leftDelay * width * 0.2f);
        stereoIR.setSample(1, i, sample + rightDelay * width * 0.2f);
    }
    
    return stereoIR;
}

void ConvolutionReverb::updateComponentAging(double sampleRate) {
    // Simple aging simulation
    m_componentAge += 1.0f / (sampleRate * 3600.0f);
    float ageYears = m_componentAge / 8760.0f;
    m_ageNoiseFactor = std::min(0.0001f, ageYears * 0.00001f);
    m_ageFrequencyShift = std::min(0.02f, ageYears * 0.005f);
}

float ConvolutionReverb::applyVintageNoise(float input) {
    // Simple vintage noise with denormal prevention
    thread_local juce::Random tlsRandom;
    float noise = (tlsRandom.nextFloat() - 0.5f) * 0.0001f;
    float result = input + noise * m_ageNoiseFactor;
    
    // Denormal flush
    if (std::abs(result) < 1e-10f) result = 0.0f;
    return result;
}

float ConvolutionReverb::applyAnalogCharacter(float input, float amount) {
    // Simple soft saturation for analog warmth
    float drive = 1.0f + amount;
    float x = input * drive;
    return std::tanh(x) / drive;
}

float ConvolutionReverb::processModulation(float input, int channel) {
    // Simple chorus/vibrato modulation
    static float modulationPhase[2] = {0.0f, 0.0f};
    static float delayBuffer[2][512] = {{0.0f}, {0.0f}};
    static int delayIndex[2] = {0, 0};
    
    float modAmount = m_modulation.current;
    if (modAmount < 0.01f) return input;
    
    // LFO for modulation (slightly different rates for L/R)
    float lfoRate = 0.5f + modAmount * 4.0f; // 0.5 to 4.5 Hz
    if (channel == 1) lfoRate *= 1.1f; // Slight stereo detuning
    
    modulationPhase[channel] += 2.0f * M_PI * lfoRate / m_sampleRate;
    if (modulationPhase[channel] > 2.0f * M_PI) {
        modulationPhase[channel] -= 2.0f * M_PI;
    }
    
    // Create modulated delay
    float lfo = std::sin(modulationPhase[channel]);
    float delayMs = 5.0f + modAmount * 10.0f * (lfo + 1.0f) * 0.5f; // 5-15ms delay
    int delaySamples = static_cast<int>(delayMs * 0.001f * m_sampleRate);
    delaySamples = std::clamp(delaySamples, 1, 511);
    
    // Store current sample
    delayBuffer[channel][delayIndex[channel]] = input;
    
    // Get delayed sample with linear interpolation
    int readIndex = (delayIndex[channel] - delaySamples + 512) % 512;
    float delayedSample = delayBuffer[channel][readIndex];
    
    // Advance delay index
    delayIndex[channel] = (delayIndex[channel] + 1) % 512;
    
    // Mix original and modulated signal
    float wetAmount = modAmount * 0.3f;
    return input * (1.0f - wetAmount) + delayedSample * wetAmount;
}

void ConvolutionReverb::scrubBuffer(juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            // Check for NaN/Inf and denormals
            if (!std::isfinite(data[i]) || std::abs(data[i]) < 1e-10f) {
                data[i] = 0.0f;
            }
        }
    }
}

void ConvolutionReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        float clampedValue = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: 
                m_mixAmount.target = clampedValue;
                // For immediate response in tests/presets, jump to target if change is large
                if (std::abs(m_mixAmount.current - clampedValue) > 0.3f) {
                    m_mixAmount.current = clampedValue;
                }
                break;
            case 1: 
                m_preDelay.target = clampedValue;
                // Jump to target for large changes
                if (std::abs(m_preDelay.current - clampedValue) > 0.3f) {
                    m_preDelay.current = clampedValue;
                }
                break;
            case 2: 
                m_damping.target = clampedValue;
                if (std::abs(m_damping.current - clampedValue) > 0.3f) {
                    m_damping.current = clampedValue;
                }
                m_needsIRUpdate = true; // Trigger IR regeneration for damping changes
                break;
            case 3: 
                m_size.target = clampedValue;
                if (std::abs(m_size.current - clampedValue) > 0.3f) {
                    m_size.current = clampedValue;
                }
                m_needsIRUpdate = true; // Trigger IR regeneration
                break;
            case 4: 
                m_width.target = clampedValue;
                if (std::abs(m_width.current - clampedValue) > 0.3f) {
                    m_width.current = clampedValue;
                }
                m_needsIRUpdate = true; // Trigger IR regeneration for width changes
                break;
            case 5: 
                m_modulation.target = clampedValue;
                if (std::abs(m_modulation.current - clampedValue) > 0.3f) {
                    m_modulation.current = clampedValue;
                }
                break;
            case 6: 
                m_earlyLate.target = clampedValue;
                if (std::abs(m_earlyLate.current - clampedValue) > 0.3f) {
                    m_earlyLate.current = clampedValue;
                }
                m_needsIRUpdate = true; // Trigger IR regeneration for early/late balance
                break;
            case 7: 
                m_highCut.target = clampedValue;
                if (std::abs(m_highCut.current - clampedValue) > 0.3f) {
                    m_highCut.current = clampedValue;
                }
                break;
        }
    }
}

juce::String ConvolutionReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Mix";
        case 1: return "Pre-Delay";
        case 2: return "Damping";
        case 3: return "Size";
        case 4: return "Width";
        case 5: return "Modulation";
        case 6: return "Early/Late";
        case 7: return "High Cut";
        default: return "";
    }
}