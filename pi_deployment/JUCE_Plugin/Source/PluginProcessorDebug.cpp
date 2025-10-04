// Debug version of PluginProcessor to trace parameter issues
// Add this code to PluginProcessor.cpp temporarily to debug

void ChimeraAudioProcessor::updateEngineParameters(int slot) {
    if (!m_activeEngines[slot]) {
        DBG("WARNING: No engine in slot " + juce::String(slot));
        return;
    }
    
    std::map<int, float> params;
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
    
    DBG("=== PARAMETER UPDATE DEBUG for Slot " + juce::String(slot) + " ===");
    
    for (int i = 0; i < 15; ++i) {
        auto paramID = slotPrefix + juce::String(i + 1);
        float value = parameters.getRawParameterValue(paramID)->load();
        params[i] = value;
        
        // Debug: Print non-default values
        if (std::abs(value - 0.5f) > 0.01f) {
            DBG("  Param " + juce::String(i) + " (" + paramID + "): " + juce::String(value));
        }
    }
    
    // Get engine type for context
    auto* engineParam = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_engine");
    int engineChoice = static_cast<int>(engineParam->load());
    int engineID = choiceIndexToEngineID(engineChoice);
    
    DBG("  Engine: " + getEngineTypeName(engineID) + " (ID: " + juce::String(engineID) + ")");
    DBG("  Sending " + juce::String(params.size()) + " parameters to engine");
    
    m_activeEngines[slot]->updateParameters(params);
}

// Also add to process() for debugging:
void ChimeraAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    
    // DEBUG: Check what's happening
    static int processCount = 0;
    if (++processCount % 100 == 0) { // Every 100 blocks
        DBG("=== PROCESS BLOCK DEBUG ===");
        for (int slot = 0; slot < NUM_SLOTS; ++slot) {
            if (m_activeEngines[slot]) {
                auto* engineParam = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_engine");
                int engineChoice = static_cast<int>(engineParam->load());
                if (engineChoice != 0) { // Not None
                    DBG("Slot " + juce::String(slot) + " active with engine choice " + juce::String(engineChoice));
                    
                    // Check a key parameter
                    auto* param1 = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_param1");
                    DBG("  Param1 value: " + juce::String(param1->load()));
                }
            }
        }
    }
    
    // ... rest of processBlock
}