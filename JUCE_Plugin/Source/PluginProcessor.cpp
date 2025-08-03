#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "EngineFactory.h"
#include "DefaultParameterValues.h"
#include "EngineTypes.h"
#include "EngineTestRunner.h"

// Engine ID to Choice Index mapping table
// Maps from raw engine IDs to their position in the dropdown (accounting for "Bypass" at index 0)
static const std::map<int, int> engineIDToChoiceMap = {
    {-1, 0},  // ENGINE_BYPASS (-1) -> "Bypass" is at index 0
    {38, 1},  // ENGINE_K_STYLE -> "K-Style Overdrive" at index 1
    {1, 2},   // ENGINE_TAPE_ECHO -> "Tape Echo" at index 2
    {3, 3},   // ENGINE_PLATE_REVERB -> "Plate Reverb" at index 3
    {36, 4},  // ENGINE_RODENT_DISTORTION -> "Rodent Distortion" at index 4
    {35, 5},  // ENGINE_MUFF_FUZZ -> "Muff Fuzz" at index 5
    {22, 6},  // ENGINE_CLASSIC_TREMOLO -> "Classic Tremolo" at index 6
    {8, 7},   // ENGINE_MAGNETIC_DRUM_ECHO -> "Magnetic Drum Echo" at index 7
    {9, 8},   // ENGINE_BUCKET_BRIGADE_DELAY -> "Bucket Brigade Delay" at index 8
    {53, 9},  // ENGINE_DIGITAL_DELAY -> "Digital Delay" at index 9
    {21, 10}, // ENGINE_HARMONIC_TREMOLO -> "Harmonic Tremolo" at index 10
    {24, 11}, // ENGINE_ROTARY_SPEAKER -> "Rotary Speaker" at index 11
    {44, 12}, // ENGINE_DETUNE_DOUBLER -> "Detune Doubler" at index 12
    {28, 13}, // ENGINE_LADDER_FILTER -> "Ladder Filter" at index 13
    {30, 14}, // ENGINE_FORMANT_FILTER -> "Formant Filter" at index 14
    {7, 15},  // ENGINE_VCA_COMPRESSOR -> "Classic Compressor" at index 15
    {29, 16}, // ENGINE_STATE_VARIABLE_FILTER -> "State Variable Filter" at index 16
    {11, 17}, // ENGINE_DIGITAL_CHORUS -> "Stereo Chorus" at index 17
    {39, 18}, // ENGINE_SPECTRAL_FREEZE -> "Spectral Freeze" at index 18
    {16, 19}, // ENGINE_GRANULAR_CLOUD -> "Granular Cloud" at index 19
    {15, 20}, // ENGINE_RING_MODULATOR -> "Analog Ring Modulator" at index 20
    {34, 21}, // ENGINE_MULTIBAND_SATURATOR -> "Multiband Saturator" at index 21
    {23, 22}, // ENGINE_COMB_RESONATOR -> "Comb Resonator" at index 22
    {14, 23}, // ENGINE_PITCH_SHIFTER -> "Pitch Shifter" at index 23
    {45, 24}, // ENGINE_PHASED_VOCODER -> "Phased Vocoder" at index 24
    {4, 25},  // ENGINE_CONVOLUTION_REVERB -> "Convolution Reverb" at index 25
    {33, 26}, // ENGINE_BIT_CRUSHER -> "Bit Crusher" at index 26
    {19, 27}, // ENGINE_FREQUENCY_SHIFTER -> "Frequency Shifter" at index 27
    {31, 28}, // ENGINE_WAVE_FOLDER -> "Wave Folder" at index 28
    {2, 29},  // ENGINE_SHIMMER_REVERB -> "Shimmer Reverb" at index 29
    {17, 30}, // ENGINE_VOCAL_FORMANT -> "Vocal Formant Filter" at index 30
    {20, 31}, // ENGINE_TRANSIENT_SHAPER -> "Transient Shaper" at index 31
    {18, 32}, // ENGINE_DIMENSION_EXPANDER -> "Dimension Expander" at index 32
    {12, 33}, // ENGINE_ANALOG_PHASER -> "Analog Phaser" at index 33
    {48, 34}, // ENGINE_ENVELOPE_FILTER -> "Envelope Filter" at index 34
    {43, 35}, // ENGINE_GATED_REVERB -> "Gated Reverb" at index 35
    {32, 36}, // ENGINE_HARMONIC_EXCITER -> "Harmonic Exciter" at index 36
    {49, 37}, // ENGINE_FEEDBACK_NETWORK -> "Feedback Network" at index 37
    {42, 38}, // ENGINE_INTELLIGENT_HARMONIZER -> "Intelligent Harmonizer" at index 38
    {27, 39}, // ENGINE_PARAMETRIC_EQ -> "Parametric EQ" at index 39
    {50, 40}, // ENGINE_MASTERING_LIMITER -> "Mastering Limiter" at index 40
    {47, 41}, // ENGINE_NOISE_GATE -> "Noise Gate" at index 41
    {6, 42},  // ENGINE_OPTO_COMPRESSOR -> "Vintage Opto" at index 42
    {46, 43}, // ENGINE_SPECTRAL_GATE -> "Spectral Gate" at index 43
    {41, 44}, // ENGINE_CHAOS_GENERATOR -> "Chaos Generator" at index 44
    {40, 45}, // ENGINE_BUFFER_REPEAT -> "Buffer Repeat" at index 45
    {26, 46}, // ENGINE_VINTAGE_CONSOLE_EQ -> "Vintage Console EQ" at index 46
    {25, 47}, // ENGINE_MID_SIDE_PROCESSOR -> "Mid/Side Processor" at index 47
    {0, 48},  // ENGINE_VINTAGE_TUBE -> "Vintage Tube Preamp" at index 48
    {5, 49},  // ENGINE_SPRING_REVERB -> "Spring Reverb" at index 49
    {52, 50}, // ENGINE_RESONANT_CHORUS -> "Resonant Chorus" at index 50
    {51, 51}, // ENGINE_STEREO_WIDENER -> "Stereo Widener" at index 51
    {54, 52}, // ENGINE_DYNAMIC_EQ -> "Dynamic EQ" at index 52
    {55, 53}  // ENGINE_STEREO_IMAGER -> "Stereo Imager" at index 53
};

