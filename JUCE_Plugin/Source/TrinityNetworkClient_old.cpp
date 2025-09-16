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
    auto headers = createAuthHeaders();
    
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
    
    juce::URL messageUrl(httpEndpoint + "/message");
    auto headers = createAuthHeaders();
    headers.set("Content-Type", "application/json");
    
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(10000)
        .withHttpRequestCmd("POST")
        .withExtraHeaders(headers)
        .withDataToPost(message);
    
    auto stream = messageUrl.createInputStream(options);
    
    if (stream) {
        juce::String response = stream->readEntireStreamAsString();
        if (onMessageReceived && !response.isEmpty()) {
            onMessageReceived(response);
        }
    } else {
        if (onConnectionError) onConnectionError("Failed to send message");
    }
}

void HTTPTrinityTransport::pollForMessages() {
    if (!isConnected() || shouldStopPolling) return;
    
    juce::URL pollUrl(httpEndpoint + "/poll?session=" + sessionId);
    auto headers = createAuthHeaders();
    
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
    juce::StringPairArray headers;
    if (!apiKey.isEmpty()) {
        headers.set("Authorization", "Bearer " + apiKey);
    }
    headers.set("User-Agent", "Chimera-Phoenix/3.0-HTTP");
    headers.set("X-Session-ID", sessionId);
    
    juce::String headerString;
    for (int i = 0; i < headers.size(); ++i) {
        if (i > 0) headerString += "\r\n";
        headerString += headers.getAllKeys()[i] + ": " + headers.getAllValues()[i];
    }
    return headerString;
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
    message.timestamp = Time::currentTimeMillis();
    
    sendMessage(message);
}

// === SESSION MANAGEMENT ===

void TrinityNetworkClient::startSession(const String& sessionType) {
    currentSessionId = generateSessionId();
    
    TrinityMessage message;
    message.type = "start_session";
    message.content = sessionType;
    message.sessionId = currentSessionId;
    message.timestamp = Time::currentTimeMillis();
    
    DynamicObject::Ptr sessionData = new DynamicObject();
    sessionData->setProperty("session_type", sessionType);
    sessionData->setProperty("plugin_version", "Chimera Phoenix v3.0");
    message.data = var(sessionData.get());
    
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
    message.timestamp = Time::currentTimeMillis();
    
    sendMessage(message, [this](const TrinityResponse& response) {
        String endedSessionId = currentSessionId;
        currentSessionId = "";
        listeners.call(&Listener::trinitySessionEnded, endedSessionId);
    });
}

// === WEBSOCKET LISTENER IMPLEMENTATION ===

void TrinityNetworkClient::connectionOpened() {
    connectionState = ConnectionState::Connected;
    connectionRetryCount = 0;
    lastHeartbeat = Time::currentTimeMillis();
    
    notifyStateChange(ConnectionState::Connected);
    
    // Start a new session automatically
    startSession("sound_design");
}

void TrinityNetworkClient::connectionClosed(int statusCode, const String& reason) {
    DBG("Trinity WebSocket connection closed: " << statusCode << " - " << reason);
    
    if (shouldStop) {
        connectionState = ConnectionState::Disconnected;
    } else {
        connectionState = ConnectionState::Reconnecting;
        notifyStateChange(ConnectionState::Reconnecting);
        connectionEvent.signal(); // Trigger reconnection
    }
}

void TrinityNetworkClient::connectionError(const String& errorMessage) {
    DBG("Trinity WebSocket error: " << errorMessage);
    
    connectionState = ConnectionState::Error;
    notifyStateChange(ConnectionState::Error);
    notifyError(errorMessage);
    
    if (trinityConfig.enableAutoReconnect && !shouldStop) {
        connectionState = ConnectionState::Reconnecting;
        notifyStateChange(ConnectionState::Reconnecting);
        connectionEvent.signal(); // Trigger reconnection
    }
}

