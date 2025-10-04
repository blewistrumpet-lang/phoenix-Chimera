#include "AdvancedSpringDispersion.h"
#include <algorithm>
#include <cstring>

//==============================================================================
// SpringPhysics Implementation
//==============================================================================

float AdvancedSpringDispersion::SpringPhysics::calculateWaveSpeed(float frequency) const {
    // Wave speed in helical spring depends on frequency due to dispersion
    // c(f) = c0 * sqrt(1 + (f/fc)^2) where fc is cutoff frequency
    
    float c0 = std::sqrt(youngsModulus / density); // Base wave speed
    float fc = c0 / (2.0f * M_PI * coilRadius);    // Cutoff frequency
    
    float ratio = frequency / fc;
    return c0 * std::sqrt(1.0f + ratio * ratio);
}

float AdvancedSpringDispersion::SpringPhysics::calculateDispersion(float frequency) const {
    // Dispersion relation for helical spring
    // Higher frequencies travel faster, creating characteristic "boing" sound
    
    float baseDelay = length / calculateWaveSpeed(20.0f);    // Delay at 20Hz
    float actualDelay = length / calculateWaveSpeed(frequency);
    
    return actualDelay / baseDelay; // Normalized dispersion factor
}

float AdvancedSpringDispersion::SpringPhysics::calculateModeDamping(int modeNumber) const {
    // Higher modes are more heavily damped
    // Damping increases with mode number squared
    
    float baseDamping = 0.001f; // Q ~1000 for fundamental
    return baseDamping * (1.0f + modeNumber * modeNumber * 0.1f);
}

//==============================================================================
// DispersiveDelayLine Implementation
//==============================================================================

void AdvancedSpringDispersion::DispersiveDelayLine::prepare(int maxDelay) {
    delayBuffer.resize(maxDelay);
    reset();
}

void AdvancedSpringDispersion::DispersiveDelayLine::reset() {
    std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
    writePos = 0;
    for (auto& ap : allpassChain) {
        ap.buffer = 0.0f;
    }
}

void AdvancedSpringDispersion::DispersiveDelayLine::setDispersion(float amount) {
    dispersionAmount = std::clamp(amount, 0.0f, 1.0f);
    
    // Set allpass coefficients for frequency-dependent delay
    // Higher frequencies get more delay relative to low frequencies
    for (int i = 0; i < NUM_ALLPASS; ++i) {
        float freq = 100.0f * std::pow(2.0f, i); // Exponentially spaced frequencies
        float coefficient = 0.3f + dispersionAmount * 0.6f * (1.0f - 1.0f / (1.0f + freq / 1000.0f));
        allpassChain[i].coefficient = coefficient;
    }
}

float AdvancedSpringDispersion::DispersiveDelayLine::process(float input, float delayTime) {
    // Write to delay buffer
    delayBuffer[writePos] = input;
    
    // Read from delay buffer with interpolation
    float delaySamples = delayTime;
    int delayInt = static_cast<int>(delaySamples);
    float delayFrac = delaySamples - delayInt;
    
    int readPos1 = (writePos - delayInt + delayBuffer.size()) % delayBuffer.size();
    int readPos2 = (readPos1 - 1 + delayBuffer.size()) % delayBuffer.size();
    
    float delayed = delayBuffer[readPos1] * (1.0f - delayFrac) + 
                   delayBuffer[readPos2] * delayFrac;
    
    // Apply dispersive allpass chain
    float dispersed = delayed;
    for (auto& ap : allpassChain) {
        dispersed = ap.process(dispersed);
    }
    
    // Mix between direct delay and dispersed delay
    float output = delayed * (1.0f - dispersionAmount) + dispersed * dispersionAmount;
    
    writePos = (writePos + 1) % delayBuffer.size();
    
    return output;
}

//==============================================================================
// SpringMode Implementation
//==============================================================================

void AdvancedSpringDispersion::SpringMode::setFrequency(float freq, double sr) {
    this->freq = freq;
    this->sampleRate = sr;
}

void AdvancedSpringDispersion::SpringMode::setDamping(float damping) {
    // Convert damping to resonance (Q factor)
    resonance = 1.0f - damping * 0.1f; // High Q even with max damping
    resonance = std::clamp(resonance, 0.9f, 0.9999f);
}

void AdvancedSpringDispersion::SpringMode::setAmplitude(float amp) {
    amplitude = amp;
}

