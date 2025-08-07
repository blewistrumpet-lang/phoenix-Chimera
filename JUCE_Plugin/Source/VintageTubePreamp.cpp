#include "VintageTubePreamp.h"
#include <cmath>
#include <algorithm>

// Constructor
VintageTubePreamp::VintageTubePreamp() {
    // Initialize parameter smoothers
    m_inputGain = std::make_unique<SmoothedParameter>();
    m_drive = std::make_unique<SmoothedParameter>();
    m_bias = std::make_unique<SmoothedParameter>();
    m_bass = std::make_unique<SmoothedParameter>();
    m_mid = std::make_unique<SmoothedParameter>();
    m_treble = std::make_unique<SmoothedParameter>();
    m_presence = std::make_unique<SmoothedParameter>();
    m_outputGain = std::make_unique<SmoothedParameter>();
    m_tubeType = std::make_unique<SmoothedParameter>();
    m_mix = std::make_unique<SmoothedParameter>();
}

// Destructor is defaulted in header

void VintageTubePreamp::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Setup smoothers
    m_inputGain->setSampleRate(sampleRate, 10.0);
    m_drive->setSampleRate(sampleRate, 20.0);
    m_bias->setSampleRate(sampleRate, 50.0);
    m_bass->setSampleRate(sampleRate, 30.0);
    m_mid->setSampleRate(sampleRate, 30.0);
    m_treble->setSampleRate(sampleRate, 30.0);
    m_presence->setSampleRate(sampleRate, 30.0);
    m_outputGain->setSampleRate(sampleRate, 10.0);
    m_tubeType->setSampleRate(sampleRate, 100.0);
    m_mix->setSampleRate(sampleRate, 20.0);
    
    // Default values
    m_inputGain->reset(0.5);
    m_drive->reset(0.3);
    m_bias->reset(0.5);
    m_bass->reset(0.5);
    m_mid->reset(0.5);
    m_treble->reset(0.5);
    m_presence->reset(0.5);
    m_outputGain->reset(0.5);
    m_tubeType->reset(0.0);
    m_mix->reset(1.0);
    
    // Prepare tube models
    for (auto& tube : m_tubeModels) {
        tube.setTubeType(AdvancedTubeModel::TubeType::ECC83_12AX7);
    }
    
    // Prepare transformers
    for (auto& transformer : m_transformers) {
        transformer.prepare(sampleRate);
    }
    
    // Prepare EQs
    for (auto& eq : m_eqs) {
        eq.prepare(sampleRate);
    }
    
    // Prepare power supplies
    for (auto& ps : m_powerSupplies) {
        ps.reset();
    }
    
    // Prepare oversamplers
    for (auto& os : m_oversamplers) {
        os.prepare(sampleRate);
    }
    
    // Prepare input stage
    m_inputStage.prepare(sampleRate);
}

void VintageTubePreamp::process(juce::AudioBuffer<float>& buffer) {
    processStereo(buffer.getWritePointer(0), 
                  buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr,
                  buffer.getNumSamples());
}

void VintageTubePreamp::reset() {
    // Reset tube models
    for (auto& tube : m_tubeModels) {
        tube.reset();
    }
    
    // Reset transformers
    for (auto& transformer : m_transformers) {
        transformer.reset();
    }
    
    // Reset EQs
    for (auto& eq : m_eqs) {
        eq.reset();
    }
    
    // Reset power supplies
    for (auto& ps : m_powerSupplies) {
        ps.reset();
    }
    
    // Reset oversamplers
    for (auto& os : m_oversamplers) {
        os.reset();
    }
    
    // Reset input stage
    m_inputStage.reset();
    
    // Clear convolution indices
    m_convolutionIndex.fill(0);
}

void VintageTubePreamp::updateParameters(const std::map<int, float>& params) {
    for (const auto& [id, value] : params) {
        switch (id) {
            case 0:  // Input Gain
                m_inputGain->setTarget(value);
                break;
            case 1:  // Drive
                m_drive->setTarget(value);
                break;
            case 2:  // Bias
                m_bias->setTarget(value);
                break;
            case 3:  // Bass
                m_bass->setTarget(value);
                break;
            case 4:  // Mid
                m_mid->setTarget(value);
                break;
            case 5:  // Treble
                m_treble->setTarget(value);
                break;
            case 6:  // Presence
                m_presence->setTarget(value);
                break;
            case 7:  // Output Gain
                m_outputGain->setTarget(value);
                break;
            case 8:  // Tube Type
                m_tubeType->setTarget(value);
                break;
            case 9:  // Mix
                m_mix->setTarget(value);
                break;
        }
    }
}

