// COMPREHENSIVE FIX for both issues
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

    DBG("=== ENCODER " << encoderIndex << " TURN ===");
    DBG("  Raw delta: " << delta);
    DBG("  Target: " << behavior.parameterID);

    // Handle AudioParameterInt (preset_index)
    auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param);
    if (intParam) {
        int currentValue = intParam->get();

        // FIX 1: Use larger threshold since delta is already scaled by sensitivity
        // The delta coming in already has sensitivity applied from PluginEditor_Pi
        int step = (delta > 0.05f) ? 1 : (delta < -0.05f) ? -1 : 0;

        if (step != 0) {
            int rangeStart = intParam->getRange().getStart();
            int rangeEnd = intParam->getRange().getEnd();
            int newValue = juce::jlimit(rangeStart, rangeEnd, currentValue + step);

            DBG("  INT: " << currentValue << " -> " << newValue);

            // Calculate proper normalization
            float normalized = static_cast<float>(newValue - rangeStart) /
                             static_cast<float>(rangeEnd - rangeStart);

            intParam->setValueNotifyingHost(normalized);

            // Update preset manager
            if (behavior.parameterID == "preset_index" && gpioPresetManager) {
                gpioPresetManager->setCurrentPresetIndex(newValue);
                DBG("  Preset manager updated to: " << newValue);
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
        float rangeLength = rangeEnd - rangeStart;

        // FIX 2: DON'T multiply by sensitivity again - it's already in delta!
        // Delta from PluginEditor_Pi = event.value * behavior.sensitivity
        float actualDelta = delta * rangeLength;
        float newValue = currentValue + actualDelta;
        newValue = juce::jlimit(rangeStart, rangeEnd, newValue);

        DBG("  FLOAT current: " << currentValue);
        DBG("  Delta scaled: " << actualDelta);
        DBG("  New value: " << newValue);

        // FIX 3: Disable A/B bank updates temporarily to prevent feedback
        bool wasUpdatingFromEncoder = isUpdatingFromEncoder;
        isUpdatingFromEncoder = true;

        // Use change gesture to prevent other listeners
        floatParam->beginChangeGesture();

        // Set the value
        *floatParam = newValue;

        // Only update A/B bank if it won't cause feedback
        if (!wasUpdatingFromEncoder && abStateEngine) {
            abStateEngine->setParameter(behavior.parameterID, newValue);
            DBG("  Saved to " << (abStateEngine->isBankB() ? "Bank B" : "Bank A"));
        }

        floatParam->endChangeGesture();

        isUpdatingFromEncoder = wasUpdatingFromEncoder;

        // Verify the value stuck
        float verifyValue = floatParam->get();
        if (std::abs(verifyValue - newValue) > 0.001f) {
            DBG("  ⚠️ WARNING: Value overridden! Expected " << newValue << ", got " << verifyValue);

            // FIX 4: Force the value again if it was overridden
            floatParam->beginChangeGesture();
            *floatParam = newValue;
            floatParam->endChangeGesture();

            float secondVerify = floatParam->get();
            DBG("  Retry: " << secondVerify << (std::abs(secondVerify - newValue) < 0.001f ? " ✓" : " ✗"));
        } else {
            DBG("  ✓ Value set successfully");
        }

        return;
    }

    // Fallback
    float currentValue = param->getValue();
    float newValue = juce::jlimit(0.0f, 1.0f, currentValue + delta);
    param->setValueNotifyingHost(newValue);
}

// ==================================================================
// ALSO ADD THIS TO PluginProcessor.h (in private section):
// ==================================================================

private:
    // Add this flag to prevent feedback loops
    std::atomic<bool> isUpdatingFromEncoder{false};

// ==================================================================
// OPTIONAL: Debug why mix gets stuck at 0.52
// Add this to processBlock() temporarily to log mix changes:
// ==================================================================

void ChimeraAudioProcessor::processBlock(...) {
    static float lastMixValue = -1.0f;
    float currentMix = parameters.getRawParameterValue("mix_wetdry")->load();
    if (std::abs(currentMix - lastMixValue) > 0.001f) {
        DBG("MIX CHANGED in processBlock: " << lastMixValue << " -> " << currentMix);
        lastMixValue = currentMix;
    }
    // ... rest of processBlock
}