float AdvancedSpringDispersion::SpringMode::process(float excitation) {
    // State-variable filter configured as resonator
    float omega = 2.0f * M_PI * freq / sampleRate;
    float sin_omega = std::sin(omega);
    float cos_omega = std::cos(omega);
    
    // Update filter states
    float v0 = excitation - resonance * state2;
    float v1 = state1 + sin_omega * v0;
    float v2 = state2 + sin_omega * v1;
    
    state1 = 2.0f * resonance * cos_omega * v1 - state1;
    state2 = 2.0f * resonance * cos_omega * v2 - v2;
    
    // Bandpass output
    return v1 * amplitude;
}

void AdvancedSpringDispersion::SpringMode::reset() {
    state1 = 0.0f;
    state2 = 0.0f;
}

//==============================================================================
// NonlinearProcessor Implementation
//==============================================================================

void AdvancedSpringDispersion::NonlinearProcessor::setAmount(float amt) {
    amount = std::clamp(amt, 0.0f, 1.0f);
}

float AdvancedSpringDispersion::NonlinearProcessor::process(float input) {
    // Apply tension-modulated waveshaping
    float shaped = tensionCurve(input);
    
    // Add velocity-dependent effects
    float velocity = input - prevSample;
    shaped += velocity * velocity * amount * 0.1f * (input > 0 ? 1.0f : -1.0f);
    
    prevSample = input;
    
    return shaped;
}

//==============================================================================
// ChirpGenerator Implementation
//==============================================================================

void AdvancedSpringDispersion::ChirpGenerator::trigger(float intensity) {
    amplitude = intensity;
    frequency = 2000.0f + intensity * 1000.0f; // Start frequency depends on intensity
    phase = 0.0f;
}

float AdvancedSpringDispersion::ChirpGenerator::generate() {
    if (amplitude < 0.001f) return 0.0f;
    
    // Generate chirp
    float sample = std::sin(phase) * amplitude;
    
    // Update phase
    phase += 2.0f * M_PI * frequency / 48000.0f; // Assumes 48kHz, should be parameterized
    if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    
    // Sweep frequency down
    frequency *= sweepRate;
    if (frequency < 50.0f) frequency = 50.0f;
    
    // Decay amplitude
    amplitude *= decay;
    
    return sample;
}

void AdvancedSpringDispersion::ChirpGenerator::setSweepRate(float rate) {
    sweepRate = std::clamp(rate, 0.99f, 0.9999f);
}

//==============================================================================
// AdvancedSpringDispersion Implementation
//==============================================================================

AdvancedSpringDispersion::AdvancedSpringDispersion() {
    // Initialize with default spring characteristics
}

void AdvancedSpringDispersion::prepare(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    
    // Prepare dispersive delay lines
    for (auto& line : m_dispersiveLines) {
        line.prepare(static_cast<int>(sampleRate * 0.1)); // 100ms max delay
    }
    
    // Initialize spring modes
    updateModes();
    
    // Set chirp parameters
    m_chirpGen.setSweepRate(0.995f);
    
    reset();
}

void AdvancedSpringDispersion::reset() {
    for (auto& line : m_dispersiveLines) {
        line.reset();
    }
    
    for (auto& mode : m_modes) {
        mode.reset();
    }
    
    m_envelope = 0.0f;
    m_prevEnvelope = 0.0f;
}

void AdvancedSpringDispersion::setSpringTension(float tension) {
    m_tension = std::clamp(tension, 0.0f, 1.0f);
    updateModes();
}

void AdvancedSpringDispersion::setSpringDiameter(float diameter) {
    m_diameter = std::clamp(diameter, 0.0f, 1.0f);
    m_physics.coilRadius = 0.005f + diameter * 0.015f; // 5-20mm
    updateModes();
}

void AdvancedSpringDispersion::setMaterialDamping(float damping) {
    m_damping = std::clamp(damping, 0.0f, 1.0f);
    for (int i = 0; i < NUM_MODES; ++i) {
        float modeDamping = m_physics.calculateModeDamping(i) * (1.0f + m_damping * 10.0f);
        m_modes[i].setDamping(modeDamping);
    }
}

void AdvancedSpringDispersion::setNonlinearity(float amount) {
    m_nonlinearity = std::clamp(amount, 0.0f, 1.0f);
    m_nonlinearProc.setAmount(m_nonlinearity);
}

