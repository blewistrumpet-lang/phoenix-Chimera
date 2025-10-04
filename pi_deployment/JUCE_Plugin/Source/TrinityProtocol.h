#pragma once

#include <JuceHeader.h>

/**
 * TrinityProtocol - Message format definitions and protocol constants for Trinity AI
 * Defines the communication protocol between the plugin and Trinity cloud services
 */
namespace TrinityProtocol {
    
    // === PROTOCOL VERSION ===
    static constexpr int PROTOCOL_VERSION = 1;
    
    // === MESSAGE TYPES ===
    namespace MessageType {
        static const juce::String QUERY = "query";
        static const juce::String PLUGIN_STATE = "plugin_state";
        static const juce::String PARAMETER_CHANGE = "parameter_change";
        static const juce::String PRESET_REQUEST = "preset_request";
        static const juce::String START_SESSION = "start_session";
        static const juce::String END_SESSION = "end_session";
        static const juce::String HEARTBEAT = "heartbeat";
    }
    
    namespace ResponseType {
        static const juce::String RESPONSE = "response";
        static const juce::String SUGGESTION = "suggestion";
        static const juce::String PRESET = "preset";
        static const juce::String PARAMETER_UPDATE = "parameter_update";
        static const juce::String ERROR = "error";
        static const juce::String ACKNOWLEDGMENT = "ack";
    }
    
    // === SESSION TYPES ===
    namespace SessionType {
        static const juce::String SOUND_DESIGN = "sound_design";
        static const juce::String MIXING = "mixing";
        static const juce::String MASTERING = "mastering";
        static const juce::String CREATIVE = "creative";
        static const juce::String LEARNING = "learning";
    }
    
    // === PARAMETER CATEGORIES ===
    namespace ParameterCategory {
        static const juce::String FILTER = "filter";
        static const juce::String DISTORTION = "distortion";
        static const juce::String MODULATION = "modulation";
        static const juce::String DELAY = "delay";
        static const juce::String REVERB = "reverb";
        static const juce::String DYNAMICS = "dynamics";
        static const juce::String PITCH = "pitch";
        static const juce::String UTILITY = "utility";
    }
    
    // === MESSAGE STRUCTURES ===
    
    /**
     * Creates a standardized query message
     */
    static juce::var createQueryMessage(const juce::String& query, 
                                  const juce::String& sessionId,
                                  const juce::var& context = juce::var()) {
        juce::DynamicObject::Ptr msg = new juce::DynamicObject();
        msg->setProperty("type", MessageType::QUERY);
        msg->setProperty("content", query);
        msg->setProperty("session_id", sessionId);
        msg->setProperty("timestamp", juce::Time::currentTimeMillis());
        msg->setProperty("protocol_version", PROTOCOL_VERSION);
        
        if (!context.isVoid()) {
            msg->setProperty("context", context);
        }
        
        return juce::var(msg.get());
    }
    
    /**
     * Creates a plugin state message with current parameter values
     */
    static juce::var createPluginStateMessage(const juce::String& sessionId,
                                        const juce::Array<juce::var>& slotStates,
                                        const juce::var& globalState = juce::var()) {
        juce::DynamicObject::Ptr msg = new juce::DynamicObject();
        msg->setProperty("type", MessageType::PLUGIN_STATE);
        msg->setProperty("session_id", sessionId);
        msg->setProperty("timestamp", juce::Time::currentTimeMillis());
        msg->setProperty("protocol_version", PROTOCOL_VERSION);
        
        // Slot information
        juce::Array<juce::var> slots;
        for (const auto& slot : slotStates) {
            slots.add(slot);
        }
        msg->setProperty("slots", slots);
        
        if (!globalState.isVoid()) {
            msg->setProperty("global_state", globalState);
        }
        
        return juce::var(msg.get());
    }
    
    /**
     * Creates a parameter change notification
     */
    static juce::var createParameterChangeMessage(const juce::String& sessionId,
                                           int slotIndex,
                                           const juce::String& parameterName,
                                           float newValue,
                                           const juce::String& category = "") {
        juce::DynamicObject::Ptr msg = new juce::DynamicObject();
        msg->setProperty("type", MessageType::PARAMETER_CHANGE);
        msg->setProperty("session_id", sessionId);
        msg->setProperty("timestamp", juce::Time::currentTimeMillis());
        msg->setProperty("protocol_version", PROTOCOL_VERSION);
        
        juce::DynamicObject::Ptr paramData = new juce::DynamicObject();
        paramData->setProperty("slot_index", slotIndex);
        paramData->setProperty("parameter_name", parameterName);
        paramData->setProperty("value", newValue);
        if (!category.isEmpty()) {
            paramData->setProperty("category", category);
        }
        
        msg->setProperty("parameter", juce::var(paramData.get()));
        
        return juce::var(msg.get());
    }
    
