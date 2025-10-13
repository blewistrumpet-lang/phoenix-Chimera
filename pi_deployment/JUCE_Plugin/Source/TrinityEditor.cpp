#include "TrinityEditor.h"

TrinityEditor::TrinityEditor(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set window size to 480×320
    setSize(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Apply Trinity look and feel
    setLookAndFeel(&trinityLookAndFeel);

    // ========================================================================
    // Setup Encoders
    // ========================================================================

    addAndMakeVisible(filterEncoder);
    addAndMakeVisible(mixEncoder);
    addAndMakeVisible(presetEncoder);

    // ========================================================================
    // Setup Voice Button
    // ========================================================================

    addAndMakeVisible(voiceButton);
    voiceButton.onGesture = [this](CompactVoiceButton::GestureType gesture) {
        handleVoiceGesture(gesture);
    };

    // ========================================================================
    // Setup 3-Way Switches
    // ========================================================================

    addAndMakeVisible(abSwitch);
    addAndMakeVisible(voiceModeSwitch);
    addAndMakeVisible(engineModeSwitch);

    // ========================================================================
    // Setup Engine Chain Slots
    // ========================================================================

    for (auto& slot : chainSlots)
    {
        addAndMakeVisible(slot);
        slot.onSlotClicked = [this](int slotIndex) {
            DBG("Slot " + juce::String(slotIndex) + " clicked");
            // TODO: Show engine selector for this slot
        };
    }

    // ========================================================================
    // Start Timer for Real-time Updates (60fps = ~16ms)
    // ========================================================================

    startTimer(16);  // 60fps refresh rate

    // Initial update
    updateEngineSlots();
    checkTrinityServer();
}

TrinityEditor::~TrinityEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

void TrinityEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // ========================================================================
    // Background - Gradient (#1a1a1a → #0d0d0d)
    // ========================================================================

    juce::ColourGradient bgGradient(
        juce::Colour(0xff1a1a1a), 0.0f, 0.0f,
        juce::Colour(0xff0d0d0d), bounds.getWidth(), bounds.getHeight(),
        false
    );
    g.setGradientFill(bgGradient);
    g.fillAll();

    bounds.reduce(PADDING, PADDING);

    // ========================================================================
    // Header Bar (25px)
    // ========================================================================

    auto header = bounds.removeFromTop(HEADER_HEIGHT);

    // "TRINITY" logo with gradient text (cyan → purple)
    {
        g.setFont(juce::Font(juce::FontOptions().withHeight(16.0f)));
        auto logoBounds = header.removeFromLeft(100);

        juce::ColourGradient logoGradient(
            TrinityColors::accentCyan, logoBounds.getX(), logoBounds.getY(),
            TrinityColors::accentPurple, logoBounds.getRight(), logoBounds.getY(),
            false
        );
        g.setGradientFill(logoGradient);
        g.drawText("TRINITY", logoBounds, juce::Justification::centredLeft);
    }

    // A/B Indicator (center-right)
    {
        auto abBounds = header.removeFromRight(150);
        abBounds.removeFromLeft(20); // spacing

        // Container background
        auto abContainer = abBounds.removeFromLeft(50);
        g.setColour(juce::Colour(0x0dffffff)); // rgba(255,255,255,0.05)
        g.fillRoundedRectangle(abContainer.toFloat(), 10.0f);

        abContainer.reduce(6, 3);
        auto aBox = abContainer.removeFromLeft(18);
        auto bBox = abContainer.removeFromRight(18);

        // A state
        if (abState == 0 || abState == 1) {
            juce::ColourGradient aGradient(
                TrinityColors::accentPurple, aBox.getX(), aBox.getY(),
                TrinityColors::accentCyan, aBox.getRight(), aBox.getY(),
                false
            );
            g.setGradientFill(aGradient);
            g.fillRoundedRectangle(aBox.toFloat(), 4.0f);
            g.setColour(juce::Colours::black);
        } else {
            g.setColour(juce::Colour(0xff666666));
        }
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)).boldened());
        g.drawText("A", aBox, juce::Justification::centred);

        // B state
        if (abState == 2 || abState == 1) {
            juce::ColourGradient bGradient(
                TrinityColors::accentPurple, bBox.getX(), bBox.getY(),
                TrinityColors::accentCyan, bBox.getRight(), bBox.getY(),
                false
            );
            g.setGradientFill(bGradient);
            g.fillRoundedRectangle(bBox.toFloat(), 4.0f);
            g.setColour(juce::Colours::black);
        } else {
            g.setColour(juce::Colour(0xff666666));
        }
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)).boldened());
        g.drawText("B", bBox, juce::Justification::centred);
    }

    // Status indicator (right)
    {
        auto statusBounds = header.removeFromRight(60);

        // Pulsing LED
        float pulseAlpha = 0.5f + 0.5f * std::sin(ledPulsePhase);
        juce::Colour ledColor = juce::Colour(0xff00ff88).withAlpha(pulseAlpha);
        g.setColour(ledColor);
        g.fillEllipse(statusBounds.getX(), statusBounds.getCentreY() - 2, 4, 4);

        // Status text
        g.setColour(juce::Colour(0xff888888));
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        g.drawText("READY", statusBounds.removeFromLeft(statusBounds.getWidth()).translated(8, 0),
                   juce::Justification::centredLeft);
    }

    bounds.removeFromTop(MARGIN);

    // ========================================================================
    // Main Display Area (165px)
    // ========================================================================

    auto mainDisplay = bounds.removeFromTop(MAIN_DISPLAY_HEIGHT);

    // Black background with subtle border
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(mainDisplay.toFloat(), 8.0f);

    g.setColour(juce::Colour(0xff7b68ee).withAlpha(0.1f));
    g.drawRoundedRectangle(mainDisplay.toFloat(), 8.0f, 1.0f);

    mainDisplay.reduce(8, 8);

    // ========================================================================
    // Preset Section (top of main display)
    // ========================================================================

    auto presetSection = mainDisplay.removeFromTop(28);

    // Left: Preset info
    auto presetInfo = presetSection.removeFromLeft(presetSection.getWidth() - 100);

    // Preset number
    g.setColour(juce::Colour(0xff666666));
    g.setFont(juce::Font(juce::FontOptions().withHeight(8.0f)));
    auto presetNumBounds = presetInfo.removeFromTop(10);
    g.drawText("PRESET " + currentPresetNumber, presetNumBounds, juce::Justification::topLeft);

    // Preset name (gold)
    g.setColour(TrinityColors::accentGold);
    g.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    g.drawText(currentPresetName, presetInfo, juce::Justification::topLeft);

    // Right: Mode badges
    auto modeBadges = presetSection;

    // Voice mode badge
    auto voiceBadge = modeBadges.removeFromTop(12);
    g.setColour(juce::Colour(0xff00ffcc).withAlpha(0.1f));
    g.fillRoundedRectangle(voiceBadge.toFloat(), 6.0f);
    g.setColour(juce::Colour(0xff00ffcc).withAlpha(0.3f));
    g.drawRoundedRectangle(voiceBadge.toFloat(), 6.0f, 1.0f);
    g.setColour(TrinityColors::accentCyan);
    g.setFont(juce::Font(juce::FontOptions().withHeight(7.0f)).boldened());
    g.drawText("VOICE: EDIT", voiceBadge, juce::Justification::centred);

    modeBadges.removeFromTop(2);

    // Engine mode badge
    auto engineBadge = modeBadges.removeFromTop(12);
    g.setColour(TrinityColors::accentGold.withAlpha(0.1f));
    g.fillRoundedRectangle(engineBadge.toFloat(), 6.0f);
    g.setColour(TrinityColors::accentGold.withAlpha(0.3f));
    g.drawRoundedRectangle(engineBadge.toFloat(), 6.0f, 1.0f);
    g.setColour(TrinityColors::accentGold);
    g.setFont(juce::Font(juce::FontOptions().withHeight(7.0f)).boldened());
    g.drawText("ENGINE: HYBRID", engineBadge, juce::Justification::centred);

    mainDisplay.removeFromTop(8);

    // Skip encoders row
    mainDisplay.removeFromTop(40);
    mainDisplay.removeFromTop(8);

    // Skip voice button and draw hint text
    mainDisplay.removeFromTop(30);  // Voice button
    auto voiceHintArea = mainDisplay.removeFromTop(8);  // Hint text area
    g.setColour(juce::Colour(0xff666666));
    g.setFont(juce::Font(juce::FontOptions().withHeight(6.0f)));
    g.drawText("HOLD: TAP TEMPO • DOUBLE: PANIC", voiceHintArea, juce::Justification::centred);

    // Skip switches
    mainDisplay.removeFromTop(42);
    mainDisplay.removeFromTop(5);

    // ========================================================================
    // Signal Chain Section
    // ========================================================================

    auto chainSection = mainDisplay;

    // Background
    g.setColour(juce::Colour(0x99000000)); // rgba(0,0,0,0.6)
    g.fillRoundedRectangle(chainSection.toFloat(), 6.0f);
    g.setColour(juce::Colour(0xff7b68ee).withAlpha(0.1f));
    g.drawRoundedRectangle(chainSection.toFloat(), 6.0f, 1.0f);

    chainSection.reduce(6, 6);

    // Chain header
    auto chainHeader = chainSection.removeFromTop(12);
    g.setColour(juce::Colour(0xff888888));
    g.setFont(juce::Font(juce::FontOptions().withHeight(7.0f)).boldened());
    g.drawText("SIGNAL CHAIN", chainHeader.removeFromLeft(80), juce::Justification::centredLeft);

    // Active count
    int activeCount = 0;
    for (const auto& slot : chainSlots) {
        if (slot.getState() != ChainSlot::SlotState::Inactive) {
            activeCount++;
        }
    }
    g.setColour(TrinityColors::accentCyan);
    g.drawText(juce::String(activeCount) + " ACTIVE", chainHeader, juce::Justification::centredRight);

    chainSection.removeFromTop(2);

    // Draw arrows between slots (will be drawn over slot positions)
    auto modulesRow = chainSection.removeFromTop(28);
    g.setColour(juce::Colour(0xff444444));
    g.setFont(juce::Font(juce::FontOptions().withHeight(8.0f)));

    int slotWidth = (modulesRow.getWidth() - 40) / 11;
    int arrowX = modulesRow.getX() + slotWidth + 2;

    for (int i = 0; i < 5; ++i) {
        g.drawText(">", arrowX, modulesRow.getY() + 10, 8, 10, juce::Justification::centred);
        arrowX += slotWidth + (slotWidth / 2);
    }

    // ========================================================================
    // Progress Indicator (if Trinity AI is processing)
    // ========================================================================

    if (trinityProgress > 0.0f && trinityProgress < 1.0f)
    {
        // Position below voice button
        auto progressBounds = mainDisplay.removeFromBottom(15).reduced(50, 0);

        // Background
        g.setColour(TrinityColors::encoderRing);
        g.fillRoundedRectangle(progressBounds.toFloat(), 3.0f);

        // Progress fill
        auto progressWidth = progressBounds.getWidth() * trinityProgress;
        auto progressFill = progressBounds.toFloat();
        progressFill.setWidth(progressWidth);

        g.setColour(TrinityColors::accentCyan);
        g.fillRoundedRectangle(progressFill, 3.0f);

        // Progress text
        g.setColour(TrinityColors::textPrimary);
        g.setFont(juce::Font(juce::FontOptions().withHeight(8.0f)));
        auto textBounds = progressBounds.translated(0, 16);
        g.drawText(trinityProgressMessage, textBounds, juce::Justification::centred);
    }
}

void TrinityEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.reduce(PADDING, PADDING);

    // ========================================================================
    // Header Bar (25px)
    // ========================================================================

    bounds.removeFromTop(HEADER_HEIGHT);
    bounds.removeFromTop(MARGIN);

    // ========================================================================
    // Main Display Area (165px)
    // ========================================================================

    auto mainDisplay = bounds.removeFromTop(MAIN_DISPLAY_HEIGHT);
    mainDisplay.reduce(8, 8); // Inner padding

    // Preset section (28px)
    mainDisplay.removeFromTop(28);
    mainDisplay.removeFromTop(8);

    // ========================================================================
    // Encoder Row (compact 16px encoders with labels + values)
    // ========================================================================

    auto encoderRow = mainDisplay.removeFromTop(40);
    int encoderSpacing = 35;
    int encoderStartX = (encoderRow.getWidth() - (3 * 16 + 2 * encoderSpacing)) / 2;

    // Filter encoder
    filterEncoder.setBounds(encoderStartX, encoderRow.getY(), 40, 40);
    encoderStartX += 16 + encoderSpacing;

    // Mix encoder
    mixEncoder.setBounds(encoderStartX, encoderRow.getY(), 40, 40);
    encoderStartX += 16 + encoderSpacing;

    // Preset encoder
    presetEncoder.setBounds(encoderStartX, encoderRow.getY(), 40, 40);

    mainDisplay.removeFromTop(8);

    // ========================================================================
    // Voice Button (200×30px, centered)
    // ========================================================================

    auto voiceSection = mainDisplay.removeFromTop(30);
    int voiceBtnX = (voiceSection.getWidth() - 200) / 2;
    voiceButton.setBounds(voiceBtnX, voiceSection.getY(), 200, 30);

    mainDisplay.removeFromTop(2);  // Space for hint text

    // ========================================================================
    // Three-Way Switches Row
    // ========================================================================

    auto switchRow = mainDisplay.removeFromTop(42);  // 16px switch + labels
    int switchSpacing = 30;
    int switchStartX = (switchRow.getWidth() - (3 * 30 + 2 * switchSpacing)) / 2;

    abSwitch.setBounds(switchStartX, switchRow.getY(), 30, 42);
    switchStartX += 30 + switchSpacing;

    voiceModeSwitch.setBounds(switchStartX, switchRow.getY(), 30, 42);
    switchStartX += 30 + switchSpacing;

    engineModeSwitch.setBounds(switchStartX, switchRow.getY(), 30, 42);

    mainDisplay.removeFromTop(5);

    // ========================================================================
    // Signal Chain Section
    // ========================================================================

    auto chainSection = mainDisplay;  // Use remaining space

    // Chain header + modules take ~40px total
    int chainY = chainSection.getY() + 14; // After header
    int slotWidth = (chainSection.getWidth() - 40) / 11; // 6 slots + 5 arrows
    int slotX = chainSection.getX() + 2;

    for (int i = 0; i < 6; ++i)
    {
        chainSlots[i].setBounds(slotX, chainY, slotWidth, 28);
        slotX += slotWidth + (slotWidth / 2); // slot + arrow space
    }
}

