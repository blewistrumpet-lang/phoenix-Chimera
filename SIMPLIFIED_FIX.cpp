// SIMPLIFIED FIX - Removes double sensitivity multiplication
// Replace updateParameterFromEncoder in PluginProcessor.cpp (lines 1785-1949)

void ChimeraAudioProcessor::updateParameterFromEncoder(int encoderIndex, float delta) {
    if (!controlState) return;

    auto behavior = controlState->getEncoderBehavior(encoderIndex);

    // Get the parameter
    auto* param = parameters.getParameter(behavior.parameterID);
    if (!param) {
        DBG("WARNING: Parameter not found: " << behavior.parameterID);
        return;
    }

    DBG("=== ENCODER " << encoderIndex << " ===" <<
        " Delta: " << delta <<
        " Target: " << behavior.parameterID);

    // Handle AudioParameterInt (preset_index) specially
    auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param);
    if (intParam) {
        int currentValue = intParam->get();

        // IMPORTANT: delta already has sensitivity applied from PluginEditor_Pi
        // Just use it to determine direction
        int step = (delta > 0.01f) ? 1 : (delta < -0.01f) ? -1 : 0;

        if (step != 0) {
            int newValue = currentValue + step;
            int rangeStart = intParam->getRange().getStart();
            int rangeEnd = intParam->getRange().getEnd();
            newValue = juce::jlimit(rangeStart, rangeEnd, newValue);

            DBG("  INT: " << currentValue << " -> " << newValue);

            // Set with correct normalization
            float normalized = static_cast<float>(newValue - rangeStart) /
                             static_cast<float>(rangeEnd - rangeStart);
            intParam->setValueNotifyingHost(normalized);

            // Update preset manager
            if (behavior.parameterID == "preset_index" && gpioPresetManager) {
                gpioPresetManager->setCurrentPresetIndex(newValue);
            }
        }
        return;
    }

    // Handle AudioParameterFloat
    auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param);
    if (floatParam) {
        float currentValue = floatParam->get();
        float rangeStart = floatParam->getNormalisableRange().start;
        float rangeEnd = floatParam->getNormalisableRange().end;

        // CRITICAL FIX: Don't multiply by sensitivity again!
        // delta already has sensitivity from PluginEditor_Pi
        float actualDelta = delta * (rangeEnd - rangeStart);
        float newValue = currentValue + actualDelta;
        newValue = juce::jlimit(rangeStart, rangeEnd, newValue);

        DBG("  FLOAT: " << currentValue << " + " << actualDelta << " = " << newValue);

        // Use gesture to prevent feedback
        floatParam->beginChangeGesture();
        *floatParam = newValue;

        // Update A/B bank
        if (abStateEngine) {
            abStateEngine->setParameter(behavior.parameterID, newValue);
        }

        floatParam->endChangeGesture();

        // Verify
        float verify = floatParam->get();
        if (std::abs(verify - newValue) > 0.001f) {
            DBG("  WARNING: Override detected! Set " << newValue << " got " << verify);
        }

        return;
    }

    // Fallback for other types (shouldn't reach here)
    float currentValue = param->getValue();
    float newValue = juce::jlimit(0.0f, 1.0f, currentValue + delta);
    param->setValueNotifyingHost(newValue);
}