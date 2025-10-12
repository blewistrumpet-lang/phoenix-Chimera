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

    // Use printf for debugging in both Debug and Release builds
    printf("[VoiceRecordButton] Starting voice recording...\n");
    fflush(stdout);

    try {
        // Initialize audio device if not already done
        if (!deviceManager->getCurrentAudioDevice()) {

            // Setup with explicit settings
            juce::AudioDeviceManager::AudioDeviceSetup setup;
            setup.sampleRate = 48000;
            setup.bufferSize = 512;

            #if JUCE_LINUX
                printf("[VoiceRecordButton] Linux platform detected - configuring ALSA\n");

                // CRITICAL FIX 1: Set ALSA device type BEFORE any device enumeration
                deviceManager->setCurrentAudioDeviceType("ALSA", true);

                // Wait for device type to be set
                juce::Thread::sleep(100);

                // List all available device types for debugging
                auto deviceTypes = deviceManager->getAvailableDeviceTypes();
                printf("[VoiceRecordButton] Available device types: %d\n", deviceTypes.size());
                for (auto* type : deviceTypes) {
                    printf("  - %s\n", type->getTypeName().toRawUTF8());
                }

                auto* currentType = deviceManager->getCurrentDeviceTypeObject();
                if (currentType != nullptr) {
                    printf("[VoiceRecordButton] Current device type: %s\n",
                           currentType->getTypeName().toRawUTF8());

                    auto inputDevices = currentType->getDeviceNames(true);  // true = input devices
                    auto outputDevices = currentType->getDeviceNames(false); // false = output devices

                    printf("[VoiceRecordButton] Found %d input devices:\n", inputDevices.size());
                    for (const auto& device : inputDevices) {
                        printf("  - Input: %s\n", device.toRawUTF8());
                    }

                    printf("[VoiceRecordButton] Found %d output devices:\n", outputDevices.size());
                    for (const auto& device : outputDevices) {
                        printf("  - Output: %s\n", device.toRawUTF8());
                    }

                    // CRITICAL FIX 2: Look for USB device with more flexible matching
                    bool foundUSB = false;
                    for (const auto& device : inputDevices) {
                        // Check for various USB device naming patterns
                        if (device.containsIgnoreCase("USB") ||
                            device.containsIgnoreCase("plughw:2") ||  // Prefer plughw for compatibility
                            device.containsIgnoreCase("hw:2,0") ||
                            device.containsIgnoreCase("Card 2") ||
                            device.contains("2,0")) {
                            setup.inputDeviceName = device;
                            foundUSB = true;
                            printf("[VoiceRecordButton] Selected USB input device: %s\n",
                                   device.toRawUTF8());
                            break;
                        }
                    }

                    // If no USB device found, use the first available input device
                    if (!foundUSB && !inputDevices.isEmpty()) {
                        setup.inputDeviceName = inputDevices[0];
                        printf("[VoiceRecordButton] No USB found, using first available input: %s\n",
                               setup.inputDeviceName.toRawUTF8());
                    }

                    // CRITICAL FIX 3: ALSA often requires output device even for input-only
                    // Set dummy output to allow callback to run
                    if (!outputDevices.isEmpty()) {
                        setup.outputDeviceName = outputDevices[0];  // Use first output device
                        printf("[VoiceRecordButton] Setting dummy output device: %s\n",
                               setup.outputDeviceName.toRawUTF8());
                    }

                    // CRITICAL FIX 4: Configure channels properly for ALSA
                    setup.inputChannels = juce::BigInteger(1);   // Channel 0 only (mono)
                    setup.outputChannels = juce::BigInteger(3);  // Channels 0 and 1 (stereo dummy)
                    setup.useDefaultInputChannels = false;
                    setup.useDefaultOutputChannels = false;

                } else {
                    printf("[VoiceRecordButton] ERROR: No current device type object after setting ALSA!\n");
                }
            #else
                // Non-Linux platforms
                setup.inputChannels = 1;  // Mono input
                setup.outputChannels = 0; // No output needed
            #endif

            printf("[VoiceRecordButton] Initializing audio with:\n");
            printf("  Input device: %s\n", setup.inputDeviceName.toRawUTF8());
            printf("  Output device: %s\n", setup.outputDeviceName.toRawUTF8());
            printf("  Sample rate: %d\n", (int)setup.sampleRate);
            printf("  Buffer size: %d\n", setup.bufferSize);
            fflush(stdout);

            // Initialize with our custom setup
            juce::String errorMessage;
            auto result = deviceManager->initialise(
                1,     // maxInputChannels
                2,     // maxOutputChannels (needed for ALSA callback to run)
                nullptr,  // No saved state
                true,  // selectDefaultDevicesIfNoneSpecified
                {},    // preferredDefaultDeviceName
                &setup // Our custom setup
            );

            if (result.isNotEmpty()) {
                printf("[VoiceRecordButton] Failed to initialize audio: %s\n",
                       result.toRawUTF8());
                showMicrophoneError("Could not access microphone.\n\n"
                                  "Error: " + result + "\n\n"
                                  "Device: " + setup.inputDeviceName);
                return;
            }

            // CRITICAL FIX 5: Verify device is actually opened and running
            auto* device = deviceManager->getCurrentAudioDevice();
            if (device) {
                sampleRate = static_cast<int>(device->getCurrentSampleRate());
                printf("[VoiceRecordButton] Device opened successfully!\n");
                printf("  Device name: %s\n", device->getName().toRawUTF8());
                printf("  Actual sample rate: %d Hz\n", sampleRate);
                printf("  Actual buffer size: %d samples\n", device->getCurrentBufferSizeSamples());
                printf("  Active input channels: %s\n",
                       device->getActiveInputChannels().toString(2).toRawUTF8());
                printf("  Active output channels: %s\n",
                       device->getActiveOutputChannels().toString(2).toRawUTF8());

                // Reallocate buffer with correct sample rate
                recordingBuffer.setSize(1, sampleRate * maxRecordingSeconds);
                recordingBuffer.clear();

                // CRITICAL FIX 6: Ensure device is actually started
                if (!device->isOpen()) {
                    printf("[VoiceRecordButton] WARNING: Device not open, trying to start...\n");
                    device->start(this);  // Pass callback for audio processing
                }

                if (!device->isPlaying()) {
                    printf("[VoiceRecordButton] WARNING: Device not playing, starting stream...\n");
                    device->start(this);  // Pass callback for audio processing
                }
            } else {
                printf("[VoiceRecordButton] ERROR: No current audio device after successful init!\n");
                showMicrophoneError("Audio device initialization failed - device is null");
                return;
            }
        }

        // Clear buffer and reset recording state
        recordingBuffer.clear();
        writePosition = 0;
        recordingLevel = 0.0f;
        callbackCount = 0;  // Reset callback counter

        // Start audio callback
        deviceManager->addAudioCallback(this);
        printf("[VoiceRecordButton] Audio callback added\n");

        isRecording = true;
        startTimer(50); // Update UI at 20fps
        repaint();

        printf("[VoiceRecordButton] Recording started successfully!\n");
        fflush(stdout);

    } catch (const std::exception& e) {
        printf("[VoiceRecordButton] Exception in startRecording: %s\n", e.what());
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

    printf("[VoiceRecordButton] Stopping recording. Captured %d samples\n", writePosition);
    fflush(stdout);

    isRecording = false;
    isProcessing = true;

    // Stop audio callback
    deviceManager->removeAudioCallback(this);

    // Check if we actually recorded any audio
    if (writePosition == 0) {
        printf("[VoiceRecordButton] ERROR: No audio captured! The audio callback may not be running.\n");
        showMicrophoneError("No audio was captured. Please check:\n\n"
                          "1. USB microphone is connected\n"
                          "2. Microphone permissions are granted\n"
                          "3. Audio device is not in use by another application");
        isProcessing = false;
        stopTimer();
        repaint();
        return;
    }

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

                        printf("[VoiceRecordButton] Transcription: %s\n",
                               transcribedText.toRawUTF8());

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
                        printf("[VoiceRecordButton] Transcription failed: %s\n",
                               result["message"].toString().toRawUTF8());
                        juce::MessageManager::callAsync([this]() {
                            isProcessing = false;
                            stopTimer();
                            repaint();
                        });
                    }
                } else {
                    printf("[VoiceRecordButton] Failed to parse transcription response\n");
                    juce::MessageManager::callAsync([this]() {
                        isProcessing = false;
                        stopTimer();
                        repaint();
                    });
                }
            } else {
                printf("[VoiceRecordButton] Failed to connect to transcription server\n");
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

    // CRITICAL DIAGNOSTIC: Log first few callbacks to verify it's being called
    if (callbackCount < 10) {
        printf("[AudioCallback %d] Called: inputs=%d, outputs=%d, samples=%d, isRecording=%d\n",
               callbackCount, numInputChannels, numOutputChannels, numSamples, isRecording);

        if (numInputChannels > 0 && inputChannelData && inputChannelData[0]) {
            // Check if we're getting actual audio data
            float peak = 0.0f;
            float sum = 0.0f;
            for (int i = 0; i < juce::jmin(100, numSamples); ++i) {
                float sample = inputChannelData[0][i];
                peak = juce::jmax(peak, std::abs(sample));
                sum += std::abs(sample);
            }
            printf("[AudioCallback %d] Input data present. Peak: %f, Avg: %f\n",
                   callbackCount, peak, sum / juce::jmin(100, numSamples));
        } else {
            printf("[AudioCallback %d] WARNING: No input data! inputChannelData=%p",
                   callbackCount, (void*)inputChannelData);
            if (inputChannelData) {
                printf(", inputChannelData[0]=%p", (void*)inputChannelData[0]);
            }
            printf("\n");
        }

        callbackCount++;
        fflush(stdout);
    }

    // Process input if recording
    if (numInputChannels > 0 && isRecording) {
        // Safety check for null pointers
        if (!inputChannelData || !inputChannelData[0]) {
            printf("[AudioCallback] ERROR: Input channel data is null!\n");
            fflush(stdout);
            return;
        }

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

            // Log progress every second
            if (writePosition % sampleRate < numSamples) {
                printf("[Recording] %d seconds captured, level: %f\n",
                       writePosition / sampleRate, recordingLevel);
                fflush(stdout);
            }

            // Auto-stop if buffer is full
            if (writePosition >= recordingBuffer.getNumSamples()) {
                printf("[Recording] Buffer full, stopping...\n");
                fflush(stdout);
                juce::MessageManager::callAsync([this]() {
                    stopRecording();
                });
            }
        }
    }

    // Clear output buffers (we're not playing anything, but ALSA may require this)
    for (int i = 0; i < numOutputChannels; ++i) {
        if (outputChannelData && outputChannelData[i]) {
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