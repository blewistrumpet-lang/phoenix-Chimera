#include "VoiceRecordButton.h"

VoiceRecordButton::VoiceRecordButton() {
    // Initialize device manager
    deviceManager = std::make_unique<juce::AudioDeviceManager>();
    
    // Default sample rate - will be updated when device is initialized
    sampleRate = 48000;
    
    // Allocate recording buffer (10 seconds max)
    recordingBuffer.setSize(1, sampleRate * maxRecordingSeconds);
    recordingBuffer.clear();
    
    // Set default color
    micColour = juce::Colours::white.withAlpha(0.8f);
    
    setSize(40, 40); // Default size
}

VoiceRecordButton::~VoiceRecordButton() {
    stopTimer();
    if (deviceManager) {
        deviceManager->removeAudioCallback(this);
        deviceManager->closeAudioDevice();
    }
}

void VoiceRecordButton::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(2);
    
    // Background circle
    if (isRecording) {
        // Pulsing red background when recording
        float pulse = 0.7f + 0.3f * std::sin(pulsePhase);
        g.setColour(juce::Colours::red.withAlpha(pulse * 0.3f));
        g.fillEllipse(bounds);
        
        // Red border
        g.setColour(juce::Colours::red.withAlpha(pulse));
        g.drawEllipse(bounds, 2.0f);
    } else if (isProcessing) {
        // Spinning animation when processing
        g.setColour(juce::Colours::orange.withAlpha(0.3f));
        g.fillEllipse(bounds);
        
        // Rotating border segments
        g.setColour(juce::Colours::orange);
        juce::Path arc;
        auto center = bounds.getCentre();
        auto radius = bounds.getWidth() * 0.5f;
        
        for (int i = 0; i < 3; ++i) {
            float startAngle = pulsePhase + (i * 2.0f * juce::MathConstants<float>::pi / 3.0f);
            float endAngle = startAngle + juce::MathConstants<float>::pi / 3.0f;
            
            arc.clear();
            arc.addCentredArc(center.x, center.y, radius, radius,
                            0, startAngle, endAngle, true);
            g.strokePath(arc, juce::PathStrokeType(2.0f));
        }
    } else {
        // Normal state - subtle background
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillEllipse(bounds);
        
        // Subtle border
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawEllipse(bounds, 1.0f);
    }
    
    // Draw microphone icon
    auto iconBounds = bounds.reduced(bounds.getWidth() * 0.25f);
    drawMicrophoneIcon(g, iconBounds, isRecording);
    
    // Show recording level
    if (isRecording && recordingLevel > 0.01f) {
        // Level indicator ring
        g.setColour(juce::Colours::lime.withAlpha(0.6f));
        float levelAngle = recordingLevel * juce::MathConstants<float>::twoPi;
        juce::Path levelArc;
        levelArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
                              bounds.getWidth() * 0.45f, bounds.getHeight() * 0.45f,
                              0, -juce::MathConstants<float>::halfPi,
                              -juce::MathConstants<float>::halfPi + levelAngle, true);
        g.strokePath(levelArc, juce::PathStrokeType(3.0f));
    }
}

void VoiceRecordButton::drawMicrophoneIcon(juce::Graphics& g, juce::Rectangle<float> bounds, bool filled) {
    // Microphone body
    auto micBody = bounds.reduced(bounds.getWidth() * 0.3f, bounds.getHeight() * 0.1f);
    micBody = micBody.withHeight(micBody.getHeight() * 0.5f);
    micBody = micBody.withY(bounds.getY() + bounds.getHeight() * 0.15f);
    
    g.setColour(isRecording ? juce::Colours::white : micColour);
    
    // Mic capsule (rounded rectangle)
    g.fillRoundedRectangle(micBody, micBody.getWidth() * 0.5f);
    
    // Mic stand
    float standWidth = bounds.getWidth() * 0.08f;
    auto standRect = juce::Rectangle<float>(
        bounds.getCentreX() - standWidth * 0.5f,
        micBody.getBottom(),
        standWidth,
        bounds.getHeight() * 0.25f
    );
    g.fillRect(standRect);
    
    // Mic base
    auto baseRect = juce::Rectangle<float>(
        bounds.getCentreX() - bounds.getWidth() * 0.2f,
        standRect.getBottom(),
        bounds.getWidth() * 0.4f,
        bounds.getHeight() * 0.08f
    );
    g.fillRect(baseRect);
    
    // Mic arc (around the mic)
    if (!filled) {
        g.setColour(micColour.withAlpha(0.5f));
        juce::Path arc;
        auto arcBounds = micBody.expanded(bounds.getWidth() * 0.1f, bounds.getHeight() * 0.05f);
        arc.addArc(arcBounds.getX(), arcBounds.getY(), arcBounds.getWidth(), arcBounds.getHeight(),
                  juce::MathConstants<float>::pi * 0.2f,
                  juce::MathConstants<float>::pi * 1.8f, true);
        g.strokePath(arc, juce::PathStrokeType(1.5f));
    }
}

