#include "TrinityNetworkClient.h"

// === HTTPTrinityTransport Implementation ===

HTTPTrinityTransport::HTTPTrinityTransport() {
    generateSessionId();
}

HTTPTrinityTransport::~HTTPTrinityTransport() {
    disconnect();
}

void HTTPTrinityTransport::connect() {
    if (httpEndpoint.isEmpty()) {
        if (onConnectionError) onConnectionError("No endpoint configured");
        return;
    }
    
    // Test connection with a simple ping
    juce::URL testUrl(httpEndpoint + "/ping");
    juce::String headers;
    if (!apiKey.isEmpty()) {
        headers += "Authorization: Bearer " + apiKey + "\r\n";
    }
    headers += "User-Agent: Chimera-Phoenix/3.0-HTTP\r\n";
    headers += "X-Session-ID: " + sessionId + "\r\n";
    
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(5000)
        .withExtraHeaders(headers);
    
    auto stream = testUrl.createInputStream(options);
    
    if (stream && stream->getTotalLength() >= 0) {
        connected = true;
        shouldStopPolling = false;
        if (onConnected) onConnected();
    } else {
        connected = false;
        if (onConnectionError) onConnectionError("Failed to connect to Trinity HTTP endpoint");
    }
}

void HTTPTrinityTransport::disconnect() {
    connected = false;
    shouldStopPolling = true;
    if (onDisconnected) onDisconnected();
}

void HTTPTrinityTransport::sendMessage(const juce::String& message) {
    if (!isConnected()) {
        if (onConnectionError) onConnectionError("Not connected");
        return;
    }
    
    // Parse message to determine endpoint
    juce::String endpoint = "/message";
    try {
        juce::var messageData = juce::JSON::parse(message);
        if (messageData.hasProperty("type")) {
            juce::String messageType = messageData["type"].toString();
            if (messageType == "modify") {
                endpoint = "/modify";
            } else if (messageType == "suggestions") {
                endpoint = "/suggestions";
            } else if (messageType == "start_session") {
                endpoint = "/session/start";
            } else if (messageType == "end_session") {
                endpoint = "/session/end";
            }
        }
    } catch (...) {
        // Default to /message if parsing fails
    }
    
    juce::URL messageUrl(httpEndpoint + endpoint);
    juce::String headers;
    if (!apiKey.isEmpty()) {
        headers += "Authorization: Bearer " + apiKey + "\r\n";
    }
    headers += "User-Agent: Chimera-Phoenix/3.0-HTTP\r\n";
    headers += "X-Session-ID: " + sessionId + "\r\n";
    headers += "Content-Type: application/json\r\n";
    
    // Use the simpler API for POST requests
    messageUrl = messageUrl.withPOSTData(message);
    
    // Try request with extended timeout (120 seconds for Trinity AI preset generation)
    // Some macOS CFNetwork configurations have hard limits, so we retry if needed
    std::unique_ptr<juce::InputStream> stream;
    int maxRetries = 2;
    int retryCount = 0;

    while (retryCount < maxRetries) {
        DBG("Trinity: Sending HTTP request (attempt " + juce::String(retryCount + 1) + " of " + juce::String(maxRetries) + ")");

        stream = messageUrl.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                              .withConnectionTimeoutMs(120000)  // 120 seconds for Trinity preset generation
                                              .withExtraHeaders(headers));

        if (stream) {
            DBG("Trinity: Stream created, reading response...");
            juce::String response = stream->readEntireStreamAsString();

            if (!response.isEmpty()) {
                DBG("Trinity: Received response (" + juce::String(response.length()) + " bytes)");
                if (onMessageReceived) {
                    onMessageReceived(response);
                }
                return;  // Success!
            } else {
                DBG("Trinity: Response was empty, retrying...");
            }
        } else {
            DBG("Trinity: Failed to create stream (timeout or connection error)");
        }

        retryCount++;
        if (retryCount < maxRetries) {
            DBG("Trinity: Waiting 2 seconds before retry...");
            juce::Thread::sleep(2000);
        }
    }

    // All retries failed
    DBG("Trinity: All attempts failed");
    if (onConnectionError) onConnectionError("Failed to send message after " + juce::String(maxRetries) + " attempts");
}

