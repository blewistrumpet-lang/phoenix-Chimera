# Unified Default Parameter System Implementation Plan

## Overview

This plan outlines the step-by-step process to replace the current fragmented default parameter system with the new unified `UnifiedDefaultParameters` system. The goal is to provide all 57 engines with musically optimized defaults while maintaining backward compatibility and system stability.

## Current State Assessment

### Files to Modify
- `Source/PluginProcessor.cpp` - Replace hardcoded switch statements
- `Source/PluginProcessor.h` - Update method signatures if needed

### Files to Deprecate/Remove
- `Source/EngineDefaults.h` - Not currently used, can be removed
- `Source/GeneratedDefaultParameterValues.cpp` - Incomplete coverage, should be deprecated

### Files to Keep
- `Source/DefaultParameterValues.cpp` - Keep as reference, but replace functionality

## Implementation Steps

### Phase 1: Integration Preparation

#### Step 1.1: Add UnifiedDefaultParameters to Build System
```cpp
// Add to CMakeLists.txt or project files:
// Source/UnifiedDefaultParameters.h
// Source/UnifiedDefaultParameters.cpp
```

#### Step 1.2: Update PluginProcessor.h Includes
```cpp
// In PluginProcessor.h, replace:
// #include "DefaultParameterValues.h"
// With:
#include "UnifiedDefaultParameters.h"
```

#### Step 1.3: Test Compilation
Ensure the new files compile correctly with the existing codebase before making functional changes.

### Phase 2: Core Integration

#### Step 2.1: Replace applyDefaultParameters Method

**Current Implementation Location**: Lines 455-600+ in PluginProcessor.cpp

**Current Method Signature**:
```cpp
void ChimeraAudioProcessor::applyDefaultParameters(int slot, int engineID)
```

**New Implementation**:
```cpp
void ChimeraAudioProcessor::applyDefaultParameters(int slot, int engineID) {
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
    
    // Get unified defaults for this engine
    auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineID);
    
    // Initialize all parameters to safe center values first
    for (int i = 1; i <= 15; ++i) {
        auto paramID = slotPrefix + juce::String(i);
        if (auto* param = parameters.getParameter(paramID)) {
            param->setValueNotifyingHost(0.5f); // Safe fallback
        }
    }
    
    // Apply engine-specific optimized defaults
    for (const auto& [paramIndex, value] : defaults) {
        auto paramID = slotPrefix + juce::String(paramIndex + 1); // Convert 0-based to 1-based
        if (auto* param = parameters.getParameter(paramID)) {
            param->setValueNotifyingHost(value);
        }
    }
    
    DBG("Applied unified defaults for engine " + juce::String(engineID) + 
        " (" + juce::String(defaults.size()) + " parameters)");
}
```

#### Step 2.2: Remove Hardcoded Switch Statement

**Lines to Remove**: ~470-600 in PluginProcessor.cpp
- The entire switch statement with hardcoded engine defaults
- All individual engine case blocks
- Mix parameter index calculations (now handled by UnifiedDefaultParameters::getMixParameterIndex)

#### Step 2.3: Update getMixParameterIndex Method

**Current Location**: Search for getMixParameterIndex in PluginProcessor.cpp

**New Implementation**:
```cpp
int ChimeraAudioProcessor::getMixParameterIndex(int engineID) {
    return UnifiedDefaultParameters::getMixParameterIndex(engineID);
}
```

### Phase 3: Enhanced Integration

#### Step 3.1: Add Parameter Validation

Add validation to ensure defaults are applied correctly:

```cpp
void ChimeraAudioProcessor::validateDefaultParameters(int slot, int engineID) {
    // Validate that all expected parameters received defaults
    auto expectedDefaults = UnifiedDefaultParameters::getDefaultParameters(engineID);
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
    
    for (const auto& [paramIndex, expectedValue] : expectedDefaults) {
        auto paramID = slotPrefix + juce::String(paramIndex + 1);
        if (auto* param = parameters.getParameter(paramID)) {
            float actualValue = param->getValue();
            if (std::abs(actualValue - expectedValue) > 0.001f) {
                DBG("Warning: Parameter " + paramID + " default mismatch. Expected: " + 
                    juce::String(expectedValue) + ", Actual: " + juce::String(actualValue));
            }
        } else {
            DBG("Warning: Parameter " + paramID + " not found for engine " + juce::String(engineID));
        }
    }
}
```

#### Step 3.2: Add Default Information API

Add methods to PluginProcessor for accessing default information:

```cpp
// In PluginProcessor.h:
public:
    std::map<int, float> getEngineDefaults(int engineID);
    std::string getDefaultParameterName(int engineID, int paramIndex);
    int getDefaultParameterCount(int engineID);

// In PluginProcessor.cpp:
std::map<int, float> ChimeraAudioProcessor::getEngineDefaults(int engineID) {
    return UnifiedDefaultParameters::getDefaultParameters(engineID);
}

std::string ChimeraAudioProcessor::getDefaultParameterName(int engineID, int paramIndex) {
    return UnifiedDefaultParameters::getParameterName(engineID, paramIndex);
}

int ChimeraAudioProcessor::getDefaultParameterCount(int engineID) {
    return UnifiedDefaultParameters::getParameterCount(engineID);
}
```

### Phase 4: Testing and Validation

#### Step 4.1: Create Test Program

Create a comprehensive test to validate the unified system:

```cpp
// File: Source/TestUnifiedDefaults.cpp
#include "UnifiedDefaultParameters.h"
#include "EngineTypes.h"
#include <iostream>

int main() {
    std::cout << "=== Unified Default Parameters Test ===" << std::endl;
    
    int totalEngines = 0;
    int enginesWithDefaults = 0;
    int totalParameters = 0;
    
    // Test all engines
    for (int engineId = ENGINE_NONE; engineId < ENGINE_COUNT; ++engineId) {
        totalEngines++;
        
        auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
        if (!defaults.empty()) {
            enginesWithDefaults++;
            totalParameters += defaults.size();
            
            std::cout << "Engine " << engineId << " (" << getEngineTypeName(engineId) << "): " 
                      << defaults.size() << " parameters" << std::endl;
            
            // Validate all values are in range
            for (const auto& [index, value] : defaults) {
                if (value < 0.0f || value > 1.0f) {
                    std::cout << "ERROR: Engine " << engineId << " parameter " << index 
                              << " out of range: " << value << std::endl;
                }
            }
        } else if (engineId != ENGINE_NONE) {
            std::cout << "WARNING: Engine " << engineId << " (" << getEngineTypeName(engineId) 
                      << ") has no defaults" << std::endl;
        }
    }
    
    // Coverage report
    std::cout << "\n=== Coverage Report ===" << std::endl;
    std::cout << "Total engines: " << totalEngines << std::endl;
    std::cout << "Engines with defaults: " << enginesWithDefaults << std::endl;
    std::cout << "Coverage: " << (100.0f * enginesWithDefaults / totalEngines) << "%" << std::endl;
    std::cout << "Total parameters: " << totalParameters << std::endl;
    
    // Test validation function
    std::cout << "\n=== Validation Test ===" << std::endl;
    for (int engineId = ENGINE_NONE; engineId < ENGINE_COUNT; ++engineId) {
        if (!UnifiedDefaultParameters::validateEngineDefaults(engineId)) {
            std::cout << "ERROR: Engine " << engineId << " failed validation" << std::endl;
        }
    }
    
    std::cout << "Test completed." << std::endl;
    return 0;
}
```

#### Step 4.2: Build and Run Test

```bash
cd /path/to/plugin
g++ -I"./Source" -std=c++17 Source/UnifiedDefaultParameters.cpp Source/TestUnifiedDefaults.cpp -o test_unified_defaults
./test_unified_defaults
```

Expected Output:
- 100% coverage (57/57 engines including ENGINE_NONE)
- All parameters in valid range [0.0, 1.0]
- No validation failures

#### Step 4.3: Integration Testing

Test the integration with the actual plugin:

1. **Load Engine Test**: Load each engine and verify defaults are applied
2. **Audio Safety Test**: Process audio with default settings - no clipping or artifacts
3. **Parameter Range Test**: Verify all parameters are within expected ranges
4. **Mix Parameter Test**: Verify mix parameters are correctly identified and set
5. **Performance Test**: Ensure default loading doesn't impact audio performance

### Phase 5: Clean Up and Documentation

#### Step 5.1: Remove Deprecated Files

Once the unified system is tested and working:

```bash
# Remove deprecated files:
rm Source/EngineDefaults.h
rm Source/GeneratedDefaultParameterValues.cpp

# Update any references in:
# - CMakeLists.txt
# - Project files  
# - Include statements
```

#### Step 5.2: Update Documentation

Update existing documentation files:

- `DefaultParameterSystemGuide.txt` - Point to new unified system
- Update any code comments referencing old default system
- Update API documentation

#### Step 5.3: Add Usage Documentation

Create usage examples for the new system:

```cpp
// Example: Get defaults for a specific engine
auto defaults = UnifiedDefaultParameters::getDefaultParameters(ENGINE_K_STYLE);

// Example: Get engine information with metadata
auto engineInfo = UnifiedDefaultParameters::getEngineDefaults(ENGINE_PLATE_REVERB);

// Example: Get engines by category
auto categories = UnifiedDefaultParameters::getEnginesByCategory();
for (const auto& reverbs : categories[UnifiedDefaultParameters::EngineCategory::REVERB]) {
    // Process reverb engines...
}
```

## Risk Mitigation

### Backward Compatibility
- Keep old DefaultParameterValues.cpp temporarily for reference
- Test thoroughly with existing presets
- Provide migration path if needed

### Performance Considerations
- Default loading happens only during engine switching (not audio processing)
- Minimal memory footprint for default storage
- Fast lookup using std::map structures

### Testing Strategy
- Unit tests for individual engine defaults
- Integration tests with actual plugin loading
- Audio safety tests with all default combinations
- Performance benchmarks for default application speed

## Success Criteria

### Functional Success
- ✅ All 57 engines have appropriate defaults
- ✅ No audio artifacts or clipping with any default combination
- ✅ Immediate musical satisfaction on engine load
- ✅ Consistent behavior across similar engine types

### Technical Success
- ✅ 100% test coverage for all engines
- ✅ No performance degradation during engine switching
- ✅ Clean, maintainable code structure
- ✅ Comprehensive documentation

### User Experience Success
- ✅ Users can create music immediately without parameter tweaking
- ✅ Professional-sounding defaults suitable for production use
- ✅ Consistent and predictable behavior
- ✅ Educational value showing proper parameter relationships

## Timeline

**Week 1**: Implementation (Phases 1-2)
- Integration preparation and core replacement

**Week 2**: Testing and Validation (Phases 3-4)  
- Comprehensive testing and validation

**Week 3**: Clean Up and Documentation (Phase 5)
- Remove deprecated code and update documentation

**Week 4**: Final Testing and Release Preparation
- Final validation and preparation for deployment

This implementation plan ensures a smooth transition to the unified default parameter system while maintaining the stability and reliability of the Chimera Phoenix plugin.