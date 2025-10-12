#include "FileExchangeClient.h"

FileExchangeClient::FileExchangeClient()
{
    // Set up exchange directory
    auto homeDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    exchangeDir = homeDir.getChildFile(".chimera_phoenix").getChildFile("preset_exchange");
    pendingDir = exchangeDir.getChildFile("pending");
    processedDir = exchangeDir.getChildFile("processed");
    
    // Ensure directories exist
    exchangeDir.createDirectory();
    pendingDir.createDirectory();
    processedDir.createDirectory();
    
    DBG("FileExchangeClient initialized with exchange dir: " << exchangeDir.getFullPathName());
}

FileExchangeClient::~FileExchangeClient()
{
    stopMonitoring();
}

void FileExchangeClient::initialize(const juce::String& sessionId)
{
    currentSessionId = sessionId;
    DBG("FileExchangeClient initialized with session: " << sessionId);
    
    // Clean up any old markers from previous sessions
    cleanupOldMarkers();
}

void FileExchangeClient::startMonitoring()
{
    if (!isMonitoring.exchange(true))
    {
        DBG("Starting file exchange monitoring for session: " << currentSessionId);
        startTimer(500); // Check every 500ms for responsiveness
    }
}

void FileExchangeClient::stopMonitoring()
{
    if (isMonitoring.exchange(false))
    {
        DBG("Stopping file exchange monitoring");
        stopTimer();
    }
}

void FileExchangeClient::timerCallback()
{
    checkForPresets();
}

bool FileExchangeClient::checkForPresets()
{
    if (currentSessionId.isEmpty())
    {
        DBG("FileExchangeClient: No session ID set");
        return false;
    }
    
    // Look for marker file for this session
    juce::String markerName = currentSessionId + "_READY.marker";
    juce::File markerFile = pendingDir.getChildFile(markerName);
    
    if (markerFile.exists())
    {
        DBG("Found marker file: " << markerFile.getFullPathName());
        
        try
        {
            // Read marker file to get preset file path
            auto markerJson = juce::JSON::parse(markerFile.loadFileAsString());
            if (markerJson.isObject())
            {
                juce::String exchangeId = markerJson.getProperty("exchange_id", "").toString();
                juce::String presetFilePath = markerJson.getProperty("preset_file", "").toString();
                
                if (exchangeId.isNotEmpty() && presetFilePath.isNotEmpty())
                {
                    // Check if we've already processed this
                    {
                        std::lock_guard<std::mutex> lock(processedIdsMutex);
                        if (processedIds.find(exchangeId) != processedIds.end())
                        {
                            DBG("Already processed exchange: " << exchangeId);
                            // Delete the marker since we've processed it
                            markerFile.deleteFile();
                            return false;
                        }
                    }
                    
                    juce::File presetFile(presetFilePath);
                    if (presetFile.exists())
                    {
                        DBG("Found preset file: " << presetFile.getFullPathName());
                        processExchangeFile(presetFile);
                        
                        // Mark as processed
                        {
                            std::lock_guard<std::mutex> lock(processedIdsMutex);
                            processedIds.insert(exchangeId);
                        }
                        
                        // Delete the marker file after successful processing
                        markerFile.deleteFile();
                        
                        // Move preset file to processed directory
                        juce::File processedFile = processedDir.getChildFile(presetFile.getFileName());
                        presetFile.moveFileTo(processedFile);
                        
                        processedCount++;
                        return true;
                    }
                    else
                    {
                        DBG("Preset file not found: " << presetFilePath);
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            DBG("Error processing marker file: " << e.what());
            notifyError("Failed to process marker file: " + juce::String(e.what()));
        }
    }
    
    // Update pending count
    auto pendingFiles = pendingDir.findChildFiles(juce::File::findFiles, false, "*.json");
    pendingCount = pendingFiles.size();
    
    return false;
}

void FileExchangeClient::processExchangeFile(const juce::File& file)
{
    DBG("Processing exchange file: " << file.getFileName());
    
    try
    {
        auto exchangeJson = juce::JSON::parse(file.loadFileAsString());
        
        if (exchangeJson.isObject())
        {
            juce::String exchangeId = exchangeJson.getProperty("id", "").toString();
            juce::String sessionId = exchangeJson.getProperty("session_id", "").toString();
            juce::String presetName = exchangeJson.getProperty("preset_name", "").toString();
            juce::var presetData = exchangeJson.getProperty("preset_data", juce::var());
            
            DBG("Exchange ID: " << exchangeId);
            DBG("Session ID: " << sessionId);
            DBG("Preset Name: " << presetName);
            
            if (sessionId == currentSessionId && presetData.isObject())
            {
                DBG("Valid preset for current session, notifying listeners");
                
                // Wrap the preset data in the expected format
                juce::DynamicObject::Ptr response = new juce::DynamicObject();
                response->setProperty("success", true);
                response->setProperty("type", "preset");
                
                juce::DynamicObject::Ptr data = new juce::DynamicObject();
                data->setProperty("preset", presetData);
                data->setProperty("exchange_id", exchangeId);
                response->setProperty("data", juce::var(data.get()));
                
                notifyPresetReceived(juce::var(response.get()));
            }
            else
            {
                DBG("Preset not for current session or invalid data");
            }
        }
    }
    catch (const std::exception& e)
    {
        DBG("Error processing exchange file: " << e.what());
        notifyError("Failed to process preset file: " + juce::String(e.what()));
    }
}

void FileExchangeClient::acknowledgePreset(const juce::String& exchangeId)
{
    DBG("Acknowledging preset: " << exchangeId);
    
    // The preset should already be moved to processed directory
    // This is just for tracking/logging
    
    // Could send acknowledgment to server if needed
    // For now, just log it
    DBG("Preset acknowledged: " << exchangeId);
}

void FileExchangeClient::notifyPresetReceived(const juce::var& presetData)
{
    std::lock_guard<std::mutex> lock(listenersMutex);
    for (auto* listener : listeners)
    {
        listener->onPresetReceived(presetData);
    }
}

void FileExchangeClient::notifyError(const juce::String& error)
{
    std::lock_guard<std::mutex> lock(listenersMutex);
    for (auto* listener : listeners)
    {
        listener->onExchangeError(error);
    }
}

void FileExchangeClient::addListener(Listener* listener)
{
    std::lock_guard<std::mutex> lock(listenersMutex);
    listeners.push_back(listener);
}

void FileExchangeClient::removeListener(Listener* listener)
{
    std::lock_guard<std::mutex> lock(listenersMutex);
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void FileExchangeClient::cleanupOldMarkers()
{
    // Clean up old marker files from previous sessions
    auto markers = pendingDir.findChildFiles(juce::File::findFiles, false, "*.marker");
    
    for (const auto& marker : markers)
    {
        // Don't delete our current session's marker
        if (!marker.getFileName().startsWith(currentSessionId))
        {
            // Check age - delete if older than 5 minutes
            auto age = juce::Time::getCurrentTime() - marker.getLastModificationTime();
            if (age.inMinutes() > 5)
            {
                DBG("Cleaning up old marker: " << marker.getFileName());
                marker.deleteFile();
            }
        }
    }
}

juce::File FileExchangeClient::getExchangeDirectory() const
{
    return exchangeDir;
}

int FileExchangeClient::getPendingCount() const
{
    return pendingCount.load();
}

int FileExchangeClient::getProcessedCount() const
{
    return processedCount.load();
}