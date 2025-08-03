#pragma once

#include <JuceHeader.h>
#include "EngineBase.h"
#include "ParameterDefinitions.h"
#include <array>
#include <memory>
#include <atomic>

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
    
    // Initialize mappings in constructor
    static void initializeEngineMappings();
    
    // Validation
    static bool isValidEngineID(int engineID);
    
    // Level metering
    float getCurrentOutputLevel() const { return m_currentOutputLevel.load(); }
    
    // Testing
    void runEngineTests();

private:
    juce::AudioProcessorValueTreeState parameters;
    static constexpr int NUM_SLOTS = 6;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChimeraAudioProcessor)
};