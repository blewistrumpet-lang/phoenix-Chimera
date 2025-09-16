//==============================================================================
// VintageOptoCompressor - Mathematically Correct T4B Opto Cell Implementation
//==============================================================================

#include "VintageOptoCompressor.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// MATHEMATICAL FORMULAS AND CONSTANTS
//==============================================================================

namespace OptoConstants {
    // T4B Opto Cell Characteristics
    static constexpr float DARK_RESISTANCE = 1e6f;      // 1MΩ in darkness
    static constexpr float LIGHT_RESISTANCE = 1e3f;      // 1kΩ in bright light
    static constexpr float ATTACK_TIME_MS = 10.0f;       // 10ms attack
    static constexpr float RELEASE_TIME_BASE_MS = 60.0f; // Base release time
    static constexpr float PROGRAM_DEPENDENT_FACTOR = 5.0f; // Release multiplier
    
    // Voltage Divider Network (T4B Cell + Fixed Resistor)
    static constexpr float FIXED_RESISTOR = 22e3f;       // 22kΩ fixed resistor
    
    // Soft Knee Parameters
    static constexpr float KNEE_WIDTH_DB = 6.0f;         // Default knee width
}

//==============================================================================
// CORRECTED OPTO CELL MODEL
//==============================================================================

class CorrectOptoCell {
public:
    void reset() {
        ledBrightness = 0.0f;
        ldrResistance = OptoConstants::DARK_RESISTANCE;
        programDependentState = 0.0f;
    }
    
    void updateCell(float inputLevel, double sampleRate) {
        // Convert input level to LED brightness (logarithmic response)
        float targetBrightness = std::log10(1.0f + inputLevel * 9.0f) / std::log10(10.0f);
        targetBrightness = std::clamp(targetBrightness, 0.0f, 1.0f);
        
        // Calculate time constants with sample rate compensation
        float attackCoeff, releaseCoeff;
        
        if (targetBrightness > ledBrightness) {
            // Attack: Fast LED response
            attackCoeff = 1.0f - std::exp(-1.0f / (OptoConstants::ATTACK_TIME_MS * 0.001f * sampleRate));
            ledBrightness += attackCoeff * (targetBrightness - ledBrightness);
        } else {
            // Release: Program-dependent slow release
            float programDependentRelease = OptoConstants::RELEASE_TIME_BASE_MS * 
                                          (1.0f + programDependentState * OptoConstants::PROGRAM_DEPENDENT_FACTOR);
            releaseCoeff = 1.0f - std::exp(-1.0f / (programDependentRelease * 0.001f * sampleRate));
            ledBrightness += releaseCoeff * (targetBrightness - ledBrightness);
        }
        
        // Update program-dependent state (memory effect)
        if (targetBrightness > 0.3f) {
            programDependentState = std::max(programDependentState, targetBrightness * 0.8f);
        } else {
            programDependentState *= 0.9995f; // Slow decay
        }
        
        // Convert brightness to LDR resistance (exponential relationship)
        float resistanceRatio = std::exp(-ledBrightness * 6.9f); // ln(1000) ≈ 6.9
        ldrResistance = OptoConstants::LIGHT_RESISTANCE + 
                       (OptoConstants::DARK_RESISTANCE - OptoConstants::LIGHT_RESISTANCE) * resistanceRatio;
    }
    
    float getGainReduction() const {
        // Voltage divider formula: Vout/Vin = R_ldr / (R_fixed + R_ldr)
        float voltageDividerRatio = ldrResistance / (OptoConstants::FIXED_RESISTOR + ldrResistance);
        
        // Convert to gain reduction (VCA gain law - square root for natural response)
        return std::sqrt(voltageDividerRatio);
    }
    
private:
    float ledBrightness = 0.0f;
    float ldrResistance = OptoConstants::DARK_RESISTANCE;
    float programDependentState = 0.0f;
};

//==============================================================================
// CORRECTED SOFT KNEE FUNCTION (dB domain)
//==============================================================================

float correctSoftKnee(float inputDb, float thresholdDb, float kneeWidthDb) {
    float kneeStart = thresholdDb - kneeWidthDb * 0.5f;
    float kneeEnd = thresholdDb + kneeWidthDb * 0.5f;
    
    if (inputDb <= kneeStart) {
        return 0.0f; // No compression
    } else if (inputDb >= kneeEnd) {
        return inputDb - thresholdDb; // Full compression
    } else {
        // Smooth quadratic transition in knee region
        float x = (inputDb - kneeStart) / kneeWidthDb;
        float compressionRatio = x * x; // Quadratic curve
        return (inputDb - thresholdDb) * compressionRatio;
    }
}

