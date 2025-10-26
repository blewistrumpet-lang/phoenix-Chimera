#include "PluginEditor_Pi.h"

ChimeraAudioProcessorEditor_Pi::ChimeraAudioProcessorEditor_Pi(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(800, 480);

    // Title - Upper left corner (11px regular, letter-spaced, subtle)
    titleLabel.setText("CHIMERAPHOENIX PI", juce::dontSendNotification);
    auto titleFont = juce::Font(juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::plain);
    titleFont.setExtraKerningFactor(0.15f);  // +15% letter spacing for premium feel
    titleLabel.setFont(titleFont);
    titleLabel.setColour(juce::Label::textColourId, textTertiary);  // 30% white, very subtle
    titleLabel.setJustificationType(juce::Justification::left);  // Left-aligned
    addAndMakeVisible(titleLabel);

    // Preset name display - HERO ELEMENT (44px bold, bright white) - MORE PROMINENT
    presetNameLabel.setText("No Preset", juce::dontSendNotification);
    presetNameLabel.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 44.0f, juce::Font::bold));
    presetNameLabel.setColour(juce::Label::textColourId, textPrimary);  // 100% white
    presetNameLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetNameLabel);

    // Status label - Refined typography (13px regular, subtle)
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::plain));
    statusLabel.setColour(juce::Label::textColourId, textSecondary);  // 50% white
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    // Trinity health indicator - just a status dot
    trinityHealthLabel.setText("●", juce::dontSendNotification);
    trinityHealthLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    trinityHealthLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    trinityHealthLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(trinityHealthLabel);

    // Progress bar (ASCII art style)
    progressLabel.setText("", juce::dontSendNotification);
    progressLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    progressLabel.setColour(juce::Label::textColourId, accentColor);
    progressLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(progressLabel);

    // Voice button - gradient button with hold-to-speak
    voiceButton.onPress = [this] {
        if (!isRecording) {
            DBG("Voice button pressed - starting recording");
            startVoiceRecording();
        }
    };
    voiceButton.onRelease = [this] {
        if (isRecording) {
            DBG("Voice button released - stopping recording");
            stopVoiceRecording();
        }
    };
    addAndMakeVisible(voiceButton);

    // Input/Output meters with labels
    inputMeterLabel.setText("IN", juce::dontSendNotification);
    inputMeterLabel.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold));
    inputMeterLabel.setColour(juce::Label::textColourId, textPrimary);
    inputMeterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inputMeterLabel);
    addAndMakeVisible(inputMeter);

    outputMeterLabel.setText("OUT", juce::dontSendNotification);
    outputMeterLabel.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold));
    outputMeterLabel.setColour(juce::Label::textColourId, textPrimary);
    outputMeterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputMeterLabel);
    addAndMakeVisible(outputMeter);

    // Engine slot grid - 6 colored boxes
    addAndMakeVisible(engineSlotGrid);

    // Initial Trinity health check
    checkTrinityHealth();

#if ENABLE_GPIO_HARDWARE && defined(__linux__)
    // Get GPIO objects from Processor (which initializes them for headless operation)
    hardwareController = audioProcessor.getHardwareController();
    eventBus = audioProcessor.getEventBus();
    controlState = audioProcessor.getControlState();

    if (hardwareController && eventBus && controlState) {
        DBG("Editor using GPIO hardware from Processor");

        // Subscribe to events for UI updates
        eventBus->subscribe(EventBus::EventType::ENCODER_TURN,
            [this](const EventBus::Event& e) { handleEncoderEvent(e); });
        eventBus->subscribe(EventBus::EventType::SWITCH_CHANGE,
            [this](const EventBus::Event& e) { handleSwitchEvent(e); });

        // Create display components
        for (int i = 0; i < 3; ++i) {
            encoderDisplays[i] = std::make_unique<EncoderDisplay>(i);
            addAndMakeVisible(encoderDisplays[i].get());

            switchDisplays[i] = std::make_unique<SwitchDisplay>(i);
            addAndMakeVisible(switchDisplays[i].get());
        }

        // Hardware callbacks are already wired in Processor
        // Just update display positions
        resized();

    } else {
        DBG("GPIO hardware not available from Processor");
    }
#endif

    // Update UI at 30Hz
    startTimer(33);
}

ChimeraAudioProcessorEditor_Pi::~ChimeraAudioProcessorEditor_Pi()
{
    stopTimer();
    system("pkill -9 arecord");

#if ENABLE_GPIO_HARDWARE && defined(__linux__)
    if (hardwareController) {
        hardwareController->stopHardwareMonitoring();
    }
#endif
}

// Helper function to map engine name to category ID for color coding
int ChimeraAudioProcessorEditor_Pi::getEngineCategoryFromName(const juce::String& engineName)
{
    // Map engine names to approximate IDs based on category
    // Categories: Dynamics (1-6), EQ (7-14), Distortion (15-22), Modulation (23-30),
    //             Pitch (31-33), Delay (34-38), Reverb (39-43), Spatial (44-46),
    //             Spectral (47-52), Utility (53-56)

    juce::String name = engineName.toLowerCase();

    // Dynamics (Blue) - ID range 1-6
    if (name.contains("compressor") || name.contains("limiter") ||
        name.contains("gate") || name.contains("expander") ||
        name.contains("transient")) return 3;

    // EQ/Filters (Green) - ID range 7-14
    if (name.contains("eq") || name.contains("filter") ||
        name.contains("shelf") || name.contains("bell")) return 10;

    // Distortion/Saturation (Red/Orange) - ID range 15-22
    if (name.contains("distortion") || name.contains("overdrive") ||
        name.contains("fuzz") || name.contains("saturation") ||
        name.contains("tube") || name.contains("preamp") ||
        name.contains("bitcrusher") || name.contains("crusher")) return 18;

    // Modulation (Purple) - ID range 23-30
    if (name.contains("chorus") || name.contains("flanger") ||
        name.contains("phaser") || name.contains("tremolo") ||
        name.contains("vibrato") || name.contains("rotary")) return 26;

    // Pitch/Harmony (Yellow) - ID range 31-33
    if (name.contains("pitch") || name.contains("harmon") ||
        name.contains("octave") || name.contains("detune")) return 32;

    // Delay/Echo (Amber) - ID range 34-38
    if (name.contains("delay") || name.contains("echo") ||
        name.contains("bucket")) return 36;

    // Reverb (Cyan) - ID range 39-43
    if (name.contains("reverb") || name.contains("room") ||
        name.contains("hall") || name.contains("plate") ||
        name.contains("spring")) return 41;

    // Spatial (Magenta) - ID range 44-46
    if (name.contains("stereo") || name.contains("width") ||
        name.contains("imager") || name.contains("spatial") ||
        name.contains("haas")) return 45;

    // Spectral/Special (Teal) - ID range 47-52
    if (name.contains("spectral") || name.contains("vocoder") ||
        name.contains("freeze") || name.contains("formant") ||
        name.contains("morph")) return 49;

    // Utility (Gray) - ID range 53-56
    if (name.contains("gain") || name.contains("utility") ||
        name.contains("trim") || name.contains("tool")) return 54;

    // Default - treat as utility
    return 54;
}