void TrinityEditor::timerCallback()
{
    // Update LED pulse animation
    ledPulsePhase += 0.1f;
    if (ledPulsePhase > juce::MathConstants<float>::twoPi) {
        ledPulsePhase -= juce::MathConstants<float>::twoPi;
    }

    // Update engine slots
    updateEngineSlots();

    // Update level meters
    updateLevelMeters();

    // Check Trinity health (every 2 seconds)
    static int healthCheckCounter = 0;
    if (++healthCheckCounter >= 120)  // 120 frames @ 60fps = 2 seconds
    {
        checkTrinityServer();
        healthCheckCounter = 0;
    }

    // Repaint for animations and updates
    repaint();
}

// ============================================================================
// Voice Recording & Trinity AI
// ============================================================================

void TrinityEditor::handleVoiceGesture(CompactVoiceButton::GestureType gesture)
{
    switch (gesture)
    {
        case CompactVoiceButton::GestureType::SingleTap:
            if (isRecording)
                stopRecording();
            else
                startRecording();
            break;

        case CompactVoiceButton::GestureType::Hold:
            // Hold-to-record mode
            startRecording();
            break;

        case CompactVoiceButton::GestureType::DoubleTap:
            // Cancel/reset - stop recording without sending to Trinity
            if (isRecording) {
                voiceRecorder.stopRecording();
                isRecording = false;
            }
            voiceButton.setState(CompactVoiceButton::ButtonState::Idle);
            DBG("Trinity: Voice recording cancelled");
            break;

        default:
            break;
    }
}

