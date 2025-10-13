# Trinity Preset Loading End-to-End Integration Test

## Overview

**File:** `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/test_trinity_e2e.cpp`

This comprehensive C++ test program verifies the complete Trinity preset loading flow, from server response parsing through data extraction and validation. It tests the exact same logic and data structures used by `TrinityManager::handlePresetResponse()` and `TrinityManager::applyPreset()`.

## Purpose

The test validates:
1. **JSON Parsing** - Server response can be parsed correctly
2. **Data Extraction** - Preset data can be extracted from the response structure
3. **Structure Validation** - `data.preset.slots` array has the correct format
4. **Parameter Verification** - Each slot contains valid engine_id and parameters
5. **Full Flow Simulation** - Complete end-to-end flow works as expected

## Test Structure

### Test 1: Parse Server Response JSON
- Parses a mock Trinity server response
- Verifies top-level JSON structure
- Checks for required properties: `success`, `type`, `message`, `data`
- Validates response metadata

### Test 2: Extract Preset Data
- Simulates `TrinityProtocol::hasPresetData()` logic
- Simulates `TrinityProtocol::getPresetData()` logic
- Extracts the `data.preset` object
- Verifies preset has a name

### Test 3: Verify Slots Structure
- Simulates `TrinityManager::applyPreset()` loop
- Iterates through `preset.slots` array
- Validates each slot has:
  - `engine_id` (0-56, matching ENGINE_COUNT from EngineTypes.h)
  - `engine_name`
  - `parameters` array
- Checks parameter structure (name/value pairs)

### Test 4: Simulate Full Application Flow
- Runs complete flow from start to finish
- Simulates engine loading for each slot
- Simulates parameter application
- Generates summary statistics
- Reports success/failure of the full pipeline

## Mock Data

The test uses a "Cosmic Thunder" preset with:
- **Slot 0:** Digital Chorus (Engine ID 23) with 5 parameters
- **Slot 1:** Tape Echo (Engine ID 34) with 3 parameters
- **Slot 2:** Plate Reverb (Engine ID 39) with 2 parameters

This matches the exact JSON structure that the Trinity AI server returns:

```json
{
    "success": true,
    "type": "preset",
    "message": "Cosmic Thunder",
    "data": {
        "preset": {
            "name": "Cosmic Thunder",
            "slots": [
                {
                    "slot": 0,
                    "engine_id": 23,
                    "engine_name": "Digital Chorus",
                    "parameters": [...]
                }
            ]
        }
    }
}
```

## Code Architecture

The test uses JUCE's JSON parsing capabilities:
- `juce::JSON::parse()` - Parse JSON strings
- `juce::var` - JUCE's variant type for JSON data
- `juce::DynamicObject` - Object structure access
- `juce::Array` - Array iteration

This matches exactly how the plugin processes Trinity responses.

## Compilation

### Option 1: Using the provided script
```bash
./compile_trinity_e2e.sh
```

### Option 2: Manual compilation
```bash
# Set JUCE directory
export JUCE_DIR=/path/to/JUCE

# Compile
g++ -std=c++17 \
    -I"$JUCE_DIR/modules" \
    -I"JUCE_Plugin/Source" \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreFoundation \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework AudioToolbox \
    -framework Accelerate \
    -framework QuartzCore \
    test_trinity_e2e.cpp \
    -o test_trinity_e2e
```

### Option 3: Using CMake
Add to CMakeLists.txt:
```cmake
add_executable(TrinityE2ETest
    test_trinity_e2e.cpp
)

target_include_directories(TrinityE2ETest PRIVATE
    Source
)

target_compile_features(TrinityE2ETest PRIVATE cxx_std_17)
```

## Running the Test

```bash
./test_trinity_e2e
```

## Expected Output

