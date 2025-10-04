#pragma once

#include <JuceHeader.h>
#include <functional>

/**
 * VoiceRecordButton - A microphone button for voice input
 * Click to start recording, release to stop and transcribe
 */
class VoiceRecordButton : public juce::Component,
                         public juce::Timer,
                         private juce::AudioIODeviceCallback {
public:
    VoiceRecordButton();
    ~VoiceRecordButton() override;
    
    // Callback when transcription is complete
    std::function<void(const juce::String&)> onTranscriptionComplete;
    
    // Set server URL for transcription
    void setServerUrl(const juce::String& url) { serverUrl = url; }
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void resized() override;
    
    // Timer callback for animation
    void timerCallback() override;
    
private:
    // Recording state
    bool isRecording = false;
    bool isProcessing = false;
    float recordingLevel = 0.0f;
    
    // Visual state
    float pulsePhase = 0.0f;
    juce::Colour micColour;
    
    // Audio recording
    void startRecording();
    void stopRecording();
    void sendAudioForTranscription();
    void showMicrophoneError(const juce::String& message);
    
    // AudioIODeviceCallback implementation
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                         int numInputChannels,
                                         float* const* outputChannelData,
                                         int numOutputChannels,
                                         int numSamples,
                                         const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override {}
    void audioDeviceStopped() override {}
    
    // Audio setup
    std::unique_ptr<juce::AudioDeviceManager> deviceManager;
    juce::AudioBuffer<float> recordingBuffer;
    int writePosition = 0;
    const int maxRecordingSeconds = 10;
    int sampleRate = 48000;
    
    // Server communication
    juce::String serverUrl = "http://localhost:8000";
    
    // Icons
    void drawMicrophoneIcon(juce::Graphics& g, juce::Rectangle<float> bounds, bool filled = false);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceRecordButton)
};