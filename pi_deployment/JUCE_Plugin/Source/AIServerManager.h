#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>

/**
 * AIServerManager - Manages the TRUE Trinity AI server lifecycle
 * Auto-starts the server when the plugin loads and keeps it running
 */
class AIServerManager : public juce::Thread {
public:
    AIServerManager();
    ~AIServerManager() override;
    
    // Singleton pattern for global access
    static AIServerManager& getInstance() {
        static AIServerManager instance;
        return instance;
    }
    
    // Server control
    bool startServerIfNeeded();
    void stopServer();
    bool isServerRunning() const { return serverRunning; }
    bool isServerHealthy() const { return serverHealthy; }
    
    // Server URL
    juce::String getServerUrl() const { return "http://localhost:8000"; }
    
    // Listener interface
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void serverStatusChanged(bool running, bool healthy) {}
    };
    
    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }
    
private:
    void run() override;  // Thread work
    bool checkServerHealth();
    bool attemptServerStart();
    void killExistingServers();
    
    std::unique_ptr<juce::ChildProcess> serverProcess;
    std::atomic<bool> serverRunning{false};
    std::atomic<bool> serverHealthy{false};
    std::atomic<bool> shouldCheckHealth{true};
    
    juce::ListenerList<Listener> listeners;
    juce::CriticalSection processLock;
    
    // Prevent copying
    AIServerManager(const AIServerManager&) = delete;
    AIServerManager& operator=(const AIServerManager&) = delete;
};