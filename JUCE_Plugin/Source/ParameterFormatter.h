#pragma once
#include <JuceHeader.h>
#include <map>
#include <string>

/**
 * ParameterFormatter - Converts normalized 0-1 parameter values to meaningful display strings
 * 
 * This class handles the UI/UX issue where parameters show meaningless 0-1 values
 * instead of actual units like Hz, dB, ms, semitones, etc.
 */
class ParameterFormatter {
public:
    enum ParameterType {
        TYPE_NORMALIZED,   // 0-1 value (default)
        TYPE_FREQUENCY,    // Hz (20Hz - 20kHz log scale)
        TYPE_TIME_MS,      // milliseconds (0-2000ms)
        TYPE_TIME_S,       // seconds (0-10s)
        TYPE_DECIBEL,      // dB (-60 to +12)
        TYPE_SEMITONES,    // semitones (-24 to +24)
        TYPE_PERCENT,      // percentage (0-100%)
        TYPE_RATIO,        // compression ratio (1:1 to 20:1)
        TYPE_MODE,         // discrete mode selection
        TYPE_OCTAVES,      // octaves (-2 to +2)
        TYPE_BPM_SYNC,     // tempo sync (1/32 to 4 bars)
        TYPE_Q_FACTOR,     // filter Q (0.1 to 20)
        TYPE_PAN,          // pan position (L100 to R100)
        TYPE_VOICES,       // voice count (1-16)
        TYPE_STAGES        // phaser/filter stages (2-24)
    };
    
    struct ParameterInfo {
        ParameterType type;
        float min;
        float max;
        float defaultValue;
        bool isLogarithmic;
        std::string suffix;
    };
    
    // Map of engine/parameter combinations to their display types
    static std::map<std::string, ParameterInfo> parameterMap;
    