void TrinityEditor::startRecording()
{
    // Get sample rate from audio processor
    double sampleRate = audioProcessor.getSampleRate();
    if (sampleRate <= 0) {
        sampleRate = 48000.0; // Default fallback
    }

    // Start the lock-free voice recorder
    voiceRecorder.startRecording(sampleRate);

    if (voiceRecorder.isCurrentlyRecording()) {
        isRecording = true;
        voiceButton.setState(CompactVoiceButton::ButtonState::Recording);
        DBG("Trinity: Started voice recording at " << sampleRate << " Hz");
    } else {
        DBG("ERROR: Failed to start voice recorder!");
    }
}

void TrinityEditor::stopRecording()
{
    if (!isRecording) return;

    // Stop the lock-free voice recorder
    voiceRecorder.stopRecording();
    isRecording = false;

    // Check if we have valid audio
    if (voiceRecorder.hasValidAudio()) {
        voiceButton.setState(CompactVoiceButton::ButtonState::Processing);
        DBG("Trinity: Stopped voice recording - " << voiceRecorder.getDiagnostics());

        // Get the recorded file
        recordedVoiceFile = voiceRecorder.getRecordedFile();

        // Send to Whisper for transcription
        sendToTrinityAI();
    } else {
        // No valid audio captured
        voiceButton.setState(CompactVoiceButton::ButtonState::Idle);
        DBG("WARNING: No valid audio captured! " << voiceRecorder.getDiagnostics());
    }
}

