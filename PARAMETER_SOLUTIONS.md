# 4 Solutions for Parameter Names and Control Types

## Problem Analysis
The UI is not properly displaying parameter names because:
1. The hardcoded switch statement in PluginEditorFull.cpp only covers ~20 engines
2. The remaining 36+ engines fall through to generic "Param 1", "Param 2", etc.
3. Control types are all sliders, not appropriate for boolean/discrete parameters
4. No snapping for pitch/harmony parameters

## Solution 1: Use EngineLibrary (RECOMMENDED - Minimal Changes)
**Simplest fix - Use the existing centralized system**

```cpp
// In PluginEditorFull.cpp, replace the entire switch statement with:
engineSelectors[i].onChange = [this, i]() {
    int selectedId = engineSelectors[i].getSelectedId();
    if (selectedId > 0) {
        int engineID = ChimeraAudioProcessor::choiceIndexToEngineID(selectedId - 1);
        
        // Get parameter names from EngineLibrary
        for (int j = 0; j < PARAMS_PER_SLOT; ++j) {
            std::string paramName = EngineLibrary::getParameterName(engineID, j);
            slotParamLabels[i][j].setText(paramName, juce::dontSendNotification);
            
            // Show/hide based on actual parameter count
            int paramCount = EngineLibrary::getParameterCount(engineID);
            bool isVisible = (j < paramCount);
            slotParamLabels[i][j].setVisible(isVisible);
            slotParamSliders[i][j].setVisible(isVisible);
        }
    }
};
```

**Pros:**
- Uses existing database with 44+ engines already defined
- Minimal code changes
- Automatically gets updates when database is updated

**Cons:**
- Doesn't address control type issues
- No snapping values

## Solution 2: Enhanced Parameter System with Control Types
**Add control type metadata to each parameter**

Create a new file `ParameterControlTypes.h`:

```cpp
enum class ControlType {
    ROTARY,           // Standard 0-1 rotary knob
    TOGGLE,           // On/Off button
    STEPPED_ROTARY,   // Rotary with discrete steps
    DROPDOWN,         // Dropdown menu for selections
    PITCH_SNAP,       // Snaps to semitones (-24 to +24)
    HARMONY_SELECT    // Dropdown for harmony types
};

struct ParameterControl {
    std::string name;
    ControlType type;
    std::vector<float> snapValues;  // For stepped controls
    std::vector<std::string> options;  // For dropdowns
};

// Define control types for each engine
static std::map<int, std::vector<ParameterControl>> engineControls = {
    {ENGINE_INTELLIGENT_HARMONIZER, {
        {"Voices", ControlType::STEPPED_ROTARY, {1, 2, 3, 4}},
        {"Chord Type", ControlType::DROPDOWN, {}, {"Major", "Minor", "Dim", "Aug", "Sus2", "Sus4"}},
        {"Root Key", ControlType::DROPDOWN, {}, {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}},
        {"Scale", ControlType::DROPDOWN, {}, {"Major", "Minor", "Dorian", "Phrygian", "Lydian", "Mixolydian"}},
        {"Master Mix", ControlType::ROTARY},
        // ... etc
    }},
    {ENGINE_PITCH_SHIFTER, {
        {"Pitch", ControlType::PITCH_SNAP, {-24, -12, -7, -5, 0, 5, 7, 12, 24}},
        {"Fine", ControlType::ROTARY},
        {"Formant", ControlType::TOGGLE},
        // ... etc
    }}
};
```

Then update UI to create appropriate controls based on type.

**Pros:**
- Proper control for each parameter type
- Better UX with appropriate widgets
- Snapping for musical values

**Cons:**
- Requires significant UI refactoring
- Need to define control types for all 56 engines

## Solution 3: Dynamic Parameter Query from Engines
**Query the actual engine instances for parameter info**

```cpp
// In PluginEditorFull.cpp
void updateParameterNames(int slot) {
    auto& engine = audioProcessor.getEngine(slot);
    if (engine) {
        for (int j = 0; j < PARAMS_PER_SLOT; ++j) {
            // Get name directly from engine
            juce::String paramName = engine->getParameterName(j);
            slotParamLabels[slot][j].setText(paramName, juce::dontSendNotification);
            
            // Get parameter attributes
            auto attributes = engine->getParameterAttributes(j);
            
            // Configure control based on attributes
            if (attributes.isBoolean) {
                // Convert to toggle button
            } else if (attributes.hasSnapValues) {
                // Configure snapping
                slotParamSliders[slot][j].setRange(attributes.min, attributes.max, attributes.step);
            }
        }
    }
}
```

**Pros:**
- Always accurate - gets info from actual engine
- Engines control their own parameters
- Can adapt dynamically

