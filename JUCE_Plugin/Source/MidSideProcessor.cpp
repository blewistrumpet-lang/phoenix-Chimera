// MidSideProcessor.cpp - Professional Studio-Quality Implementation
#include "MidSideProcessor.h"
#include <cmath>
#include <algorithm>

MidSideProcessor::MidSideProcessor() {
    // Initialize all parameter smoothers
    m_midGain = std::make_unique<SmoothedParameter>();
    m_sideGain = std::make_unique<SmoothedParameter>();
    m_width = std::make_unique<SmoothedParameter>();
    m_bassWidth = std::make_unique<SmoothedParameter>();
    m_highWidth = std::make_unique<SmoothedParameter>();
    m_midEqLow = std::make_unique<SmoothedParameter>();
    m_midEqMid = std::make_unique<SmoothedParameter>();
    m_midEqHigh = std::make_unique<SmoothedParameter>();
    m_sideEqHigh = std::make_unique<SmoothedParameter>();
    m_mode = std::make_unique<SmoothedParameter>();
    
    // Set professional defaults
    m_midGain->reset(0.0f);        // 0dB mid gain
    m_sideGain->reset(0.0f);       // 0dB side gain
    m_width->reset(1.0f);          // 100% width (normal)
    m_bassWidth->reset(0.5f);      // 50% bass width (some mono)
    m_highWidth->reset(1.2f);      // 120% high width (slight enhancement)
    m_midEqLow->reset(0.0f);       // 0dB low EQ
    m_midEqMid->reset(0.0f);       // 0dB mid EQ
    m_midEqHigh->reset(0.0f);      // 0dB high EQ
    m_sideEqHigh->reset(0.0f);     // 0dB side high EQ
    m_mode->reset(0.0f);           // Classic mode
}

void MidSideProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Configure parameter smoothers with appropriate rates
    m_midGain->setSampleRate(sampleRate, 20.0f);      // Fast for gain
    m_sideGain->setSampleRate(sampleRate, 20.0f);     // Fast for gain
    m_width->setSampleRate(sampleRate, 30.0f);        // Moderate for width
    m_bassWidth->setSampleRate(sampleRate, 40.0f);    // Slower for bass
    m_highWidth->setSampleRate(sampleRate, 40.0f);    // Slower for highs
    m_midEqLow->setSampleRate(sampleRate, 30.0f);     // Moderate for EQ
    m_midEqMid->setSampleRate(sampleRate, 30.0f);     // Moderate for EQ
    m_midEqHigh->setSampleRate(sampleRate, 30.0f);    // Moderate for EQ
    m_sideEqHigh->setSampleRate(sampleRate, 30.0f);   // Moderate for EQ
    m_mode->setSampleRate(sampleRate, 50.0f);         // Very slow for mode
    
    // Initialize processing components
    m_midEQ.prepare(sampleRate);
    m_sideEQ.prepare(sampleRate);
    m_widthProcessor.prepare(sampleRate);
    
    // Initialize dynamics processors
    for (auto& dyn : m_dynamics) {
        dyn.setParameters(0.8f, 3.0f, 5.0f, 50.0f, sampleRate);
    }
    
    // Initialize oversamplers
    for (auto& os : m_oversamplers) {
        os.prepare(sampleRate);
    }
    
    reset();
}

void MidSideProcessor::reset() {
    // Reset all DSP components
    m_midEQ.reset();
    m_sideEQ.reset();
    m_widthProcessor.reset();
    
    for (auto& dyn : m_dynamics) {
        dyn.reset();
    }
    
    for (auto& os : m_oversamplers) {
        os.reset();
    }
    
    // Clear work buffers
    for (auto& buffer : m_oversampledBuffers) {
        buffer.fill(0.0f);
    }
}

void MidSideProcessor::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels < 2 || numSamples == 0) {
        // M/S processing requires stereo input
        return;
    }
    
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    
    processStereo(left, right, numSamples);
}

