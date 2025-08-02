#include "MultibandSaturator.h"
#include <algorithm>

MultibandSaturator::MultibandSaturator() {
    m_channels.resize(2);
    
    // Initialize smooth parameters
    m_lowDrive.setImmediate(1.0f);
    m_midDrive.setImmediate(1.0f);
    m_highDrive.setImmediate(1.0f);
    m_saturationType.setImmediate(0.0f);
    m_harmonicCharacter.setImmediate(0.5f);
    
    // Set smoothing rates
    m_lowDrive.setSmoothingRate(0.995f);
    m_midDrive.setSmoothingRate(0.995f);
    m_highDrive.setSmoothingRate(0.995f);
    m_saturationType.setSmoothingRate(0.998f);
    m_harmonicCharacter.setSmoothingRate(0.995f);
}

void MultibandSaturator::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize channels
    for (auto& channel : m_channels) {
        channel.init(sampleRate);
        channel.reset();
    }

void MultibandSaturator::reset() {
    // Reset all internal state
    // TODO: Implement specific reset logic for MultibandSaturator
}

    
    // Initialize DC blockers
    m_inputDCBlockers.resize(2);
    m_outputDCBlockers.resize(2);
    
    // Prepare oversampler
    if (m_useOversampling) {
        m_oversampler.prepare(samplesPerBlock);
    }
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    
    // Reset thermal model
    m_thermalModel = ThermalModel();
}

void MultibandSaturator::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_lowDrive.update();
    m_midDrive.update();
    m_highDrive.update();
    m_saturationType.update();
    m_harmonicCharacter.update();
    
    // Update thermal model
    m_thermalModel.update(m_sampleRate);
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update component aging (very slow)
    m_sampleCount += numSamples;
    if (m_sampleCount > m_sampleRate * 5) { // Every 5 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.0001f);
        m_sampleCount = 0;
        
        // Update channel aging
        for (auto& channel : m_channels) {
            channel.updateAging(m_componentAge);
        }
    }
    
    // Determine saturation type
    SaturationType satType;
    if (m_saturationType.current < 0.25f) {
        satType = TUBE;
    } else if (m_saturationType.current < 0.5f) {
        satType = TAPE;
    } else if (m_saturationType.current < 0.75f) {
        satType = TRANSISTOR;
    } else {
        satType = DIGITAL;
    }
    
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        auto& state = m_channels[ch % m_channels.size()];
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            
            // DC block input
            input = m_inputDCBlockers[ch].process(input);
            
            // Split into bands using Linkwitz-Riley crossover with aging
            state.lowBand = state.lowpass1.processWithAging(input, m_componentAge);
            float midHigh = state.highpass1.processWithAging(input, m_componentAge);
            
            // Second split: mid vs high
            state.midBand = state.lowpass2.processWithAging(midHigh, m_componentAge);
            state.highBand = state.highpass2.processWithAging(midHigh, m_componentAge);
            
            // Apply saturation to each band with thermal and aging effects
            float lowSat = processComponentModeling(state.lowBand, m_lowDrive.current, satType, thermalFactor, m_componentAge);
            float midSat = processComponentModeling(state.midBand, m_midDrive.current, satType, thermalFactor, m_componentAge);
            float highSat = processComponentModeling(state.highBand, m_highDrive.current, satType, thermalFactor, m_componentAge);
            
            // Recombine bands
            float output = lowSat + midSat + highSat;
            
            // Add subtle noise floor for analog realism
            output += state.noiseFloor * thermalFactor;
            
            // DC block output
            output = m_outputDCBlockers[ch].process(output);
            
            // Soft limiting with thermal variation
            float limitThreshold = 0.7f * thermalFactor;
            output = std::tanh(output * limitThreshold) * (1.4f / limitThreshold);
            
            // Mix with dry signal (70% wet, modulated by aging)
            float wetAmount = 0.7f * (1.0f - m_componentAge * 0.1f);
            channelData[sample] = input * (1.0f - wetAmount) + output * wetAmount;
        }
    }
}

