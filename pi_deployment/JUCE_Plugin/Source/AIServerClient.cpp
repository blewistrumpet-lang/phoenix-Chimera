#include "AIServerClient.h"

AIServerClient::AIServerClient() : Thread("AIServerClient") {
    startThread();
}

AIServerClient::~AIServerClient() {
    shouldStop = true;
    notify();  // Wake up the thread
    stopThread(5000);
    
    if (serverProcess && serverProcess->isRunning()) {
        serverProcess->kill();
    }
}

bool AIServerClient::isServerAvailable() {
    URL healthUrl(serverUrl + "/health");
    
    auto options = URL::InputStreamOptions(URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(2000);
    
    std::unique_ptr<InputStream> stream(healthUrl.createInputStream(options));
    
    if (stream) {
        String response = stream->readEntireStreamAsString();
        return response.contains("healthy");
    }
    
    return false;
}

AIServerClient::ServerHealth AIServerClient::checkServerHealth() {
    ServerHealth health;
    
    URL healthUrl(serverUrl + "/health");
    int64 startTime = Time::currentTimeMillis();
    
    auto options = URL::InputStreamOptions(URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(5000);
    
    std::unique_ptr<InputStream> stream(healthUrl.createInputStream(options));
    
    if (stream) {
        String response = stream->readEntireStreamAsString();
        health.responseTimeMs = (int)(Time::currentTimeMillis() - startTime);
        
        // Parse JSON response
        var jsonResponse = parseJsonResponse(response);
        if (jsonResponse.isObject()) {
            health.isHealthy = true;
            health.status = jsonResponse["status"].toString();
            health.version = jsonResponse.hasProperty("version") 
                ? jsonResponse["version"].toString() 
                : "Unknown";
        }
    }
    
    return health;
}

void AIServerClient::generatePreset(const PresetRequest& request, PresetCallback callback) {
    PendingRequest pending;
    pending.id = generateRequestId();
    pending.request = request;
    pending.callback = callback;
    pending.startTime = Time::currentTimeMillis();
    
    {
        const ScopedLock sl(queueLock);
        requestQueue.push(pending);
    }
    
    notify();  // Wake up the worker thread
    notifyRequestStarted(pending.id);
}

AIServerClient::PresetResponse AIServerClient::generatePresetSync(const PresetRequest& request) {
    return sendHttpRequest(request);
}

void AIServerClient::setServerUrl(const String& url) {
    serverUrl = url;
    if (!serverUrl.endsWith("/")) {
        serverUrl = serverUrl.trimCharactersAtEnd("/");
    }
}

bool AIServerClient::startServer() {
    if (serverProcess && serverProcess->isRunning()) {
        return true;  // Already running
    }
    
    // Path to Python script
    File serverScript = File::getSpecialLocation(File::currentExecutableFile)
        .getParentDirectory()
        .getParentDirectory()
        .getParentDirectory()
        .getChildFile("AI_Server/main.py");
    
    if (!serverScript.exists()) {
        DBG("AI Server script not found at: " << serverScript.getFullPathName());
        return false;
    }
    
    serverProcess = std::make_unique<ChildProcess>();
    
    // Start the FastAPI server with uvicorn
    String command = "python3 -m uvicorn main:app --host 0.0.0.0 --port 8000";
    
    if (serverProcess->start(command, serverScript.getParentDirectory())) {
        DBG("AI Server process started");
        
        // Wait for server to be ready
        for (int i = 0; i < 30; ++i) {  // Try for 30 seconds
            Thread::sleep(1000);
            if (isServerAvailable()) {
                serverAvailable = true;
                listeners.call(&Listener::serverConnected);
                return true;
            }
        }
        
        DBG("AI Server process started but not responding");
        serverProcess->kill();
        return false;
    }
    
    DBG("Failed to start AI Server process");
    return false;
}

void AIServerClient::stopServer() {
    if (serverProcess && serverProcess->isRunning()) {
        serverProcess->kill();
        serverAvailable = false;
        listeners.call(&Listener::serverDisconnected);
    }
}

bool AIServerClient::isServerProcessRunning() const {
    return serverProcess && serverProcess->isRunning();
}

void AIServerClient::run() {
    while (!shouldStop && !threadShouldExit()) {
        PendingRequest pending;
        bool hasRequest = false;
        
        {
            const ScopedLock sl(queueLock);
            if (!requestQueue.empty()) {
                pending = requestQueue.front();
                requestQueue.pop();
                hasRequest = true;
            }
        }
        
        if (hasRequest) {
            // Check timeout
            int64 elapsed = Time::currentTimeMillis() - pending.startTime;
            if (elapsed > pending.request.timeoutMs) {
                // Timeout occurred
                PresetResponse response;
                response.success = false;
                response.message = "Request timed out";
                response.responseTimeMs = (int)elapsed;
                
                if (pending.callback) {
                    pending.callback(response);
                }
                notifyRequestCompleted(pending.id, false);
            } else {
                // Process the request
                PresetResponse response = sendHttpRequest(pending.request);
                response.responseTimeMs = (int)(Time::currentTimeMillis() - pending.startTime);
                
                // Handle retry logic
                if (!response.success && shouldRetry(0) && pending.retryCount < maxRetries) {
                    pending.retryCount++;
                    Thread::sleep(retryDelayMs);
                    
                    {
                        const ScopedLock sl(queueLock);
                        requestQueue.push(pending);
                    }
                } else {
                    // Final response
                    if (pending.callback) {
                        pending.callback(response);
                    }
                    notifyRequestCompleted(pending.id, response.success);
                }
            }
        } else {
            // No requests, wait for notification
            wait(100);
        }
    }
}

AIServerClient::PresetResponse AIServerClient::sendHttpRequest(const PresetRequest& request) {
    PresetResponse response;
    
    try {
        URL generateUrl(serverUrl + "/generate");
        
        // Create JSON request body
        String jsonBody = createJsonRequest(request);
        
        // Setup HTTP POST request
        auto options = URL::InputStreamOptions(URL::ParameterHandling::inAddress)
            .withConnectionTimeoutMs(request.timeoutMs)
            .withHttpRequestCmd("POST")
            .withExtraHeaders("Content-Type: application/json")
            .withDataToPost(jsonBody);
        
        std::unique_ptr<InputStream> stream(generateUrl.createInputStream(options));
        
        if (stream) {
            String jsonResponse = stream->readEntireStreamAsString();
            
            // Parse response
            var parsedResponse = parseJsonResponse(jsonResponse);
            if (parsedResponse.isObject()) {
                response.success = parsedResponse["success"];
                response.message = parsedResponse["message"].toString();

                // Server returns data.preset structure
                if (parsedResponse.hasProperty("data")) {
                    var data = parsedResponse["data"];
                    if (data.isObject() && data.hasProperty("preset")) {
                        response.presetData = data["preset"];
                    }
                }
            } else {
                response.success = false;
                response.message = "Invalid JSON response from server";
            }
        } else {
            response.success = false;
            response.message = "Failed to connect to AI server";
        }
    } catch (const std::exception& e) {
        response.success = false;
        response.message = String("Exception: ") + e.what();
    }
    
    return response;
}

var AIServerClient::parseJsonResponse(const String& jsonString) {
    var result;
    Result parseResult = JSON::parse(jsonString, result);
    
    if (parseResult.failed()) {
        DBG("JSON parse error: " << parseResult.getErrorMessage());
        return var();
    }
    
    return result;
}

String AIServerClient::createJsonRequest(const PresetRequest& request) {
    DynamicObject::Ptr jsonObject = new DynamicObject();
    jsonObject->setProperty("prompt", request.prompt);
    
    // Add context if provided
    DynamicObject::Ptr contextObject = new DynamicObject();
    for (const auto& pair : request.context) {
        contextObject->setProperty(pair.first, pair.second);
    }
    jsonObject->setProperty("context", var(contextObject.get()));
    
    return JSON::toString(var(jsonObject.get()));
}

bool AIServerClient::shouldRetry(int httpStatusCode) const {
    // Retry on network errors or 5xx server errors
    return httpStatusCode == 0 || httpStatusCode >= 500;
}

String AIServerClient::generateRequestId() const {
    return String::toHexString(Random::getSystemRandom().nextInt64());
}

void AIServerClient::notifyRequestStarted(const String& requestId) {
    listeners.call(&Listener::requestStarted, requestId);
}

void AIServerClient::notifyRequestCompleted(const String& requestId, bool success) {
    listeners.call(&Listener::requestCompleted, requestId, success);
}

void AIServerClient::addListener(Listener* listener) {
    listeners.add(listener);
}

void AIServerClient::removeListener(Listener* listener) {
    listeners.remove(listener);
}