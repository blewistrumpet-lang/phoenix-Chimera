#pragma once

#include <JuceHeader.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

/**
 * FileExchangeClient - File-based preset exchange for guaranteed delivery
 * Monitors the file exchange directory for new presets from the AI server
 * Replaces HTTP polling with robust file-based transport
 */
class FileExchangeClient : public juce::Timer
{
public:
    struct ExchangeMessage {
        juce::String id;
        juce::String sessionId;
        juce::String presetName;
        juce::var presetData;
        double timestamp;
    };
    
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void onPresetReceived(const juce::var& presetData) = 0;
        virtual void onExchangeError(const juce::String& error) = 0;
    };
    
    FileExchangeClient();
    ~FileExchangeClient();
    
    // Initialize with session ID
    void initialize(const juce::String& sessionId);
    
    // Start/stop monitoring
    void startMonitoring();
    void stopMonitoring();
    
    // Check for pending presets
    bool checkForPresets();
    
    // Acknowledge preset processing
    void acknowledgePreset(const juce::String& exchangeId);
    
    // Listener management
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    // Get exchange directory path
    juce::File getExchangeDirectory() const;
    
    // Get current session ID
    juce::String getSessionId() const { return currentSessionId; }
    
    // Stats
    int getPendingCount() const;
    int getProcessedCount() const;
    
private:
    void timerCallback() override;
    void processExchangeFile(const juce::File& file);
    void notifyPresetReceived(const juce::var& presetData);
    void notifyError(const juce::String& error);
    void cleanupOldMarkers();
    
    juce::String currentSessionId;
    juce::File exchangeDir;
    juce::File pendingDir;
    juce::File processedDir;
    
    std::vector<Listener*> listeners;
    std::mutex listenersMutex;
    
    std::atomic<bool> isMonitoring{false};
    std::atomic<int> pendingCount{0};
    std::atomic<int> processedCount{0};
    
    // Track processed exchange IDs to avoid duplicates
    std::set<juce::String> processedIds;
    std::mutex processedIdsMutex;
    
    // Last check timestamp
    double lastCheckTime = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileExchangeClient)
};