void MultibandSaturator::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    // Drive parameters: 0-10x gain
    m_lowDrive.target = 1.0f + getParam(0, 0.0f) * 9.0f;
    m_midDrive.target = 1.0f + getParam(1, 0.0f) * 9.0f;
    m_highDrive.target = 1.0f + getParam(2, 0.0f) * 9.0f;
    
    // Saturation type: 0-1 mapped to 4 types
    m_saturationType.target = getParam(3, 0.0f);
    
    // Harmonic character: 0 = even, 1 = odd
    m_harmonicCharacter.target = getParam(4, 0.5f);
}

juce::String MultibandSaturator::getParameterName(int index) const {
    switch (index) {
        case 0: return "Low Drive";
        case 1: return "Mid Drive";
        case 2: return "High Drive";
        case 3: return "Saturation";
        case 4: return "Harmonics";
        default: return "";
    }
}

void MultibandSaturator::ChannelState::init(double sampleRate) {
    // Configure crossover filters
    lowpass1.calculateCoefficients(LOW_CROSSOVER, sampleRate, false);
    highpass1.calculateCoefficients(LOW_CROSSOVER, sampleRate, true);
    lowpass2.calculateCoefficients(HIGH_CROSSOVER, sampleRate, false);
    highpass2.calculateCoefficients(HIGH_CROSSOVER, sampleRate, true);
}

void MultibandSaturator::ChannelState::reset() {
    lowpass1.reset();
    highpass1.reset();
    lowpass2.reset();
    highpass2.reset();
    lowBand = midBand = highBand = 0.0f;
    componentDrift = 0.0f;
    noiseFloor = 0.0f;
}

void MultibandSaturator::ChannelState::updateAging(float aging) {
    componentDrift = aging * 0.02f; // 2% max drift
    noiseFloor = aging * 0.0001f;   // Very subtle noise floor
}

void MultibandSaturator::LinkwitzRileyFilter::calculateCoefficients(float frequency, double sampleRate, bool highpass) {
    // Calculate normalized frequency
    float omega = 2.0f * M_PI * frequency / sampleRate;
    float cosOmega = cos(omega);
    float sinOmega = sin(omega);
    
    // Butterworth Q = 1/sqrt(2)
    float Q = 0.7071f;
    float alpha = sinOmega / (2.0f * Q);
    
    if (highpass) {
        // Highpass coefficients
        b1 = -2.0f * cosOmega;
        b2 = 1.0f - alpha;
        a0 = (1.0f + cosOmega) / 2.0f;
        a1 = -(1.0f + cosOmega);
        a2 = a0;
    } else {
        // Lowpass coefficients
        b1 = -2.0f * cosOmega;
        b2 = 1.0f - alpha;
        a0 = (1.0f - cosOmega) / 2.0f;
        a1 = 1.0f - cosOmega;
        a2 = a0;
    }
    
    // Normalize
    float norm = 1.0f / (1.0f + alpha);
    a0 *= norm;
    a1 *= norm;
    a2 *= norm;
    b1 *= norm;
    b2 *= norm;
}

float MultibandSaturator::LinkwitzRileyFilter::process(float input) {
    // First section
    float y1 = a0 * input + a1 * x1_1 + a2 * x2_1 - b1 * y1_1 - b2 * y2_1;
    x2_1 = x1_1;
    x1_1 = input;
    y2_1 = y1_1;
    y1_1 = y1;
    
    // Second section (cascade for LR4)
    float y2 = a0 * y1 + a1 * x1_2 + a2 * x2_2 - b1 * y1_2 - b2 * y2_2;
    x2_2 = x1_2;
    x1_2 = y1;
    y2_2 = y1_2;
    y1_2 = y2;
    
    return y2;
}

float MultibandSaturator::LinkwitzRileyFilter::processWithAging(float input, float aging) {
    // Process normally, then apply aging effects
    float output = process(input);
    
    // Component aging affects frequency response
    if (aging > 0.01f) {
        // Slight frequency drift and reduced precision
        float drift = aging * 0.05f * ((rand() % 1000) / 1000.0f - 0.5f);
        output *= (1.0f + drift);
        
        // Increased noise and nonlinearity
        if (std::abs(output) > 0.1f) {
            output += aging * 0.001f * ((rand() % 1000) / 1000.0f - 0.5f);
        }
    }
    
    return output;
}

