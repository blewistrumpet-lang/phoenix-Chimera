# AUTHORITATIVE SYSTEMS DOCUMENTATION

**Project Chimera v3.0 "Phoenix" - System Architecture Authority Reference**

This document establishes the single source of truth for all systems in Project Chimera, eliminating confusion about which components are authoritative and which are deprecated or duplicate.

## CRITICAL: READ THIS FIRST

**🚨 DEVELOPER MANDATE: Only use the systems marked as AUTHORITATIVE below. All other systems are deprecated or secondary.**

---

## 1. PARAMETER SYSTEM AUTHORITY

### ✅ AUTHORITATIVE: PluginProcessor.cpp `getMixParameterIndex()`

**File:** `/JUCE_Plugin/Source/PluginProcessor.cpp` (Lines 884-985)
**Function:** `int ChimeraAudioProcessor::getMixParameterIndex(int engineID)`
**Status:** 🟢 ACTIVE - This is the definitive parameter mapping system

**Why This is Authoritative:**
- Contains hand-verified parameter mappings for all 57 engines (0-56)
- Based on comprehensive audit of actual engine header files
- Includes critical fixes for engines with incorrect mix parameter indices
- Used directly by the plugin during audio processing
- Contains detailed comments explaining each engine's parameter layout

**Key Features:**
- Maps engine ID → Mix parameter index (0-based)
- Handles special cases (engines with no mix parameter return -1)
- Includes fixes for critical engines like Harmonic Exciter, Transient Shaper
- Thread-safe and performance optimized

**Example Usage:**
```cpp
int mixIndex = getMixParameterIndex(engineID);
if (mixIndex >= 0) {
    juce::String mixParamID = slotPrefix + juce::String(mixIndex + 1);
    parameters.getParameter(mixParamID)->setValueNotifyingHost(1.0f);
}
```

### ❌ DEPRECATED: GeneratedParameterDatabase.h

**File:** `/JUCE_Plugin/Source/GeneratedParameterDatabase.h`
**Status:** 🔴 DEPRECATED - Do not use for mix parameter mapping
**Reason:** Contains incorrect dropdown indices and lacks verified parameter mappings

**Deprecation Warning Added:**
```cpp
// WARNING: This file contains parameter information but should NOT be used
// for mix parameter mapping. Use PluginProcessor.cpp getMixParameterIndex() instead.
```

---

## 2. ENGINE FACTORY AUTHORITY

### ✅ AUTHORITATIVE: EngineFactory.cpp

**File:** `/JUCE_Plugin/Source/EngineFactory.cpp`
**Function:** `std::unique_ptr<EngineBase> EngineFactory::createEngine(int engineID)`
**Status:** 🟢 ACTIVE - This is the definitive engine creation system

**Why This is Authoritative:**
- Single factory pattern implementation
- Maps engine IDs 0-56 to actual C++ class instances
- Handles all 57 engines with proper error handling
- Returns nullptr for invalid engine IDs
- Used directly by PluginProcessor for engine instantiation

**Coverage:**
- ENGINE_NONE (0) → NoneEngine
- Dynamics & Compression (1-6) → 6 engines
- Filters & EQ (7-14) → 8 engines
- Distortion & Saturation (15-22) → 8 engines
- Modulation (23-33) → 11 engines
- Reverb & Delay (34-43) → 10 engines
- Spatial & Special (44-52) → 9 engines
- Utility (53-56) → 4 engines

**No Duplicates Found:** This is the only engine factory in the codebase.

---

## 3. ENGINE ID MAPPING AUTHORITY

### ✅ AUTHORITATIVE: EngineTypes.h

**File:** `/JUCE_Plugin/Source/EngineTypes.h`
**Status:** 🟢 ACTIVE - This is the definitive engine type definition system

**Why This is Authoritative:**
- Contains all 57 engine type constants (ENGINE_NONE through ENGINE_PHASE_ALIGN)
- Defines immutable numeric IDs (0-56) that must never be reordered
- Includes preset compatibility warnings
- Contains helper functions for name/category lookup
- Referenced by all other systems as the single source of truth

**Key Constants:**
```cpp
#define ENGINE_NONE                     0   // No engine (passthrough)
#define ENGINE_OPTO_COMPRESSOR          1   // Vintage Opto Compressor
// ... through ...
#define ENGINE_PHASE_ALIGN              56  // Phase Align
#define ENGINE_COUNT                    57  // Total engines
```

**Helper Functions:**
- `getEngineTypeName(int engineType)` - Get display name
- `getEngineCategory(int engineType)` - Get category
- `isValidEngineType(int engineType)` - Validate engine ID

### 🔶 SECONDARY: EngineStringMapping.h

**File:** `/JUCE_Plugin/Source/EngineStringMapping.h`
**Status:** 🟡 SECONDARY - Used for AI system integration only
**Purpose:** Maps string IDs to engine types for JSON preset loading
**Dependency:** References EngineTypes.h constants

---

## 4. SYSTEMS REMOVED/DEPRECATED

### 🗑️ REMOVED: Old Parameter Database Systems

