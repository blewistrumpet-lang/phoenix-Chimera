#pragma once

#include "JuceHeader.h"
#include <queue>
#include <mutex>
#include <functional>
#include <vector>

/**
 * Simple Event Bus for routing hardware events to parameter changes
 * Thread-safe for posting from GPIO thread and processing on message thread
 */
class EventBus
{
public:
    // Event types
    enum class EventType {
        ENCODER_TURN,
        ENCODER_PRESS,
        SWITCH_CHANGE,
        MODE_CHANGE,
        VARIANT_CHANGE
    };

    // Event structure
    struct Event {
        EventType type;
        int deviceIndex;    // 0-2 for encoders/switches
        float value;        // Delta for encoders, position for switches
        int intValue;       // For discrete values
        bool boolValue;     // For button presses

        Event() = default;
        Event(EventType t, int device, float val)
            : type(t), deviceIndex(device), value(val), intValue(0), boolValue(false) {}
        Event(EventType t, int device, int val)
            : type(t), deviceIndex(device), value(0), intValue(val), boolValue(false) {}
        Event(EventType t, int device, bool val)
            : type(t), deviceIndex(device), value(0), intValue(0), boolValue(val) {}
    };

    // Event handler callback
    using EventHandler = std::function<void(const Event&)>;

    EventBus() = default;
    ~EventBus() = default;

    // Thread-safe event posting (from GPIO thread)
    void postEvent(const Event& event) {
        std::lock_guard<std::mutex> lock(queueMutex);
        eventQueue.push(event);
    }

    // Process all pending events (call from message thread)
    void processEvents() {
        std::queue<Event> eventsToProcess;

        // Quick swap under lock
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            eventsToProcess.swap(eventQueue);
        }

        // Process events without holding lock
        while (!eventsToProcess.empty()) {
            const Event& event = eventsToProcess.front();

            // Call all registered handlers for this event type
            auto it = handlers.find(event.type);
            if (it != handlers.end()) {
                for (const auto& handler : it->second) {
                    handler(event);
                }
            }

            eventsToProcess.pop();
        }
    }

    // Subscribe to events of a specific type
    void subscribe(EventType type, EventHandler handler) {
        handlers[type].push_back(handler);
    }

    // Clear all handlers (useful for cleanup)
    void clearHandlers() {
        handlers.clear();
    }

    // Get queue size (for debugging)
    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(queueMutex);
        return eventQueue.size();
    }

private:
    mutable std::mutex queueMutex;
    std::queue<Event> eventQueue;
    std::map<EventType, std::vector<EventHandler>> handlers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EventBus)
};