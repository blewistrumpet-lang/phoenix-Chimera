#include "TrinityTextBox.h"

TrinityTextBox::TrinityTextBox() {
    // Create input text editor
    inputEditor = std::make_unique<juce::TextEditor>("TrinityInput");
    inputEditor->setMultiLine(false);
    inputEditor->setReturnKeyStartsNewLine(false);
    inputEditor->setTextToShowWhenEmpty("Ask Trinity AI for sound design help...", juce::Colours::grey);
    inputEditor->setFont(juce::Font("Roboto", 14.0f, juce::Font::plain));
    inputEditor->addListener(this);
    addAndMakeVisible(inputEditor.get());
    
    // Create status label
    statusLabel = std::make_unique<juce::Label>("TrinityStatus", "Disconnected");
    statusLabel->setFont(juce::Font("Roboto", 12.0f, juce::Font::plain));
    statusLabel->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel.get());
    
    // Create response label
    responseLabel = std::make_unique<juce::Label>("TrinityResponse", "");
    responseLabel->setFont(juce::Font("Roboto", 13.0f, juce::Font::plain));
    responseLabel->setJustificationType(juce::Justification::topLeft);
    responseLabel->setColour(juce::Label::textColourId, juce::Colours::lightcyan);
    responseLabel->setVisible(false);
    addAndMakeVisible(responseLabel.get());
    
    // Create send button
    sendButton = std::make_unique<juce::TextButton>("Send");
    sendButton->setButtonText("Send");
    sendButton->onClick = [this] { handleSendButton(); };
    sendButton->setEnabled(false);
    addAndMakeVisible(sendButton.get());
    
    // Create alter button for preset modification
    alterButton = std::make_unique<juce::TextButton>("Alter");
    alterButton->setButtonText("Alter");
    alterButton->onClick = [this] { handleAlterButton(); };
    alterButton->setEnabled(false);
    alterButton->setTooltip("Modify the current preset with natural language");
    addAndMakeVisible(alterButton.get());
    
    // Initialize animation state
    setVisualState(VisualState::Disconnected);
    lastUpdateTime = juce::Time::currentTimeMillis();
    
    // Start animation timer
    startTimer(1000.0f / GLOW_ANIMATION_FPS); // 60 FPS for smooth animations
}

TrinityTextBox::~TrinityTextBox() {
    stopTimer();
    
    if (trinityClient) {
        trinityClient->removeListener(this);
    }
}

// === COMPONENT INTERFACE ===

void TrinityTextBox::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Draw background with glass effect
    drawBackground(g, bounds);
    
    // Draw glow effect
    drawGlowEffect(g, bounds);
    
    // Draw status indicator
    drawStatusIndicator(g);
}

void TrinityTextBox::resized() {
    auto bounds = getLocalBounds();
    
    // Layout components with margins
    auto workingArea = bounds.reduced(12, 8);
    
    // Status label at top
    auto statusArea = workingArea.removeFromTop(20);
    statusLabel->setBounds(statusArea);
    
    workingArea.removeFromTop(4); // Spacing
    
    // Input area with send and alter buttons
    auto inputArea = workingArea.removeFromTop(32);
    auto buttonArea = inputArea.removeFromRight(120); // Space for 2 buttons
    inputArea.removeFromRight(4); // Spacing between input and buttons
    
    auto sendButtonArea = buttonArea.removeFromLeft(60);
    buttonArea.removeFromLeft(4); // Spacing between buttons
    auto alterButtonArea = buttonArea;
    
    inputEditor->setBounds(inputArea);
    sendButton->setBounds(sendButtonArea);
    alterButton->setBounds(alterButtonArea);
    
    // Response area (if visible)
    if (responseLabel->isVisible()) {
        workingArea.removeFromTop(4); // Spacing
        auto responseArea = workingArea.removeFromTop(60);
        responseLabel->setBounds(responseArea);
    }
}

void TrinityTextBox::mouseEnter(const juce::MouseEvent& event) {
    hoverAlpha = 0.3f; // Increase glow on hover
    repaint();
}