void ChimeraAudioProcessorEditor_Pi::paint(juce::Graphics& g)
{
    // Premium background gradient (subtle)
    juce::ColourGradient bgGradient(
        bgPrimary, 0, 0,
        bgSecondary, 0, (float)getHeight(),
        false
    );
    g.setGradientFill(bgGradient);
    g.fillAll();

    // No card backgrounds - cleaner, more modern
    // Elements have their own styling
}

void ChimeraAudioProcessorEditor_Pi::resized()
{
    auto bounds = getLocalBounds().reduced(16);  // Outer margins

    // Reserve space for input/output meters on sides (35px wide)
    auto leftMeterArea = bounds.removeFromLeft(35);
    bounds.removeFromLeft(16);  // Spacing
    auto rightMeterArea = bounds.removeFromRight(35);
    bounds.removeFromRight(16);  // Spacing

    // NO VERTICAL CENTERING - Start from very top of window

    // HEADER (24px) - Title at very top left, Trinity status in upper right
    auto headerArea = bounds.removeFromTop(24);
    trinityHealthLabel.setBounds(headerArea.getRight() - 80, headerArea.getY() + 4, 20, 20);
    titleLabel.setBounds(headerArea);  // Full header area, left-aligned
    bounds.removeFromTop(16);  // Gap

    // PRESET NAME (56px) - HERO ELEMENT with larger 44px font
    presetNameLabel.setBounds(bounds.removeFromTop(56));
    bounds.removeFromTop(12);  // Gap

    // STATUS (20px) - Subtle
    statusLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(6);  // Gap

    // PROGRESS (16px) - Always reserve space for stable layout
    progressLabel.setBounds(bounds.removeFromTop(16));
    bounds.removeFromTop(20);  // Gap before button

    // VOICE BUTTON (72px, 90% width, centered) - HERO interaction element, taller
    auto buttonHeight = 72;
    auto buttonArea = bounds.removeFromTop(buttonHeight);
    auto buttonWidth = static_cast<int>(buttonArea.getWidth() * 0.90f);  // 90% width
    auto buttonX = buttonArea.getX() + (buttonArea.getWidth() - buttonWidth) / 2;
    voiceButton.setBounds(buttonX, buttonArea.getY(), buttonWidth, buttonHeight);

    // Calculate remaining space and position slots in lower half (split the difference)
    int remainingHeight = bounds.getHeight();
    int slotsHeight = 90;
    int gapBeforeSlots = (remainingHeight - slotsHeight) / 2;  // Split remaining space evenly

    bounds.removeFromTop(gapBeforeSlots);  // Position slots in lower portion

    // ENGINE SLOTS - FIXED 90px HEIGHT (balanced, readable, not dominating)
    engineSlotGrid.setBounds(bounds.removeFromTop(90));

    // Layout meters vertically on the sides
    inputMeterLabel.setBounds(leftMeterArea.removeFromTop(20));
    leftMeterArea.removeFromTop(4);
    inputMeter.setBounds(leftMeterArea);

    outputMeterLabel.setBounds(rightMeterArea.removeFromTop(20));
    rightMeterArea.removeFromTop(4);
    outputMeter.setBounds(rightMeterArea);

#if ENABLE_GPIO_HARDWARE && defined(__linux__)
    // Position hardware displays using FULL window bounds (not reduced bounds)
    // These overlay on top of the main UI
    auto fullBounds = getLocalBounds();  // Get FULL window size: 800x480

    if (encoderDisplays[0] && encoderDisplays[1] && encoderDisplays[2]) {
        // Encoders at top - evenly spaced across full width
        int encY = 10;
        int encWidth = 100;
        int encHeight = 80;
        int windowWidth = fullBounds.getWidth();  // Should be 800

        // Calculate even spacing: divide into quarters
        int spacing = windowWidth / 4;

        encoderDisplays[0]->setBounds(spacing - encWidth/2, encY, encWidth, encHeight);      // Left: 200-50 = 150
        encoderDisplays[1]->setBounds(spacing*2 - encWidth/2, encY, encWidth, encHeight);    // Center: 400-50 = 350
        encoderDisplays[2]->setBounds(spacing*3 - encWidth/2, encY, encWidth, encHeight);    // Right: 600-50 = 550

        DBG("Encoder positions set - Window width: " << windowWidth);
        DBG("Enc1: " << encoderDisplays[0]->getBounds().toString());
        DBG("Enc2: " << encoderDisplays[1]->getBounds().toString());
        DBG("Enc3: " << encoderDisplays[2]->getBounds().toString());
    }

    if (switchDisplays[0] && switchDisplays[1] && switchDisplays[2]) {
        // Switches at bottom - evenly spaced across full width
        int swY = fullBounds.getHeight() - 70;  // 480 - 70 = 410
        int swWidth = 80;
        int swHeight = 60;
        int windowWidth = fullBounds.getWidth();

        // Calculate even spacing: divide into quarters
        int spacing = windowWidth / 4;

        switchDisplays[0]->setBounds(spacing - swWidth/2, swY, swWidth, swHeight);      // Left: 200-40 = 160
        switchDisplays[1]->setBounds(spacing*2 - swWidth/2, swY, swWidth, swHeight);    // Center: 400-40 = 360
        switchDisplays[2]->setBounds(spacing*3 - swWidth/2, swY, swWidth, swHeight);    // Right: 600-40 = 560

        DBG("Switch positions set - Window height: " << fullBounds.getHeight());
        DBG("SW1: " << switchDisplays[0]->getBounds().toString());
        DBG("SW2: " << switchDisplays[1]->getBounds().toString());
        DBG("SW3: " << switchDisplays[2]->getBounds().toString());

        // Ensure hardware displays are on top
        for (int i = 0; i < 3; ++i) {
            encoderDisplays[i]->toFront(true);
            switchDisplays[i]->toFront(true);
        }
    }
#endif
}

