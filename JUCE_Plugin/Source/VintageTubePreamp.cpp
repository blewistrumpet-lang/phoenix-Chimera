// VintageTubePreamp.cpp - Professional Studio-Quality Implementation
#include "VintageTubePreamp.h"
#include <algorithm>
#include <cmath>

VintageTubePreamp::VintageTubePreamp() {
    // Initialize all parameter smoothers
    m_inputGain = std::make_unique<ParameterSmoother>();
    m_drive = std::make_unique<ParameterSmoother>();
    m_bias = std::make_unique<ParameterSmoother>();
    m_bass = std::make_unique<ParameterSmoother>();
    m_mid = std::make_unique<ParameterSmoother>();
    m_treble = std::make_unique<ParameterSmoother>();
    m_presence = std::make_unique<ParameterSmoother>();
    m_outputGain = std::make_unique<ParameterSmoother>();
    m_tubeType = std::make_unique<ParameterSmoother>();
    m_mix = std::make_unique<ParameterSmoother>();
    
    // Set default values (professional preamp defaults)
    m_inputGain->reset(0.5f);   // Unity gain
    m_drive->reset(0.3f);        // Moderate drive
    m_bias->reset(0.5f);         // Centered bias
    m_bass->reset(0.5f);         // Flat EQ
    m_mid->reset(0.5f);          // Flat EQ
    m_treble->reset(0.5f);       // Flat EQ
    m_presence->reset(0.5f);     // Moderate presence
    m_outputGain->reset(0.5f);   // Unity output
    m_tubeType->reset(0.0f);     // 12AX7 default
    m_mix->reset(1.0f);          // 100% wet
}

void VintageTubePreamp::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Configure parameter smoothers with appropriate rates
    m_inputGain->setSampleRate(sampleRate, 10.0f);    // Fast for input
    m_drive->setSampleRate(sampleRate, 20.0f);        // Moderate
    m_bias->setSampleRate(sampleRate, 50.0f);         // Slow for bias
    m_bass->setSampleRate(sampleRate, 30.0f);         // Tone controls
    m_mid->setSampleRate(sampleRate, 30.0f);
    m_treble->setSampleRate(sampleRate, 30.0f);
    m_presence->setSampleRate(sampleRate, 30.0f);
    m_outputGain->setSampleRate(sampleRate, 10.0f);   // Fast for output
    m_tubeType->setSampleRate(sampleRate, 100.0f);    // Very slow for tube switching
    m_mix->setSampleRate(sampleRate, 20.0f);
    
    // Initialize all DSP components
    for (int ch = 0; ch < 2; ++ch) {
        // Transformers
        m_inputTransformers[ch].setSampleRate(sampleRate);
        m_outputTransformers[ch].setSampleRate(sampleRate);
        
        // Tube stages
        m_tubeStages1[ch].setSampleRate(sampleRate);
        m_tubeStages1[ch].setTubeType(TubeType::TUBE_12AX7);
        
        m_tubeStages2[ch].setSampleRate(sampleRate);
        m_tubeStages2[ch].setTubeType(TubeType::TUBE_12AX7);
        
        // Tone stack
        m_toneStacks[ch].setSampleRate(sampleRate);
        
        // DC blockers
        m_inputDCBlockers[ch].setCutoff(10.0, sampleRate);   // 10Hz high-pass
        m_outputDCBlockers[ch].setCutoff(5.0, sampleRate);    // 5Hz high-pass
        
        // Oversampling
        m_oversamplers[ch].prepare(samplesPerBlock, sampleRate);
    }
    
    reset();
}

void VintageTubePreamp::reset() {
    // Reset all DSP components
    for (int ch = 0; ch < 2; ++ch) {
        m_inputTransformers[ch].reset();
        m_outputTransformers[ch].reset();
        m_tubeStages1[ch].reset();
        m_tubeStages2[ch].reset();
        m_toneStacks[ch].reset();
        m_inputDCBlockers[ch].reset();
        m_outputDCBlockers[ch].reset();
        m_oversamplers[ch].reset();
    }
    
    // Clear work buffers
    for (auto& buffer : m_oversampledBuffers) {
        buffer.fill(0.0f);
    }
}

void VintageTubePreamp::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Process stereo or mono
    if (numChannels == 1) {
        // Mono processing
        float* data = buffer.getWritePointer(0);
        processStereo(data, data, numSamples);
    } else {
        // Stereo processing
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        processStereo(left, right, numSamples);
    }
}