void VoiceRecordButton::mouseDown(const juce::MouseEvent& event) {
    if (!isProcessing) {
        startRecording();
    }
}

void VoiceRecordButton::mouseUp(const juce::MouseEvent& event) {
    if (isRecording) {
        stopRecording();
    }
}

void VoiceRecordButton::startRecording() {
    if (isRecording) return;
    
    DBG("Starting voice recording...");
    
    try {
        // Initialize audio device if not already done
        if (!deviceManager->getCurrentAudioDevice()) {
            // Try to get available input devices first
            const auto& inputDevices = deviceManager->getAvailableDeviceTypes();
            if (inputDevices.isEmpty()) {
                DBG("No audio device types available");
                showMicrophoneError("No audio input devices found.");
                return;
            }
            
            // Setup with explicit settings
            juce::AudioDeviceManager::AudioDeviceSetup setup;
            setup.inputChannels = 1;  // Mono input
            setup.outputChannels = 0; // No output needed
            setup.sampleRate = 48000;
            setup.bufferSize = 512;
            
            auto result = deviceManager->initialise(1, 0, nullptr, true, {}, &setup);
            if (result.isNotEmpty()) {
                DBG("Failed to initialize audio input: " << result);
                showMicrophoneError("Could not access microphone.\n\n"
                                  "Please check:\n"
                                  "1. System Preferences > Security & Privacy > Microphone\n"
                                  "2. Ensure ChimeraPhoenix is allowed\n"
                                  "3. No other app is using the microphone");
                return;
            }
            
            // Update sample rate from actual device
            if (auto* device = deviceManager->getCurrentAudioDevice()) {
                sampleRate = static_cast<int>(device->getCurrentSampleRate());
                DBG("Audio device initialized at " << sampleRate << "Hz");
                
                // Reallocate buffer with correct sample rate
                recordingBuffer.setSize(1, sampleRate * maxRecordingSeconds);
            }
        }
        
        // Clear buffer
        recordingBuffer.clear();
        writePosition = 0;
        recordingLevel = 0.0f;
        
        // Start audio callback
        deviceManager->addAudioCallback(this);
        
        isRecording = true;
        startTimer(50); // Update UI at 20fps
        repaint();
        
    } catch (const std::exception& e) {
        DBG("Exception in startRecording: " << e.what());
        showMicrophoneError("Recording failed: " + juce::String(e.what()));
    }
}

void VoiceRecordButton::showMicrophoneError(const juce::String& message) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon,
        "Microphone Access",
        message,
        "OK");
}

void VoiceRecordButton::stopRecording() {
    if (!isRecording) return;
    
    DBG("Stopping voice recording...");
    
    isRecording = false;
    isProcessing = true;
    
    // Stop audio callback
    deviceManager->removeAudioCallback(this);
    
    // Send for transcription
    sendAudioForTranscription();
    
    repaint();
}