void ChimeraAudioProcessorEditor_Pi::timerCallback()
{
    // Update input/output meters
    float inputLevel = audioProcessor.getCurrentInputLevel();
    float outputLevel = audioProcessor.getCurrentOutputLevel();

    // DEBUG: Write levels to file occasionally
    static int debugCounter = 0;
    if (++debugCounter % 30 == 0) {  // Every ~1 second (30 * 33ms)
        juce::File debugFile("/tmp/meter_debug.txt");
        juce::String debugText = "Input: " + juce::String(inputLevel, 6) + ", Output: " + juce::String(outputLevel, 6) + "\n";
        debugFile.appendText(debugText);
    }

    inputMeter.setLevel(inputLevel);
    outputMeter.setLevel(outputLevel);

    // Update engine slot grid - colored boxes showing active engines
    for (int i = 0; i < 6; ++i) {
        auto& engine = audioProcessor.getEngine(i);
        if (engine) {
            juce::String engineName = engine->getName();
            // Treat "None" engine as empty slot
            if (engineName == "None") {
                engineSlotGrid.updateSlot(i, 0, "");  // 0 = empty slot
            } else {
                // Map engine name to approximate category ID for color coding
                int categoryID = getEngineCategoryFromName(engineName);
                engineSlotGrid.updateSlot(i, categoryID, engineName);
            }
        } else {
            engineSlotGrid.updateSlot(i, 0, "");  // 0 = empty slot
        }
    }

    // Simulate loading progress during Trinity generation (ONLY if no real progress available)
    // If progressMonitor is active, it will update the progress bar with real values
    if (isTrinityProcessing && progressMonitor == nullptr) {
        loadingProgress += 0.05f;
        if (loadingProgress >= 1.0f) {
            loadingProgress = 0.0f;  // Loop for indeterminate progress
        }
        updateLoadingBar(loadingProgress);
    }

    // Periodic Trinity health check (every 30 seconds)
    healthCheckCounter++;
    if (healthCheckCounter >= 150) {  // 900 * 33ms = ~30 seconds
        healthCheckCounter = 0;
        checkTrinityHealth();
    }

#if ENABLE_GPIO_HARDWARE && defined(__linux__)
    // Process hardware events
    if (eventBus) {
        eventBus->processEvents();
    }

    // Update hardware displays
    if (hardwareController && controlState) {
        // Get current bank indicator
        auto& state = controlState->getState();
        juce::String bankIndicator = (state.variant == ControlState::Variant::B) ? "B" : "A";

        for (int i = 0; i < 3; ++i) {
            if (encoderDisplays[i]) {
                auto& enc = hardwareController->getEncoder(i);
                encoderDisplays[i]->setPosition(enc.getPosition());
                encoderDisplays[i]->setButtonPressed(enc.isButtonPressed());

                // Update parameter info with meaningful display
                auto behavior = controlState->getEncoderBehavior(i);

                // Get normalized parameter value (0.0 to 1.0)
                float normalizedValue = 0.5f;  // Default
                if (auto* param = audioProcessor.getValueTreeState().getParameter(behavior.parameterID)) {
                    normalizedValue = param->getValue();
                }

                juce::String paramName;

                // Get preset name if browsing presets
                if (behavior.parameterID == "preset_index") {
                    // Get the actual preset index from the int parameter
                    if (auto* presetParam = dynamic_cast<juce::AudioParameterInt*>(
                        audioProcessor.getValueTreeState().getParameter("preset_index"))) {
                        int presetIndex = presetParam->get();
                        if (audioProcessor.getGPIOPresetManager()) {
                            paramName = audioProcessor.getGPIOPresetManager()->getPresetName(presetIndex);
                        } else {
                            paramName = "Preset " + juce::String(presetIndex + 1);
                        }
                        // Update normalized value for display
                        normalizedValue = static_cast<float>(presetIndex) / 9.0f;
                    } else {
                        paramName = "Preset";
                    }
                } else {
                    // Get the label based on encoder index
                    switch (i) {
                        case 0: paramName = state.encoder1Label; break;
                        case 1: paramName = state.encoder2Label; break;
                        case 2: paramName = state.encoder3Label; break;
                    }
                }

                // Update display with formatted info
                encoderDisplays[i]->setParameterInfo(paramName, normalizedValue, behavior.parameterID, bankIndicator);
            }

            if (switchDisplays[i]) {
                auto& sw = hardwareController->getSwitch(i);
                auto pos = static_cast<SwitchDisplay::Position>(sw.positionValue.load());
                switchDisplays[i]->setPosition(pos);
            }
        }
    }

    // Update status with current mode if not processing
    if (controlState && !isTrinityProcessing && !isRecording) {
        auto& state = controlState->getState();
        statusLabel.setText(state.getModeString() + " Mode | " +
                          state.getVariantString() + " Active",
                          juce::dontSendNotification);
    }
#endif
}

