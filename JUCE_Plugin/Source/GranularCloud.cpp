#include "GranularCloud.h"
#include <algorithm>

GranularCloud::GranularCloud() : m_rng(std::random_device{}()) {
    m_grains.resize(MAX_GRAINS);
    for (int i = 0; i < 2; ++i) {
        m_circularBuffer[i].resize(BUFFER_SIZE);
    }
    
    // Initialize smooth parameters
    m_grainSize.setImmediate(50.0f);
    m_density.setImmediate(10.0f);
    m_pitchScatter.setImmediate(0.0f);
    m_cloudPosition.setImmediate(0.5f);
    
    // Set smoothing rates
    m_grainSize.setSmoothingRate(0.990f);
    m_density.setSmoothingRate(0.995f);
    m_pitchScatter.setSmoothingRate(0.998f);
    m_cloudPosition.setSmoothingRate(0.995f);
}

void GranularCloud::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Clear circular buffers
    for (int i = 0; i < 2; ++i) {
        std::fill(m_circularBuffer[i].begin(), m_circularBuffer[i].end(), 0.0f);
    }
    
    m_writePos = 0;
    
    // Reset all grains
    for (auto& grain : m_grains) {
        grain.reset();
    }
    
    // Generate maximum size window
    m_hannWindow.resize(static_cast<int>(0.5f * m_sampleRate)); // 500ms max
    generateHannWindow(static_cast<int>(m_hannWindow.size()));
    
    // Reset grain timer
    m_grainTimer = 0.0f;
    m_nextGrainTime = 0.0f;
    
    // Initialize DC blockers
    m_inputDCBlockers.fill({});
    m_outputDCBlockers.fill({});
    
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

void GranularCloud::reset() {
    // Reset all internal state
    // TODO: Implement specific reset logic for GranularCloud
}

void GranularCloud::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_grainSize.update();
    m_density.update();
    m_pitchScatter.update();
    m_cloudPosition.update();
    
    // Update thermal model
    m_thermalModel.update(m_sampleRate);
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update component aging (very slow)
    m_sampleCount += numSamples;
    if (m_sampleCount > m_sampleRate * 8) { // Every 8 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00006f);
        m_sampleCount = 0;
    }
    
    // Calculate grain interval based on density with thermal variation
    float grainInterval = (1.0f / m_density.current) * thermalFactor;
    
    for (int sample = 0; sample < numSamples; ++sample) {
        // Write input to circular buffer with DC blocking
        for (int ch = 0; ch < numChannels; ++ch) {
            float input = buffer.getReadPointer(ch)[sample];
            input = m_inputDCBlockers[ch].process(input);
            m_circularBuffer[ch][m_writePos] = input;
        }
        
        // Check if it's time to trigger a new grain
        m_grainTimer += 1.0f / m_sampleRate;
        if (m_grainTimer >= m_nextGrainTime) {
            triggerGrain();
            m_nextGrainTime = m_grainTimer + grainInterval * (0.5f + 0.5f * m_uniformDist(m_rng));
        }
        
        // Clear output
        for (int ch = 0; ch < numChannels; ++ch) {
            buffer.getWritePointer(ch)[sample] = 0.0f;
        }
        
        // Process all active grains with boutique modeling
        for (auto& grain : m_grains) {
            if (grain.active) {
                float grainSample = processGrainWithModeling(grain, thermalFactor, m_componentAge);
                
                if (!grain.active) continue; // Grain finished
                
                // Apply panning with thermal variation
                float leftGain, rightGain;
                float thermalPan = grain.pan + (thermalFactor - 1.0f) * 0.1f;
                thermalPan = std::max(0.0f, std::min(1.0f, thermalPan));
                calculateStereoPan(leftGain, rightGain, thermalPan);
                
                // Add subtle analog saturation
                grainSample = std::tanh(grainSample * (1.0f + m_componentAge * 0.1f));
                
                // Mix to output with DC blocking
                if (numChannels >= 1) {
                    float output = grainSample * leftGain * 0.4f; // Reduced gain for multiple grains
                    output = m_outputDCBlockers[0].process(output);
                    buffer.getWritePointer(0)[sample] += output;
                }
                if (numChannels >= 2) {
                    float output = grainSample * rightGain * 0.4f;
                    output = m_outputDCBlockers[1].process(output);
                    buffer.getWritePointer(1)[sample] += output;
                }
            }
        }
        
        // Increment circular buffer position
        m_writePos = (m_writePos + 1) % BUFFER_SIZE;
    }
}

void GranularCloud::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    // Grain size: 10-500ms
    m_grainSize.target = 10.0f + getParam(0, 0.08f) * 490.0f;
    
    // Density: 1-50 grains per second (exponential)
    float densityParam = getParam(1, 0.2f);
    m_density.target = 1.0f * std::pow(50.0f, densityParam);
    
    // Pitch scatter: ±2 octaves
    m_pitchScatter.target = getParam(2, 0.0f) * 2.0f;
    
    // Cloud position: stereo spread
    m_cloudPosition.target = getParam(3, 0.5f);
}

juce::String GranularCloud::getParameterName(int index) const {
    switch (index) {
        case 0: return "Grain Size";
        case 1: return "Density";
        case 2: return "Pitch Scatter";
        case 3: return "Cloud Position";
        default: return "";
    }
}

