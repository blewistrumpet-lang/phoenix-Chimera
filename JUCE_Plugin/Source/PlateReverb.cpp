#include "PlateReverb.h"
#include <cmath>
#include <algorithm>

PlateReverb::PlateReverb() {
    // Initialize parameters with classic plate settings
    m_size.setImmediate(0.5f);
    m_damping.setImmediate(0.5f);
    m_predelay.setImmediate(0.0f);
    m_mix.setImmediate(0.3f);
}

void PlateReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    initializeDelays();
}

void PlateReverb::reset() {
    // Reset all comb filters
    for (auto& comb : m_combs) {
        std::fill(comb.buffer.begin(), comb.buffer.end(), 0.0f);
        comb.writePos = 0;
        comb.filterState = 0.0f;
        comb.modPhase = static_cast<float>(rand()) / RAND_MAX; // Random phase
    }
    
    // Reset all allpass filters
    for (auto& allpass : m_allpasses) {
        std::fill(allpass.buffer.begin(), allpass.buffer.end(), 0.0f);
        allpass.writePos = 0;
    }
    
    // Reset lattice networks
    for (auto& lattice : m_latticeNetwork) {
        std::fill(lattice.stage1.buffer.begin(), lattice.stage1.buffer.end(), 0.0f);
        std::fill(lattice.stage2.buffer.begin(), lattice.stage2.buffer.end(), 0.0f);
        lattice.stage1.writePos = 0;
        lattice.stage2.writePos = 0;
    }
    
    // Reset pre-delay buffers
    std::fill(m_predelayLeft.buffer.begin(), m_predelayLeft.buffer.end(), 0.0f);
    std::fill(m_predelayRight.buffer.begin(), m_predelayRight.buffer.end(), 0.0f);
    m_predelayLeft.writePos = 0.0f;
    m_predelayRight.writePos = 0.0f;
    
    // Reset input diffusion
    for (auto& diffuser : m_inputDiffusion) {
        std::fill(diffuser.buffer.begin(), diffuser.buffer.end(), 0.0f);
        diffuser.writePos = 0;
    }
    
    // Reset high-shelf filters
    for (auto& shelf : m_highShelves) {
        shelf.x1 = 0.0f;
        shelf.y1 = 0.0f;
    }
    
    // Reset DC blockers
    for (auto& blocker : m_dcBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
}

void PlateReverb::initializeDelays() {
    // Scale delay times based on sample rate
    float sampleRateRatio = m_sampleRate / 44100.0f;
    
    // Initialize comb filters with modulation
    for (int i = 0; i < NUM_COMBS; ++i) {
        int delaySamples = static_cast<int>(m_combDelayTimes[i] * sampleRateRatio);
        float modDepth = 0.3f + (i * 0.05f); // Increasing mod depth for variety
        m_combs[i].init(delaySamples, m_combModRates[i], modDepth);
    }
    
    // Initialize allpass filters
    for (int i = 0; i < NUM_ALLPASS; ++i) {
        int delaySamples = static_cast<int>(m_allpassDelayTimes[i] * sampleRateRatio);
        m_allpasses[i].init(delaySamples);
    }
    
    // Initialize input diffusion network
    const int diffusionDelays[] = {113, 162, 204, 278};
    for (int i = 0; i < 4; ++i) {
        m_inputDiffusion[i].init(static_cast<int>(diffusionDelays[i] * sampleRateRatio));
    }
    
    // Initialize lattice networks for extra density
    m_latticeNetwork[0].init(
        static_cast<int>(89 * sampleRateRatio),
        static_cast<int>(137 * sampleRateRatio)
    );
    m_latticeNetwork[1].init(
        static_cast<int>(181 * sampleRateRatio),
        static_cast<int>(239 * sampleRateRatio)
    );
    
    // Initialize pre-delay buffers (max 100ms)
    int maxPredelaySamples = static_cast<int>(0.1f * m_sampleRate);
    m_predelayLeft.init(maxPredelaySamples);
    m_predelayRight.init(maxPredelaySamples);
}

void PlateReverb::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update parameter smoothing
    m_size.update();
    m_damping.update();
    m_predelay.update();
    m_mix.update();
    
    // Update high shelf filters for "air" (subtle boost at 12kHz)
    for (auto& shelf : m_highShelves) {
        shelf.updateCoefficients(12000.0f, 1.5f, m_sampleRate); // +1.5dB at 12kHz
    }
    
    // Process true stereo
    if (numChannels >= 2) {
        float* leftData = buffer.getWritePointer(0);
        float* rightData = buffer.getWritePointer(1);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Get input samples
            float leftIn = leftData[sample];
            float rightIn = rightData[sample];
            
            // Apply pre-delay
            float predelaySamples = m_predelay.current * 0.1f * m_sampleRate;
            float leftDelayed = m_predelayLeft.process(leftIn, predelaySamples);
            float rightDelayed = m_predelayRight.process(rightIn, predelaySamples + 3.7f); // Slight offset
            
            // Mix channels for reverb input (with stereo width preservation)
            float reverbInput = (leftDelayed + rightDelayed) * 0.5f;
            
            // Input diffusion stage
            for (auto& diffuser : m_inputDiffusion) {
                reverbInput = diffuser.process(reverbInput, 0.7f);
            }
            
            // Process through reverb
            float reverbLeft = processChannel(reverbInput, 0);
            float reverbRight = processChannel(reverbInput * 0.95f, 1); // Slight decorrelation
            
            // Additional lattice diffusion for density
            reverbLeft = m_latticeNetwork[0].process(reverbLeft);
            reverbRight = m_latticeNetwork[1].process(reverbRight);
            
            // Apply high shelf for brightness
            reverbLeft = m_highShelves[0].process(reverbLeft);
            reverbRight = m_highShelves[1].process(reverbRight);
            
            // DC blocking
            reverbLeft = m_dcBlockers[0].process(reverbLeft);
            reverbRight = m_dcBlockers[1].process(reverbRight);
            
            // Mix dry and wet
            leftData[sample] = leftIn * (1.0f - m_mix.current) + reverbLeft * m_mix.current;
            rightData[sample] = rightIn * (1.0f - m_mix.current) + reverbRight * m_mix.current;
        }
    }
    // Process mono as dual mono
    else if (numChannels == 1) {
        float* channelData = buffer.getWritePointer(0);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            
            // Apply pre-delay
            float predelaySamples = m_predelay.current * 0.1f * m_sampleRate;
            float delayed = m_predelayLeft.process(input, predelaySamples);
            
            // Input diffusion
            for (auto& diffuser : m_inputDiffusion) {
                delayed = diffuser.process(delayed, 0.7f);
            }
            
            // Process through reverb
            float reverb = processChannel(delayed, 0);
            
            // Additional processing
            reverb = m_latticeNetwork[0].process(reverb);
            reverb = m_highShelves[0].process(reverb);
            reverb = m_dcBlockers[0].process(reverb);
            
            // Mix
            channelData[sample] = input * (1.0f - m_mix.current) + reverb * m_mix.current;
        }
    }
}