void ChimeraAudioProcessorEditor_Pi::startVoiceRecording()
{
    // Check Trinity health before allowing voice recording
    if (!trinityFeaturesEnabled) {
        statusLabel.setText("Error: Trinity server offline", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, errorColor);
        DBG("Voice recording blocked - Trinity server not available");
        DBG("Hint: Run the Trinity launch script to start the server");
        return;
    }

    isRecording = true;
    voiceButton.setButtonText("RECORDING...");
    voiceButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    statusLabel.setText("Listening...", juce::dontSendNotification);

    // Generate temp file path
    recordedVoiceFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("chimera_voice_" + juce::String(juce::Time::currentTimeMillis()) + ".wav");

#ifdef __linux__
    // On Linux/Pi, use jack_rec to capture from JACK
    juce::String command = "/home/branden/jack_voice_wrapper.sh start \"" + recordedVoiceFile.getFullPathName() + "\"";
    int result = system(command.toRawUTF8());
    if (result == 0) {
        DBG("Started JACK recording to: " << recordedVoiceFile.getFullPathName());
    } else {
        DBG("Failed to start JACK recording");
        isRecording = false;
        voiceButton.setButtonText("HOLD TO SPEAK");
        voiceButton.setColour(juce::TextButton::buttonColourId, accentColor);
        statusLabel.setText("Recording failed", juce::dontSendNotification);
        return;
    }
#else
    // Start internal JUCE voice recorder (from Input 2 audio stream)
    voiceRecorder.startRecording(audioProcessor.getSampleRate());
    DBG("Started internal voice recording to: " << recordedVoiceFile.getFullPathName());
#endif

    // Auto-stop after 5 seconds
    juce::Timer::callAfterDelay(5000, [this]() {
        if (isRecording) {
            stopVoiceRecording();
        }
    });
}

void ChimeraAudioProcessorEditor_Pi::stopVoiceRecording()
{
    if (!isRecording) return;

    isRecording = false;
    voiceButton.setButtonText("HOLD TO SPEAK");
    voiceButton.setColour(juce::TextButton::buttonColourId, accentColor);

#ifdef __linux__
    // Stop jack_rec recording
    juce::String command = "/home/branden/jack_voice_wrapper.sh stop \"" + recordedVoiceFile.getFullPathName() + "\"";
    system(command.toRawUTF8());

    // Check the recorded file
    juce::File audioFile(recordedVoiceFile.getFullPathName());
    DBG("Stopped JACK recording. File: " << recordedVoiceFile.getFullPathName());
    DBG("File size: " << audioFile.getSize() << " bytes");

    // Check if we have valid audio
    if (!audioFile.existsAsFile() || audioFile.getSize() < 1000) {
        statusLabel.setText("Error: No audio detected (silent)", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, errorColor);
        return;
    }
#else
    // Stop internal recorder and get the recorded file
    voiceRecorder.stopRecording();
    recordedVoiceFile = voiceRecorder.getRecordedFile();

    DBG("Stopped voice recording. File: " << recordedVoiceFile.getFullPathName());
    DBG("File size: " << recordedVoiceFile.getSize() << " bytes");
    DBG("Recording diagnostics: " << voiceRecorder.getDiagnostics());

    // Check if we have valid audio using the new validation
    if (!voiceRecorder.hasValidAudio()) {
        statusLabel.setText("Error: No audio detected (silent)", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, errorColor);
        DBG("DIAGNOSTIC: " << voiceRecorder.getDiagnostics());
        return;
    }
#endif

    // Check if we have valid audio file
    if (recordedVoiceFile.existsAsFile() && recordedVoiceFile.getSize() > 1000) {
        statusLabel.setText("Transcribing...", juce::dontSendNotification);
        sendToWhisper(recordedVoiceFile);
    } else {
        statusLabel.setText("Error: No audio recorded", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, errorColor);
        DBG("Voice recording failed - file too small or doesn't exist");
    }
}

void ChimeraAudioProcessorEditor_Pi::feedVoiceRecorder(const float* channel2Data, int numSamples)
{
    // Feed Input 2 audio to the voice recorder when recording
    if (isRecording && voiceRecorder.isCurrentlyRecording()) {
        voiceRecorder.recordSamples(channel2Data, numSamples);
    }
}

void ChimeraAudioProcessorEditor_Pi::sendToWhisper(const juce::File& audioFile)
{
    // Use curl to send to Trinity server's /transcribe endpoint
    juce::Thread::launch([this, audioFile]() {
        juce::File outputFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                       .getChildFile("whisper_response.json");

        juce::String curlCommand = "curl -s -m 20 -X POST http://localhost:8000/transcribe "
                                  "-F audio=@" + audioFile.getFullPathName().quoted() + " "
                                  "-o " + outputFile.getFullPathName().quoted();

        int exitCode = system(curlCommand.toRawUTF8());

        if (exitCode == 0 && outputFile.existsAsFile()) {
            juce::String response = outputFile.loadFileAsString();
            auto jsonResponse = juce::JSON::parse(response);

            if (jsonResponse.hasProperty("text")) {
                juce::String transcribedText = jsonResponse["text"].toString().trim();

                juce::MessageManager::callAsync([this, transcribedText]() {
                    DBG("✅ Transcription received: " << transcribedText);
                    printf("[PluginEditor] ✅ Transcription: %s\n", transcribedText.toRawUTF8());
                    fflush(stdout);

                    // Store transcription and display it prominently
                    currentPrompt = transcribedText;

                    // Show transcription in preset name area immediately (will be replaced by actual preset name)
                    presetNameLabel.setText("\"" + transcribedText + "\"", juce::dontSendNotification);
                    presetNameLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);

                    statusLabel.setText("Generating preset...", juce::dontSendNotification);
                    statusLabel.setColour(juce::Label::textColourId, textColor);
                    repaint();  // Force UI update

                    // Start the Trinity generation process
                    sendTrinityRequest(transcribedText);
                });
            } else {
                juce::MessageManager::callAsync([this, response]() {
                    statusLabel.setText("Whisper: " + response.substring(0, 30), juce::dontSendNotification);
                    statusLabel.setColour(juce::Label::textColourId, errorColor);
                });
            }
        } else {
            juce::MessageManager::callAsync([this]() {
                statusLabel.setText("Whisper: curl failed", juce::dontSendNotification);
                statusLabel.setColour(juce::Label::textColourId, errorColor);
            });
        }

        outputFile.deleteFile();
    });
}