void HTTPTrinityTransport::pollForMessages() {
    if (!isConnected() || shouldStopPolling) return;
    
    juce::URL pollUrl(httpEndpoint + "/poll?session=" + sessionId);
    juce::String headers;
    if (!apiKey.isEmpty()) {
        headers += "Authorization: Bearer " + apiKey + "\r\n";
    }
    headers += "User-Agent: Chimera-Phoenix/3.0-HTTP\r\n";
    headers += "X-Session-ID: " + sessionId + "\r\n";
    
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(30000) // Long polling
        .withExtraHeaders(headers);
    
    auto stream = pollUrl.createInputStream(options);
    
    if (stream) {
        juce::String response = stream->readEntireStreamAsString();
        if (onMessageReceived && !response.isEmpty() && response != "{}" && response != "null") {
            onMessageReceived(response);
        }
    }
}

void HTTPTrinityTransport::generateSessionId() {
    sessionId = "http_session_" + juce::String::toHexString(juce::Time::currentTimeMillis()) + "_" + 
                juce::String::toHexString(juce::Random::getSystemRandom().nextInt64());
}

juce::String HTTPTrinityTransport::createAuthHeaders() const {
    juce::String headers;
    if (!apiKey.isEmpty()) {
        headers += "Authorization: Bearer " + apiKey + "\r\n";
    }
    headers += "User-Agent: Chimera-Phoenix/3.0-HTTP\r\n";
    headers += "X-Session-ID: " + sessionId + "\r\n";
    
    // Headers are already formatted, just return them
    return headers;
}

// === TrinityNetworkClient Implementation ===

TrinityNetworkClient::TrinityNetworkClient() : juce::Thread("TrinityNetworkClient") {
    // Initialize default configuration
    trinityConfig = TrinityConfig();
    
    // Create HTTP transport by default
    transport = std::make_unique<HTTPTrinityTransport>();
    
    // Set up transport callbacks
    auto httpTransport = static_cast<HTTPTrinityTransport*>(transport.get());
    httpTransport->onConnected = [this]() { onTransportConnected(); };
    httpTransport->onDisconnected = [this]() { onTransportDisconnected(); };
    httpTransport->onConnectionError = [this](const juce::String& error) { onTransportError(error); };
    httpTransport->onMessageReceived = [this](const juce::String& message) { onTransportMessageReceived(message); };
    
    // Start the background thread for message processing
    startThread();
}

TrinityNetworkClient::~TrinityNetworkClient() {
    // Signal shutdown
    shouldStop = true;
    connectionEvent.signal();
    
    // Disconnect cleanly
    disconnect();
    
    // Stop the thread
    stopThread(5000);
}

// === CONNECTION MANAGEMENT ===

void TrinityNetworkClient::connectToTrinity(const juce::String& apiKey, const juce::String& endpoint) {
    if (!apiKey.isEmpty()) {
        trinityConfig.apiKey = apiKey;
        transport->setApiKey(apiKey);
    }
    if (!endpoint.isEmpty()) {
        trinityConfig.httpEndpoint = endpoint;
        transport->setEndpoint(endpoint);
    } else {
        transport->setEndpoint(trinityConfig.httpEndpoint);
    }
    
    if (connectionState.load() == ConnectionState::Connected) {
        return; // Already connected
    }
    
    connectionState = ConnectionState::Connecting;
    notifyStateChange(ConnectionState::Connecting);
    
    // Signal the background thread to attempt connection
    connectionEvent.signal();
}

void TrinityNetworkClient::disconnect() {
    connectionState = ConnectionState::Disconnected;
    
    if (transport) {
        transport->disconnect();
    }
    
    // End current session
    if (!currentSessionId.isEmpty()) {
        endSession();
    }
    
    notifyStateChange(ConnectionState::Disconnected);
}

juce::String TrinityNetworkClient::getConnectionStateString() const {
    switch (connectionState.load()) {
        case ConnectionState::Disconnected: return "Disconnected";
        case ConnectionState::Connecting: return "Connecting";
        case ConnectionState::Connected: return "Connected";
        case ConnectionState::Reconnecting: return "Reconnecting";
        case ConnectionState::Error: return "Error";
        default: return "Unknown";
    }
}

// === AI COMMUNICATION ===

void TrinityNetworkClient::sendMessage(const TrinityMessage& message, ResponseCallback callback) {
    if (!isConnected() && connectionState.load() != ConnectionState::Connecting) {
        // Auto-connect if not connected
        connectToTrinity();
    }
    
    PendingMessage pending;
    pending.id = generateMessageId();
    pending.message = message;
    pending.callback = callback;
    pending.timestamp = juce::Time::currentTimeMillis();
    
    {
        const juce::ScopedLock sl(queueLock);
        messageQueue.push(pending);
    }
    
    connectionEvent.signal();
}

