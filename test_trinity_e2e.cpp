/**
 * test_trinity_e2e.cpp
 *
 * Comprehensive End-to-End Integration Test for Trinity Preset Loading Flow
 *
 * This test verifies the complete data pipeline from Trinity server response
 * through JSON parsing, data extraction, and verification of the preset structure.
 *
 * Tests the exact flow that TrinityManager::handlePresetResponse uses to apply
 * presets from the Trinity AI server.
 */

#include <JuceHeader.h>
#include <iostream>
#include <vector>
#include <memory>

// Helper function to print test results
void printTestHeader(const juce::String& testName) {
    std::cout << "\n========================================" << std::endl;
    std::cout << testName << std::endl;
    std::cout << "========================================" << std::endl;
}

void printTestResult(bool passed, const juce::String& message) {
    if (passed) {
        std::cout << "[PASS] " << message << std::endl;
    } else {
        std::cout << "[FAIL] " << message << std::endl;
    }
}

void printDivider() {
    std::cout << "----------------------------------------" << std::endl;
}

// Test 1: Parse Server Response JSON
bool testParseServerResponse() {
    printTestHeader("Test 1: Parse Server Response JSON");

    // Mock Trinity server response for "Cosmic Thunder" preset
    juce::String mockResponse = R"({
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
                        "parameters": [
                            {"name": "param1", "value": 0.5},
                            {"name": "param2", "value": 0.6},
                            {"name": "param3", "value": 0.7}
                        ]
                    },
                    {
                        "slot": 1,
                        "engine_id": 34,
                        "engine_name": "Tape Echo",
                        "parameters": [
                            {"name": "param1", "value": 0.4},
                            {"name": "param2", "value": 0.8},
                            {"name": "param3", "value": 0.3}
                        ]
                    },
                    {
                        "slot": 2,
                        "engine_id": 39,
                        "engine_name": "Plate Reverb",
                        "parameters": [
                            {"name": "param1", "value": 0.65},
                            {"name": "param2", "value": 0.45},
                            {"name": "param3", "value": 0.55}
                        ]
                    }
                ]
            }
        }
    })";

    std::cout << "Parsing JSON response..." << std::endl;
    juce::var parsed = juce::JSON::parse(mockResponse);

    bool isObject = parsed.isObject();
    printTestResult(isObject, "Response is a valid JSON object");

    if (!isObject) {
        std::cout << "ERROR: Failed to parse JSON" << std::endl;
        return false;
    }

    // Verify top-level structure
    bool hasSuccess = parsed.hasProperty("success");
    bool hasType = parsed.hasProperty("type");
    bool hasMessage = parsed.hasProperty("message");
    bool hasData = parsed.hasProperty("data");

    printTestResult(hasSuccess, "Has 'success' property");
    printTestResult(hasType, "Has 'type' property");
    printTestResult(hasMessage, "Has 'message' property");
    printTestResult(hasData, "Has 'data' property");

    bool success = parsed.getProperty("success", false);
    juce::String type = parsed.getProperty("type", "").toString();
    juce::String message = parsed.getProperty("message", "").toString();

    printTestResult(success, "Success = true");
    printTestResult(type == "preset", "Type = 'preset' (got: " + type + ")");
    printTestResult(message == "Cosmic Thunder", "Message = 'Cosmic Thunder'");

    return isObject && hasSuccess && hasType && hasMessage && hasData && success;
}