void ChimeraAudioProcessorEditor_Pi::sendTrinityRequest(const juce::String& prompt)
{
    // Double-check Trinity health before sending request
    if (!trinityFeaturesEnabled) {
        statusLabel.setText("Error: Trinity server offline", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, errorColor);
        DBG("Trinity request blocked - server not available");
        return;
    }

    isTrinityProcessing = true;
    loadingProgress = 0.0f;
    statusLabel.setText("Trinity: Generating...", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, textColor);

    // Generate unique request ID for progress tracking
    currentRequestId = "gen_" + juce::String(juce::Time::currentTimeMillis());

    // Start progress monitor BEFORE sending request
    stopProgressMonitoring(); // Clean up any previous monitor
    progressMonitor = std::make_unique<FileProgressMonitor>(currentRequestId);
    progressMonitor->onProgressUpdate = [this](const juce::var& progress) {
        updateUIFromProgress(progress);
    };
    progressMonitor->startThread();

    // Prepare JSON request
    juce::DynamicObject::Ptr requestObj = new juce::DynamicObject();
    requestObj->setProperty("prompt", prompt);
    requestObj->setProperty("request_id", currentRequestId); // Added for progress tracking
    juce::String jsonRequest = juce::JSON::toString(juce::var(requestObj.get()));

    // Use curl for HTTP with timeout to prevent hanging
    juce::Thread::launch([this, jsonRequest]() {
        // Write JSON to temp file for curl
        juce::File jsonFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                     .getChildFile("trinity_request.json");
        jsonFile.replaceWithText(jsonRequest);

        juce::File outputFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                       .getChildFile("trinity_response.json");

        // Use -m 60 for 60 second timeout (preset generation can take 30-40 seconds)
        juce::String curlCommand = "curl -s -m 60 -X POST " + trinityServerUrl + "/generate "
                                  "-H 'Content-Type: application/json' "
                                  "-d @" + jsonFile.getFullPathName().quoted() + " "
                                  "-o " + outputFile.getFullPathName().quoted();

        int exitCode = system(curlCommand.toRawUTF8());
        jsonFile.deleteFile();

        if (exitCode == 0 && outputFile.existsAsFile()) {
            juce::String response = outputFile.loadFileAsString();
            auto jsonResponse = juce::JSON::parse(response);

            if (jsonResponse.hasProperty("preset")) {
                auto preset = jsonResponse["preset"];

                juce::MessageManager::callAsync([this, preset]() {
                    isTrinityProcessing = false;
                    stopProgressMonitoring();
                    applyTrinityPreset(preset);
                });
            } else {
                juce::MessageManager::callAsync([this, response]() {
                    isTrinityProcessing = false;
                    stopProgressMonitoring();
                    statusLabel.setText("Trinity: " + response.substring(0, 30), juce::dontSendNotification);
                    statusLabel.setColour(juce::Label::textColourId, errorColor);
                    progressLabel.setText("", juce::dontSendNotification);
                    // Recheck health after failure
                    checkTrinityHealth();
                });
            }
        } else {
            juce::MessageManager::callAsync([this]() {
                isTrinityProcessing = false;
                stopProgressMonitoring();
                statusLabel.setText("Trinity: Request timeout/failed", juce::dontSendNotification);
                statusLabel.setColour(juce::Label::textColourId, errorColor);
                progressLabel.setText("", juce::dontSendNotification);
                // Recheck health after failure
                checkTrinityHealth();
            });
        }

        outputFile.deleteFile();
    });
}

void ChimeraAudioProcessorEditor_Pi::updateLoadingBar(float progress)
{
    // Call the enhanced version with calculated percentage
    int percent = static_cast<int>(progress * 100.0f);
    updateLoadingBarWithPercent(progress, percent);
}

void ChimeraAudioProcessorEditor_Pi::updateLoadingBarWithPercent(float progress, int percent)
{
    // ASCII art progress bar with percentage: [========    ] 40%
    int totalChars = 20;
    int filledChars = static_cast<int>(progress * totalChars);

    juce::String bar = "[";
    for (int i = 0; i < totalChars; ++i) {
        bar += (i < filledChars) ? "=" : " ";
    }
    bar += "] " + juce::String(percent) + "%";

    progressLabel.setText(bar, juce::dontSendNotification);
    progressLabel.setColour(juce::Label::textColourId,
                           progress < 0.3f ? juce::Colours::red :
                           progress < 0.7f ? juce::Colours::orange :
                           juce::Colours::green);
}

void ChimeraAudioProcessorEditor_Pi::applyTrinityPreset(const juce::var& preset)
{
    // Extract preset name and display it directly (without "Preset:" prefix)
    if (preset.hasProperty("name")) {
        currentPresetName = preset["name"].toString();
        presetNameLabel.setText(currentPresetName, juce::dontSendNotification);
    }

    // Apply slots (same logic as Mac version)
    if (preset.hasProperty("slots")) {
        auto slots = preset["slots"];
        if (slots.isArray()) {
            for (int i = 0; i < juce::jmin(6, slots.size()); ++i) {
                auto slot = slots[i];

                // Set engine
                if (slot.hasProperty("engine_id")) {
                    int engineId = slot["engine_id"];
                    audioProcessor.setSlotEngine(i, engineId);
                }

                // Apply parameters
                if (slot.hasProperty("parameters")) {
                    auto params = slot["parameters"];
                    if (params.isArray()) {  // Changed from isObject() to handle array format
                        for (auto& paramEntry : *params.getArray()) {
                            if (paramEntry.isObject()) {
                                juce::String paramName = paramEntry["name"].toString();
                                float value = static_cast<float>(paramEntry["value"]);

                                // Map to actual parameter ID
                                juce::String paramID = "slot" + juce::String(i + 1) + "_" + paramName;
                                if (auto* parameter = audioProcessor.getValueTreeState().getParameter(paramID)) {
                                    parameter->setValueNotifyingHost(value);
                                }
                            }
                        }
                    }
                }

                // Apply mix
                if (slot.hasProperty("mix")) {
                    float mix = static_cast<float>(slot["mix"]);
                    juce::String mixID = "slot" + juce::String(i + 1) + "_mix";
                    if (auto* mixParam = audioProcessor.getValueTreeState().getParameter(mixID)) {
                        mixParam->setValueNotifyingHost(mix);
                    }
                }
            }
        }
    }

    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, textColor);
    progressLabel.setText("", juce::dontSendNotification);
}

// =====================================================================
// Trinity Health Check Functions
// =====================================================================

