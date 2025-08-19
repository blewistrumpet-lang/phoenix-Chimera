# UnifiedDefaultParameters API Documentation

**Version:** v3.0 Phoenix  
**API Version:** 1.0  
**Documentation Date:** August 19, 2025  
**Documentation Officer:** Documentation Sync Agent  

## Overview

The **UnifiedDefaultParameters** system provides a comprehensive API for accessing professionally crafted default parameter values for all 57 engines in Chimera Phoenix. This system replaces the fragmented legacy approaches with a single, authoritative source that delivers immediate musical satisfaction.

## Core API Reference

### Class: UnifiedDefaultParameters

The main API class providing static methods for accessing default parameter values.

```cpp
#include "UnifiedDefaultParameters.h"

class UnifiedDefaultParameters {
public:
    // Core value retrieval
    static float getDefaultValue(int engineId, int parameterIndex);
    static bool hasDefaults(int engineId);
    static int getParameterCount(int engineId);
    
    // Bulk operations
    static std::vector<float> getDefaultParameters(int engineId);
    static void applyDefaults(int engineId, std::function<void(int, float)> setValue);
    
    // Validation
    static bool isValidEngine(int engineId);
    static bool isValidParameter(int engineId, int parameterIndex);
};
```

## Method Documentation

### getDefaultValue()

Returns the default value for a specific parameter of a specific engine.

```cpp
static float getDefaultValue(int engineId, int parameterIndex);
```

**Parameters:**
- `engineId`: Engine identifier (0-56)
- `parameterIndex`: Parameter index (0-based)

**Returns:**
- `float`: Default value in range [0.0, 1.0]
- Returns `0.5f` for invalid engine/parameter combinations

**Example:**
```cpp
// Get default mix value for VCA Compressor (engine 1, parameter 5)
float defaultMix = UnifiedDefaultParameters::getDefaultValue(1, 5);
// Returns: 1.0f (100% mix)

// Get default frequency for Ladder Filter (engine 8, parameter 0)
float defaultFreq = UnifiedDefaultParameters::getDefaultValue(8, 0);
// Returns: 0.2f (1000Hz when mapped)
```

### hasDefaults()

Checks if an engine has default parameters defined in the system.

```cpp
static bool hasDefaults(int engineId);
```

**Parameters:**
- `engineId`: Engine identifier (0-56)

**Returns:**
- `bool`: `true` if engine has defaults, `false` otherwise

**Example:**
```cpp
// Check if VCA Compressor has defaults
bool hasDefaults = UnifiedDefaultParameters::hasDefaults(1);
// Returns: true

// Check invalid engine
bool hasDefaults = UnifiedDefaultParameters::hasDefaults(999);
// Returns: false
```

### getParameterCount()

Returns the expected number of parameters for a specific engine.

```cpp
static int getParameterCount(int engineId);
```

**Parameters:**
- `engineId`: Engine identifier (0-56)

**Returns:**
- `int`: Number of parameters for the engine
- Returns `0` for invalid engines

**Example:**
```cpp
// Get parameter count for VCA Compressor
int paramCount = UnifiedDefaultParameters::getParameterCount(1);
// Returns: 6

// Get parameter count for Digital Chorus
int paramCount = UnifiedDefaultParameters::getParameterCount(24);
// Returns: 5
```

### getDefaultParameters()

Returns all default parameter values for an engine as a vector.

```cpp
static std::vector<float> getDefaultParameters(int engineId);
```

**Parameters:**
- `engineId`: Engine identifier (0-56)

**Returns:**
- `std::vector<float>`: Vector of default values
- Returns empty vector for invalid engines

**Example:**
```cpp
// Get all defaults for VCA Compressor
auto defaults = UnifiedDefaultParameters::getDefaultParameters(1);
// Returns: {0.3f, 0.5f, 0.4f, 0.6f, 0.3f, 1.0f}
//          ratio, attack, release, threshold, knee, mix

// Apply to parameters
for (int i = 0; i < defaults.size(); ++i) {
    setParameter(i, defaults[i]);
}
```