void TrinityTextBox::mouseExit(const juce::MouseEvent& event) {
    hoverAlpha = 0.0f;
    repaint();
}

// === TRINITY INTEGRATION ===

void TrinityTextBox::setTrinityClient(TrinityNetworkClient* client) {
    if (trinityClient) {
        trinityClient->removeListener(this);
    }
    
    trinityClient = client;
    
    if (trinityClient) {
        trinityClient->addListener(this);
        
        // Update visual state based on current connection
        auto state = trinityClient->getConnectionState();
        switch (state) {
            case TrinityNetworkClient::ConnectionState::Disconnected:
                setVisualState(VisualState::Disconnected);
                break;
            case TrinityNetworkClient::ConnectionState::Connecting:
                setVisualState(VisualState::Connecting);
                break;
            case TrinityNetworkClient::ConnectionState::Connected:
                setVisualState(VisualState::Connected);
                break;
            case TrinityNetworkClient::ConnectionState::Reconnecting:
                setVisualState(VisualState::Connecting);
                break;
            case TrinityNetworkClient::ConnectionState::Error:
                setVisualState(VisualState::Error);
                break;
        }
    }
    
    updateSendButtonState();
    updateAlterButtonState();
}

void TrinityTextBox::sendQuery() {
    if (!trinityClient || !trinityClient->isConnected()) {
        showResponse("Not connected to Trinity AI", true);
        return;
    }
    
    juce::String query = inputEditor->getText().trim();
    if (query.isEmpty()) {
        return;
    }
    
    // Show thinking state
    setVisualState(VisualState::Thinking);
    clearResponse();
    
    // Send query to Trinity
    trinityClient->sendQuery(query, [this](const TrinityNetworkClient::TrinityResponse& response) {
        juce::MessageManager::callAsync([this, response]() {
            if (response.success) {
                setVisualState(VisualState::Responding);
                showResponse(response.message);
                
                // Return to connected state after showing response
                juce::Timer::callAfterDelay(RESPONSE_DISPLAY_TIME_MS, [this]() {
                    if (trinityClient && trinityClient->isConnected()) {
                        setVisualState(VisualState::Connected);
                    }
                });
            } else {
                setVisualState(VisualState::Error);
                showResponse("Error: " + response.message, true);
                
                // Return to previous state after error
                juce::Timer::callAfterDelay(3000, [this]() {
                    if (trinityClient && trinityClient->isConnected()) {
                        setVisualState(VisualState::Connected);
                    } else {
                        setVisualState(VisualState::Disconnected);
                    }
                });
            }
        });
    });
    
    // Clear input after sending
    inputEditor->clear();
    updateSendButtonState();
    updateAlterButtonState();
}

// === TEXT EDITOR LISTENER ===

void TrinityTextBox::textEditorReturnKeyPressed(juce::TextEditor& editor) {
    if (&editor == inputEditor.get()) {
        sendQuery();
    }
}

void TrinityTextBox::textEditorTextChanged(juce::TextEditor& editor) {
    if (&editor == inputEditor.get()) {
        updateSendButtonState();
        updateAlterButtonState();
        lastActivityTime = juce::Time::currentTimeMillis();
    }
}

void TrinityTextBox::textEditorFocusLost(juce::TextEditor& editor) {
    // Optional: Could save draft or perform other actions
}

// === TIMER FOR ANIMATIONS ===

void TrinityTextBox::timerCallback() {
    juce::int64 currentTime = juce::Time::currentTimeMillis();
    float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
    lastUpdateTime = currentTime;
    
    updateAnimations(deltaTime);
    repaint();
}

// === TRINITY CLIENT LISTENER ===

