#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>

/**
 * AIServerClient - HTTP client for communicating with the AI Trinity Pipeline server
 * Handles async communication with the FastAPI server for preset generation
 */
class AIServerClient : public juce::Thread {
public:
    AIServerClient();
    ~AIServerClient() override;
    
    // === CONNECTION MANAGEMENT ===
    
    // Check if server is running and responsive
    bool isServerAvailable();
    
    // Get server health status
    struct ServerHealth {
        bool isHealthy = false;
        String status;
        String version;
        int responseTimeMs = 0;
    };
    ServerHealth checkServerHealth();
    
    // === PRESET GENERATION ===
    
    // Preset request structure
    struct PresetRequest {
        String prompt;
        StringPairArray context;  // Additional context key-value pairs
        int timeoutMs = 30000;    // 30 second default timeout
    };
    
    // Preset response structure
    struct PresetResponse {
        bool success = false;
        String message;
        var presetData;  // JSON preset data
        int responseTimeMs = 0;
    };
    
    // Callback for async preset generation
    using PresetCallback = std::function<void(const PresetResponse&)>;
    
    // Generate preset asynchronously
    void generatePreset(const PresetRequest& request, PresetCallback callback);
    
    // Generate preset synchronously (blocks until response)
    PresetResponse generatePresetSync(const PresetRequest& request);
    
    // === CONFIGURATION ===
    
    // Set server URL (default: http://localhost:8000)
    void setServerUrl(const String& url);
    String getServerUrl() const { return serverUrl; }
    
    // Set retry policy
    void setMaxRetries(int retries) { maxRetries = retries; }
    void setRetryDelayMs(int delayMs) { retryDelayMs = delayMs; }
    
    // === SERVER PROCESS MANAGEMENT ===
    
    // Start the AI server process
    bool startServer();
    
    // Stop the server process
    void stopServer();
    
    // Check if server process is running
    bool isServerProcessRunning() const;
    
    // === LISTENERS ===
    
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void serverConnected() {}
        virtual void serverDisconnected() {}
        virtual void requestStarted(const String& requestId) {}
        virtual void requestCompleted(const String& requestId, bool success) {}
        virtual void requestProgress(const String& requestId, float progress) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // Thread execution
    void run() override;
    
    // Request queue
    struct PendingRequest {
        String id;
        PresetRequest request;
        PresetCallback callback;
        int64 startTime;
        int retryCount = 0;
    };
    
    std::queue<PendingRequest> requestQueue;
    CriticalSection queueLock;
    
    // HTTP helpers
    PresetResponse sendHttpRequest(const PresetRequest& request);
    var parseJsonResponse(const String& jsonString);
    String createJsonRequest(const PresetRequest& request);
    
    // Retry logic
    bool shouldRetry(int httpStatusCode) const;
    void retryRequest(PendingRequest& pending);
    
    // Server process
    std::unique_ptr<ChildProcess> serverProcess;
    void monitorServerProcess();
    
    // Configuration
    String serverUrl = "http://localhost:8000";
    int maxRetries = 3;
    int retryDelayMs = 1000;
    
    // State
    std::atomic<bool> serverAvailable{false};
    std::atomic<bool> shouldStop{false};
    
    // Listeners
    ListenerList<Listener> listeners;
    
    // Utilities
    String generateRequestId() const;
    void notifyRequestStarted(const String& requestId);
    void notifyRequestCompleted(const String& requestId, bool success);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIServerClient)
};