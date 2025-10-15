#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor_Pi_Components.h"

/**
 * Raspberry Pi UI - Voice-controlled Trinity AI preset generation
 * Features:
 * - Voice input via Whisper API (USB mic)
 * - Trinity AI integration (same 3-agent system as Mac)
 * - Loading progress bar
 * - Preset name display
 * - Minimal 480x320 display for 3.5" OLED
 * - Trinity health monitoring
 */
class ChimeraAudioProcessorEditor_Pi : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    ChimeraAudioProcessorEditor_Pi(ChimeraAudioProcessor&);
    ~ChimeraAudioProcessorEditor_Pi() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // Public method to feed audio from processor to voice recorder
    void feedVoiceRecorder(const float* channel2Data, int numSamples);

private:
    void startVoiceRecording();
    void stopVoiceRecording();
    void sendToWhisper(const juce::File& audioFile);
    void sendTrinityRequest(const juce::String& prompt);
    void updateLoadingBar(float progress);
    void updateLoadingBarWithPercent(float progress, int percent);
    void applyTrinityPreset(const juce::var& preset);

    // Trinity health check functions
    void checkTrinityHealth();
    void updateTrinityHealthIndicator();

    // Engine category helper for color coding
    int getEngineCategoryFromName(const juce::String& engineName);

    ChimeraAudioProcessor& audioProcessor;

    // =====================================================================
    // LOCK-FREE VOICE RECORDER - Real-time safe for audio thread
    // =====================================================================
    class VoiceRecorder {
    public:
        VoiceRecorder()
            : audioFifo(fifoSize)
        {
            // Pre-allocate FIFO buffer
            fifoBuffer.setSize(1, fifoSize);
            fifoBuffer.clear();
        }

        ~VoiceRecorder() {
            stopRecording();
        }

        void startRecording(double sampleRate);
        void stopRecording();

        // REAL-TIME SAFE: Called from audio thread (processBlock)
        // Only writes to lock-free FIFO, NEVER blocks
        void recordSamples(const float* inputChannel, int numSamples);

        juce::File getRecordedFile() { return recordedFile; }
        bool isCurrentlyRecording() const { return isRecording.load(); }

        bool hasValidAudio() const {
            return maxRecordedLevel > 0.01f && nonZeroSamples > 100;
        }

        juce::String getDiagnostics() const {
            return "Max level: " + juce::String(maxRecordedLevel, 3)
                 + ", Active samples: " + juce::String(nonZeroSamples)
                 + ", Total samples: " + juce::String(samplesRecorded)
                 + ", FIFO overflow count: " + juce::String(fifoOverflowCount.load());
        }

    private:
        // Background writer thread function
        void writerThreadFunction();

        // File writing members
        juce::File recordedFile;
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::FileOutputStream> outputStream;
        std::unique_ptr<juce::AudioFormatWriter> writer;

        // Thread-safe state
        std::atomic<bool> isRecording { false };
        std::atomic<bool> shouldStopWriterThread { false };

        // Statistics (updated from audio thread, read from message thread)
        std::atomic<int> samplesRecorded { 0 };
        std::atomic<float> maxRecordedLevel { 0.0f };
        std::atomic<int> nonZeroSamples { 0 };
        std::atomic<int> fifoOverflowCount { 0 };

        double deviceSampleRate = 48000.0;

        // =====================================================================
        // LOCK-FREE FIFO BUFFER - Real-time safe communication
        // =====================================================================
        static constexpr int fifoSize = 48000 * 10;  // 10 seconds at 48kHz
        juce::AbstractFifo audioFifo;
        juce::AudioBuffer<float> fifoBuffer;

        // Background writer thread
        std::unique_ptr<juce::Thread> writerThread;
    };

    VoiceRecorder voiceRecorder;
    juce::AudioDeviceManager voiceDeviceManager;
    bool isRecording = false;
    juce::File recordedVoiceFile;

    // UI Components
    juce::Label titleLabel;
    juce::Label presetNameLabel;          // Shows Trinity-generated preset name
    juce::Label statusLabel;              // Shows current state
    juce::Label progressLabel;            // Loading bar (ASCII art)
    GradientButton voiceButton{"HOLD TO SPEAK"};  // Hold to speak with gradient styling
    juce::Label trinityHealthLabel;       // Trinity health indicator

    // Engine display (all 6 slots)

    // New gradient UI components
    GradientMeter inputMeter;
    GradientMeter outputMeter;
    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;

    // Engine slot grid - 6 colored boxes showing active engines
    EngineSlotGrid engineSlotGrid;

    // Trinity state
    juce::String currentPresetName = "No Preset";
    juce::String currentPrompt = "";
    float loadingProgress = 0.0f;
    bool isTrinityProcessing = false;
    
    // Trinity health monitoring
    enum class TrinityHealthStatus {
        Unknown,
        Healthy,
        Slow,
        Unreachable
    };
    TrinityHealthStatus trinityHealth = TrinityHealthStatus::Unknown;
    int healthCheckCounter = 0;  // Counter for 30-second health checks
    bool trinityFeaturesEnabled = true;  // DIAGNOSTIC: Enabled by default

    // Server config (can point to Mac or Pi)
    juce::String trinityServerUrl = "http://localhost:8000";  // Default to localhost
    juce::String whisperServerUrl = "https://api.openai.com/v1/audio/transcriptions";

    // ChimeraDesign - Premium Apple-esque color palette
    // Backgrounds
    juce::Colour bgPrimary = juce::Colour(0xff0a0a0a);      // #0A0A0A - Warmer black
    juce::Colour bgSecondary = juce::Colour(0xff1a1a1a);    // #1A1A1A - Secondary bg
    juce::Colour surfaceDark = juce::Colour(0xff1c1c1e);    // #1C1C1E - Empty slots
    juce::Colour surfaceLight = juce::Colour(0xff2c2c2e);   // #2C2C2E - Hover states

    // Brand colors
    juce::Colour brandBlue = juce::Colour(0xff0a84ff);      // #0A84FF - Button start
    juce::Colour brandPurple = juce::Colour(0xff5e5ce6);    // #5E5CE6 - Button end

    // Category colors (subtle for active slots)
    juce::Colour catDynamics = juce::Colour(0xffbf5af2);    // #BF5AF2 Purple
    juce::Colour catFilters = juce::Colour(0xff30d158);     // #30D158 Green
    juce::Colour catDistortion = juce::Colour(0xffff453a);  // #FF453A Red
    juce::Colour catModulation = juce::Colour(0xff0a84ff);  // #0A84FF Blue
    juce::Colour catReverb = juce::Colour(0xff64d2ff);      // #64D2FF Cyan
    juce::Colour catSpatial = juce::Colour(0xffff9f0a);     // #FF9F0A Orange
    juce::Colour catUtility = juce::Colour(0xff98989d);     // #98989D Gray

    // Status colors
    juce::Colour statusSuccess = juce::Colour(0xff30d158);  // #30D158 Green
    juce::Colour statusWarning = juce::Colour(0xffffd60a);  // #FFD60A Yellow
    juce::Colour statusError = juce::Colour(0xffff453a);    // #FF453A Red
    juce::Colour statusRecording = juce::Colour(0xffff453a);// #FF453A Red pulse
    juce::Colour statusProcessing = juce::Colour(0xffff9f0a);// #FF9F0A Orange

    // Text colors
    juce::Colour textPrimary = juce::Colours::white;                           // 100% white
    juce::Colour textSecondary = juce::Colours::white.withAlpha(0.5f);        // 50% white
    juce::Colour textTertiary = juce::Colours::white.withAlpha(0.3f);         // 30% white

    // Legacy aliases for compatibility
    juce::Colour bgColor = bgPrimary;
    juce::Colour cardBg = surfaceDark;
    juce::Colour textColor = textPrimary;
    juce::Colour accentColor = brandBlue;
    juce::Colour errorColor = statusError;
    juce::Colour successGreen = statusSuccess;


    // =====================================================================
    // FILE-BASED PROGRESS MONITOR - Polls /tmp/trinity_progress/ files
    // =====================================================================
    class FileProgressMonitor : public juce::Thread {
    public:
        FileProgressMonitor(const juce::String& requestId)
            : Thread("TrinityProgressMonitor"), requestId(requestId) {}

        void run() override {
            juce::File progressFile = juce::File("/tmp/trinity_progress/" + requestId + ".json");
            juce::Time lastModTime;
            int pollCount = 0;
            const int maxPolls = 600; // 200ms * 600 = 120 seconds max

            while (!threadShouldExit() && pollCount < maxPolls) {
                if (progressFile.existsAsFile()) {
                    juce::Time currentMod = progressFile.getLastModificationTime();

                    if (currentMod != lastModTime) {
                        lastModTime = currentMod;

                        juce::String jsonStr = progressFile.loadFileAsString();
                        juce::var progressData = juce::JSON::parse(jsonStr);

                        if (progressData.isObject() && onProgressUpdate) {
                            juce::MessageManager::callAsync([this, progressData]() {
                                onProgressUpdate(progressData);
                            });
                        }

                        // Stop if complete
                        if (progressData.hasProperty("overall_progress")) {
                            float overall = progressData["overall_progress"];
                            if (overall >= 0.99f) {
                                if (onComplete) {
                                    juce::MessageManager::callAsync([this]() {
                                        onComplete();
                                    });
                                }
                                break;
                            }
                        }
                    }
                }

                juce::Thread::sleep(200); // Poll every 200ms
                pollCount++;
            }
        }

        std::function<void(const juce::var&)> onProgressUpdate;
        std::function<void()> onComplete;

    private:
        juce::String requestId;
    };

    std::unique_ptr<FileProgressMonitor> progressMonitor;
    juce::String currentRequestId;

    void updateUIFromProgress(const juce::var& progress);
    void stopProgressMonitoring();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChimeraAudioProcessorEditor_Pi)
};
