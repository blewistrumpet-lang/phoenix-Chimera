#include "RotarySpeaker.h"
#include <cmath>

RotarySpeaker::RotarySpeaker() {
}

void RotarySpeaker::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize crossover filters
    m_crossover.clear();
    m_preampState.clear();
    m_cabinetResonance.clear();
    m_hornDelayBuffers.clear();
    m_drumDelayBuffers.clear();
    m_hornWritePos.clear();
    m_drumWritePos.clear();
    
    // Doppler delay buffer size (max ~5ms for realistic Doppler effect)
    int maxDopplerSamples = static_cast<int>(sampleRate * 0.005);
    
    for (int ch = 0; ch < 2; ++ch) {
        m_crossover.push_back(CrossoverFilters());
        m_preampState.push_back(0.0f);
        m_cabinetResonance.push_back(0.0f);
        
        m_hornDelayBuffers.push_back(std::vector<float>(maxDopplerSamples, 0.0f));
        m_drumDelayBuffers.push_back(std::vector<float>(maxDopplerSamples, 0.0f));
        m_hornWritePos.push_back(0);
        m_drumWritePos.push_back(0);
    }
    
    // Initialize rotation state
    m_hornRotation = 0.0f;
    m_drumRotation = 0.0f;
    m_hornVelocity = 0.0f;
    m_drumVelocity = 0.0f;
    
    updateRotationSpeed();
}

void RotarySpeaker::reset() {
    // Reset crossover filter states
    for (auto& filter : m_crossover) {
        filter.horn_z1 = 0.0f;
        filter.horn_z2 = 0.0f;
        filter.drum_z1 = 0.0f;
        filter.drum_z2 = 0.0f;
    }
    
    // Clear all delay buffers
    for (auto& buffer : m_hornDelayBuffers) {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }
    
    for (auto& buffer : m_drumDelayBuffers) {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }
    
    // Reset write positions
    std::fill(m_hornWritePos.begin(), m_hornWritePos.end(), 0);
    std::fill(m_drumWritePos.begin(), m_drumWritePos.end(), 0);
    
    // Reset preamp state
    std::fill(m_preampState.begin(), m_preampState.end(), 0.0f);
    
    // Reset cabinet resonance state
    std::fill(m_cabinetResonance.begin(), m_cabinetResonance.end(), 0.0f);
    
    // Reset rotation state
    m_hornRotation = 0.0f;
    m_drumRotation = 0.0f;
    m_hornVelocity = 0.0f;
    m_drumVelocity = 0.0f;
    
    // Reset target speeds based on current parameter values
    updateRotationSpeed();
}

void RotarySpeaker::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = processSample(channelData[sample], channel);
        }
    }
}

float RotarySpeaker::processSample(float input, int channel) {
    // Apply tube preamp coloration
    float preamp = tubePreamp(input, channel);
    
    // Split signal into horn (high) and drum (low) bands
    float hornBand, drumBand;
    processCrossover(preamp, channel, hornBand, drumBand);
    
    // Process each band through its respective rotating speaker
    float hornOutput = processDoppler(hornBand, channel, true, m_hornRotation, m_hornVelocity);
    float drumOutput = processDoppler(drumBand, channel, false, m_drumRotation, m_drumVelocity);
    
    // Combine the bands
    float combined = hornOutput + drumOutput;
    
    // Apply cabinet resonance
    combined = cabinetResonance(combined, channel);
    
    // Update rotation angles and velocities
    updateRotationSpeed();
    
    float deltaTime = 1.0f / m_sampleRate;
    m_hornRotation += m_hornVelocity * deltaTime;
    m_drumRotation += m_drumVelocity * deltaTime;
    
    // Wrap rotation angles
    if (m_hornRotation > 2.0f * M_PI) m_hornRotation -= 2.0f * M_PI;
    if (m_drumRotation > 2.0f * M_PI) m_drumRotation -= 2.0f * M_PI;
    
    return combined * 0.7f; // Scale to prevent clipping
}

