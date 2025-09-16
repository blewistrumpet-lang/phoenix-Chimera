# Parameter System Maintenance Guide

## Overview
The Chimera Phoenix parameter system uses a centralized registry with multiple validation layers to ensure consistency across the plugin, UI, presets, and AI systems.

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────┐
│           EngineParameterRegistry               │
│         (Single Source of Truth)                │
└────────────┬───────────────────┬────────────────┘
             │                   │
    ┌────────▼──────┐   ┌───────▼──────┐
    │   Engine.cpp  │   │   Plugin UI   │
    │  (Register)   │   │   (Query)     │
    └───────────────┘   └──────────────┘
             │                   │
    ┌────────▼──────────────────▼────────┐
    │      ParameterValidation           │
    │   (Automated Checking)              │
    └─────────────────────────────────────┘
```

## 🔧 How to Maintain Parameters

### Adding a New Engine

1. **Create the engine class** inheriting from `EngineBase`

2. **Register parameters in constructor:**
```cpp
MyNewEngine::MyNewEngine() {
    REGISTER_ENGINE_PARAMS(ENGINE_MY_NEW, "My New Engine")
        .param("Drive", 0.3f, "%", "Distortion amount")
        .param("Tone", 0.5f, "", "Tonal balance")
        .param("Mix", 1.0f, "%", "Dry/wet mix")
        .commit();
}
```

3. **Implement getParameterName using registry:**
```cpp
juce::String MyNewEngine::getParameterName(int index) const {
    return EngineParameterRegistry::getInstance()
        .getParameterName(ENGINE_MY_NEW, index);
}
```

4. **Run validation:**
```bash
cd JUCE_Plugin
./build/parameter_validator --engine ENGINE_MY_NEW
```

### Modifying Existing Parameters

1. **Update the registration** in the engine constructor
2. **Run migration check:**
```bash
python3 Tools/migrate_parameters.py --engine EngineName --report
```
3. **Update any hardcoded references** in:
   - UI components
   - Preset systems
   - AI parameter database

### Validation Checklist

Run these checks after any parameter changes:

```bash
# 1. Check C++ side
cd JUCE_Plugin/build
./parameter_validator --all

# 2. Check cross-system consistency
python3 ../Tools/check_parameter_consistency.py

# 3. Run unit tests
./test_parameters

# 4. Generate updated documentation
python3 ../Tools/generate_parameter_docs.py --format all
```

## 🚨 Common Issues and Solutions

### Issue: "Parameter X has generic name"
**Solution:** Update the engine's registration to use descriptive names:
```cpp
// Bad
.param("Param 1", 0.5f)

// Good  
.param("Cutoff Frequency", 0.5f, "Hz", "Filter cutoff")
```

### Issue: "Engine not registered"
**Solution:** Ensure the engine constructor includes registration:
```cpp
EngineConstructor() {
    REGISTER_ENGINE_PARAMS(ENGINE_ID, "Engine Name")
        // ... parameters
        .commit();
}
```

### Issue: "Parameter count mismatch"
**Solution:** Verify that:
1. Registration includes all parameters
2. `getNumParameters()` returns correct count
3. `updateParameters()` handles all indices

### Issue: "UI showing wrong parameter names"
**Solution:** Update UI to use registry:
```cpp
// Old way (hardcoded)
paramLabel.setText("Param 1", dontSendNotification);

// New way (from registry)
auto paramName = EngineParameterRegistry::getInstance()
    .getParameterName(engineId, paramIndex);
paramLabel.setText(paramName, dontSendNotification);
```

## 📊 Monitoring System Health

### Daily Checks (Automated via CI/CD)
- ✅ All engines have registered parameters
- ✅ No generic "Param X" names
- ✅ Default values are sensible
- ✅ Parameter counts match implementation

### Weekly Checks (Manual)
- Review parameter consistency report
- Check for new unmigrated engines
- Update documentation if needed
- Sync with AI parameter database

### Monthly Checks
- Full system validation
- Performance profiling of registry
- Review and update parameter guidelines
- Training/documentation updates

## 🔄 Migration Status Tracking

Check migration progress:
```bash
python3 Tools/migrate_parameters.py --report
```

This shows:
- Engines still using old system
- Successfully migrated engines
- Engines with validation warnings

## 🧪 Testing New Parameters

1. **Unit Test Template:**
```cpp
TEST(ParameterTest, NewEngineParameters) {
    auto engine = EngineFactory::createEngine(ENGINE_NEW);
    ASSERT_NE(engine, nullptr);
    
    // Check registration
    auto& registry = EngineParameterRegistry::getInstance();
    EXPECT_TRUE(registry.isEngineRegistered(ENGINE_NEW));
    
    // Validate parameters
    auto result = ParameterValidation::validateEngine(
        engine.get(), ENGINE_NEW);
    EXPECT_TRUE(result.passed) << result.errors[0];
}
```

2. **Integration Test:**
```bash
# Test with actual audio
./test_engine_audio ENGINE_NEW test_audio.wav
```

3. **UI Test:**
- Load plugin in DAW
- Check all parameters display correctly
- Verify parameter automation works

## 📝 Documentation Updates

When parameters change, update:

1. **Auto-generated docs:**
```bash
python3 Tools/generate_parameter_docs.py --all
```

2. **Manual updates needed:**
- User manual (if parameter behavior changed)
- Preset compatibility notes
- AI training data

## 🔐 Security & Performance

### Performance Considerations
- Registry lookup is O(1) after initialization
- Parameters cached in UI components
- No runtime file I/O after startup

### Thread Safety
- Registry uses mutex for all operations
- Safe to query from audio thread
- Registration only happens at startup

## 🚀 Best Practices

1. **Always use descriptive names**
   - Good: "Cutoff Frequency", "Drive Amount"
   - Bad: "Param1", "Value", "Amount"

2. **Include units where applicable**
   - Frequency: Hz
   - Time: ms or s
   - Level: dB or %

3. **Provide helpful descriptions**
   - Explain what the parameter does
   - Note any special ranges or sweet spots

4. **Keep defaults musical**
   - Should sound good immediately
   - Safe values that won't damage speakers
   - Demonstrate the engine's character

5. **Maintain backward compatibility**
   - Don't change parameter order
   - Keep same parameter count
   - Use same value ranges

## 📧 Support

For parameter system issues:
1. Check this guide first
2. Run validation tools
3. Check CI/CD logs
4. Contact: [development team]

## 🔄 Version History

- v1.0: Initial registry system
- v1.1: Added validation layer
- v1.2: CI/CD integration
- v1.3: Migration tools
- Current: v1.4 (Full system)

---

Last Updated: 2025-09-13
System Version: 1.4
Validation Status: ✅ ACTIVE