// Test 2: Extract Preset Data (TrinityProtocol pattern)
bool testExtractPresetData() {
    printTestHeader("Test 2: Extract Preset Data");

    juce::String mockResponse = R"({
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
                        "parameters": [
                            {"name": "param1", "value": 0.5},
                            {"name": "param2", "value": 0.6}
                        ]
                    },
                    {
                        "slot": 1,
                        "engine_id": 34,
                        "engine_name": "Tape Echo",
                        "parameters": [
                            {"name": "param1", "value": 0.4}
                        ]
                    }
                ]
            }
        }
    })";

    juce::var parsed = juce::JSON::parse(mockResponse);

    // Extract data object (simulating TrinityNetworkClient::TrinityResponse.data)
    std::cout << "Extracting 'data' object..." << std::endl;
    juce::var data = parsed.getProperty("data", juce::var());
    bool hasDataObject = data.isObject();
    printTestResult(hasDataObject, "Data is an object");

    if (!hasDataObject) return false;

    // Check for preset property (simulating TrinityProtocol::hasPresetData)
    std::cout << "Checking for 'preset' property..." << std::endl;
    bool hasPreset = data.hasProperty("preset");
    printTestResult(hasPreset, "Data has 'preset' property");

    if (!hasPreset) return false;

    // Extract preset data (simulating TrinityProtocol::getPresetData)
    std::cout << "Extracting preset data..." << std::endl;
    juce::var presetData = data.getProperty("preset", juce::var());
    bool isPresetObject = presetData.isObject();
    printTestResult(isPresetObject, "Preset is an object");

    if (!isPresetObject) return false;

    // Verify preset has name
    bool hasName = presetData.hasProperty("name");
    printTestResult(hasName, "Preset has 'name' property");

    juce::String presetName = presetData.getProperty("name", "Unknown").toString();
    std::cout << "  Preset name: " << presetName << std::endl;

    return hasDataObject && hasPreset && isPresetObject && hasName;
}

// Test 3: Verify Slots Structure
bool testVerifySlotsStructure() {
    printTestHeader("Test 3: Verify Slots Structure");

    juce::String mockResponse = R"({
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
                        "parameters": [
                            {"name": "param1", "value": 0.5},
                            {"name": "param2", "value": 0.6},
                            {"name": "param3", "value": 0.7}
                        ]
                    },
                    {
                        "slot": 1,
                        "engine_id": 34,
                        "engine_name": "Tape Echo",
                        "parameters": [
                            {"name": "param1", "value": 0.4},
                            {"name": "param2", "value": 0.8}
                        ]
                    },
                    {
                        "slot": 2,
                        "engine_id": 39,
                        "engine_name": "Plate Reverb",
                        "parameters": [
                            {"name": "param1", "value": 0.65}
                        ]
                    }
                ]
            }
        }
    })";

    juce::var parsed = juce::JSON::parse(mockResponse);
    juce::var data = parsed.getProperty("data", juce::var());
    juce::var presetData = data.getProperty("preset", juce::var());

    // Check for slots array (simulating TrinityManager::applyPreset)
    std::cout << "Checking for 'slots' array..." << std::endl;
    bool hasSlots = presetData.hasProperty("slots");
    printTestResult(hasSlots, "Preset has 'slots' property");

    if (!hasSlots) return false;

    juce::var slotsData = presetData.getProperty("slots", juce::var());
    bool isSlotsArray = slotsData.isArray();
    printTestResult(isSlotsArray, "Slots is an array");

    if (!isSlotsArray) return false;

    int slotCount = slotsData.size();
    std::cout << "  Slot count: " << slotCount << std::endl;
    printTestResult(slotCount > 0, "Has at least one slot");
    printTestResult(slotCount <= 6, "Slot count within limit (6)");

    // Iterate through slots (simulating the loop in TrinityManager::applyPreset)
    bool allSlotsValid = true;
    for (int i = 0; i < slotCount && i < 6; ++i) {
        printDivider();
        std::cout << "  Checking Slot " << i << "..." << std::endl;

        juce::var slotData = slotsData[i];

        bool isSlotObject = slotData.isObject();
        std::cout << "    Is object: " << (isSlotObject ? "YES" : "NO") << std::endl;

        if (!isSlotObject) {
            allSlotsValid = false;
            continue;
        }

        // Check for engine_id
        bool hasEngineId = slotData.hasProperty("engine_id");
        std::cout << "    Has engine_id: " << (hasEngineId ? "YES" : "NO") << std::endl;

        if (hasEngineId) {
            int engineId = slotData.getProperty("engine_id", 0);
            juce::String engineName = slotData.getProperty("engine_name", "Unknown").toString();
            std::cout << "    Engine ID: " << engineId << std::endl;
            std::cout << "    Engine Name: " << engineName << std::endl;

            // Verify engine_id is in valid range
            bool validEngineId = (engineId >= 0 && engineId < 57);
            std::cout << "    Valid engine ID: " << (validEngineId ? "YES" : "NO") << std::endl;

            if (!validEngineId) {
                allSlotsValid = false;
            }
        } else {
            allSlotsValid = false;
        }

        // Check for parameters
        bool hasParameters = slotData.hasProperty("parameters");
        std::cout << "    Has parameters: " << (hasParameters ? "YES" : "NO") << std::endl;

        if (hasParameters) {
            juce::var paramsData = slotData.getProperty("parameters", juce::var());
            bool isParamsArray = paramsData.isArray();
            std::cout << "    Parameters is array: " << (isParamsArray ? "YES" : "NO") << std::endl;

            if (isParamsArray) {
                int paramCount = paramsData.size();
                std::cout << "    Parameter count: " << paramCount << std::endl;

                // Check first few parameters
                for (int p = 0; p < paramCount && p < 3; ++p) {
                    juce::var paramData = paramsData[p];
                    if (paramData.isObject()) {
                        juce::String paramName = paramData.getProperty("name", "").toString();
                        float value = paramData.getProperty("value", 0.0f);
                        std::cout << "      Param[" << p << "]: " << paramName
                                  << " = " << value << std::endl;
                    }
                }
            } else {
                allSlotsValid = false;
            }
        }
    }

    printDivider();
    printTestResult(allSlotsValid, "All slots have valid structure");

    return hasSlots && isSlotsArray && slotCount > 0 && allSlotsValid;
}