float PlateReverb::processChannel(float input, int channel) {
    // Process through parallel comb filters
    float combSum = 0.0f;
    
    // Use different comb filters for left/right to increase stereo width
    int startIdx = (channel == 0) ? 0 : 1;
    
    for (int i = startIdx; i < NUM_COMBS; i += 2) {
        // Calculate feedback based on size parameter
        float feedback = 0.84f + m_size.current * 0.14f; // 0.84 to 0.98
        
        // Process comb filter with modulation
        float combOut = m_combs[i].process(input, feedback, m_damping.current, m_sampleRate);
        
        // Weight earlier reflections more heavily (like real plates)
        float weight = 1.0f - (i * 0.03f);
        combSum += combOut * weight;
    }
    
    // Normalize and apply some compression to prevent buildup
    float reverbSignal = std::tanh(combSum * 0.3f);
    
    // Process through series allpass filters for diffusion
    for (int i = 0; i < NUM_ALLPASS; ++i) {
        // Alternating positive/negative feedback for better diffusion
        float feedback = (i % 2 == 0) ? 0.5f : -0.5f;
        reverbSignal = m_allpasses[i].process(reverbSignal, feedback);
    }
    
    return reverbSignal;
}

// CombFilter implementation
float PlateReverb::CombFilter::process(float input, float feedback, float damping, double sampleRate) {
    // Update modulation
    modPhase += modRate / sampleRate;
    if (modPhase >= 1.0f) modPhase -= 1.0f;
    
    // Calculate modulated delay
    float modulation = std::sin(2.0f * M_PI * modPhase) * modDepth;
    int modulatedDelay = writePos - static_cast<int>(buffer.size() * 0.98f + modulation);
    if (modulatedDelay < 0) modulatedDelay += buffer.size();
    
    // Read from delay with linear interpolation
    float delayed = buffer[modulatedDelay];
    
    // Apply damping (one-pole lowpass)
    filterState = delayed * (1.0f - damping) + filterState * damping;
    
    // Calculate output
    float output = input + filterState * feedback;
    
    // Soft limiting to prevent explosion
    output = std::tanh(output * 0.7f) * 1.428f;
    
    // Write to buffer
    buffer[writePos] = output;
    writePos = (writePos + 1) % buffer.size();
    
    return output;
}

