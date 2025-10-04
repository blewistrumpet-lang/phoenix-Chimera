#include "AIServerManager.h"

AIServerManager::AIServerManager() : Thread("AIServerManager") {
    // Start the monitoring thread
    startThread();  // Start with default priority
}

AIServerManager::~AIServerManager() {
    shouldCheckHealth = false;
    
    // Stop monitoring thread
    stopThread(5000);
    
    // Stop server if running
    stopServer();
}

bool AIServerManager::startServerIfNeeded() {
    if (serverRunning && serverHealthy) {
        return true;  // Already running and healthy
    }
    
    DBG("AIServerManager: Checking if TRUE Trinity server needs to be started...");
    
    // First check if server is already running (maybe started externally)
    if (checkServerHealth()) {
        DBG("AIServerManager: Server already running and healthy!");
        serverRunning = true;
        serverHealthy = true;
        listeners.call(&Listener::serverStatusChanged, true, true);
        return true;
    }
    
    // Kill any zombie processes on port 8000
    killExistingServers();
    
    // Attempt to start the server
    return attemptServerStart();
}

void AIServerManager::stopServer() {
    const juce::ScopedLock sl(processLock);
    
    if (serverProcess && serverProcess->isRunning()) {
        DBG("AIServerManager: Stopping TRUE Trinity server...");
        serverProcess->kill();
        serverProcess.reset();
    }
    
    serverRunning = false;
    serverHealthy = false;
    listeners.call(&Listener::serverStatusChanged, false, false);
}

void AIServerManager::run() {
    // Background thread that monitors server health
    while (!threadShouldExit()) {
        if (shouldCheckHealth && serverRunning) {
            bool healthy = checkServerHealth();
            
            if (healthy != serverHealthy) {
                serverHealthy = healthy;
                
                if (!healthy) {
                    DBG("AIServerManager: Server became unhealthy, attempting restart...");
                    startServerIfNeeded();
                } else {
                    DBG("AIServerManager: Server is healthy");
                }
                
                listeners.call(&Listener::serverStatusChanged, serverRunning, serverHealthy);
            }
        }
        
        // Check every 5 seconds
        wait(5000);
    }
}

bool AIServerManager::checkServerHealth() {
    try {
        juce::URL healthUrl(getServerUrl() + "/health");
        
        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withConnectionTimeoutMs(2000)
            .withNumRedirectsToFollow(0);
        
        std::unique_ptr<juce::InputStream> stream(healthUrl.createInputStream(options));
        
        if (stream) {
            juce::String response = stream->readEntireStreamAsString();
            
            // Parse JSON to check it's the TRUE Trinity
            juce::var jsonResponse;
            if (juce::JSON::parse(response, jsonResponse).wasOk()) {
                auto components = jsonResponse["components"];
                
                // Verify it's the TRUE Trinity (oracle should be "removed")
                if (components.isObject()) {
                    juce::String oracleStatus = components["oracle"].toString();
                    juce::String corpusStatus = components["corpus"].toString();
                    
                    bool isTrueTrinity = (oracleStatus == "removed" && 
                                         corpusStatus == "not_needed");
                    
                    if (!isTrueTrinity) {
                        DBG("WARNING: Server is running but it's not the TRUE Trinity!");
                        DBG("  Oracle status: " << oracleStatus);
                        DBG("  Corpus status: " << corpusStatus);
                    }
                    
                    return response.contains("healthy") || response.contains("degraded");
                }
            }
        }
    } catch (...) {
        // Server not responding
    }
    
    return false;
}

