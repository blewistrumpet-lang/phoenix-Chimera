#pragma once

// Comprehensive debug logging helpers for GPIO parameter tracking
// Include this in PluginProcessor.cpp and PluginEditor_Pi.cpp

#define DEBUG_GPIO 1

#if DEBUG_GPIO
    #define GPIO_DBG(msg) DBG("[GPIO] " << msg)
    #define PARAM_DBG(msg) DBG("[PARAM] " << msg)
    #define BANK_DBG(msg) DBG("[BANK] " << msg)
    #define PRESET_DBG(msg) DBG("[PRESET] " << msg)
    #define ENCODER_DBG(msg) DBG("[ENCODER] " << msg)
    #define DISPLAY_DBG(msg) DBG("[DISPLAY] " << msg)
#else
    #define GPIO_DBG(msg)
    #define PARAM_DBG(msg)
    #define BANK_DBG(msg)
    #define PRESET_DBG(msg)
    #define ENCODER_DBG(msg)
    #define DISPLAY_DBG(msg)
#endif

// Trace macro for following parameter values through the system
#define TRACE_PARAM(id, value, context) \
    DBG(">>> TRACE [" << context << "] " << id << " = " << value << \
        " @ " << __FILE__ << ":" << __LINE__)

// Log parameter state at key points
inline void logParameterState(const juce::String& context,
                              const juce::String& paramID,
                              float normalizedValue,
                              float actualValue,
                              const juce::String& extra = "") {
    DBG("=== PARAMETER STATE: " << context << " ===");
    DBG("  Param ID: " << paramID);
    DBG("  Normalized: " << normalizedValue);
    DBG("  Actual: " << actualValue);
    if (extra.isNotEmpty()) {
        DBG("  " << extra);
    }
    DBG("  Thread: " << (juce::MessageManager::getInstance()->isThisTheMessageThread() ? "MESSAGE" : "AUDIO"));
    DBG("=====================================");
}

// Log encoder event details
inline void logEncoderEvent(int encoderIndex, float delta, const juce::String& paramID) {
    DBG("=== ENCODER EVENT ===");
    DBG("  Encoder: " << encoderIndex);
    DBG("  Delta: " << delta);
    DBG("  Target Param: " << paramID);
    DBG("  Timestamp: " << juce::Time::getMillisecondCounter());
    DBG("====================");
}

// Log A/B bank operations
inline void logBankOperation(const juce::String& operation,
                             bool isBankB,
                             float inputGain,
                             float mixWetDry,
                             float outputLevel) {
    DBG("=== BANK OPERATION: " << operation << " ===");
    DBG("  Active Bank: " << (isBankB ? "B" : "A"));
    DBG("  Input Gain: " << inputGain);
    DBG("  Mix Wet/Dry: " << mixWetDry);
    DBG("  Output Level: " << outputLevel);
    DBG("====================================");
}

// Track preset operations
inline void logPresetOperation(const juce::String& operation,
                               int presetIndex,
                               bool success,
                               const juce::String& details = "") {
    DBG("=== PRESET OPERATION: " << operation << " ===");
    DBG("  Index: " << presetIndex);
    DBG("  Success: " << (success ? "YES" : "NO"));
    if (details.isNotEmpty()) {
        DBG("  Details: " << details);
    }
    DBG("=========================================");
}