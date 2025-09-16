#include "TrinityManager.h"
#include "PluginProcessor.h"

TrinityManager::TrinityManager(ChimeraAudioProcessor& processor) : audioProcessor(processor) {
    // Initialize default settings
    settings = TrinitySettings();
    
    // Create Trinity client
    trinityClient = std::make_unique<TrinityNetworkClient>();
    trinityClient->addListener(this);
}

TrinityManager::~TrinityManager() {
    shutdown();
}

// === LIFECYCLE MANAGEMENT ===

void TrinityManager::initialize() {
    if (isInitialized.load()) return;
    
    DBG("Initializing Trinity Manager...");
    
    // Configure Trinity client
    configureTrinityClient();
    
    // Auto-connect if enabled
    if (settings.autoConnect) {
        trinityClient->connectToTrinity(settings.apiKey, settings.cloudEndpoint);
    }
    
    // Start auto-suggestions if enabled
    if (settings.enableAutoSuggestions) {
        startAutoSuggestions();
    }
    
    isInitialized = true;
    DBG("Trinity Manager initialized successfully");
}

void TrinityManager::shutdown() {
    if (!isInitialized.load()) return;
    
    DBG("Shutting down Trinity Manager...");
    
    // Stop auto-suggestions
    stopAutoSuggestions();
    
    // Disconnect Trinity client
    if (trinityClient) {
        trinityClient->removeListener(this);
        trinityClient->disconnect();
    }
    
    // Clear pending callbacks
    {
        const juce::ScopedLock sl(callbackLock);
        pendingCallbacks.clear();
    }
    
    isInitialized = false;
    isConnected = false;
    
    DBG("Trinity Manager shutdown complete");
}

bool TrinityManager::isAvailable() const {
    return isInitialized.load() && isConnected.load() && trinityClient && trinityClient->isConnected();
}

juce::String TrinityManager::getConnectionStatus() const {
    if (!isInitialized.load()) return "Not Initialized";
    if (!trinityClient) return "No Client";
    
    return trinityClient->getConnectionStateString();
}

// === AI INTERACTION ===

void TrinityManager::sendQuery(const juce::String& query, std::function<void(const juce::String&, bool)> callback) {
    if (!isAvailable()) {
        if (callback) {
            callback("Trinity AI is not available", true);
        }
        return;
    }
    
    // Store callback for async response
    juce::String callbackId;
    if (callback) {
        callbackId = juce::String::toHexString(juce::Random::getSystemRandom().nextInt64());
        
        const juce::ScopedLock sl(callbackLock);
        pendingCallbacks[callbackId] = {callbackId, callback, juce::Time::currentTimeMillis()};
    }
    
    // Send query with callback ID in context
    TrinityNetworkClient::TrinityMessage message;
    message.type = TrinityProtocol::MessageType::QUERY;
    message.content = query;
    message.sessionId = currentSessionId;
    message.timestamp = juce::Time::currentTimeMillis();
    
    if (!callbackId.isEmpty()) {
        juce::DynamicObject::Ptr context = new juce::DynamicObject();
        context->setProperty("callback_id", callbackId);
        message.data = juce::var(context.get());
    }
    
    trinityClient->sendMessage(message, [this](const TrinityNetworkClient::TrinityResponse& response) {
        handleQueryResponse(response);
    });
}

void TrinityManager::requestSuggestions(const juce::String& context) {
    if (!isAvailable()) return;
    
    // Update plugin context first
    updatePluginContext();
    
    // Send suggestion request
    TrinityNetworkClient::TrinityMessage message;
    message.type = "suggestion_request";
    message.content = context.isEmpty() ? "Request parameter suggestions" : context;
    message.sessionId = currentSessionId;
    message.timestamp = juce::Time::currentTimeMillis();
    
    juce::DynamicObject::Ptr requestData = new juce::DynamicObject();
    requestData->setProperty("type", "parameter_suggestions");
    requestData->setProperty("context", context);
    message.data = juce::var(requestData.get());
    
    trinityClient->sendMessage(message, [this](const TrinityNetworkClient::TrinityResponse& response) {
        handleSuggestionResponse(response);
    });
}

void TrinityManager::requestPreset(const juce::String& description, const juce::String& genre, const juce::String& mood) {
    if (!isAvailable()) return;
    
    juce::var presetRequest = TrinityProtocol::createPresetRequestMessage(
        currentSessionId, description, genre, mood
    );
    
    TrinityNetworkClient::TrinityMessage message;
    message.type = TrinityProtocol::MessageType::PRESET_REQUEST;
    message.content = description;
    message.data = presetRequest;
    message.sessionId = currentSessionId;
    message.timestamp = juce::Time::currentTimeMillis();
    
    trinityClient->sendMessage(message, [this](const TrinityNetworkClient::TrinityResponse& response) {
        handlePresetResponse(response);
    });
}

