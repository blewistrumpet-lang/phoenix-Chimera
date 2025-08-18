# Project Chimera v3.0 "Phoenix" - Architecture Diagram

## System Flow Diagram

```
                           PROJECT CHIMERA ARCHITECTURE
                                  (Authoritative)
    
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                                   USER                                      │
    │                            (Plugin Host DAW)                               │
    └──────────────────────────────┬──────────────────────────────────────────────┘
                                   │
                                   ▼
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                          PLUGIN UI LAYER                                   │
    │                                                                             │
    │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │
    │  │   SLOT 1    │  │   SLOT 2    │  │   SLOT 3    │  │   SLOT 4    │  ...  │
    │  │             │  │             │  │             │  │             │       │
    │  │ Dropdown:   │  │ Dropdown:   │  │ Dropdown:   │  │ Dropdown:   │       │
    │  │ Engine 0-56 │  │ Engine 0-56 │  │ Engine 0-56 │  │ Engine 0-56 │       │
    │  │             │  │             │  │             │  │             │       │
    │  │ Param 1-15  │  │ Param 1-15  │  │ Param 1-15  │  │ Param 1-15  │       │
    │  │ Bypass      │  │ Bypass      │  │ Bypass      │  │ Bypass      │       │
    │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘       │
    └──────────────────────────────┬──────────────────────────────────────────────┘
                                   │
                                   ▼
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                    PLUGIN PROCESSOR LAYER                                  │
    │                        (PluginProcessor.cpp)                               │
    │                                                                             │
    │  ┌─────────────────────────────────────────────────────────────────────┐   │
    │  │                   🏛️ AUTHORITATIVE SYSTEMS                         │   │
    │  │                                                                     │   │
    │  │  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐  │   │
    │  │  │  EngineTypes.h  │    │ EngineFactory   │    │getMixParameter  │  │   │
    │  │  │                 │    │    .cpp         │    │    Index()      │  │   │
    │  │  │ • ENGINE_NONE   │    │                 │    │                 │  │   │
    │  │  │ • ENGINE_TUBE   │───▶│ createEngine()  │───▶│ Param Mapping   │  │   │
    │  │  │ • ENGINE_ECHO   │    │                 │    │                 │  │   │
    │  │  │ • [55 more...]  │    │ Returns         │    │ Mix Index for   │  │   │
    │  │  │                 │    │ EngineBase*     │    │ Each Engine     │  │   │
    │  │  │ Constants 0-56  │    │                 │    │                 │  │   │
    │  │  └─────────────────┘    └─────────────────┘    └─────────────────┘  │   │
    │  └─────────────────────────────────────────────────────────────────────┘   │
    └──────────────────────────────┬──────────────────────────────────────────────┘
                                   │
                                   ▼
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                         DSP ENGINE LAYER                                   │
    │                                                                             │
    │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │
    │  │NoneEngine   │  │VintageTube  │  │TapeEcho     │  │PlateReverb  │       │
    │  │             │  │Preamp       │  │             │  │             │       │
    │  │• process()  │  │             │  │• process()  │  │• process()  │  ...  │
    │  │• reset()    │  │• process()  │  │• reset()    │  │• reset()    │       │
    │  │• getName()  │  │• reset()    │  │• getName()  │  │• getName()  │       │
    │  │             │  │• getName()  │  │             │  │             │       │
    │  │(Passthrough)│  │             │  │(Analog Echo)│  │(Vintage Rev)│       │
    │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘       │
    │                                                                             │
    │                            [+ 53 More Engines]                             │
    └──────────────────────────────┬──────────────────────────────────────────────┘
                                   │
                                   ▼
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                          AUDIO OUTPUT                                      │
    │                     (Processed Audio Signal)                               │
    └─────────────────────────────────────────────────────────────────────────────┘
```

## Data Flow

### 1. User Interaction
```
User selects "Vintage Tube Preamp" from Slot 1 dropdown
    ↓
UI sends choice index 15 to PluginProcessor
    ↓
PluginProcessor calls choiceIndexToEngineID(15) → ENGINE_VINTAGE_TUBE (15)
```