// Reverse mapping from choice index to engine ID
static std::map<int, int> createReverseMap() {
    std::map<int, int> reverseMap;
    for (const auto& pair : engineIDToChoiceMap) {
        reverseMap[pair.second] = pair.first;
    }
    return reverseMap;
}

static const std::map<int, int> choiceToEngineIDMap = createReverseMap();

// Initialize the static mappings
void ChimeraAudioProcessor::initializeEngineMappings() {
    // Already initialized as static const above
}

// Validation function
bool ChimeraAudioProcessor::isValidEngineID(int engineID) {
    return engineIDToChoiceMap.find(engineID) != engineIDToChoiceMap.end();
}

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Engine choices array
    juce::StringArray engineChoices{"Bypass", "K-Style Overdrive", "Tape Echo", "Plate Reverb",
                                   "Rodent Distortion", "Muff Fuzz", "Classic Tremolo",
                                   "Magnetic Drum Echo", "Bucket Brigade Delay", "Digital Delay",
                                   "Harmonic Tremolo", "Rotary Speaker", "Detune Doubler",
                                   "Ladder Filter", "Formant Filter", "Classic Compressor",
                                   "State Variable Filter", "Stereo Chorus", "Spectral Freeze",
                                   "Granular Cloud", "Analog Ring Modulator", "Multiband Saturator",
                                   "Comb Resonator", "Pitch Shifter", "Phased Vocoder",
                                   "Convolution Reverb", "Bit Crusher", "Frequency Shifter",
                                   "Wave Folder", "Shimmer Reverb", "Vocal Formant Filter",
                                   "Transient Shaper", "Dimension Expander", "Analog Phaser",
                                   "Envelope Filter", "Gated Reverb", "Harmonic Exciter",
                                   "Feedback Network", "Intelligent Harmonizer", "Parametric EQ",
                                   "Mastering Limiter", "Noise Gate", "Vintage Opto",
                                   "Spectral Gate", "Chaos Generator", "Buffer Repeat",
                                   "Vintage Console EQ", "Mid/Side Processor", "Vintage Tube Preamp",
                                   "Spring Reverb", "Resonant Chorus", "Stereo Widener",
                                   "Dynamic EQ", "Stereo Imager"};
    
    // Create parameters for all 6 slots
    for (int slot = 1; slot <= 6; ++slot) {
        juce::String slotStr = juce::String(slot);
        
        // Slot parameters (10 per slot)
        for (int i = 0; i < 10; ++i) {
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                "slot" + slotStr + "_param" + juce::String(i + 1),
                "Slot " + slotStr + " Param " + juce::String(i + 1),
                0.0f, 1.0f, 0.5f));
        }
        
        // Engine selector
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "slot" + slotStr + "_engine",
            "Slot " + slotStr + " Engine",
            engineChoices,
            0));
        
        // Bypass switch
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "slot" + slotStr + "_bypass",
            "Slot " + slotStr + " Bypass",
            false));
    }
    
    return { params.begin(), params.end() };
}

