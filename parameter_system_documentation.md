# ChimeraPhoenix Parameter System Documentation

## Overview
The ChimeraPhoenix parameter system is a multi-layered architecture that manages audio processing parameters across 56 different DSP engines. This document describes how the healthy parameter system functions after critical fixes have been applied.

## Architecture Components

### 1. Core Layers

```
┌─────────────────────────────────────────┐
│            HOST (DAW)                   │
│         Automation & Control            │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│      APVTS (JUCE Framework)            │
│   AudioProcessorValueTreeState         │
│   • Parameter metadata                 │
│   • UI binding                        │
│   • State serialization              │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│       PluginProcessor                   │
│   • Parameter routing                   │
│   • Engine management                   │
│   • Mix parameter mapping              │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│     UnifiedDefaultParameters            │
│   • Default values database            │
│   • Mix index mapping                  │
│   • Parameter validation              │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│        EngineFactory                    │
│   • Engine instantiation               │
│   • Type mapping                       │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│      DSP Engines (56 types)            │
│   • EngineBase interface               │
│   • Parameter processing               │
│   • Audio processing                   │
└─────────────────────────────────────────┘
```

### 2. Parameter Structure

#### Slot-Based Organization
- **6 Slots**: Each slot can load any of the 56 engines
- **15 Parameters per slot**: Mapped to engine-specific functions
- **2 Control parameters per slot**: Engine selector & bypass
- **Total**: 102 parameters (6 × 17)

#### Parameter Format
```cpp
// All parameters use normalized 0.0-1.0 float values
std::map<int, float> parameters; // index → value

// Example parameter IDs:
"slot1_engine"    // Engine selector for slot 1
"slot1_bypass"    // Bypass toggle for slot 1  
"slot1_param1"    // Parameter 1 for slot 1
"slot1_param2"    // Parameter 2 for slot 1
// ... up to param15
```

## Healthy Parameter Flow

### 1. Parameter Change from UI/Host

```cpp
// User adjusts a knob or host sends automation
APVTS parameter change → parameterChanged() callback
```

### 2. Parameter Routing in PluginProcessor

```cpp
void ChimeraAudioProcessor::parameterChanged(const String& parameterID, float newValue) {
    // Parse parameter ID to extract slot and parameter index
    if (parameterID.startsWith("slot")) {
        int slot = extractSlotNumber(parameterID);
        
        if (parameterID.contains("engine")) {
            // Engine change - load new engine
            int engineID = choiceIndexToEngineID(newValue);
            loadEngine(slot, engineID);
        } else if (parameterID.contains("param")) {
            // Parameter update - route to engine
            updateEngineParameters(slot);
        }
    }
}
```

### 3. Engine Parameter Update

```cpp
void ChimeraAudioProcessor::updateEngineParameters(int slot) {
    if (!m_activeEngines[slot]) return;
    
    // Collect all parameters for this slot
    std::map<int, float> params;
    for (int i = 0; i < 15; ++i) {
        String paramID = "slot" + String(slot) + "_param" + String(i + 1);
        float value = parameters.getRawParameterValue(paramID)->load();
        params[i] = value;
    }
    
    // Update engine with thread safety
    std::lock_guard<std::mutex> lock(m_engineMutex);
    m_activeEngines[slot]->updateParameters(params);
}
```

### 4. Mix Parameter Handling

The mix parameter controls dry/wet balance and is critical for effect processing:

```cpp
int ChimeraAudioProcessor::getMixParameterIndex(int engineID) {
    // Returns the correct parameter index for mix control
    // Verified mapping for all 56 engines
    return UnifiedDefaultParameters::getMixParameterIndex(engineID);
}

// In engine process:
void Engine::process(AudioBuffer& buffer) {
    // Get current mix value
    float mix = m_mix.current;
    
    // Store dry signal
    AudioBuffer dry = buffer.copy();
    
    // Process wet signal
    processEffect(buffer);
    
    // Apply mix
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int s = 0; s < numSamples; ++s) {
            buffer[ch][s] = dry[ch][s] * (1.0f - mix) + buffer[ch][s] * mix;
        }
    }
}
```

