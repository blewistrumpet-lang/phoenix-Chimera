# Engine System Sources of Truth - Chimera Phoenix v3.0

**Quick Reference Guide for Developers**  
**Last Updated:** August 19, 2025  
**Purpose:** Definitive authority hierarchy for all engine system components  

## 🚨 Critical Warning

**DO NOT** use deprecated or outdated files. This document defines the authoritative sources for all engine system components. Using incorrect files will lead to:
- Broken builds
- Parameter mismatches
- Runtime crashes
- Preset incompatibilities

## 📋 Quick Authority Reference

| Component | Authoritative File | Purpose | Status |
|-----------|-------------------|---------|---------|
| **Engine Types** | `Source/EngineTypes.h` | All engine ID definitions | ✅ ACTIVE |
| **Engine Creation** | `Source/EngineFactory.cpp` | Engine instantiation | ✅ ACTIVE |
| **Engine Interface** | `Source/EngineBase.h` | Base class for all engines | ✅ ACTIVE |
| **Slot Configuration** | `Source/SlotConfiguration.h` | Slot count and management | ✅ ACTIVE |
| **Parameter System** | `Source/ParameterDefinitions.h` | Parameter ID mapping | ✅ ACTIVE |
| **Default Parameters** | `Source/UnifiedDefaultParameters.h` | Default parameter values | ✅ ACTIVE |
| **System Brain** | `Source/PluginProcessor.h/cpp` | Main system coordinator | ✅ ACTIVE |

## 🏗️ System Architecture Authority

### 1. Engine Type Definitions
**SOURCE OF TRUTH:** `Source/EngineTypes.h`

```cpp
// ENGINE_NONE = 0, ENGINE_OPTO_COMPRESSOR = 1, etc.
#define ENGINE_COUNT 57  // Total engines (0-56)
```

**What it defines:**
- All 57 engine type constants
- Engine categories and organization
- Helper functions for name/category lookup
- Backward compatibility mappings

**Usage:**
```cpp
#include "EngineTypes.h"
auto name = getEngineTypeName(ENGINE_TAPE_ECHO);  // "Tape Echo"
bool valid = isValidEngineType(engineID);
```

### 2. Engine Creation Authority
**SOURCE OF TRUTH:** `Source/EngineFactory.cpp`

**What it defines:**
- How to create each of the 57 engines
- Engine ID to implementation mapping
- Include statements for all engine headers

**Usage:**
```cpp
#include "EngineFactory.h"
auto engine = EngineFactory::createEngine(ENGINE_TAPE_ECHO);
```

### 3. Engine Interface Standard
**SOURCE OF TRUTH:** `Source/EngineBase.h`

**What it defines:**
- Abstract base class for all engines
- Core processing interface (prepareToPlay, process, reset)
- Parameter update interface
- Extended features (latency, bypass, quality settings)

**Usage:**
```cpp
#include "EngineBase.h"
class MyEngine : public EngineBase {
    void process(juce::AudioBuffer<float>& buffer) override;
    // ... implement required methods
};
```

### 4. Slot Architecture Authority
**SOURCE OF TRUTH:** `Source/SlotConfiguration.h`

**What it defines:**
- Current slot count: 6 slots
- Maximum supported: 8 slots
- Slot validation functions
- CPU monitoring thresholds

**Usage:**
```cpp
#include "SlotConfiguration.h"
static_assert(CHIMERA_NUM_SLOTS == 6);
bool valid = ChimeraConfig::isValidSlot(slotIndex);
```

### 5. Parameter System Authority
**SOURCE OF TRUTH:** `Source/ParameterDefinitions.h`

**What it defines:**
- Parameter ID enums for all 96 parameters
- Slot-to-parameter mapping
- Engine parameter organization

**Usage:**
```cpp
#include "ParameterDefinitions.h"
// Access slot 1, parameter 3
auto paramID = SLOT_1_PARAM_3;
```

### 6. Default Parameters Authority
**SOURCE OF TRUTH:** `Source/UnifiedDefaultParameters.h`

**What it defines:**
- Default values for all 57 engines
- Parameter validation and metadata
- Engine category organization for defaults
- Musical optimization principles

**Usage:**
```cpp
#include "UnifiedDefaultParameters.h"
auto defaults = UnifiedDefaultParameters::getEngineDefaults(ENGINE_TAPE_ECHO);
```

## 🔧 Implementation Files Authority

### Individual Engine Files
**PATTERN:** `Source/[EngineName].h` and `Source/[EngineName].cpp`

Each engine has its own implementation files:
```
Source/TapeEcho.h         ✅ ACTIVE
Source/TapeEcho.cpp       ✅ ACTIVE
Source/PlateReverb.h      ✅ ACTIVE
Source/PlateReverb.cpp    ✅ ACTIVE
// ... 57 total engines
```