void TrinityEditor::sendToTrinityAI()
{
    // Generate unique request ID for progress tracking
    currentRequestId = "voice_" + juce::String(juce::Time::currentTimeMillis());

    // Start progress monitor BEFORE sending request
    stopProgressMonitoring(); // Clean up any previous monitor
    progressMonitor = std::make_unique<FileProgressMonitor>(currentRequestId);
    progressMonitor->onProgressUpdate = [this](const juce::var& progress) {
        updateUIFromProgress(progress);
    };
    progressMonitor->startThread();

    // Set initial progress state
    trinityProgress = 0.1f;
    trinityProgressMessage = "Sending to Trinity AI...";

    // Prepare JSON request
    juce::DynamicObject::Ptr requestObj = new juce::DynamicObject();
    requestObj->setProperty("prompt", "Voice command"); // TODO: Get actual transcribed text
    requestObj->setProperty("request_id", currentRequestId);
    juce::String jsonRequest = juce::JSON::toString(juce::var(requestObj.get()));

    DBG("Sending to Trinity AI server, request ID: " + currentRequestId);

    // Use curl for HTTP with timeout to prevent hanging
    juce::Thread::launch([this, jsonRequest]() {
        // Write JSON to temp file for curl
        juce::File jsonFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                     .getChildFile("trinity_request.json");
        jsonFile.replaceWithText(jsonRequest);

        juce::File outputFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                       .getChildFile("trinity_response.json");

        // Use -m 60 for 60 second timeout (preset generation can take 30-40 seconds)
        juce::String curlCommand = "curl -s -m 60 -X POST http://localhost:8000/generate "
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
                    stopProgressMonitoring();
                    applyTrinityPreset(preset);
                    trinityProgress = 1.0f;
                    trinityProgressMessage = "Complete!";
                    voiceButton.setState(CompactVoiceButton::ButtonState::Idle);
                });
            } else {
                juce::MessageManager::callAsync([this, response]() {
                    stopProgressMonitoring();
                    trinityProgress = 0.0f;
                    trinityProgressMessage = "Failed: " + response.substring(0, 30);
                    voiceButton.setState(CompactVoiceButton::ButtonState::Idle);
                    // Recheck health after failure
                    checkTrinityServer();
                });
            }
        } else {
            juce::MessageManager::callAsync([this]() {
                stopProgressMonitoring();
                trinityProgress = 0.0f;
                trinityProgressMessage = "Request timeout/failed";
                voiceButton.setState(CompactVoiceButton::ButtonState::Idle);
                // Recheck health after failure
                checkTrinityServer();
            });
        }

        outputFile.deleteFile();
    });
}

// ============================================================================
// Trinity Health Monitoring
// ============================================================================

void TrinityEditor::checkTrinityServer()
{
    // Check Trinity AI server health
    auto healthResponse = trinityClient.checkHealth();

    switch (healthResponse.status)
    {
        case TrinityAIClient::HealthStatus::Healthy:
            trinityHealth = TrinityHealth::Healthy;
            break;
        case TrinityAIClient::HealthStatus::Degraded:
            trinityHealth = TrinityHealth::Degraded;
            break;
        case TrinityAIClient::HealthStatus::Offline:
        default:
            trinityHealth = TrinityHealth::Offline;
            break;
    }
}

void TrinityEditor::updateTrinityHealth()
{
    // Called when health status changes
    repaint();
}

// ============================================================================
// Real-time Updates
// ============================================================================

