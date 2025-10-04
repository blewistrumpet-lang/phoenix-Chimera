// SlotConfiguration.h - Flexible Slot Architecture Configuration
#pragma once

namespace ChimeraConfig {
    // Current slot configuration
    static constexpr int NUM_SLOTS = 6;
    
    // Maximum slots supported (for future expansion)
    static constexpr int MAX_SLOTS_SUPPORTED = 8;
    
    // Slot names for UI
    static constexpr const char* SLOT_NAMES[MAX_SLOTS_SUPPORTED] = {
        "Slot 1",
        "Slot 2", 
        "Slot 3",
        "Slot 4",
        "Slot 5",
        "Slot 6",
        "Slot 7",  // Reserved for future
        "Slot 8"   // Reserved for future
    };
    
    // CPU threshold for automatic slot limiting
    static constexpr float CPU_THRESHOLD_WARNING = 70.0f;    // Warn at 70% CPU
    static constexpr float CPU_THRESHOLD_CRITICAL = 85.0f;   // Critical at 85% CPU
    
    // Configuration flags
    struct SlotConfig {
        bool enableDynamicSlotCount = false;      // Allow runtime slot adjustment
        bool enableSlotBypass = true;             // Allow bypassing individual slots
        bool enableParallelProcessing = false;    // Future: parallel slot chains
        int defaultActiveSlots = NUM_SLOTS;       // Default number of active slots
    };
    
    // Helper to check if slot index is valid
    inline bool isValidSlot(int slotIndex) {
        return slotIndex >= 0 && slotIndex < NUM_SLOTS;
    }
    
    // Helper to check if slot index is valid for future expansion
    inline bool isValidExpandedSlot(int slotIndex) {
        return slotIndex >= 0 && slotIndex < MAX_SLOTS_SUPPORTED;
    }
}

// Macro for easy migration if slot count changes
#define CHIMERA_NUM_SLOTS ChimeraConfig::NUM_SLOTS
#define CHIMERA_MAX_SLOTS ChimeraConfig::MAX_SLOTS_SUPPORTED