void GranularCloud::triggerGrain() {
    Grain* grain = findInactiveGrain();
    if (!grain) return;
    
    // Calculate grain length in samples with thermal variation
    float thermalFactor = m_thermalModel.getThermalFactor();
    grain->grainLength = static_cast<int>(m_grainSize.current * 0.001f * m_sampleRate * thermalFactor);
    grain->grainLength = std::min(grain->grainLength, static_cast<int>(m_hannWindow.size()));
    
    // Set buffer pointer (using channel 0 for simplicity, could alternate)
    grain->bufferPtr = m_circularBuffer[0].data();
    grain->bufferSize = BUFFER_SIZE;
    
    // Random start position (recent history)
    int maxDelay = static_cast<int>(0.5f * m_sampleRate); // 500ms history
    int delay = m_uniformDist(m_rng) * maxDelay;
    grain->readPos = (m_writePos - delay + BUFFER_SIZE) % BUFFER_SIZE;
    
    // Random pitch with component aging effects
    grain->pitch = calculatePitchFactor(m_pitchScatter.current * (1.0f + m_componentAge * 0.05f));
    
    // Random amplitude (slight variation) affected by aging
    grain->amplitude = (0.8f + 0.2f * m_uniformDist(m_rng)) * (1.0f - m_componentAge * 0.1f);
    
    // Stereo positioning based on cloud position with thermal drift
    float spread = m_cloudPosition.current * thermalFactor;
    grain->pan = 0.5f + (m_uniformDist(m_rng) - 0.5f) * spread;
    
    // Activate grain
    grain->envelopePos = 0;
    grain->active = true;
}

GranularCloud::Grain* GranularCloud::findInactiveGrain() {
    for (auto& grain : m_grains) {
        if (!grain.active) {
            return &grain;
        }
    }
    return nullptr;
}

float GranularCloud::Grain::process() {
    if (!active || !bufferPtr) return 0.0f;
    
    // Calculate envelope
    float envelope = 1.0f;
    if (envelopePos < grainLength) {
        float pos = static_cast<float>(envelopePos) / grainLength;
        envelope = 0.5f * (1.0f - cos(2.0f * M_PI * pos)); // Hann window
    }
    
    // Read sample with linear interpolation for pitch shifting
    float readPosFloat = readPos + envelopePos * pitch;
    int readPos1 = static_cast<int>(readPosFloat) % bufferSize;
    int readPos2 = (readPos1 + 1) % bufferSize;
    float frac = readPosFloat - floor(readPosFloat);
    
    float sample = bufferPtr[readPos1] * (1.0f - frac) + bufferPtr[readPos2] * frac;
    
    // Apply envelope and amplitude
    sample *= envelope * amplitude;
    
    // Advance envelope
    envelopePos++;
    
    // Check if grain is finished
    if (envelopePos >= grainLength) {
        active = false;
    }
    
    return sample;
}

void GranularCloud::Grain::reset() {
    bufferPtr = nullptr;
    bufferSize = 0;
    readPos = 0;
    grainLength = 0;
    pitch = 1.0f;
    amplitude = 1.0f;
    pan = 0.5f;
    active = false;
    envelopePos = 0;
}

void GranularCloud::generateHannWindow(int size) {
    for (int i = 0; i < size; ++i) {
        m_hannWindow[i] = 0.5f * (1.0f - cos(2.0f * M_PI * i / (size - 1)));
    }
}

float GranularCloud::calculatePitchFactor(float scatter) {
    if (scatter == 0.0f) return 1.0f;
    
    // Generate random pitch deviation in octaves
    float octaveShift = m_normalDist(m_rng) * scatter;
    octaveShift = std::max(-2.0f, std::min(2.0f, octaveShift)); // Clamp to ±2 octaves
    
    // Convert to pitch factor
    return std::pow(2.0f, octaveShift);
}

float GranularCloud::processGrainWithModeling(Grain& grain, float thermalFactor, float aging) {
    if (!grain.active || !grain.bufferPtr) return 0.0f;
    
    // Calculate envelope with thermal variations
    float envelope = 1.0f;
    if (grain.envelopePos < grain.grainLength) {
        float pos = static_cast<float>(grain.envelopePos) / grain.grainLength;
        envelope = 0.5f * (1.0f - cos(2.0f * M_PI * pos)); // Hann window
        
        // Add slight envelope variations from aging
        if (aging > 0.01f) {
            envelope *= (1.0f + aging * 0.02f * sin(pos * 8.0f * M_PI));
        }
    }
    
    // Read sample with enhanced interpolation for pitch shifting
    float readPosFloat = grain.readPos + grain.envelopePos * grain.pitch * thermalFactor;
    int readPos1 = static_cast<int>(readPosFloat) % grain.bufferSize;
    int readPos2 = (readPos1 + 1) % grain.bufferSize;
    int readPos3 = (readPos1 + 2) % grain.bufferSize;
    int readPos4 = (readPos1 + 3) % grain.bufferSize;
    float frac = readPosFloat - floor(readPosFloat);
    
    // Hermite interpolation for smoother pitch shifting
    float y0 = grain.bufferPtr[readPos1];
    float y1 = grain.bufferPtr[readPos2];
    float y2 = grain.bufferPtr[readPos3];
    float y3 = grain.bufferPtr[readPos4];
    
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 1.5f * (y1 - y2) + 0.5f * (y3 - y0);
    
    float sample = ((c3 * frac + c2) * frac + c1) * frac + c0;
    
    // Apply envelope and amplitude with aging effects
    sample *= envelope * grain.amplitude;
    
    // Add subtle thermal noise
    if (aging > 0.01f) {
        sample += aging * 0.001f * ((rand() % 1000) / 1000.0f - 0.5f);
    }
    
    // Advance envelope
    grain.envelopePos++;
    
    // Check if grain is finished
    if (grain.envelopePos >= grain.grainLength) {
        grain.active = false;
    }
    
    return sample;
}

void GranularCloud::calculateStereoPan(float& leftGain, float& rightGain, float pan) {
    // Equal power panning
    float angle = pan * M_PI * 0.5f;
    leftGain = cos(angle);
    rightGain = sin(angle);
}