bool AIServerManager::attemptServerStart() {
    const juce::ScopedLock sl(processLock);
    
    DBG("AIServerManager: Starting TRUE Trinity server...");
    
    // Find the AI_Server directory relative to the plugin
    juce::File pluginLocation = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    
    // Navigate to AI_Server directory
    // Try different possible paths
    juce::Array<juce::File> possiblePaths = {
        pluginLocation.getParentDirectory().getParentDirectory().getParentDirectory()
            .getChildFile("AI_Server"),
        pluginLocation.getParentDirectory().getParentDirectory()
            .getChildFile("Project_Chimera_v3.0_Phoenix/AI_Server"),
        juce::File("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server")
    };
    
    juce::File serverDir;
    juce::File mainPy;
    
    for (auto& path : possiblePaths) {
        mainPy = path.getChildFile("main.py");
        if (mainPy.exists()) {
            serverDir = path;
            DBG("Found AI_Server at: " << serverDir.getFullPathName());
            break;
        }
    }
    
    if (!mainPy.exists()) {
        DBG("ERROR: Cannot find main.py for TRUE Trinity server!");
        DBG("Searched in:");
        for (auto& path : possiblePaths) {
            DBG("  " << path.getFullPathName());
        }
        return false;
    }
    
    // Create the server process
    serverProcess = std::make_unique<juce::ChildProcess>();
    
    // Build the command to run the server
    juce::StringArray args;
    
    // Check if we have the API key
    juce::String apiKey = juce::SystemStats::getEnvironmentVariable("OPENAI_API_KEY", "");
    if (apiKey.isEmpty()) {
        DBG("WARNING: OPENAI_API_KEY not set! AI generation will be limited.");
    }
    
    // Python command with proper module execution
    juce::String pythonCmd = "python3";
    
    // Check if python3 exists
    if (juce::File("/usr/bin/python3").exists() || 
        juce::File("/usr/local/bin/python3").exists() ||
        juce::File("/opt/homebrew/bin/python3").exists()) {
        args.add(pythonCmd);
    } else {
        args.add("python");  // Fallback to python
    }
    
    // Use uvicorn to run the FastAPI server
    args.add("-m");
    args.add("uvicorn");
    args.add("main:app");
    args.add("--host");
    args.add("0.0.0.0");
    args.add("--port");
    args.add("8000");
    args.add("--log-level");
    args.add("warning");  // Reduce log verbosity
    
    DBG("Starting server with command: " << args.joinIntoString(" "));
    
    // Start the process
    if (serverProcess->start(args)) {
        DBG("Server process started, waiting for it to be ready...");
        
        // Wait for server to be ready (max 15 seconds)
        for (int i = 0; i < 15; ++i) {
            juce::Thread::sleep(1000);
            
            if (checkServerHealth()) {
                DBG("TRUE Trinity server is ready!");
                serverRunning = true;
                serverHealthy = true;
                listeners.call(&Listener::serverStatusChanged, true, true);
                return true;
            }
            
            // Check if process died
            if (!serverProcess->isRunning()) {
                DBG("Server process died unexpectedly!");
                break;
            }
        }
        
        // Server didn't respond in time
        DBG("Server started but not responding, killing process...");
        serverProcess->kill();
        serverProcess.reset();
    } else {
        DBG("Failed to start server process!");
        DBG("Working directory: " << serverDir.getFullPathName());
    }
    
    return false;
}

void AIServerManager::killExistingServers() {
    DBG("Checking for existing processes on port 8000...");
    
    // Kill any existing Python processes on port 8000
    #if JUCE_MAC || JUCE_LINUX
        // Use lsof to find process using port 8000
        juce::ChildProcess killProcess;
        juce::StringArray killArgs;
        
        // First, find the PID
        killArgs.add("sh");
        killArgs.add("-c");
        killArgs.add("lsof -ti:8000 | xargs -r kill -9 2>/dev/null || true");
        
        killProcess.start(killArgs);
        killProcess.waitForProcessToFinish(2000);
        
        // Give it a moment to clean up
        juce::Thread::sleep(500);
    #elif JUCE_WINDOWS
        // Windows command to kill process on port 8000
        juce::ChildProcess killProcess;
        juce::StringArray killArgs;
        
        killArgs.add("cmd");
        killArgs.add("/c");
        killArgs.add("for /f \"tokens=5\" %a in ('netstat -aon ^| findstr :8000') do taskkill /F /PID %a");
        
        killProcess.start(killArgs);
        killProcess.waitForProcessToFinish(2000);
    #endif
}