void MidSideProcessor::processStereo(float* left, float* right, int numSamples) {
    // Update all parameters once per block
    float midGain = m_midGain->getNextValue();
    float sideGain = m_sideGain->getNextValue();
    float width = m_width->getNextValue();
    float bassWidth = m_bassWidth->getNextValue();
    float highWidth = m_highWidth->getNextValue();
    float midEqLow = m_midEqLow->getNextValue();
    float midEqMid = m_midEqMid->getNextValue();
    float midEqHigh = m_midEqHigh->getNextValue();
    float sideEqHigh = m_sideEqHigh->getNextValue();
    float modeParam = m_mode->getNextValue();
    
    // Convert parameters to usable values
    float midGainLin = std::pow(10.0f, midGain / 20.0f);  // dB to linear
    float sideGainLin = std::pow(10.0f, sideGain / 20.0f); // dB to linear
    
    // Update EQ coefficients only if needed
    static float lastMidEqLow = -999.0f;
    static float lastMidEqMid = -999.0f;
    static float lastMidEqHigh = -999.0f;
    static float lastSideEqHigh = -999.0f;
    
    bool eqChanged = false;
    if (std::abs(midEqLow - lastMidEqLow) > 0.001f ||
        std::abs(midEqMid - lastMidEqMid) > 0.001f ||
        std::abs(midEqHigh - lastMidEqHigh) > 0.001f) {
        
        m_midEQ.setBandGain(0, midEqLow);
        m_midEQ.setBandGain(1, midEqMid);
        m_midEQ.setBandGain(2, midEqHigh);
        m_midEQ.updateCoefficients(m_sampleRate);
        
        lastMidEqLow = midEqLow;
        lastMidEqMid = midEqMid;
        lastMidEqHigh = midEqHigh;
        eqChanged = true;
    }
    
    if (std::abs(sideEqHigh - lastSideEqHigh) > 0.001f) {
        m_sideEQ.setBandGain(2, sideEqHigh); // Only high band for side
        m_sideEQ.updateCoefficients(m_sampleRate);
        lastSideEqHigh = sideEqHigh;
        eqChanged = true;
    }
    
    // Set width parameters
    m_widthProcessor.setWidth(width, bassWidth, highWidth);
    
    // Determine processing mode
    m_processingMode = getModeFromParam(modeParam);
    
    // Determine if oversampling is needed
    m_useOversampling = (m_processingMode == ProcessingMode::MASTERING) ||
                       (std::abs(midGainLin - 1.0f) > 0.5f) ||
                       (std::abs(sideGainLin - 1.0f) > 0.5f);
    
    if (m_useOversampling) {
        // Process with 2x oversampling for highest quality
        
        // Upsample
        m_oversamplers[0].processUpsample(left, m_oversampledBuffers[0].data(), numSamples);
        m_oversamplers[1].processUpsample(right, m_oversampledBuffers[1].data(), numSamples);
        
        // Process at 2x sample rate
        for (int i = 0; i < numSamples * OVERSAMPLE_FACTOR; ++i) {
            float l = m_oversampledBuffers[0][i];
            float r = m_oversampledBuffers[1][i];
            
            // Encode to M/S
            float mid, side;
            encodeMidSide(l, r, mid, side);
            
            // Apply gains
            mid *= midGainLin;
            side *= sideGainLin;
            
            // Process mid channel EQ
            if (eqChanged || m_processingMode == ProcessingMode::MASTERING) {
                mid = m_midEQ.process(mid);
            }
            
            // Process side channel EQ (high frequencies only for sparkle)
            if ((eqChanged || m_processingMode == ProcessingMode::MASTERING) && 
                std::abs(sideEqHigh) > 0.01f) {
                side = m_sideEQ.process(side);
            }
            
            // Apply dynamics in mastering mode
            if (m_processingMode == ProcessingMode::MASTERING) {
                mid = m_dynamics[0].process(mid);
                side = m_dynamics[1].process(side);
            }
            
            // Decode back to L/R
            decodeMidSide(mid, side, l, r);
            
            // Apply stereo width processing
            if (m_processingMode == ProcessingMode::ENHANCED ||
                m_processingMode == ProcessingMode::CREATIVE) {
                m_widthProcessor.processStereo(l, r);
            }
            
            // Soft saturation to prevent clipping
            if (std::abs(l) > 0.95f) {
                l = std::tanh(l * 1.05f) * 0.952f;
            }
            if (std::abs(r) > 0.95f) {
                r = std::tanh(r * 1.05f) * 0.952f;
            }
            
            m_oversampledBuffers[0][i] = l;
            m_oversampledBuffers[1][i] = r;
        }
        
        // Downsample back
        m_oversamplers[0].processDownsample(m_oversampledBuffers[0].data(), left, numSamples);
        m_oversamplers[1].processDownsample(m_oversampledBuffers[1].data(), right, numSamples);
        
    } else {
        // Standard processing without oversampling
        
        // Process in blocks for efficiency
        const int blockSize = 32;
        int samplesProcessed = 0;
        
        while (samplesProcessed < numSamples) {
            int samplesToProcess = std::min(blockSize, numSamples - samplesProcessed);
            
            for (int i = 0; i < samplesToProcess; ++i) {
                int idx = samplesProcessed + i;
                float l = left[idx];
                float r = right[idx];
                
                // Encode to M/S
                float mid, side;
                encodeMidSide(l, r, mid, side);
                
                // Apply gains
                mid *= midGainLin;
                side *= sideGainLin;
                
                // Process mid channel EQ
                mid = m_midEQ.process(mid);
                
                // Process side channel EQ
                if (std::abs(sideEqHigh) > 0.01f) {
                    side = m_sideEQ.process(side);
                }
                
                // Decode back to L/R
                decodeMidSide(mid, side, l, r);
                
                // Apply stereo width processing
                if (m_processingMode == ProcessingMode::ENHANCED ||
                    m_processingMode == ProcessingMode::CREATIVE) {
                    m_widthProcessor.processStereo(l, r);
                }
                
                // Soft saturation
                if (std::abs(l) > 0.95f) {
                    l = std::tanh(l * 1.05f) * 0.952f;
                }
                if (std::abs(r) > 0.95f) {
                    r = std::tanh(r * 1.05f) * 0.952f;
                }
                
                left[idx] = l;
                right[idx] = r;
            }
            
            samplesProcessed += samplesToProcess;
        }
    }
    
    // Apply creative mode processing if needed
    if (m_processingMode == ProcessingMode::CREATIVE) {
        // Add subtle stereo movement
        static float creativePhase = 0.0f;
        float lfoRate = 0.3f; // Hz
        float lfoDepth = 0.02f; // Very subtle
        
        for (int i = 0; i < numSamples; ++i) {
            float lfo = std::sin(creativePhase) * lfoDepth;
            creativePhase += 2.0f * M_PI * lfoRate / m_sampleRate;
            if (creativePhase > 2.0f * M_PI) {
                creativePhase -= 2.0f * M_PI;
            }
            
            float temp = left[i];
            left[i] = left[i] * (1.0f + lfo) + right[i] * lfo;
            right[i] = right[i] * (1.0f - lfo) + temp * lfo;
        }
    }
}