void TrinityTextBox::trinityConnectionStateChanged(TrinityNetworkClient::ConnectionState newState) {
    juce::MessageManager::callAsync([this, newState]() {
        juce::String stateText;
        VisualState visualState;
        
        switch (newState) {
            case TrinityNetworkClient::ConnectionState::Disconnected:
                stateText = "Disconnected";
                visualState = VisualState::Disconnected;
                break;
            case TrinityNetworkClient::ConnectionState::Connecting:
                stateText = "Connecting...";
                visualState = VisualState::Connecting;
                break;
            case TrinityNetworkClient::ConnectionState::Connected:
                stateText = "Connected to Trinity AI";
                visualState = VisualState::Connected;
                break;
            case TrinityNetworkClient::ConnectionState::Reconnecting:
                stateText = "Reconnecting...";
                visualState = VisualState::Connecting;
                break;
            case TrinityNetworkClient::ConnectionState::Error:
                stateText = "Connection Error";
                visualState = VisualState::Error;
                break;
        }
        
        statusLabel->setText(stateText, juce::dontSendNotification);
        setVisualState(visualState);
        updateSendButtonState();
        updateAlterButtonState();
    });
}

void TrinityTextBox::trinityMessageReceived(const TrinityNetworkClient::TrinityResponse& response) {
    // This is handled in the sendQuery callback
}

void TrinityTextBox::trinityError(const juce::String& error) {
    juce::MessageManager::callAsync([this, error]() {
        setVisualState(VisualState::Error);
        showResponse("Trinity Error: " + error, true);
    });
}

// === VISUAL STATES ===

void TrinityTextBox::setVisualState(VisualState state) {
    if (currentVisualState == state) return;
    
    startStateTransition(state);
    currentVisualState = state;
}

juce::Colour TrinityTextBox::getStateColor() const {
    switch (currentVisualState) {
        case VisualState::Disconnected:
            return juce::Colours::red;
        case VisualState::Connecting:
            return juce::Colours::yellow;
        case VisualState::Connected:
            return juce::Colours::green;
        case VisualState::Thinking:
            return juce::Colours::blue;
        case VisualState::Responding:
            return juce::Colours::cyan;
        case VisualState::Error:
            return juce::Colours::orange;
        default:
            return juce::Colours::grey;
    }
}

juce::Colour TrinityTextBox::getStateSecondaryColor() const {
    return getStateColor().withAlpha(0.3f);
}

// === RESPONSE DISPLAY ===

void TrinityTextBox::showResponse(const juce::String& response, bool isError) {
    juce::String formattedResponse = formatResponse(response);
    responseLabel->setText(formattedResponse, juce::dontSendNotification);
    responseLabel->setColour(juce::Label::textColourId, 
                           isError ? juce::Colours::lightcoral : juce::Colours::lightcyan);
    responseLabel->setVisible(true);
    resized(); // Relayout to accommodate response
}

void TrinityTextBox::clearResponse() {
    responseLabel->setVisible(false);
    resized();
}

// === PRIVATE IMPLEMENTATION ===

void TrinityTextBox::updateAnimations(float deltaTime) {
    // Update pulse phase
    if (glowSettings.enablePulsing) {
        pulsePhase += deltaTime * glowSettings.pulseSpeed * juce::MathConstants<float>::twoPi;
        if (pulsePhase > juce::MathConstants<float>::twoPi) {
            pulsePhase -= juce::MathConstants<float>::twoPi;
        }
    }
    
    // Calculate target values based on state
    float targetRadius, targetAlpha;
    
    switch (currentVisualState) {
        case VisualState::Disconnected:
            targetRadius = glowSettings.baseGlowRadius * 0.5f;
            targetAlpha = 0.2f;
            break;
        case VisualState::Connecting:
            targetRadius = glowSettings.baseGlowRadius + 
                          std::sin(pulsePhase) * (glowSettings.maxGlowRadius - glowSettings.baseGlowRadius) * 0.5f;
            targetAlpha = 0.6f + std::sin(pulsePhase) * 0.3f;
            break;
        case VisualState::Connected:
            targetRadius = glowSettings.baseGlowRadius;
            targetAlpha = 0.8f;
            break;
        case VisualState::Thinking:
            targetRadius = glowSettings.baseGlowRadius + 
                          std::sin(pulsePhase * 2.0f) * (glowSettings.maxGlowRadius - glowSettings.baseGlowRadius) * 0.3f;
            targetAlpha = 0.9f + std::sin(pulsePhase * 2.0f) * 0.1f;
            break;
        case VisualState::Responding:
            targetRadius = glowSettings.baseGlowRadius + 
                          std::sin(pulsePhase * 0.5f) * (glowSettings.maxGlowRadius - glowSettings.baseGlowRadius) * 0.4f;
            targetAlpha = 0.7f + std::sin(pulsePhase * 0.5f) * 0.2f;
            break;
        case VisualState::Error:
            targetRadius = glowSettings.baseGlowRadius + 
                          std::sin(pulsePhase * 4.0f) * (glowSettings.maxGlowRadius - glowSettings.baseGlowRadius) * 0.2f;
            targetAlpha = 0.8f + std::sin(pulsePhase * 4.0f) * 0.2f;
            break;
    }
    
    // Add hover effect
    targetAlpha += hoverAlpha;
    targetRadius += hoverAlpha * 4.0f;
    
    // Smooth interpolation to target values
    float speed = glowSettings.fadeSpeed * deltaTime;
    currentGlowRadius += (targetRadius - currentGlowRadius) * speed;
    currentGlowAlpha += (targetAlpha - currentGlowAlpha) * speed;
    
    // Clamp values
    currentGlowRadius = juce::jlimit(0.0f, glowSettings.maxGlowRadius * 2.0f, currentGlowRadius);
    currentGlowAlpha = juce::jlimit(0.0f, 1.0f, currentGlowAlpha);
}