### 2. Engine Loading
```
PluginProcessor::loadEngine(slot=0, engineID=15)
    ↓
EngineFactory::createEngine(15) → std::make_unique<VintageTubePreamp_Studio>()
    ↓
Engine stored in m_activeEngines[0]
```

### 3. Parameter Mapping
```
PluginProcessor::applyDefaultParameters(slot=0, engineID=15)
    ↓
getMixParameterIndex(15) → returns 9 (Mix is at parameter index 9)
    ↓
Set parameter "slot1_param10" to 1.0f (Mix = 100%)
```

### 4. Audio Processing
```
processBlock() called by host
    ↓
For each active slot:
    ↓
updateEngineParameters(slot) → Updates engine with current parameter values
    ↓
m_activeEngines[slot]->process(buffer) → DSP processing
    ↓
Audio flows to next slot or output
```

## System Boundaries

### ✅ AUTHORITATIVE (Core Plugin)
- **EngineTypes.h**: All engine constants and IDs
- **EngineFactory.cpp**: All engine creation logic  
- **PluginProcessor::getMixParameterIndex()**: All parameter mappings
- **Individual Engine Classes**: All DSP implementations

### 🔶 SECONDARY (AI Integration)
- **EngineStringMapping.h**: String ↔ Engine ID conversion for JSON presets
- **DefaultParameterValues.h**: Safe defaults for engine initialization

### ❌ DEPRECATED (Don't Use)
- **GeneratedParameterDatabase.h**: Unverified parameter mappings
- **Any hard-coded engine mappings**: Use EngineTypes.h constants instead

## Engine Categories

```
DYNAMICS (6 engines)        FILTERS & EQ (8 engines)     DISTORTION (8 engines)
├─ Opto Compressor          ├─ Parametric EQ              ├─ Vintage Tube
├─ VCA Compressor           ├─ Vintage Console EQ         ├─ Wave Folder  
├─ Transient Shaper         ├─ Ladder Filter              ├─ Harmonic Exciter
├─ Noise Gate               ├─ State Variable Filter      ├─ Bit Crusher
├─ Mastering Limiter        ├─ Formant Filter             ├─ Multiband Saturator
└─ Dynamic EQ               ├─ Envelope Filter            ├─ Muff Fuzz
                            ├─ Comb Resonator             ├─ Rodent Distortion
MODULATION (11 engines)     └─ Vocal Formant             └─ K-Style Overdrive
├─ Digital Chorus           
├─ Resonant Chorus          REVERB & DELAY (10 engines)  SPATIAL (9 engines)
├─ Analog Phaser            ├─ Tape Echo                  ├─ Stereo Widener
├─ Ring Modulator           ├─ Digital Delay              ├─ Stereo Imager
├─ Frequency Shifter        ├─ Magnetic Drum Echo         ├─ Dimension Expander
├─ Harmonic Tremolo         ├─ Bucket Brigade Delay       ├─ Spectral Freeze
├─ Classic Tremolo          ├─ Buffer Repeat              ├─ Spectral Gate
├─ Rotary Speaker           ├─ Plate Reverb               ├─ Phased Vocoder
├─ Pitch Shifter            ├─ Spring Reverb              ├─ Granular Cloud
├─ Detune Doubler           ├─ Convolution Reverb         ├─ Chaos Generator
└─ Intelligent Harmonizer   ├─ Shimmer Reverb             └─ Feedback Network
                            └─ Gated Reverb               
                                                          UTILITY (4 engines)
                                                          ├─ Mid-Side Processor
                                                          ├─ Gain Utility
                                                          ├─ Mono Maker
                                                          └─ Phase Align
```

**Total: 57 Engines (Including ENGINE_NONE)**

---

**Document Version:** 1.0  
**Last Updated:** August 18, 2025  
**Maintained By:** System Authority Documentation Agent