void ChimeraAudioProcessorEditor_Pi::checkTrinityHealth()
{
    // Run health check in background thread to avoid blocking UI
    juce::Thread::launch([this]() {
        juce::File outputFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                       .getChildFile("trinity_health.txt");

        // Use curl with timeout to check Trinity /health endpoint
        juce::String curlCommand = "curl -s -m 3 -w '%{http_code}' -o " + 
                                  outputFile.getFullPathName().quoted() + " " +
                                  trinityServerUrl + "/health";

        auto startTime = juce::Time::getMillisecondCounterHiRes();
        int exitCode = system(curlCommand.toRawUTF8());
        auto responseTime = juce::Time::getMillisecondCounterHiRes() - startTime;

        TrinityHealthStatus newStatus = TrinityHealthStatus::Unreachable;

        if (exitCode == 0 && outputFile.existsAsFile()) {
            juce::String response = outputFile.loadFileAsString();
            
            // Check if response contains HTTP 200
            if (response.contains("\"status\":") && response.contains("\"healthy\"")) {
                if (responseTime < 1000.0) {  // Less than 1 second
                    newStatus = TrinityHealthStatus::Healthy;
                } else {
                    newStatus = TrinityHealthStatus::Slow;
                }
                DBG("Trinity health check: Healthy (" + juce::String(responseTime, 0) + "ms)");
            } else {
                DBG("Trinity health check: Bad response - " + response);
            }
        } else {
            DBG("Trinity health check: Unreachable (curl exit code: " + juce::String(exitCode) + ")");
        }

        outputFile.deleteFile();

        // Update status on message thread
        juce::MessageManager::callAsync([this, newStatus]() {
            trinityHealth = newStatus;
            trinityFeaturesEnabled = (newStatus == TrinityHealthStatus::Healthy || 
                                     newStatus == TrinityHealthStatus::Slow);
            updateTrinityHealthIndicator();
        });
    });
}

void ChimeraAudioProcessorEditor_Pi::updateTrinityHealthIndicator()
{
    // Only show colored dot, no text
    juce::Colour statusColor;

    switch (trinityHealth) {
        case TrinityHealthStatus::Healthy:
            statusColor = juce::Colours::green;
            break;

        case TrinityHealthStatus::Slow:
            statusColor = juce::Colours::yellow;
            break;

        case TrinityHealthStatus::Unreachable:
            statusColor = juce::Colours::red;
            break;

        case TrinityHealthStatus::Unknown:
        default:
            statusColor = juce::Colours::grey;
            break;
    }

    trinityHealthLabel.setText("●", juce::dontSendNotification);
    trinityHealthLabel.setColour(juce::Label::textColourId, statusColor);

    // Update voice button state based on Trinity availability
    if (!trinityFeaturesEnabled) {
        voiceButton.setEnabled(false);
        voiceButton.setButtonText("Trinity Offline");
        voiceButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
    } else if (!isRecording) {
        voiceButton.setEnabled(true);
        voiceButton.setButtonText("HOLD TO SPEAK");
        voiceButton.setColour(juce::TextButton::buttonColourId, accentColor);
    }
}

// =====================================================================
// VoiceRecorder LOCK-FREE IMPLEMENTATION
// =====================================================================

// Custom background writer thread class
class VoiceRecorderWriterThread : public juce::Thread
{
public:
    VoiceRecorderWriterThread(
        juce::AbstractFifo& fifo,
        juce::AudioBuffer<float>& buffer,
        juce::AudioFormatWriter* writer,
        std::atomic<bool>& shouldStop,
        std::atomic<int>& samplesRecorded,
        std::atomic<float>& maxLevel,
        std::atomic<int>& nonZeroSamples)
        : juce::Thread("VoiceRecorderWriter"),
          audioFifo(fifo),
          fifoBuffer(buffer),
          writer(writer),
          shouldStopFlag(shouldStop),
          samplesRecorded(samplesRecorded),
          maxRecordedLevel(maxLevel),
          nonZeroSamples(nonZeroSamples)
    {
    }

    void run() override
    {
        // Temporary buffer for reading from FIFO
        juce::AudioBuffer<float> tempBuffer(1, 2048);

        while (!shouldStopFlag.load() && !threadShouldExit())
        {
            // Check if there's data available in FIFO
            int numReady = audioFifo.getNumReady();

            if (numReady > 0)
            {
                // Read in chunks to avoid blocking too long
                int numToRead = juce::jmin(numReady, tempBuffer.getNumSamples());

                // Get read positions from FIFO
                int start1, size1, start2, size2;
                audioFifo.prepareToRead(numToRead, start1, size1, start2, size2);

                // Copy from FIFO buffer to temp buffer
                int destOffset = 0;

                if (size1 > 0)
                {
                    tempBuffer.copyFrom(0, destOffset, fifoBuffer, 0, start1, size1);
                    destOffset += size1;
                }

                if (size2 > 0)
                {
                    tempBuffer.copyFrom(0, destOffset, fifoBuffer, 0, start2, size2);
                }

                // Finish read operation
                audioFifo.finishedRead(size1 + size2);

                // Write to disk (this is blocking, but that's OK on background thread)
                if (writer != nullptr)
                {
                    const float* channelData = tempBuffer.getReadPointer(0);
                    writer->writeFromFloatArrays(&channelData, 1, numToRead);

                    // Update statistics
                    for (int i = 0; i < numToRead; ++i)
                    {
                        float sample = std::abs(channelData[i]);
                        float currentMax = maxRecordedLevel.load();
                        while (sample > currentMax &&
                               !maxRecordedLevel.compare_exchange_weak(currentMax, sample))
                        {
                            // Retry until successful
                        }

                        if (sample > 0.001f)
                        {
                            nonZeroSamples.fetch_add(1);
                        }
                    }

                    samplesRecorded.fetch_add(numToRead);
                }
            }
            else
            {
                // No data available, sleep briefly to avoid busy-waiting
                juce::Thread::sleep(1);
            }
        }

        // Final flush - drain any remaining data in FIFO
        while (audioFifo.getNumReady() > 0)
        {
            int numReady = audioFifo.getNumReady();
            int numToRead = juce::jmin(numReady, tempBuffer.getNumSamples());

            int start1, size1, start2, size2;
            audioFifo.prepareToRead(numToRead, start1, size1, start2, size2);

            int destOffset = 0;
            if (size1 > 0)
            {
                tempBuffer.copyFrom(0, destOffset, fifoBuffer, 0, start1, size1);
                destOffset += size1;
            }

            if (size2 > 0)
            {
                tempBuffer.copyFrom(0, destOffset, fifoBuffer, 0, start2, size2);
            }

            audioFifo.finishedRead(size1 + size2);

            if (writer != nullptr)
            {
                const float* channelData = tempBuffer.getReadPointer(0);
                writer->writeFromFloatArrays(&channelData, 1, numToRead);
                samplesRecorded.fetch_add(numToRead);
            }
        }

        // Flush the writer
        if (writer != nullptr)
        {
            writer->flush();
        }

        DBG("VoiceRecorder writer thread finished. Total samples written: " << samplesRecorded.load());
    }

private:
    juce::AbstractFifo& audioFifo;
    juce::AudioBuffer<float>& fifoBuffer;
    juce::AudioFormatWriter* writer;
    std::atomic<bool>& shouldStopFlag;
    std::atomic<int>& samplesRecorded;
    std::atomic<float>& maxRecordedLevel;
    std::atomic<int>& nonZeroSamples;
};