### applyDefaults()

Applies default values using a callback function for parameter setting.

```cpp
static void applyDefaults(int engineId, std::function<void(int, float)> setValue);
```

**Parameters:**
- `engineId`: Engine identifier (0-56)
- `setValue`: Callback function taking (parameterIndex, value)

**Example:**
```cpp
// Apply defaults to JUCE AudioProcessor
UnifiedDefaultParameters::applyDefaults(engineId, 
    [this](int paramIndex, float value) {
        setParameterNotifyingHost(paramIndex, value);
    });

// Apply defaults to custom parameter manager
UnifiedDefaultParameters::applyDefaults(engineId,
    [&manager](int paramIndex, float value) {
        manager.setParameter(paramIndex, value);
    });
```

### isValidEngine()

Validates if an engine ID is within the supported range.

```cpp
static bool isValidEngine(int engineId);
```

**Parameters:**
- `engineId`: Engine identifier to validate

**Returns:**
- `bool`: `true` if engine ID is valid (0-56), `false` otherwise

**Example:**
```cpp
bool valid = UnifiedDefaultParameters::isValidEngine(25);  // true
bool invalid = UnifiedDefaultParameters::isValidEngine(999);  // false
```

### isValidParameter()

Validates if a parameter index is valid for a specific engine.

```cpp
static bool isValidParameter(int engineId, int parameterIndex);
```

**Parameters:**
- `engineId`: Engine identifier (0-56)
- `parameterIndex`: Parameter index to validate

**Returns:**
- `bool`: `true` if parameter is valid for the engine, `false` otherwise

**Example:**
```cpp
// Valid parameter for VCA Compressor (6 parameters: 0-5)
bool valid = UnifiedDefaultParameters::isValidParameter(1, 3);  // true

// Invalid parameter index
bool invalid = UnifiedDefaultParameters::isValidParameter(1, 10);  // false
```

## Engine Categories and Patterns

### Category-Based Default Patterns

The API organizes engines into 12 categories, each with consistent default patterns:

#### Dynamics & Compression (Engines 1-6)
```cpp
// Common pattern: 100% mix, musical ratios, transparent settings
float mix = UnifiedDefaultParameters::getDefaultValue(1, 5);     // 1.0f (100%)
float ratio = UnifiedDefaultParameters::getDefaultValue(1, 0);   // 0.3f (3:1 ratio)
```

#### Filters & EQ (Engines 7-14)
```cpp
// Common pattern: Variable mix, midrange cutoff, musical resonance
float freq = UnifiedDefaultParameters::getDefaultValue(8, 0);    // 0.2f (~1000Hz)
float res = UnifiedDefaultParameters::getDefaultValue(8, 1);     // 0.3f (musical resonance)
float mix = UnifiedDefaultParameters::getDefaultValue(8, 4);     // 0.8f (80% mix)
```

#### Distortion & Saturation (Engines 15-22)
```cpp
// Common pattern: High mix, conservative drive levels
float drive = UnifiedDefaultParameters::getDefaultValue(15, 0);  // 0.25f (25% drive)
float mix = UnifiedDefaultParameters::getDefaultValue(15, 3);    // 1.0f (100% mix)
```

#### Modulation Effects (Engines 23-33)
```cpp
// Common pattern: 30-50% mix, 2-5Hz rates, subtle movement
float rate = UnifiedDefaultParameters::getDefaultValue(24, 0);   // 0.3f (~3Hz)
float depth = UnifiedDefaultParameters::getDefaultValue(24, 1);  // 0.4f (medium depth)
float mix = UnifiedDefaultParameters::getDefaultValue(24, 4);    // 0.4f (40% mix)
```