void TrinityNetworkClient::messageReceived(const String& message) {
    try {
        TrinityResponse response = parseResponse(message);
        notifyResponse(response);
    } catch (const std::exception& e) {
        notifyError(String("Failed to parse Trinity response: ") + e.what());
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
        
        // Send heartbeat if connected
        if (isConnected()) {
            int64 now = Time::currentTimeMillis();
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
    int64 now = Time::currentTimeMillis();
    
    // Rate limit connection attempts
    if (now - lastConnectionAttempt < trinityConfig.retryDelayMs) {
        return;
    }
    
    lastConnectionAttempt = now;
    
    try {
        DBG("Attempting Trinity connection to: " << trinityConfig.cloudEndpoint);
        
        // Create WebSocket connection
        webSocket = std::make_unique<WebSocket>();
        
        // Set up headers with API key
        StringPairArray headers;
        if (!trinityConfig.apiKey.isEmpty()) {
            headers.set("Authorization", "Bearer " + trinityConfig.apiKey);
        }
        headers.set("User-Agent", "Chimera-Phoenix/3.0");
        
        // Connect with headers
        bool connected = webSocket->connect(URL(trinityConfig.cloudEndpoint), this, headers);
        
        if (!connected) {
            throw std::runtime_error("Failed to initiate WebSocket connection");
        }
        
        // Connection attempt successful, now wait for connectionOpened callback
        
    } catch (const std::exception& e) {
        connectionRetryCount++;
        
        if (connectionRetryCount >= trinityConfig.maxRetries) {
            connectionState = ConnectionState::Error;
            notifyStateChange(ConnectionState::Error);
            notifyError(String("Connection failed after ") + String(trinityConfig.maxRetries) + " attempts: " + e.what());
        } else {
            // Continue trying to reconnect
            DBG("Trinity connection attempt " << connectionRetryCount << " failed: " << e.what());
        }
    }
}

void TrinityNetworkClient::sendHeartbeat() {
    if (!webSocket || !isConnected()) return;
    
    TrinityMessage heartbeat;
    heartbeat.type = "heartbeat";
    heartbeat.content = "ping";
    heartbeat.sessionId = currentSessionId;
    heartbeat.timestamp = Time::currentTimeMillis();
    
    String jsonMessage = createMessageJson(heartbeat);
    webSocket->send(jsonMessage);
}

void TrinityNetworkClient::processMessageQueue() {
    std::queue<PendingMessage> currentQueue;
    
    {
        const ScopedLock sl(queueLock);
        currentQueue.swap(messageQueue);
    }
    
    while (!currentQueue.empty() && !shouldStop) {
        PendingMessage pending = currentQueue.front();
        currentQueue.pop();
        
        // Check timeout
        int64 elapsed = Time::currentTimeMillis() - pending.timestamp;
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
        if (isConnected() && webSocket) {
            String jsonMessage = createMessageJson(pending.message);
            bool sent = webSocket->send(jsonMessage);
            
            if (!sent) {
                // Failed to send, add to retry queue
                if (pending.retryCount < trinityConfig.maxRetries) {
                    pending.retryCount++;
                    
                    const ScopedLock sl(queueLock);
                    retryQueue.push(pending);
                }
            }
            // If sent successfully and has callback, response will come via messageReceived
        } else {
            // Not connected, try HTTP fallback or retry later
            if (pending.retryCount < trinityConfig.maxRetries) {
                pending.retryCount++;
                
                const ScopedLock sl(queueLock);
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
        const ScopedLock sl(queueLock);
        currentRetryQueue.swap(retryQueue);
    }
    
    while (!currentRetryQueue.empty() && !shouldStop) {
        PendingMessage pending = currentRetryQueue.front();
        currentRetryQueue.pop();
        
        // Add back to main queue for retry
        const ScopedLock sl(queueLock);
        messageQueue.push(pending);
    }
}

String TrinityNetworkClient::createMessageJson(const TrinityMessage& message) {
    DynamicObject::Ptr jsonObject = new DynamicObject();
    
    jsonObject->setProperty("type", message.type);
    jsonObject->setProperty("content", message.content);
    jsonObject->setProperty("session_id", message.sessionId);
    jsonObject->setProperty("timestamp", message.timestamp);
    
    if (!message.data.isVoid()) {
        jsonObject->setProperty("data", message.data);
    }
    
    return JSON::toString(var(jsonObject.get()));
}

TrinityNetworkClient::TrinityResponse TrinityNetworkClient::parseResponse(const String& jsonResponse) {
    TrinityResponse response;
    
    var parsed;
    Result parseResult = JSON::parse(jsonResponse, parsed);
    
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
            response.data = parsed.getProperty("data", var());
        }
        
        if (parsed.hasProperty("response_time_ms")) {
            response.responseTimeMs = parsed.getProperty("response_time_ms", 0);
        }
    }
    
    return response;
}

// === HTTP FALLBACK ===

TrinityNetworkClient::TrinityResponse TrinityNetworkClient::sendHttpRequest(const TrinityMessage& message) {
    TrinityResponse response;
    
    try {
        URL apiUrl(trinityConfig.httpEndpoint + "/message");
        
        String jsonBody = createMessageJson(message);
        
        StringPairArray headers;
        headers.set("Content-Type", "application/json");
        if (!trinityConfig.apiKey.isEmpty()) {
            headers.set("Authorization", "Bearer " + trinityConfig.apiKey);
        }
        
        auto options = URL::InputStreamOptions(URL::ParameterHandling::inAddress)
            .withConnectionTimeoutMs(trinityConfig.connectionTimeoutMs)
            .withHttpRequestCmd("POST")
            .withExtraHeaders(headers)
            .withDataToPost(jsonBody);
        
        std::unique_ptr<InputStream> stream(apiUrl.createInputStream(options));
        
        if (stream) {
            String jsonResponse = stream->readEntireStreamAsString();
            response = parseResponse(jsonResponse);
        } else {
            response.success = false;
            response.message = "Failed to connect to Trinity HTTP API";
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.message = String("HTTP request failed: ") + e.what();
    }
    
    return response;
}

// === UTILITY METHODS ===

String TrinityNetworkClient::generateMessageId() const {
    return String::toHexString(Random::getSystemRandom().nextInt64());
}

String TrinityNetworkClient::generateSessionId() const {
    return "session_" + String::toHexString(Time::currentTimeMillis()) + "_" + 
           String::toHexString(Random::getSystemRandom().nextInt64());
}

void TrinityNetworkClient::notifyStateChange(ConnectionState newState) {
    listeners.call(&Listener::trinityConnectionStateChanged, newState);
}

void TrinityNetworkClient::notifyResponse(const TrinityResponse& response) {
    listeners.call(&Listener::trinityMessageReceived, response);
}

void TrinityNetworkClient::notifyError(const String& error) {
    listeners.call(&Listener::trinityError, error);
}

void TrinityNetworkClient::addListener(Listener* listener) {
    listeners.add(listener);
}

void TrinityNetworkClient::removeListener(Listener* listener) {
    listeners.remove(listener);
}