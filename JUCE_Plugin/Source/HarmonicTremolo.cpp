#include "HarmonicTremolo.h"
#include <cmath>

HarmonicTremolo::HarmonicTremolo() {
}

void HarmonicTremolo::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize LFO phases
    m_lfoPhase.clear();
    m_crossover.clear();
    m_tubeState.clear();
    
    for (int ch = 0; ch < 2; ++ch) {
        m_lfoPhase.push_back(ch * M_PI * m_stereoPhase); // Phase offset for stereo
        m_crossover.push_back(CrossoverFilters());
        m_tubeState.push_back(0.0f);
    }

void HarmonicTremolo::reset() {
    // Reset modulation state
    m_lfoPhase = 0.0f;
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
}

}

void HarmonicTremolo::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = processSample(channelData[sample], channel);
        }
    }
}

float HarmonicTremolo::processSample(float input, int channel) {
    // Apply subtle tube-style coloration
    float colored = tubeWaveshape(input, channel);
    
    // Split signal into low and high bands
    float lowBand, highBand;
    processCrossover(colored, channel, lowBand, highBand);
    
    // Generate LFO for this channel
    float lfo = calculateLFO(channel);
    
    // Create complementary modulation for harmonic tremolo
    // Low band gets normal LFO, high band gets inverted LFO
    float lowMod = 1.0f + lfo * m_depth;
    float highMod = 1.0f - lfo * m_depth;
    
    // Ensure modulation doesn't go negative
    lowMod = std::max(0.0f, lowMod);
    highMod = std::max(0.0f, highMod);
    
    // Apply modulation to each band
    float modulatedLow = lowBand * lowMod;
    float modulatedHigh = highBand * highMod;
    
    // Recombine the bands
    float output = modulatedLow + modulatedHigh;
    
    // Update LFO phase
    float rateHz = 0.1f + m_rate * 19.9f; // 0.1Hz to 20Hz
    m_lfoPhase[channel] += 2.0f * M_PI * rateHz / m_sampleRate;
    
    // Wrap phase
    if (m_lfoPhase[channel] > 2.0f * M_PI) {
        m_lfoPhase[channel] -= 2.0f * M_PI;
    }
    
    return output * 0.8f; // Slight attenuation to prevent clipping
}

void HarmonicTremolo::processCrossover(float input, int channel, float& lowBand, float& highBand) {
    // Crossover frequency based on harmonics parameter (200Hz to 2kHz)
    float crossoverFreq = 200.0f + m_harmonics * 1800.0f;
    float omega = 2.0f * M_PI * crossoverFreq / m_sampleRate;
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    float alpha = sinOmega / (2.0f * 0.707f); // Q = 0.707 for Butterworth response
    
    // Coefficients for 2nd-order Butterworth filters
    float b0 = (1.0f - cosOmega) / 2.0f;  // Lowpass
    float b1 = 1.0f - cosOmega;
    float b2 = (1.0f - cosOmega) / 2.0f;
    float a1 = -2.0f * cosOmega;
    float a2 = 1.0f - alpha;
    float a0 = 1.0f + alpha;
    
    // Normalize coefficients
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
    
    // Process lowpass filter
    float lowOutput = b0 * input + b1 * m_crossover[channel].lowpass_z1 + b2 * m_crossover[channel].lowpass_z2
                      - a1 * m_crossover[channel].lowpass_z1 - a2 * m_crossover[channel].lowpass_z2;
    
    // Update lowpass delay line
    m_crossover[channel].lowpass_z2 = m_crossover[channel].lowpass_z1;
    m_crossover[channel].lowpass_z1 = lowOutput;
    
    // For highpass, use different coefficients
    float hp_b0 = (1.0f + cosOmega) / 2.0f;
    float hp_b1 = -(1.0f + cosOmega);
    float hp_b2 = (1.0f + cosOmega) / 2.0f;
    
    hp_b0 /= a0;
    hp_b1 /= a0;
    hp_b2 /= a0;
    
    // Process highpass filter
    float highOutput = hp_b0 * input + hp_b1 * m_crossover[channel].highpass_z1 + hp_b2 * m_crossover[channel].highpass_z2
                       - a1 * m_crossover[channel].highpass_z1 - a2 * m_crossover[channel].highpass_z2;
    
    // Update highpass delay line
    m_crossover[channel].highpass_z2 = m_crossover[channel].highpass_z1;
    m_crossover[channel].highpass_z1 = highOutput;
    
    lowBand = lowOutput;
    highBand = highOutput;
}

float HarmonicTremolo::tubeWaveshape(float input, int channel) {
    // Subtle tube-style waveshaping for vintage character
    float drive = 1.5f;
    float driven = input * drive;
    
    // Asymmetric saturation characteristic of tube circuits
    float shaped;
    if (driven > 0.0f) {
        shaped = std::tanh(driven * 0.8f) / 0.8f;
    } else {
        shaped = std::tanh(driven * 1.2f) / 1.2f;
    }
    
    // Add subtle even harmonics
    float harmonics = shaped + 0.05f * shaped * shaped;
    
    // Simple RC filtering for tube warmth
    float cutoff = 0.05f;
    m_tubeState[channel] += cutoff * (harmonics - m_tubeState[channel]);
    
    return m_tubeState[channel] / drive; // Scale back down
}

float HarmonicTremolo::calculateLFO(int channel) {
    // Vintage tremolo uses a triangle-ish wave rather than pure sine
    // This creates a more musical, less choppy sound
    
    float phase = m_lfoPhase[channel];
    float triangle;
    
    if (phase < M_PI) {
        // Rising portion
        triangle = (2.0f * phase / M_PI) - 1.0f;
    } else {
        // Falling portion
        triangle = 3.0f - (2.0f * phase / M_PI);
    }
    
    // Smooth the triangle wave slightly for more vintage character
    float smoothed = triangle + 0.1f * std::sin(phase * 3.0f);
    
    // Apply stereo phase offset
    if (channel == 1) {
        float phaseOffset = m_stereoPhase * 2.0f * M_PI;
        float offsetPhase = phase + phaseOffset;
        if (offsetPhase > 2.0f * M_PI) offsetPhase -= 2.0f * M_PI;
        
        if (offsetPhase < M_PI) {
            triangle = (2.0f * offsetPhase / M_PI) - 1.0f;
        } else {
            triangle = 3.0f - (2.0f * offsetPhase / M_PI);
        }
        smoothed = triangle + 0.1f * std::sin(offsetPhase * 3.0f);
    }
    
    return smoothed * 0.5f; // Scale to reasonable range
}

void HarmonicTremolo::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) m_rate = params.at(0);
    if (params.find(1) != params.end()) m_depth = params.at(1);
    if (params.find(2) != params.end()) m_harmonics = params.at(2);
    if (params.find(3) != params.end()) m_stereoPhase = params.at(3);
}

juce::String HarmonicTremolo::getParameterName(int index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Harmonics";
        case 3: return "Stereo Phase";
        default: return "";
    }
}