#include "ResonantChorus.h"
#include "DspEngineUtilities.h"

ResonantChorus::ResonantChorus() {
    // Initialize parameter smoothers
    m_rateParam.setTimeMs(15.0, 44100.0);
    m_depthParam.setTimeMs(15.0, 44100.0);
    m_resonanceParam.setTimeMs(15.0, 44100.0);
    m_mixParam.setTimeMs(15.0, 44100.0);
    m_widthParam.setTimeMs(15.0, 44100.0);
}

void ResonantChorus::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_samplesPerBlock = samplesPerBlock;
    
    // Update parameter smoother sample rates
    m_rateParam.setTimeMs(15.0, sampleRate);
    m_depthParam.setTimeMs(15.0, sampleRate);
    m_resonanceParam.setTimeMs(15.0, sampleRate);
    m_mixParam.setTimeMs(15.0, sampleRate);
    m_widthParam.setTimeMs(15.0, sampleRate);
    
    // Prepare left channel voices
    for (int voice = 0; voice < NUM_VOICES; ++voice) {
        m_leftVoices[voice].prepare(sampleRate, BASE_DELAYS[voice], LFO_PHASES[voice]);
    }
    
    // Prepare right channel voices with slightly different phase offsets for stereo width
    for (int voice = 0; voice < NUM_VOICES; ++voice) {
        float rightPhaseOffset = LFO_PHASES[voice] + (M_PI / 6.0f); // 30 degree offset
        m_rightVoices[voice].prepare(sampleRate, BASE_DELAYS[voice], rightPhaseOffset);
    }
}

void ResonantChorus::reset() {
    // Reset all parameter smoothers
    m_rateParam.reset(0.8f);  // Default rate
    m_depthParam.reset(0.4f); // Default depth
    m_resonanceParam.reset(0.7f); // Default resonance
    m_mixParam.reset(0.5f);   // Default mix
    m_widthParam.reset(1.0f); // Default width
    
    // Reset all voices
    for (int voice = 0; voice < NUM_VOICES; ++voice) {
        m_leftVoices[voice].reset();
        m_rightVoices[voice].reset();
    }
}

void ResonantChorus::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Early bypass check for mix parameter
    float currentMix = m_mixParam.process(m_mixTarget);
    if (currentMix < 0.001f) {
        // Completely dry - no processing needed, just update parameters for smooth operation
        m_rateParam.process(m_rateTarget);
        m_depthParam.process(m_depthTarget);
        m_resonanceParam.process(m_resonanceTarget);
        m_widthParam.process(m_widthTarget);
        return;
    }
    
    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample) {
        // Get smoothed parameter values (using current target values)
        float rate = m_rateParam.process(m_rateTarget);           // 0.1 - 1.5 Hz
        float depth = m_depthParam.process(m_depthTarget);         // 0.2 - 0.6 samples modulation
        float resonance = m_resonanceParam.process(m_resonanceTarget); // 0.5 - 1.0 Q
        float mix = currentMix;                                  // Use pre-calculated mix value
        float width = m_widthParam.process(m_widthTarget);         // 0.0 - 1.5 stereo width
        
        // Map parameters to proper ranges
        rate = 0.01f + rate * 1.99f;        // 0.01 - 2.0 Hz (musical chorus range)
        depth = 0.2f + depth * 0.4f;        // 0.2 - 0.6 samples
        resonance = 0.5f + resonance * 0.5f; // Q 0.5 - 1.0
        width = width * 1.5f;                // 0.0 - 1.5
        
        // Process left channel
        float leftInput = buffer.getSample(0, sample);
        float leftChorusSum = 0.0f;
        
        for (int voice = 0; voice < NUM_VOICES; ++voice) {
            leftChorusSum += m_leftVoices[voice].process(leftInput, rate, depth, resonance);
        }
        leftChorusSum /= static_cast<float>(NUM_VOICES); // Normalize by voice count
        
        // Process right channel (or copy from left if mono)
        float rightInput = (numChannels > 1) ? buffer.getSample(1, sample) : leftInput;
        float rightChorusSum = 0.0f;
        
        for (int voice = 0; voice < NUM_VOICES; ++voice) {
            rightChorusSum += m_rightVoices[voice].process(rightInput, rate, depth, resonance);
        }
        rightChorusSum /= static_cast<float>(NUM_VOICES); // Normalize by voice count
        
        // Apply stereo width processing
        if (numChannels > 1) {
            float mid = (leftChorusSum + rightChorusSum) * 0.5f;
            float side = (leftChorusSum - rightChorusSum) * 0.5f * width;
            leftChorusSum = mid + side;
            rightChorusSum = mid - side;
        }
        
        // Apply equal-power crossfade between dry and wet signals
        float leftOutput = equalPowerMix(leftInput, leftChorusSum, mix);
        float rightOutput = (numChannels > 1) ? 
            equalPowerMix(rightInput, rightChorusSum, mix) : leftOutput;
        
        // Flush denormals and clamp output
        leftOutput = DSPUtils::flushDenorm(leftOutput);
        rightOutput = DSPUtils::flushDenorm(rightOutput);
        
        // Write to buffer
        buffer.setSample(0, sample, clampSafe(leftOutput, -2.0f, 2.0f));
        if (numChannels > 1) {
            buffer.setSample(1, sample, clampSafe(rightOutput, -2.0f, 2.0f));
        }
    }
}

void ResonantChorus::updateParameters(const std::map<int, float>& params) {
    // Parameter indices:
    // 0: Rate (0.0 - 1.0)
    // 1: Depth (0.0 - 1.0) 
    // 2: Resonance (0.0 - 1.0)
    // 3: Mix (0.0 - 1.0)
    // 4: Width (0.0 - 1.0)
    
    for (const auto& param : params) {
        float value = clampSafe(param.second, 0.0f, 1.0f);
        
        switch (param.first) {
            case 0: // Rate
                m_rateTarget = value;
                break;
            case 1: // Depth
                m_depthTarget = value;
                break;
            case 2: // Resonance
                m_resonanceTarget = value;
                break;
            case 3: // Mix
                m_mixTarget = value;
                break;
            case 4: // Width
                m_widthTarget = value;
                break;
            default:
                break;
        }
    }
}

juce::String ResonantChorus::getParameterName(int index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Resonance";
        case 3: return "Mix";
        case 4: return "Width";
        default: return "Unknown";
    }
}