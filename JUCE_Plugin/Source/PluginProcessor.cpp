#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "EngineFactory.h"
#include "DefaultParameterValues.h"
#include "EngineTypes.h"
// #include "EngineTestRunner.h"  // Commented out for plugin build
#include "QuickEngineDiagnostic.h"
#include "QuickProcessingTest.h"

// Engine ID to Choice Index mapping table - NEW SIMPLIFIED SYSTEM
// Direct 1:1 mapping where engine ID = dropdown index (0-56)
static const std::map<int, int> engineIDToChoiceMap = {
    {0, 0},   // ENGINE_NONE -> "None" at index 0
    {1, 1},   // ENGINE_OPTO_COMPRESSOR -> "Vintage Opto Compressor" at index 1
    {2, 2},   // ENGINE_VCA_COMPRESSOR -> "Classic Compressor" at index 2
    {3, 3},   // ENGINE_TRANSIENT_SHAPER -> "Transient Shaper" at index 3
    {4, 4},   // ENGINE_NOISE_GATE -> "Noise Gate" at index 4
    {5, 5},   // ENGINE_MASTERING_LIMITER -> "Mastering Limiter" at index 5
    {6, 6},   // ENGINE_DYNAMIC_EQ -> "Dynamic EQ" at index 6
    {7, 7},   // ENGINE_PARAMETRIC_EQ -> "Parametric EQ" at index 7
    {8, 8},   // ENGINE_VINTAGE_CONSOLE_EQ -> "Vintage Console EQ" at index 8
    {9, 9},   // ENGINE_LADDER_FILTER -> "Ladder Filter" at index 9
    {10, 10}, // ENGINE_STATE_VARIABLE_FILTER -> "State Variable Filter" at index 10
    {11, 11}, // ENGINE_FORMANT_FILTER -> "Formant Filter" at index 11
    {12, 12}, // ENGINE_ENVELOPE_FILTER -> "Envelope Filter" at index 12
    {13, 13}, // ENGINE_COMB_RESONATOR -> "Comb Resonator" at index 13
    {14, 14}, // ENGINE_VOCAL_FORMANT -> "Vocal Formant Filter" at index 14
    {15, 15}, // ENGINE_VINTAGE_TUBE -> "Vintage Tube Preamp" at index 15
    {16, 16}, // ENGINE_WAVE_FOLDER -> "Wave Folder" at index 16
    {17, 17}, // ENGINE_HARMONIC_EXCITER -> "Harmonic Exciter" at index 17
    {18, 18}, // ENGINE_BIT_CRUSHER -> "Bit Crusher" at index 18
    {19, 19}, // ENGINE_MULTIBAND_SATURATOR -> "Multiband Saturator" at index 19
    {20, 20}, // ENGINE_MUFF_FUZZ -> "Muff Fuzz" at index 20
    {21, 21}, // ENGINE_RODENT_DISTORTION -> "Rodent Distortion" at index 21
    {22, 22}, // ENGINE_K_STYLE -> "K-Style Overdrive" at index 22
    {23, 23}, // ENGINE_DIGITAL_CHORUS -> "Stereo Chorus" at index 23
    {24, 24}, // ENGINE_RESONANT_CHORUS -> "Resonant Chorus" at index 24
    {25, 25}, // ENGINE_ANALOG_PHASER -> "Analog Phaser" at index 25
    {26, 26}, // ENGINE_RING_MODULATOR -> "Ring Modulator" at index 26
    {27, 27}, // ENGINE_FREQUENCY_SHIFTER -> "Frequency Shifter" at index 27
    {28, 28}, // ENGINE_HARMONIC_TREMOLO -> "Harmonic Tremolo" at index 28
    {29, 29}, // ENGINE_CLASSIC_TREMOLO -> "Classic Tremolo" at index 29
    {30, 30}, // ENGINE_ROTARY_SPEAKER -> "Rotary Speaker" at index 30
    {31, 31}, // ENGINE_PITCH_SHIFTER -> "Pitch Shifter" at index 31
    {32, 32}, // ENGINE_DETUNE_DOUBLER -> "Detune Doubler" at index 32
    {33, 33}, // ENGINE_INTELLIGENT_HARMONIZER -> "Intelligent Harmonizer" at index 33
    {34, 34}, // ENGINE_TAPE_ECHO -> "Tape Echo" at index 34
    {35, 35}, // ENGINE_DIGITAL_DELAY -> "Digital Delay" at index 35
    {36, 36}, // ENGINE_MAGNETIC_DRUM_ECHO -> "Magnetic Drum Echo" at index 36
    {37, 37}, // ENGINE_BUCKET_BRIGADE_DELAY -> "Bucket Brigade Delay" at index 37
    {38, 38}, // ENGINE_BUFFER_REPEAT -> "Buffer Repeat" at index 38
    {39, 39}, // ENGINE_PLATE_REVERB -> "Plate Reverb" at index 39
    {40, 40}, // ENGINE_SPRING_REVERB -> "Spring Reverb" at index 40
    {41, 41}, // ENGINE_CONVOLUTION_REVERB -> "Convolution Reverb" at index 41
    {42, 42}, // ENGINE_SHIMMER_REVERB -> "Shimmer Reverb" at index 42
    {43, 43}, // ENGINE_GATED_REVERB -> "Gated Reverb" at index 43
    {44, 44}, // ENGINE_STEREO_WIDENER -> "Stereo Widener" at index 44
    {45, 45}, // ENGINE_STEREO_IMAGER -> "Stereo Imager" at index 45
    {46, 46}, // ENGINE_DIMENSION_EXPANDER -> "Dimension Expander" at index 46
    {47, 47}, // ENGINE_SPECTRAL_FREEZE -> "Spectral Freeze" at index 47
    {48, 48}, // ENGINE_SPECTRAL_GATE -> "Spectral Gate" at index 48
    {49, 49}, // ENGINE_PHASED_VOCODER -> "Phased Vocoder" at index 49
    {50, 50}, // ENGINE_GRANULAR_CLOUD -> "Granular Cloud" at index 50
    {51, 51}, // ENGINE_CHAOS_GENERATOR -> "Chaos Generator" at index 51
    {52, 52}, // ENGINE_FEEDBACK_NETWORK -> "Feedback Network" at index 52
    {53, 53}, // ENGINE_MID_SIDE_PROCESSOR -> "Mid-Side Processor" at index 53
    {54, 54}, // ENGINE_GAIN_UTILITY -> "Gain Utility" at index 54
    {55, 55}, // ENGINE_MONO_MAKER -> "Mono Maker" at index 55
    {56, 56}  // ENGINE_PHASE_ALIGN -> "Phase Align" at index 56
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
    
    // Engine choices array - NEW SYSTEM: 57 entries (0-56) matching engine IDs directly
    juce::StringArray engineChoices{
        "None",                         // 0  = ENGINE_NONE
        "Vintage Opto Compressor",      // 1  = ENGINE_OPTO_COMPRESSOR
        "Classic Compressor",           // 2  = ENGINE_VCA_COMPRESSOR
        "Transient Shaper",             // 3  = ENGINE_TRANSIENT_SHAPER
        "Noise Gate",                   // 4  = ENGINE_NOISE_GATE
        "Mastering Limiter",            // 5  = ENGINE_MASTERING_LIMITER
        "Dynamic EQ",                   // 6  = ENGINE_DYNAMIC_EQ
        "Parametric EQ",                // 7  = ENGINE_PARAMETRIC_EQ
        "Vintage Console EQ",           // 8  = ENGINE_VINTAGE_CONSOLE_EQ
        "Ladder Filter",                // 9  = ENGINE_LADDER_FILTER
        "State Variable Filter",        // 10 = ENGINE_STATE_VARIABLE_FILTER
        "Formant Filter",               // 11 = ENGINE_FORMANT_FILTER
        "Envelope Filter",              // 12 = ENGINE_ENVELOPE_FILTER
        "Comb Resonator",               // 13 = ENGINE_COMB_RESONATOR
        "Vocal Formant Filter",         // 14 = ENGINE_VOCAL_FORMANT
        "Vintage Tube Preamp",          // 15 = ENGINE_VINTAGE_TUBE
        "Wave Folder",                  // 16 = ENGINE_WAVE_FOLDER
        "Harmonic Exciter",             // 17 = ENGINE_HARMONIC_EXCITER
        "Bit Crusher",                  // 18 = ENGINE_BIT_CRUSHER
        "Multiband Saturator",          // 19 = ENGINE_MULTIBAND_SATURATOR
        "Muff Fuzz",                    // 20 = ENGINE_MUFF_FUZZ
        "Rodent Distortion",            // 21 = ENGINE_RODENT_DISTORTION
        "K-Style Overdrive",            // 22 = ENGINE_K_STYLE
        "Stereo Chorus",                // 23 = ENGINE_DIGITAL_CHORUS
        "Resonant Chorus",              // 24 = ENGINE_RESONANT_CHORUS
        "Analog Phaser",                // 25 = ENGINE_ANALOG_PHASER
        "Ring Modulator",               // 26 = ENGINE_RING_MODULATOR
        "Frequency Shifter",            // 27 = ENGINE_FREQUENCY_SHIFTER
        "Harmonic Tremolo",             // 28 = ENGINE_HARMONIC_TREMOLO
        "Classic Tremolo",              // 29 = ENGINE_CLASSIC_TREMOLO
        "Rotary Speaker",               // 30 = ENGINE_ROTARY_SPEAKER
        "Pitch Shifter",                // 31 = ENGINE_PITCH_SHIFTER
        "Detune Doubler",               // 32 = ENGINE_DETUNE_DOUBLER
        "Intelligent Harmonizer",       // 33 = ENGINE_INTELLIGENT_HARMONIZER
        "Tape Echo",                    // 34 = ENGINE_TAPE_ECHO
        "Digital Delay",                // 35 = ENGINE_DIGITAL_DELAY
        "Magnetic Drum Echo",           // 36 = ENGINE_MAGNETIC_DRUM_ECHO
        "Bucket Brigade Delay",         // 37 = ENGINE_BUCKET_BRIGADE_DELAY
        "Buffer Repeat",                // 38 = ENGINE_BUFFER_REPEAT
        "Plate Reverb",                 // 39 = ENGINE_PLATE_REVERB
        "Spring Reverb",                // 40 = ENGINE_SPRING_REVERB
        "Convolution Reverb",           // 41 = ENGINE_CONVOLUTION_REVERB
        "Shimmer Reverb",               // 42 = ENGINE_SHIMMER_REVERB
        "Gated Reverb",                 // 43 = ENGINE_GATED_REVERB
        "Stereo Widener",               // 44 = ENGINE_STEREO_WIDENER
        "Stereo Imager",                // 45 = ENGINE_STEREO_IMAGER
        "Dimension Expander",           // 46 = ENGINE_DIMENSION_EXPANDER
        "Spectral Freeze",              // 47 = ENGINE_SPECTRAL_FREEZE
        "Spectral Gate",                // 48 = ENGINE_SPECTRAL_GATE
        "Phased Vocoder",               // 49 = ENGINE_PHASED_VOCODER
        "Granular Cloud",               // 50 = ENGINE_GRANULAR_CLOUD
        "Chaos Generator",              // 51 = ENGINE_CHAOS_GENERATOR
        "Feedback Network",             // 52 = ENGINE_FEEDBACK_NETWORK
        "Mid-Side Processor",           // 53 = ENGINE_MID_SIDE_PROCESSOR
        "Gain Utility",                 // 54 = ENGINE_GAIN_UTILITY
        "Mono Maker",                   // 55 = ENGINE_MONO_MAKER
        "Phase Align"                   // 56 = ENGINE_PHASE_ALIGN
    };
    
    // Create parameters for all 6 slots
    for (int slot = 1; slot <= 6; ++slot) {
        juce::String slotStr = juce::String(slot);
        
        // Slot parameters (15 per slot to accommodate all engine parameters)
        for (int i = 0; i < 15; ++i) {
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
    
    // Run audio processing test in debug builds
    #ifdef DEBUG
    DBG("Running audio processing test...");
    QuickProcessingTest::runAllTests();
    #endif
    
    // Log the engine choice array for debugging
    auto* testParam = dynamic_cast<juce::AudioParameterChoice*>(parameters.getParameter("slot1_engine"));
    if (testParam) {
        DBG("Engine choices in dropdown:");
        for (int i = 0; i < testParam->choices.size(); ++i) {
            DBG("  Choice " + juce::String(i) + ": " + testParam->choices[i]);
        }
    }
    
    // Initialize all slots with null engines (no processing)
    DBG("Initializing " + juce::String(NUM_SLOTS) + " slots with null engines");
    for (int i = 0; i < NUM_SLOTS; ++i) {
        DBG("Setting null engine for slot " + juce::String(i));
        m_activeEngines[i] = nullptr;  // Use null for bypassed slots
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
    
    // Run diagnostic on first load (only once)
    // DISABLED - causing crashes
    // static bool diagnosticRun = false;
    // if (!diagnosticRun) {
    //     diagnosticRun = true;
    //     runComprehensiveDiagnostic();
    // }
    
    // Run isolated engine tests (temporary for debugging)
    // DISABLED - causing crashes
    // static bool testRun = false;
    // if (!testRun) {
    //     testRun = true;
    //     runIsolatedEngineTests();
    // }
    
    // Quick diagnostic test
    // DISABLED - for debugging only
    // static bool quickTestRun = false;
    // if (!quickTestRun) {
    //     quickTestRun = true;
    //     QuickEngineDiagnostic::runQuickTest();
    // }
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
    
    // Validate buffer size to prevent crashes
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0 || numSamples > 8192) {
        // Abnormal buffer size - skip processing
        buffer.clear();
        return;
    }
    
    // Update engine parameters for all slots
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        updateEngineParameters(slot);
    }
    
    // Process through each slot in series
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        bool isBypassed = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_bypass")->load() > 0.5f;
        
        if (!isBypassed) {
            // Thread-safe engine access
            std::unique_ptr<EngineBase> engineCopy;
            {
                std::lock_guard<std::mutex> lock(m_engineMutex);
                if (m_activeEngines[slot]) {
                    // Check if this is the None engine (ID == 0)
                    auto* engineParam = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_engine");
                    int engineChoice = static_cast<int>(engineParam->load());
                    
                    // Skip None engines (engine ID 0)
                    if (engineChoice == 0) {
                        continue;
                    }
                    
                    // Process through the engine (still under lock)
                    m_activeEngines[slot]->process(buffer);
                }
            }
        }
    }
    
    // Apply gentle gain compensation once at the end to prevent buildup
    // Only apply if any processing occurred
    bool anyProcessingOccurred = false;
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        bool isBypassed = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_bypass")->load() > 0.5f;
        auto* engineParam = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_engine");
        int engineChoice = static_cast<int>(engineParam->load());
        
        if (!isBypassed && m_activeEngines[slot] && engineChoice != 0) {
            anyProcessingOccurred = true;
            break;
        }
    }
    
    if (anyProcessingOccurred) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int s = 0; s < buffer.getNumSamples(); ++s) {
                data[s] *= 0.99f; // Minimal gain reduction (1% max)
            }
        }
    }
    
    // Apply output limiting to prevent clipping and distortion
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float& sampleValue = channelData[sample];
            
            // Soft clipping to prevent harsh distortion
            if (std::abs(sampleValue) > 0.98f) {
                sampleValue = std::tanh(sampleValue * 0.7f) * 1.3f;
            }
            
            // Hard limit at -0.2dB to prevent digital clipping
            sampleValue = juce::jlimit(-0.98f, 0.98f, sampleValue);
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
    
    // Runtime assertion to guarantee only valid engines are loaded
    jassert(engineID >= 0 && engineID < ENGINE_COUNT);
    if (engineID < 0 || engineID >= ENGINE_COUNT) {
        DBG("ERROR: Invalid engine ID " + juce::String(engineID) + " - using ENGINE_NONE");
        engineID = ENGINE_NONE;
    }
    
    // Create and prepare engine outside of critical section
    std::unique_ptr<EngineBase> newEngine = EngineFactory::createEngine(engineID);
    if (newEngine) {
        newEngine->prepareToPlay(m_sampleRate, m_samplesPerBlock);
        
        // Apply default parameters for this engine
        applyDefaultParameters(slot, engineID);
        
        // Lock only for the actual swap
        {
            std::lock_guard<std::mutex> lock(m_engineMutex);
            m_activeEngines[slot] = std::move(newEngine);
        }
        
        updateEngineParameters(slot);
        
        DBG("Successfully loaded engine into slot " + juce::String(slot));
    } else {
        std::lock_guard<std::mutex> lock(m_engineMutex);
        m_activeEngines[slot].reset();
        DBG("ERROR: Failed to create engine for ID " + juce::String(engineID));
    }
}