ChimeraAudioProcessor::ChimeraAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "ChimeraParameters", createParameterLayout())
{
    // Validate engine mappings on construction
    DBG("Initializing ChimeraAudioProcessor - Validating engine mappings...");
    
    // Log the engine choice array for debugging
    auto* testParam = dynamic_cast<juce::AudioParameterChoice*>(parameters.getParameter("slot1_engine"));
    if (testParam) {
        DBG("Engine choices in dropdown:");
        for (int i = 0; i < testParam->choices.size(); ++i) {
            DBG("  Choice " + juce::String(i) + ": " + testParam->choices[i]);
        }
    }
    
    // Initialize all slots with bypass engines
    DBG("Initializing " + juce::String(NUM_SLOTS) + " slots with bypass engines");
    for (int i = 0; i < NUM_SLOTS; ++i) {
        DBG("Creating engine for slot " + juce::String(i));
        m_activeEngines[i] = EngineFactory::createEngine(ENGINE_BYPASS);
        if (m_activeEngines[i]) {
            DBG("  Successfully created engine for slot " + juce::String(i));
        } else {
            DBG("  ERROR: Failed to create engine for slot " + juce::String(i));
        }
    }
    
    // Add parameter change listeners for all slots
    for (int i = 1; i <= NUM_SLOTS; ++i) {
        parameters.addParameterListener("slot" + juce::String(i) + "_engine", this);
    }
    
    // Start AI server
    startAIServer();
}

ChimeraAudioProcessor::~ChimeraAudioProcessor() {
    // Remove parameter listeners for all slots
    for (int i = 1; i <= NUM_SLOTS; ++i) {
        parameters.removeParameterListener("slot" + juce::String(i) + "_engine", this);
    }
    
    // Stop AI server
    stopAIServer();
}

void ChimeraAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_samplesPerBlock = samplesPerBlock;
    
    for (auto& engine : m_activeEngines) {
        if (engine) {
            engine->prepareToPlay(sampleRate, samplesPerBlock);
        }
    }
}

void ChimeraAudioProcessor::releaseResources() {
}

