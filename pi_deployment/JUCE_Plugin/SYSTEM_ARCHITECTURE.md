# Chimera Phoenix v3.0 - System Architecture

**Version:** v3.0 Phoenix  
**Date:** August 19, 2025  
**Status:** Production Ready  

## Executive Summary

Chimera Phoenix is a professional audio plugin featuring a **6-slot serial processing chain** powered by **57 high-quality DSP engines**. The system provides a modular, expandable architecture where any engine can be loaded into any slot, creating unlimited creative possibilities for audio processing.

## System Overview

```
Audio Input → [Slot 1] → [Slot 2] → [Slot 3] → [Slot 4] → [Slot 5] → [Slot 6] → Audio Output
                ↓          ↓          ↓          ↓          ↓          ↓
           [Engine 0-56] [Engine 0-56] [Engine 0-56] [Engine 0-56] [Engine 0-56] [Engine 0-56]
```

## Core Architecture Components

### 1. ChimeraAudioProcessor - The System Brain
**File:** `Source/PluginProcessor.h` / `Source/PluginProcessor.cpp`

The ChimeraAudioProcessor is the central coordinator that:
- Manages the 6-slot processing chain
- Handles parameter automation and MIDI
- Coordinates engine lifecycle (creation, destruction, parameter updates)
- Provides diagnostic and validation capabilities
- Manages audio threading and real-time safety

**Key Responsibilities:**
```cpp
class ChimeraAudioProcessor : public juce::AudioProcessor {
    // 6-slot engine management
    std::array<std::unique_ptr<EngineBase>, NUM_SLOTS> m_activeEngines;
    
    // Parameter coordination
    juce::AudioProcessorValueTreeState parameters;
    
    // Engine lifecycle
    void loadEngine(int slot, int engineID);
    void updateEngineParameters(int slot);
};
```

### 2. Slot Configuration System
**File:** `Source/SlotConfiguration.h`

Centralized slot management that defines:
- Current slot count: **6 slots**
- Maximum supported: **8 slots** (future expansion)
- Slot validation and naming
- CPU monitoring thresholds

```cpp
namespace ChimeraConfig {
    static constexpr int NUM_SLOTS = 6;
    static constexpr int MAX_SLOTS_SUPPORTED = 8;
}
```

### 3. Engine System (57 DSP Engines)
**Files:** `Source/EngineTypes.h`, `Source/EngineBase.h`, `Source/EngineFactory.cpp`

#### Engine Categories:
1. **Dynamics & Compression (IDs 1-6)**
   - Vintage Opto Compressor, VCA Compressor, Transient Shaper
   - Noise Gate, Mastering Limiter, Dynamic EQ

2. **Filters & EQ (IDs 7-14)**
   - Parametric EQ, Vintage Console EQ, Ladder Filter
   - State Variable Filter, Formant Filter, Envelope Filter
   - Comb Resonator, Vocal Formant Filter

3. **Distortion & Saturation (IDs 15-22)**
   - Vintage Tube Preamp, Wave Folder, Harmonic Exciter
   - Bit Crusher, Multiband Saturator, Muff Fuzz
   - Rodent Distortion, K-Style Overdrive

4. **Modulation Effects (IDs 23-33)**
   - Digital/Stereo Chorus, Resonant Chorus, Analog Phaser
   - Ring Modulator, Frequency Shifter, Harmonic Tremolo
   - Classic Tremolo, Rotary Speaker, Pitch Shifter
   - Detune Doubler, Intelligent Harmonizer

5. **Reverb & Delay (IDs 34-43)**
   - Tape Echo, Digital Delay, Magnetic Drum Echo
   - Bucket Brigade Delay, Buffer Repeat, Plate Reverb
   - Spring Reverb, Convolution Reverb, Shimmer Reverb, Gated Reverb

6. **Spatial & Special Effects (IDs 44-52)**
   - Stereo Widener, Stereo Imager, Dimension Expander
   - Spectral Freeze, Spectral Gate, Phased Vocoder
   - Granular Cloud, Chaos Generator, Feedback Network

7. **Utility (IDs 53-56)**
   - Mid-Side Processor, Gain Utility, Mono Maker, Phase Align

### 4. Parameter System
**Files:** `Source/ParameterDefinitions.h`, `Source/UnifiedDefaultParameters.h`

#### Parameter Architecture:
- **15 parameters per slot** × 6 slots = **90 engine parameters**
- **6 engine selection parameters** (one per slot)
- **Total: 96 parameters**

#### Parameter Flow:
```
DAW Parameter Change → ChimeraAudioProcessor::parameterChanged() 
                    → updateEngineParameters() 
                    → EngineBase::updateParameters()
```

#### UnifiedDefaultParameters System:
The system includes a comprehensive default parameter management system:
- **100% engine coverage** - all 57 engines have optimized defaults
- **Musical optimization** - defaults provide immediate satisfaction
- **Category consistency** - similar engines share similar default patterns
- **Safety validation** - all parameters within safe operational ranges

### 5. Engine Factory System
**File:** `Source/EngineFactory.cpp`

