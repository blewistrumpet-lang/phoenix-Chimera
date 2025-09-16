// ==================== ConvolutionReverb_Fixed.cpp ====================
// Simplified version that works without crashes
#include "ConvolutionReverb.h"
#include <cmath>
#include <random>
#include <algorithm>

ConvolutionReverb::ConvolutionReverb() {
    // Initialize smoothed parameters with proper defaults
    m_mixAmount.reset(0.5f);
    m_preDelay.reset(0.0f);
    m_damping.reset(0.5f);
    m_size.reset(0.5f);
    m_width.reset(1.0f);
    m_modulation.reset(0.0f);
    m_earlyLate.reset(0.5f);
    m_highCut.reset(1.0f);
    
    m_needsIRUpdate = true;
    m_currentRoomType = RoomType::Hall;
}

void ConvolutionReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_samplesPerBlock = samplesPerBlock;
    
    // Set smoothing times for parameters
    float smoothingTime = 100.0f;
    m_mixAmount.setSmoothingTime(50.0f, sampleRate);
    m_preDelay.setSmoothingTime(smoothingTime, sampleRate);
    m_damping.setSmoothingTime(smoothingTime, sampleRate);
    m_size.setSmoothingTime(200.0f, sampleRate);
    m_width.setSmoothingTime(smoothingTime, sampleRate);
    m_modulation.setSmoothingTime(50.0f, sampleRate);
    m_earlyLate.setSmoothingTime(smoothingTime, sampleRate);
    m_highCut.setSmoothingTime(50.0f, sampleRate);
    
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
    
    // Generate initial IR
    generateEnhancedImpulseResponse();
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
    
    // Early bypass check for mix parameter
    if (m_mixAmount.current < 0.001f) {
        // Completely dry - no processing needed, parameters already updated
        return;
    }
    
    // Store dry signal
    juce::AudioBuffer<float> dryBuffer(numChannels, numSamples);
    for (int ch = 0; ch < numChannels; ++ch) {
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }
    
    // Process through convolution
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    
    // Apply convolution (simplified - no oversampling for now)
    if (m_useZeroLatency) {
        m_zeroLatencyEngine.process(context);
    } else {
        m_convolutionEngine.process(context);
    }
    
    // Mix dry/wet
    float mix = m_mixAmount.current;
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            float dry = dryBuffer.getSample(ch, i);
            float wet = buffer.getSample(ch, i);
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
        m_convolutionEngine.loadImpulseResponse(std::move(stereoIR), 
                                              juce::dsp::Convolution::Stereo::yes,
                                              juce::dsp::Convolution::Trim::yes,
                                              juce::dsp::Convolution::Normalise::yes);
        
        // For zero-latency, use shorter version
        if (ir.size() > 1024) {
            ir.resize(1024);
        }
        auto shortStereoIR = IRGenerator::createStereoIR(ir, m_width.current, m_sampleRate);
        m_zeroLatencyEngine.loadImpulseResponse(std::move(shortStereoIR),
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
    
    // IR length based on size (0.5 to 3 seconds max for stability)
    int irLength = static_cast<int>(sampleRate * (0.5f + size * 2.5f));
    irLength = std::min(irLength, static_cast<int>(sampleRate * 3.0)); // Cap at 3 seconds
    
    std::vector<float> ir(irLength, 0.0f);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    // Simple exponential decay reverb
    float rt60 = 0.5f + size * 2.5f; // 0.5 to 3 seconds RT60
    float decayRate = -3.0f / rt60;
    
    // Generate basic reverb tail
    for (int i = 0; i < irLength; ++i) {
        float time = i / static_cast<float>(sampleRate);
        float envelope = std::exp(decayRate * time);
        
        // Apply damping
        envelope *= (1.0f - damping * time / rt60 * 0.5f);
        
        // Random noise shaped by envelope
        ir[i] = dist(gen) * envelope * 0.5f;
    }
    
    // Add some early reflections
    int numEarlyReflections = 8;
    for (int i = 0; i < numEarlyReflections; ++i) {
        float time = 0.005f + i * 0.01f; // 5ms to 85ms
        int index = static_cast<int>(time * sampleRate);
        if (index < irLength) {
            ir[index] += (1.0f - i / static_cast<float>(numEarlyReflections)) * 0.3f * earlyLate;
        }
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
    // Simple vintage noise
    thread_local juce::Random tlsRandom;
    float noise = (tlsRandom.nextFloat() - 0.5f) * 0.0001f;
    return input + noise * m_ageNoiseFactor;
}

float ConvolutionReverb::applyAnalogCharacter(float input, float amount) {
    // Simple soft saturation for analog warmth
    float drive = 1.0f + amount;
    float x = input * drive;
    return std::tanh(x) / drive;
}

void ConvolutionReverb::scrubBuffer(juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (!std::isfinite(data[i])) {
                data[i] = 0.0f;
            }
        }
    }
}

void ConvolutionReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        float clampedValue = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: m_mixAmount.target = clampedValue; break;
            case 1: m_preDelay.target = clampedValue; break;
            case 2: m_damping.target = clampedValue; break;
            case 3: 
                m_size.target = clampedValue; 
                m_needsIRUpdate = true; // Trigger IR regeneration
                break;
            case 4: m_width.target = clampedValue; break;
            case 5: m_modulation.target = clampedValue; break;
            case 6: m_earlyLate.target = clampedValue; break;
            case 7: m_highCut.target = clampedValue; break;
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