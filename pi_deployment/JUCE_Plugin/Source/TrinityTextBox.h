#pragma once

#include <JuceHeader.h>
#include "TrinityNetworkClient.h"

/**
 * TrinityTextBox - Advanced text input component with dynamic glow effects
 * Provides visual feedback for Trinity AI connection status and interaction
 * Features:
 * - Dynamic glow effect based on connection state
 * - Animated pulsing for activity indication
 * - Glass-morphism aesthetic matching the Nexus theme
 * - Real-time response display
 */
class TrinityTextBox : public juce::Component,
                       public juce::TextEditor::Listener,
                       public juce::Timer,
                       public TrinityNetworkClient::Listener {
public:
    TrinityTextBox();
    ~TrinityTextBox() override;
    
    // === COMPONENT INTERFACE ===
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    
    // === TRINITY INTEGRATION ===
    
    // Set the Trinity network client
    void setTrinityClient(TrinityNetworkClient* client);
    
    // Send current text as query to Trinity
    void sendQuery();
    
    // Get access to the internal text editor
    juce::TextEditor* getTextEditor() { return inputEditor.get(); }
    
    // === TEXT EDITOR LISTENER ===
    
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;
    
    // === TIMER FOR ANIMATIONS ===
    
    void timerCallback() override;
    
    // === TRINITY CLIENT LISTENER ===
    
    void trinityConnectionStateChanged(TrinityNetworkClient::ConnectionState newState) override;
    void trinityMessageReceived(const TrinityNetworkClient::TrinityResponse& response) override;
    void trinityError(const juce::String& error) override;
    
    // === CONFIGURATION ===
    
    struct GlowSettings {
        float baseGlowRadius = 8.0f;
        float maxGlowRadius = 16.0f;
        float pulseSpeed = 2.0f;          // Pulses per second
        float fadeSpeed = 5.0f;           // Alpha change per second
        bool enablePulsing = true;
        bool enableFadeAnimation = true;
    };
    
    void setGlowSettings(const GlowSettings& settings) { glowSettings = settings; }
    GlowSettings getGlowSettings() const { return glowSettings; }
    
    // === VISUAL STATES ===
    
    enum class VisualState {
        Disconnected,    // Red glow, dim
        Connecting,      // Yellow glow, pulsing
        Connected,       // Green glow, steady
        Thinking,        // Blue glow, fast pulse
        Responding,      // Cyan glow, breathing
        Error           // Red glow, fast flash
    };
    
    void setVisualState(VisualState state);
    VisualState getVisualState() const { return currentVisualState; }
    
    // === RESPONSE DISPLAY ===
    
    void showResponse(const juce::String& response, bool isError = false);
    void clearResponse();
    
    // === PRESET MODIFICATION ===
    
    void sendModification();
    void setCurrentPreset(const juce::var& preset) { currentPreset = preset; }
    void showModificationSuggestions();
    
    // Callback for when a modified preset should be applied
    std::function<void(const juce::var&)> onPresetModified;
    
    // Callback for when a new preset is received from a query
    std::function<void(const juce::var&)> onPresetReceived;
    
private:
    // UI Components
    std::unique_ptr<juce::TextEditor> inputEditor;
    std::unique_ptr<juce::Label> statusLabel;
    std::unique_ptr<juce::Label> responseLabel;
    std::unique_ptr<juce::TextButton> sendButton;
    std::unique_ptr<juce::TextButton> alterButton;
    
    // Trinity integration
    TrinityNetworkClient* trinityClient = nullptr;
    
    // Current preset for modifications
    juce::var currentPreset;
    bool modificationMode = false;
    
    // Visual state and animation
    VisualState currentVisualState = VisualState::Disconnected;
    GlowSettings glowSettings;
    
    // Animation variables
    float currentGlowRadius = 0.0f;
    float targetGlowRadius = 0.0f;
    float currentGlowAlpha = 0.0f;
    float targetGlowAlpha = 0.0f;
    float pulsePhase = 0.0f;
    float hoverAlpha = 0.0f;
    
    // State colors
    juce::Colour getStateColor() const;
    juce::Colour getStateSecondaryColor() const;
    
    // Animation helpers
    void updateAnimations(float deltaTime);
    void startStateTransition(VisualState newState);
    
    // Drawing methods
    void drawGlowEffect(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawStatusIndicator(juce::Graphics& g);
    
    // Input handling
    void handleSendButton();
    void handleAlterButton();
    void updateSendButtonState();
    void updateAlterButtonState();
    
    // Response management
    void displayThinkingAnimation();
    void stopThinkingAnimation();
    juce::String formatResponse(const juce::String& rawResponse);
    
    // Timing
    juce::int64 lastUpdateTime = 0;
    juce::int64 lastActivityTime = 0;
    
    // Constants
    static constexpr float GLOW_ANIMATION_FPS = 60.0f;
    static constexpr int THINKING_TIMEOUT_MS = 30000;
    static constexpr int RESPONSE_DISPLAY_TIME_MS = 10000;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrinityTextBox)
};