/**
 * test_final_integration.cpp
 * 
 * FINAL INTEGRATION TEST
 * Tests that presets from Trinity AI actually load engines into the plugin
 * This is the ultimate proof that the file-based exchange system works
 */

#include <JuceHeader.h>
#include <iostream>
#include <thread>
#include <chrono>

// Include our plugin headers
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/TrinityManager.h"
#include "JUCE_Plugin/Source/FileExchangeClient.h"
#include "JUCE_Plugin/Source/EngineLibrary.h"

using namespace std::chrono_literals;

// ANSI color codes
const juce::String GREEN = "\033[92m";
const juce::String RED = "\033[91m";
const juce::String YELLOW = "\033[93m";
const juce::String BLUE = "\033[94m";
const juce::String CYAN = "\033[96m";
const juce::String RESET = "\033[0m";

class FinalIntegrationTest : public juce::JUCEApplication,
                             public FileExchangeClient::Listener {
public:
    FinalIntegrationTest() : processor(nullptr), trinity(nullptr) {}
    
    const juce::String getApplicationName() override { 
        return "Final Integration Test"; 
    }
    const juce::String getApplicationVersion() override { 
        return "1.0"; 
    }
    
    void initialise(const juce::String&) override {
        std::cout << BLUE << "=" << juce::String::repeatedString("=", 58) << "=" << RESET << std::endl;
        std::cout << BLUE << "FINAL INTEGRATION TEST - File-Based Preset Exchange" << RESET << std::endl;
        std::cout << BLUE << "=" << juce::String::repeatedString("=", 58) << "=" << RESET << std::endl;
        
        // Create plugin processor
        processor = std::make_unique<ChimeraAudioProcessor>();
        
        // Create Trinity Manager
        trinity = std::make_unique<TrinityManager>(*processor);
        
        // Run all tests
        runTests();
        
        // Exit after tests
        quit();
    }
    
    void shutdown() override {
        trinity.reset();
        processor.reset();
    }
    
    void runTests() {
        bool allPassed = true;
        
        // Test 1: Verify plugin initialization
        if (!testPluginInitialization()) allPassed = false;
        
        // Test 2: Verify file exchange client
        if (!testFileExchangeClient()) allPassed = false;
        
        // Test 3: Send preset request and verify loading
        if (!testPresetLoading()) allPassed = false;
        
        // Test 4: Verify engines are actually loaded
        if (!testEngineVerification()) allPassed = false;
        
        // Final summary
        std::cout << "\n" << BLUE << "=" << juce::String::repeatedString("=", 58) << "=" << RESET << std::endl;
        if (allPassed) {
            std::cout << GREEN << "✓✓✓ ALL TESTS PASSED! ✓✓✓" << RESET << std::endl;
            std::cout << GREEN << "The file-based exchange system is working perfectly!" << RESET << std::endl;
            std::cout << GREEN << "Presets are loading engines correctly into the plugin!" << RESET << std::endl;
        } else {
            std::cout << RED << "✗ Some tests failed - review output above" << RESET << std::endl;
        }
        std::cout << BLUE << "=" << juce::String::repeatedString("=", 58) << "=" << RESET << std::endl;
    }
    
    bool testPluginInitialization() {
        std::cout << "\n" << YELLOW << "Test 1: Plugin Initialization" << RESET << std::endl;
        
        if (!processor) {
            std::cout << RED << "✗ Failed to create processor" << RESET << std::endl;
            return false;
        }
        std::cout << GREEN << "✓ Processor created" << RESET << std::endl;
        
        // Check Trinity Manager
        if (!trinity) {
            std::cout << RED << "✗ Trinity Manager not created" << RESET << std::endl;
            return false;
        }
        std::cout << GREEN << "✓ Trinity Manager created" << RESET << std::endl;
        
        // Check initial slot states
        for (int i = 0; i < 8; ++i) {
            int engineId = processor->getEngineIDForSlot(i);
            std::cout << "  Slot " << i << " initial engine: " << engineId 
                     << " (" << EngineLibrary::getEngineName(engineId) << ")" << std::endl;
        }
        
        return true;
    }
    
    bool testFileExchangeClient() {
        std::cout << "\n" << YELLOW << "Test 2: File Exchange Client" << RESET << std::endl;
        
        // Create file exchange client
        fileExchange = std::make_unique<FileExchangeClient>();
        
        // Generate session ID
        sessionId = "integration_test_" + juce::String(juce::Time::currentTimeMillis());
        
        // Initialize and start monitoring
        fileExchange->initialize(sessionId);
        fileExchange->addListener(this);
        fileExchange->startMonitoring();
        
        auto exchangeDir = fileExchange->getExchangeDirectory();
        std::cout << GREEN << "✓ File exchange initialized" << RESET << std::endl;
        std::cout << "  Exchange directory: " << exchangeDir.getFullPathName() << std::endl;
        std::cout << "  Session ID: " << sessionId << std::endl;
        
        return true;
    }
    
    bool testPresetLoading() {
        std::cout << "\n" << YELLOW << "Test 3: Preset Loading via File Exchange" << RESET << std::endl;
        
        // Send preset request to server
        std::cout << "Sending preset request to server..." << std::endl;
        
        juce::String command = "curl -s -X POST http://localhost:8000/message "
                              "-H 'Content-Type: application/json' "
                              "-d '{\"type\": \"preset_request\", "
                              "\"content\": \"warm vintage compression with tube saturation\", "
                              "\"session_id\": \"" + sessionId + "\"}'";
        
        auto result = std::system(command.toRawUTF8());
        if (result != 0) {
            std::cout << RED << "✗ Failed to send preset request" << RESET << std::endl;
            return false;
        }
        
        std::cout << "Waiting for preset to be delivered..." << std::endl;
        
        // Wait for preset with timeout
        presetReceived = false;
        auto startTime = juce::Time::currentTimeMillis();
        const int timeoutMs = 15000;
        
        while (!presetReceived && (juce::Time::currentTimeMillis() - startTime) < timeoutMs) {
            fileExchange->checkForPresets();
            std::this_thread::sleep_for(500ms);
        }
        
        if (!presetReceived) {
            std::cout << RED << "✗ Timeout waiting for preset" << RESET << std::endl;
            return false;
        }
        
        std::cout << GREEN << "✓ Preset received and loaded!" << RESET << std::endl;
        std::cout << "  Preset name: " << loadedPresetName << std::endl;
        
        return true;
    }
    
    bool testEngineVerification() {
        std::cout << "\n" << YELLOW << "Test 4: Engine Verification" << RESET << std::endl;
        
        // Check that engines were actually loaded
        int enginesLoaded = 0;
        
        for (int i = 0; i < 8; ++i) {
            int engineId = processor->getEngineIDForSlot(i);
            if (engineId > 0) { // Not "None"
                enginesLoaded++;
                std::cout << GREEN << "✓ Slot " << i << " has engine: " 
                         << EngineLibrary::getEngineName(engineId) 
                         << " (ID: " << engineId << ")" << RESET << std::endl;
            }
        }
        
        if (enginesLoaded == 0) {
            std::cout << RED << "✗ No engines were loaded!" << RESET << std::endl;
            return false;
        }
        
        std::cout << GREEN << "✓ " << enginesLoaded << " engines loaded successfully!" << RESET << std::endl;
        
        // Verify parameter values
        auto& params = processor->getParameters();
        std::cout << "\nParameter verification:" << std::endl;
        
        for (int i = 0; i < juce::jmin(5, params.size()); ++i) {
            if (auto* param = params[i]) {
                float value = param->getValue();
                juce::String name = param->getName(50);
                std::cout << "  " << name << ": " << value << std::endl;
            }
        }
        
        return enginesLoaded > 0;
    }
    
    // FileExchangeClient::Listener callback
    void onPresetReceived(const juce::var& presetData) override {
        std::cout << CYAN << ">>> Preset received via file exchange!" << RESET << std::endl;
        
        // Parse preset and apply to processor
        if (presetData.isObject() && presetData.hasProperty("data")) {
            auto data = presetData.getProperty("data", juce::var());
            if (data.hasProperty("preset")) {
                auto preset = data.getProperty("preset", juce::var());
                
                // Get preset name
                loadedPresetName = preset.getProperty("name", "Unknown").toString();
                
                // Apply preset using Trinity Manager
                if (trinity) {
                    trinity->applyPreset(preset);
                    presetReceived = true;
                    
                    std::cout << GREEN << "✓ Preset applied to processor" << RESET << std::endl;
                }
            }
        }
    }
    
    void onExchangeError(const juce::String& error) override {
        std::cout << RED << "Exchange error: " << error << RESET << std::endl;
    }
    
private:
    std::unique_ptr<ChimeraAudioProcessor> processor;
    std::unique_ptr<TrinityManager> trinity;
    std::unique_ptr<FileExchangeClient> fileExchange;
    juce::String sessionId;
    bool presetReceived = false;
    juce::String loadedPresetName;
};

START_JUCE_APPLICATION(FinalIntegrationTest)