bool ChimeraAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void ChimeraAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    
    // Update engine parameters for all slots
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        updateEngineParameters(slot);
    }
    
    // Process through each slot in series
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        bool isBypassed = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_bypass")->load() > 0.5f;
        
        if (!isBypassed && m_activeEngines[slot]) {
            // Check if this is actually a bypass engine (ID == -1)
            auto* engineParam = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_engine");
            int engineChoice = static_cast<int>(engineParam->load());
            
            // Skip bypass engines (choice index 0)
            if (engineChoice == 0) {
                continue;
            }
            
            // Process through the engine
            m_activeEngines[slot]->process(buffer);
            
            // Apply gain compensation only after processing to prevent buildup
            // but only if we actually processed something
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                auto* data = buffer.getWritePointer(ch);
                for (int s = 0; s < buffer.getNumSamples(); ++s) {
                    data[s] *= 0.9f; // Gentle gain reduction to prevent clipping
                }
            }
        }
    }
    
    // Apply output limiting to prevent clipping and distortion
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float& sampleValue = channelData[sample];
            
            // Soft clipping to prevent harsh distortion
            if (std::abs(sampleValue) > 0.95f) {
                sampleValue = std::tanh(sampleValue * 0.7f) * 1.3f;
            }
            
            // Hard limit at -0.5dB to prevent digital clipping
            sampleValue = juce::jlimit(-0.95f, 0.95f, sampleValue);
        }
    }
    
    // Calculate output level for metering (after limiting)
    float maxLevel = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto* channelData = buffer.getReadPointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float absValue = std::abs(channelData[sample]);
            if (absValue > maxLevel) {
                maxLevel = absValue;
            }
        }
    }
    
    // Update the atomic level (only if it's higher than current)
    float currentLevel = m_currentOutputLevel.load();
    if (maxLevel > currentLevel) {
        m_currentOutputLevel.store(maxLevel);
    }
}

juce::AudioProcessorEditor* ChimeraAudioProcessor::createEditor() {
    return new ChimeraAudioProcessorEditor(*this);
}

void ChimeraAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ChimeraAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(parameters.state.getType())) {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

void ChimeraAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue) {
    // Check if it's an engine selector parameter
    for (int slot = 1; slot <= NUM_SLOTS; ++slot) {
        if (parameterID == "slot" + juce::String(slot) + "_engine") {
            int choiceIndex = static_cast<int>(newValue);
            int engineID = choiceIndexToEngineID(choiceIndex);
            
            DBG("Engine parameter changed: " + parameterID + 
                " choice index=" + juce::String(choiceIndex) + 
                " -> engine ID=" + juce::String(engineID));
            
            loadEngine(slot - 1, engineID);
            break;
        }
    }
}

void ChimeraAudioProcessor::loadEngine(int slot, int engineID) {
    DBG("Loading engine ID " + juce::String(engineID) + " into slot " + juce::String(slot));
    
    m_activeEngines[slot] = EngineFactory::createEngine(engineID);
    if (m_activeEngines[slot]) {
        m_activeEngines[slot]->prepareToPlay(m_sampleRate, m_samplesPerBlock);
        
        // Apply default parameters for this engine
        applyDefaultParameters(slot, engineID);
        
        updateEngineParameters(slot);
        
        DBG("Successfully loaded engine into slot " + juce::String(slot));
    } else {
        DBG("ERROR: Failed to create engine for ID " + juce::String(engineID));
    }
}

