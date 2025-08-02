#include "LadderFilter.h"
#include <cmath>
#include <algorithm>
#include <random>

LadderFilter::LadderFilter() {
    // Initialize smoothed parameters
    m_cutoffFreq.reset(0.5f);
    m_resonance.reset(0.3f);
    m_drive.reset(0.2f);
    m_filterType.reset(0.0f);
    m_asymmetry.reset(0.0f);
    m_vintageMode.reset(0.0f);
    
    // Generate nonlinear lookup tables
    m_coeffs.generateLUTs();
}

void LadderFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    float smoothingTime = 20.0f; // 20ms for cutoff/resonance
    m_cutoffFreq.setSmoothingTime(smoothingTime, sampleRate);
    m_resonance.setSmoothingTime(smoothingTime, sampleRate);
    m_drive.setSmoothingTime(100.0f, sampleRate);  // Slower for drive
    m_filterType.setSmoothingTime(50.0f, sampleRate);
    m_asymmetry.setSmoothingTime(200.0f, sampleRate);
    m_vintageMode.setSmoothingTime(500.0f, sampleRate);
    
    // Prepare channel states
    for (auto& channel : m_channelStates) {
        channel.prepare();
    }
    
    // Initialize filter coefficients
    m_coeffs.updateCoefficients(m_cutoffFreq.current, m_resonance.current, 
                               m_asymmetry.current, m_vintageMode.current > 0.5f, sampleRate);
}

void LadderFilter::reset() {
    // Reset all channel states
    for (auto& channel : m_channelStates) {
        channel.prepare(); // This already does comprehensive reset
    }
    
    // Reset thermal model
    m_thermalModel.ambientTemp = 25.0f;
    m_thermalModel.thermalNoise = 0.0f;
    
    // Reset DC blockers
    for (auto& blocker : m_dcBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
}

void LadderFilter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update thermal modeling
    m_thermalModel.update(m_sampleRate);
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update smoothed parameters
            m_cutoffFreq.update();
            m_resonance.update();
            m_drive.update();
            m_filterType.update();
            m_asymmetry.update();
            m_vintageMode.update();
            
            // Update filter coefficients if needed
            static int updateCounter = 0;
            if (++updateCounter >= 16) { // Update every 16 samples
                updateCounter = 0;
                m_coeffs.updateCoefficients(m_cutoffFreq.current, m_resonance.current,
                                          m_asymmetry.current, m_vintageMode.current > 0.5f, m_sampleRate);
            }
            
            channelData[sample] = processSample(channelData[sample], channel);
        }
    }
}

float LadderFilter::processSample(float input, int channel) {
    // Apply DC blocking
    float cleanInput = m_dcBlockers[channel].process(input);
    
    // Use oversampling for high-quality processing
    return processOversampled(cleanInput, channel);
}

float LadderFilter::processLadderCore(float input, int channel) {
    auto& state = m_channelStates[channel];
    
    // Apply input drive with thermal compensation
    float thermalFactor = m_thermalModel.getThermalFactor();
    float driveAmount = (m_drive.current * 5.0f + 0.1f) * thermalFactor;
    
    float drivenInput;
    if (m_vintageMode.current > 0.5f) {
        drivenInput = vintageSaturation(input, driveAmount);
    } else {
        drivenInput = transistorSaturation(input, driveAmount, m_asymmetry.current);
    }
    
    // Calculate feedback
    float feedback = calculateFeedback(state, m_coeffs.k, m_coeffs.alpha);
    
    // Input with feedback subtraction
    float inputWithFeedback = drivenInput - feedback;
    
    // Process through 4 cascaded stages
    float stageInput = inputWithFeedback;
    for (int i = 0; i < 4; ++i) {
        stageInput = processLadderStage(stageInput, state.stages[i], 
                                      m_coeffs.g * thermalFactor, driveAmount, i == 0);
    }
    
    // Calculate different filter responses
    return calculateFilterResponse(state, inputWithFeedback, m_filterType.current);
}

