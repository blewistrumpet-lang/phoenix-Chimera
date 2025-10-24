# Trinity Control Stack v0.5 - Gap Analysis & Implementation Strategy

**Project:** Chimera Phoenix v3.0
**Document Version:** 1.0
**Date:** 2025-10-23
**Status:** PLANNING - Implementation Roadmap

---

## Executive Summary

This document provides a comprehensive gap analysis between the current Chimera Phoenix codebase and the proposed Trinity Control Stack (TCS) v0.5 specification. The TCS v0.5 represents a complete reimagining of the hardware control layer, introducing sophisticated state machines, A/B morphing, parameter macros, and multi-layer interaction paradigms.

### Current State Overview
- **Working:** Hardware GPIO layer (3 encoders, 3 switches)
- **Working:** 57 DSP engines with parameter control
- **Working:** Trinity AI preset generation and modification
- **Working:** Basic preset save/load via PresetManager
- **Missing:** All advanced control features from TCS v0.5 spec

### Development Scope
- **Estimated Total Effort:** 12-16 weeks (3-4 months)
- **New Code:** ~8,000-10,000 lines
- **Refactored Code:** ~3,000-4,000 lines
- **Sprint Count:** 9 sprints (as outlined in spec)
- **Risk Level:** MEDIUM-HIGH (complex state management, real-time constraints)

---

## Table of Contents