**Authority Rules:**
- Each engine header defines the engine class interface
- Each engine cpp file provides the complete implementation
- All engines inherit from `EngineBase`
- All engines must be included in `EngineFactory.cpp`

## 🚨 Deprecated Files to AVOID

### Legacy Engine Lists (DO NOT USE)
- `SimplifiedEngineMapping.h` ❌ DEPRECATED
- Any hardcoded engine arrays in test files ❌ DEPRECATED
- Old engine enum definitions ❌ DEPRECATED

### Legacy Parameter Systems (DO NOT USE)
- `EngineDefaults.h` ❌ DEPRECATED (replaced by UnifiedDefaultParameters)
- Hardcoded default arrays ❌ DEPRECATED
- Individual engine default constants ❌ DEPRECATED

### Obsolete Test Mappings (DO NOT USE)
- Test-specific engine lists ❌ DEPRECATED
- Standalone engine arrays ❌ DEPRECATED
- Local engine mappings in individual files ❌ DEPRECATED

## 🎯 Development Workflow Authority

### Adding a New Engine
1. **Define engine type** in `EngineTypes.h`
2. **Create engine files** `Source/NewEngine.h/cpp`
3. **Add to factory** in `EngineFactory.cpp`
4. **Add defaults** in `UnifiedDefaultParameters.cpp`
5. **Update engine count** in `EngineTypes.h`

### Modifying Engine Parameters
1. **Check interface** in `EngineBase.h`
2. **Update engine implementation** in `Source/[Engine].cpp`
3. **Verify defaults** in `UnifiedDefaultParameters.cpp`
4. **Test parameter mapping** via `ParameterDefinitions.h`

### System Configuration Changes
1. **Slot changes:** Modify `SlotConfiguration.h`
2. **Parameter changes:** Update `ParameterDefinitions.h`
3. **Engine changes:** Update `EngineTypes.h` and `EngineFactory.cpp`

## 📚 Documentation Authority

### System Documentation
- `SYSTEM_ARCHITECTURE.md` - Complete system overview
- `ENGINE_SYSTEM_SOURCES_OF_TRUTH.md` - This file (quick reference)
- `COMPLETE_ENGINE_DOCUMENTATION.md` - Engine status and details

### Specialized Documentation
- `SLOT_ARCHITECTURE.md` - Slot system details
- `UNIFIED_DEFAULT_PARAMETERS_*.md` - Default parameter system docs
- Individual `*_REPORT.md` files - Validation and testing reports

## 🔍 Validation and Testing Authority

### Engine Validation
- **Test harness:** Use `EngineTestHarness.cpp`
- **Validation:** Use diagnostic methods in `PluginProcessor`
- **Quality metrics:** Follow `PRODUCTION_READINESS_CERTIFICATION.md`

### Parameter Validation
- **Defaults testing:** Use `TestUnifiedDefaults.cpp`
- **Range validation:** Built into `UnifiedDefaultParameters` system
- **Integration testing:** Use comprehensive test suite

## 🚀 Production Authority

### Build Configuration
- **CMake:** Use `CMakeLists.txt`
- **JUCE Project:** Use `ChimeraPhoenix.jucer`
- **Dependencies:** All dependencies defined in JUCE project

### Quality Assurance
- **Status:** 98.2% operational (56/57 engines)
- **Certification:** See `PRODUCTION_READINESS_CERTIFICATION.md`
- **Validation:** See `FINAL_SYSTEM_VALIDATION_REPORT.md`

## 📞 When in Doubt

1. **Check this file first** - This is the authoritative guide
2. **Refer to system architecture** - `SYSTEM_ARCHITECTURE.md`
3. **Check production status** - `PRODUCTION_READINESS_CERTIFICATION.md`
4. **Validate with tests** - Run comprehensive validation suite

## ⚡ Emergency Reference

### Quick File Checklist
```bash
# Essential files that must exist and be current:
Source/EngineTypes.h              ✅ Engine definitions
Source/EngineFactory.cpp          ✅ Engine creation
Source/EngineBase.h               ✅ Engine interface
Source/SlotConfiguration.h        ✅ Slot management
Source/ParameterDefinitions.h     ✅ Parameter mapping
Source/UnifiedDefaultParameters.h ✅ Default values
Source/PluginProcessor.h/cpp      ✅ System coordinator
```

### Quick Validation
```bash
# Verify system integrity:
grep "ENGINE_COUNT" Source/EngineTypes.h     # Should show 57
wc -l Source/EngineFactory.cpp              # Should have all engines
ls Source/*Engine*.h | wc -l                # Should match engine count
```

---

**Remember:** This document defines the authoritative sources for all engine system components. Using any other files or patterns will lead to system inconsistencies and failures. When developing, always refer to these sources of truth.