void TrinityNetworkClient::sendQuery(const juce::String& query, ResponseCallback callback) {
    TrinityMessage message;
    message.type = "query";
    message.content = query;
    message.sessionId = currentSessionId;
    message.timestamp = juce::Time::currentTimeMillis();
    
    sendMessage(message, callback);
}

void TrinityNetworkClient::sendPluginState(const juce::var& stateData) {
    TrinityMessage message;
    message.type = "plugin_state";
    message.content = "Current plugin state update";
    message.data = stateData;
    message.sessionId = currentSessionId;
    message.timestamp = juce::Time::currentTimeMillis();
    
    sendMessage(message);
}

void TrinityNetworkClient::sendModification(const juce::var& preset, const juce::String& modification, ResponseCallback callback) {
    TrinityMessage message;
    message.type = "modify";
    message.content = modification;
    
    juce::DynamicObject::Ptr data = new juce::DynamicObject();
    data->setProperty("preset", preset);
    data->setProperty("modification", modification);
    message.data = juce::var(data.get());
    
    message.sessionId = currentSessionId;
    message.timestamp = juce::Time::currentTimeMillis();
    
    sendMessage(message, callback);
}

void TrinityNetworkClient::getSuggestions(const juce::var& preset, ResponseCallback callback) {
    TrinityMessage message;
    message.type = "suggestions";
    message.content = "Get modification suggestions";
    
    juce::DynamicObject::Ptr data = new juce::DynamicObject();
    data->setProperty("preset", preset);
    message.data = juce::var(data.get());
    
    message.sessionId = currentSessionId;
    message.timestamp = juce::Time::currentTimeMillis();
    
    sendMessage(message, callback);
}

// === SESSION MANAGEMENT ===

void TrinityNetworkClient::startSession(const juce::String& sessionType) {
    currentSessionId = generateSessionId();
    
    TrinityMessage message;
    message.type = "start_session";
    message.content = sessionType;
    message.sessionId = currentSessionId;
    message.timestamp = juce::Time::currentTimeMillis();
    
    juce::DynamicObject::Ptr sessionData = new juce::DynamicObject();
    sessionData->setProperty("session_type", sessionType);
    sessionData->setProperty("plugin_version", "Chimera Phoenix v3.0");
    message.data = juce::var(sessionData.get());
    
    sendMessage(message, [this](const TrinityResponse& response) {
        if (response.success) {
            listeners.call(&Listener::trinitySessionStarted, currentSessionId);
        }
    });
}

void TrinityNetworkClient::endSession() {
    if (currentSessionId.isEmpty()) return;
    
    TrinityMessage message;
    message.type = "end_session";
    message.content = "Session ended";
    message.sessionId = currentSessionId;
    message.timestamp = juce::Time::currentTimeMillis();
    
    sendMessage(message, [this](const TrinityResponse& response) {
        juce::String endedSessionId = currentSessionId;
        currentSessionId = "";
        listeners.call(&Listener::trinitySessionEnded, endedSessionId);
    });
}

// === TRANSPORT LAYER CALLBACKS ===

void TrinityNetworkClient::onTransportConnected() {
    connectionState = ConnectionState::Connected;
    connectionRetryCount = 0;
    lastHeartbeat = juce::Time::currentTimeMillis();
    
    notifyStateChange(ConnectionState::Connected);
    
    // Start a new session automatically
    startSession("sound_design");
}

void TrinityNetworkClient::onTransportDisconnected() {
    DBG("Trinity transport disconnected");
    
    if (shouldStop) {
        connectionState = ConnectionState::Disconnected;
    } else {
        connectionState = ConnectionState::Reconnecting;
        notifyStateChange(ConnectionState::Reconnecting);
        connectionEvent.signal(); // Trigger reconnection
    }
}

void TrinityNetworkClient::onTransportError(const juce::String& errorMessage) {
    DBG("Trinity transport error: " << errorMessage);
    
    connectionState = ConnectionState::Error;
    notifyStateChange(ConnectionState::Error);
    notifyError(errorMessage);
    
    if (trinityConfig.enableAutoReconnect && !shouldStop) {
        connectionState = ConnectionState::Reconnecting;
        notifyStateChange(ConnectionState::Reconnecting);
        connectionEvent.signal(); // Trigger reconnection
    }
}

void TrinityNetworkClient::onTransportMessageReceived(const juce::String& message) {
    try {
        TrinityResponse response = parseResponse(message);
        notifyResponse(response);
    } catch (const std::exception& e) {
        notifyError(juce::String("Failed to parse Trinity response: ") + e.what());
    }
}