void ChimeraAudioProcessor::applyDefaultParameters(int slot, int engineID) {
    // Set safe default parameters for each engine type
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
    
    // Initialize all parameters to safe center values
    for (int i = 1; i <= 15; ++i) {
        auto paramID = slotPrefix + juce::String(i);
        if (auto* param = parameters.getParameter(paramID)) {
            param->setValueNotifyingHost(0.5f); // Center/neutral position
        }
    }
    
    // Get the correct Mix parameter index for this engine
    int mixIndex = getMixParameterIndex(engineID);
    juce::String mixParamID = mixIndex >= 0 ? slotPrefix + juce::String(mixIndex + 1) : ""; // Convert 0-based to 1-based, handle -1 case
    
    // Engine-specific safe defaults to prevent static/noise
    // NOTE: Parameters are 1-based in UI but 0-based in engine (param1 -> index 0)
    switch (engineID) {
            
        case ENGINE_VCA_COMPRESSOR: // Same as ENGINE_CLASSIC_COMPRESSOR
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.7f); // Threshold (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.3f); // Ratio (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.2f); // Attack (index 2)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.4f); // Release (index 3)
            // Set Mix at correct index
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(1.0f); // Full wet for compressors
            }
            break;
            
        case ENGINE_TRANSIENT_SHAPER:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.5f); // Attack (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.5f); // Sustain (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.3f); // Attack Time (index 2)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.3f); // Release Time (index 3)
            // Mix is at index 9 (parameter 10)
            parameters.getParameter(slotPrefix + "10")->setValueNotifyingHost(1.0f); // Mix at index 9 - CRITICAL FIX
            break;
            
        case ENGINE_MAGNETIC_DRUM_ECHO:
        case ENGINE_BUFFER_REPEAT:
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.4f); // Delay time
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.3f); // Feedback (index 2)
            // Set Mix at correct index (should be index 3 for these)
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(0.8f); // 80% mix for delays
            }
            break;
            
        case ENGINE_BUCKET_BRIGADE_DELAY:
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.4f); // Delay time
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.3f); // Feedback
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.2f); // Modulation
            parameters.getParameter(slotPrefix + "5")->setValueNotifyingHost(0.3f); // Tone
            // Set Mix at correct index (should be index 5 for BBD)
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(0.8f); // Higher mix for analog delay
            }
            break;
            
        case ENGINE_PLATE_REVERB:
        case ENGINE_CONVOLUTION_REVERB:
        case ENGINE_GATED_REVERB:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.5f); // Size (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.5f); // Damping (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.1f); // Predelay (index 2)
            // Set Mix at correct index (should be index 3 for reverbs)
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(0.8f); // Higher mix for reverbs
            }
            break;
            
        case ENGINE_BIT_CRUSHER:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.9f); // Bit depth (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.9f); // Sample rate (index 1)
            // Set Mix at correct index (should be index 3)
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(1.0f); // Full wet for bit crusher
            }
            break;
            
        case ENGINE_CHAOS_GENERATOR:
        case ENGINE_SPECTRAL_FREEZE:
        case ENGINE_GRANULAR_CLOUD:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.1f); // Minimal effect
            // Set Mix at correct index
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(0.8f); // Higher mix for experimental effects
            }
            break;
            
        case ENGINE_K_STYLE:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.3f); // Drive (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.5f); // Tone (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.5f); // Output (index 2)
            // Set Mix at correct index (should be index 3)
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(1.0f); // Full wet for overdrive
            }
            break;
            
        case ENGINE_RODENT_DISTORTION:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.4f); // Distortion
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.5f); // Filter
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.5f); // Output
            // Set Mix at correct index (should be index 5)
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(1.0f); // Full wet for distortion
            }
            break;
            
        case ENGINE_MUFF_FUZZ:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.5f); // Sustain
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.5f); // Tone
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.5f); // Volume
            // Set Mix at correct index (should be index 6)
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(1.0f); // Full wet for fuzz
            }
            break;
            
        case ENGINE_STEREO_CHORUS:
        case ENGINE_RESONANT_CHORUS:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.3f); // Rate
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.4f); // Depth
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.5f); // Voices/Width
            // Set Mix at correct index (should be index 5)
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(0.8f); // Higher mix for chorus
            }
            break;
            
        case ENGINE_ROTARY_SPEAKER:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.5f); // Speed
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.5f); // Horn/Drum Mix
            // Set Mix at correct index (should be index 5)
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(0.8f); // Higher mix for rotary
            }
            break;
            
        case ENGINE_LADDER_FILTER:
        case ENGINE_STATE_VARIABLE_FILTER:
        case ENGINE_FORMANT_FILTER:
        case ENGINE_ENVELOPE_FILTER:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.5f); // Cutoff (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.3f); // Resonance (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.5f); // Type (index 2)
            // Set Mix at correct index (should be index 3 for most filters)
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(1.0f); // Full wet for filters typically
            }
            break;
            
        case ENGINE_MULTIBAND_SATURATOR:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.2f); // Low drive
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.2f); // Mid drive
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.2f); // High drive
            // Set Mix at correct index (should be index 6)
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(1.0f); // Full wet for multiband
            }
            break;
            
        case ENGINE_HARMONIC_EXCITER:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.5f); // Frequency (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.3f); // Drive (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.5f); // Harmonics (index 2)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.5f); // Clarity (index 3)
            // Mix is at index 7 (parameter 8) - CRITICAL FIX
            parameters.getParameter(slotPrefix + "8")->setValueNotifyingHost(1.0f); // Mix at index 7
            break;
            
        case ENGINE_VINTAGE_TUBE:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.3f); // Drive (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.5f); // Bass (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.5f); // Mid (index 2)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.5f); // Treble (index 3)
            // Mix is at index 9 (parameter 10)
            parameters.getParameter(slotPrefix + "10")->setValueNotifyingHost(1.0f); // Mix at index 9
            break;
            
        case ENGINE_SHIMMER_REVERB:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.5f); // Size (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.4f); // Damping (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.5f); // Shimmer (index 2)
            // Mix is at index 9 (parameter 10)
            parameters.getParameter(slotPrefix + "10")->setValueNotifyingHost(0.8f); // Mix at index 9
            break;
            
        case ENGINE_SPRING_REVERB:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.5f); // Size (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.4f); // Damping (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.5f); // Tension (index 2)
            // Mix is at index 9 (parameter 10)
            parameters.getParameter(slotPrefix + "10")->setValueNotifyingHost(0.8f); // Mix at index 9
            break;
            
        case ENGINE_OPTO_COMPRESSOR:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.6f); // Threshold (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.4f); // Ratio (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.3f); // Attack (index 2)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.4f); // Release (index 3)
            // Mix is at index 5 (parameter 6)
            parameters.getParameter(slotPrefix + "6")->setValueNotifyingHost(1.0f); // Mix at index 5
            break;
            
        case ENGINE_NOISE_GATE:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.3f); // Threshold (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.8f); // Ratio (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.1f); // Attack (index 2)
            parameters.getParameter(slotPrefix + "4")->setValueNotifyingHost(0.3f); // Hold (index 3)
            parameters.getParameter(slotPrefix + "5")->setValueNotifyingHost(0.4f); // Release (index 4)
            // Mix is at index 6 (parameter 7)
            parameters.getParameter(slotPrefix + "7")->setValueNotifyingHost(1.0f); // Mix at index 6
            break;
            
        case ENGINE_MASTERING_LIMITER:
            parameters.getParameter(slotPrefix + "1")->setValueNotifyingHost(0.9f); // Ceiling (index 0)
            parameters.getParameter(slotPrefix + "2")->setValueNotifyingHost(0.4f); // Release (index 1)
            parameters.getParameter(slotPrefix + "3")->setValueNotifyingHost(0.0f); // Lookahead off (index 2)
            // Mix is at index 5 (parameter 6)
            parameters.getParameter(slotPrefix + "6")->setValueNotifyingHost(1.0f); // Mix at index 5
            break;
            
        default:
            // For all other engines, set Mix parameter at correct index
            if (auto* mixParam = parameters.getParameter(mixParamID)) {
                mixParam->setValueNotifyingHost(0.8f); // Default 80% mix
            }
            break;
    }
    
    DBG("Applied default parameters for engine " + juce::String(engineID) + " in slot " + juce::String(slot) + 
        " with Mix at param index " + (mixIndex >= 0 ? juce::String(mixIndex + 1) : "N/A"));
}