void ChimeraAudioProcessorEditor_Pi::VoiceRecorder::startRecording(double sampleRate)
{
    if (isRecording.load()) return;

    deviceSampleRate = sampleRate;

    // Create temp file for recording
    recordedFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getChildFile("chimera_voice_" + juce::String(juce::Time::currentTimeMillis()) + ".wav");

    // Create output stream
    outputStream = recordedFile.createOutputStream();

    if (outputStream != nullptr)
    {
        // Create WAV writer for mono
        DBG("Creating WAV writer with sample rate: " << deviceSampleRate);
        writer.reset(wavFormat.createWriterFor(outputStream.get(), deviceSampleRate, 1, 16, {}, 0));

        if (writer != nullptr)
        {
            outputStream.release();  // Writer now owns the stream

            // Reset FIFO and statistics
            audioFifo.reset();
            fifoBuffer.clear();
            samplesRecorded = 0;
            maxRecordedLevel = 0.0f;
            nonZeroSamples = 0;
            fifoOverflowCount = 0;
            shouldStopWriterThread = false;

            // Start background writer thread
            writerThread.reset(new VoiceRecorderWriterThread(
                audioFifo,
                fifoBuffer,
                writer.get(),
                shouldStopWriterThread,
                samplesRecorded,
                maxRecordedLevel,
                nonZeroSamples
            ));

            writerThread->startThread(juce::Thread::Priority::normal);

            // Mark as recording (MUST be last, after everything is set up)
            isRecording = true;

            DBG("VoiceRecorder started - lock-free FIFO mode with background writer thread");
        }
    }
}

// REAL-TIME SAFE: Called from audio thread (processBlock)
void ChimeraAudioProcessorEditor_Pi::VoiceRecorder::recordSamples(const float* inputChannel, int numSamples)
{
    if (!isRecording.load()) return;

    // Check available space in FIFO (lock-free operation)
    int numFree = audioFifo.getFreeSpace();

    if (numSamples <= numFree)
    {
        // Write to FIFO (lock-free, never blocks)
        int start1, size1, start2, size2;
        audioFifo.prepareToWrite(numSamples, start1, size1, start2, size2);

        // Copy audio data to FIFO buffer
        if (size1 > 0)
        {
            fifoBuffer.copyFrom(0, start1, inputChannel, size1);
        }

        if (size2 > 0)
        {
            fifoBuffer.copyFrom(0, start2, inputChannel + size1, size2);
        }

        // Finish write operation
        audioFifo.finishedWrite(size1 + size2);
    }
    else
    {
        // FIFO overflow - drop samples and increment counter
        fifoOverflowCount.fetch_add(1);
        DBG("WARNING: VoiceRecorder FIFO overflow! Dropped " << numSamples << " samples. Total overflows: " << fifoOverflowCount.load());
    }

    // Auto-stop after 10 seconds max (safety check)
    if (samplesRecorded.load() > deviceSampleRate * 10)
    {
        // Signal to stop (this will be handled by message thread)
        // Don't call stopRecording() directly from audio thread!
        juce::MessageManager::callAsync([this]() {
            stopRecording();
        });
    }
}

void ChimeraAudioProcessorEditor_Pi::VoiceRecorder::stopRecording()
{
    if (!isRecording.load()) return;

    DBG("VoiceRecorder stopping...");

    // Mark as no longer recording (prevents new audio from being written to FIFO)
    isRecording = false;

    // Signal writer thread to stop
    shouldStopWriterThread = true;

    // Wait for writer thread to finish (with timeout)
    if (writerThread != nullptr)
    {
        DBG("Waiting for writer thread to finish...");
        writerThread->stopThread(5000);  // 5 second timeout
        writerThread.reset();
    }

    // Close writer and stream
    if (writer != nullptr)
    {
        writer->flush();
        writer.reset();
    }

    if (outputStream != nullptr)
    {
        outputStream.reset();
    }

    DBG("VoiceRecorder stopped. File: " << recordedFile.getFullPathName());
    DBG("Diagnostics: " << getDiagnostics());
}

