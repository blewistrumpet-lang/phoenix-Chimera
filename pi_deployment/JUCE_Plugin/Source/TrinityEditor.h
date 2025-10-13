#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "TrinityLookAndFeel.h"
#include "CompactEncoder.h"
#include "CompactVoiceButton.h"
#include "CompactThreeWaySwitch.h"
#include "ChainSlot.h"
#include "TrinityAIClient.h"

/**
 * TrinityEditor - Compact 480×320 UI for Raspberry Pi 5
 *
 * Layout (480×320px) - Matches trinity_ui.html design:
 * ┌─────────────────────────────────────────────────────────┐
 * │  TRINITY              [A|B]            🟢 READY         │ ← Header (25px)
 * ├─────────────────────────────────────────────────────────┤
 * │  ┌──────────────────────────────────────────────────┐   │
 * │  │ PRESET 042                      VOICE: EDIT      │   │
 * │  │ Celestial Cascade               ENGINE: HYBRID   │   │ ← Main Display (165px)
 * │  │                                                   │   │
 * │  │   FILTER    MIX    PRESET                        │   │   - Preset info
 * │  │     ●        ●       ●                           │   │   - 3 encoders (16px)
 * │  │    2.8k     65%     042                          │   │   - Voice button
 * │  │                                                   │   │   - 3-way switches
 * │  │   [───────── TAP TO SPEAK ──────────]            │   │   - Signal chain
 * │  │   HOLD: TAP TEMPO • DOUBLE: PANIC               │   │
 * │  │                                                   │   │
 * │  │    A/B    VOICE   ENGINE                         │   │
 * │  │    [●]     [●]     [●]                           │   │
 * │  │                                                   │   │
 * │  │  ┌──────── SIGNAL CHAIN ───────┐   4 ACTIVE     │   │
 * │  │  │ PREM › HYBR › EXPR › HYBR › OFF › OFF │     │   │
 * │  │  │ Hall   Tape   Grain  Chorus  ---   --- │     │   │
 * │  │  └────────────────────────────────────────┘     │   │
 * │  └──────────────────────────────────────────────────┘   │
 * └─────────────────────────────────────────────────────────┘
 *
 * Features:
 * - Gradient background (#1a1a1a → #0d0d0d)
 * - Gradient logo text (cyan → purple)
 * - A/B indicator with active highlighting
 * - Pulsing green status LED
 * - Preset name in gold (#ffd700)
 * - Mode badges (cyan/gold with transparency)
 * - 3 compact rotary encoders (16px rings)
 * - Voice button with gradient (purple → cyan)
 * - 3 three-way switches with state labels
 * - 6 signal chain slots with type badges (PREM/HYBR/EXPR)
 * - 60fps refresh rate (16.6ms frame budget)
 */
