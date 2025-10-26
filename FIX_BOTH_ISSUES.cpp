// COMPREHENSIVE FIX for both Mix Freeze and Preset Display issues
// Replace updateParameterFromEncoder in PluginProcessor.cpp (lines 1785-1875)

void ChimeraAudioProcessor::updateParameterFromEncoder(int encoderIndex, float delta) {
    if (!controlState) return;

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
        // Get actual integer value (not normalized)
        int currentValue = intParam->get();
        int intDelta = (delta > 0) ? 1 : (delta < 0) ? -1 : 0;

        if (intDelta != 0) {
            int newValue = currentValue + intDelta;
            newValue = juce::jlimit(intParam->getRange().getStart(),
                                   intParam->getRange().getEnd(),
                                   newValue);

            DBG("=== INT PARAM UPDATE ===");
            DBG("  Parameter: " << behavior.parameterID);
            DBG("  Current: " << currentValue);
            DBG("  Delta: " << intDelta);
            DBG("  New Value: " << newValue);

            // Set using the base parameter class method with proper normalization
            float normalized = static_cast<float>(newValue - intParam->getRange().getStart()) /
                             static_cast<float>(intParam->getRange().getLength());
            param->setValueNotifyingHost(normalized);

            // Update preset manager
            if (behavior.parameterID == "preset_index" && gpioPresetManager) {
                gpioPresetManager->setCurrentPresetIndex(newValue);
                DBG("Preset manager index updated to: " << newValue);
            }
        }
        return;
    }

    // Handle AudioParameterFloat (all other parameters)
    auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param);
    if (floatParam) {
        // Get the actual current value (not normalized)
        float currentActual = floatParam->get();

        // Calculate the change in actual units
        float range = floatParam->getNormalisableRange().getLength();
        float actualDelta = delta * behavior.sensitivity * range;

        // Calculate new actual value
        float newActual = currentActual + actualDelta;
        newActual = juce::jlimit(floatParam->getNormalisableRange().start,
                                floatParam->getNormalisableRange().end,
                                newActual);

        DBG("=== FLOAT PARAM UPDATE ===");
        DBG("  Parameter: " << behavior.parameterID);
        DBG("  Current actual: " << currentActual);
        DBG("  Delta: " << delta << " (actual: " << actualDelta << ")");
        DBG("  New actual: " << newActual);

        // Set using the parameter's actual value
        *floatParam = newActual;

        // Update A/B bank with the actual value
        if (abStateEngine) {
            abStateEngine->setParameter(behavior.parameterID, newActual);
            DBG("  Saved to " << (abStateEngine->isBankB() ? "Bank B" : "Bank A")
                << ": " << newActual);
        }

        return;
    }

    // Fallback for other parameter types
    float currentValue = param->getValue();
    float newValue = juce::jlimit(0.0f, 1.0f, currentValue + delta * behavior.sensitivity);
    param->setValueNotifyingHost(newValue);
    DBG("Generic param " << behavior.parameterID << " changed to " << newValue);
}

// ===========================================================================
// ADDITIONAL FIX: Update timerCallback display section in PluginEditor_Pi.cpp
// Around line 400-410, replace the encoder display update with:

// In timerCallback(), around line 400:
if (encoderDisplays[i] && controlState) {
    auto behavior = controlState->getEncoderBehavior(i);
    juce::String paramName = behavior.label;
    float normalizedValue = 0.0f;

    // Special handling for preset_index
    if (behavior.parameterID == "preset_index") {
        if (auto* presetParam = dynamic_cast<juce::AudioParameterInt*>(
            audioProcessor.parameters.getParameter("preset_index"))) {
            int presetIndex = presetParam->get();
            paramName = "Preset " + juce::String(presetIndex + 1);  // Show 1-10
            normalizedValue = static_cast<float>(presetIndex) / 9.0f;
        }
    } else {
        // Regular parameter handling
        if (auto* paramValue = audioProcessor.parameters.getRawParameterValue(behavior.parameterID)) {
            // Get actual value for display
            if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(
                audioProcessor.parameters.getParameter(behavior.parameterID))) {
                normalizedValue = floatParam->getValue();
            } else {
                normalizedValue = paramValue->load();
            }
        }
    }

    juce::String bankIndicator = (abStateEngine && abStateEngine->isBankB()) ? "B" : "A";
    encoderDisplays[i]->setParameterInfo(paramName, normalizedValue, behavior.parameterID, bankIndicator);
}