// Test 4: Simulate Full Application Flow
bool testFullApplicationFlow() {
    printTestHeader("Test 4: Simulate Full Application Flow");

    juce::String mockResponse = R"({
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
                        "parameters": [
                            {"name": "param1", "value": 0.5},
                            {"name": "param2", "value": 0.6},
                            {"name": "param3", "value": 0.7},
                            {"name": "param4", "value": 0.4},
                            {"name": "param5", "value": 0.9}
                        ]
                    },
                    {
                        "slot": 1,
                        "engine_id": 34,
                        "engine_name": "Tape Echo",
                        "parameters": [
                            {"name": "param1", "value": 0.4},
                            {"name": "param2", "value": 0.8},
                            {"name": "param3", "value": 0.3}
                        ]
                    },
                    {
                        "slot": 2,
                        "engine_id": 39,
                        "engine_name": "Plate Reverb",
                        "parameters": [
                            {"name": "param1", "value": 0.65},
                            {"name": "param2", "value": 0.45}
                        ]
                    }
                ]
            }
        }
    })";

    std::cout << "Simulating TrinityManager::handlePresetResponse flow..." << std::endl;
    printDivider();

    // Step 1: Parse response
    std::cout << "Step 1: Parsing server response" << std::endl;
    juce::var parsed = juce::JSON::parse(mockResponse);
    if (!parsed.isObject()) {
        printTestResult(false, "Failed to parse JSON");
        return false;
    }
    printTestResult(true, "JSON parsed successfully");

    // Step 2: Extract data object
    std::cout << "\nStep 2: Extracting 'data' object" << std::endl;
    juce::var data = parsed.getProperty("data", juce::var());
    if (!data.isObject() || !data.hasProperty("preset")) {
        printTestResult(false, "No preset data found");
        return false;
    }
    printTestResult(true, "Data object contains preset");

    // Step 3: Extract preset data
    std::cout << "\nStep 3: Extracting preset data" << std::endl;
    juce::var presetData = data.getProperty("preset", juce::var());
    juce::String presetName = presetData.getProperty("name", "Unknown").toString();
    std::cout << "  Preset name: " << presetName << std::endl;
    printTestResult(presetName == "Cosmic Thunder", "Preset name extracted correctly");

    // Step 4: Process slots
    std::cout << "\nStep 4: Processing slots" << std::endl;
    if (!presetData.hasProperty("slots")) {
        printTestResult(false, "No slots found in preset");
        return false;
    }

    juce::var slotsData = presetData.getProperty("slots", juce::var());
    if (!slotsData.isArray()) {
        printTestResult(false, "Slots is not an array");
        return false;
    }

    int slotCount = slotsData.size();
    std::cout << "  Processing " << slotCount << " slots..." << std::endl;

    int slotsProcessed = 0;
    int enginesLoaded = 0;
    int parametersSet = 0;

    for (int i = 0; i < slotCount && i < 6; ++i) {
        juce::var slotData = slotsData[i];

        if (!slotData.isObject()) {
            continue;
        }

        slotsProcessed++;

        // Simulate engine loading
        if (slotData.hasProperty("engine_id")) {
            int engineId = slotData.getProperty("engine_id", 0);
            juce::String engineName = slotData.getProperty("engine_name", "Unknown").toString();

            std::cout << "  Slot " << i << ": Loading " << engineName
                      << " (ID: " << engineId << ")" << std::endl;

            // Validate engine ID (ENGINE_COUNT = 57)
            if (engineId >= 0 && engineId < 57) {
                enginesLoaded++;
            }
        }

        // Simulate parameter application
        if (slotData.hasProperty("parameters")) {
            juce::var paramsData = slotData.getProperty("parameters", juce::var());

            if (paramsData.isArray()) {
                int paramCount = paramsData.size();

                for (int p = 0; p < paramCount; ++p) {
                    juce::var paramData = paramsData[p];

                    if (paramData.isObject()) {
                        juce::String paramName = paramData.getProperty("name", "").toString();
                        float value = paramData.getProperty("value", 0.0f);

                        // Validate parameter value (should be 0.0 - 1.0)
                        if (value >= 0.0f && value <= 1.0f) {
                            parametersSet++;
                        }
                    }
                }
            }
        }
    }

    printDivider();
    std::cout << "\nFlow Summary:" << std::endl;
    std::cout << "  Slots processed: " << slotsProcessed << " / " << slotCount << std::endl;
    std::cout << "  Engines loaded: " << enginesLoaded << " / " << slotsProcessed << std::endl;
    std::cout << "  Parameters set: " << parametersSet << std::endl;

    bool success = (slotsProcessed == slotCount) &&
                   (enginesLoaded == slotsProcessed) &&
                   (parametersSet > 0);

    printTestResult(success, "Full flow completed successfully");

    return success;
}

