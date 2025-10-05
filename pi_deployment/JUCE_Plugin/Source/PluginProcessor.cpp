#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginEditor_Pi.h"
#include "PluginEditorSkunkworks.h"
#include "PluginEditorRefined.h"
#include "PluginEditorNexus.h"
#include "PluginEditorNexus_Final.h"
#include "PluginEditorNexusDynamic.h"
#include "PluginEditorNexusDynamicMinimal.h"
#include "PluginEditorNexusDynamicSafe.h"
#include "PluginEditorNexusStatic.h"
#include "PluginEditorBasic.h"
#include "PluginEditorBasicWithSelectors.h"
#include "PluginEditorWithOneAttachment.h"
#include "PluginEditorWithAllAttachments.h"
#include "PluginEditorStaticWithDynamic.h"
#include "PluginEditorSimpleFinal.h"
#include "PluginEditorComplete.h"
#include "PluginEditorTestBypass.h"
#include "PluginEditorWorking.h"
#include "PluginEditorFull.h"
#include "EngineFactory.h"
#include "UnifiedDefaultParameters.h"
#include "EngineTypes.h"
#include "AIServerManager.h"
// #include "EngineTestRunner.h"  // Commented out for plugin build
// #include "QuickEngineDiagnostic.h" // Removed - file was moved to tests
// #include "QuickProcessingTest.h" // Removed - file was moved to tests

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
        
        // Mix control (0 = dry, 1 = wet)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "slot" + slotStr + "_mix",
            "Slot " + slotStr + " Mix",
            0.0f, 1.0f, 1.0f));  // Default to fully wet
        
        // Solo switch
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "slot" + slotStr + "_solo",
            "Slot " + slotStr + " Solo",
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
    
    // Auto-start the TRUE Trinity AI server
    DBG("Starting TRUE Trinity AI Server...");
    AIServerManager::getInstance().startServerIfNeeded();
    
    // Check server status
    if (AIServerManager::getInstance().isServerHealthy()) {
        DBG("✅ TRUE Trinity server is running and healthy!");
    } else {
        DBG("⚠️ TRUE Trinity server could not be started. AI features may be limited.");
    }
    
    // Diagnostic tests removed - production build should not run tests
    // All engine testing has been moved to standalone test harnesses
    
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
        m_activeEngines[i] = nullptr;  // Start with null engines (bypassed/empty slots)
        // This is intentional - slots start empty and engines are loaded on demand
        m_slotActivityLevels[i].store(0.0f);  // Initialize activity levels
    }
    
    // Add parameter change listeners for all slots
    for (int i = 1; i <= NUM_SLOTS; ++i) {
        // Listen for engine changes
        parameters.addParameterListener("slot" + juce::String(i) + "_engine", this);
        
        // Listen for parameter changes (15 params per slot)
        for (int j = 1; j <= 15; ++j) {
            parameters.addParameterListener("slot" + juce::String(i) + "_param" + juce::String(j), this);
        }
    }
    
    // IMPORTANT: Do NOT initialize engines in the constructor
    // Engines should only be loaded from saved state (via setStateInformation)
    // or when explicitly set by user/Trinity
    // This ensures the plugin starts with empty slots by default
    DBG("Constructor - All slots start empty (no engines loaded)");
    
    // Start AI server
    // TEMPORARILY DISABLED FOR DEBUGGING
    // startAIServer();

    // Explicitly set all engine selectors to 0 (None) to ensure clean start
    for (int slot = 1; slot <= 6; ++slot) {
        auto* param = parameters.getParameter("slot" + juce::String(slot) + "_engine");
        if (param != nullptr) {
            param->setValueNotifyingHost(0.0f);
        }
    }
    DBG("Explicitly initialized all engine selectors to None");
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
    DBG("ChimeraAudioProcessor::prepareToPlay called with fs=" + juce::String(sampleRate));
    m_sampleRate = sampleRate;
    m_samplesPerBlock = samplesPerBlock;
    
    // Debug: Log to file to see what's happening
    FILE* f = fopen("/tmp/opto_debug.txt", "a");
    if (f) {
        fprintf(f, "=== ChimeraAudioProcessor::prepareToPlay called ===\n");
        fprintf(f, "sampleRate=%.1f samplesPerBlock=%d\n", sampleRate, samplesPerBlock);
        fclose(f);
    }
    
    int maxLatency = 0;
    int engineCount = 0;
    for (int i = 0; i < 6; ++i) {
        if (m_activeEngines[i]) {
            engineCount++;
            DBG("Calling prepareToPlay on engine in slot " + juce::String(i) + 
                ": " + m_activeEngines[i]->getName());
            
            FILE* f2 = fopen("/tmp/opto_debug.txt", "a");
            if (f2) {
                fprintf(f2, "Slot %d has engine: %s\n", i, 
                    m_activeEngines[i]->getName().toRawUTF8());
                fclose(f2);
            }
            
            m_activeEngines[i]->prepareToPlay(sampleRate, samplesPerBlock);
            maxLatency = std::max(maxLatency, m_activeEngines[i]->getLatencySamples());
        } else {
            FILE* f3 = fopen("/tmp/opto_debug.txt", "a");
            if (f3) {
                fprintf(f3, "Slot %d is empty\n", i);
                fclose(f3);
            }
        }
    }
    
    DBG("Total engines prepared: " + juce::String(engineCount));
    
    // Report latency to host
    setLatencySamples(maxLatency);
    
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
    
    // Capture input level for metering
    float inputLevel = buffer.getMagnitude(0, numSamples);
    m_currentInputLevel.store(inputLevel);
    
    // Check if any slot is soloed
    bool anySoloed = false;
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        bool isSoloed = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_solo")->load() > 0.5f;
        if (isSoloed) {
            anySoloed = true;
            break;
        }
    }
    
    // Keep a copy of the dry signal for mixing
    juce::AudioBuffer<float> dryBuffer(buffer.getNumChannels(), numSamples);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }
    
    // Process through each slot in series
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        bool isBypassed = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_bypass")->load() > 0.5f;
        bool isSoloed = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_solo")->load() > 0.5f;
        float mixLevel = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_mix")->load();
        
        // Skip if bypassed or if soloing is active and this isn't soloed
        if (isBypassed || (anySoloed && !isSoloed)) {
            m_slotActivityLevels[slot].store(0.0f);
            continue;
        }
        
        {
            // Get parameters for this slot
            std::map<int, float> params;
            juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
            for (int i = 0; i < 15; ++i) {
                auto paramID = slotPrefix + juce::String(i + 1);
                float value = parameters.getRawParameterValue(paramID)->load();
                params[i] = value;
            }
            
            // Thread-safe engine access for both parameter update and processing
            {
                std::lock_guard<std::mutex> lock(m_engineMutex);
                if (m_activeEngines[slot]) {
                    // Check if this is the None engine (ID == 0)
                    auto* engineParam = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_engine");
                    int engineChoice = static_cast<int>(engineParam->load());
                    
                    // Skip None engines (engine ID 0)
                    // Since we have 1:1 mapping, choice 0 = None
                    if (engineChoice == 0) {
                        continue;
                    }
                    
                    // Also skip if engine is nullptr
                    if (!m_activeEngines[slot]) {
                        DBG("Warning: Slot " + juce::String(slot) + " has null engine!");
                        continue;
                    }
                    
                    // Debug: Log parameters being sent (only for slot 1 and engine 1)
                    if (slot == 0 && engineChoice == 1) {
                        static int debugCounter = 0;
                        if (++debugCounter % 100 == 0) {
                            DBG("Slot 1 params: [0]=" + juce::String(params[0]) + 
                                " [1]=" + juce::String(params[1]) + 
                                " [4]=" + juce::String(params[4]));
                            
                            // DEBUG: Log that we're about to call process
                            FILE* f = fopen("/tmp/process_chain.txt", "a");
                            if (f) {
                                fprintf(f, "About to call process on engine %p in slot %d\n",
                                    (void*)m_activeEngines[slot].get(), slot);
                                fclose(f);
                            }
                        }
                    }
                    
                    // Keep a copy of the slot input (dry for this effect)
                    juce::AudioBuffer<float> slotDryBuffer(buffer.getNumChannels(), numSamples);
                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                        slotDryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
                    }
                    
                    // Capture pre-process level for activity monitoring
                    float preLevel = buffer.getMagnitude(0, numSamples);
                    
                    // Update parameters
                    m_activeEngines[slot]->updateParameters(params);
                    
                    // Debug: Check buffer before/after
                    float preRMS = buffer.getRMSLevel(0, 0, numSamples);
                    
                    // Process the buffer IN PLACE
                    m_activeEngines[slot]->process(buffer);
                    
                    float postRMS = buffer.getRMSLevel(0, 0, numSamples);
                    
                    // Log significant processing for debugging
                    static int processCount = 0;
                    if (++processCount % 100 == 0 && slot == 0) {
                        DBG("Slot 0 Engine " + juce::String(engineChoice) + 
                            " RMS: " + juce::String(preRMS) + " -> " + juce::String(postRMS) +
                            " Mix: " + juce::String(mixLevel));
                    }
                    
                    // Apply mix control: blend slot dry (input) with wet (processed)
                    if (mixLevel < 0.999f) { // Only mix if not fully wet
                        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                            auto* bufferData = buffer.getWritePointer(ch);
                            auto* dryData = slotDryBuffer.getReadPointer(ch);
                            
                            for (int s = 0; s < numSamples; ++s) {
                                // Mix = 0: fully dry (slot input), Mix = 1: fully wet (processed)
                                bufferData[s] = dryData[s] * (1.0f - mixLevel) + bufferData[s] * mixLevel;
                            }
                        }
                    }
                    
                    // Calculate activity based on difference
                    float postLevel = buffer.getMagnitude(0, numSamples);
                    float activity = std::abs(postLevel - preLevel);
                    m_slotActivityLevels[slot].store(activity);
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

    // Feed Input 2 to voice recorder for Pi build
    #ifdef JUCE_LINUX
    if (auto* piEditor = dynamic_cast<ChimeraAudioProcessorEditor_Pi*>(getActiveEditor())) {
        if (buffer.getNumChannels() >= 2) {
            const float* channel2 = buffer.getReadPointer(1); // Input 2
            piEditor->feedVoiceRecorder(channel2, buffer.getNumSamples());
        }
    }
    #endif
}

