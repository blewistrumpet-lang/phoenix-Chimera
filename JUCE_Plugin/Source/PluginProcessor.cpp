#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "EngineFactory.h"
#include "DefaultParameterValues.h"

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
                                   "Spring Reverb", "Resonant Chorus"};
    
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
    // Initialize all slots with bypass engines
    for (int i = 0; i < NUM_SLOTS; ++i) {
        m_activeEngines[i] = EngineFactory::createEngine(ENGINE_BYPASS);
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
            m_activeEngines[slot]->process(buffer);
        }
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
            loadEngine(slot - 1, static_cast<int>(newValue) - 1); // -1 because Bypass is at index 0
            break;
        }
    }
}

void ChimeraAudioProcessor::loadEngine(int slot, int engineID) {
    m_activeEngines[slot] = EngineFactory::createEngine(engineID);
    if (m_activeEngines[slot]) {
        m_activeEngines[slot]->prepareToPlay(m_sampleRate, m_samplesPerBlock);
        
        // Apply default parameters for this engine
        applyDefaultParameters(slot, engineID);
        
        updateEngineParameters(slot);
    }
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

void ChimeraAudioProcessor::applyDefaultParameters(int slot, int engineID) {
    // Get the default parameters for this engine
    auto defaults = DefaultParameterValues::getDefaultParameters(engineID);
    
    // Apply each default parameter to the plugin's parameter state
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
    
    for (const auto& [paramIndex, defaultValue] : defaults) {
        // Only apply to valid parameter indices (0-9)
        if (paramIndex >= 0 && paramIndex < 10) {
            auto paramID = slotPrefix + juce::String(paramIndex + 1);
            auto param = parameters.getParameter(paramID);
            if (param) {
                // Set the parameter value directly
                param->setValueNotifyingHost(defaultValue);
            }
        }
    }
}

void ChimeraAudioProcessor::startAIServer() {
    // Only start if not already running
    if (m_aiServerProcess && m_aiServerProcess->isRunning()) {
        return;
    }
    
    m_aiServerProcess = std::make_unique<juce::ChildProcess>();
    
    // Try multiple paths to find the AI server
    juce::File aiServerDir;
    
    // Path 1: Development path - relative to current plugin location
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
        // Try to find Python 3
        juce::String pythonPath;
        if (juce::File("/usr/bin/python3").existsAsFile()) {
            pythonPath = "/usr/bin/python3";
        } else if (juce::File("/usr/local/bin/python3").existsAsFile()) {
            pythonPath = "/usr/local/bin/python3";
        } else if (juce::File("/Library/Frameworks/Python.framework/Versions/3.10/bin/python3").existsAsFile()) {
            pythonPath = "/Library/Frameworks/Python.framework/Versions/3.10/bin/python3";
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
        
        juce::String mainScript = aiServerDir.getChildFile("main.py").getFullPathName();
        
        // Create environment variables
        juce::StringPairArray environment;
        environment.set("PYTHONPATH", aiServerDir.getFullPathName());
        
        // Start the server with proper working directory
        juce::Logger::writeToLog("Starting AI Server with Python at: " + pythonPath);
        juce::Logger::writeToLog("AI Server script: " + mainScript);
        
        if (m_aiServerProcess->start(pythonPath + " " + mainScript.quoted(), 
                                    juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr)) {
            // Give the server a moment to start
            juce::Thread::sleep(1000);
            
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new ChimeraAudioProcessor();
}