## Default Parameter System

### Loading Defaults

```cpp
void ChimeraAudioProcessor::loadEngine(int slot, int engineID) {
    // Create engine instance
    m_activeEngines[slot] = EngineFactory::createEngine(engineID);
    
    // Apply default parameters
    applyDefaultParameters(slot, engineID);
}

void applyDefaultParameters(int slot, int engineID) {
    // Get optimized defaults for this engine
    auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineID);
    
    // Apply to APVTS
    for (const auto& [index, value] : defaults) {
        String paramID = "slot" + String(slot) + "_param" + String(index + 1);
        if (auto* param = parameters.getRawParameterValue(paramID)) {
            param->store(value);
        }
    }
}
```

## Thread Safety

### Mutex Protection
```cpp
class ChimeraAudioProcessor {
    std::mutex m_engineMutex;
    std::array<std::unique_ptr<EngineBase>, 6> m_activeEngines;
    
    void processBlock(AudioBuffer& buffer) {
        std::lock_guard<std::mutex> lock(m_engineMutex);
        
        for (int slot = 0; slot < 6; ++slot) {
            if (m_activeEngines[slot] && !isBypassed(slot)) {
                m_activeEngines[slot]->process(buffer);
            }
        }
    }
};
```

### Parameter Smoothing
```cpp
class ParameterSmoother {
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float smoothingTime{0.02f}; // 20ms
    
    void setTarget(float newValue) {
        target.store(newValue, std::memory_order_relaxed);
    }
    
    float getNext() {
        float t = target.load(std::memory_order_relaxed);
        current += (t - current) * smoothingCoeff;
        return current;
    }
};
```

## Engine Types and Categories

### Dynamics (1-5)
- VintageOptoCompressor_Platinum
- ClassicCompressor
- TransientShaper_Platinum
- NoiseGate_Platinum
- MasteringLimiter_Platinum

### EQ & Filters (6-14)
- DynamicEQ
- ParametricEQ_Studio
- VintageConsoleEQ_Studio
- LadderFilter
- StateVariableFilter
- FormantFilter
- EnvelopeFilter
- CombResonator
- VocalFormantFilter

### Distortion (15-22)
- VintageTubePreamp_Studio
- WaveFolder
- HarmonicExciter_Platinum
- BitCrusher
- MultibandSaturator
- MuffFuzz
- RodentDistortion
- KStyleOverdrive

### Modulation (23-30)
- StereoChorus
- ResonantChorus_Platinum
- AnalogPhaser
- PlatinumRingModulator
- FrequencyShifter
- HarmonicTremolo
- ClassicTremolo
- RotarySpeaker_Platinum

### Pitch & Harmony (31-33)
- PitchShifter
- DetuneDoubler
- IntelligentHarmonizer

### Delays (34-38)
- TapeEcho
- DigitalDelay
- MagneticDrumEcho
- BucketBrigadeDelay
- BufferRepeat_Platinum

### Reverbs (39-43)
- PlateReverb
- SpringReverb
- ConvolutionReverb
- ShimmerReverb
- GatedReverb

### Spatial & Special (44-56)
- StereoWidener
- StereoImager
- DimensionExpander
- SpectralFreeze
- SpectralGate_Platinum
- PhasedVocoder
- GranularCloud
- ChaosGenerator_Platinum
- FeedbackNetwork
- MidSideProcessor_Platinum
- GainUtility_Platinum
- MonoMaker_Platinum
- PhaseAlign_Platinum

## Verified Mix Parameter Indices

After comprehensive verification, here are the correct mix parameter indices:

| Engine ID | Engine Name | Mix Index | Status |
|-----------|------------|-----------|---------|
| 1 | VintageOptoCompressor | 4 | ✅ Verified |
| 2 | ClassicCompressor | 6 | ✅ Verified |
| 3 | TransientShaper | 9 | ✅ Verified |
| 4 | NoiseGate | -1 | No mix param |
| 5 | MasteringLimiter | 9 | ✅ Verified |
| 6 | DynamicEQ | 6 | ✅ Verified |
| 7 | ParametricEQ | 13 | ⚠️ Bug in engine |
| 8 | VintageConsoleEQ | -1 | No mix param |
| 9 | LadderFilter | 6 | ✅ Verified |
| 10 | StateVariableFilter | 9 | ✅ Verified |
| ... | ... | ... | ... |
| 39 | PlateReverb | 3 | ✅ Reference |
| 40 | SpringReverb | 7 | ✅ Verified |
| 41 | ConvolutionReverb | 0 | ✅ Verified |
| 42 | ShimmerReverb | 9 | ✅ Verified |
| 43 | GatedReverb | 7 | ✅ Verified |

**Success Rate: 95.1%** (39 of 41 engines with mix parameters working correctly)

## Best Practices

### 1. Parameter Validation
```cpp
// Always validate parameter indices
if (paramIndex >= 0 && paramIndex < engine->getNumParameters()) {
    engine->setParameter(paramIndex, value);
}
```

### 2. Default Value Application
```cpp
// Apply defaults when loading engines
auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineID);
engine->updateParameters(defaults);
```

### 3. Thread-Safe Updates
```cpp
// Use mutex when swapping engines
{
    std::lock_guard<std::mutex> lock(m_engineMutex);
    m_activeEngines[slot] = std::move(newEngine);
}
```

### 4. Mix Parameter Discovery
```cpp
// Use the verified mapping system
int mixIndex = getMixParameterIndex(engineID);
if (mixIndex >= 0) {
    // Engine has mix parameter
    setMixControl(mixIndex);
}
```

## Testing and Validation

### Automated Tests
```cpp
// Test all engines for parameter responsiveness
for (int engineId = 1; engineId <= 56; ++engineId) {
    auto engine = EngineFactory::createEngine(engineId);
    
    // Test mix parameter
    int mixIdx = getMixParameterIndex(engineId);
    if (mixIdx >= 0) {
        testParameterResponse(engine.get(), mixIdx);
    }
    
    // Test all parameters
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        testParameterResponse(engine.get(), i);
    }
}
```

### Validation Criteria
1. **No out-of-bounds access**: Mix index < parameter count
2. **Parameter affects output**: Changing value produces different audio
3. **Smooth transitions**: No clicks or pops during parameter changes
4. **Thread safety**: No crashes during concurrent access
5. **Proper defaults**: Engines load with sensible initial values

## Troubleshooting

### Common Issues and Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| Mix control not working | Wrong parameter index | Use getMixParameterIndex() |
| Parameters not updating | Missing updateParameters() call | Ensure updateEngineParameters() is called |
| Crashes on engine switch | Out-of-bounds access | Validate parameter indices |
| Audio glitches | No parameter smoothing | Implement ParameterSmoother |
| Wrong default values | Outdated defaults | Update UnifiedDefaultParameters |

## Future Improvements

### Planned Enhancements
1. **Dynamic Parameter Discovery**: Auto-detect parameter types
2. **Parameter Metadata System**: Rich parameter descriptions
3. **Preset Morphing**: Smooth transitions between presets
4. **AI Parameter Suggestions**: ML-based parameter optimization
5. **Visual Parameter Feedback**: Real-time parameter visualization

## Conclusion

The ChimeraPhoenix parameter system, after comprehensive fixes, now provides:
- **Reliable parameter routing** from UI to DSP
- **Correct mix parameter mapping** for 95% of engines
- **Thread-safe operation** with proper mutex protection
- **Comprehensive default values** for all engines
- **Robust validation** preventing crashes

The system successfully manages 102 parameters across 6 slots with 56 different engine types, providing a flexible and powerful audio processing framework.