void ChimeraAudioProcessor::updateEngineParameters(int slot) {
    if (!m_activeEngines[slot]) return;
    
    std::map<int, float> params;
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
    
    for (int i = 0; i < 15; ++i) {
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
    // Test runner disabled for plugin build
    DBG("Engine test runner is disabled in plugin build");
    
    /* Commented out for plugin build - use standalone test harness instead
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
    */
}

int ChimeraAudioProcessor::engineIDToChoiceIndex(int engineID) {
    auto it = engineIDToChoiceMap.find(engineID);
    if (it != engineIDToChoiceMap.end()) {
        return it->second;
    }
    
    // Log error for debugging
    DBG("ERROR: Unknown engine ID " + juce::String(engineID) + " - defaulting to None");
    jassertfalse; // This should never happen in production
    return 0; // Default to "None" if not found
}

int ChimeraAudioProcessor::choiceIndexToEngineID(int choiceIndex) {
    auto it = choiceToEngineIDMap.find(choiceIndex);
    if (it != choiceToEngineIDMap.end()) {
        return it->second;
    }
    
    // Log error for debugging
    DBG("ERROR: Unknown choice index " + juce::String(choiceIndex) + " - defaulting to ENGINE_NONE");
    jassertfalse; // This should never happen in production
    return ENGINE_NONE; // Default to ENGINE_NONE if not found
}

int ChimeraAudioProcessor::getMixParameterIndex(int engineID) {
    // Returns the 0-based index of the Mix parameter for each engine
    // Based on comprehensive analysis of each engine's header file
    
    switch (engineID) {
        // Mix at index 2
        case ENGINE_FREQUENCY_SHIFTER:
        case ENGINE_PITCH_SHIFTER:
            return 2;
            
        // Mix at index 3  
        case ENGINE_K_STYLE:
            return 3;
            
        // Mix at index 4
        case ENGINE_CONVOLUTION_REVERB:
        case ENGINE_TAPE_ECHO:
        case ENGINE_DETUNE_DOUBLER:
            return 4;
            
        // Mix at index 5
        case ENGINE_OPTO_COMPRESSOR: // VintageOptoCompressor
        case ENGINE_MASTERING_LIMITER:
        case ENGINE_RODENT_DISTORTION:
            return 5;
            
        // Mix at index 6
        case ENGINE_NOISE_GATE:
        case ENGINE_BIT_CRUSHER:
        case ENGINE_VCA_COMPRESSOR: // ClassicCompressor - FIXED: Mix is at index 6, not 4
        case ENGINE_DIGITAL_DELAY:
        case ENGINE_BUCKET_BRIGADE_DELAY:
        case ENGINE_STEREO_CHORUS:
        case ENGINE_CLASSIC_TREMOLO:
        case ENGINE_HARMONIC_TREMOLO:
        case ENGINE_COMB_RESONATOR:
        case ENGINE_MUFF_FUZZ:
        case ENGINE_MULTIBAND_SATURATOR:
        case ENGINE_DYNAMIC_EQ:
            return 6;
            
        // Mix at index 7
        case ENGINE_ANALOG_PHASER:
        case ENGINE_ENVELOPE_FILTER:
        case ENGINE_STATE_VARIABLE_FILTER:
        case ENGINE_FORMANT_FILTER:
        case ENGINE_WAVE_FOLDER:
        case ENGINE_SPECTRAL_GATE:
        case ENGINE_HARMONIC_EXCITER: // CRITICAL FIX - was missing!
        case ENGINE_BUFFER_REPEAT:
        case ENGINE_STEREO_WIDENER:
        case ENGINE_STEREO_IMAGER:
        case ENGINE_DIMENSION_EXPANDER:
        case ENGINE_CHAOS_GENERATOR:
        case ENGINE_FEEDBACK_NETWORK:
        case ENGINE_INTELLIGENT_HARMONIZER:
            return 7;
            
        // Mix at index 8
        case ENGINE_GATED_REVERB:
        case ENGINE_MAGNETIC_DRUM_ECHO:
        case ENGINE_RESONANT_CHORUS:
        case ENGINE_LADDER_FILTER:
        case ENGINE_VOCAL_FORMANT:
        case ENGINE_PARAMETRIC_EQ:
            return 8;
            
        // Mix at index 7
        case ENGINE_SPRING_REVERB: // FIXED: Mix is at index 7, not 9
            return 7;
            
        // Mix at index 3
        case ENGINE_PLATE_REVERB: // FIXED: Mix is at index 3, not 6
            return 3;
            
        // Mix at index 9
        case ENGINE_SHIMMER_REVERB:
        case ENGINE_ROTARY_SPEAKER:
        case ENGINE_VINTAGE_TUBE:
        case ENGINE_TRANSIENT_SHAPER:
        case ENGINE_PHASE_ALIGN:
            return 9;
            
        // Mix at index 10
        case ENGINE_VINTAGE_CONSOLE_EQ:
            return 10;
            
        // Special cases - no mix parameter
        case ENGINE_GRANULAR_CLOUD:
        case ENGINE_RING_MODULATOR:
        case ENGINE_MID_SIDE_PROCESSOR:
        case ENGINE_GAIN_UTILITY:
        case ENGINE_MONO_MAKER:
        case ENGINE_SPECTRAL_FREEZE:
            return -1; // No mix parameter
            
        // Default case
        case ENGINE_NONE:
        default:
            return 6; // Default to index 6 for unknown engines
    }
}

// Comprehensive Engine Diagnostic
void ChimeraAudioProcessor::runComprehensiveDiagnostic()
{
    m_diagnosticResults.clear();
    
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    const int numChannels = 2;
    
    // Create test buffer
    juce::AudioBuffer<float> testBuffer(numChannels, blockSize);
    juce::AudioBuffer<float> originalBuffer(numChannels, blockSize);
    
    // Generate test signal (1kHz sine wave)
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* data = testBuffer.getWritePointer(ch);
        for (int i = 0; i < blockSize; ++i) {
            float phase = (float)i / blockSize * 2.0f * M_PI;
            data[i] = 0.5f * std::sin(1000.0f * phase / sampleRate * 2.0f * M_PI);
        }
    }
    
    DBG("=== CHIMERA ENGINE DIAGNOSTIC ===");
    DBG("Testing " << ENGINE_COUNT << " engines...");
    
    // Test each engine
    for (int engineID = 0; engineID < ENGINE_COUNT; ++engineID) {
        DiagnosticResult result;
        result.engineID = engineID;
        result.engineName = getEngineTypeName(engineID);
        result.passed = false;
        result.confidence = 0.0f;
        result.issues = "";
        
        if (engineID == ENGINE_NONE) {
            result.passed = true;
            result.confidence = 100.0f;
            result.issues = "None engine (bypassed)";
            m_diagnosticResults.push_back(result);
            continue;
        }
        
        try {
            // Create engine instance
            auto engine = EngineFactory::createEngine(engineID);
            if (!engine) {
                result.issues = "Failed to create engine";
                m_diagnosticResults.push_back(result);
                continue;
            }
            
            // Prepare engine
            engine->prepareToPlay(sampleRate, blockSize);
            engine->reset();
            
            // Set test parameters
            std::map<int, float> params;
            
            // Get mix parameter index for this engine
            int mixIndex = getMixParameterIndex(engineID);
            
            // Set all parameters to safe defaults
            for (int i = 0; i < 15; ++i) {
                params[i] = 0.5f;
            }
            
            // Set mix to 100% wet to ensure we hear the effect
            params[mixIndex] = 1.0f;
            
            // Set engine-specific test parameters
            switch (getEngineCategory(engineID)) {
                case 1: // Dynamics
                    params[0] = 0.3f; // Low threshold
                    params[1] = 0.7f; // High ratio
                    break;
                case 2: // Filters
                    params[0] = 0.5f; // Mid frequency
                    params[1] = 0.7f; // Moderate Q
                    break;
                case 3: // Distortion
                    params[0] = 0.7f; // High drive
                    params[1] = 0.5f; // Tone
                    break;
                case 4: // Modulation
                    params[0] = 0.5f; // Rate
                    params[1] = 0.7f; // Depth
                    break;
                case 5: // Reverb/Delay
                    params[0] = 0.5f; // Time/Size
                    params[1] = 0.3f; // Feedback/Damping
                    break;
            }
            
            engine->updateParameters(params);
            
            // Copy test buffer
            originalBuffer.makeCopyOf(testBuffer);
            
            // Process audio
            engine->process(testBuffer);
            
            // Analyze results
            float inputRMS = 0.0f, outputRMS = 0.0f;
            float maxDiff = 0.0f;
            
            for (int ch = 0; ch < numChannels; ++ch) {
                const float* original = originalBuffer.getReadPointer(ch);
                const float* processed = testBuffer.getReadPointer(ch);
                
                for (int i = 0; i < blockSize; ++i) {
                    inputRMS += original[i] * original[i];
                    outputRMS += processed[i] * processed[i];
                    maxDiff = std::max(maxDiff, std::abs(processed[i] - original[i]));
                }
            }
            
            inputRMS = std::sqrt(inputRMS / (blockSize * numChannels));
            outputRMS = std::sqrt(outputRMS / (blockSize * numChannels));
            
            // Determine if audio was modified
            bool audioModified = maxDiff > 0.001f || std::abs(outputRMS - inputRMS) > 0.001f;
            
            if (audioModified) {
                result.passed = true;
                result.confidence = std::min(100.0f, maxDiff * 200.0f); // Scale difference to confidence
                
                float gainDB = 20.0f * std::log10(outputRMS / (inputRMS + 0.00001f));
                result.issues = juce::String::formatted("RMS change: %.1f dB, Mix: %.0f%%", 
                    gainDB, params[mixIndex] * 100.0f);
            } else {
                result.passed = false;
                result.confidence = 0.0f;
                result.issues = "No audio modification detected";
            }
            
        } catch (const std::exception& e) {
            result.issues = "Exception: " + juce::String(e.what());
        } catch (...) {
            result.issues = "Unknown exception";
        }
        
        m_diagnosticResults.push_back(result);
        
        // Log result
        DBG(juce::String::formatted("[%d] %s: %s (%.0f%%) - %s",
            engineID,
            result.engineName.toRawUTF8(),
            result.passed ? "PASS" : "FAIL",
            result.confidence,
            result.issues.toRawUTF8()));
    }
    
    // Summary
    int passed = 0;
    int failed = 0;
    for (const auto& result : m_diagnosticResults) {
        if (result.engineID != ENGINE_NONE) {
            if (result.passed) passed++;
            else failed++;
        }
    }
    
    DBG("=== DIAGNOSTIC SUMMARY ===");
    DBG("Total engines tested: " << (ENGINE_COUNT - 1)); // Exclude NONE
    DBG("Passed: " << passed);
    DBG("Failed: " << failed);
    DBG("Pass rate: " << ((float)passed / (ENGINE_COUNT - 1) * 100.0f) << "%");
}