void MidSideProcessor::encodeMidSide(float left, float right, float& mid, float& side) {
    // Standard M/S encoding with proper scaling
    mid = (left + right) * 0.5f;
    side = (left - right) * 0.5f;
}

void MidSideProcessor::decodeMidSide(float mid, float side, float& left, float& right) {
    // Standard M/S decoding
    left = mid + side;
    right = mid - side;
}

MidSideProcessor::ProcessingMode MidSideProcessor::getModeFromParam(float param) const {
    if (param < 0.25f) return ProcessingMode::CLASSIC;
    else if (param < 0.5f) return ProcessingMode::ENHANCED;
    else if (param < 0.75f) return ProcessingMode::CREATIVE;
    else return ProcessingMode::MASTERING;
}

void MidSideProcessor::updateParameters(const std::map<int, float>& params) {
    // Update parameter targets
    auto it = params.find(0);
    if (it != params.end()) m_midGain->setTarget((it->second - 0.5f) * 24.0f); // ±12dB
    
    it = params.find(1);
    if (it != params.end()) m_sideGain->setTarget((it->second - 0.5f) * 24.0f); // ±12dB
    
    it = params.find(2);
    if (it != params.end()) m_width->setTarget(it->second * 2.0f); // 0-200%
    
    it = params.find(3);
    if (it != params.end()) m_bassWidth->setTarget(it->second); // 0-100%
    
    it = params.find(4);
    if (it != params.end()) m_highWidth->setTarget(0.5f + it->second * 1.5f); // 50-200%
    
    it = params.find(5);
    if (it != params.end()) m_midEqLow->setTarget((it->second - 0.5f) * 12.0f); // ±6dB
    
    it = params.find(6);
    if (it != params.end()) m_midEqMid->setTarget((it->second - 0.5f) * 12.0f); // ±6dB
    
    it = params.find(7);
    if (it != params.end()) m_midEqHigh->setTarget((it->second - 0.5f) * 12.0f); // ±6dB
    
    it = params.find(8);
    if (it != params.end()) m_sideEqHigh->setTarget((it->second - 0.5f) * 12.0f); // ±6dB
    
    it = params.find(9);
    if (it != params.end()) m_mode->setTarget(it->second);
}

juce::String MidSideProcessor::getParameterName(int index) const {
    switch (index) {
        case 0: return "Mid Gain";
        case 1: return "Side Gain";
        case 2: return "Width";
        case 3: return "Bass Width";
        case 4: return "High Width";
        case 5: return "Mid Low";
        case 6: return "Mid Mid";
        case 7: return "Mid High";
        case 8: return "Side High";
        case 9: return "Mode";
        default: return "";
    }
}