juce::AudioProcessorEditor* ChimeraAudioProcessor::createEditor() {
    // Raspberry Pi gets simplified UI
    #ifdef CHIMERA_PI
        return new ChimeraAudioProcessorEditor_Pi(*this);
    // Use the new dynamic parameter system that queries live engines
    #elif defined(USE_DYNAMIC_NEXUS)
        // Back to using the real editor - we've identified the issue!
        return new PluginEditorNexusStatic(*this);  // STATIC architecture
    #elif defined(USE_ORIGINAL_UI)
        return new ChimeraAudioProcessorEditor(*this);
    #elif defined(USE_SKUNKWORKS_UI)
        return new ChimeraAudioProcessorEditorSkunkworks(*this);
    #elif defined(USE_REFINED_UI)
        return new ChimeraAudioProcessorEditorRefined(*this);
    #else
        // Check environment variables for UI selection
        // Temporarily disable Dynamic UI until added to Xcode project
        // if (std::getenv("CHIMERA_DYNAMIC_UI") != nullptr) {
        //     return new PluginEditorNexusDynamic(*this);
        // }
        // else if (std::getenv("CHIMERA_ORIGINAL_UI") != nullptr) {
        if (std::getenv("CHIMERA_ORIGINAL_UI") != nullptr) {
            return new ChimeraAudioProcessorEditor(*this);
        }
        if (std::getenv("CHIMERA_SKUNKWORKS_UI") != nullptr) {
            return new ChimeraAudioProcessorEditorSkunkworks(*this);
        }
        if (std::getenv("CHIMERA_REFINED_UI") != nullptr) {
            return new ChimeraAudioProcessorEditorRefined(*this);
        }
        // Test with selectors but NO attachments
        return new PluginEditorFull(*this);
    #endif
}

void ChimeraAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ChimeraAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // Always start fresh mode - ignore saved state to ensure clean startup
    if (m_alwaysStartFresh) {
        DBG("Always start fresh mode enabled - ignoring saved state");
        DBG("Plugin will start with all slots set to None");
        return;
    }

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(parameters.state.getType())) {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
            
            // CRITICAL FIX: After loading state, recreate engines based on saved parameters
            // This ensures engines are initialized when the plugin loads from saved state
            DBG("setStateInformation: Recreating engines from saved state");
            for (int slot = 0; slot < NUM_SLOTS; ++slot) {
                auto* engineParam = parameters.getRawParameterValue("slot" + juce::String(slot + 1) + "_engine");
                if (engineParam) {
                    int choiceIndex = static_cast<int>(engineParam->load());
                    int engineID = choiceIndexToEngineID(choiceIndex);
                    
                    DBG("Slot " + juce::String(slot) + " loading engine ID " + juce::String(engineID));
                    
                    // Create the engine
                    if (engineID >= 0 && engineID < ENGINE_COUNT) {
                        std::unique_ptr<EngineBase> engine = EngineFactory::createEngine(engineID);
                        if (engine) {
                            // CRITICAL FIX: Must prepare the engine immediately
                            // prepareToPlay won't be called again after state load
                            engine->prepareToPlay(m_sampleRate, m_samplesPerBlock);
                            
                            // Store the prepared engine
                            m_activeEngines[slot] = std::move(engine);
                            
                            DBG("Engine prepared after state load: " + 
                                m_activeEngines[slot]->getName() + " in slot " + juce::String(slot));
                        }
                    }
                }
            }
        }
    }
}

void ChimeraAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue) {
    DBG("parameterChanged called: " + parameterID + " = " + juce::String(newValue));
    
    // Check if it's an engine selector parameter
    for (int slot = 1; slot <= NUM_SLOTS; ++slot) {
        juce::String slotStr = juce::String(slot);
        
        if (parameterID == "slot" + slotStr + "_engine") {
            // CRITICAL FIX: Properly handle AudioParameterChoice normalization
            // newValue is normalized 0-1, we need the actual choice index
            auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(
                parameters.getParameter(parameterID));
            
            if (choiceParam) {
                int choiceIndex = choiceParam->getIndex();  // Gets actual choice index
                int engineID = choiceIndexToEngineID(choiceIndex);
                
                DBG(">>> ENGINE PARAMETER CHANGED: " + parameterID + 
                    " normalized=" + juce::String(newValue) +
                    " choice index=" + juce::String(choiceIndex) + 
                    " -> engine ID=" + juce::String(engineID));
                
                DBG(">>> Calling loadEngine for slot " + juce::String(slot - 1) + " with engineID " + juce::String(engineID));
                loadEngine(slot - 1, engineID);
                DBG(">>> loadEngine call completed");
            } else {
                DBG(">>> ERROR: Could not cast parameter to AudioParameterChoice");
            }
            break;
        }
        
        // Check if it's a parameter knob change
        for (int param = 1; param <= 15; ++param) {
            if (parameterID == "slot" + slotStr + "_param" + juce::String(param)) {
                // Special handling for parameter 2 (index 1) - only snap for specific engines
                if (param == 2) {
                    // Check which engine is loaded
                    auto* engineParam = parameters.getRawParameterValue("slot" + slotStr + "_engine");
                    int engineChoice = static_cast<int>(engineParam->load());
                    int engineID = choiceIndexToEngineID(engineChoice);
                    
                    // Only allow snapping for IntelligentHarmonizer (engine ID 12)
                    // All other engines should have smooth parameter control
                    if (engineID != ENGINE_INTELLIGENT_HARMONIZER) {
                        // For non-harmonizer engines, ensure smooth parameter values
                        // Don't modify the value - just pass it through
                    }
                }
                
                // Update the engine parameters for this slot
                updateEngineParameters(slot - 1);
                break;
            }
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
    DBG("loadEngine: Creating engine ID " + juce::String(engineID) + " for slot " + juce::String(slot));
    std::unique_ptr<EngineBase> newEngine = EngineFactory::createEngine(engineID);
    if (newEngine) {
        DBG("  Engine created successfully: " + newEngine->getName() + 
            " with " + juce::String(newEngine->getNumParameters()) + " parameters");
        newEngine->prepareToPlay(m_sampleRate, m_samplesPerBlock);
        
        // Apply default parameters for this engine
        applyDefaultParameters(slot, engineID);
        
        // Lock only for the actual swap
        {
            std::lock_guard<std::mutex> lock(m_engineMutex);
            m_activeEngines[slot] = std::move(newEngine);
            DBG("  Engine stored in slot " + juce::String(slot) + " at address: " + 
                juce::String::toHexString((juce::int64)m_activeEngines[slot].get()));
        }
        
        // Update latency reporting
        int maxLatency = 0;
        for (const auto& engine : m_activeEngines) {
            if (engine) {
                maxLatency = std::max(maxLatency, engine->getLatencySamples());
            }
        }
        setLatencySamples(maxLatency);
        
        updateEngineParameters(slot);
        
        // CRITICAL: Force parameter update to ensure UI sync
        // This ensures the UI reflects the engine change even if parameter listeners fail
        updateEngineParameters(slot);
        
        DBG("Successfully loaded engine into slot " + juce::String(slot) + " with parameters updated");
    } else {
        std::lock_guard<std::mutex> lock(m_engineMutex);
        m_activeEngines[slot].reset();
        DBG("ERROR: Failed to create engine for ID " + juce::String(engineID));
    }
}

void ChimeraAudioProcessor::applyDefaultParameters(int slot, int engineID) {
    // Use the new unified default parameter system for all 57 engines
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
    
    // Get optimized defaults from the unified system
    auto defaultParams = UnifiedDefaultParameters::getDefaultParameters(engineID);
    
    // Initialize all parameters to safe center values first
    for (int i = 1; i <= 15; ++i) {
        auto paramID = slotPrefix + juce::String(i);
        if (auto* param = parameters.getParameter(paramID)) {
            param->setValueNotifyingHost(0.5f); // Safe center/neutral position
        }
    }
    
    // Apply the specific defaults from unified system
    for (const auto& paramPair : defaultParams) {
        int paramIndex = paramPair.first;  // 0-based index from engine
        float defaultValue = paramPair.second;  // Optimized default value
        
        // Convert 0-based engine index to 1-based UI parameter ID
        auto paramID = slotPrefix + juce::String(paramIndex + 1);
        if (auto* param = parameters.getParameter(paramID)) {
            param->setValueNotifyingHost(defaultValue);
        }
    }
    
    // Validate the defaults were applied
    if (!UnifiedDefaultParameters::validateEngineDefaults(engineID)) {
        DBG("WARNING: Engine " + juce::String(engineID) + " defaults failed validation");
    }
    
    DBG("Applied " + juce::String(defaultParams.size()) + " unified default parameters for engine " + 
        juce::String(engineID) + " in slot " + juce::String(slot));
}

void ChimeraAudioProcessor::updateEngineParameters(int slot) {
    std::map<int, float> params;
    juce::String slotPrefix = "slot" + juce::String(slot + 1) + "_param";
    
    for (int i = 0; i < 15; ++i) {
        auto paramID = slotPrefix + juce::String(i + 1);
        float value = parameters.getRawParameterValue(paramID)->load();
        params[i] = value;
    }
    
    // Validate parameters to ensure consistency
    // Note: ParameterValidator ensures all values are in 0-1 range
    // and provides defaults for any missing parameters
    
    // Thread-safe parameter update
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (m_activeEngines[slot]) {
        m_activeEngines[slot]->updateParameters(params);
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

float ChimeraAudioProcessor::getSlotActivity(int slot) const {
    if (slot >= 0 && slot < NUM_SLOTS) {
        return m_slotActivityLevels[slot].load();
    }
    return 0.0f;
}

void ChimeraAudioProcessor::clearAllSlots() {
    DBG("Clearing all slots - setting to ENGINE_NONE");
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        setSlotEngine(slot, 0);  // Set to ENGINE_NONE
    }
}

void ChimeraAudioProcessor::setSlotEngine(int slot, int engineID) {
    if (slot < 0 || slot >= NUM_SLOTS) {
        DBG("setSlotEngine: INVALID slot=" + juce::String(slot) + " (must be 0-" + juce::String(NUM_SLOTS-1) + ")");
        return;
    }
    
    // Convert engine ID to choice index for the parameter
    int choiceIndex = engineIDToChoiceIndex(engineID);
    auto paramID = "slot" + juce::String(slot + 1) + "_engine";
    
    DBG("setSlotEngine: slot=" + juce::String(slot) + " engineID=" + juce::String(engineID) + 
        " -> choiceIndex=" + juce::String(choiceIndex) + " paramID=" + paramID);
    
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(parameters.getParameter(paramID))) {
        int currentIndex = choiceParam->getIndex();
        DBG("  AudioParameterChoice found: current index=" + juce::String(currentIndex) + " setting to index=" + juce::String(choiceIndex));
        
        // For AudioParameterChoice, we need to normalize the choice index
        // The normalized value should be: choiceIndex / (numChoices - 1)
        float normalizedValue = choiceIndex / static_cast<float>(choiceParam->choices.size() - 1);
        
        DBG("  Setting normalized value=" + juce::String(normalizedValue) + " for " + juce::String(choiceParam->choices.size()) + " choices");
        choiceParam->setValueNotifyingHost(normalizedValue);
        
        // Verify the parameter was actually set
        int verifyIndex = choiceParam->getIndex();
        DBG("  Parameter after setting: index=" + juce::String(verifyIndex));
        
        if (verifyIndex != choiceIndex) {
            DBG("  WARNING: Engine index mismatch! Expected " + juce::String(choiceIndex) + " but got " + juce::String(verifyIndex));
        }
    } else {
        DBG("  ERROR: Could not find or cast parameter: " + paramID);
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
    // Delegate to the unified default parameter system for consistency
    return UnifiedDefaultParameters::getMixParameterIndex(engineID);
}

// Comprehensive Engine Diagnostic - REMOVED FROM PRODUCTION
// This function has been moved to standalone test harnesses
void ChimeraAudioProcessor::runComprehensiveDiagnostic()
{
    DBG("Comprehensive diagnostic disabled in production build");
    return;
    
    /* DIAGNOSTIC CODE REMOVED - USE STANDALONE TEST HARNESS INSTEAD
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
            
            // Set mix to 100% wet to ensure we hear the effect (if engine has mix parameter)
            if (mixIndex >= 0) {
                params[mixIndex] = 1.0f;
            }
            
            // Set engine-specific test parameters based on ACTUAL categories
            int category = getEngineCategory(engineID);
            switch (category) {
                case EngineCategory::VINTAGE_EFFECTS: // Reverbs, delays, vintage effects
                    params[0] = 0.5f; // Time/Size
                    params[1] = 0.3f; // Feedback/Damping
                    params[2] = 0.4f; // Decay/Density
                    break;
                case EngineCategory::MODULATION: // Chorus, phaser, tremolo, etc.
                    params[0] = 0.5f; // Rate
                    params[1] = 0.7f; // Depth
                    params[2] = 0.5f; // Shape/Type
                    break;
                case EngineCategory::FILTERS_EQ: // Filters and EQs
                    params[0] = 0.5f; // Frequency
                    params[1] = 0.7f; // Resonance/Q
                    params[2] = 0.5f; // Type/Mode
                    break;
                case EngineCategory::DISTORTION_SATURATION: // Distortion, saturation, fuzz
                    params[0] = 0.7f; // Drive/Gain
                    params[1] = 0.5f; // Tone/Filter
                    params[2] = 0.6f; // Output Level
                    break;
                case EngineCategory::SPATIAL_TIME: // Spatial effects, freeze, etc.
                    params[0] = 0.5f; // Size/Width
                    params[1] = 0.5f; // Feedback/Depth
                    params[2] = 0.5f; // Mix/Blend
                    break;
                case EngineCategory::DYNAMICS: // Compressors, limiters, gates
                    params[0] = 0.3f; // Threshold
                    params[1] = 0.7f; // Ratio/Range
                    params[2] = 0.2f; // Attack
                    params[3] = 0.4f; // Release
                    break;
                case EngineCategory::UTILITY: // Gain, M/S, phase tools
                    params[0] = 0.5f; // Primary control
                    params[1] = 0.5f; // Secondary control
                    break;
                default:
                    // Leave at 0.5f defaults
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
    */ // END OF REMOVED DIAGNOSTIC CODE
}

void ChimeraAudioProcessor::runIsolatedEngineTests() {
    DBG("Isolated engine tests disabled in production build");
    return;
    
    /* ISOLATED TEST CODE REMOVED - USE STANDALONE TEST HARNESS INSTEAD
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
    */ // END OF REMOVED ISOLATED TEST CODE
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new ChimeraAudioProcessor();
}