    static void initializeParameterMap() {
        // PitchShifter parameters
        parameterMap["PitchShifter_Pitch"] = {TYPE_SEMITONES, -24.0f, 24.0f, 0.0f, false, " st"};
        parameterMap["PitchShifter_Formant"] = {TYPE_SEMITONES, -12.0f, 12.0f, 0.0f, false, " st"};
        parameterMap["PitchShifter_Mix"] = {TYPE_PERCENT, 0.0f, 100.0f, 100.0f, false, "%"};
        parameterMap["PitchShifter_Window"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["PitchShifter_Gate"] = {TYPE_DECIBEL, -60.0f, 0.0f, -60.0f, false, " dB"};
        parameterMap["PitchShifter_Grain"] = {TYPE_TIME_MS, 10.0f, 500.0f, 100.0f, true, " ms"};
        parameterMap["PitchShifter_Feedback"] = {TYPE_PERCENT, 0.0f, 90.0f, 0.0f, false, "%"};
        parameterMap["PitchShifter_Width"] = {TYPE_PERCENT, 0.0f, 200.0f, 100.0f, false, "%"};
        
        // Delay parameters
        parameterMap["Delay_Time"] = {TYPE_TIME_MS, 0.0f, 2000.0f, 250.0f, false, " ms"};
        parameterMap["Delay_Feedback"] = {TYPE_PERCENT, 0.0f, 95.0f, 50.0f, false, "%"};
        parameterMap["Delay_Mix"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["Delay_Filter"] = {TYPE_FREQUENCY, 20.0f, 20000.0f, 5000.0f, true, " Hz"};
        
        // Compressor parameters
        parameterMap["Compressor_Threshold"] = {TYPE_DECIBEL, -60.0f, 0.0f, -12.0f, false, " dB"};
        parameterMap["Compressor_Ratio"] = {TYPE_RATIO, 1.0f, 20.0f, 4.0f, true, ":1"};
        parameterMap["Compressor_Attack"] = {TYPE_TIME_MS, 0.1f, 100.0f, 10.0f, true, " ms"};
        parameterMap["Compressor_Release"] = {TYPE_TIME_MS, 10.0f, 1000.0f, 100.0f, true, " ms"};
        parameterMap["Compressor_Knee"] = {TYPE_DECIBEL, 0.0f, 12.0f, 2.0f, false, " dB"};
        parameterMap["Compressor_Makeup"] = {TYPE_DECIBEL, 0.0f, 24.0f, 0.0f, false, " dB"};
        
        // Filter parameters
        parameterMap["Filter_Frequency"] = {TYPE_FREQUENCY, 20.0f, 20000.0f, 1000.0f, true, " Hz"};
        parameterMap["Filter_Resonance"] = {TYPE_Q_FACTOR, 0.5f, 20.0f, 1.0f, true, ""};
        parameterMap["LowPass_Frequency"] = {TYPE_FREQUENCY, 20.0f, 20000.0f, 5000.0f, true, " Hz"};
        parameterMap["HighPass_Frequency"] = {TYPE_FREQUENCY, 20.0f, 20000.0f, 100.0f, true, " Hz"};
        
        // Reverb parameters
        parameterMap["Reverb_Size"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["Reverb_Decay"] = {TYPE_TIME_S, 0.1f, 10.0f, 2.0f, true, " s"};
        parameterMap["Reverb_Damping"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["Reverb_PreDelay"] = {TYPE_TIME_MS, 0.0f, 200.0f, 20.0f, false, " ms"};
        
        // Add more mappings as needed...
    }
    
    /**
     * Format a normalized parameter value (0-1) for display
     */
    static juce::String formatValue(const juce::String& engineName, 
                                    const juce::String& paramName, 
                                    float normalizedValue) {
        // Initialize map on first use
        if (parameterMap.empty()) {
            initializeParameterMap();
        }
        
        // Look up parameter info
        juce::String key = engineName + "_" + paramName;
        auto it = parameterMap.find(key.toStdString());
        
        if (it == parameterMap.end()) {
            // Default formatting for unknown parameters
            return juce::String(normalizedValue, 2);
        }
        
        const ParameterInfo& info = it->second;
        float actualValue;
        
        // Convert normalized to actual value
        if (info.isLogarithmic) {
            // Logarithmic scaling
            float logMin = std::log10(info.min);
            float logMax = std::log10(info.max);
            float logValue = logMin + normalizedValue * (logMax - logMin);
            actualValue = std::pow(10.0f, logValue);
        } else {
            // Linear scaling
            actualValue = info.min + normalizedValue * (info.max - info.min);
        }
        
        // Format based on type
        juce::String formatted;
        
        switch (info.type) {
            case TYPE_SEMITONES:
                // Show + for positive values
                if (actualValue > 0) {
                    formatted = "+" + juce::String(actualValue, 1);
                } else {
                    formatted = juce::String(actualValue, 1);
                }
                break;
                
            case TYPE_FREQUENCY:
                if (actualValue >= 1000.0f) {
                    formatted = juce::String(actualValue / 1000.0f, 2) + " k";
                } else {
                    formatted = juce::String(actualValue, 0);
                }
                break;
                
            case TYPE_TIME_MS:
                if (actualValue >= 1000.0f) {
                    formatted = juce::String(actualValue / 1000.0f, 2) + " s";
                    return formatted;  // Skip suffix
                } else {
                    formatted = juce::String(actualValue, 0);
                }
                break;
                
            case TYPE_TIME_S:
                formatted = juce::String(actualValue, 2);
                break;
                
            case TYPE_DECIBEL:
                formatted = juce::String(actualValue, 1);
                break;
                
            case TYPE_PERCENT:
                formatted = juce::String(actualValue, 0);
                break;
                
            case TYPE_RATIO:
                formatted = juce::String(actualValue, 1);
                break;
                
            case TYPE_Q_FACTOR:
                formatted = juce::String(actualValue, 1);
                break;
                
            default:
                formatted = juce::String(actualValue, 2);
                break;
        }
        
        return formatted + info.suffix;
    }
    
    /**
     * Parse a display string back to normalized value
     */
    static float parseValue(const juce::String& engineName,
                           const juce::String& paramName,
                           const juce::String& text) {
        // Initialize map on first use
        if (parameterMap.empty()) {
            initializeParameterMap();
        }
        
        // Look up parameter info
        juce::String key = engineName + "_" + paramName;
        auto it = parameterMap.find(key.toStdString());
        
        if (it == parameterMap.end()) {
            // Default parsing for unknown parameters
            return text.getFloatValue();
        }
        
        const ParameterInfo& info = it->second;
        
        // Remove suffix and clean up
        juce::String cleanText = text.trimEnd();
        if (info.suffix.length() > 0) {
            cleanText = cleanText.upToLastOccurrenceOf(info.suffix, false, false);
        }
        
        // Handle special cases
        if (info.type == TYPE_FREQUENCY && cleanText.contains("k")) {
            cleanText = cleanText.replace("k", "");
            float khz = cleanText.getFloatValue();
            cleanText = juce::String(khz * 1000.0f);
        }
        
        float actualValue = cleanText.getFloatValue();
        
        // Convert actual to normalized
        float normalized;
        if (info.isLogarithmic) {
            float logMin = std::log10(info.min);
            float logMax = std::log10(info.max);
            float logValue = std::log10(std::max(actualValue, info.min));
            normalized = (logValue - logMin) / (logMax - logMin);
        } else {
            normalized = (actualValue - info.min) / (info.max - info.min);
        }
        
        return juce::jlimit(0.0f, 1.0f, normalized);
    }
};