void TrinityEditor::updateEngineSlots()
{
    for (int i = 0; i < 6; ++i)
    {
        int engineID = audioProcessor.getEngineIDForSlot(i);

        if (engineID == 0)
        {
            chainSlots[i].setState(ChainSlot::SlotState::Inactive, "EMPTY");
        }
        else
        {
            // TODO: Determine engine type (Premium, Hybrid, Experimental) from engineID
            // For now, use Premium for all active engines
            chainSlots[i].setState(ChainSlot::SlotState::Premium, "ENGINE");

            // Update activity level
            float activity = audioProcessor.getSlotActivity(i);
            chainSlots[i].setActivity(activity);
        }
    }
}

void TrinityEditor::updateLevelMeters()
{
    // Get levels from processor
    inputLevel = audioProcessor.getCurrentInputLevel();
    outputLevel = audioProcessor.getCurrentOutputLevel();

    // Smooth decay for visual appeal
    inputLevel *= 0.95f;
    outputLevel *= 0.95f;
}

// ============================================================================
// Trinity AI Progress & Preset Application
// ============================================================================

void TrinityEditor::updateUIFromProgress(const juce::var& progress)
{
    if (!progress.isObject()) return;

    // Read progress data from JSON (simplified for Trinity compact UI)
    int percent = progress.hasProperty("percent") ? (int)progress["percent"] : 0;
    juce::String stage = progress.hasProperty("stage") ? progress["stage"].toString() : "processing";

    // Convert percent (0-100) to float (0.0-1.0) for progress bar
    trinityProgress = percent / 100.0f;

    // Update progress message based on stage
    if (stage == "initializing") {
        trinityProgressMessage = "Initializing...";
    } else if (stage == "visionary") {
        trinityProgressMessage = "Creating...";
    } else if (stage == "calculator") {
        trinityProgressMessage = "Calculating...";
    } else if (stage == "alchemist") {
        trinityProgressMessage = "Finalizing...";
    } else if (stage == "complete") {
        trinityProgressMessage = "Complete!";
        voiceButton.setState(CompactVoiceButton::ButtonState::Idle);
    } else if (stage == "error") {
        trinityProgressMessage = "Error";
        voiceButton.setState(CompactVoiceButton::ButtonState::Idle);
    } else {
        trinityProgressMessage = juce::String(percent) + "%";
    }

    repaint();
}

void TrinityEditor::stopProgressMonitoring()
{
    if (progressMonitor != nullptr) {
        progressMonitor->signalThreadShouldExit();
        progressMonitor->waitForThreadToExit(1000);
        progressMonitor.reset();
    }
}

void TrinityEditor::applyTrinityPreset(const juce::var& preset)
{
    DBG("Applying Trinity preset to audio processor");

    // Apply slots (same logic as PluginEditor_Pi)
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
                    if (params.isArray()) {
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

    // Clear progress
    trinityProgress = 0.0f;
    trinityProgressMessage = "";
}

// ============================================================================
// VOICE RECORDING - feedVoiceRecorder (called from PluginProcessor)
// ============================================================================

void TrinityEditor::feedVoiceRecorder(const float* channel2Data, int numSamples)
{
    // Feed Input 2 audio to the voice recorder when recording
    if (isRecording && voiceRecorder.isCurrentlyRecording()) {
        voiceRecorder.recordSamples(channel2Data, numSamples);
    }
}

// ============================================================================
// VOICE RECORDER - Implementation (Lock-free FIFO with background writer)
// ============================================================================

/**
 * VoiceRecorderWriterThread - Background thread that writes audio from FIFO to WAV file
 * This allows real-time safe recording from the audio thread via lock-free FIFO
 */
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

void TrinityEditor::VoiceRecorder::startRecording(double sampleRate)
{
    if (isRecording.load()) return;

    deviceSampleRate = sampleRate;

    // Create temp file for recording
    recordedFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getChildFile("trinity_voice_" + juce::String(juce::Time::currentTimeMillis()) + ".wav");

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
void TrinityEditor::VoiceRecorder::recordSamples(const float* inputChannel, int numSamples)
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
        // FIFO overflow - drop samples
        DBG("WARNING: VoiceRecorder FIFO overflow! Dropped " << numSamples << " samples.");
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

void TrinityEditor::VoiceRecorder::stopRecording()
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

    DBG("VoiceRecorder stopped. " << getDiagnostics());
}
