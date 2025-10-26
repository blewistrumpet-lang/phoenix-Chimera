// FIXED version of updateParameterFromEncoder that prevents feedback loop
// Replace the entire function in PluginProcessor.cpp (lines 1785-1949)

void ChimeraAudioProcessor::updateParameterFromEncoder(int encoderIndex, float delta) {
    if (!controlState) return;

    auto behavior = controlState->getEncoderBehavior(encoderIndex);
    auto& state = controlState->getState();

    DBG(""); // Empty line for readability
    DBG("╔════════════════════════════════════════════════════════════════╗");
    DBG("║ ENCODER EVENT START                                            ║");
    DBG("╠════════════════════════════════════════════════════════════════╣");
    DBG("║ Encoder: " << encoderIndex << " | Delta: " << delta);
    DBG("║ Target Param: " << behavior.parameterID);
    DBG("╚════════════════════════════════════════════════════════════════╝");

    // Get the parameter
    auto* param = parameters.getParameter(behavior.parameterID);
    if (!param) {
        DBG("✗ ERROR: Parameter not found: " << behavior.parameterID);
        return;
    }

    // Handle AudioParameterInt (preset_index) specially
    auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param);
    if (intParam) {
        DBG("┌─── INTEGER PARAMETER HANDLING ───┐");

        // Get actual integer value (not normalized)
        int currentValue = intParam->get();
        int rangeStart = intParam->getRange().getStart();
        int rangeEnd = intParam->getRange().getEnd();

        DBG("│ Current value: " << currentValue);
        DBG("│ Range: [" << rangeStart << " to " << rangeEnd << "]");

        int intDelta = (delta > 0) ? 1 : (delta < 0) ? -1 : 0;

        if (intDelta != 0) {
            int newValue = currentValue + intDelta;
            int clampedValue = juce::jlimit(rangeStart, rangeEnd, newValue);

            DBG("│ New value: " << clampedValue);

            // FIXED: Use proper normalization for integer parameters
            float normalizedValue = (clampedValue - rangeStart) / static_cast<float>(rangeEnd - rangeStart);
            intParam->setValueNotifyingHost(normalizedValue);

            // Update preset manager
            if (behavior.parameterID == "preset_index" && gpioPresetManager) {
                gpioPresetManager->setCurrentPresetIndex(clampedValue);
                DBG("│ Preset manager updated to: " << clampedValue);
            }
        }

        DBG("└───────────────────────────────────┘");
        return;
    }

    // Handle AudioParameterFloat (all other parameters)
    auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param);
    if (floatParam) {
        DBG("┌─── FLOAT PARAMETER HANDLING ───┐");
        DBG("│ Parameter: " << behavior.parameterID);

        // Get the actual current value (not normalized)
        float currentActual = floatParam->get();
        float rangeStart = floatParam->getNormalisableRange().start;
        float rangeEnd = floatParam->getNormalisableRange().end;
        float rangeLength = rangeEnd - rangeStart;

        DBG("│ Current actual: " << currentActual);

        // Calculate the change
        float actualDelta = delta * behavior.sensitivity * rangeLength;
        float newActual = currentActual + actualDelta;
        float clampedActual = juce::jlimit(rangeStart, rangeEnd, newActual);

        DBG("│ Delta: " << actualDelta << " -> New: " << clampedActual);

        // CRITICAL FIX: Set value WITHOUT triggering listeners first
        // This prevents feedback loop with A/B bank
        floatParam->beginChangeGesture();

        // Set the actual value directly
        *floatParam = clampedActual;

        // THEN update A/B bank with the new value
        if (abStateEngine) {
            abStateEngine->setParameter(behavior.parameterID, clampedActual);
            DBG("│ Updated " << (abStateEngine->isBankB() ? "Bank B" : "Bank A") << " with: " << clampedActual);
        }

        floatParam->endChangeGesture();

        // Verify the value stuck
        float verifyActual = floatParam->get();
        if (std::abs(verifyActual - clampedActual) >= 0.001f) {
            DBG("│ ⚠️ WARNING: Value was overridden!");
            DBG("│   Expected: " << clampedActual);
            DBG("│   Got: " << verifyActual);
        } else {
            DBG("│ ✓ Value set successfully");
        }

        DBG("└───────────────────────────────────┘");
        return;
    }

    // Fallback for other parameter types
    float currentValue = param->getValue();
    float newValue = juce::jlimit(0.0f, 1.0f, currentValue + delta * behavior.sensitivity);
    param->setValueNotifyingHost(newValue);
    DBG("Generic param " << behavior.parameterID << " changed to " << newValue);
}