void ChimeraAudioProcessor::runIsolatedEngineTests() {
    DBG("=== ISOLATED ENGINE TESTS ===");
    DBG("Testing engines in complete isolation from plugin architecture");
    DBG("");
    
    // Also write to file for easier access
    juce::File testFile("/tmp/chimera_engine_test_results.txt");
    testFile.replaceWithText("=== ISOLATED ENGINE TESTS ===\n");
    
    // Test critical engines
    std::vector<int> testEngines = {0, 1, 2, 6, 11, 21, 31, 16, 26};
    
    for (int engineID : testEngines) {
        DBG("Testing Engine ID: " << engineID);
        
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            DBG("  FAILED to create engine!");
            continue;
        }
        
        DBG("  Name: " << engine->getName());
        
        // Prepare
        engine->prepareToPlay(44100.0, 512);
        
        // Create test buffer
        juce::AudioBuffer<float> buffer(2, 512);
        juce::AudioBuffer<float> original(2, 512);
        
        // Fill with test signal
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < 512; ++i) {
                data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
            }
        }
        original.makeCopyOf(buffer);
        
        // Calculate input level
        float inputLevel = 0;
        for (int ch = 0; ch < 2; ++ch) {
            const auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < 512; ++i) {
                inputLevel += std::abs(data[i]);
            }
        }
        inputLevel /= (2 * 512);
        
        // Set parameters for maximum effect
        std::map<int, float> params;
        params[0] = 0.8f;  // Main parameter high
        params[3] = 1.0f;  // Mix 100%
        params[5] = 1.0f;  // Alternate mix 100%
        params[6] = 1.0f;  // Alternate mix 100%
        params[7] = 1.0f;  // Alternate mix 100%
        
        engine->updateParameters(params);
        engine->process(buffer);
        
        // Calculate output level and difference
        float outputLevel = 0;
        float maxDiff = 0;
        for (int ch = 0; ch < 2; ++ch) {
            const auto* outData = buffer.getReadPointer(ch);
            const auto* inData = original.getReadPointer(ch);
            for (int i = 0; i < 512; ++i) {
                outputLevel += std::abs(outData[i]);
                maxDiff = std::max(maxDiff, std::abs(outData[i] - inData[i]));
            }
        }
        outputLevel /= (2 * 512);
        
        DBG("  Input level:  " << inputLevel);
        DBG("  Output level: " << outputLevel);
        DBG("  Max diff:     " << maxDiff);
        DBG("  Result: " << (maxDiff > 0.01f ? "WORKING ✅" : "NOT WORKING ❌"));
        DBG("");
        
        // Write to file
        juce::String result = juce::String::formatted("Engine %d (%s): Input=%.4f, Output=%.4f, Diff=%.4f - %s\n",
            engineID, engine->getName().toRawUTF8(), inputLevel, outputLevel, maxDiff,
            maxDiff > 0.01f ? "WORKING" : "NOT WORKING");
        testFile.appendText(result);
    }
    
    DBG("=== TESTS COMPLETE ===");
    DBG("");
    testFile.appendText("\n=== TESTS COMPLETE ===\n");
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new ChimeraAudioProcessor();
}