The EngineFactory provides centralized engine creation:
```cpp
std::unique_ptr<EngineBase> EngineFactory::createEngine(int engineID) {
    switch (engineID) {
        case ENGINE_NONE: return std::make_unique<NoneEngine>();
        case ENGINE_OPTO_COMPRESSOR: return std::make_unique<VintageOptoCompressor_Platinum>();
        // ... 55 more engines
    }
}
```

## Data Flow Architecture

### 1. Audio Processing Chain
```
Input Buffer → Slot 1 Engine → Slot 2 Engine → ... → Slot 6 Engine → Output Buffer
```

Each slot processes audio serially, with the output of one slot feeding the input of the next.

### 2. Parameter Updates
```
Host/UI Parameter → ChimeraAudioProcessor → Parameter Mapping → Engine::updateParameters()
```

### 3. Engine Lifecycle
```
User Selects Engine → loadEngine() → EngineFactory::createEngine() → 
applyDefaultParameters() → Engine Ready for Processing
```

## File Hierarchy and Responsibilities

### Core System Files
- `PluginProcessor.h/cpp` - Main plugin processor and system coordinator
- `SlotConfiguration.h` - Slot architecture configuration
- `EngineTypes.h` - Complete engine type definitions and metadata
- `EngineBase.h` - Abstract base class for all engines
- `EngineFactory.h/cpp` - Engine instantiation and management
- `ParameterDefinitions.h` - Parameter ID definitions and mapping

### Engine Implementation Files
- `[EngineName].h/cpp` - Individual engine implementations (57 files)
- Each engine inherits from `EngineBase` and implements the standard interface

### Parameter System Files
- `UnifiedDefaultParameters.h/cpp` - Default parameter management
- Provides musically optimized defaults for all 57 engines

### UI Files
- `PluginEditor.h/cpp` - Main UI coordinator
- Communicates with ChimeraAudioProcessor for parameter updates

## Engine Interface Standard

All engines implement the `EngineBase` interface:

```cpp
class EngineBase {
public:
    // Core Processing API
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;
    virtual void updateParameters(const std::map<int, float>& params) = 0;
    
    // Metadata API
    virtual juce::String getName() const = 0;
    virtual int getNumParameters() const = 0;
    virtual juce::String getParameterName(int index) const = 0;
    
    // Extended Features (optional)
    virtual int getLatencySamples() const noexcept { return 0; }
    virtual void setBypassed(bool shouldBypass) { }
    virtual bool supportsFeature(Feature f) const noexcept { return false; }
};
```

## System Capabilities

### Current Features
- **6-slot serial processing chain**
- **57 professional DSP engines**
- **Real-time parameter automation**
- **Comprehensive default parameter system**
- **Engine hot-swapping** (change engines without audio dropouts)
- **Individual slot bypass**
- **CPU usage monitoring**
- **Diagnostic and validation tools**

### Future Expansion Capabilities
- **8-slot expansion** (architecture already supports it)
- **Parallel processing chains**
- **Dynamic slot count adjustment**
- **Sidechain processing**
- **Double-precision processing**
- **Oversampling support**

## Performance Characteristics

### CPU Usage
- **Typical Usage:** 30-50% CPU with moderate engines
- **6 Slots:** Recommended for most use cases
- **8 Slots:** For advanced users with powerful systems
- **CPU Monitoring:** Built-in thresholds at 70% (warning) and 85% (critical)

### Memory Usage
- **Engine Footprint:** Minimal per-engine overhead
- **Parameter Storage:** 96 parameters × 4 bytes = 384 bytes
- **Audio Buffers:** Shared buffer system for efficiency

### Latency
- **Base Latency:** 0 samples (most engines)
- **Variable Latency:** Some engines report actual latency for host compensation
- **PDC Support:** Plugin Delay Compensation for lookahead processors

## Threading and Real-Time Safety

### Audio Thread Safety
- All engines designed for real-time audio processing
- Lock-free parameter updates where possible
- Thread-safe engine swapping with atomic operations

### UI Thread Separation
- Parameter updates coordinated through JUCE's ValueTreeState
- UI rendering completely separate from audio processing
- Real-time audio never blocked by UI operations

## Quality Assurance

### Testing Coverage
- **98.2% operational rate** (56/57 engines fully functional)
- Comprehensive validation suite
- Golden corpus of test presets
- Automated quality metrics

### Validation Systems
- Engine isolation testing
- Parameter boundary validation
- Audio quality metrics
- CPU performance profiling

## Integration Points

### Host Integration
- **VST3, AU, AAX** plugin format support
- **JUCE Framework** for cross-platform compatibility
- **Standard plugin parameters** for DAW automation

### AI Integration
- Optional AI server for intelligent preset generation
- Machine learning-based parameter optimization
- Preset similarity analysis and recommendation

## Conclusion

The Chimera Phoenix architecture provides a robust, scalable foundation for professional audio processing. The modular design allows for easy expansion while maintaining real-time performance and professional audio quality. The comprehensive default parameter system ensures immediate musical satisfaction, while the flexible slot architecture supports creative experimentation and professional production workflows.