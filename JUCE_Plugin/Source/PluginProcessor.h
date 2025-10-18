#pragma once

#include <JuceHeader.h>
#include "EngineBase.h"
#include "ParameterDefinitions.h"
#include "SlotConfiguration.h"
#include <array>
#include <memory>
#include <atomic>
#include <mutex>
#ifdef __linux__
    #include <jack/jack.h>
#endif

class ChimeraAudioProcessor : public juce::AudioProcessor,
                              private juce::AudioProcessorValueTreeState::Listener {
public:
    ChimeraAudioProcessor();
    ~ChimeraAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Chimera v3.0"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }
    
    std::unique_ptr<EngineBase>& getEngine(int slot) { 
        return m_activeEngines[slot]; 
    }
    
    // Engine ID mapping functions
    static int engineIDToChoiceIndex(int engineID);
    static int choiceIndexToEngineID(int choiceIndex);
    
    // Mix parameter index mapping
    static int getMixParameterIndex(int engineID);
    
    // Diagnostic methods
    void runComprehensiveDiagnostic();
    struct DiagnosticResult {
        int engineID;
        juce::String engineName;
        bool passed;
        float confidence;
        juce::String issues;
    };
    std::vector<DiagnosticResult> getLastDiagnosticResults() const { return m_diagnosticResults; }
    
    // Level metering (public for UI)
    float getCurrentOutputLevel() const { return m_currentOutputLevel.load(); }
    float getCurrentInputLevel() const { return m_currentInputLevel.load(); }
    
    // Slot management for UI
    float getSlotActivity(int slot) const;
    void setSlotEngine(int slot, int engineID);
    int getEngineIDForSlot(int slot) const {
        if (slot < 0 || slot >= NUM_SLOTS) return 0;
        if (m_activeEngines[slot]) {
            // Get engine ID from the actual engine instance
            // For now, get from parameter value
            auto* param = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_engine");
            if (param) {
                int choiceIndex = static_cast<int>(param->load());
                return choiceIndexToEngineID(choiceIndex);
            }
        }
        return 0;
    }
    
    // Performance monitoring
    float getCpuUsage() const { return 0.0f; } // TODO: Implement actual CPU measurement
    
private:
    std::vector<DiagnosticResult> m_diagnosticResults;
    
    // Initialize mappings in constructor
    static void initializeEngineMappings();
    
    // Validation
    static bool isValidEngineID(int engineID);
    
    // Testing
    void runEngineTests();
    void runIsolatedEngineTests();

    // Allow PluginEditorNexusStatic to call loadEngine for Trinity preset loading
    friend class PluginEditorNexusStatic;

private:
#ifdef __linux__
    // Direct JACK connection to bypass JUCE wrapper bug
    jack_client_t* jackClient = nullptr;
    jack_port_t* jackInputL = nullptr;
    jack_port_t* jackInputR = nullptr;
    jack_port_t* jackOutputL = nullptr;
    jack_port_t* jackOutputR = nullptr;
    void initializeJackDirect();
    void shutdownJackDirect();
#endif
    juce::AudioProcessorValueTreeState parameters;
    static constexpr int NUM_SLOTS = CHIMERA_NUM_SLOTS;  // Using centralized configuration
    std::array<std::unique_ptr<EngineBase>, NUM_SLOTS> m_activeEngines;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void loadEngine(int slot, int engineID);
    void updateEngineParameters(int slot);
    void applyDefaultParameters(int slot, int engineID);
    
    double m_sampleRate = 44100.0;
    int m_samplesPerBlock = 512;
    
    // AI Server management
    std::unique_ptr<juce::ChildProcess> m_aiServerProcess;
    void startAIServer();
    void stopAIServer();
    
    // Level metering
    std::atomic<float> m_currentOutputLevel{0.0f};
    std::atomic<float> m_currentInputLevel{0.0f};
    std::array<std::atomic<float>, NUM_SLOTS> m_slotActivityLevels;
    
    // Thread safety for engine management
    mutable std::mutex m_engineMutex;
    std::atomic<bool> m_engineChangePending{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChimeraAudioProcessor)
};