void VintageTubePreamp::processStereo(float* left, float* right, int numSamples) {
    // Update all parameters once per block
    float inputGain = m_inputGain->process();
    float drive = m_drive->process();
    float bias = m_bias->process();
    float bass = m_bass->process();
    float mid = m_mid->process();
    float treble = m_treble->process();
    float presence = m_presence->process();
    float outputGain = m_outputGain->process();
    float tubeTypeParam = m_tubeType->process();
    float mix = m_mix->process();
    
    // Convert parameters to usable ranges
    float inputLevel = std::pow(10.0f, (inputGain - 0.5f) * 40.0f / 20.0f);  // ±20dB
    float outputLevel = std::pow(10.0f, (outputGain - 0.5f) * 40.0f / 20.0f); // ±20dB
    
    // Determine tube type
    TubeType currentTubeType = getTubeTypeFromParam(tubeTypeParam);
    
    // Update tube types if changed
    static TubeType lastTubeType = TubeType::TUBE_12AX7;
    if (currentTubeType != lastTubeType) {
        for (int ch = 0; ch < 2; ++ch) {
            m_tubeStages1[ch].setTubeType(currentTubeType);
            m_tubeStages2[ch].setTubeType(currentTubeType);
        }
        lastTubeType = currentTubeType;
    }
    
    // Process each channel
    float* channelData[2] = {left, right};
    
    for (int ch = 0; ch < 2; ++ch) {
        float* data = channelData[ch];
        
        if (m_useOversampling) {
            // Upsample to 4x
            float* oversampledData = m_oversampledBuffers[ch].data();
            m_oversamplers[ch].processUpsample(data, oversampledData, numSamples);
            
            // Process at 4x sample rate
            int oversampledSamples = numSamples * OVERSAMPLE_FACTOR;
            
            for (int i = 0; i < oversampledSamples; ++i) {
                float input = oversampledData[i];
                float dry = input;
                
                // Input gain
                input *= inputLevel;
                
                // DC blocking
                input = m_inputDCBlockers[ch].process(input);
                
                // Input transformer
                input = m_inputTransformers[ch].processInput(input, drive * 0.5f);
                
                // First tube stage
                input = m_tubeStages1[ch].process(input, drive, bias, m_sampleRate * OVERSAMPLE_FACTOR);
                
                // Interstage coupling (AC coupled)
                input = m_inputDCBlockers[ch].process(input);
                
                // Second tube stage (with less drive)
                input = m_tubeStages2[ch].process(input, drive * 0.7f, bias, m_sampleRate * OVERSAMPLE_FACTOR);
                
                // Tone stack (at oversampled rate)
                input = m_toneStacks[ch].process(input, bass, mid, treble + presence * 0.3f);
                
                // Output transformer
                input = m_outputTransformers[ch].processOutput(input);
                
                // Thermal noise (subtle)
                float noise = m_noiseModels[ch].process(300.0f + drive * 50.0f);
                input += noise * 0.0001f;
                
                // Output DC blocking
                input = m_outputDCBlockers[ch].process(input);
                
                // Output gain
                input *= outputLevel;
                
                // Soft limiting
                if (std::abs(input) > 0.95f) {
                    input = std::tanh(input * 0.8f) / 0.8f;
                }
                
                // Mix dry/wet
                oversampledData[i] = input * mix + dry * (1.0f - mix);
            }
            
            // Downsample back to original rate
            m_oversamplers[ch].processDownsample(oversampledData, numSamples);
            
            // Copy back to output
            std::copy(oversampledData, oversampledData + numSamples, data);
            
        } else {
            // Non-oversampled processing (lower quality but lower CPU)
            for (int i = 0; i < numSamples; ++i) {
                float input = data[i];
                float dry = input;
                
                // Input gain
                input *= inputLevel;
                
                // DC blocking
                input = m_inputDCBlockers[ch].process(input);
                
                // Input transformer
                input = m_inputTransformers[ch].processInput(input, drive * 0.5f);
                
                // First tube stage
                input = m_tubeStages1[ch].process(input, drive, bias, m_sampleRate);
                
                // Interstage coupling
                input = m_inputDCBlockers[ch].process(input);
                
                // Second tube stage
                input = m_tubeStages2[ch].process(input, drive * 0.7f, bias, m_sampleRate);
                
                // Tone stack
                input = m_toneStacks[ch].process(input, bass, mid, treble + presence * 0.3f);
                
                // Output transformer
                input = m_outputTransformers[ch].processOutput(input);
                
                // Output DC blocking
                input = m_outputDCBlockers[ch].process(input);
                
                // Output gain
                input *= outputLevel;
                
                // Soft limiting
                if (std::abs(input) > 0.95f) {
                    input = std::tanh(input * 0.8f) / 0.8f;
                }
                
                // Mix dry/wet
                data[i] = input * mix + dry * (1.0f - mix);
            }
        }
    }
}

VintageTubePreamp::TubeType VintageTubePreamp::getTubeTypeFromParam(float param) const {
    // Map 0-1 parameter to tube types
    int typeIndex = static_cast<int>(param * 7.99f); // 8 tube types
    
    switch (typeIndex) {
        case 0: return TubeType::TUBE_12AX7;
        case 1: return TubeType::TUBE_12AU7;
        case 2: return TubeType::TUBE_12AT7;
        case 3: return TubeType::TUBE_6SN7;
        case 4: return TubeType::TUBE_ECC88;
        case 5: return TubeType::TUBE_6V6;
        case 6: return TubeType::TUBE_EL34;
        case 7: return TubeType::TUBE_EL84;
        default: return TubeType::TUBE_12AX7;
    }
}

void VintageTubePreamp::updateParameters(const std::map<int, float>& params) {
    // Update parameter targets
    auto it = params.find(0);
    if (it != params.end()) m_inputGain->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(1);
    if (it != params.end()) m_drive->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(2);
    if (it != params.end()) m_bias->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(3);
    if (it != params.end()) m_bass->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(4);
    if (it != params.end()) m_mid->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(5);
    if (it != params.end()) m_treble->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(6);
    if (it != params.end()) m_presence->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(7);
    if (it != params.end()) m_outputGain->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(8);
    if (it != params.end()) m_tubeType->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(9);
    if (it != params.end()) m_mix->setTarget(std::clamp(it->second, 0.0f, 1.0f));
}

juce::String VintageTubePreamp::getParameterName(int index) const {
    switch (index) {
        case 0: return "Input Gain";
        case 1: return "Drive";
        case 2: return "Bias";
        case 3: return "Bass";
        case 4: return "Mid";
        case 5: return "Treble";
        case 6: return "Presence";
        case 7: return "Output Gain";
        case 8: return "Tube Type";
        case 9: return "Mix";
        default: return "";
    }
}