//==============================================================================
// CORRECTED DUAL TIME CONSTANT ENVELOPE DETECTOR
//==============================================================================

class DualTimeConstantDetector {
public:
    void prepare(double sampleRate) {
        fs = sampleRate;
        reset();
    }
    
    void reset() {
        peakEnvelope = 0.0f;
        rmsEnvelope = 0.0f;
        rmsBuffer.fill(0.0f);
        bufferIndex = 0;
        rmsSum = 0.0f;
    }
    
    float process(float input, float attackMs, float releaseMs) {
        float absInput = std::abs(input);
        
        // Peak detector with attack/release
        float peakCoeff = (absInput > peakEnvelope) ? 
            1.0f - std::exp(-1.0f / (attackMs * 0.001f * fs)) :
            1.0f - std::exp(-1.0f / (releaseMs * 0.001f * fs));
        
        peakEnvelope += peakCoeff * (absInput - peakEnvelope);
        
        // RMS detector (sliding window for stability)
        rmsSum -= rmsBuffer[bufferIndex];
        rmsBuffer[bufferIndex] = input * input;
        rmsSum += rmsBuffer[bufferIndex];
        bufferIndex = (bufferIndex + 1) % RMS_WINDOW_SIZE;
        
        rmsEnvelope = std::sqrt(rmsSum / RMS_WINDOW_SIZE);
        
        // Combine peak and RMS (weighted toward RMS for smooth compression)
        return rmsEnvelope * 0.8f + peakEnvelope * 0.2f;
    }
    
private:
    static constexpr int RMS_WINDOW_SIZE = 64;
    std::array<float, RMS_WINDOW_SIZE> rmsBuffer;
    int bufferIndex = 0;
    float rmsSum = 0.0f;
    float peakEnvelope = 0.0f;
    float rmsEnvelope = 0.0f;
    double fs = 44100.0;
};

//==============================================================================
// MAIN PROCESSING FUNCTION
//==============================================================================

void VintageOptoCompressor::processCorrect(juce::AudioBuffer<float>& buffer) {
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    // Get parameter values (properly scaled)
    float inputGainDb = m_gain.current * 40.0f;        // 0 to 40dB
    float inputGainLinear = std::pow(10.0f, inputGainDb / 20.0f);
    
    float thresholdDb = -30.0f + m_peakReduction.current * 25.0f; // -30 to -5dB
    float kneeWidthDb = m_knee.current * OptoConstants::KNEE_WIDTH_DB;
    
    float outputGainDb = (m_outputGain.current - 0.5f) * 20.0f;   // -10 to +10dB
    float outputGainLinear = std::pow(10.0f, outputGainDb / 20.0f);
    
    // Process each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        
        for (int n = 0; n < numSamples; ++n) {
            float input = channelData[n];
            float dry = input;
            
            // Apply input gain
            input *= inputGainLinear;
            
            // Envelope detection
            float envelope = m_detectors[ch].process(input, 1.0f, 10.0f);
            float envelopeDb = 20.0f * std::log10(std::max(1e-6f, envelope));
            
            // Update opto cell with envelope level
            m_optoCells[ch].updateCell(envelope, m_sampleRate);
            
            // Calculate compression amount using soft knee
            float compressionDb = correctSoftKnee(envelopeDb, thresholdDb, kneeWidthDb);
            
            // Get gain reduction from opto cell (natural VCA response)
            float optoGainReduction = m_optoCells[ch].getGainReduction();
            
            // Combine calculated compression with opto cell response
            float totalGainReductionDb = compressionDb * (1.0f - optoGainReduction);
            float gainReductionLinear = std::pow(10.0f, -totalGainReductionDb / 20.0f);
            
            // Apply compression
            float compressed = input * gainReductionLinear;
            
            // Apply output gain
            compressed *= outputGainLinear;
            
            // Dry/wet mix
            float output = compressed * m_mix.current + dry * (1.0f - m_mix.current);
            
            channelData[n] = output;
        }
    }
}

//==============================================================================
// MEMBER VARIABLES (add to header)
//==============================================================================
/*
private:
    std::array<CorrectOptoCell, 2> m_optoCells;
    std::array<DualTimeConstantDetector, 2> m_detectors;
    double m_sampleRate = 44100.0;
    
    // Smoothed parameters (existing)
    SmoothParam m_gain;
    SmoothParam m_peakReduction;
    SmoothParam m_outputGain;
    SmoothParam m_mix;
    SmoothParam m_knee;
*/