void ChimeraAudioProcessor::applyDefaultParameters(int slot, int engineID) {
    // Set safe default parameters for each engine type
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
    
    // Initialize all parameters to safe center values
    for (int i = 1; i <= 10; ++i) {
        auto paramID = slotPrefix + juce::String(i);
        if (auto* param = parameters.getParameter(paramID)) {
            param->setValueNotifyingHost(0.5f); // Center/neutral position
        }
    }
    
    // Engine-specific safe defaults to prevent static/noise
    // NOTE: Parameters are 1-based in UI but 0-based in engine (param1 -> index 0)
    switch (engineID) {
        case ENGINE_BYPASS:
            // No parameters needed
            break;
            
        case ENGINE_CLASSIC_COMPRESSOR:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.7f); // Threshold (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.3f); // Ratio (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.2f); // Attack (index 2)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.4f); // Release (index 3)
            break;
            
        case ENGINE_TAPE_ECHO:
        case ENGINE_BUCKET_BRIGADE_DELAY:
        case ENGINE_DIGITAL_DELAY:
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.3f); // Feedback (index 2)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.5f); // Mix (index 3)
            break;
            
        case ENGINE_PLATE_REVERB:
        case ENGINE_SHIMMER_REVERB:
        case ENGINE_SPRING_REVERB:
            // PlateReverb parameters: 0=Size, 1=Damping, 2=Predelay, 3=Mix
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.5f); // Size (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.5f); // Damping (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.1f); // Predelay (index 2)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.5f); // Mix (index 3) - 50% for testing
            break;
            
        case ENGINE_BIT_CRUSHER:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.9f); // Bit depth (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.9f); // Sample rate (index 1)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.3f); // Mix (index 3)
            break;
            
        case ENGINE_CHAOS_GENERATOR:
        case ENGINE_SPECTRAL_FREEZE:
        case ENGINE_GRANULAR_CLOUD:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.1f); // Minimal effect
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.3f); // Mix (index 3)
            break;
            
        case ENGINE_K_STYLE:
        case ENGINE_RODENT_DISTORTION:
        case ENGINE_MUFF_FUZZ:
            // Distortion engines typically have Drive, Tone, Level, Mix
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.5f); // Drive (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.5f); // Tone (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.5f); // Level (index 2)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.5f); // Mix (index 3)
            break;
            
        case ENGINE_LADDER_FILTER:
        case ENGINE_STATE_VARIABLE_FILTER:
        case ENGINE_FORMANT_FILTER:
            // Filter engines: Cutoff, Resonance, Type/Mode, Mix
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.5f); // Cutoff (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.3f); // Resonance (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.5f); // Type (index 2)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.7f); // Mix (index 3)
            break;
            
        default:
            // For all other engines, set reasonable defaults
            if (auto* mixParam = parameters.getParameter(slotPrefix + "4")) {
                mixParam->setValueNotifyingHost(0.5f); // 50% mix at param4 (index 3)
            }
            break;
    }
    
    DBG("Applied default parameters for engine " + juce::String(engineID) + " in slot " + juce::String(slot));
}

void ChimeraAudioProcessor::updateEngineParameters(int slot) {
    if (!m_activeEngines[slot]) return;
    
    std::map<int, float> params;
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
    
    for (int i = 0; i < 10; ++i) {
        auto paramID = slotPrefix + juce::String(i + 1);
        float value = parameters.getRawParameterValue(paramID)->load();
        params[i] = value;
    }
    
    m_activeEngines[slot]->updateParameters(params);
}