void TrinityTextBox::startStateTransition(VisualState newState) {
    targetGlowRadius = glowSettings.baseGlowRadius;
    targetGlowAlpha = 0.8f;
    pulsePhase = 0.0f; // Reset pulse for smooth transitions
}

void TrinityTextBox::drawGlowEffect(juce::Graphics& g, const juce::Rectangle<float>& bounds) {
    if (currentGlowAlpha <= 0.0f || currentGlowRadius <= 0.0f) return;
    
    auto glowBounds = bounds.expanded(currentGlowRadius);
    auto color = getStateColor().withAlpha(currentGlowAlpha);
    
    // Create radial gradient for glow
    juce::ColourGradient gradient(
        color,
        bounds.getCentreX(), bounds.getCentreY(),
        juce::Colours::transparentBlack,
        glowBounds.getCentreX(), glowBounds.getY(),
        true
    );
    
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(glowBounds, 8.0f);
}

void TrinityTextBox::drawBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds) {
    // Glass morphism background
    auto bgColor = juce::Colour(0xff1a1a2e).withAlpha(0.9f);
    auto borderColor = getStateColor().withAlpha(0.5f);
    
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds, 8.0f);
    
    g.setColour(borderColor);
    g.drawRoundedRectangle(bounds, 8.0f, 1.5f);
    
    // Subtle inner highlight
    auto highlightColor = juce::Colours::white.withAlpha(0.1f);
    g.setColour(highlightColor);
    g.drawRoundedRectangle(bounds.reduced(1.0f), 7.0f, 1.0f);
}

void TrinityTextBox::drawStatusIndicator(juce::Graphics& g) {
    auto statusBounds = getLocalBounds().toFloat();
    auto indicatorBounds = juce::Rectangle<float>(statusBounds.getRight() - 20, statusBounds.getY() + 6, 8, 8);
    
    auto color = getStateColor().withAlpha(currentGlowAlpha);
    g.setColour(color);
    g.fillEllipse(indicatorBounds);
    
    // Add inner highlight
    g.setColour(juce::Colours::white.withAlpha(currentGlowAlpha * 0.6f));
    g.fillEllipse(indicatorBounds.reduced(2));
}

void TrinityTextBox::handleSendButton() {
    sendQuery();
}

void TrinityTextBox::handleAlterButton() {
    sendModification();
}

void TrinityTextBox::updateSendButtonState() {
    bool canSend = trinityClient && 
                   trinityClient->isConnected() && 
                   !inputEditor->getText().trim().isEmpty();
    
    sendButton->setEnabled(canSend);
    
    // Update button color based on state
    if (canSend) {
        sendButton->setColour(juce::TextButton::buttonColourId, getStateColor().withAlpha(0.7f));
    } else {
        sendButton->setColour(juce::TextButton::buttonColourId, juce::Colours::grey.withAlpha(0.3f));
    }
}