void ChimeraAudioProcessorEditor_Pi::updateUIFromProgress(const juce::var& progress)
{
    if (!progress.isObject()) return;

    // Read enhanced progress data from JSON
    juce::String stage = progress.hasProperty("stage") ? progress["stage"].toString() : "processing";
    int percent = progress.hasProperty("percent") ? (int)progress["percent"] : 0;
    juce::String message = progress.hasProperty("message") ? progress["message"].toString() : "";
    juce::String presetName = progress.hasProperty("preset_name") ? progress["preset_name"].toString() : "";

    // Convert percent (0-100) to float (0.0-1.0) for progress bar
    float overall = percent / 100.0f;

    // Update progress bar with percentage display
    updateLoadingBarWithPercent(overall, percent);

    // FIXED: Update preset name IMMEDIATELY when available (before stage-specific logic)
    // Filter out placeholder/default names
    bool isPlaceholder = (presetName.isEmpty() ||
                         presetName == "Preset-Generated Name" ||
                         presetName == "Generated Preset");

    if (!isPlaceholder) {
        DBG("📝 Preset name from progress: " << presetName << " (stage: " << stage << ")");
        printf("[PluginEditor] 📝 Preset name: %s (stage: %s)\n", presetName.toRawUTF8(), stage.toRawUTF8());
        fflush(stdout);

        // Store the AI-generated preset name (we'll show it in status messages)
        currentPresetName = presetName;

        // Keep the user's spoken prompt visible in the preset name area
        // (Don't overwrite it - it stays there until completion)
    }

    // Update status with detailed stage-specific messages
    // ENHANCEMENT: Show user's spoken prompt below preset name when available
    juce::String statusText;

    if (stage == "initializing") {
        statusText = "Initializing Trinity AI...";
    } else if (stage == "visionary") {
        if (percent <= 5) {
            statusText = "Starting creative generation...";
        } else if (percent >= 40) {
            statusText = "Creative phase complete";
            if (!currentPresetName.isEmpty()) {
                statusText += ": " + currentPresetName;
            }
        } else {
            statusText = "Analyzing your prompt...";
        }
    } else if (stage == "calculator") {
        statusText = "Calculating parameters";
        if (!currentPresetName.isEmpty()) {
            statusText += " for " + currentPresetName;
        }
    } else if (stage == "alchemist") {
        statusText = "Finalizing";
        if (!currentPresetName.isEmpty()) {
            statusText += " " + currentPresetName;
        }
    } else if (stage == "complete") {
        statusText = "Generated: " + currentPresetName;
        // Show success for 3 seconds, then replace transcription with preset name
        juce::Timer::callAfterDelay(3000, [this]() {
            statusLabel.setText("Ready", juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, textColor);
            // NOW replace the transcription with the preset name
            presetNameLabel.setText(currentPresetName, juce::dontSendNotification);
            presetNameLabel.setColour(juce::Label::textColourId, juce::Colours::green);
        });
    } else if (stage == "error") {
        statusText = "Error: " + message;
        progressLabel.setText("[ERROR]", juce::dontSendNotification);
        progressLabel.setColour(juce::Label::textColourId, errorColor);
    } else if (stage == "transcribing") {
        statusText = "Transcribing audio...";
    } else if (stage == "loading") {
        statusText = "Loading engines...";
    } else {
        statusText = "Processing: " + stage;
    }

    // Add custom message if provided and not an error or complete stage
    if (message.isNotEmpty() && stage != "error" && stage != "complete") {
        statusText = message;
    }

    statusLabel.setText(statusText, juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId,
                         stage == "error" ? errorColor :
                         stage == "complete" ? juce::Colours::green : textColor);

    // Color coding based on progress
    if (percent >= 100) {
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::green);
    } else if (percent >= 50) {
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    } else {
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    }

    loadingProgress = overall;
}

void ChimeraAudioProcessorEditor_Pi::stopProgressMonitoring()
{
    if (progressMonitor != nullptr) {
        progressMonitor->signalThreadShouldExit();
        progressMonitor->waitForThreadToExit(1000);
        progressMonitor.reset();
    }
}



#if ENABLE_GPIO_HARDWARE && defined(__linux__)

// Hardware event handlers

void ChimeraAudioProcessorEditor_Pi::handleEncoderEvent(const EventBus::Event& event)
{
    // Get encoder behavior based on current mode
    auto behavior = controlState->getEncoderBehavior(event.deviceIndex);

    // Calculate parameter delta
    float delta = event.value * behavior.sensitivity;

    // Update the appropriate parameter
    updateParameterFromEncoder(event.deviceIndex, delta);
}

void ChimeraAudioProcessorEditor_Pi::handleSwitchEvent(const EventBus::Event& event)
{
    // Switch events are already handled in the callback for MODE/VARIANT
    // This is for future expansion (e.g., LIVE switch)
    
    // Log the switch event
    DBG("Switch " << event.deviceIndex << " changed to position " << event.intValue);
}

void ChimeraAudioProcessorEditor_Pi::updateParameterFromEncoder(int encoderIndex, float delta)
{
    // Get the parameter to control based on mode and encoder
    auto behavior = controlState->getEncoderBehavior(encoderIndex);
    juce::String paramID = behavior.parameterID;

    // Handle special parameters that are not in the APVTS
    if (paramID == "preset_browse") {
        // TODO: Implement preset browsing
        DBG("Preset browse: " << delta);
        statusLabel.setText("Preset browsing: " + juce::String(delta > 0 ? "Next" : "Previous"), 
                           juce::dontSendNotification);
        return;
    }
    else if (paramID.startsWith("macro_")) {
        // TODO: Implement macro control
        DBG("Macro control: " << paramID << " delta=" << delta);
        statusLabel.setText("Macro: " + paramID.substring(6) + " " + juce::String(delta, 2), 
                           juce::dontSendNotification);
        return;
    }
    else if (paramID.startsWith("ai_")) {
        // TODO: Implement AI control
        DBG("AI control: " << paramID << " delta=" << delta);
        statusLabel.setText("AI: " + paramID.substring(3) + " " + juce::String(delta, 2), 
                           juce::dontSendNotification);
        return;
    }

    // Handle normal APVTS parameters
    auto* param = audioProcessor.getValueTreeState().getParameter(paramID);
    if (param) {
        // Get current normalized value
        float currentValue = param->getValue();

        // Apply delta
        float newValue = juce::jlimit(0.0f, 1.0f, currentValue + delta);

        // Set the new value
        param->setValueNotifyingHost(newValue);

        // Log the change
        DBG("Parameter " << paramID << " changed to " << newValue);

        // Update status display
        auto& state = controlState->getState();
        juce::String encoderLabel;

        switch (encoderIndex) {
            case 0: encoderLabel = state.encoder1Label; break;
            case 1: encoderLabel = state.encoder2Label; break;
            case 2: encoderLabel = state.encoder3Label; break;
            default: encoderLabel = "Unknown"; break;
        }

        // Show parameter value in status (convert from normalized)
        float displayValue = newValue * 100.0f;  // Show as percentage
        statusLabel.setText(encoderLabel + ": " + juce::String(displayValue, 1) + "%",
                          juce::dontSendNotification);
    } else {
        DBG("Parameter not found: " << paramID);
    }
}

#endif // ENABLE_GPIO_HARDWARE