void MultibandSaturator::LinkwitzRileyFilter::reset() {
    x1_1 = x2_1 = y1_1 = y2_1 = 0.0f;
    x1_2 = x2_2 = y1_2 = y2_2 = 0.0f;
}

float MultibandSaturator::applySaturation(float input, float drive, SaturationType type) {
    switch (type) {
        case TUBE:
            return saturateTube(input, drive, m_harmonicCharacter.current);
        case TAPE:
            return saturateTape(input, drive, m_harmonicCharacter.current);
        case TRANSISTOR:
            return saturateTransistor(input, drive, m_harmonicCharacter.current);
        case DIGITAL:
            return saturateDigital(input, drive, m_harmonicCharacter.current);
        default:
            return input;
    }
}

float MultibandSaturator::processComponentModeling(float input, float drive, SaturationType type, float thermalFactor, float aging) {
    // Apply thermal and aging modulation to drive
    float modulatedDrive = drive * thermalFactor * (1.0f - aging * 0.1f);
    
    // Basic saturation
    float saturated = applySaturation(input, modulatedDrive, type);
    
    // Component tolerances simulation
    float tolerance = 1.0f + aging * 0.05f * ((rand() % 1000) / 1000.0f - 0.5f);
    saturated *= tolerance;
    
    // Frequency-dependent aging effects
    if (aging > 0.1f) {
        // High frequency roll-off due to aging
        static float hfState = 0.0f;
        float cutoff = 0.02f * (1.0f - aging * 0.3f); // Reduces with age
        hfState += (saturated - hfState) * cutoff;
        saturated = saturated * 0.7f + hfState * 0.3f;
    }
    
    return saturated;
}

float MultibandSaturator::saturateTube(float input, float drive, float harmonics) {
    // Tube-style saturation with asymmetry
    float x = input;
    
    // Asymmetric waveshaping
    if (x > 0) {
        x = 1.0f - exp(-x * 2.0f);
    } else {
        x = -1.0f + exp(x * 1.5f);
    }
    
    // Add harmonic coloration
    x = shapeHarmonics(x, harmonics);
    
    return x * 0.7f;
}

float MultibandSaturator::saturateTape(float input, float drive, float harmonics) {
    // Tape-style soft saturation
    float x = input;
    
    // Soft knee compression
    float threshold = 0.5f;
    if (std::abs(x) > threshold) {
        float over = std::abs(x) - threshold;
        x = x > 0 ? threshold + tanh(over) * (1.0f - threshold)
                  : -threshold - tanh(over) * (1.0f - threshold);
    }
    
    // Hysteresis simulation (simplified)
    x = x + 0.1f * sin(x * 3.0f) * harmonics;
    
    return x;
}

float MultibandSaturator::saturateTransistor(float input, float drive, float harmonics) {
    // Transistor-style hard clipping with crossover distortion
    float x = input;
    
    // Crossover distortion
    float crossover = 0.05f * (1.0f - harmonics);
    if (std::abs(x) < crossover) {
        x = 0.0f;
    } else {
        x = x > 0 ? x - crossover : x + crossover;
    }
    
    // Hard clipping
    x = std::max(-0.9f, std::min(0.9f, x * 1.5f));
    
    // Add odd harmonics
    x = x - 0.1f * x * x * x * harmonics;
    
    return x * 0.8f;
}

float MultibandSaturator::saturateDigital(float input, float drive, float harmonics) {
    // Digital-style waveshaping
    float x = input;
    
    // Bit-crush style quantization
    float bits = 8.0f + harmonics * 8.0f; // 8-16 bits
    float levels = pow(2.0f, bits);
    x = round(x * levels) / levels;
    
    // Foldback distortion
    while (std::abs(x) > 1.0f) {
        x = x > 1.0f ? 2.0f - x : -2.0f - x;
    }
    
    return x;
}

float MultibandSaturator::shapeHarmonics(float x, float evenOddBalance) {
    // Balance between even (warm) and odd (harsh) harmonics
    float even = x * x * (x > 0 ? 1.0f : -1.0f); // Preserves sign
    float odd = x * x * x;
    
    return x + (even * (1.0f - evenOddBalance) + odd * evenOddBalance) * 0.2f;
}