#### Reverb & Delay (Engines 34-43)
```cpp
// Common pattern: 25-35% mix, medium decay/timing
float decay = UnifiedDefaultParameters::getDefaultValue(34, 1);  // 0.5f (medium decay)
float mix = UnifiedDefaultParameters::getDefaultValue(34, 4);    // 0.3f (30% mix)
```

#### Spatial & Special Effects (Engines 44-52)
```cpp
// Common pattern: Conservative mix, balanced processing
float width = UnifiedDefaultParameters::getDefaultValue(44, 0);  // 0.6f (60% width)
float mix = UnifiedDefaultParameters::getDefaultValue(44, 2);    // 0.25f (25% mix)
```

#### Utility (Engines 53-56)
```cpp
// Common pattern: 100% mix, unity gain, neutral settings
float gain = UnifiedDefaultParameters::getDefaultValue(53, 0);   // 0.5f (0dB/unity)
float mix = UnifiedDefaultParameters::getDefaultValue(53, 9);    // 1.0f (100% mix)
```

## Integration Patterns

### Plugin Initialization

```cpp
class ChimeraPhoenixAudioProcessor : public juce::AudioProcessor {
public:
    void initializeParameters() {
        // Apply defaults for current engine
        UnifiedDefaultParameters::applyDefaults(currentEngineId,
            [this](int paramIndex, float value) {
                setParameterNotifyingHost(paramIndex, value);
            });
    }
    
    void changeEngine(int newEngineId) {
        currentEngineId = newEngineId;
        initializeParameters();  // Apply new defaults
    }
};
```

### Custom Parameter Manager

```cpp
class ParameterManager {
private:
    std::vector<float> parameters;
    
public:
    void loadEngineDefaults(int engineId) {
        parameters = UnifiedDefaultParameters::getDefaultParameters(engineId);
    }
    
    float getParameter(int index) const {
        if (index < parameters.size()) {
            return parameters[index];
        }
        return UnifiedDefaultParameters::getDefaultValue(currentEngine, index);
    }
};
```

### Preset System Integration

```cpp
class PresetManager {
public:
    void createDefaultPreset(int engineId) {
        if (!UnifiedDefaultParameters::hasDefaults(engineId)) {
            return;  // No defaults available
        }
        
        Preset preset;
        preset.name = "Default";
        preset.engineId = engineId;
        
        // Get all parameter values
        auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
        for (int i = 0; i < defaults.size(); ++i) {
            preset.parameters[i] = defaults[i];
        }
        
        savePreset(preset);
    }
};
```

## Error Handling

### Safe Parameter Access

```cpp
float getDefaultValueSafe(int engineId, int paramIndex) {
    if (!UnifiedDefaultParameters::isValidEngine(engineId)) {
        return 0.5f;  // Safe fallback
    }
    
    if (!UnifiedDefaultParameters::isValidParameter(engineId, paramIndex)) {
        return 0.5f;  // Safe fallback
    }
    
    return UnifiedDefaultParameters::getDefaultValue(engineId, paramIndex);
}
```

### Bulk Operations with Validation

```cpp
void applyDefaultsSafe(int engineId, std::function<void(int, float)> setValue) {
    if (!UnifiedDefaultParameters::hasDefaults(engineId)) {
        return;  // No defaults available
    }
    
    UnifiedDefaultParameters::applyDefaults(engineId, setValue);
}
```

## Performance Considerations

### Optimization Tips

1. **Cache Default Values**: Store frequently accessed defaults locally
```cpp
class EngineProcessor {
private:
    std::vector<float> cachedDefaults;
    
public:
    void cacheDefaults(int engineId) {
        cachedDefaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
    }
};
```

2. **Batch Operations**: Use `getDefaultParameters()` for bulk access
```cpp
// Efficient
auto allDefaults = UnifiedDefaultParameters::getDefaultParameters(engineId);

// Less efficient
for (int i = 0; i < paramCount; ++i) {
    float value = UnifiedDefaultParameters::getDefaultValue(engineId, i);
}
```