```
========================================
TRINITY PRESET LOADING E2E TEST
========================================
Testing: Complete Trinity preset loading flow
From: Server response -> JSON parsing -> Data extraction
To: Preset structure verification
========================================

========================================
Test 1: Parse Server Response JSON
========================================
Parsing JSON response...
[PASS] Response is a valid JSON object
[PASS] Has 'success' property
[PASS] Has 'type' property
[PASS] Has 'message' property
[PASS] Has 'data' property
[PASS] Success = true
[PASS] Type = 'preset' (got: preset)
[PASS] Message = 'Cosmic Thunder'

========================================
Test 2: Extract Preset Data
========================================
Extracting 'data' object...
[PASS] Data is an object
Checking for 'preset' property...
[PASS] Data has 'preset' property
Extracting preset data...
[PASS] Preset is an object
[PASS] Preset has 'name' property
  Preset name: Cosmic Thunder

========================================
Test 3: Verify Slots Structure
========================================
Checking for 'slots' array...
[PASS] Preset has 'slots' property
[PASS] Slots is an array
  Slot count: 3
[PASS] Has at least one slot
[PASS] Slot count within limit (6)
----------------------------------------
  Checking Slot 0...
    Is object: YES
    Has engine_id: YES
    Engine ID: 23
    Engine Name: Digital Chorus
    Valid engine ID: YES
    Has parameters: YES
    Parameters is array: YES
    Parameter count: 5
      Param[0]: param1 = 0.5
      Param[1]: param2 = 0.6
      Param[2]: param3 = 0.7
----------------------------------------
  Checking Slot 1...
    Is object: YES
    Has engine_id: YES
    Engine ID: 34
    Engine Name: Tape Echo
    Valid engine ID: YES
    Has parameters: YES
    Parameters is array: YES
    Parameter count: 3
      Param[0]: param1 = 0.4
      Param[1]: param2 = 0.8
      Param[2]: param3 = 0.3
----------------------------------------
  Checking Slot 2...
    Is object: YES
    Has engine_id: YES
    Engine ID: 39
    Engine Name: Plate Reverb
    Valid engine ID: YES
    Has parameters: YES
    Parameters is array: YES
    Parameter count: 2
      Param[0]: param1 = 0.65
      Param[1]: param2 = 0.45
----------------------------------------
[PASS] All slots have valid structure

========================================
Test 4: Simulate Full Application Flow
========================================
Simulating TrinityManager::handlePresetResponse flow...
----------------------------------------
Step 1: Parsing server response
[PASS] JSON parsed successfully

Step 2: Extracting 'data' object
[PASS] Data object contains preset

Step 3: Extracting preset data
  Preset name: Cosmic Thunder
[PASS] Preset name extracted correctly

Step 4: Processing slots
  Processing 3 slots...
  Slot 0: Loading Digital Chorus (ID: 23)
  Slot 1: Loading Tape Echo (ID: 34)
  Slot 2: Loading Plate Reverb (ID: 39)
----------------------------------------

Flow Summary:
  Slots processed: 3 / 3
  Engines loaded: 3 / 3
  Parameters set: 10
[PASS] Full flow completed successfully

========================================
TEST SUMMARY
========================================
Tests run: 4
Tests passed: 4
Tests failed: 0

[SUCCESS] All tests passed!
========================================
```

## Integration with Plugin Code

This test verifies the exact same flow used in the plugin:

### TrinityManager.cpp Line 471-495
```cpp
void TrinityManager::handlePresetResponse(const TrinityNetworkClient::TrinityResponse& response) {
    if (response.data.isObject() && response.data.hasProperty("preset")) {
        auto presetData = response.data.getProperty("preset", juce::var());
        applyPreset(presetData);
    }
}
```

### TrinityManager.cpp Line 258-355
```cpp
void TrinityManager::applyPreset(const juce::var& presetData) {
    if (presetData.hasProperty("slots")) {
        juce::var slotsData = presetData.getProperty("slots", juce::var());

        if (slotsData.isArray()) {
            for (int i = 0; i < slotsData.size() && i < 6; ++i) {
                juce::var slotData = slotsData[i];

                if (slotData.hasProperty("engine_id")) {
                    int engineId = slotData.getProperty("engine_id", 0);
                    audioProcessor.loadEngine(i, engineId);
                }

                if (slotData.hasProperty("parameters")) {
                    // Apply parameters...
                }
            }
        }
    }
}
```

## Related Files

- `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/TrinityManager.cpp` - Production preset loading code
- `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/TrinityProtocol.h` - Protocol definitions
- `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/EngineTypes.h` - Engine ID definitions (ENGINE_COUNT = 57)
- `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/PluginProcessor.h` - Audio processor with loadEngine()

## Engine ID Reference

From EngineTypes.h:
- Engine ID 23 = ENGINE_DIGITAL_CHORUS
- Engine ID 34 = ENGINE_TAPE_ECHO
- Engine ID 39 = ENGINE_PLATE_REVERB
- Valid range: 0-56 (ENGINE_COUNT = 57)

## Success Criteria

All tests pass if:
1. JSON parsing succeeds
2. Data extraction finds preset object
3. All slots have valid structure
4. All engine IDs are in range (0-56)
5. All parameters are valid (0.0-1.0)
6. Full flow completes without errors

## Troubleshooting

### Compilation Errors

**Error:** Cannot find JUCE headers
```bash
# Set JUCE_DIR to your JUCE installation
export JUCE_DIR=/Applications/JUCE
```

**Error:** Framework not found (macOS)
```bash
# Make sure you're on macOS with Xcode installed
xcode-select --install
```

### Runtime Errors

**Error:** JSON parsing fails
- Check JSON syntax in mock response
- Verify JUCE JSON module is linked

**Error:** Property not found
- Verify JSON structure matches expected format
- Check property names (case-sensitive)

## Future Enhancements

Possible additions:
1. Test with invalid JSON
2. Test with missing properties
3. Test with out-of-range engine IDs
4. Test with invalid parameter values
5. Test with empty slots array
6. Test with more than 6 slots
7. Performance benchmarking

## Notes

- This test uses JUCE's JSON parsing but does NOT require the full JUCE AudioProcessor framework
- It's a standalone console application
- It tests the data flow only, not the actual engine loading or audio processing
- Perfect for CI/CD integration testing