void VoiceRecordButton::sendAudioForTranscription() {
    // Create WAV file in memory
    juce::MemoryOutputStream wavStream;
    juce::WavAudioFormat wavFormat;
    
    // Create audio format writer
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(&wavStream, sampleRate, 1, 16, {}, 0)
    );
    
    if (writer) {
        // Write the recorded audio
        writer->writeFromAudioSampleBuffer(recordingBuffer, 0, writePosition);
        writer.reset(); // Flush and close
        
        // Get the WAV data
        auto wavData = wavStream.getMemoryBlock();
        
        // Send to server
        juce::URL url(serverUrl + "/transcribe");
        
        // Create multipart form data
        juce::MemoryBlock postData;
        juce::String boundary = "----JUCEFormBoundary" + juce::String(juce::Random::getSystemRandom().nextInt());
        
        // Build multipart form
        juce::MemoryOutputStream formStream(postData, false);
        formStream << "--" << boundary << "\r\n";
        formStream << "Content-Disposition: form-data; name=\"audio\"; filename=\"recording.wav\"\r\n";
        formStream << "Content-Type: audio/wav\r\n\r\n";
        formStream.write(wavData.getData(), wavData.getSize());
        formStream << "\r\n--" << boundary << "--\r\n";
        
        // Send async request  
        // Use a lambda to handle the response
        juce::Thread::launch([this, url, postData, boundary]() {
            auto urlWithData = url.withPOSTData(postData);
            auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                .withExtraHeaders("Content-Type: multipart/form-data; boundary=" + boundary)
                .withConnectionTimeoutMs(30000);
            
            std::unique_ptr<juce::InputStream> stream(
                urlWithData.createInputStream(options)
            );
            
            if (stream) {
                auto response = stream->readEntireStreamAsString();
                
                // Parse JSON response
                juce::var result;
                if (juce::JSON::parse(response, result).wasOk()) {
                    if (result["success"].toString() == "true") {
                        juce::String transcribedText = result["text"].toString();
                        
                        DBG("Transcription: " << transcribedText);
                        
                        // Call the callback on the message thread
                        juce::MessageManager::callAsync([this, transcribedText]() {
                            if (onTranscriptionComplete) {
                                onTranscriptionComplete(transcribedText);
                            }
                            isProcessing = false;
                            stopTimer();
                            repaint();
                        });
                    } else {
                        DBG("Transcription failed: " << result["message"].toString());
                        juce::MessageManager::callAsync([this]() {
                            isProcessing = false;
                            stopTimer();
                            repaint();
                        });
                    }
                } else {
                    DBG("Failed to parse transcription response");
                    juce::MessageManager::callAsync([this]() {
                        isProcessing = false;
                        stopTimer();
                        repaint();
                    });
                }
            } else {
                DBG("Failed to connect to transcription server");
                juce::MessageManager::callAsync([this]() {
                    isProcessing = false;
                    stopTimer();
                    repaint();
                });
            }
        });
    }
}

void VoiceRecordButton::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context) {
    
    // We only care about input
    if (numInputChannels > 0 && isRecording) {
        // Calculate how many samples we can write
        int samplesToWrite = juce::jmin(numSamples, 
                                       recordingBuffer.getNumSamples() - writePosition);
        
        if (samplesToWrite > 0) {
            // Copy input to recording buffer (mono)
            recordingBuffer.copyFrom(0, writePosition, inputChannelData[0], samplesToWrite);
            
            // Calculate level for visual feedback
            float level = 0.0f;
            for (int i = 0; i < samplesToWrite; ++i) {
                level = juce::jmax(level, std::abs(inputChannelData[0][i]));
            }
            recordingLevel = level;
            
            writePosition += samplesToWrite;
            
            // Auto-stop if buffer is full
            if (writePosition >= recordingBuffer.getNumSamples()) {
                juce::MessageManager::callAsync([this]() {
                    stopRecording();
                });
            }
        }
    }
    
    // Clear output (we're not playing anything)
    for (int i = 0; i < numOutputChannels; ++i) {
        if (outputChannelData[i]) {
            juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
        }
    }
}

void VoiceRecordButton::timerCallback() {
    // Animate the UI
    pulsePhase += 0.1f;
    if (pulsePhase > juce::MathConstants<float>::twoPi) {
        pulsePhase -= juce::MathConstants<float>::twoPi;
    }
    
    // Decay recording level for smoother animation
    recordingLevel *= 0.9f;
    
    repaint();
}

void VoiceRecordButton::resized() {
    // Ensure we maintain aspect ratio
    auto size = juce::jmin(getWidth(), getHeight());
    setSize(size, size);
}