1. [Current State Analysis](#current-state-analysis)
2. [Gap Analysis by Component](#gap-analysis-by-component)
3. [Architectural Changes Required](#architectural-changes-required)
4. [Critical Dependencies](#critical-dependencies)
5. [Technical Challenges](#technical-challenges)
6. [Development Roadmap](#development-roadmap)
7. [Sprint Breakdown](#sprint-breakdown)
8. [Risk Assessment](#risk-assessment)
9. [Testing Strategy](#testing-strategy)
10. [Success Criteria](#success-criteria)

---

## 1. Current State Analysis

### 1.1 What Currently Exists

#### HardwareController (GPIO Layer)
**File:** `/pi_deployment/JUCE_Plugin/Source/HardwareController.cpp/h`
**Status:** WORKING
**Capabilities:**
- 3 rotary encoders with push buttons (GPIO polling at 1ms)
- 3 three-way switches (UP/MIDDLE/DOWN detection)
- Callback system for encoder/button/switch events
- Thread-safe atomic state storage
- Clean initialization/shutdown
- Active-low button detection with proper debouncing

**What Works:**
```cpp
// Encoder reading with direction detection
encoders[0].position.load();  // Atomic position counter
encoders[0].isButtonPressed(); // Button state

// Switch position reading
switches[0].getPosition();  // Returns UP/MIDDLE/DOWN

// Callbacks for real-time events
setEncoderCallback([](int num, int pos, bool cw) { ... });
setSwitchCallback([](int num, SwitchPosition pos) { ... });
```

**Limitations:**
- No concept of "modes" or layers
- No parameter mapping logic
- No timeout or auto-return behavior
- Direct hardware access only (no abstraction for parameter control)

#### DSP Engine System
**Files:** `EngineBase.h`, `EngineFactory.cpp`, various engine implementations
**Status:** PRODUCTION READY
**Capabilities:**
- 57 fully validated DSP engines (ID 0-56)
- Standardized `updateParameters(map<int, float>)` interface
- 6-slot parallel processing architecture
- Parameter smoothing available in DspEngineUtilities
- Atomic parameter updates via APVTS

**Parameter Control:**
```cpp
// Current parameter update mechanism
engine->updateParameters({
    {0, 0.5f},  // param0 = 0.5
    {1, 0.7f}   // param1 = 0.7
});
```

**Limitations:**
- No parameter registry/metadata system
- No macro parameter grouping
- No A/B state storage
- No morphing/interpolation between states
- Parameters are engine-specific (0-15 indices)

#### Preset System
**Files:** `PresetManager.cpp/h`, `PresetSerializer.cpp/h`, `GoldenPreset.h`
**Status:** WORKING
**Capabilities:**
- 250 preset Golden Corpus management
- JSON serialization/deserialization
- Preset search, filtering, categorization
- Validation and quality scoring
- Preset variation generation

**Preset Structure:**
```cpp
struct GoldenPreset {
    String id, name, category;
    int engineTypes[6];           // Engine IDs per slot
    float parameters[6][16];      // All parameters
    float mix[6];                 // Per-slot mix levels
    // ... metadata fields
};
```

**Limitations:**
- No A/B snapshot storage
- No morph position tracking
- No live performance layer
- No undo/redo history
- Single preset loaded at a time (no dual-buffer)

#### Trinity AI Integration
**Files:** `AI_Server/*.py`, Trinity pipeline
**Status:** WORKING
**Capabilities:**
- Natural language to preset generation
- Preset modification based on user requests
- FAISS vector search for similar presets
- Context-aware parameter adjustment
- Quality validation and naming

**What Works:**
```python
# AI generates presets from text
"warm vocal compressor with vintage character"
→ Returns preset with specific engines + parameters
```

**Limitations:**
- No integration with A/B morphing
- No awareness of performance layers
- No macro parameter understanding
- Returns full presets only (not parameter deltas)

### 1.2 Current Architecture Diagram

```
┌─────────────────────────────────────────────────────┐
│                  JUCE Plugin                         │
│  ┌──────────────┐  ┌──────────────┐                │
│  │ APVTS        │  │ PresetManager│                 │
│  │ (Parameters) │  │              │                 │
│  └──────┬───────┘  └──────┬───────┘                │
│         │                  │                         │
│  ┌──────▼──────────────────▼───────┐                │
│  │   ChimeraAudioProcessor         │                │
│  │  ┌─────────────────────────┐    │                │
│  │  │ 6 Slots (EngineBase*)   │    │                │
│  │  │ [E1][E2][E3][E4][E5][E6]│    │                │
│  │  └─────────────────────────┘    │                │
│  └──────────────────────────────────┘                │
└─────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────┐
│              Raspberry Pi Hardware                   │
│  ┌──────────────────────────────────┐               │
│  │     HardwareController           │               │
│  │  ┌─────┐ ┌─────┐ ┌─────┐        │               │
│  │  │Enc 1│ │Enc 2│ │Enc 3│        │               │
│  │  └─────┘ └─────┘ └─────┘        │               │
│  │  ┌─────┐ ┌─────┐ ┌─────┐        │               │
│  │  │Sw 1 │ │Sw 2 │ │Sw 3 │        │               │
│  │  └─────┘ └─────┘ └─────┘        │               │
│  └────────┬─────────────────────────┘               │
│           │                                          │
│           ▼                                          │
│      Callbacks → UI or direct param changes         │
└─────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────┐
│              Trinity AI Server                       │
│  Python Flask → Preset Generation                   │
│  FAISS Vector Search → Similar Presets              │
└─────────────────────────────────────────────────────┘
```

### 1.3 Missing Components Summary

| Component | Status | Criticality |
|-----------|--------|-------------|
| Parameter Registry | Missing | HIGH |
| Macro Parameter System | Missing | HIGH |
| A/B State Engine | Missing | CRITICAL |
| Morph Interpolator | Missing | CRITICAL |
| Mode State Machine | Missing | CRITICAL |
| Event Bus | Missing | HIGH |
| Touch Focus System | Missing | MEDIUM |
| Performance Layer | Missing | HIGH |
| Encoder Mapping Layer | Missing | HIGH |
| Lock-Free Updates | Partial | MEDIUM |
| Atomic Preset Swap | Missing | HIGH |
| Undo/Redo System | Missing | LOW |

---

## 2. Gap Analysis by Component

### 2.1 Parameter Registry System

**Current State:** NONE
**Required Functionality:**
- Central registry of all parameters with metadata
- Parameter type definitions (continuous, discrete, toggle)
- Range and scaling information
- Parameter grouping and categories
- Name/label lookup

**Gap Assessment:**
```cpp
// CURRENT: Direct APVTS parameter access
auto* param = parameters.getRawParameterValue("slot1_param0");
float value = param->load();

// REQUIRED: Rich parameter metadata
struct ParameterDescriptor {
    String id;                    // "slot1_compressor_threshold"
    String displayName;           // "Threshold"
    ParamType type;               // Continuous, Discrete, Toggle
    Range<float> range;           // Min, max, default
    NormalisableRange<float> scaling; // Log, linear, etc.
    String units;                 // "dB", "ms", "%"
    std::vector<String> macros;   // Which macros control this
    int engineSlot;               // Which slot owns it
    int engineParamIndex;         // Engine's internal index
};

class ParameterRegistry {
public:
    void registerParameter(const ParameterDescriptor& desc);
    const ParameterDescriptor* getParameter(const String& id);
    std::vector<String> getParametersInMacro(const String& macroName);
    std::vector<String> getParametersForSlot(int slot);
    float normalizedToPhysical(const String& id, float normalized);
    float physicalToNormalized(const String& id, float physical);
};
```

**Development Effort:** 1-2 weeks
**Files to Create:**
- `ParameterRegistry.h/cpp`
- `ParameterDescriptor.h`
- `ParameterTypes.h`

**Dependencies:**
- Must integrate with existing APVTS
- Must support all 57 engines × 16 params = 912 possible parameters
- Must work with 6 slots = up to 96 active parameters simultaneously

### 2.2 Macro Parameter System

**Current State:** NONE
**Required Functionality:**
- Group multiple parameters under single macro control
- Non-linear scaling curves per parameter within macro
- Macro presets (saved macro configurations)
- Real-time macro value interpolation

**Gap Assessment:**
```cpp
// REQUIRED: Macro system
struct MacroDefinition {
    String name;                  // "Character"
    String description;           // "Adjusts harmonic saturation and tone"

    struct Mapping {
        String parameterId;       // "slot1_param5"
        Curve curve;              // Linear, exponential, S-curve
        float minValue;           // Macro at 0.0 → param = minValue
        float maxValue;           // Macro at 1.0 → param = maxValue
        bool inverted;            // Reverse the mapping
    };

    std::vector<Mapping> mappings; // All parameters controlled by this macro
};

class MacroController {
public:
    void defineMacro(const String& name, const MacroDefinition& def);
    void setMacroValue(const String& name, float value); // 0.0 to 1.0
    float getMacroValue(const String& name) const;
    void applyMacroToParameters(); // Update all mapped parameters

private:
    std::map<String, MacroDefinition> macros;
    std::map<String, float> macroValues;
};
```

**Example Macro:**
```cpp
// "Warmth" macro controls multiple parameters across engines
MacroDefinition warmth;
warmth.name = "Warmth";
warmth.mappings = {
    {"slot1_tube_harmonics", Curve::Linear, 0.0f, 0.8f},      // 0-80%
    {"slot2_tape_saturation", Curve::Exponential, 0.1f, 0.6f},
    {"slot3_eq_highshelf", Curve::Linear, -3.0f, 2.0f}        // -3dB to +2dB
};
```

**Development Effort:** 2-3 weeks
**Files to Create:**
- `MacroController.h/cpp`
- `MacroDefinition.h`
- `MacroCurves.h` (curve math)
- `MacroPresets.h` (saved configs)

**Dependencies:**
- Requires ParameterRegistry
- Must support real-time updates (audio thread safe)
- Must serialize with presets

### 2.3 A/B State Engine

**Current State:** NONE (only single preset state)
**Required Functionality:**
- Dual state storage (A and B snapshots)
- Atomic state capture
- Atomic state restore
- State comparison/diff
- State morphing support

**Gap Assessment:**
```cpp
// CURRENT: Single state in APVTS
// No A/B concept at all

// REQUIRED: A/B state management
class ABStateEngine {
public:
    enum class ActiveState { A, B };

    // Snapshot current parameters to A or B
    void captureToA();
    void captureToB();

    // Restore from snapshot
    void recallA();
    void recallB();

    // Get current active state
    ActiveState getActiveState() const;

    // Swap A and B states
    void swapAB();

    // Copy one state to another
    void copyAtoB();
    void copyBtoA();

    // State comparison
    std::vector<String> getDifferences() const; // List changed params

    // Get state data for morphing
    const StateSnapshot& getStateA() const;
    const StateSnapshot& getStateB() const;

private:
    struct StateSnapshot {
        std::map<String, float> parameters;  // All parameter values
        std::array<int, 6> engineTypes;      // Engine IDs per slot
        std::array<float, 6> mixLevels;      // Mix per slot
        int64_t timestamp;                   // When captured
        String label;                        // User label
    };

    StateSnapshot stateA;
    StateSnapshot stateB;
    std::atomic<ActiveState> activeState{ActiveState::A};

    mutable std::mutex stateMutex;
};
```

**Development Effort:** 2 weeks
**Files to Create:**
- `ABStateEngine.h/cpp`
- `StateSnapshot.h`
- `StateSerializer.h` (for save/load)

**Dependencies:**
- Requires ParameterRegistry
- Must be thread-safe for real-time capture
- Must integrate with PresetManager

### 2.4 Morph Interpolator

**Current State:** NONE
**Required Functionality:**
- Real-time interpolation between A and B states
- Morph position control (0.0 = A, 1.0 = B)
- Per-parameter interpolation curves
- Smooth parameter ramping to avoid clicks
- Lock-free updates for audio thread

**Gap Assessment:**
```cpp
// REQUIRED: Morph interpolation engine
class MorphInterpolator {
public:
    // Set morph position (0.0 = pure A, 1.0 = pure B)
    void setMorphPosition(float position);

    // Get current morph position
    float getMorphPosition() const { return morphPosition.load(); }

    // Update states to morph between
    void setSourceStates(const ABStateEngine::StateSnapshot& a,
                        const ABStateEngine::StateSnapshot& b);

    // Calculate interpolated parameter values
    // Called from audio thread
    std::map<String, float> getInterpolatedParameters() const;

    // Set interpolation curve for specific parameter
    enum class InterpolationCurve {
        Linear,
        Exponential,
        Logarithmic,
        SCurve,
        Stepped  // For discrete params
    };
    void setParameterCurve(const String& paramId, InterpolationCurve curve);

    // Smoothing settings
    void setSmoothingTimeMs(float ms);

private:
    std::atomic<float> morphPosition{0.0f};
    ABStateEngine::StateSnapshot sourceA;
    ABStateEngine::StateSnapshot sourceB;

    std::map<String, InterpolationCurve> parameterCurves;

    // Parameter smoothers to avoid zipper noise
    std::map<String, ParamSmoother> smoothers;

    float interpolateValue(float a, float b, float position,
                          InterpolationCurve curve) const;
};
```

**Development Effort:** 2-3 weeks
**Files to Create:**
- `MorphInterpolator.h/cpp`
- `InterpolationCurves.h` (curve math)
- `MorphSmoother.h` (specialized smoother)

**Dependencies:**
- Requires ABStateEngine
- Requires DspEngineUtilities::ParamSmoother
- Must be lock-free for audio thread access
- Complex interpolation math for different curve types

### 2.5 Mode State Machine

**Current State:** NONE
**Required Functionality:**
- Four distinct modes: MODE, PRESET, MIX, AI
- Mode-specific encoder behaviors
- Mode transitions with validation
- Mode-specific UI feedback
- Timeout and auto-return logic

**Gap Assessment:**
```cpp
// REQUIRED: Mode state machine
class ModeStateMachine {
public:
    enum class Mode {
        MODE,    // Select operating mode
        PRESET,  // Browse/load presets
        MIX,     // Adjust slot levels
        AI       // Trinity AI interaction
    };

    enum class ModeTransition {
        ToMode,
        ToPreset,
        ToMix,
        ToAI,
        Return   // Back to previous mode
    };

    // State transitions
    void transitionTo(Mode newMode);
    void returnToPreviousMode();

    // Current state
    Mode getCurrentMode() const { return currentMode.load(); }
    Mode getPreviousMode() const { return previousMode.load(); }

    // Mode-specific behavior queries
    struct EncoderBehavior {
        String encoder1Function;  // What encoder 1 does in this mode
        String encoder2Function;
        String encoder3Function;
        bool encoder1Active;      // Is this encoder enabled?
        bool encoder2Active;
        bool encoder3Active;
    };
    EncoderBehavior getEncoderBehavior() const;

    // Mode validation
    bool canTransitionTo(Mode newMode) const;

    // Listeners for mode changes
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void modeChanged(Mode newMode, Mode oldMode) = 0;
        virtual void modeTransitionFailed(Mode attemptedMode, String reason) = 0;
    };
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    std::atomic<Mode> currentMode{Mode::MODE};
    std::atomic<Mode> previousMode{Mode::MODE};

    std::vector<Listener*> listeners;
    mutable std::mutex modeMutex;

    void notifyModeChanged(Mode newMode, Mode oldMode);
};
```

**Mode-Specific Encoder Mappings:**
```cpp
// MODE mode: Select which layer to work in
Encoder1: Cycle through [PRESET | MIX | AI | LIVE]
Encoder2: Not used
Encoder3: Not used
Switch1: Quick mode shortcuts

// PRESET mode: Browse and load presets
Encoder1: Browse presets (category)
Encoder2: Browse presets (within category)
Encoder3: Adjust morph position (A/B)
Switch1: Load selected preset to A or B
Switch2: Swap A/B

// MIX mode: Adjust slot levels
Encoder1: Select slot 1-6
Encoder2: Adjust selected slot mix
Encoder3: Master output level
Switch1: Mute/unmute selected slot

// AI mode: Trinity AI control
Encoder1: Browse AI suggestions
Encoder2: Adjust modification strength
Encoder3: Refine parameters
Switch1: Accept AI suggestion
```

**Development Effort:** 2-3 weeks
**Files to Create:**
- `ModeStateMachine.h/cpp`
- `ModeDefinitions.h`
- `ModeValidator.h`

**Dependencies:**
- Must integrate with HardwareController callbacks
- Must trigger UI updates
- Must coordinate with all other subsystems

### 2.6 Event Bus

**Current State:** Callback-based (limited)
**Required Functionality:**
- Centralized event dispatching
- Multiple subscribers per event type
- Event filtering and prioritization
- Thread-safe event posting
- Event history/logging (optional)

**Gap Assessment:**
```cpp
// CURRENT: Direct callbacks
hardwareController.setEncoderCallback([](int num, int pos, bool cw) {
    // Single handler only
});

// REQUIRED: Event bus system
class EventBus {
public:
    enum class EventType {
        EncoderRotated,
        EncoderPressed,
        SwitchChanged,
        ModeChanged,
        PresetLoaded,
        ParameterChanged,
        MorphPositionChanged,
        AIResponseReceived,
        StateCapture,
        // ... many more
    };

    struct Event {
        EventType type;
        int64_t timestamp;
        std::map<String, var> data;  // Flexible payload
    };

    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void handleEvent(const Event& event) = 0;
    };

    // Subscribe to specific event types
    void subscribe(EventType type, Listener* listener);
    void unsubscribe(EventType type, Listener* listener);

    // Subscribe to all events
    void subscribeToAll(Listener* listener);

    // Post events (thread-safe)
    void post(const Event& event);
    void postAsync(const Event& event);  // Non-blocking

    // Event filtering
    void setFilter(std::function<bool(const Event&)> filter);

private:
    std::map<EventType, std::vector<Listener*>> subscribers;
    mutable std::mutex subscribersMutex;

    // Ring buffer for async events
    LockFreeQueue<Event> eventQueue;

    void dispatchEvent(const Event& event);
};
```

**Development Effort:** 1-2 weeks
**Files to Create:**
- `EventBus.h/cpp`
- `Event.h`
- `LockFreeQueue.h` (if not using existing)

**Dependencies:**
- Must integrate with all components
- Requires thread-safe design
- Optional: event recording for debugging

### 2.7 Touch Focus System

**Current State:** NONE
**Required Functionality:**
- Track last-touched encoder/switch
- Timeout-based auto-unfocus
- Focus priority management
- Visual feedback support
- Mode-aware focus behavior

**Gap Assessment:**
```cpp
// REQUIRED: Touch focus tracker
class TouchFocusManager {
public:
    enum class FocusTarget {
        None,
        Encoder1,
        Encoder2,
        Encoder3,
        Switch1,
        Switch2,
        Switch3
    };

    // Set focus (called when user touches control)
    void setFocus(FocusTarget target);

    // Get current focus
    FocusTarget getCurrentFocus() const { return currentFocus.load(); }

    // Focus timeout settings
    void setTimeoutMs(int ms) { timeoutMs = ms; }
    void enableTimeout(bool enable) { timeoutEnabled = enable; }

    // Check if focus is active
    bool hasFocus(FocusTarget target) const;

    // Clear focus
    void clearFocus();

    // Listeners for focus changes
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void focusChanged(FocusTarget newFocus, FocusTarget oldFocus) = 0;
        virtual void focusTimedOut(FocusTarget target) = 0;
    };
    void addListener(Listener* listener);

private:
    std::atomic<FocusTarget> currentFocus{FocusTarget::None};
    std::atomic<int64_t> lastTouchTime{0};
    int timeoutMs = 3000;  // 3 second default
    bool timeoutEnabled = true;

    std::vector<Listener*> listeners;

    // Timer thread for timeout checking
    std::unique_ptr<juce::Timer> timeoutTimer;
    void checkTimeout();
};
```

**Development Effort:** 1 week
**Files to Create:**
- `TouchFocusManager.h/cpp`

**Dependencies:**
- Requires timer system (JUCE Timer available)
- Must integrate with EventBus
- Must coordinate with UI layer

### 2.8 Performance Layer (LIVE)

**Current State:** NONE
**Required Functionality:**
- Real-time parameter tweaking without affecting presets
- Temporary parameter overrides
- Quick-access to key parameters
- Performance macros
- Easy return to preset values

**Gap Assessment:**
```cpp
// REQUIRED: Performance layer
class PerformanceLayer {
public:
    // Enable/disable performance mode
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled.load(); }

    // Tweak parameter in performance mode
    void tweakParameter(const String& paramId, float value);

    // Get current performance value (or preset value if not tweaked)
    float getEffectiveValue(const String& paramId) const;

    // Check if parameter has been tweaked
    bool isParameterTweaked(const String& paramId) const;

    // Revert parameter to preset value
    void revertParameter(const String& paramId);
    void revertAllParameters();

    // Save performance tweaks as new preset
    void saveAsPreset(const String& name);

    // Apply performance tweaks to current preset
    void applyToPreset();

    // Performance macros (quick access controls)
    void definePerformanceMacro(const String& name,
                               const std::vector<String>& paramIds);
    void setPerformanceMacro(const String& name, float value);

private:
    std::atomic<bool> enabled{false};

    // Tweaked values overlay
    std::map<String, float> performanceTweaks;

    // Reference to base preset values
    const PresetManager* presetManager;

    mutable std::mutex tweakMutex;
};
```

**Development Effort:** 1-2 weeks
**Files to Create:**
- `PerformanceLayer.h/cpp`
- `PerformanceMacros.h`

**Dependencies:**
- Requires ParameterRegistry
- Requires PresetManager
- Must integrate with encoder mapping

### 2.9 Encoder Mapping Layer

**Current State:** Direct GPIO callbacks (no abstraction)
**Required Functionality:**
- Map encoder inputs to parameter/function actions
- Mode-aware mapping
- Encoder sensitivity/acceleration
- Value clamping and range mapping
- Undo/redo for encoder changes

**Gap Assessment:**
```cpp
// REQUIRED: Encoder mapping system
class EncoderMapper {
public:
    enum class EncoderAction {
        AdjustParameter,
        BrowseList,
        AdjustMacro,
        NavigateMenu,
        AdjustMorph,
        Custom
    };

    struct EncoderMapping {
        int encoderNum;                    // 0, 1, 2
        EncoderAction action;
        String targetId;                   // Parameter ID, macro name, etc.
        float sensitivity;                 // Rotation speed multiplier
        bool accelerationEnabled;          // Speed-based acceleration
        Range<float> clampRange;           // Min/max limits
    };

    // Set mapping for mode
    void setMapping(ModeStateMachine::Mode mode,
                   int encoderNum,
                   const EncoderMapping& mapping);

    // Handle encoder rotation (called by HardwareController callback)
    void handleEncoderRotation(int encoderNum, int delta, bool clockwise);

    // Handle encoder button press
    void handleEncoderButton(int encoderNum);

    // Get current mapping
    const EncoderMapping* getCurrentMapping(int encoderNum) const;

    // Sensitivity adjustment
    void setSensitivity(int encoderNum, float sensitivity);
    void setAccelerationCurve(int encoderNum, std::function<float(float)> curve);

private:
    // Mode → Encoder → Mapping
    std::map<ModeStateMachine::Mode,
             std::array<EncoderMapping, 3>> mappings;

    // Current mode (reference from ModeStateMachine)
    const ModeStateMachine* modeStateMachine;

    // Reference to parameter/macro systems
    ParameterRegistry* paramRegistry;
    MacroController* macroController;
    MorphInterpolator* morphInterpolator;

    // Acceleration tracking
    std::array<int64_t, 3> lastRotationTime;
    std::array<float, 3> currentAcceleration;
};
```

**Development Effort:** 2 weeks
**Files to Create:**
- `EncoderMapper.h/cpp`
- `EncoderMappingDefinitions.h`

**Dependencies:**
- Requires HardwareController integration
- Requires ModeStateMachine
- Requires ParameterRegistry, MacroController, MorphInterpolator
- Complex logic for acceleration curves

### 2.10 Lock-Free Parameter Updates

**Current State:** PARTIAL (atomic values in APVTS)
**Required Functionality:**
- True lock-free audio thread parameter access
- No allocations in audio thread
- No mutex locks in audio thread
- Wait-free reads for critical path
- Safe concurrent updates from UI/hardware threads

**Gap Assessment:**
```cpp
// CURRENT: APVTS uses atomics but not fully lock-free
auto* param = parameters.getRawParameterValue("slot1_param0");
float value = param->load();  // Atomic, but APVTS may lock internally

// REQUIRED: Lock-free parameter cache
template<typename T>
class LockFreeParameter {
public:
    // Set from UI/hardware thread (may occasionally fail if contended)
    bool trySet(T value) {
        return atomicValue.compare_exchange_weak(currentValue, value);
    }

    // Force set (may spin briefly)
    void set(T value) {
        while (!trySet(value)) {
            std::this_thread::yield();
        }
    }

    // Get from audio thread (always succeeds, wait-free)
    T get() const noexcept {
        return atomicValue.load(std::memory_order_relaxed);
    }

private:
    std::atomic<T> atomicValue;
    T currentValue;
};

class LockFreeParameterCache {
public:
    // Pre-allocate space for all possible parameters
    void initialize(int maxParameters);

    // Audio thread: get parameter (wait-free)
    float getParameter(int slotIndex, int paramIndex) const noexcept {
        return cache[slotIndex][paramIndex].get();
    }

    // UI/hardware thread: update parameter
    void setParameter(int slotIndex, int paramIndex, float value) {
        cache[slotIndex][paramIndex].set(value);
    }

    // Bulk update from morph interpolator
    void updateFromMorph(const std::map<String, float>& values);

private:
    // 6 slots × 16 params × lock-free atomic storage
    std::array<std::array<LockFreeParameter<float>, 16>, 6> cache;
};
```

**Development Effort:** 1-2 weeks
**Files to Create:**
- `LockFreeParameterCache.h/cpp`
- `LockFreeQueue.h` (for event bus)

**Dependencies:**
- Requires careful atomic ordering considerations
- Must not allocate memory in audio thread
- Must integrate with existing APVTS
- Extensive testing required for race conditions

### 2.11 Atomic Preset System

**Current State:** PresetManager exists but no atomic swap
**Required Functionality:**
- Atomic preset state capture
- Atomic preset loading (no glitches)
- Versioned preset format
- Preset diff/merge
- Crash-safe preset save

**Gap Assessment:**
```cpp
// CURRENT: PresetManager loads presets but may glitch
void loadPreset(const GoldenPreset& preset) {
    for (int slot = 0; slot < 6; ++slot) {
        loadEngine(slot, preset.engineTypes[slot]);
        for (int p = 0; p < 16; ++p) {
            // Multiple parameter updates = potential clicks
            setParameter(slot, p, preset.parameters[slot][p]);
        }
    }
}

// REQUIRED: Atomic preset swap
class AtomicPresetLoader {
public:
    // Prepare preset in background (allocate engines, set params)
    PreparedPreset* preparePreset(const GoldenPreset& preset);

    // Atomically swap to prepared preset (on audio thread boundary)
    void swapToPreset(PreparedPreset* prepared);

    // Double-buffered preset state
    struct PreparedPreset {
        std::array<std::unique_ptr<EngineBase>, 6> engines;
        std::array<std::array<float, 16>, 6> parameters;
        std::array<float, 6> mixLevels;
        bool ready = false;
    };

    // Versioned preset format
    struct PresetVersion {
        int major, minor, patch;
        String schemaHash;  // Detect incompatible changes
    };

    // Load preset with version checking
    bool loadPresetSafe(const GoldenPreset& preset, bool allowUpgrade = true);

    // Crash-safe save (atomic file replace)
    bool savePresetAtomic(const GoldenPreset& preset, const File& path);

private:
    std::unique_ptr<PreparedPreset> currentPreset;
    std::unique_ptr<PreparedPreset> nextPreset;
    std::atomic<bool> swapReady{false};
};
```

**Development Effort:** 2 weeks
**Files to Create:**
- `AtomicPresetLoader.h/cpp`
- `PresetVersioning.h`
- Integration with existing PresetManager

**Dependencies:**
- Requires double-buffered engine allocation
- Must coordinate with audio thread safely
- File I/O must be crash-safe (write temp, atomic rename)

---

## 3. Architectural Changes Required

### 3.1 System Architecture Transformation

**Current Architecture:** Simple, direct control flow
**Target Architecture:** Layered, event-driven, state-managed

```
CURRENT SIMPLE ARCHITECTURE:
┌─────────────┐
│  Hardware   │
│  Controller │
└──────┬──────┘
       │ (direct callbacks)
       ▼
┌─────────────┐
│    APVTS    │
│ Parameters  │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  6 Engines  │
└─────────────┘


TARGET TCS v0.5 ARCHITECTURE:
┌──────────────────────────────────────────────────────────┐
│                     PRESENTATION LAYER                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐               │
│  │   UI     │  │ Hardware │  │  Trinity │               │
│  │ Display  │  │ Controls │  │   AI     │               │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘               │
└───────┼─────────────┼─────────────┼────────────────────┘
        │             │             │
        └─────────────┼─────────────┘
                      ▼
┌──────────────────────────────────────────────────────────┐
│                    EVENT BUS LAYER                        │
│         (Centralized event routing & filtering)           │
└─────────────┬────────────────────────────────────────────┘
              │
              ▼
┌──────────────────────────────────────────────────────────┐
│                   CONTROL LAYER                           │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │ Mode State   │  │ Touch Focus  │  │   Encoder     │  │
│  │   Machine    │  │   Manager    │  │    Mapper     │  │
│  └──────┬───────┘  └──────┬───────┘  └───────┬───────┘  │
└─────────┼──────────────────┼──────────────────┼──────────┘
          └──────────────────┼──────────────────┘
                             ▼
┌──────────────────────────────────────────────────────────┐
│                 PARAMETER LAYER                           │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │  Parameter   │  │    Macro     │  │ Performance   │  │
│  │   Registry   │  │  Controller  │  │     Layer     │  │
│  └──────┬───────┘  └──────┬───────┘  └───────┬───────┘  │
└─────────┼──────────────────┼──────────────────┼──────────┘
          └──────────────────┼──────────────────┘
                             ▼
┌──────────────────────────────────────────────────────────┐
│                   STATE LAYER                             │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │  A/B State   │  │    Morph     │  │    Preset     │  │
│  │   Engine     │  │ Interpolator │  │    Manager    │  │
│  └──────┬───────┘  └──────┬───────┘  └───────┬───────┘  │
└─────────┼──────────────────┼──────────────────┼──────────┘
          └──────────────────┼──────────────────┘
                             ▼
┌──────────────────────────────────────────────────────────┐
│                LOCK-FREE CACHE LAYER                      │
│         (Wait-free parameter access for audio thread)     │
└──────────────────────┬───────────────────────────────────┘
                       ▼
┌──────────────────────────────────────────────────────────┐
│                   AUDIO ENGINE LAYER                      │
│  ┌────────┬────────┬────────┬────────┬────────┬────────┐ │
│  │ Slot 1 │ Slot 2 │ Slot 3 │ Slot 4 │ Slot 5 │ Slot 6 │ │
│  │Engine X│Engine Y│Engine Z│Engine A│Engine B│Engine C│ │
│  └────────┴────────┴────────┴────────┴────────┴────────┘ │
└──────────────────────────────────────────────────────────┘
```

### 3.2 Data Flow Changes

**Current Data Flow:** Top-down, synchronous
```
User turns encoder
  → HardwareController detects
    → Callback fires
      → APVTS parameter updated (atomic)
        → Engine reads new value next audio block
```

**TCS v0.5 Data Flow:** Event-driven, layered, asynchronous
```
User turns encoder
  → HardwareController detects
    → Event posted to EventBus: {EncoderRotated, num=1, delta=5}
      → TouchFocusManager: Update focus timestamp
      → ModeStateMachine: Query current mode
        → EncoderMapper: Map encoder to action based on mode
          → [PRESET mode] → Browse preset list
          → [MIX mode] → Adjust slot mix level
          → [AI mode] → Modify AI strength parameter
            → MacroController: Update macro value (if mapped)
              → ParameterRegistry: Resolve macro → parameters
                → PerformanceLayer: Record tweak (if LIVE mode)
                  → MorphInterpolator: Update morph position (if morphing)
                    → LockFreeCache: Update cached values
                      → Audio thread: Read new values (wait-free)
```

### 3.3 Threading Model Changes

**Current Threading:**
- Audio thread: Runs processBlock(), reads APVTS atomics
- Message thread: UI updates, APVTS parameter changes
- Hardware thread: HardwareController polling (1ms intervals)

**TCS v0.5 Threading:**
- **Audio Thread** (highest priority, lock-free):
  - Reads from LockFreeParameterCache only
  - No mutex locks, no allocations
  - Processes audio buffers

- **Hardware Thread** (high priority, runs on RPi core):
  - Polls GPIO at 1ms intervals
  - Posts events to EventBus
  - Minimal processing, delegate to other threads

- **Control Thread** (medium priority):
  - Processes EventBus events
  - Updates state machines
  - Calculates morph interpolations
  - Updates LockFreeParameterCache

- **UI Thread** (JUCE message thread):
  - Updates visual displays
  - Handles user interactions from GUI
  - Low-latency response to hardware events

- **AI Thread** (low priority, background):
  - Communicates with Trinity AI server
  - Processes preset generation requests
  - Updates UI when responses arrive

### 3.4 File Structure Changes

**New Directory Structure:**
```
JUCE_Plugin/Source/
├── Control/                    # NEW: Control layer
│   ├── ModeStateMachine.h/cpp
│   ├── EncoderMapper.h/cpp
│   ├── TouchFocusManager.h/cpp
│   ├── EventBus.h/cpp
│   └── Event.h
├── Parameters/                 # NEW: Parameter layer
│   ├── ParameterRegistry.h/cpp
│   ├── MacroController.h/cpp
│   ├── PerformanceLayer.h/cpp
│   ├── LockFreeParameterCache.h/cpp
│   └── ParameterDescriptor.h
├── State/                      # NEW: State management
│   ├── ABStateEngine.h/cpp
│   ├── MorphInterpolator.h/cpp
│   ├── StateSnapshot.h
│   ├── AtomicPresetLoader.h/cpp
│   └── PresetVersioning.h
├── Hardware/                   # REORGANIZED
│   ├── HardwareController.h/cpp (existing)
│   └── HardwareTypes.h
├── Engines/                    # REORGANIZED (group all engines)
│   ├── EngineBase.h (existing)
│   ├── EngineFactory.h/cpp (existing)
│   ├── [All 57 engine implementations]
│   └── DspEngineUtilities.h (existing)
├── Presets/                    # REORGANIZED
│   ├── PresetManager.h/cpp (existing)
│   ├── GoldenPreset.h (existing)
│   └── PresetSerializer.h/cpp (existing)
├── UI/                         # Existing UI code
│   └── [All existing UI components]
└── PluginProcessor.h/cpp       # Modified to integrate TCS
```

---

## 4. Critical Dependencies

### 4.1 Internal Dependencies

**Dependency Graph:**
```
ParameterRegistry
  └─> MacroController
       └─> PerformanceLayer
            └─> EncoderMapper
                 └─> ModeStateMachine

ABStateEngine
  └─> MorphInterpolator
       └─> LockFreeParameterCache
            └─> Audio Engines

EventBus
  └─> ALL COMPONENTS (hub pattern)

HardwareController
  └─> EventBus
       └─> EncoderMapper, TouchFocusManager

PresetManager
  └─> ABStateEngine
       └─> AtomicPresetLoader
```

**Critical Path Analysis:**
1. **ParameterRegistry** must be built first (foundational)
2. **ABStateEngine** can be built in parallel
3. **EventBus** should be built early (integration point)
4. **ModeStateMachine** requires EventBus
5. **EncoderMapper** requires almost everything else
6. **MorphInterpolator** requires ABStateEngine + ParameterRegistry

### 4.2 External Dependencies

**JUCE Framework:**
- `juce::AudioProcessorValueTreeState` (existing)
- `juce::ListenerList` for callback management
- `juce::Timer` for timeout management
- `juce::CriticalSection` and `juce::ScopedLock` for mutexes
- `juce::Atomic<T>` for lock-free primitives
- `juce::var` for flexible event payloads

**Standard Library:**
- `<atomic>` for lock-free programming
- `<thread>` for thread identification
- `<map>`, `<vector>` for data structures
- `<functional>` for callbacks

**Trinity AI Server:**
- HTTP communication (existing via JUCE)
- JSON parsing (existing via JUCE)
- Async request/response pattern

**Raspberry Pi GPIO:**
- libgpiod (existing in HardwareController)
- Linux-specific threading for hardware polling

### 4.3 Build System Changes

**CMakeLists.txt Updates:**
```cmake
# NEW: Add Control layer sources
set(CONTROL_SOURCES
    Source/Control/ModeStateMachine.cpp
    Source/Control/EncoderMapper.cpp
    Source/Control/TouchFocusManager.cpp
    Source/Control/EventBus.cpp
)

# NEW: Add Parameter layer sources
set(PARAMETER_SOURCES
    Source/Parameters/ParameterRegistry.cpp
    Source/Parameters/MacroController.cpp
    Source/Parameters/PerformanceLayer.cpp
    Source/Parameters/LockFreeParameterCache.cpp
)

# NEW: Add State layer sources
set(STATE_SOURCES
    Source/State/ABStateEngine.cpp
    Source/State/MorphInterpolator.cpp
    Source/State/AtomicPresetLoader.cpp
)

# Add to target
target_sources(ChimeraPhoenix PRIVATE
    ${CONTROL_SOURCES}
    ${PARAMETER_SOURCES}
    ${STATE_SOURCES}
    # ... existing sources
)
```

---

## 5. Technical Challenges

### 5.1 Lock-Free Programming

**Challenge:** Ensure audio thread is truly wait-free
**Complexity:** HIGH
**Risk:** Audio glitches if done incorrectly

**Considerations:**
- Atomic operations must use correct memory ordering
- No allocations in audio thread (pre-allocate everything)
- No mutex locks, even "fast" ones
- Careful design of lock-free data structures
- Testing for race conditions is difficult

**Mitigation:**
- Use proven lock-free patterns (e.g., single-producer/single-consumer queues)
- Extensive stress testing with thread sanitizers
- Code review with concurrency expert
- Fallback to simple atomic<float> if complex structures fail

### 5.2 Real-Time Morph Interpolation

**Challenge:** Smoothly interpolate 96 parameters simultaneously
**Complexity:** MEDIUM-HIGH
**Risk:** Zipper noise, clicks, CPU spikes

**Considerations:**
- Each parameter needs independent smoother
- Interpolation curves must be computationally cheap
- 96 smoothers × 48kHz = 4.6M calculations/second
- Must handle discrete vs continuous parameters differently
- Morph position changes must ramp smoothly

**Mitigation:**
- Use efficient one-pole smoothers (already in DspEngineUtilities)
- Compute interpolation curves with lookup tables
- Only update changed parameters (sparse updates)
- Profile and optimize hot paths
- Consider SIMD for batch parameter updates

### 5.3 Mode Transition Glitch-Free

**Challenge:** Change encoder behavior without audio artifacts
**Complexity:** MEDIUM
**Risk:** Parameter jumps, audio pops

**Considerations:**
- Mode change mid-encoder-turn can cause value jump
- Parameters must ramp to new values
- UI must update in sync with audio
- A/B state must remain coherent

**Mitigation:**
- Freeze current parameter values during mode transition
- Ramp to new values over short time (50-100ms)
- Use atomic flags to coordinate audio/UI threads
- Test all mode transition combinations

### 5.4 Preset Load Atomicity

**Challenge:** Load new preset without clicks/glitches
**Complexity:** HIGH
**Risk:** Audio dropout during preset change

**Considerations:**
- Engine allocation may take time (non-real-time)
- Parameter updates must be atomic
- Mix levels must fade smoothly
- Previous audio tail must decay naturally

**Mitigation:**
- Double-buffer preset state (prepare in background)
- Crossfade between old and new preset
- Use engine bypass during swap
- Test with all 57 engines × 6 slots = 342 combinations

### 5.5 Hardware Polling Latency

**Challenge:** 1ms GPIO polling may miss fast encoder turns
**Complexity:** LOW-MEDIUM
**Risk:** Dropped encoder counts

**Considerations:**
- Fast encoder rotation may exceed polling rate
- Quadrature decoding requires catching all state changes
- Switch debouncing adds latency
- RPi CPU load from other threads

**Mitigation:**
- Keep hardware thread at high priority
- Optimize polling loop (already efficient)
- Consider interrupt-driven GPIO (more complex)
- Test with rapid encoder rotation

### 5.6 Trinity AI Integration Async

**Challenge:** AI responses arrive asynchronously
**Complexity:** MEDIUM
**Risk:** State coherency when preset changes during AI request

**Considerations:**
- AI request may take 500ms - 2 seconds
- User may change preset while waiting
- Response may no longer be relevant
- Must handle request cancellation

**Mitigation:**
- Track request ID and current preset hash
- Discard stale AI responses
- Show "processing" indicator in UI
- Allow user to cancel in-flight requests

### 5.7 Undo/Redo for Performance Tweaks

**Challenge:** Track parameter changes for undo
**Complexity:** MEDIUM
**Risk:** Memory usage, slow undo stack

**Considerations:**
- Every encoder turn generates potential undo point
- Undo stack size must be limited
- Grouping related changes (e.g., rapid encoder turns)
- Undo across mode changes

**Mitigation:**
- Coalesce rapid changes into single undo point
- Limit undo stack to 50-100 entries
- Store deltas, not full state snapshots
- Periodic cleanup of old undo entries

---

## 6. Development Roadmap

### 6.1 Phase 1: Foundation (Weeks 1-4)

**Sprint 1: Parameter Infrastructure**
- Build ParameterRegistry
- Define ParameterDescriptor schema
- Integration with existing APVTS
- Unit tests for parameter lookup

**Sprint 2: State Management Core**
- Build ABStateEngine
- Implement StateSnapshot
- State capture and restore
- State serialization

**Sprint 3: Event Bus**
- Build EventBus core
- Define all event types
- Integrate with HardwareController
- Event logging for debugging

**Sprint 4: Lock-Free Cache**
- Build LockFreeParameterCache
- Audio thread integration
- Performance testing
- Race condition stress tests

**Deliverables:**
- 4 core components operational
- Unit tests passing
- Integration test harness
- Performance benchmarks

### 6.2 Phase 2: Control Layer (Weeks 5-8)

**Sprint 5: Mode State Machine**
- Define all modes and transitions
- Build state machine logic
- Mode-specific encoder mappings
- UI integration for mode display

**Sprint 6: Encoder Mapper**
- Build EncoderMapper
- Implement mode-aware mapping
- Encoder acceleration curves
- Integration with ParameterRegistry

**Sprint 7: Morph Interpolator**
- Build MorphInterpolator
- Implement interpolation curves
- Integration with ABStateEngine
- Real-time morph testing

**Sprint 8: Macro System**
- Build MacroController
- Define standard macros
- Macro-to-parameter mapping
- Macro presets

**Deliverables:**
- All control layer components working
- Mode transitions functional
- Morph system operational
- Macro system tested

### 6.3 Phase 3: Integration & Features (Weeks 9-12)

**Sprint 9: Performance Layer**
- Build PerformanceLayer
- LIVE mode implementation
- Performance tweaks tracking
- Save tweaks to preset

**Sprint 10: Touch Focus & Polish**
- Build TouchFocusManager
- Timeout implementation
- UI feedback integration
- Focus priority management

**Sprint 11: Atomic Preset Loader**
- Build AtomicPresetLoader
- Double-buffered preset swap
- Crossfade implementation
- Preset versioning

**Sprint 12: Integration & Testing**
- Full system integration
- End-to-end testing
- Performance optimization
- Bug fixes

**Deliverables:**
- Complete TCS v0.5 system
- All features working
- Comprehensive test suite
- Performance targets met

### 6.4 Phase 4: Validation & Polish (Weeks 13-16)

**Sprint 13: User Testing**
- Alpha testing with hardware
- Collect user feedback
- Identify usability issues
- Performance profiling

**Sprint 14: Refinement**
- Fix identified bugs
- Improve encoder feel (acceleration tuning)
- Optimize hot paths
- Add missing features

**Sprint 15: Documentation**
- API documentation
- User manual
- Developer guide
- Architecture diagrams

**Sprint 16: Production Prep**
- Final testing
- Code review
- Release candidate build
- Deployment preparation

---

## 7. Sprint Breakdown

### Sprint 1: Parameter Registry (Week 1)

**Goals:**
- Create ParameterRegistry class
- Define ParameterDescriptor schema
- Integrate with APVTS
- Support all 57 engines

**Tasks:**
```
[ ] Create ParameterDescriptor.h
    [ ] Define parameter types (continuous, discrete, toggle)
    [ ] Define range and scaling structures
    [ ] Add metadata fields (units, name, category)

[ ] Create ParameterRegistry.h/cpp
    [ ] Implement registration API
    [ ] Build lookup tables (by ID, by slot, by engine)
    [ ] Add range conversion functions

[ ] Populate registry with all engine parameters
    [ ] Extract parameter info from trinity_context.md
    [ ] Create initialization code
    [ ] Validate against UnifiedDefaultParameters.cpp

[ ] Unit tests
    [ ] Test parameter lookup
    [ ] Test range conversions
    [ ] Test edge cases (invalid IDs, etc.)

[ ] Integration with PluginProcessor
    [ ] Initialize registry on startup
    [ ] Expose to other components
```

**Deliverables:**
- ParameterRegistry.h/cpp
- ParameterDescriptor.h
- ParameterTypes.h
- Unit test suite
- Documentation

**Estimated Effort:** 40 hours

### Sprint 2: A/B State Engine (Week 2)

**Goals:**
- Implement dual-state storage
- State capture/restore
- State serialization
- Thread-safe operations

**Tasks:**
```
[ ] Create StateSnapshot.h
    [ ] Define snapshot data structure
    [ ] Include all parameter values
    [ ] Include engine IDs and mix levels
    [ ] Add timestamp and metadata

[ ] Create ABStateEngine.h/cpp
    [ ] Implement capture to A/B
    [ ] Implement recall from A/B
    [ ] Implement state swap
    [ ] Add state comparison (diff)

[ ] Serialization
    [ ] JSON export/import
    [ ] Binary format (for speed)
    [ ] Version tagging

[ ] Thread safety
    [ ] Use mutex for state updates
    [ ] Atomic active state flag
    [ ] Safe concurrent access

[ ] Unit tests
    [ ] Test capture/recall
    [ ] Test state swapping
    [ ] Test serialization round-trip
    [ ] Test thread safety (stress test)
```

**Deliverables:**
- ABStateEngine.h/cpp
- StateSnapshot.h
- StateSerializer.h/cpp
- Unit tests
- Integration example

**Estimated Effort:** 40 hours

### Sprint 3: Event Bus (Week 3)

**Goals:**
- Centralized event system
- Multiple subscribers
- Thread-safe event posting
- Event filtering

**Tasks:**
```
[ ] Create Event.h
    [ ] Define EventType enum (30+ types)
    [ ] Define Event structure (type, timestamp, data)
    [ ] Add helper functions

[ ] Create EventBus.h/cpp
    [ ] Implement subscribe/unsubscribe
    [ ] Implement synchronous event posting
    [ ] Implement async event queue
    [ ] Add event filtering

[ ] Lock-free queue for async events
    [ ] Single-producer/single-consumer queue
    [ ] Wait-free reads
    [ ] Bounded queue size

[ ] Integration with HardwareController
    [ ] Post encoder events
    [ ] Post switch events
    [ ] Post button events

[ ] Event logging (debug mode)
    [ ] Log all events to file
    [ ] Event replay for testing

[ ] Unit tests
    [ ] Test subscribe/unsubscribe
    [ ] Test event delivery
    [ ] Test filtering
    [ ] Test threading safety
```

**Deliverables:**
- EventBus.h/cpp
- Event.h
- LockFreeQueue.h (if new)
- Integration with HardwareController
- Event logging system

**Estimated Effort:** 35 hours

### Sprint 4: Lock-Free Cache (Week 4)

**Goals:**
- Wait-free parameter access
- Pre-allocated storage
- Audio thread integration
- Performance validation

**Tasks:**
```
[ ] Create LockFreeParameter<T> template
    [ ] Atomic storage
    [ ] trySet() and set() methods
    [ ] Wait-free get() method

[ ] Create LockFreeParameterCache
    [ ] 6 slots × 16 params storage
    [ ] getParameter() (audio thread)
    [ ] setParameter() (UI/control thread)
    [ ] Bulk update support

[ ] Integration with PluginProcessor
    [ ] Replace APVTS reads in audio thread
    [ ] Keep APVTS for UI/automation
    [ ] Sync cache from APVTS

[ ] Performance testing
    [ ] Measure read latency (should be ~1ns)
    [ ] Measure write contention
    [ ] Stress test with rapid updates

[ ] Thread sanitizer testing
    [ ] Run with TSan
    [ ] Fix any race conditions
    [ ] Verify memory ordering

[ ] Documentation
    [ ] Memory model explanation
    [ ] Usage guidelines
    [ ] Performance characteristics
```

**Deliverables:**
- LockFreeParameterCache.h/cpp
- Integration in PluginProcessor
- Performance benchmark results
- Thread safety validation report

**Estimated Effort:** 45 hours

### Sprints 5-16: [Detailed breakdown available on request]

---

## 8. Risk Assessment

### 8.1 Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Lock-free bugs cause audio glitches | MEDIUM | HIGH | Extensive testing, code review, fallback plan |
| Morph interpolation too CPU-intensive | LOW | MEDIUM | Profiling, SIMD optimization, reduce smoothers |
| Mode transitions cause parameter jumps | MEDIUM | MEDIUM | Careful state management, ramping |
| Preset loading not atomic (clicks) | LOW | HIGH | Double-buffering, crossfade |
| Event bus becomes bottleneck | LOW | MEDIUM | Lock-free queue, profiling |
| Trinity AI integration issues | LOW | LOW | Well-defined interface, existing code works |
| Hardware polling misses encoder counts | LOW | MEDIUM | High-priority thread, interrupt-driven GPIO (plan B) |

### 8.2 Schedule Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Scope creep | MEDIUM | HIGH | Strict adherence to spec, defer nice-to-haves |
| Underestimated complexity | MEDIUM | HIGH | 20% time buffer, prioritize core features |
| Integration issues between components | MEDIUM | MEDIUM | Early integration testing, interface contracts |
| Testing takes longer than planned | MEDIUM | MEDIUM | Parallel test development, automated testing |
| Learning curve for lock-free programming | LOW | MEDIUM | Research phase, use proven patterns |

### 8.3 Mitigation Strategies

**Technical Mitigation:**
1. **Prototype risky components early** (lock-free cache, morph interpolator)
2. **Extensive unit testing** (aim for 80%+ coverage)
3. **Thread sanitizer** integration in CI/CD
4. **Performance benchmarks** after each sprint
5. **Code review** for all lock-free code

**Schedule Mitigation:**
1. **Build foundation first** (ParameterRegistry, ABState, EventBus)
2. **Parallel development** where dependencies allow
3. **Incremental integration** (don't wait until end)
4. **Feature prioritization** (must-have vs nice-to-have)
5. **Buffer time** (plan for 12 weeks, expect 14-16)

---

## 9. Testing Strategy

### 9.1 Unit Testing

**Coverage Target:** 80%+

**Key Unit Tests:**
- ParameterRegistry: Lookup, range conversion, edge cases
- ABStateEngine: Capture, restore, swap, serialization
- EventBus: Subscribe, post, filter, threading
- LockFreeCache: Concurrent reads/writes, memory ordering
- MorphInterpolator: Interpolation curves, edge cases
- MacroController: Macro mapping, value calculation
- ModeStateMachine: All transitions, invalid states
- EncoderMapper: Mapping logic, acceleration

**Tools:**
- JUCE UnitTest framework
- Google Test (if preferred)
- Mock objects for dependencies

### 9.2 Integration Testing

**Test Scenarios:**
1. **Hardware → Parameter Flow**
   - Turn encoder → parameter updates → audio changes
   - Validate latency < 10ms

2. **Mode Transitions**
   - Test all mode combinations
   - Verify encoder behavior changes
   - Check for parameter jumps

3. **A/B Morphing**
   - Capture states A and B
   - Morph between them
   - Verify smooth interpolation
   - Check for audio artifacts

4. **Preset Loading**
   - Load preset atomically
   - Verify no clicks/pops
   - Check all parameters loaded correctly

5. **Trinity AI Integration**
   - Request preset from AI
   - Load AI preset
   - Verify parameters applied

6. **Performance Layer**
   - Tweak parameters in LIVE mode
   - Verify tweaks don't affect preset
   - Save tweaks, verify persistence

### 9.3 Performance Testing

**Benchmarks:**
- LockFreeCache read latency: < 10ns target
- Event bus dispatch latency: < 100μs target
- Morph interpolation CPU: < 5% target
- Mode transition latency: < 50ms target
- Parameter update latency: < 10ms target

**Stress Tests:**
- Rapid encoder rotation (100+ events/second)
- Continuous morph sweeping
- Frequent mode changes
- Simultaneous parameter updates
- Long-running stability (24+ hours)

### 9.4 Thread Safety Testing

**Tools:**
- ThreadSanitizer (TSan)
- AddressSanitizer (ASan)
- Valgrind (Helgrind, DRD)

**Test Cases:**
- Concurrent parameter reads/writes
- Event bus multi-threaded posting
- State capture during audio processing
- Preset loading during parameter changes

### 9.5 User Acceptance Testing

**Test Scenarios:**
1. **Workflow: Browse and Load Preset**
   - Enter PRESET mode
   - Browse categories
   - Load preset to A
   - Load different preset to B
   - Morph between A and B

2. **Workflow: Performance Tweaking**
   - Load preset
   - Enter LIVE mode
   - Tweak parameters
   - Verify preset unchanged
   - Save tweaks as new preset

3. **Workflow: Macro Control**
   - Load preset
   - Adjust macro parameter
   - Verify multiple parameters change
   - Check sonic result

4. **Workflow: AI Assistance**
   - Enter AI mode
   - Request "warm vocal compressor"
   - Load AI suggestion
   - Refine with encoders

---

## 10. Success Criteria

### 10.1 Functional Requirements

**Must Have (Critical):**
- [ ] All 4 modes operational (MODE, PRESET, MIX, AI)
- [ ] A/B state capture and restore working
- [ ] Morph interpolation smooth and glitch-free
- [ ] Encoder mapping changes per mode
- [ ] Preset loading atomic (no clicks)
- [ ] Lock-free parameter updates validated
- [ ] All 57 engines work with new system
- [ ] Trinity AI integration functional

**Should Have (Important):**
- [ ] Macro parameter system working
- [ ] Performance layer (LIVE mode) operational
- [ ] Touch focus with timeout
- [ ] Event bus logging for debugging
- [ ] Preset versioning and validation

**Nice to Have (Optional):**
- [ ] Undo/redo for parameter changes
- [ ] Macro presets library
- [ ] Advanced encoder acceleration curves
- [ ] Event replay for testing
- [ ] Performance profiling UI

### 10.2 Performance Requirements

**Audio Performance:**
- [ ] No audio glitches during mode changes
- [ ] No clicks during preset loading
- [ ] Morph interpolation < 5% CPU usage
- [ ] Lock-free cache reads < 10ns latency
- [ ] Total system CPU increase < 10% vs current

**Latency Requirements:**
- [ ] Encoder to parameter update: < 10ms
- [ ] Mode transition: < 50ms
- [ ] Preset load time: < 200ms (user-perceived instant)
- [ ] AI response handling: < 100ms after response arrives

**Stability Requirements:**
- [ ] No crashes during 24-hour stress test
- [ ] No memory leaks (Valgrind clean)
- [ ] No race conditions (TSan clean)
- [ ] Handles rapid user input without lockup

### 10.3 Quality Requirements

**Code Quality:**
- [ ] 80%+ unit test coverage
- [ ] All public APIs documented
- [ ] Code review completed for lock-free code
- [ ] No compiler warnings (-Wall -Wextra)
- [ ] Clang-tidy clean

**User Experience:**
- [ ] Encoder feel is responsive and natural
- [ ] Mode transitions are intuitive
- [ ] Morph control is smooth and musical
- [ ] UI updates match hardware state
- [ ] Error states are clearly indicated

**Maintainability:**
- [ ] Clear separation of concerns
- [ ] Well-documented architecture
- [ ] Minimal code duplication
- [ ] Consistent coding style
- [ ] Easy to extend with new modes/features

---

## Appendix A: Component Dependency Matrix

| Component | Depends On | Used By |
|-----------|-----------|---------|
| ParameterRegistry | APVTS | MacroController, EncoderMapper, PerformanceLayer |
| MacroController | ParameterRegistry | EncoderMapper, PerformanceLayer |
| ABStateEngine | ParameterRegistry | MorphInterpolator, AtomicPresetLoader |
| MorphInterpolator | ABStateEngine, ParameterRegistry | LockFreeCache, EncoderMapper |
| EventBus | None | ALL COMPONENTS |
| ModeStateMachine | EventBus | EncoderMapper, UI |
| EncoderMapper | ModeStateMachine, ParameterRegistry, MacroController | HardwareController integration |
| TouchFocusManager | EventBus | EncoderMapper, UI |
| PerformanceLayer | ParameterRegistry, MacroController | EncoderMapper |
| LockFreeCache | None | Audio Engines, MorphInterpolator |
| AtomicPresetLoader | ABStateEngine, PresetManager | PresetManager integration |

---

## Appendix B: File Size Estimates

| Component | Header (LOC) | Implementation (LOC) | Total |
|-----------|--------------|---------------------|-------|
| ParameterRegistry | 150 | 400 | 550 |
| MacroController | 120 | 350 | 470 |
| ABStateEngine | 100 | 300 | 400 |
| MorphInterpolator | 130 | 450 | 580 |
| EventBus | 100 | 250 | 350 |
| ModeStateMachine | 120 | 300 | 420 |
| EncoderMapper | 140 | 500 | 640 |
| TouchFocusManager | 80 | 180 | 260 |
| PerformanceLayer | 100 | 250 | 350 |
| LockFreeCache | 90 | 200 | 290 |
| AtomicPresetLoader | 110 | 350 | 460 |
| **TOTAL** | **1,240** | **3,530** | **4,770** |

Additional files (definitions, types, tests): ~3,000 LOC
**Grand Total: ~8,000 LOC**

---

## Appendix C: Implementation Checklist

### Phase 1: Foundation
- [ ] Sprint 1: ParameterRegistry complete
- [ ] Sprint 2: ABStateEngine complete
- [ ] Sprint 3: EventBus complete
- [ ] Sprint 4: LockFreeCache complete
- [ ] Phase 1 integration tests passing
- [ ] Performance benchmarks recorded

### Phase 2: Control Layer
- [ ] Sprint 5: ModeStateMachine complete
- [ ] Sprint 6: EncoderMapper complete
- [ ] Sprint 7: MorphInterpolator complete
- [ ] Sprint 8: MacroController complete
- [ ] Phase 2 integration tests passing
- [ ] Mode transitions validated

### Phase 3: Features
- [ ] Sprint 9: PerformanceLayer complete
- [ ] Sprint 10: TouchFocusManager complete
- [ ] Sprint 11: AtomicPresetLoader complete
- [ ] Sprint 12: Full integration complete
- [ ] All functional requirements met
- [ ] All performance requirements met

### Phase 4: Validation
- [ ] Sprint 13: User testing complete
- [ ] Sprint 14: Refinements applied
- [ ] Sprint 15: Documentation complete
- [ ] Sprint 16: Production ready
- [ ] All success criteria met
- [ ] Release approved

---

## Conclusion

The Trinity Control Stack v0.5 represents a significant architectural upgrade to Chimera Phoenix. While the scope is substantial, the phased approach with clear sprints and dependencies provides a realistic path to completion.

**Key Takeaways:**
1. **Foundation First:** Build ParameterRegistry, ABState, EventBus early
2. **Incremental Integration:** Test components as they're built, don't wait
3. **Performance Critical:** Lock-free cache and morph interpolator need extra attention
4. **User Experience:** Mode transitions and encoder feel require tuning
5. **Timeline Realistic:** 12-16 weeks with experienced developer(s)

**Next Steps:**
1. Review and approve this gap analysis
2. Allocate development resources
3. Begin Sprint 1: ParameterRegistry
4. Set up continuous integration for automated testing
5. Establish weekly progress reviews

**Questions for Stakeholders:**
1. Is 12-16 week timeline acceptable?
2. Priority of nice-to-have features (undo/redo, advanced macros)?
3. Level of testing rigor (80% coverage vs 95%+)?
4. UI design for mode indication and morph display?
5. Trinity AI API changes needed for integration?

---

**Document Status:** Draft for Review
**Author:** Trinity Control Stack Planning Team
**Date:** 2025-10-23
**Version:** 1.0