// === PRIVATE IMPLEMENTATION ===

void TrinityNetworkClient::run() {
    while (!shouldStop && !threadShouldExit()) {
        // Handle connection state
        if (connectionState.load() == ConnectionState::Connecting ||
            connectionState.load() == ConnectionState::Reconnecting) {
            attemptConnection();
        }
        
        // Process message queues
        processMessageQueue();
        processRetryQueue();
        
        // Poll for messages if using HTTP transport
        if (isConnected() && transport) {
            auto httpTransport = dynamic_cast<HTTPTrinityTransport*>(transport.get());
            if (httpTransport) {
                httpTransport->pollForMessages();
            }
        }
        
        // Send heartbeat if connected
        if (isConnected()) {
            juce::int64 now = juce::Time::currentTimeMillis();
            if (now - lastHeartbeat > trinityConfig.heartbeatIntervalMs) {
                sendHeartbeat();
                lastHeartbeat = now;
            }
        }
        
        // Wait for next cycle or signal
        connectionEvent.wait(1000); // Check every second
    }
}

void TrinityNetworkClient::attemptConnection() {
    juce::int64 now = juce::Time::currentTimeMillis();
    
    // Rate limit connection attempts
    if (now - lastConnectionAttempt < trinityConfig.retryDelayMs) {
        return;
    }
    
    lastConnectionAttempt = now;
    
    try {
        DBG("Attempting Trinity connection to: " << trinityConfig.httpEndpoint);
        
        // Configure transport
        if (transport) {
            transport->setEndpoint(trinityConfig.httpEndpoint);
            transport->setApiKey(trinityConfig.apiKey);
            transport->connect();
        } else {
            throw std::runtime_error("No transport configured");
        }
        
        // Connection attempt initiated, callbacks will handle the result
        
    } catch (const std::exception& e) {
        connectionRetryCount++;
        
        if (connectionRetryCount >= trinityConfig.maxRetries) {
            connectionState = ConnectionState::Error;
            notifyStateChange(ConnectionState::Error);
            notifyError(juce::String("Connection failed after ") + juce::String(trinityConfig.maxRetries) + " attempts: " + e.what());
        } else {
            // Continue trying to reconnect
            DBG("Trinity connection attempt " << connectionRetryCount << " failed: " << e.what());
        }
    }
}

void TrinityNetworkClient::sendHeartbeat() {
    if (!transport || !isConnected()) return;
    
    TrinityMessage heartbeat;
    heartbeat.type = "heartbeat";
    heartbeat.content = "ping";
    heartbeat.sessionId = currentSessionId;
    heartbeat.timestamp = juce::Time::currentTimeMillis();
    
    juce::String jsonMessage = createMessageJson(heartbeat);
    transport->sendMessage(jsonMessage);
}

void TrinityNetworkClient::processMessageQueue() {
    std::queue<PendingMessage> currentQueue;
    
    {
        const juce::ScopedLock sl(queueLock);
        currentQueue.swap(messageQueue);
    }
    
    while (!currentQueue.empty() && !shouldStop) {
        PendingMessage pending = currentQueue.front();
        currentQueue.pop();
        
        // Check timeout
        juce::int64 elapsed = juce::Time::currentTimeMillis() - pending.timestamp;
        if (elapsed > trinityConfig.messageTimeoutMs) {
            // Message timed out
            if (pending.callback) {
                TrinityResponse response;
                response.success = false;
                response.type = "timeout";
                response.message = "Message timed out";
                response.responseTimeMs = (int)elapsed;
                pending.callback(response);
            }
            continue;
        }
        
        // Try to send the message
        if (isConnected() && transport) {
            juce::String jsonMessage = createMessageJson(pending.message);
            try {
                transport->sendMessage(jsonMessage);
                // Message sent successfully, response will come via onTransportMessageReceived
            } catch (const std::exception& e) {
                // Failed to send, add to retry queue
                if (pending.retryCount < trinityConfig.maxRetries) {
                    pending.retryCount++;
                    
                    const juce::ScopedLock sl(queueLock);
                    retryQueue.push(pending);
                }
            }
        } else {
            // Not connected, try HTTP fallback or retry later
            if (pending.retryCount < trinityConfig.maxRetries) {
                pending.retryCount++;
                
                const juce::ScopedLock sl(queueLock);
                retryQueue.push(pending);
            } else if (pending.callback) {
                // Max retries exceeded
                TrinityResponse response;
                response.success = false;
                response.type = "error";
                response.message = "Failed to send message - not connected";
                pending.callback(response);
            }
        }
    }
}