class TrinityEditor : public juce::AudioProcessorEditor,
                      public juce::Timer
{
public:
    TrinityEditor(ChimeraAudioProcessor& processor);
    ~TrinityEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Public method to feed audio from processor to voice recorder
    void feedVoiceRecorder(const float* channel2Data, int numSamples);

private:
    ChimeraAudioProcessor& audioProcessor;

    // Custom look and feel
    TrinityLookAndFeel trinityLookAndFeel;

    // ========================================================================
    // UI Components
    // ========================================================================

    // Encoders (Filter, Mix, Preset)
    CompactEncoder filterEncoder{"FILT"};
    CompactEncoder mixEncoder{"MIX"};
    CompactEncoder presetEncoder{"PRST"};

    // Voice button
    CompactVoiceButton voiceButton;

    // 3-way switches
    CompactThreeWaySwitch abSwitch{"A/B"};
    CompactThreeWaySwitch voiceModeSwitch{"VOC"};
    CompactThreeWaySwitch engineModeSwitch{"ENG"};

    // Engine chain slots
    std::array<ChainSlot, 6> chainSlots{
        ChainSlot{0}, ChainSlot{1}, ChainSlot{2},
        ChainSlot{3}, ChainSlot{4}, ChainSlot{5}
    };

    // ========================================================================
    // Parameter Attachments
    // ========================================================================

    // Encoder attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> presetAttachment;

    // ========================================================================
    // Voice Recording & Trinity AI
    // ========================================================================

    void handleVoiceGesture(CompactVoiceButton::GestureType gesture);
    void startRecording();
    void stopRecording();
    void sendToTrinityAI();

    // Trinity AI client
    TrinityAIClient trinityClient;
    juce::String currentRequestId;

    // Progress tracking
    float trinityProgress = 0.0f;
    juce::String trinityProgressMessage;

    // ========================================================================
    // Trinity Health Monitoring
    // ========================================================================

    enum class TrinityHealth {
        Healthy,
        Degraded,
        Offline
    };

    TrinityHealth trinityHealth = TrinityHealth::Healthy;
    void updateTrinityHealth();
    void checkTrinityServer();

    // ========================================================================
    // File-based Progress Monitor (from PluginEditor_Pi)
    // ========================================================================

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
    void updateUIFromProgress(const juce::var& progress);
    void stopProgressMonitoring();
    void applyTrinityPreset(const juce::var& preset);

    // ========================================================================
    // VOICE RECORDING - Lock-free FIFO for real-time audio capture
    // ========================================================================

    class VoiceRecorder {
    public:
        VoiceRecorder()
            : audioFifo(fifoSize)
        {
            fifoBuffer.setSize(1, fifoSize);
            fifoBuffer.clear();
        }

        ~VoiceRecorder() {
            stopRecording();
        }

        void startRecording(double sampleRate);
        void stopRecording();

        // REAL-TIME SAFE: Called from audio thread
        void recordSamples(const float* inputChannel, int numSamples);

        juce::File getRecordedFile() { return recordedFile; }
        bool isCurrentlyRecording() const { return isRecording.load(); }

        bool hasValidAudio() const {
            return maxRecordedLevel > 0.01f && nonZeroSamples > 100;
        }

        juce::String getDiagnostics() const {
            return "Max level: " + juce::String(maxRecordedLevel, 3)
                 + ", Active samples: " + juce::String(nonZeroSamples)
                 + ", Total samples: " + juce::String(samplesRecorded);
        }

    private:
        juce::File recordedFile;
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::FileOutputStream> outputStream;
        std::unique_ptr<juce::AudioFormatWriter> writer;

        std::atomic<bool> isRecording { false };
        std::atomic<bool> shouldStopWriterThread { false };
        std::atomic<int> samplesRecorded { 0 };
        std::atomic<float> maxRecordedLevel { 0.0f };
        std::atomic<int> nonZeroSamples { 0 };

        double deviceSampleRate = 48000.0;

        // Lock-free FIFO buffer
        static constexpr int fifoSize = 48000 * 10;  // 10 seconds at 48kHz
        juce::AbstractFifo audioFifo;
        juce::AudioBuffer<float> fifoBuffer;

        std::unique_ptr<juce::Thread> writerThread;
    };

    VoiceRecorder voiceRecorder;
    bool isRecording = false;
    juce::File recordedVoiceFile;

    // ========================================================================
    // Real-time Updates
    // ========================================================================

    void updateEngineSlots();
    void updateLevelMeters();

    // Level meter values
    float inputLevel = 0.0f;
    float outputLevel = 0.0f;

    // ========================================================================
    // Layout Constants (matching trinity_ui.html)
    // ========================================================================

    static constexpr int WINDOW_WIDTH = 480;
    static constexpr int WINDOW_HEIGHT = 320;
    static constexpr int HEADER_HEIGHT = 25;
    static constexpr int MAIN_DISPLAY_HEIGHT = 165;
    static constexpr int PADDING = 8;
    static constexpr int MARGIN = 5;

    // LED animation
    float ledPulsePhase = 0.0f;

    // Preset display
    juce::String currentPresetNumber = "042";
    juce::String currentPresetName = "Celestial Cascade";

    // A/B state
    int abState = 0; // 0=A, 1=Link, 2=B

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrinityEditor)
};
