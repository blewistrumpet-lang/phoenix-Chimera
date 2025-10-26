// BUG FIX for ChimeraAudioProcessor::updateParameterFromEncoder
// Replace lines 1785-1829 in PluginProcessor.cpp with this corrected version

void ChimeraAudioProcessor::updateParameterFromEncoder(int encoderIndex, float delta) {
    auto behavior = controlState->getEncoderBehavior(encoderIndex);
    auto& state = controlState->getState();

    // Get the parameter
    auto* param = parameters.getParameter(behavior.parameterID);
    if (!param) {
        DBG("WARNING: Parameter not found: " << behavior.parameterID);
        return;
    }

    // Handle AudioParameterInt (preset_index) specially
    auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param);
    if (intParam) {
        // FIXED: Use proper get() method for integer parameters
        int currentValue = intParam->get();  // Returns actual value (0-9)
        int intDelta = static_cast<int>(delta);

        // Calculate new value and clamp to range
        int newValue = currentValue + intDelta;
        newValue = juce::jlimit(intParam->getRange().getStart(),
                                intParam->getRange().getEnd(),
                                newValue);

        DBG("=== INT PARAM UPDATE ===");
        DBG("  Parameter: " << behavior.parameterID);
        DBG("  Current: " << currentValue);
        DBG("  Delta: " << intDelta);
        DBG("  New Value: " << newValue);
        DBG("  Range: " << intParam->getRange().getStart() << " to " << intParam->getRange().getEnd());

        // Convert to normalized 0-1 for setValueNotifyingHost
        float normalized = intParam->convertTo0to1(newValue);
        intParam->setValueNotifyingHost(normalized);

        // Special handling for preset_index
        if (behavior.parameterID == "preset_index" && gpioPresetManager) {
            gpioPresetManager->setCurrentPresetIndex(newValue);
            DBG("Preset manager index updated to: " << newValue);
        }

        return;
    }

    // Handle AudioParameterFloat (all other parameters)
    auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param);
    if (floatParam) {
        // Get current normalized value (0-1)
        float currentValue = floatParam->getValue();

        // Apply delta with sensitivity
        float newValue = currentValue + (delta * behavior.sensitivity);
        newValue = juce::jlimit(0.0f, 1.0f, newValue);

        // Set the new value
        floatParam->setValueNotifyingHost(newValue);

        DBG("Float param " << behavior.parameterID << " changed: "
            << currentValue << " -> " << newValue);

        // Update A/B bank with actual (denormalized) value
        if (abStateEngine) {
            float actualValue = floatParam->convertFrom0to1(newValue);
            abStateEngine->setParameter(behavior.parameterID, actualValue);
            DBG("  Saved to " << (abStateEngine->isBankB() ? "Bank B" : "Bank A")
                << ": " << actualValue);
        }

        return;
    }

    // Fallback for other parameter types
    float currentValue = param->getValue();
    float newValue = juce::jlimit(0.0f, 1.0f, currentValue + delta * behavior.sensitivity);
    param->setValueNotifyingHost(newValue);
    DBG("Generic param " << behavior.parameterID << " changed to " << newValue);
}