void ChimeraAudioProcessor::startAIServer() {
    // Only start if not already running
    if (m_aiServerProcess && m_aiServerProcess->isRunning()) {
        return;
    }
    
    m_aiServerProcess = std::make_unique<juce::ChildProcess>();
    
    // Try multiple paths to find the AI server
    juce::File aiServerDir;
    
    // Path 1: Development path - absolute path to AI server
    aiServerDir = juce::File("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server");
    
    if (!aiServerDir.exists()) {
        // Path 2: Relative to plugin bundle
        juce::File pluginFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        aiServerDir = pluginFile.getParentDirectory().getParentDirectory().getParentDirectory()
                               .getChildFile("AI_Server");
    }
    
    if (!aiServerDir.exists()) {
        // Path 3: In user's Application Support
        aiServerDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("Chimera/AI_Server");
    }
    
    if (aiServerDir.exists()) {
        // Build the command to start the server
        // Try to find Python 3 - prioritize the one with packages installed
        juce::String pythonPath;
        if (juce::File("/Library/Frameworks/Python.framework/Versions/3.10/bin/python3").existsAsFile()) {
            pythonPath = "/Library/Frameworks/Python.framework/Versions/3.10/bin/python3";
        } else if (juce::File("/usr/local/bin/python3").existsAsFile()) {
            pythonPath = "/usr/local/bin/python3";
        } else if (juce::File("/usr/bin/python3").existsAsFile()) {
            pythonPath = "/usr/bin/python3";
        } else {
            // Try to find Python via which command
            juce::ChildProcess whichPython;
            if (whichPython.start("which python3")) {
                pythonPath = whichPython.readAllProcessOutput().trim();
            }
        }
        
        if (pythonPath.isEmpty()) {
            juce::Logger::writeToLog("Python 3 not found - AI Server cannot start");
            return;
        }
        
        // Use the main.py script directly
        juce::String mainScript = aiServerDir.getChildFile("main.py").getFullPathName();
        
        // Create environment variables
        juce::StringPairArray environment;
        environment.set("PYTHONPATH", aiServerDir.getFullPathName());
        
        // Pass through OpenAI API key if available
        if (auto* apiKey = std::getenv("OPENAI_API_KEY")) {
            environment.set("OPENAI_API_KEY", apiKey);
            juce::Logger::writeToLog("Found OPENAI_API_KEY in environment");
        } else {
            // Try to load from .env file
            juce::File envFile = aiServerDir.getChildFile(".env");
            if (envFile.existsAsFile()) {
                juce::String envContent = envFile.loadFileAsString();
                if (envContent.contains("OPENAI_API_KEY=")) {
                    juce::Logger::writeToLog("Found .env file with API key");
                }
            }
        }
        
        // Start the server with proper working directory
        juce::Logger::writeToLog("Starting AI Server with Python at: " + pythonPath);
        juce::Logger::writeToLog("AI Server script: " + mainScript);
        
        // Start the server - uvicorn will be started by main.py
        // Check if server is already running
        juce::URL healthCheck("http://localhost:8000/health");
        if (auto healthStream = healthCheck.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                             .withConnectionTimeoutMs(500))) {
            juce::Logger::writeToLog("AI Server already running");
            return;
        }
        
        juce::Logger::writeToLog("Starting AI Server command: " + pythonPath + " " + mainScript);
        
        if (m_aiServerProcess->start(pythonPath + " " + mainScript, 
                                    juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr)) {
            // Give the server time to start
            juce::Thread::sleep(3000);
            
            // Check if it's still running
            if (m_aiServerProcess->isRunning()) {
                juce::Logger::writeToLog("AI Server started successfully");
            } else {
                // Try to get error output
                auto errorOutput = m_aiServerProcess->readAllProcessOutput();
                juce::Logger::writeToLog("AI Server failed to start");
                if (errorOutput.isNotEmpty()) {
                    juce::Logger::writeToLog("Error output: " + errorOutput);
                }
            }
        } else {
            juce::Logger::writeToLog("Failed to launch AI Server process");
        }
    } else {
        juce::Logger::writeToLog("AI Server directory not found at any expected location");
    }
}

void ChimeraAudioProcessor::stopAIServer() {
    if (m_aiServerProcess && m_aiServerProcess->isRunning()) {
        m_aiServerProcess->kill();
        m_aiServerProcess.reset();
        // AI Server stopped
    }
}

void ChimeraAudioProcessor::runEngineTests() {
    DBG("Starting engine tests...");
    
    // Run the tests
    auto summary = EngineTestRunner::runAllTests();
    
    // Print to console
    EngineTestRunner::printConsoleReport(summary);
    
    // Generate HTML report
    juce::File desktop = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
    juce::File reportFile = desktop.getChildFile("chimera_engine_test_report.html");
    EngineTestRunner::generateHTMLReport(summary, reportFile);
    
    DBG("Test report saved to: " + reportFile.getFullPathName());
    
    // Open the report in browser
    reportFile.startAsProcess();
}

int ChimeraAudioProcessor::engineIDToChoiceIndex(int engineID) {
    auto it = engineIDToChoiceMap.find(engineID);
    if (it != engineIDToChoiceMap.end()) {
        return it->second;
    }
    
    // Log error for debugging
    DBG("ERROR: Unknown engine ID " + juce::String(engineID) + " - defaulting to Bypass");
    jassertfalse; // This should never happen in production
    return 0; // Default to "Bypass" if not found
}

int ChimeraAudioProcessor::choiceIndexToEngineID(int choiceIndex) {
    auto it = choiceToEngineIDMap.find(choiceIndex);
    if (it != choiceToEngineIDMap.end()) {
        return it->second;
    }
    
    // Log error for debugging
    DBG("ERROR: Unknown choice index " + juce::String(choiceIndex) + " - defaulting to ENGINE_BYPASS");
    jassertfalse; // This should never happen in production
    return ENGINE_BYPASS; // Default to bypass if not found
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new ChimeraAudioProcessor();
}