void TrinityManager::updatePluginContext() {
    if (!isAvailable()) return;
    
    juce::var pluginState = getCurrentPluginState();
    trinityClient->sendPluginState(pluginState);
}

// === CONFIGURATION ===

void TrinityManager::setSettings(const TrinitySettings& newSettings) {
    settings = newSettings;
    
    if (isInitialized.load()) {
        // Reconfigure if already initialized
        configureTrinityClient();
        
        // Handle auto-suggestions
        if (settings.enableAutoSuggestions) {
            startAutoSuggestions();
        } else {
            stopAutoSuggestions();
        }
    }
}

// === PLUGIN STATE HELPERS ===

juce::var TrinityManager::getCurrentPluginState() {
    juce::Array<juce::var> slotStates;
    
    for (int i = 0; i < 6; ++i) {
        // Get current engine ID for this slot
        int engineId = audioProcessor.getEngineIDForSlot(i);
        juce::String engineName = "Engine_" + juce::String(engineId); // Could be enhanced with engine name lookup
        
        // Get bypass and solo states
        auto* bypassParam = audioProcessor.getValueTreeState().getRawParameterValue("slot" + juce::String(i + 1) + "_bypass");
        auto* soloParam = audioProcessor.getValueTreeState().getRawParameterValue("slot" + juce::String(i + 1) + "_solo");
        
        bool bypassed = bypassParam ? bypassParam->load() > 0.5f : false;
        bool soloed = soloParam ? soloParam->load() > 0.5f : false;
        
        // Collect parameter values
        juce::Array<juce::var> parameters;
        for (int p = 0; p < 15; ++p) {
            juce::String paramName = "slot" + juce::String(i + 1) + "_param" + juce::String(p + 1);
            auto* param = audioProcessor.getValueTreeState().getRawParameterValue(paramName);
            
            if (param) {
                float value = param->load();
                juce::var paramData = TrinityProtocol::createParameter(
                    "param" + juce::String(p + 1),
                    value,
                    0.5f, // Default value
                    "",   // Category
                    ""    // Unit
                );
                parameters.add(paramData);
            }
        }
        
        juce::var slotState = TrinityProtocol::createSlotState(
            i, engineId, engineName, bypassed, soloed, parameters
        );
        slotStates.add(slotState);
    }
    
    return TrinityProtocol::createPluginStateMessage(
        currentSessionId,
        slotStates
    );
}

void TrinityManager::applySuggestions(const juce::Array<juce::var>& suggestions) {
    for (const auto& suggestion : suggestions) {
        if (!suggestion.isObject()) continue;
        
        int slotIndex = suggestion.getProperty("slot_index", -1);
        juce::String parameterName = suggestion.getProperty("parameter_name", "").toString();
        float newValue = suggestion.getProperty("value", 0.0f);
        juce::String reason = suggestion.getProperty("reason", "").toString();
        
        if (slotIndex >= 0 && slotIndex < 6 && !parameterName.isEmpty()) {
            // Apply the suggested parameter change
            juce::String fullParamName = "slot" + juce::String(slotIndex + 1) + "_" + parameterName;
            auto* param = audioProcessor.getValueTreeState().getParameter(fullParamName);
            
            if (param) {
                param->setValueNotifyingHost(newValue);
                
                DBG("Trinity suggestion applied: " << fullParamName << " = " << newValue << " (" << reason << ")");
                
                // Notify listeners
                notifyListeners([=](Listener* l) {
                    l->trinityParameterSuggestion(slotIndex, parameterName, newValue);
                });
            }
        }
    }
}