3. **Validation Caching**: Cache validation results for repeated checks
```cpp
class ValidationCache {
private:
    std::unordered_set<int> validEngines;
    
public:
    bool isEngineValid(int engineId) {
        if (validEngines.find(engineId) != validEngines.end()) {
            return true;  // Cached result
        }
        
        bool valid = UnifiedDefaultParameters::isValidEngine(engineId);
        if (valid) {
            validEngines.insert(engineId);
        }
        return valid;
    }
};
```

## Thread Safety

The UnifiedDefaultParameters API is **thread-safe** for read operations:

- All `get*()` methods are safe for concurrent access
- No internal state modification occurs during read operations
- Static data is immutable after initialization

```cpp
// Thread-safe usage
void processInThread(int engineId) {
    // Safe to call from multiple threads
    auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
    
    // Process defaults...
}
```

## Version Compatibility

### API Stability

The UnifiedDefaultParameters API is designed for long-term stability:

- **Major Version Changes**: Breaking API changes (rare)
- **Minor Version Changes**: New features, backward compatible
- **Patch Version Changes**: Bug fixes, value refinements

### Versioning Strategy

```cpp
// Future-proof parameter access
namespace UnifiedDefaultParameters {
    namespace v1 {
        float getDefaultValue(int engineId, int paramIndex);
    }
    
    namespace v2 {
        // Future enhanced API
    }
    
    // Current stable API (aliases latest stable)
    using v1::getDefaultValue;
}
```

## Migration from Legacy Systems

### Replacing Old API Calls

```cpp
// OLD: Multiple possible sources
float getOldDefault(int engineId, int paramIndex) {
    // Complex logic checking multiple sources
    return 0.5f;  // Generic fallback
}

// NEW: Single source of truth
float getNewDefault(int engineId, int paramIndex) {
    return UnifiedDefaultParameters::getDefaultValue(engineId, paramIndex);
}
```

### Legacy Compatibility Wrapper

```cpp
// Temporary compatibility layer
namespace LegacyDefaults {
    float getDefaultParameterValue(int engineId, int paramIndex) {
        return UnifiedDefaultParameters::getDefaultValue(engineId, paramIndex);
    }
}
```

## Testing and Validation

### Unit Test Examples

```cpp
#include "TestUnifiedDefaults.cpp"

TEST(UnifiedDefaultParameters, ValidEngineRange) {
    for (int i = 0; i <= 56; ++i) {
        EXPECT_TRUE(UnifiedDefaultParameters::isValidEngine(i));
    }
    EXPECT_FALSE(UnifiedDefaultParameters::isValidEngine(-1));
    EXPECT_FALSE(UnifiedDefaultParameters::isValidEngine(57));
}

TEST(UnifiedDefaultParameters, ParameterBounds) {
    for (int engineId = 0; engineId <= 56; ++engineId) {
        if (!UnifiedDefaultParameters::hasDefaults(engineId)) continue;
        
        int paramCount = UnifiedDefaultParameters::getParameterCount(engineId);
        for (int paramIndex = 0; paramIndex < paramCount; ++paramIndex) {
            float value = UnifiedDefaultParameters::getDefaultValue(engineId, paramIndex);
            EXPECT_GE(value, 0.0f);
            EXPECT_LE(value, 1.0f);
        }
    }
}
```

## Conclusion

The UnifiedDefaultParameters API provides a comprehensive, stable, and efficient interface for accessing professionally crafted default parameter values across all 57 engines in Chimera Phoenix. The API is designed for:

- **Simplicity**: Clear, intuitive method names and signatures
- **Safety**: Robust validation and error handling
- **Performance**: Efficient bulk operations and caching support
- **Maintainability**: Consistent patterns and comprehensive documentation
- **Future-proofing**: Stable API design with extension points

**Status: ✅ API DOCUMENTATION COMPLETE**  
**Coverage: 100% of public API surface**  
**Examples: Comprehensive usage patterns included**