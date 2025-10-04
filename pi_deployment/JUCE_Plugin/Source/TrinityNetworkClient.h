#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <queue>
#include <memory>
#include "FileExchangeClient.h"

/**
 * ITrinityTransport - Abstract transport layer for Trinity communication
 * Allows switching between HTTP, WebSocket, and File-based implementations
 */
class ITrinityTransport {
public:
    virtual ~ITrinityTransport() = default;
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual void sendMessage(const juce::String& message) = 0;
    virtual bool isConnected() const = 0;
    virtual void setEndpoint(const juce::String& endpoint) = 0;
    virtual void setApiKey(const juce::String& apiKey) = 0;
};

/**
 * HTTPTrinityTransport - HTTP-based transport for Trinity communication
 * Uses JUCE's URL class for HTTP requests with polling for responses
 */
class HTTPTrinityTransport : public ITrinityTransport {
public:
    HTTPTrinityTransport();
    ~HTTPTrinityTransport() override;
    
    void connect() override;
    void disconnect() override;
    void sendMessage(const juce::String& message) override;
    bool isConnected() const override { return connected.load(); }
    void setEndpoint(const juce::String& endpoint) override { httpEndpoint = endpoint; }
    void setApiKey(const juce::String& apiKey) override { this->apiKey = apiKey; }
    
    // Get the current HTTP session ID
    juce::String getSessionId() const { return sessionId; }
    
    // Callback for received messages
    std::function<void(const juce::String&)> onMessageReceived;
    std::function<void(const juce::String&)> onConnectionError;
    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    
    // Start polling for messages (call from background thread)
    void pollForMessages();
    
private:
    juce::String httpEndpoint;
    juce::String apiKey;
    juce::String sessionId;
    std::atomic<bool> connected{false};
    std::atomic<bool> shouldStopPolling{false};
    
    void generateSessionId();
    juce::String createAuthHeaders() const;
};

/**
 * FileExchangeTransport - File-based transport for guaranteed preset delivery
 * Uses FileExchangeClient to monitor preset exchange directory
 */
class FileExchangeTransport : public ITrinityTransport, public FileExchangeClient::Listener {
public:
    FileExchangeTransport();
    ~FileExchangeTransport() override;
    
    void connect() override;
    void disconnect() override;
    void sendMessage(const juce::String& message) override;
    bool isConnected() const override { return connected.load(); }
    void setEndpoint(const juce::String& endpoint) override { httpEndpoint = endpoint; }
    void setApiKey(const juce::String& apiKey) override { this->apiKey = apiKey; }
    
    // Get the current session ID
    juce::String getSessionId() const { return sessionId; }
    
    // Callbacks for received messages
    std::function<void(const juce::String&)> onMessageReceived;
    std::function<void(const juce::String&)> onConnectionError;
    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    
    // FileExchangeClient::Listener overrides
    void onPresetReceived(const juce::var& presetData) override;
    void onExchangeError(const juce::String& error) override;
    
private:
    std::unique_ptr<FileExchangeClient> fileExchange;
    std::unique_ptr<HTTPTrinityTransport> httpTransport; // For sending messages to server
    juce::String httpEndpoint;
    juce::String apiKey;
    juce::String sessionId;
    std::atomic<bool> connected{false};
    
    void generateSessionId();
};

/**
 * TrinityNetworkClient - Enhanced cloud-based AI network client for Trinity integration
 * Uses abstracted transport layer to support both HTTP and WebSocket communication
 * Provides real-time bidirectional communication with connection status monitoring
 */
class TrinityNetworkClient : public juce::Thread {
public:
    TrinityNetworkClient();
    ~TrinityNetworkClient() override;
    