void TrinityManager::applyPreset(const juce::var& presetData) {
    if (!presetData.isObject()) return;
    
    DBG("Applying Trinity preset...");
    
    // Parse and apply preset data
    if (presetData.hasProperty("slots")) {
        juce::var slotsData = presetData.getProperty("slots", juce::var());
        
        if (slotsData.isArray()) {
            for (int i = 0; i < slotsData.size() && i < 6; ++i) {
                juce::var slotData = slotsData[i];
                
                if (slotData.isObject()) {
                    // Apply engine selection
                    if (slotData.hasProperty("engine_id")) {
                        int engineId = slotData.getProperty("engine_id", 0);
                        audioProcessor.setSlotEngine(i, engineId);
                    }
                    
                    // Apply parameters
                    if (slotData.hasProperty("parameters")) {
        juce::var paramsData = slotData.getProperty("parameters", juce::var());
                        
                        if (paramsData.isArray()) {
                            for (int p = 0; p < paramsData.size(); ++p) {
                                juce::var paramData = paramsData[p];
                                
                                if (paramData.isObject()) {
                                    juce::String paramName = paramData.getProperty("name", "").toString();
                                    float value = paramData.getProperty("value", 0.5f);
                                    
                                    juce::String fullParamName = "slot" + juce::String(i + 1) + "_" + paramName;
                                    auto* param = audioProcessor.getValueTreeState().getParameter(fullParamName);
                                    
                                    if (param) {
                                        param->setValueNotifyingHost(value);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Notify listeners
    notifyListeners([=](Listener* l) {
        l->trinityPresetReceived(presetData);
    });
}

// === LISTENERS ===

void TrinityManager::addListener(Listener* listener) {
    listeners.add(listener);
}

void TrinityManager::removeListener(Listener* listener) {
    listeners.remove(listener);
}

// === TRINITY CLIENT LISTENER IMPLEMENTATION ===

void TrinityManager::trinityConnectionStateChanged(TrinityNetworkClient::ConnectionState newState) {
    bool wasConnected = isConnected.load();
    isConnected = (newState == TrinityNetworkClient::ConnectionState::Connected);
    
    if (wasConnected != isConnected.load()) {
        notifyListeners([=](Listener* l) {
            l->trinityStatusChanged(isConnected.load());
        });
    }
}

void TrinityManager::trinityMessageReceived(const TrinityNetworkClient::TrinityResponse& response) {
    if (!response.success) return;
    
    // Route response based on type
    if (response.type == "response") {
        handleQueryResponse(response);
    } else if (response.type == "suggestion") {
        handleSuggestionResponse(response);
    } else if (response.type == "preset") {
        handlePresetResponse(response);
    }
}

void TrinityManager::trinitySessionStarted(const juce::String& sessionId) {
    currentSessionId = sessionId;
    DBG("Trinity session started: " << sessionId);
}

void TrinityManager::trinitySessionEnded(const juce::String& sessionId) {
    currentSessionId = "";
    DBG("Trinity session ended: " << sessionId);
}

void TrinityManager::trinityError(const juce::String& error) {
    DBG("Trinity error: " << error);
    
    notifyListeners([=](Listener* l) {
        l->trinityError(error);
    });
}

// === PRIVATE METHODS ===

void TrinityManager::configureTrinityClient() {
    if (!trinityClient) return;
    
    TrinityNetworkClient::TrinityConfig config;
    config.cloudEndpoint = settings.cloudEndpoint;
    config.httpEndpoint = settings.httpEndpoint;
    config.apiKey = settings.apiKey;
    config.enableAutoReconnect = true;
    config.connectionTimeoutMs = 10000;
    config.heartbeatIntervalMs = 30000;
    
    trinityClient->setConfig(config);
}

void TrinityManager::handleQueryResponse(const TrinityNetworkClient::TrinityResponse& response) {
    // Check for callback ID
    juce::String callbackId;
    if (response.data.isObject() && response.data.hasProperty("callback_id")) {
        callbackId = response.data.getProperty("callback_id", "").toString();
    }
    
    // Execute callback if found
    if (!callbackId.isEmpty()) {
        const juce::ScopedLock sl(callbackLock);
        auto it = pendingCallbacks.find(callbackId);
        if (it != pendingCallbacks.end()) {
            auto callback = it->second.callback;
            pendingCallbacks.erase(it);
            
            if (callback) {
                callback(response.message, !response.success);
            }
        }
    }
    
    // Notify listeners
    notifyListeners([=](Listener* l) {
        l->trinityResponseReceived(response.message, !response.success);
    });
}

void TrinityManager::handleSuggestionResponse(const TrinityNetworkClient::TrinityResponse& response) {
    if (TrinityProtocol::hasParameterSuggestions(response.data)) {
        auto suggestions = TrinityProtocol::getParameterSuggestions(response.data);
        applySuggestions(suggestions);
    }
}

void TrinityManager::handlePresetResponse(const TrinityNetworkClient::TrinityResponse& response) {
    if (TrinityProtocol::hasPresetData(response.data)) {
        auto presetData = TrinityProtocol::getPresetData(response.data);
        applyPreset(presetData);
    }
}

void TrinityManager::startAutoSuggestions() {
    // TODO: Implement auto-suggestion timer
    // Requires creating a concrete Timer class
}

void TrinityManager::stopAutoSuggestions() {
    // TODO: Implement auto-suggestion timer
}

void TrinityManager::requestAutoSuggestions() {
    if (isAvailable()) {
        requestSuggestions("Automatic suggestion request");
    }
}

void TrinityManager::notifyListeners(std::function<void(Listener*)> callback) {
    listeners.call([&callback](Listener& l) { callback(&l); });
}