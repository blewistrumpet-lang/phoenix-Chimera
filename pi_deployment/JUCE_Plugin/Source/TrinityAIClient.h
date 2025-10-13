#pragma once
#include <JuceHeader.h>

/**
 * TrinityAIClient - HTTP client for voice-to-preset generation
 *
 * Connects to Trinity AI server at localhost:8000
 * Endpoints:
 * - POST /generate - Generate preset from voice prompt
 * - GET /health - Check server health
 * - GET /progress/{request_id} - Check generation progress
 */
class TrinityAIClient
{
public:
    TrinityAIClient();
    ~TrinityAIClient() = default;

    // Health check
    enum class HealthStatus {
        Healthy,
        Degraded,
        Offline
    };

    struct HealthResponse {
        HealthStatus status;
        juce::String message;
    };

    HealthResponse checkHealth();

    // Voice-to-preset generation
    struct GenerateRequest {
        juce::AudioBuffer<float> audioBuffer;
        juce::String textPrompt;  // Optional text override
        juce::String requestId;
    };

    struct GenerateResponse {
        bool success;
        juce::String requestId;
        juce::String errorMessage;
        juce::var presetData;  // JSON preset if successful
    };

    void generatePresetAsync(const GenerateRequest& request,
                            std::function<void(const GenerateResponse&)> callback);

    // Progress tracking
    struct ProgressResponse {
        juce::String status;  // "pending", "processing", "completed", "failed"
        float progress;       // 0.0 - 1.0
        juce::String message;
        juce::var presetData; // Available when status == "completed"
    };

    ProgressResponse checkProgress(const juce::String& requestId);

private:
    juce::String serverUrl = "http://localhost:8000";

    // Helper methods
    juce::var makeRequest(const juce::String& endpoint,
                         const juce::String& method,
                         const juce::var& payload = juce::var());

    juce::String encodeAudioToBase64(const juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrinityAIClient)
};