void TrinityNetworkClient::processRetryQueue() {
    std::queue<PendingMessage> currentRetryQueue;
    
    {
        const juce::ScopedLock sl(queueLock);
        currentRetryQueue.swap(retryQueue);
    }
    
    while (!currentRetryQueue.empty() && !shouldStop) {
        PendingMessage pending = currentRetryQueue.front();
        currentRetryQueue.pop();
        
        // Add back to main queue for retry
        const juce::ScopedLock sl(queueLock);
        messageQueue.push(pending);
    }
}

juce::String TrinityNetworkClient::createMessageJson(const TrinityMessage& message) {
    juce::DynamicObject::Ptr jsonObject = new juce::DynamicObject();
    
    jsonObject->setProperty("type", message.type);
    jsonObject->setProperty("content", message.content);
    jsonObject->setProperty("session_id", message.sessionId);
    jsonObject->setProperty("timestamp", message.timestamp);
    
    if (!message.data.isVoid()) {
        jsonObject->setProperty("data", message.data);
    }
    
    return juce::JSON::toString(juce::var(jsonObject.get()));
}

TrinityNetworkClient::TrinityResponse TrinityNetworkClient::parseResponse(const juce::String& jsonResponse) {
    TrinityResponse response;
    
    juce::var parsed;
    juce::Result parseResult = juce::JSON::parse(jsonResponse, parsed);
    
    if (parseResult.failed()) {
        response.success = false;
        response.type = "parse_error";
        response.message = "Failed to parse JSON response";
        return response;
    }
    
    if (parsed.isObject()) {
        response.success = parsed.getProperty("success", false);
        response.type = parsed.getProperty("type", "unknown").toString();
        response.message = parsed.getProperty("message", "").toString();
        response.sessionId = parsed.getProperty("session_id", "").toString();
        
        if (parsed.hasProperty("data")) {
            response.data = parsed.getProperty("data", juce::var());
        }
        
        if (parsed.hasProperty("response_time_ms")) {
            response.responseTimeMs = parsed.getProperty("response_time_ms", 0);
        }
    }
    
    return response;
}

// === HTTP METHODS (Legacy support) ===

TrinityNetworkClient::TrinityResponse TrinityNetworkClient::sendHttpRequest(const TrinityMessage& message) {
    TrinityResponse response;
    
    try {
        juce::URL apiUrl(trinityConfig.httpEndpoint + "/message");
        
        juce::String jsonBody = createMessageJson(message);
        
        juce::String headers;
        headers += "Content-Type: application/json\r\n";
        if (!trinityConfig.apiKey.isEmpty()) {
            headers += "Authorization: Bearer " + trinityConfig.apiKey + "\r\n";
        }
        
        // Use the simpler API for POST requests
        apiUrl = apiUrl.withPOSTData(jsonBody);
        
        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withConnectionTimeoutMs(trinityConfig.connectionTimeoutMs)
            .withExtraHeaders(headers);
        
        std::unique_ptr<juce::InputStream> stream(apiUrl.createInputStream(options));
        
        if (stream) {
            juce::String jsonResponse = stream->readEntireStreamAsString();
            response = parseResponse(jsonResponse);
        } else {
            response.success = false;
            response.message = "Failed to connect to Trinity HTTP API";
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.message = juce::String("HTTP request failed: ") + e.what();
    }
    
    return response;
}

// === UTILITY METHODS ===

juce::String TrinityNetworkClient::generateMessageId() const {
    return juce::String::toHexString(juce::Random::getSystemRandom().nextInt64());
}

juce::String TrinityNetworkClient::generateSessionId() const {
    return "session_" + juce::String::toHexString(juce::Time::currentTimeMillis()) + "_" + 
           juce::String::toHexString(juce::Random::getSystemRandom().nextInt64());
}

void TrinityNetworkClient::notifyStateChange(ConnectionState newState) {
    listeners.call(&Listener::trinityConnectionStateChanged, newState);
}

void TrinityNetworkClient::notifyResponse(const TrinityResponse& response) {
    listeners.call(&Listener::trinityMessageReceived, response);
}

void TrinityNetworkClient::notifyError(const juce::String& error) {
    listeners.call(&Listener::trinityError, error);
}

void TrinityNetworkClient::addListener(Listener* listener) {
    listeners.add(listener);
}

void TrinityNetworkClient::removeListener(Listener* listener) {
    listeners.remove(listener);
}