float LadderFilter::processOversampled(float input, int channel) {
    auto& state = m_channelStates[channel];
    
    // Upsample to 4x for better quality
    std::array<float, ChannelState::Oversampler::OVERSAMPLE_FACTOR> oversampledInput;
    oversampledInput[0] = input * ChannelState::Oversampler::OVERSAMPLE_FACTOR;
    for (int i = 1; i < ChannelState::Oversampler::OVERSAMPLE_FACTOR; ++i) {
        oversampledInput[i] = 0.0f; // Zero-stuff
    }
    
    // Anti-aliasing filter coefficients (Butterworth 8th order approx)
    static const float aaCoeffs[] = {0.0179f, 0.0716f, 0.1075f, 0.0716f, 0.0179f};
    
    float output = 0.0f;
    
    // Process oversampled samples
    for (int i = 0; i < ChannelState::Oversampler::OVERSAMPLE_FACTOR; ++i) {
        float filtered = state.oversampler.upsampleFilter.process(oversampledInput[i], aaCoeffs);
        
        // Main ladder filter processing
        float ladderOutput = processLadderCore(filtered, channel);
        
        // Downsample filter
        float downsampled = state.oversampler.downsampleFilter.process(ladderOutput, aaCoeffs);
        
        // Accumulate (only the non-zero-stuffed samples contribute)
        if (i == 0) output = downsampled;
    }
    
    return output / ChannelState::Oversampler::OVERSAMPLE_FACTOR;
}

float LadderFilter::processLadderCore(float input, int channel) {
    auto& state = m_channelStates[channel];
    
    // Apply input drive with thermal compensation
    float thermalFactor = m_thermalModel.getThermalFactor();
    float driveAmount = (m_drive.current * 5.0f + 0.1f) * thermalFactor;
    
    float drivenInput;
    if (m_vintageMode.current > 0.5f) {
        drivenInput = vintageSaturation(input, driveAmount);
    } else {
        drivenInput = transistorSaturation(input, driveAmount, m_asymmetry.current);
    }
    
    // Calculate feedback
    float feedback = calculateFeedback(state, m_coeffs.k, m_coeffs.alpha);
    
    // Input with feedback subtraction
    float inputWithFeedback = drivenInput - feedback;
    
    // Process through 4 cascaded stages
    float stageInput = inputWithFeedback;
    for (int i = 0; i < 4; ++i) {
        stageInput = processLadderStage(stageInput, state.stages[i], 
                                      m_coeffs.g * thermalFactor, driveAmount, i == 0);
    }
    
    // Calculate different filter responses
    return calculateFilterResponse(state, inputWithFeedback, m_filterType.current);
}

float LadderFilter::processLadderStage(float input, LadderStage& stage, float g, 
                                       float drive, bool isFirst) {
    // Huovilainen's improved model with delay-free feedback
    
    // Input processing with nonlinearity
    float processedInput = input;
    if (isFirst) {
        // First stage gets more drive
        processedInput = transistorSaturation(input, drive * 1.5f, m_asymmetry.current);
    } else {
        // Subsequent stages have milder saturation
        processedInput = softClip(input * (1.0f + drive * 0.2f));
    }
    
    // Component aging/drift simulation
    stage.componentDrift += (((rand() % 1000) / 1000.0f - 0.5f) * 0.0001f) / m_sampleRate;
    stage.componentDrift = std::max(-0.01f, std::min(0.01f, stage.componentDrift));
    
    float effectiveG = g * (1.0f + stage.componentDrift);
    effectiveG = std::max(0.0f, std::min(0.99f, effectiveG));
    
    // Enhanced one-pole filter with better numerical properties
    float integrator = effectiveG * processedInput + (1.0f - effectiveG) * stage.state;
    
    // Update stage state
    stage.delay = stage.state;
    stage.state = integrator;
    
    // Capacitor leakage (very subtle high-frequency loss)
    stage.state *= 0.9999f;
    
    return stage.state;
}

float LadderFilter::calculateFeedback(const ChannelState& state, float k, float alpha) {
    // Huovilainen's delay-free feedback calculation
    float y4 = state.stages[3].state;
    float y3 = state.stages[2].state;
    
    // Compensation for delay in feedback path
    float compensatedFeedback = y4 + alpha * (y4 - state.stages[3].delay);
    
    return k * compensatedFeedback;
}

float LadderFilter::calculateFilterResponse(const ChannelState& state, float input, float filterType) {
    // Extract outputs from different stages
    float stage1 = state.stages[0].state;
    float stage2 = state.stages[1].state;
    float stage3 = state.stages[2].state;
    float stage4 = state.stages[3].state;
    
    // Calculate different filter responses
    float lowpass = stage4;
    float bandpass = stage2 - stage4;
    float highpass = input - stage1;
    float notch = lowpass + highpass;
    
    // Smooth morphing between filter types
    float output;
    if (filterType < 0.333f) {
        // Lowpass to bandpass transition
        float mix = filterType * 3.0f;
        output = lowpass * (1.0f - mix) + bandpass * mix;
    } else if (filterType < 0.666f) {
        // Bandpass to highpass transition
        float mix = (filterType - 0.333f) * 3.0f;
        output = bandpass * (1.0f - mix) + highpass * mix;
    } else {
        // Highpass to notch transition
        float mix = (filterType - 0.666f) * 3.0f;
        output = highpass * (1.0f - mix) + notch * mix;
    }
    
    return output;
}

