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
        // BitCrusher parameters  
        parameterMap["Bit Crusher_Bits"] = {TYPE_NORMALIZED, 1.0f, 24.0f, 16.0f, false, " bits"};
        parameterMap["Bit Crusher_Downsample"] = {TYPE_NORMALIZED, 1.0f, 16.0f, 1.0f, false, "x"};
        parameterMap["Bit Crusher_Mix"] = {TYPE_PERCENT, 0.0f, 100.0f, 100.0f, false, "%"};
        
        // Dynamic EQ parameters
        parameterMap["Dynamic EQ_Frequency"] = {TYPE_FREQUENCY, 20.0f, 20000.0f, 1000.0f, true, " Hz"};
        parameterMap["Dynamic EQ_Threshold"] = {TYPE_DECIBEL, -60.0f, 0.0f, -12.0f, false, " dB"};
        parameterMap["Dynamic EQ_Ratio"] = {TYPE_RATIO, 1.0f, 20.0f, 4.0f, true, ":1"};
        parameterMap["Dynamic EQ_Attack"] = {TYPE_TIME_MS, 0.1f, 100.0f, 10.0f, true, " ms"};
        parameterMap["Dynamic EQ_Release"] = {TYPE_TIME_MS, 10.0f, 1000.0f, 100.0f, true, " ms"};
        parameterMap["Dynamic EQ_Gain"] = {TYPE_DECIBEL, -18.0f, 18.0f, 0.0f, false, " dB"};
        parameterMap["Dynamic EQ_Mix"] = {TYPE_PERCENT, 0.0f, 100.0f, 100.0f, false, "%"};
        
        // Vintage Tube Preamp parameters
        parameterMap["Vintage Tube Preamp Studio_Drive"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["Vintage Tube Preamp Studio_Input Trim (dB)"] = {TYPE_DECIBEL, -24.0f, 24.0f, 0.0f, false, " dB"};
        parameterMap["Vintage Tube Preamp Studio_Output Trim (dB)"] = {TYPE_DECIBEL, -24.0f, 24.0f, 0.0f, false, " dB"};
        parameterMap["Vintage Tube Preamp Studio_Bright"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["Vintage Tube Preamp Studio_Bass"] = {TYPE_DECIBEL, -12.0f, 12.0f, 0.0f, false, " dB"};
        parameterMap["Vintage Tube Preamp Studio_Mid"] = {TYPE_DECIBEL, -12.0f, 12.0f, 0.0f, false, " dB"};
        parameterMap["Vintage Tube Preamp Studio_Treble"] = {TYPE_DECIBEL, -12.0f, 12.0f, 0.0f, false, " dB"};
        parameterMap["Vintage Tube Preamp Studio_Presence"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        
        // Intelligent Harmonizer parameters
        parameterMap["Intelligent Harmonizer_Voices"] = {TYPE_VOICES, 1.0f, 4.0f, 2.0f, false, ""};
        parameterMap["Intelligent Harmonizer_Master Mix"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["Intelligent Harmonizer_Voice 1 Vol"] = {TYPE_PERCENT, 0.0f, 100.0f, 100.0f, false, "%"};
        parameterMap["Intelligent Harmonizer_Voice 2 Vol"] = {TYPE_PERCENT, 0.0f, 100.0f, 80.0f, false, "%"};
        parameterMap["Intelligent Harmonizer_Voice 3 Vol"] = {TYPE_PERCENT, 0.0f, 100.0f, 60.0f, false, "%"};
        
        // Chaos Generator parameters
        parameterMap["Chaos Generator_Rate"] = {TYPE_FREQUENCY, 0.01f, 20.0f, 1.0f, true, " Hz"};
        parameterMap["Chaos Generator_Depth"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["Chaos Generator_Smoothing"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["Chaos Generator_Mix"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        
        // Gain Utility parameters
        parameterMap["Gain Utility_Gain"] = {TYPE_DECIBEL, -60.0f, 24.0f, 0.0f, false, " dB"};
        parameterMap["Gain Utility_Left Gain"] = {TYPE_DECIBEL, -12.0f, 12.0f, 0.0f, false, " dB"};
        parameterMap["Gain Utility_Right Gain"] = {TYPE_DECIBEL, -12.0f, 12.0f, 0.0f, false, " dB"};
        parameterMap["Gain Utility_Mid Gain"] = {TYPE_DECIBEL, -12.0f, 12.0f, 0.0f, false, " dB"};
        parameterMap["Gain Utility_Side Gain"] = {TYPE_DECIBEL, -12.0f, 12.0f, 0.0f, false, " dB"};
        
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
        
        // Distortion parameters
        parameterMap["Distortion_Drive"] = {TYPE_DECIBEL, 0.0f, 40.0f, 12.0f, false, " dB"};
        parameterMap["Distortion_Tone"] = {TYPE_FREQUENCY, 100.0f, 10000.0f, 2000.0f, true, " Hz"};
        parameterMap["BitCrusher_Bits"] = {TYPE_NORMALIZED, 1.0f, 16.0f, 8.0f, false, " bits"};
        parameterMap["BitCrusher_Rate"] = {TYPE_FREQUENCY, 1000.0f, 48000.0f, 22050.0f, true, " Hz"};
        
        // Modulation parameters
        parameterMap["Chorus_Rate"] = {TYPE_FREQUENCY, 0.1f, 10.0f, 1.0f, true, " Hz"};
        parameterMap["Chorus_Depth"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["Phaser_Rate"] = {TYPE_FREQUENCY, 0.01f, 10.0f, 0.5f, true, " Hz"};
        parameterMap["Phaser_Depth"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["Phaser_Feedback"] = {TYPE_PERCENT, -95.0f, 95.0f, 0.0f, false, "%"};
        parameterMap["Phaser_Stages"] = {TYPE_STAGES, 2.0f, 24.0f, 4.0f, false, ""};
        
        // Tremolo/Vibrato parameters
        parameterMap["Tremolo_Rate"] = {TYPE_FREQUENCY, 0.1f, 20.0f, 5.0f, true, " Hz"};
        parameterMap["Tremolo_Depth"] = {TYPE_PERCENT, 0.0f, 100.0f, 50.0f, false, "%"};
        parameterMap["Vibrato_Rate"] = {TYPE_FREQUENCY, 0.1f, 10.0f, 4.0f, true, " Hz"};
        parameterMap["Vibrato_Depth"] = {TYPE_SEMITONES, 0.0f, 2.0f, 0.5f, false, " st"};
        
        // EQ parameters
        parameterMap["EQ_LowGain"] = {TYPE_DECIBEL, -18.0f, 18.0f, 0.0f, false, " dB"};
        parameterMap["EQ_MidGain"] = {TYPE_DECIBEL, -18.0f, 18.0f, 0.0f, false, " dB"};
        parameterMap["EQ_HighGain"] = {TYPE_DECIBEL, -18.0f, 18.0f, 0.0f, false, " dB"};
        parameterMap["EQ_LowFreq"] = {TYPE_FREQUENCY, 20.0f, 1000.0f, 100.0f, true, " Hz"};
        parameterMap["EQ_MidFreq"] = {TYPE_FREQUENCY, 200.0f, 8000.0f, 1000.0f, true, " Hz"};
        parameterMap["EQ_HighFreq"] = {TYPE_FREQUENCY, 1000.0f, 20000.0f, 8000.0f, true, " Hz"};
        
        // Gate parameters
        parameterMap["Gate_Threshold"] = {TYPE_DECIBEL, -80.0f, 0.0f, -40.0f, false, " dB"};
        parameterMap["Gate_Attack"] = {TYPE_TIME_MS, 0.01f, 100.0f, 1.0f, true, " ms"};
        parameterMap["Gate_Hold"] = {TYPE_TIME_MS, 0.0f, 500.0f, 10.0f, false, " ms"};
        parameterMap["Gate_Release"] = {TYPE_TIME_MS, 1.0f, 5000.0f, 100.0f, true, " ms"};
        
        // Utility parameters
        parameterMap["Pan_Position"] = {TYPE_PAN, -100.0f, 100.0f, 0.0f, false, ""};
        parameterMap["Width_Amount"] = {TYPE_PERCENT, 0.0f, 200.0f, 100.0f, false, "%"};
        parameterMap["Gain_Amount"] = {TYPE_DECIBEL, -60.0f, 24.0f, 0.0f, false, " dB"};
        
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
            // Smart default formatting based on parameter name
            juce::String lowerParam = paramName.toLowerCase();
            
            // Try to guess the type from parameter name
            if (lowerParam.contains("freq") || lowerParam.contains("cutoff") || 
                lowerParam.contains("x-over") || lowerParam.contains("crossover")) {
                // Frequency - logarithmic 20Hz to 20kHz
                float freq = 20.0f * std::pow(1000.0f, normalizedValue);
                if (freq >= 1000.0f) {
                    return juce::String(freq / 1000.0f, 1) + " kHz";
                } else {
                    return juce::String((int)freq) + " Hz";
                }
            }
            else if (lowerParam.contains("gain") || lowerParam.contains("trim") ||
                     lowerParam.contains("threshold") || lowerParam.contains("ceiling")) {
                // Decibels - -60 to +12 range
                float db = -60.0f + normalizedValue * 72.0f;
                return juce::String(db, 1) + " dB";
            }
            else if (lowerParam.contains("time") || lowerParam.contains("delay")) {
                // Time in ms - 0 to 1000ms
                float ms = normalizedValue * 1000.0f;
                if (ms >= 1000.0f) {
                    return juce::String(ms / 1000.0f, 2) + " s";
                } else {
                    return juce::String((int)ms) + " ms";
                }
            }
            else if (lowerParam.contains("attack") || lowerParam.contains("release") ||
                     lowerParam.contains("hold") || lowerParam.contains("decay")) {
                // Envelope times - logarithmic 0.1ms to 5000ms
                float ms = 0.1f * std::pow(50000.0f, normalizedValue);
                if (ms >= 1000.0f) {
                    return juce::String(ms / 1000.0f, 2) + " s";
                } else {
                    return juce::String(ms, 1) + " ms";
                }
            }
            else if (lowerParam.contains("mix") || lowerParam.contains("depth") ||
                     lowerParam.contains("amount") || lowerParam.contains("drive") ||
                     lowerParam.contains("feedback") || lowerParam.contains("resonance")) {
                // Percentage 0-100%
                return juce::String((int)(normalizedValue * 100)) + "%";
            }
            else if (lowerParam.contains("ratio")) {
                // Compression ratio 1:1 to 20:1
                float ratio = 1.0f + normalizedValue * 19.0f;
                return juce::String(ratio, 1) + ":1";
            }
            else if (lowerParam.contains("pitch") || lowerParam.contains("semi") ||
                     lowerParam.contains("detune")) {
                // Semitones -12 to +12
                float st = -12.0f + normalizedValue * 24.0f;
                if (st > 0) return "+" + juce::String(st, 1) + " st";
                else return juce::String(st, 1) + " st";
            }
            else if (lowerParam.contains("pan")) {
                // Pan L100 to R100
                float pan = -100.0f + normalizedValue * 200.0f;
                if (pan < -1.0f) return "L" + juce::String(-pan, 0);
                else if (pan > 1.0f) return "R" + juce::String(pan, 0);
                else return "C";
            }
            else if (lowerParam.contains("width") || lowerParam.contains("stereo")) {
                // Stereo width 0-200%
                return juce::String((int)(normalizedValue * 200)) + "%";
            }
            else {
                // Default to percentage for anything else
                return juce::String((int)(normalizedValue * 100)) + "%";
            }
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
                
            case TYPE_PAN:
                // Format pan as L100 to R100
                if (actualValue < 0) {
                    formatted = "L" + juce::String(-actualValue, 0);
                } else if (actualValue > 0) {
                    formatted = "R" + juce::String(actualValue, 0);
                } else {
                    formatted = "C";
                }
                break;
                
            case TYPE_VOICES:
                formatted = juce::String((int)actualValue);
                break;
                
            case TYPE_STAGES:
                formatted = juce::String((int)actualValue);
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