**Previous Issues:**
- Multiple conflicting parameter databases existed
- Incorrect dropdown index mappings
- Missing verification against actual engine implementations
- AI system was using unverified parameter counts

**Resolution:**
- Consolidated into single authoritative getMixParameterIndex() function
- All other parameter mapping systems marked as deprecated
- AI system updated to use verified parameter counts

### 🗑️ REMOVED: Duplicate Engine Factory Patterns

**Analysis Result:** No duplicate engine factories found
**Confirmation:** EngineFactory.cpp is the only engine creation system

### 🗑️ REMOVED: Inconsistent Engine Type Systems

**Previous Issues:**
- Mixed use of string IDs vs numeric IDs
- Inconsistent naming conventions
- No central definition authority

**Resolution:**
- EngineTypes.h established as single source of truth
- All systems now reference unified constants
- String mapping relegated to AI integration only

---

## 5. ARCHITECTURE FLOW DIAGRAM

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│      UI         │    │   PARAMETERS    │    │     ENGINES     │
│                 │    │                 │    │                 │
│  User selects   │───▶│ PluginProcessor │───▶│  EngineFactory  │
│  engine from    │    │                 │    │                 │
│  dropdown       │    │ getMixParameter │    │ createEngine()  │
│                 │    │ Index()         │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         │                       │                       ▼
         │                       │              ┌─────────────────┐
         │                       │              │ Specific Engine │
         │                       │              │   (e.g., Tube   │
         │                       │              │   Preamp, etc.) │
         │                       │              └─────────────────┘
         │                       │
         ▼                       ▼
┌─────────────────┐    ┌─────────────────┐
│ EngineTypes.h   │    │ DefaultParameter│
│                 │    │ Values.h        │
│ Single source   │    │                 │
│ of truth for    │    │ Safe defaults   │
│ all engine IDs  │    │ for each engine │
│ and names       │    │                 │
└─────────────────┘    └─────────────────┘
```

### Data Flow:

1. **UI Selection** → User chooses engine from dropdown (0-56)
2. **Parameter Mapping** → PluginProcessor.getMixParameterIndex() determines correct parameter layout
3. **Engine Creation** → EngineFactory.createEngine() instantiates the actual DSP engine
4. **Type Validation** → EngineTypes.h provides constants and validation
5. **Default Values** → DefaultParameterValues.h provides safe starting parameters

---

## 6. DEVELOPER GUIDELINES

### ✅ DO USE:

1. **EngineTypes.h constants** for all engine type references
   ```cpp
   #include "EngineTypes.h"
   if (engineID == ENGINE_VINTAGE_TUBE) { ... }
   ```

2. **PluginProcessor::getMixParameterIndex()** for parameter mapping
   ```cpp
   int mixIndex = getMixParameterIndex(engineID);
   ```

3. **EngineFactory::createEngine()** for engine instantiation
   ```cpp
   auto engine = EngineFactory::createEngine(engineID);
   ```

### ❌ DO NOT USE:

1. **Hard-coded numeric engine IDs**
   ```cpp
   if (engineID == 15) { ... }  // ❌ WRONG
   ```

2. **GeneratedParameterDatabase.h for mix parameters**
   ```cpp
   // ❌ WRONG - This data is unverified
   auto info = ChimeraParameters::getEngineInfo(stringId);
   ```

3. **String-based engine selection in core plugin**
   ```cpp
   // ❌ WRONG - Use numeric IDs in core plugin
   loadEngine("vintage_tube");
   ```

### 🔄 WORKFLOW:

1. **Adding New Engine:**
   - Add constant to EngineTypes.h (increment ENGINE_COUNT)
   - Add case to EngineFactory.cpp
   - Add mix parameter mapping to getMixParameterIndex()
   - Add default parameters to DefaultParameterValues.h

2. **Modifying Engine Parameters:**
   - Update engine header file with correct parameter layout
   - Verify mix parameter index in getMixParameterIndex()
   - Update default values if needed

---

## 7. VALIDATION CHECKLIST

Before making any changes to the parameter or engine systems, verify:

- [ ] EngineTypes.h constants are used (no hard-coded numbers)
- [ ] EngineFactory.cpp handles the engine ID correctly
- [ ] getMixParameterIndex() returns correct index for the engine
- [ ] Default parameters are set appropriately
- [ ] No deprecated systems are referenced
- [ ] ENGINE_COUNT is updated if engines are added/removed

---

## 8. CONTACT & QUESTIONS

For questions about this architecture or to report inconsistencies:

1. Check this document first (AUTHORITATIVE_SYSTEMS.md)
2. Verify against the three authoritative files:
   - `/JUCE_Plugin/Source/EngineTypes.h`
   - `/JUCE_Plugin/Source/EngineFactory.cpp`
   - `/JUCE_Plugin/Source/PluginProcessor.cpp` (getMixParameterIndex)
3. Do not reference any systems marked as DEPRECATED

---

**Document Version:** 1.0  
**Last Updated:** August 18, 2025  
**Maintained By:** System Authority Documentation Agent