**Cons:**
- Requires modifying EngineBase interface
- All 56 engines need updating

## Solution 4: Hybrid with Parameter Manifest
**Use existing GeneratedParameterDatabase + control type extensions**

```cpp
// Extend GeneratedParameterDatabase.h entries:
static constexpr ParameterInfo VintageOpto_params[] = {
    {"Threshold", -30.0f, -60.0f, 0.0f, "Compression threshold", "dB", 1.0f, ControlType::ROTARY},
    {"Ratio", 4.0f, 1.0f, 20.0f, "Compression ratio", ":1", 0.5f, ControlType::STEPPED, {1,2,4,8,10,20}},
    {"Attack", 10.0f, 0.1f, 100.0f, "Attack time", "ms", 0.3f, ControlType::ROTARY},
    {"Release", 100.0f, 10.0f, 1000.0f, "Release time", "ms", 0.3f, ControlType::ROTARY},
    {"Makeup", 0.0f, -12.0f, 24.0f, "Makeup gain", "dB", 1.0f, ControlType::ROTARY},
    {"Mix", 100.0f, 0.0f, 100.0f, "Dry/wet mix", "%", 1.0f, ControlType::ROTARY},
    {"Auto", 0.0f, 0.0f, 1.0f, "Auto makeup gain", "", 1.0f, ControlType::TOGGLE},
    {"Knee", 2.0f, 0.0f, 10.0f, "Knee softness", "dB", 1.0f, ControlType::ROTARY}
};
```

Then in UI:
```cpp
// Use EngineLibrary but also get control type
auto paramInfo = EngineLibrary::getParameterInfo(engineID, paramIndex);
if (paramInfo.controlType == ControlType::TOGGLE) {
    // Show toggle button instead of slider
} else if (paramInfo.controlType == ControlType::PITCH_SNAP) {
    // Configure slider with snapping
}
```

**Pros:**
- Leverages existing database
- Gradual migration possible
- Centralized configuration

**Cons:**
- Need to update database structure
- Still need UI changes for different control types

## Recommended Implementation Order

1. **Immediate Fix (5 minutes):** Implement Solution 1 to use EngineLibrary
2. **Phase 2 (1 hour):** Add snapping for pitch parameters using Solution 4
3. **Phase 3 (2-3 hours):** Implement toggle buttons for boolean parameters
4. **Phase 4 (Future):** Full Solution 2 implementation with all control types

## Missing Engines in Current Switch Statement

Engines currently falling back to generic names:
- ENGINE_OPTO_COMPRESSOR (1)
- ENGINE_VCA_COMPRESSOR (2)
- ENGINE_TRANSIENT_SHAPER (3)
- ENGINE_NOISE_GATE (4)
- ENGINE_MASTERING_LIMITER (5)
- ENGINE_DYNAMIC_EQ (6)
- ENGINE_PARAMETRIC_EQ (7)
- ENGINE_VINTAGE_CONSOLE_EQ (8)
- ENGINE_LADDER_FILTER (9)
- ENGINE_COMB_RESONATOR (13)
- ENGINE_VOCAL_FORMANT (14)
- ENGINE_VINTAGE_TUBE (15)
- ENGINE_WAVE_FOLDER (16)
- ENGINE_HARMONIC_EXCITER (17)
- ENGINE_MULTIBAND_SATURATOR (19)
- ENGINE_MUFF_FUZZ (20)
- ENGINE_RODENT_DISTORTION (21)
- ENGINE_RESONANT_CHORUS (24)
- ENGINE_FREQUENCY_SHIFTER (27)
- ENGINE_HARMONIC_TREMOLO (28)
- ENGINE_CLASSIC_TREMOLO (29)
- ENGINE_ROTARY_SPEAKER (30)
- ENGINE_DETUNE_DOUBLER (32)
- ENGINE_INTELLIGENT_HARMONIZER (33) - Has names but not in switch
- ENGINE_MAGNETIC_DRUM_ECHO (36)
- ENGINE_BUCKET_BRIGADE_DELAY (37)
- ENGINE_BUFFER_REPEAT (38)
- ENGINE_CONVOLUTION_REVERB (41)
- ENGINE_STEREO_IMAGER (45)
- ENGINE_DIMENSION_EXPANDER (46)
- ENGINE_SPECTRAL_GATE (48)
- ENGINE_PHASED_VOCODER (49)
- ENGINE_CHAOS_GENERATOR (51)
- ENGINE_FEEDBACK_NETWORK (52)
- ENGINE_MID_SIDE_PROCESSOR (53)
- ENGINE_GAIN_UTILITY (54)
- ENGINE_MONO_MAKER (55)
- ENGINE_PHASE_ALIGN (56)

Total: 36+ engines missing parameter names in UI!