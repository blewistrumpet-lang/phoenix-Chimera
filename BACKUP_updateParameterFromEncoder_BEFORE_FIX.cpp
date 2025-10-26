// BACKUP of original buggy implementation
// From PluginProcessor.cpp lines 1785-1837
// Date: October 24, 2025

void ChimeraAudioProcessor::updateParameterFromEncoder(int encoderIndex, float delta) {
    auto behavior = controlState->getEncoderBehavior(encoderIndex);
    auto& state = controlState->getState();

    // Use the parameterID from behavior to control the appropriate parameter
    auto* param = parameters.getParameter(behavior.parameterID);
    if (param) {
        // Special handling for preset_index (integer parameter)
        if (behavior.parameterID == "preset_index") {
            // Use getRawParameterValue for atomic read (returns normalized 0-1)
            auto* rawValue = parameters.getRawParameterValue("preset_index");
            if (rawValue) {
                // Convert normalized value back to integer (0-9)
                int currentValue = static_cast<int>(rawValue->load() * 9.0f + 0.5f);
                int newValue = currentValue + static_cast<int>(delta);  // Add delta
                newValue = juce::jlimit(0, 9, newValue);  // Clamp to 0-9

                DBG("Preset browse: " << currentValue << " + " << delta << " = " << newValue);

                // Convert to normalized 0-1 range for JUCE
                float normalized = static_cast<float>(newValue) / 9.0f;
                param->setValueNotifyingHost(normalized);

                // Update preset manager's current index
                if (gpioPresetManager) {
                    gpioPresetManager->setCurrentPresetIndex(newValue);
                }

                DBG("Preset index set to " << newValue << " (normalized: " << normalized << ")");
                return;
            }
        }

        // Standard float parameter handling
        float currentValue = param->getValue();
        float newValue = juce::jlimit(0.0f, 1.0f, currentValue + delta * behavior.sensitivity);
        param->setValueNotifyingHost(newValue);
        DBG("Parameter " << behavior.parameterID << " changed to " << newValue);

        // Also save to active A/B bank
        if (abStateEngine) {
            // Convert normalized value back to actual range
            float actualValue = newValue;
            if (behavior.parameterID == "input_gain" || behavior.parameterID == "output_level") {
                actualValue = newValue * 2.0f;  // 0-1 → 0-2
            }
            abStateEngine->setParameter(behavior.parameterID, actualValue);
        }
    } else {
        // Log what parameter would be controlled (for future implementation)
        DBG("Would control " << behavior.parameterID << " in " << state.getModeString() << " mode");
    }
}