void TrinityTextBox::updateAlterButtonState() {
    bool canAlter = trinityClient && 
                    trinityClient->isConnected() && 
                    !inputEditor->getText().trim().isEmpty() &&
                    currentPreset.isObject();
    
    alterButton->setEnabled(canAlter);
    
    // Update button color based on state
    if (canAlter) {
        alterButton->setColour(juce::TextButton::buttonColourId, 
                              juce::Colours::purple.withAlpha(0.7f));
    } else {
        alterButton->setColour(juce::TextButton::buttonColourId, 
                              juce::Colours::grey.withAlpha(0.3f));
    }
}

juce::String TrinityTextBox::formatResponse(const juce::String& rawResponse) {
    // Simple formatting - could be enhanced with markdown or rich text
    return rawResponse.substring(0, 200) + (rawResponse.length() > 200 ? "..." : "");
}

// === PRESET MODIFICATION ===

void TrinityTextBox::sendModification() {
    if (!trinityClient || !trinityClient->isConnected()) {
        showResponse("Not connected to Trinity AI", true);
        return;
    }
    
    if (!currentPreset.isObject()) {
        showResponse("No preset loaded to modify", true);
        return;
    }
    
    juce::String modification = inputEditor->getText().trim();
    if (modification.isEmpty()) {
        return;
    }
    
    // Show thinking state
    setVisualState(VisualState::Thinking);
    clearResponse();
    
    // Send modification request to Trinity
    trinityClient->sendModification(currentPreset, modification, 
        [this](const TrinityNetworkClient::TrinityResponse& response) {
            juce::MessageManager::callAsync([this, response]() {
                if (response.success) {
                    setVisualState(VisualState::Responding);
                    
                    // Show what was modified
                    juce::String message = response.message;
                    if (message.isEmpty()) {
                        message = "Preset modified successfully";
                    }
                    showResponse("✨ " + message);
                    
                    // Update the current preset if data is provided
                    if (response.data.isObject()) {
                        currentPreset = response.data;
                        // Notify parent editor to apply the modified preset
                        if (onPresetModified) {
                            onPresetModified(response.data);
                        }
                    }
                    
                    // Return to connected state after showing response
                    juce::Timer::callAfterDelay(RESPONSE_DISPLAY_TIME_MS, [this]() {
                        if (trinityClient && trinityClient->isConnected()) {
                            setVisualState(VisualState::Connected);
                        }
                    });
                } else {
                    setVisualState(VisualState::Error);
                    showResponse("Error: " + response.message, true);
                    
                    // Return to previous state after error
                    juce::Timer::callAfterDelay(3000, [this]() {
                        if (trinityClient && trinityClient->isConnected()) {
                            setVisualState(VisualState::Connected);
                        } else {
                            setVisualState(VisualState::Disconnected);
                        }
                    });
                }
            });
        });
    
    // Clear input after sending
    inputEditor->clear();
    updateSendButtonState();
    updateAlterButtonState();
}

void TrinityTextBox::showModificationSuggestions() {
    if (!trinityClient || !trinityClient->isConnected()) {
        return;
    }
    
    if (!currentPreset.isObject()) {
        return;
    }
    
    // Request suggestions from the server
    trinityClient->getSuggestions(currentPreset, 
        [this](const TrinityNetworkClient::TrinityResponse& response) {
            juce::MessageManager::callAsync([this, response]() {
                if (response.success && response.data.hasProperty("suggestions")) {
                    auto suggestions = response.data["suggestions"];
                    if (suggestions.isArray() && suggestions.size() > 0) {
                        // Display suggestions as placeholder text
                        juce::String hint = "Try: ";
                        for (int i = 0; i < juce::jmin(3, suggestions.size()); ++i) {
                            if (i > 0) hint += ", ";
                            hint += suggestions[i].toString();
                        }
                        inputEditor->setTextToShowWhenEmpty(hint, juce::Colours::grey);
                    }
                }
            });
        });
}