void RotarySpeaker::processCrossover(float input, int channel, float& hornBand, float& drumBand) {
    // Crossover at ~800Hz (typical Leslie crossover frequency)
    float crossoverFreq = 800.0f;
    float omega = 2.0f * M_PI * crossoverFreq / m_sampleRate;
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    float alpha = sinOmega / (2.0f * 0.707f);
    
    // Lowpass coefficients for drum
    float lp_b0 = (1.0f - cosOmega) / 2.0f;
    float lp_b1 = 1.0f - cosOmega;
    float lp_b2 = (1.0f - cosOmega) / 2.0f;
    float a1 = -2.0f * cosOmega;
    float a2 = 1.0f - alpha;
    float a0 = 1.0f + alpha;
    
    // Normalize
    lp_b0 /= a0; lp_b1 /= a0; lp_b2 /= a0; a1 /= a0; a2 /= a0;
    
    // Process drum (lowpass)
    drumBand = lp_b0 * input + lp_b1 * m_crossover[channel].drum_z1 + lp_b2 * m_crossover[channel].drum_z2
               - a1 * m_crossover[channel].drum_z1 - a2 * m_crossover[channel].drum_z2;
    
    // Update drum delay line
    m_crossover[channel].drum_z2 = m_crossover[channel].drum_z1;
    m_crossover[channel].drum_z1 = drumBand;
    
    // Highpass coefficients for horn
    float hp_b0 = (1.0f + cosOmega) / 2.0f;
    float hp_b1 = -(1.0f + cosOmega);
    float hp_b2 = (1.0f + cosOmega) / 2.0f;
    
    hp_b0 /= a0; hp_b1 /= a0; hp_b2 /= a0;
    
    // Process horn (highpass)
    hornBand = hp_b0 * input + hp_b1 * m_crossover[channel].horn_z1 + hp_b2 * m_crossover[channel].horn_z2
               - a1 * m_crossover[channel].horn_z1 - a2 * m_crossover[channel].horn_z2;
    
    // Update horn delay line
    m_crossover[channel].horn_z2 = m_crossover[channel].horn_z1;
    m_crossover[channel].horn_z1 = hornBand;
}

float RotarySpeaker::processDoppler(float input, int channel, bool isHorn, float rotation, float velocity) {
    // Calculate Doppler shift based on rotation and microphone distance
    float micAngle = (channel == 0) ? -m_stereoWidth * 0.5f : m_stereoWidth * 0.5f;
    
    // Calculate relative position of speaker to microphone
    float speakerX = std::cos(rotation);
    float speakerY = std::sin(rotation);
    float micX = std::cos(micAngle) * m_micDistance;
    float micY = std::sin(micAngle) * m_micDistance;
    
    // Calculate distance and velocity component towards microphone
    float distance = std::sqrt((speakerX - micX) * (speakerX - micX) + 
                              (speakerY - micY) * (speakerY - micY));
    
    // Velocity component towards microphone (simplified)
    float velocityTowardsMic = velocity * std::cos(rotation - micAngle);
    
    // Calculate Doppler delay (speed of sound ~343 m/s, Leslie radius ~0.3m)
    float speedOfSound = 343.0f;
    float lesliRadius = 0.3f;
    float dopplerDelay = (distance * lesliRadius / speedOfSound) * m_sampleRate;
    dopplerDelay += velocityTowardsMic * 0.0001f * m_sampleRate; // Velocity component
    
    // Clamp delay to buffer size
    int bufferSize = isHorn ? m_hornDelayBuffers[channel].size() : m_drumDelayBuffers[channel].size();
    dopplerDelay = std::max(1.0f, std::min(dopplerDelay, float(bufferSize - 1)));
    
    // Write to delay buffer
    if (isHorn) {
        m_hornDelayBuffers[channel][m_hornWritePos[channel]] = input;
        m_hornWritePos[channel] = (m_hornWritePos[channel] + 1) % bufferSize;
        
        // Read with interpolation
        float readPos = m_hornWritePos[channel] - dopplerDelay;
        while (readPos < 0) readPos += bufferSize;
        
        return interpolatedRead(m_hornDelayBuffers[channel], readPos);
    } else {
        m_drumDelayBuffers[channel][m_drumWritePos[channel]] = input;
        m_drumWritePos[channel] = (m_drumWritePos[channel] + 1) % bufferSize;
        
        // Read with interpolation
        float readPos = m_drumWritePos[channel] - dopplerDelay;
        while (readPos < 0) readPos += bufferSize;
        
        return interpolatedRead(m_drumDelayBuffers[channel], readPos);
    }
}