    /**
     * Creates a preset request message
     */
    static juce::var createPresetRequestMessage(const juce::String& sessionId,
                                         const juce::String& description,
                                         const juce::String& genre = "",
                                         const juce::String& mood = "") {
        juce::DynamicObject::Ptr msg = new juce::DynamicObject();
        msg->setProperty("type", MessageType::PRESET_REQUEST);
        msg->setProperty("content", description);
        msg->setProperty("session_id", sessionId);
        msg->setProperty("timestamp", juce::Time::currentTimeMillis());
        msg->setProperty("protocol_version", PROTOCOL_VERSION);
        
        juce::DynamicObject::Ptr metadata = new juce::DynamicObject();
        if (!genre.isEmpty()) metadata->setProperty("genre", genre);
        if (!mood.isEmpty()) metadata->setProperty("mood", mood);
        
        if (metadata->getProperties().size() > 0) {
            msg->setProperty("metadata", juce::var(metadata.get()));
        }
        
        return juce::var(msg.get());
    }
    
    /**
     * Creates a session start message
     */
    static juce::var createSessionStartMessage(const juce::String& sessionId,
                                        const juce::String& sessionType = SessionType::SOUND_DESIGN) {
        juce::DynamicObject::Ptr msg = new juce::DynamicObject();
        msg->setProperty("type", MessageType::START_SESSION);
        msg->setProperty("content", sessionType);
        msg->setProperty("session_id", sessionId);
        msg->setProperty("timestamp", juce::Time::currentTimeMillis());
        msg->setProperty("protocol_version", PROTOCOL_VERSION);
        
        juce::DynamicObject::Ptr sessionData = new juce::DynamicObject();
        sessionData->setProperty("session_type", sessionType);
        sessionData->setProperty("plugin_version", "Chimera Phoenix v3.0");
        sessionData->setProperty("client_id", juce::SystemStats::getComputerName());
        
        msg->setProperty("data", juce::var(sessionData.get()));
        
        return juce::var(msg.get());
    }
    
    // === RESPONSE PARSING HELPERS ===
    
    /**
     * Checks if a response contains parameter suggestions
     */
    static bool hasParameterSuggestions(const juce::var& response) {
        if (!response.isObject()) return false;
        
        juce::var data = response.getProperty("data", juce::var());
        if (!data.isObject()) return false;
        
        return data.hasProperty("parameter_suggestions");
    }
    
    /**
     * Extracts parameter suggestions from a response
     */
    static juce::Array<juce::var> getParameterSuggestions(const juce::var& response) {
        juce::Array<juce::var> suggestions;
        
        if (!hasParameterSuggestions(response)) return suggestions;
        
        juce::var data = response.getProperty("data", juce::var());
        juce::var suggestionsData = data.getProperty("parameter_suggestions", juce::var());
        
        if (suggestionsData.isArray()) {
            for (int i = 0; i < suggestionsData.size(); ++i) {
                suggestions.add(suggestionsData[i]);
            }
        }
        
        return suggestions;
    }
    
    /**
     * Checks if a response contains a preset
     * NOTE: This receives response.data directly, not the full response!
     */
    static bool hasPresetData(const juce::var& data) {
        if (!data.isObject()) return false;
        return data.hasProperty("preset");
    }

    /**
     * Extracts preset data from a response
     * NOTE: This receives response.data directly, not the full response!
     */
    static juce::var getPresetData(const juce::var& data) {
        if (!hasPresetData(data)) return juce::var();
        return data.getProperty("preset", juce::var());
    }
    
    /**
     * Creates a slot state object for plugin state messages
     */
    static juce::var createSlotState(int slotIndex,
                              int engineId,
                              const juce::String& engineName,
                              bool bypassed,
                              bool soloed,
                              const juce::Array<juce::var>& parameters) {
        juce::DynamicObject::Ptr slot = new juce::DynamicObject();
        slot->setProperty("index", slotIndex);
        slot->setProperty("engine_id", engineId);
        slot->setProperty("engine_name", engineName);
        slot->setProperty("bypassed", bypassed);
        slot->setProperty("soloed", soloed);
        
        juce::Array<juce::var> params;
        for (const auto& param : parameters) {
            params.add(param);
        }
        slot->setProperty("parameters", params);
        
        return juce::var(slot.get());
    }
    
    /**
     * Creates a parameter object for slot states
     */
    static juce::var createParameter(const juce::String& name,
                              float value,
                              float defaultValue,
                              const juce::String& category = "",
                              const juce::String& unit = "") {
        juce::DynamicObject::Ptr param = new juce::DynamicObject();
        param->setProperty("name", name);
        param->setProperty("value", value);
        param->setProperty("default_value", defaultValue);
        
        if (!category.isEmpty()) param->setProperty("category", category);
        if (!unit.isEmpty()) param->setProperty("unit", unit);
        
        return juce::var(param.get());
    }
    
    // === VALIDATION ===
    
    /**
     * Validates a message structure
     */
    static bool isValidMessage(const juce::var& message) {
        if (!message.isObject()) return false;
        
        // Check required fields
        if (!message.hasProperty("type")) return false;
        if (!message.hasProperty("timestamp")) return false;
        if (!message.hasProperty("protocol_version")) return false;
        
        // Check protocol version
        int version = message.getProperty("protocol_version", 0);
        if (version != PROTOCOL_VERSION) return false;
        
        return true;
    }
    
    /**
     * Validates a response structure
     */
    static bool isValidResponse(const juce::var& response) {
        if (!response.isObject()) return false;
        
        // Check required fields
        if (!response.hasProperty("success")) return false;
        if (!response.hasProperty("type")) return false;
        
        return true;
    }
}