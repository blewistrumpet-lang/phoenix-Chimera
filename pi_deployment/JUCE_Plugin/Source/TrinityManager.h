#pragma once

#include <JuceHeader.h>
#include "TrinityNetworkClient.h"
#include "TrinityProtocol.h"

// Forward declarations
class ChimeraAudioProcessor;

/**
 * TrinityManager - Central coordinator for Trinity AI integration
 * Manages the lifecycle of Trinity AI components and handles high-level AI interactions
 * Provides a simplified interface for plugin components to interact with Trinity
 */
class TrinityManager : public TrinityNetworkClient::Listener {
public:
    TrinityManager(ChimeraAudioProcessor& processor);
    ~TrinityManager() override;
    
    // === LIFECYCLE MANAGEMENT ===
    
    // Initialize Trinity AI system
    void initialize();
    
    // Shutdown Trinity AI system
    void shutdown();
    
    // Check if Trinity is available and connected
    bool isAvailable() const;
    
    // Get connection status for UI display
    juce::String getConnectionStatus() const;
    
    // === AI INTERACTION ===
    
    // Send a user query to Trinity AI
    void sendQuery(const juce::String& query, std::function<void(const juce::String&, bool)> callback = nullptr);
    
    // Request AI suggestions for current plugin state
    void requestSuggestions(const juce::String& context = "");
    
    // Request a preset based on description
    void requestPreset(const juce::String& description, const juce::String& genre = "", const juce::String& mood = "");
    
    // Send current plugin state for AI context
    void updatePluginContext();
    
    // === CONFIGURATION ===
    
    struct TrinitySettings {
        bool autoConnect = true;
        bool sendParameterChanges = true;
        bool enableAutoSuggestions = false;
        int suggestionIntervalSeconds = 30;
        juce::String apiKey;
        juce::String cloudEndpoint = "wss://trinity.chimera-audio.com/ws";
        juce::String httpEndpoint = "https://trinity.chimera-audio.com/api";
    };
    
    void setSettings(const TrinitySettings& settings);
    TrinitySettings getSettings() const { return settings; }
    
    // === LISTENERS ===
    
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void trinityStatusChanged(bool connected) {}
        virtual void trinityResponseReceived(const juce::String& response, bool isError) {}
        virtual void trinityParameterSuggestion(int slotIndex, const juce::String& paramName, float value) {}
        virtual void trinityPresetReceived(const juce::var& presetData) {}
        virtual void trinityError(const juce::String& error) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    // === CLIENT ACCESS ===
    
    TrinityNetworkClient* getClient() { return trinityClient.get(); }
    
    // === PLUGIN STATE HELPERS ===
    
    // Get formatted plugin state for AI context
    juce::var getCurrentPluginState();
    
    // Apply AI suggestions to plugin
    void applySuggestions(const juce::Array<juce::var>& suggestions);
    
    // Apply AI preset to plugin
    void applyPreset(const juce::var& presetData);
    
private:
    // === TRINITY CLIENT LISTENER IMPLEMENTATION ===
    
    void trinityConnectionStateChanged(TrinityNetworkClient::ConnectionState newState) override;
    void trinityMessageReceived(const TrinityNetworkClient::TrinityResponse& response) override;
    void trinitySessionStarted(const juce::String& sessionId) override;
    void trinitySessionEnded(const juce::String& sessionId) override;
    void trinityError(const juce::String& error) override;
    
    // === PRIVATE METHODS ===
    
    // Configure Trinity client with current settings
    void configureTrinityClient();
    
    // Handle different types of AI responses
    void handleQueryResponse(const TrinityNetworkClient::TrinityResponse& response);
    void handleSuggestionResponse(const TrinityNetworkClient::TrinityResponse& response);
    void handlePresetResponse(const TrinityNetworkClient::TrinityResponse& response);
    
    // Automatic suggestion system
    void startAutoSuggestions();
    void stopAutoSuggestions();
    void requestAutoSuggestions();
    
    // State management
    void notifyListeners(std::function<void(Listener*)> callback);
    
    // Reference to audio processor
    ChimeraAudioProcessor& audioProcessor;
    
    // Trinity components
    std::unique_ptr<TrinityNetworkClient> trinityClient;
    
    // Configuration
    TrinitySettings settings;
    
    // State
    std::atomic<bool> isInitialized{false};
    std::atomic<bool> isConnected{false};
    juce::String currentSessionId;
    
    // Auto-suggestions
    std::unique_ptr<juce::Timer> autoSuggestionTimer;
    
    // Listeners
    juce::ListenerList<Listener> listeners;
    
    // Callback storage for async operations
    struct PendingCallback {
        juce::String id;
        std::function<void(const juce::String&, bool)> callback;
        juce::int64 timestamp;
    };
    std::map<juce::String, PendingCallback> pendingCallbacks;
    juce::CriticalSection callbackLock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrinityManager)
};