float RotarySpeaker::tubePreamp(float input, int channel) {
    // Leslie preamp tube coloration
    float drive = 1.8f;
    float driven = input * drive;
    
    // Tube-style saturation
    float saturated = std::tanh(driven * 0.6f) / 0.6f;
    
    // Add even harmonics
    float harmonics = saturated + 0.08f * saturated * saturated;
    
    // Tube warmth filtering
    float cutoff = 0.08f;
    m_preampState[channel] += cutoff * (harmonics - m_preampState[channel]);
    
    return m_preampState[channel] / drive;
}

float RotarySpeaker::cabinetResonance(float input, int channel) {
    // Leslie cabinet has characteristic resonances around 100Hz and 3kHz
    float resonanceFreq = 100.0f;
    float omega = 2.0f * M_PI * resonanceFreq / m_sampleRate;
    float q = 2.0f; // Higher Q for resonance
    
    float alpha = std::sin(omega) / (2.0f * q);
    float cosOmega = std::cos(omega);
    
    // Resonant filter coefficients
    float b0 = alpha;
    float b1 = 0.0f;
    float b2 = -alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosOmega;
    float a2 = 1.0f - alpha;
    
    // Normalize
    b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    
    // Apply resonance (simplified single pole for now)
    float cutoff = 0.02f;
    m_cabinetResonance[channel] += cutoff * (input - m_cabinetResonance[channel]);
    
    return input + m_cabinetResonance[channel] * 0.1f; // Subtle resonance
}

float RotarySpeaker::interpolatedRead(const std::vector<float>& buffer, float position) {
    int pos1 = static_cast<int>(position);
    int pos2 = (pos1 + 1) % buffer.size();
    float frac = position - pos1;
    
    return buffer[pos1] * (1.0f - frac) + buffer[pos2] * frac;
}

void RotarySpeaker::updateRotationSpeed() {
    // Update target speeds based on speed parameter
    // Chorale (slow): Horn ~40rpm, Drum ~30rpm
    // Tremolo (fast): Horn ~340rpm, Drum ~400rpm
    
    float choraleHornRPM = 40.0f;
    float tremoloHornRPM = 340.0f;
    float choraleDrumRPM = 30.0f;
    float tremoloDrumRPM = 400.0f;
    
    float targetHornRPM = choraleHornRPM + m_speed * (tremoloHornRPM - choraleHornRPM);
    float targetDrumRPM = choraleDrumRPM + m_speed * (tremoloDrumRPM - choraleDrumRPM);
    
    // Convert RPM to radians per second
    m_targetHornSpeed = targetHornRPM * 2.0f * M_PI / 60.0f;
    m_targetDrumSpeed = targetDrumRPM * 2.0f * M_PI / 60.0f;
    
    // Apply acceleration (Leslie has characteristic acceleration/deceleration)
    float accel = m_acceleration * 10.0f + 0.5f; // 0.5 to 10.5 rad/s²
    float deltaTime = 1.0f / m_sampleRate;
    
    // Accelerate towards target speeds
    if (m_hornVelocity < m_targetHornSpeed) {
        m_hornVelocity += accel * deltaTime;
        m_hornVelocity = std::min(m_hornVelocity, m_targetHornSpeed);
    } else if (m_hornVelocity > m_targetHornSpeed) {
        m_hornVelocity -= accel * deltaTime;
        m_hornVelocity = std::max(m_hornVelocity, m_targetHornSpeed);
    }
    
    if (m_drumVelocity < m_targetDrumSpeed) {
        m_drumVelocity += accel * deltaTime;
        m_drumVelocity = std::min(m_drumVelocity, m_targetDrumSpeed);
    } else if (m_drumVelocity > m_targetDrumSpeed) {
        m_drumVelocity -= accel * deltaTime;
        m_drumVelocity = std::max(m_drumVelocity, m_targetDrumSpeed);
    }
}

void RotarySpeaker::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) m_speed = params.at(0);
    if (params.find(1) != params.end()) m_acceleration = params.at(1);
    if (params.find(2) != params.end()) m_micDistance = params.at(2);
    if (params.find(3) != params.end()) m_stereoWidth = params.at(3);
}

juce::String RotarySpeaker::getParameterName(int index) const {
    switch (index) {
        case 0: return "Speed";
        case 1: return "Acceleration";
        case 2: return "Mic Distance";
        case 3: return "Stereo Width";
        default: return "";
    }
}