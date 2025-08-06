// Audio Quality Fix for ChimeraPhoenix
// Addresses static noise and poor sound quality issues

/* CRITICAL AUDIO FIXES NEEDED:

1. In PluginProcessor.cpp processBlock():
   - Add safety checks for NaN/Inf values
   - Ensure proper gain staging
   - Add soft clipping to prevent harsh distortion

2. Parameter initialization issues:
   - Many engines may have uninitialized or extreme parameter values
   - Need to ensure all parameters start at safe defaults

3. Engine processing chain:
   - Verify engines are created properly
   - Check for null engines
   - Ensure proper mix levels

*/

// In PluginProcessor::processBlock, add safety processing:
/*
void ChimeraAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, 
                                        juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    // Safety: Clear any NaN or infinite values
    for (int channel = 0; channel < numChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample) {
            if (std::isnan(channelData[sample]) || std::isinf(channelData[sample])) {
                channelData[sample] = 0.0f;
            }
        }
    }
    
    // Store dry signal for mix
    juce::AudioBuffer<float> dryBuffer(numChannels, numSamples);
    for (int ch = 0; ch < numChannels; ++ch) {
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }
    
    // Process through each slot
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        auto* bypassParam = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_bypass");
        bool bypassed = bypassParam && *bypassParam > 0.5f;
        
        if (!bypassed && engines[slot]) {
            // Get mix parameter for this slot
            auto* mixParam = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_mix");
            float slotMix = mixParam ? *mixParam : 0.5f;
            
            // Process with safety
            juce::AudioBuffer<float> slotBuffer(numChannels, numSamples);
            
            // Copy input to slot buffer
            for (int ch = 0; ch < numChannels; ++ch) {
                slotBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
            }
            
            // Process engine
            try {
                engines[slot]->process(slotBuffer);
            } catch (...) {
                // If engine fails, bypass it
                continue;
            }
            
            // Safety check output
            for (int ch = 0; ch < numChannels; ++ch) {
                float* slotData = slotBuffer.getWritePointer(ch);
                for (int sample = 0; sample < numSamples; ++sample) {
                    // Remove NaN/Inf
                    if (std::isnan(slotData[sample]) || std::isinf(slotData[sample])) {
                        slotData[sample] = 0.0f;
                    }
                    
                    // Soft clip to prevent harsh distortion
                    slotData[sample] = std::tanh(slotData[sample] * 0.7f) / 0.7f;
                }
            }
            
            // Mix processed signal with input
            for (int ch = 0; ch < numChannels; ++ch) {
                buffer.applyGain(ch, 0, numSamples, 1.0f - slotMix);
                buffer.addFrom(ch, 0, slotBuffer, ch, 0, numSamples, slotMix);
            }
        }
    }
    
    // Apply master gain (if implemented)
    auto* masterGainParam = parameters.getRawParameterValue("master_gain");
    if (masterGainParam) {
        float gainDb = *masterGainParam;
        float gain = juce::Decibels::decibelsToGain(gainDb);
        buffer.applyGain(gain);
    }
    
    // Final safety limiting
    for (int channel = 0; channel < numChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        float maxLevel = 0.0f;
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Hard limit at ±2.0 to prevent speaker damage
            channelData[sample] = juce::jlimit(-2.0f, 2.0f, channelData[sample]);
            
            // Track level for metering
            maxLevel = std::max(maxLevel, std::abs(channelData[sample]));
        }
        
        // Update output level for metering
        if (maxLevel > currentOutputLevel) {
            currentOutputLevel = maxLevel;
        }
    }
    
    // Decay output level for meter
    currentOutputLevel *= 0.95f;
}
*/

// Add parameter initialization in PluginProcessor constructor:
/*
void ChimeraAudioProcessor::initializeParameters() {
    // Set all parameters to safe defaults
    for (int slot = 1; slot <= NUM_SLOTS; ++slot) {
        juce::String slotStr = juce::String(slot);
        
        // Start with bypass engaged
        if (auto* param = parameters.getParameter("slot" + slotStr + "_bypass")) {
            param->setValueNotifyingHost(1.0f); // Bypassed by default
        }
        
        // Set engine to first valid engine (K-Style at choice index 0)
        if (auto* param = parameters.getParameter("slot" + slotStr + "_engine")) {
            param->setValueNotifyingHost(0.0f); // First engine choice index
        }
        
        // Set mix to 50%
        if (auto* param = parameters.getParameter("slot" + slotStr + "_mix")) {
            param->setValueNotifyingHost(0.5f);
        }
        
        // Set all parameters to center/safe values
        for (int p = 1; p <= 10; ++p) {
            if (auto* param = parameters.getParameter("slot" + slotStr + "_param" + juce::String(p))) {
                param->setValueNotifyingHost(0.5f); // Center position
            }
        }
    }
    
    // Master gain at 0dB
    if (auto* param = parameters.getParameter("master_gain")) {
        param->setValueNotifyingHost(0.5f); // 0dB
    }
}
*/

// Fix engine creation to ensure safe defaults:
/*
void ChimeraAudioProcessor::createEngine(int slot, int engineId) {
    if (slot < 0 || slot >= NUM_SLOTS) return;
    
    // Clear existing engine
    engines[slot].reset();
    
    // Create new engine
    engines[slot] = EngineFactory::createEngine(engineId);
    
    if (engines[slot]) {
        // Prepare with current settings
        engines[slot]->prepareToPlay(getSampleRate(), getBlockSize());
        
        // Initialize with safe parameter values
        std::map<int, float> safeParams;
        for (int i = 0; i < 10; ++i) {
            // Start all parameters at 0.5 (center)
            safeParams[i] = 0.5f;
        }
        
        // Special cases for certain parameters that should start differently
        switch (engineId) {
                
            case ENGINE_CLASSIC_COMPRESSOR:
                safeParams[0] = 0.7f; // Threshold at -10dB
                safeParams[1] = 0.3f; // Ratio at 3:1
                safeParams[2] = 0.2f; // Fast attack
                safeParams[3] = 0.4f; // Medium release
                safeParams[5] = 0.5f; // Makeup gain at 0dB
                break;
                
            case ENGINE_PLATE_REVERB:
                safeParams[0] = 0.5f; // Room size
                safeParams[1] = 0.6f; // Damping
                safeParams[6] = 0.3f; // Mix at 30%
                break;
                
            // Add more engine-specific safe defaults...
            
            default:
                // Use center values
                break;
        }
        
        engines[slot]->updateParameters(safeParams);
    }
}
*/

// Add this to check for problematic parameter values:
/*
bool ChimeraAudioProcessor::validatePresetParameters(const juce::var& preset) {
    if (!preset.hasProperty("parameters")) return false;
    
    auto params = preset["parameters"];
    if (auto* dynObj = params.getDynamicObject()) {
        for (auto& prop : dynObj->getProperties()) {
            auto value = prop.value;
            
            // Check for invalid values
            if (value.isDouble()) {
                double v = value;
                if (std::isnan(v) || std::isinf(v) || v < 0.0 || v > 1.0) {
                    DBG("Invalid parameter value: " << prop.name.toString() << " = " << v);
                    return false;
                }
            }
        }
    }
    
    return true;
}
*/

// Update loadPresetFromJSON to validate and sanitize:
/*
void ChimeraAudioProcessor::loadPresetFromJSON(const juce::var& preset) {
    if (!validatePresetParameters(preset)) {
        DBG("Preset validation failed - using safe defaults");
        initializeParameters();
        return;
    }
    
    // Continue with normal loading...
}
*/