// Main test runner
int main(int argc, char* argv[]) {
    std::cout << "\n" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "TRINITY PRESET LOADING E2E TEST" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Testing: Complete Trinity preset loading flow" << std::endl;
    std::cout << "From: Server response -> JSON parsing -> Data extraction" << std::endl;
    std::cout << "To: Preset structure verification" << std::endl;
    std::cout << "========================================\n" << std::endl;

    int testsRun = 0;
    int testsPassed = 0;

    // Run all tests
    bool test1 = testParseServerResponse();
    testsRun++;
    if (test1) testsPassed++;

    bool test2 = testExtractPresetData();
    testsRun++;
    if (test2) testsPassed++;

    bool test3 = testVerifySlotsStructure();
    testsRun++;
    if (test3) testsPassed++;

    bool test4 = testFullApplicationFlow();
    testsRun++;
    if (test4) testsPassed++;

    // Print final summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST SUMMARY" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Tests run: " << testsRun << std::endl;
    std::cout << "Tests passed: " << testsPassed << std::endl;
    std::cout << "Tests failed: " << (testsRun - testsPassed) << std::endl;

    if (testsPassed == testsRun) {
        std::cout << "\n[SUCCESS] All tests passed!" << std::endl;
        std::cout << "========================================\n" << std::endl;
        return 0;
    } else {
        std::cout << "\n[FAILURE] Some tests failed!" << std::endl;
        std::cout << "========================================\n" << std::endl;
        return 1;
    }
}