// AllpassFilter implementation
float PlateReverb::AllpassFilter::process(float input, float feedback) {
    float delayed = buffer[writePos];
    float output = -input + delayed;
    buffer[writePos] = input + delayed * feedback;
    writePos = (writePos + 1) % buffer.size();
    return output;
}

// PreDelay implementation
float PlateReverb::PreDelay::process(float input, float delaySamples) {
    // Clamp delay
    delaySamples = std::max(0.0f, std::min(delaySamples, buffer.size() - 1.0f));
    
    // Calculate read position
    float readPos = writePos - delaySamples;
    if (readPos < 0) readPos += buffer.size();
    
    // Linear interpolation
    int idx0 = static_cast<int>(readPos);
    int idx1 = (idx0 + 1) % buffer.size();
    float frac = readPos - idx0;
    
    float output = buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
    
    // Write input
    buffer[static_cast<int>(writePos)] = input;
    writePos += 1.0f;
    if (writePos >= buffer.size()) writePos -= buffer.size();
    
    return output;
}

// HighShelf implementation
void PlateReverb::HighShelf::updateCoefficients(float freq, float gainDB, double sampleRate) {
    float A = std::pow(10.0f, gainDB / 40.0f);
    float w = 2.0f * M_PI * freq / sampleRate;
    float cosw = std::cos(w);
    float sinw = std::sin(w);
    float beta = std::sqrt(A) / 0.7071f; // Q = 0.7071 (Butterworth)
    
    float b0 = A * ((A + 1) + (A - 1) * cosw + beta * sinw);
    float b1 = -2 * A * ((A - 1) + (A + 1) * cosw);
    float b2 = A * ((A + 1) + (A - 1) * cosw - beta * sinw);
    float a0_inv = 1.0f / ((A + 1) - (A - 1) * cosw + beta * sinw);
    float a1_raw = 2 * ((A - 1) - (A + 1) * cosw);
    float a2_raw = (A + 1) - (A - 1) * cosw - beta * sinw;
    
    // Normalize coefficients
    a0 = b0 * a0_inv;
    a1 = b1 * a0_inv;
    float a2 = b2 * a0_inv;
    b1 = -a1_raw * a0_inv;
    float b2 = -a2_raw * a0_inv;
    
    // Store for process (we only need first order for efficiency)
    this->a1 = a1 / a0;
    this->b1 = b1;
}

float PlateReverb::HighShelf::process(float input) {
    float output = a0 * input + a1 * x1 - b1 * y1;
    x1 = input;
    y1 = output;
    return output;
}

void PlateReverb::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) m_size.target = params.at(0);
    if (params.find(1) != params.end()) m_damping.target = params.at(1);
    if (params.find(2) != params.end()) m_predelay.target = params.at(2);
    if (params.find(3) != params.end()) m_mix.target = params.at(3);
}

juce::String PlateReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Size";
        case 1: return "Damping";
        case 2: return "Predelay";
        case 3: return "Mix";
        default: return "";
    }
}