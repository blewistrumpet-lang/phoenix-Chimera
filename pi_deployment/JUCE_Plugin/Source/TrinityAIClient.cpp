#include "TrinityAIClient.h"

TrinityAIClient::TrinityAIClient()
{
}

TrinityAIClient::HealthResponse TrinityAIClient::checkHealth()
{
    HealthResponse response;

    try
    {
        auto result = makeRequest("/health", "GET");

        if (result.isObject() && result.hasProperty("status"))
        {
            juce::String status = result["status"].toString();

            if (status == "healthy")
            {
                response.status = HealthStatus::Healthy;
                response.message = "Trinity AI server is healthy";
            }
            else if (status == "degraded")
            {
                response.status = HealthStatus::Degraded;
                response.message = result.getProperty("message", "Server is degraded").toString();
            }
            else
            {
                response.status = HealthStatus::Offline;
                response.message = "Unknown status: " + status;
            }
        }
        else
        {
            response.status = HealthStatus::Offline;
            response.message = "Invalid health response";
        }
    }
    catch (...)
    {
        response.status = HealthStatus::Offline;
        response.message = "Connection failed";
    }

    return response;
}

void TrinityAIClient::generatePresetAsync(const GenerateRequest& request,
                                         std::function<void(const GenerateResponse&)> callback)
{
    // Create JSON payload
    juce::var payload = new juce::DynamicObject();
    auto* obj = payload.getDynamicObject();

    if (request.textPrompt.isNotEmpty())
    {
        obj->setProperty("prompt", request.textPrompt);
    }

    if (request.audioBuffer.getNumSamples() > 0)
    {
        obj->setProperty("audio_data", encodeAudioToBase64(request.audioBuffer));
        obj->setProperty("sample_rate", 44100);
    }

    obj->setProperty("request_id", request.requestId);

    // Launch async request
    juce::Thread::launch([this, payload, callback, requestId = request.requestId]() {
        GenerateResponse response;
        response.requestId = requestId;

        try
        {
            auto result = makeRequest("/generate", "POST", payload);

            if (result.isObject())
            {
                if (result.hasProperty("error"))
                {
                    response.success = false;
                    response.errorMessage = result["error"].toString();
                }
                else if (result.hasProperty("request_id"))
                {
                    response.success = true;
                    response.requestId = result["request_id"].toString();

                    // If preset is immediately available
                    if (result.hasProperty("preset"))
                    {
                        response.presetData = result["preset"];
                    }
                }
                else
                {
                    response.success = false;
                    response.errorMessage = "Invalid response format";
                }
            }
            else
            {
                response.success = false;
                response.errorMessage = "Request failed";
            }
        }
        catch (const std::exception& e)
        {
            response.success = false;
            response.errorMessage = juce::String("Exception: ") + e.what();
        }

        // Call callback on message thread
        juce::MessageManager::callAsync([callback, response]() {
            callback(response);
        });
    });
}

TrinityAIClient::ProgressResponse TrinityAIClient::checkProgress(const juce::String& requestId)
{
    ProgressResponse response;

    try
    {
        auto result = makeRequest("/progress/" + requestId, "GET");

        if (result.isObject())
        {
            response.status = result.getProperty("status", "unknown").toString();
            response.progress = result.getProperty("progress", 0.0f);
            response.message = result.getProperty("message", "").toString();

            if (result.hasProperty("preset"))
            {
                response.presetData = result["preset"];
            }
        }
        else
        {
            response.status = "error";
            response.progress = 0.0f;
            response.message = "Invalid response";
        }
    }
    catch (...)
    {
        response.status = "error";
        response.progress = 0.0f;
        response.message = "Request failed";
    }

    return response;
}

juce::var TrinityAIClient::makeRequest(const juce::String& endpoint,
                                      const juce::String& method,
                                      const juce::var& payload)
{
    juce::URL url(serverUrl + endpoint);

    juce::URL::InputStreamOptions options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(5000)
        .withResponseHeaders(nullptr)
        .withExtraHeaders("Content-Type: application/json");

    if (method == "POST" && !payload.isVoid())
    {
        juce::String jsonString = juce::JSON::toString(payload);
        options = options.withPostData(jsonString);
    }

    std::unique_ptr<juce::InputStream> stream(url.createInputStream(options));

    if (stream != nullptr)
    {
        juce::String response = stream->readEntireStreamAsString();
        return juce::JSON::parse(response);
    }

    return juce::var();
}

juce::String TrinityAIClient::encodeAudioToBase64(const juce::AudioBuffer<float>& buffer)
{
    // Convert float buffer to 16-bit PCM
    juce::MemoryOutputStream pcmStream;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float floatSample = buffer.getSample(0, sample);  // Mono for now

        // Clamp and convert to 16-bit
        floatSample = juce::jlimit(-1.0f, 1.0f, floatSample);
        int16_t pcmSample = static_cast<int16_t>(floatSample * 32767.0f);

        // Write little-endian 16-bit
        pcmStream.writeByte(static_cast<char>(pcmSample & 0xFF));
        pcmStream.writeByte(static_cast<char>((pcmSample >> 8) & 0xFF));
    }

    // Encode to base64
    return juce::Base64::toBase64(pcmStream.getData(), pcmStream.getDataSize());
}