void AdvancedSpringDispersion::updateModes() {
    // Calculate mode frequencies based on spring physics
    float fundamental = 100.0f + m_tension * 200.0f; // 100-300 Hz
    
    for (int i = 0; i < NUM_MODES; ++i) {
        // Inharmonic mode frequencies due to spring stiffness
        float modeFreq = fundamental * (i + 1) * (1.0f + i * 0.02f * (1.0f - m_tension));
        m_modes[i].setFrequency(modeFreq, m_sampleRate);
        
        // Mode amplitude decreases with mode number
        float modeAmp = 1.0f / (i + 1);
        m_modes[i].setAmplitude(modeAmp);
        
        // Set damping
        float modeDamping = m_physics.calculateModeDamping(i) * (1.0f + m_damping * 10.0f);
        m_modes[i].setDamping(modeDamping);
    }
}

void AdvancedSpringDispersion::detectTransient(float input) {
    // Simple envelope follower
    float absInput = std::abs(input);
    m_envelope = absInput + m_envelope * 0.95f;
    
    // Detect sharp attack
    float attack = m_envelope - m_prevEnvelope;
    if (attack > TRANSIENT_THRESHOLD) {
        m_chirpGen.trigger(attack);
    }
    
    m_prevEnvelope = m_envelope;
}

float AdvancedSpringDispersion::process(float input) {
    // Detect transients for chirp generation
    detectTransient(input);
    
    // Apply nonlinear processing
    float processed = m_nonlinearProc.process(input);
    
    // Process through dispersive delay lines in parallel
    float dispersed = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float delayTime = (10.0f + i * 15.0f) * (1.0f + m_tension * 0.5f); // Tension affects delay
        m_dispersiveLines[i].setDispersion(0.3f + m_tension * 0.4f);
        dispersed += m_dispersiveLines[i].process(processed, delayTime) * 0.33f;
    }
    
    // Excite spring modes
    float modalSum = 0.0f;
    for (auto& mode : m_modes) {
        modalSum += mode.process(dispersed);
    }
    modalSum *= 0.5f; // Scale modal contribution
    
    // Add chirp for characteristic spring sound
    float chirp = m_chirpGen.generate();
    
    // Mix components
    float output = dispersed * 0.5f + modalSum * 0.3f + chirp * 0.2f;
    
    return output;
}

void AdvancedSpringDispersion::processBlock(float* buffer, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        buffer[i] = process(buffer[i]);
    }
}

//==============================================================================
// SpringCouplingMatrix Implementation
//==============================================================================

SpringCouplingMatrix::SpringCouplingMatrix() {
    // Initialize with diagonal matrix (no coupling)
    for (int i = 0; i < MAX_SPRINGS; ++i) {
        for (int j = 0; j < MAX_SPRINGS; ++j) {
            m_couplingMatrix[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

void SpringCouplingMatrix::setCoupling(int spring1, int spring2, float coefficient) {
    if (spring1 >= 0 && spring1 < MAX_SPRINGS && 
        spring2 >= 0 && spring2 < MAX_SPRINGS) {
        m_couplingMatrix[spring1][spring2] = coefficient;
        m_couplingMatrix[spring2][spring1] = coefficient; // Symmetric
        normalizeMatrix();
    }
}

void SpringCouplingMatrix::normalizeMatrix() {
    // Ensure energy conservation by normalizing rows
    for (int i = 0; i < MAX_SPRINGS; ++i) {
        float rowSum = 0.0f;
        for (int j = 0; j < MAX_SPRINGS; ++j) {
            rowSum += std::abs(m_couplingMatrix[i][j]);
        }
        
        if (rowSum > 1.0f) {
            for (int j = 0; j < MAX_SPRINGS; ++j) {
                m_couplingMatrix[i][j] /= rowSum;
            }
        }
    }
}

float SpringCouplingMatrix::getCoupledFeedback(int springIndex, 
                                              const std::array<float, MAX_SPRINGS>& states) const {
    if (springIndex < 0 || springIndex >= MAX_SPRINGS) return 0.0f;
    
    float feedback = 0.0f;
    for (int i = 0; i < MAX_SPRINGS; ++i) {
        feedback += states[i] * m_couplingMatrix[springIndex][i];
    }
    
    return feedback;
}

void SpringCouplingMatrix::process(std::array<float, MAX_SPRINGS>& springStates,
                                  const std::array<float, MAX_SPRINGS>& inputs) {
    std::array<float, MAX_SPRINGS> newStates;
    
    for (int i = 0; i < MAX_SPRINGS; ++i) {
        newStates[i] = inputs[i] + getCoupledFeedback(i, springStates);
    }
    
    springStates = newStates;
}