juce::String VintageTubePreamp::getParameterName(int index) const {
    switch (index) {
        case 0:  return "Input Gain";
        case 1:  return "Drive";
        case 2:  return "Bias";
        case 3:  return "Bass";
        case 4:  return "Mid";
        case 5:  return "Treble";
        case 6:  return "Presence";
        case 7:  return "Output Gain";
        case 8:  return "Tube Type";
        case 9:  return "Mix";
        default: return "Param " + juce::String(index + 1);
    }
}

// Private methods implementation
void VintageTubePreamp::processStereo(float* left, float* right, int numSamples) {
    // Process left channel
    processChannel(reinterpret_cast<double*>(m_oversampledBuffers[0].data()), 0, numSamples);
    
    // Process right channel if stereo
    if (right) {
        processChannel(reinterpret_cast<double*>(m_oversampledBuffers[1].data()), 1, numSamples);
    }
    
    // Copy back to float buffers
    for (int i = 0; i < numSamples; ++i) {
        left[i] = static_cast<float>(m_oversampledBuffers[0][i]);
        if (right) {
            right[i] = static_cast<float>(m_oversampledBuffers[1][i]);
        }
    }
}

void VintageTubePreamp::processChannel(double* buffer, int channel, int numSamples) {
    // Get smoothed parameters
    double inputLevel = m_inputGain->getNextValue();
    double driveAmount = m_drive->getNextValue();
    double biasAmount = m_bias->getNextValue();
    double bassAmount = m_bass->getNextValue();
    double midAmount = m_mid->getNextValue();
    double trebleAmount = m_treble->getNextValue();
    double presenceAmount = m_presence->getNextValue();
    double outputLevel = m_outputGain->getNextValue();
    double mixAmount = m_mix->getNextValue();
    
    // Update tube type if needed
    double tubeTypeValue = m_tubeType->getNextValue();
    auto newTubeType = getTubeTypeFromParam(static_cast<float>(tubeTypeValue));
    
    // Simple processing for now - to be expanded
    for (int i = 0; i < numSamples; ++i) {
        double sample = buffer[i] * inputLevel;
        double dry = sample;
        
        // Input stage modeling
        sample = m_inputStage.process(sample, m_sampleRate);
        
        // Tube stage
        sample = m_tubeModels[channel].process(sample, driveAmount, biasAmount, m_sampleRate);
        
        // Transformer
        sample = m_transformers[channel].process(sample, m_sampleRate);
        
        // EQ
        m_eqs[channel].setParams(bassAmount, 0.5, midAmount, 0.5, trebleAmount + presenceAmount * 0.3, m_sampleRate);
        sample = m_eqs[channel].process(sample);
        
        // Output gain and mix
        sample *= outputLevel;
        buffer[i] = sample * mixAmount + dry * (1.0 - mixAmount);
    }
}

VintageTubePreamp::AdvancedTubeModel::TubeType VintageTubePreamp::getTubeTypeFromParam(float param) const {
    int typeIndex = static_cast<int>(param * 9.99f);
    typeIndex = juce::jlimit(0, 9, typeIndex);
    
    switch (typeIndex) {
        case 0: return AdvancedTubeModel::TubeType::ECC83_12AX7;
        case 1: return AdvancedTubeModel::TubeType::ECC82_12AU7;
        case 2: return AdvancedTubeModel::TubeType::ECC81_12AT7;
        case 3: return AdvancedTubeModel::TubeType::EF86;
        case 4: return AdvancedTubeModel::TubeType::E88CC_6922;
        case 5: return AdvancedTubeModel::TubeType::EL34;
        case 6: return AdvancedTubeModel::TubeType::EL84;
        case 7: return AdvancedTubeModel::TubeType::KT88;
        case 8: return AdvancedTubeModel::TubeType::MODEL_300B;
        case 9: return AdvancedTubeModel::TubeType::MODEL_2A3;
        default: return AdvancedTubeModel::TubeType::ECC83_12AX7;
    }
}