float LadderFilter::transistorSaturation(float input, float drive, float asymmetry) {
    // Accurate transistor saturation modeling
    float driven = input * drive;
    
    // Asymmetric clipping based on transistor characteristics
    if (driven > 0.0f) {
        float pos_factor = 0.7f + asymmetry * 0.3f;
        return std::tanh(driven * pos_factor) / (pos_factor * drive);
    } else {
        float neg_factor = 0.9f - asymmetry * 0.2f;
        return std::tanh(driven * neg_factor) / (neg_factor * drive);
    }
}

float LadderFilter::vintageSaturation(float input, float drive) {
    // Vintage Moog saturation with even harmonics
    float driven = input * drive;
    
    // Add subtle even harmonic distortion
    float saturated = std::tanh(driven * 0.8f);
    float evenHarmonics = driven * driven * 0.1f;
    
    return (saturated + evenHarmonics) / drive;
}

// FilterCoefficients implementation
void LadderFilter::FilterCoefficients::updateCoefficients(float cutoff, float resonance, 
                                                         float asymmetry, bool vintageMode, 
                                                         double sampleRate) {
    // Calculate cutoff frequency in Hz
    float cutoffHz = 10.0f + cutoff * cutoff * 19990.0f; // Exponential scaling
    
    // Pre-warp for bilinear transform (4x oversampled)
    float omega = 2.0f * M_PI * cutoffHz / (sampleRate * 4.0f);
    float g_unwrapped = std::tan(omega * 0.5f);
    
    // Calculate g coefficient with temperature compensation
    g = g_unwrapped / (1.0f + g_unwrapped);
    g = std::min(g, 0.995f); // Prevent instability
    
    // Enhanced resonance calculation
    if (vintageMode) {
        // Vintage mode - more musical resonance curve
        k = resonance * resonance * 4.2f;
        alpha = 1.0f / (1.0f + k); // Simple compensation
    } else {
        // Modern mode - linear resonance with better compensation
        k = resonance * 4.0f;
        alpha = 1.0f / (1.0f + k * 0.5f); // Better compensation
    }
    
    // Limit feedback to prevent instability
    k = std::min(k, 3.95f);
}

void LadderFilter::FilterCoefficients::generateLUTs() {
    // Generate saturation lookup tables for efficiency
    for (int i = 0; i < 256; ++i) {
        float x = (i - 128) / 64.0f; // -2 to +2 range
        saturationLUT[i] = std::tanh(x);
        
        // Resonance compensation LUT
        float res = i / 255.0f;
        resonanceLUT[i] = 1.0f / (1.0f + res * res * 16.0f);
    }
}

// Oversampler implementation
float LadderFilter::ChannelState::Oversampler::AAFilter::process(float input, const float* coeffs) {
    // Shift input delays
    for (int i = 3; i > 0; --i) {
        x[i] = x[i-1];
    }
    x[0] = input;
    
    // Calculate output (FIR filter)
    float output = 0.0f;
    for (int i = 0; i < 4; ++i) {
        output += x[i] * coeffs[i];
    }
    
    // Shift output delays
    for (int i = 3; i > 0; --i) {
        y[i] = y[i-1];
    }
    y[0] = output;
    
    return output;
}

void LadderFilter::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) m_cutoffFreq.target = params.at(0);
    if (params.find(1) != params.end()) m_resonance.target = params.at(1);
    if (params.find(2) != params.end()) m_drive.target = params.at(2);
    if (params.find(3) != params.end()) m_filterType.target = params.at(3);
    if (params.find(4) != params.end()) m_asymmetry.target = params.at(4);
    if (params.find(5) != params.end()) m_vintageMode.target = params.at(5);
}

juce::String LadderFilter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Cutoff";
        case 1: return "Resonance";
        case 2: return "Drive";
        case 3: return "Filter Type";
        case 4: return "Asymmetry";
        case 5: return "Vintage Mode";
        default: return "";
    }
}