    // === CONNECTION MANAGEMENT ===
    
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Error
    };
    
    // Connect to Trinity cloud services
    void connectToTrinity(const juce::String& apiKey = "", const juce::String& endpoint = "");
    
    // Disconnect from Trinity
    void disconnect();
    
    // Get current connection state
    ConnectionState getConnectionState() const { return connectionState.load(); }
    juce::String getConnectionStateString() const;
    
    // Check if connected and ready
    bool isConnected() const { return connectionState.load() == ConnectionState::Connected; }
    
    // === AI COMMUNICATION ===
    
    struct TrinityMessage {
        juce::String type;          // "query", "suggestion", "preset", "parameter_change"
        juce::String content;       // Main message content
        juce::var data;            // Additional structured data
        juce::String sessionId;     // Session identifier
        juce::int64 timestamp;      // Message timestamp
    };
    
    struct TrinityResponse {
        bool success = false;
        juce::String type;          // "response", "suggestion", "error"
        juce::String message;       // Human-readable response
        juce::var data;            // Structured response data
        juce::String sessionId;     // Session identifier
        int responseTimeMs = 0;
    };
    
    // Callback for async responses
    using ResponseCallback = std::function<void(const TrinityResponse&)>;
    
    // Send message to Trinity AI (async)
    void sendMessage(const TrinityMessage& message, ResponseCallback callback = nullptr);
    
    // Send text query to Trinity AI (simplified interface)
    void sendQuery(const juce::String& query, ResponseCallback callback = nullptr);
    
    // Send current plugin state for context
    void sendPluginState(const juce::var& stateData);
    
    // Send preset modification request
    void sendModification(const juce::var& preset, const juce::String& modification, ResponseCallback callback = nullptr);
    
    // Get modification suggestions for current preset
    void getSuggestions(const juce::var& preset, ResponseCallback callback = nullptr);
    
    // === SESSION MANAGEMENT ===
    
    // Start new AI session
    void startSession(const juce::String& sessionType = "sound_design");
    
    // End current session
    void endSession();
    
    // Get current session ID
    juce::String getCurrentSessionId() const { return currentSessionId; }
    
    // === CONFIGURATION ===
    
    struct TrinityConfig {
        juce::String cloudEndpoint = "wss://trinity.chimera-audio.com/ws";
        juce::String httpEndpoint = "https://trinity.chimera-audio.com/api";
        juce::String apiKey;
        int connectionTimeoutMs = 10000;
        int messageTimeoutMs = 30000;
        int maxRetries = 3;
        int retryDelayMs = 2000;
        bool enableAutoReconnect = true;
        int heartbeatIntervalMs = 30000;
    };
    
    void setConfig(const TrinityConfig& config) { trinityConfig = config; }
    TrinityConfig getConfig() const { return trinityConfig; }
    
    // === LISTENERS ===
    
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void trinityConnectionStateChanged(ConnectionState newState) {}
        virtual void trinityMessageReceived(const TrinityResponse& response) {}
        virtual void trinitySessionStarted(const juce::String& sessionId) {}
        virtual void trinitySessionEnded(const juce::String& sessionId) {}
        virtual void trinityError(const juce::String& error) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    // === TRANSPORT LAYER CALLBACKS ===
    
    void onTransportConnected();
    void onTransportDisconnected();
    void onTransportError(const juce::String& errorMessage);
    void onTransportMessageReceived(const juce::String& message);
    
private:
    // Thread execution for background processing
    void run() override;
    
    // Connection management
    void attemptConnection();
    void handleReconnection();
    void sendHeartbeat();
    bool shouldAttemptReconnection() const;
    
    // Message queue management
    struct PendingMessage {
        juce::String id;
        TrinityMessage message;
        ResponseCallback callback;
        juce::int64 timestamp;
        int retryCount = 0;
    };
    
    std::queue<PendingMessage> messageQueue;
    std::queue<PendingMessage> retryQueue;
    std::map<juce::String, ResponseCallback> pendingCallbacks;
    juce::CriticalSection queueLock;
    
    // HTTP fallback for when WebSocket is unavailable
    TrinityResponse sendHttpRequest(const TrinityMessage& message);
    
    // Message processing
    void processMessageQueue();
    void processRetryQueue();
    juce::String createMessageJson(const TrinityMessage& message);
    TrinityResponse parseResponse(const juce::String& jsonResponse);
    
    // Utilities
    juce::String generateMessageId() const;
    juce::String generateSessionId() const;
    void notifyStateChange(ConnectionState newState);
    void notifyResponse(const TrinityResponse& response);
    void notifyError(const juce::String& error);
    
    // Transport layer
    std::unique_ptr<ITrinityTransport> transport;
    
    // Configuration and state
    TrinityConfig trinityConfig;
    std::atomic<ConnectionState> connectionState{ConnectionState::Disconnected};
    juce::String currentSessionId;
    
    // Timing and retry logic
    juce::int64 lastHeartbeat = 0;
    juce::int64 lastConnectionAttempt = 0;
    int connectionRetryCount = 0;
    
    // Thread synchronization
    std::atomic<bool> shouldStop{false};
    juce::WaitableEvent connectionEvent;
    
    // Listeners
    juce::ListenerList<Listener> listeners;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrinityNetworkClient)
};