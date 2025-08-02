#pragma once

#include "EngineTypes.h"

enum ParameterID {
    // Slot 1 Parameters
    SLOT_1_PARAM_1 = 0,
    SLOT_1_PARAM_2,
    SLOT_1_PARAM_3,
    SLOT_1_PARAM_4,
    SLOT_1_PARAM_5,
    SLOT_1_PARAM_6,
    SLOT_1_PARAM_7,
    SLOT_1_PARAM_8,
    SLOT_1_PARAM_9,
    SLOT_1_PARAM_10,
    
    // Slot 2 Parameters
    SLOT_2_PARAM_1,
    SLOT_2_PARAM_2,
    SLOT_2_PARAM_3,
    SLOT_2_PARAM_4,
    SLOT_2_PARAM_5,
    SLOT_2_PARAM_6,
    SLOT_2_PARAM_7,
    SLOT_2_PARAM_8,
    SLOT_2_PARAM_9,
    SLOT_2_PARAM_10,
    
    // Slot 3 Parameters
    SLOT_3_PARAM_1,
    SLOT_3_PARAM_2,
    SLOT_3_PARAM_3,
    SLOT_3_PARAM_4,
    SLOT_3_PARAM_5,
    SLOT_3_PARAM_6,
    SLOT_3_PARAM_7,
    SLOT_3_PARAM_8,
    SLOT_3_PARAM_9,
    SLOT_3_PARAM_10,
    
    // Slot 4 Parameters
    SLOT_4_PARAM_1,
    SLOT_4_PARAM_2,
    SLOT_4_PARAM_3,
    SLOT_4_PARAM_4,
    SLOT_4_PARAM_5,
    SLOT_4_PARAM_6,
    SLOT_4_PARAM_7,
    SLOT_4_PARAM_8,
    SLOT_4_PARAM_9,
    SLOT_4_PARAM_10,
    
    // Slot 5 Parameters
    SLOT_5_PARAM_1,
    SLOT_5_PARAM_2,
    SLOT_5_PARAM_3,
    SLOT_5_PARAM_4,
    SLOT_5_PARAM_5,
    SLOT_5_PARAM_6,
    SLOT_5_PARAM_7,
    SLOT_5_PARAM_8,
    SLOT_5_PARAM_9,
    SLOT_5_PARAM_10,
    
    // Slot 6 Parameters
    SLOT_6_PARAM_1,
    SLOT_6_PARAM_2,
    SLOT_6_PARAM_3,
    SLOT_6_PARAM_4,
    SLOT_6_PARAM_5,
    SLOT_6_PARAM_6,
    SLOT_6_PARAM_7,
    SLOT_6_PARAM_8,
    SLOT_6_PARAM_9,
    SLOT_6_PARAM_10,
    
    // Engine Selectors
    SLOT_1_ENGINE_SELECTOR,
    SLOT_2_ENGINE_SELECTOR,
    SLOT_3_ENGINE_SELECTOR,
    SLOT_4_ENGINE_SELECTOR,
    SLOT_5_ENGINE_SELECTOR,
    SLOT_6_ENGINE_SELECTOR,
    
    // Bypass Switches
    SLOT_1_BYPASS,
    SLOT_2_BYPASS,
    SLOT_3_BYPASS,
    SLOT_4_BYPASS,
    SLOT_5_BYPASS,
    SLOT_6_BYPASS,
    
    PARAMETER_COUNT
};

// Engine IDs are now defined in EngineTypes.h
// Special bypass value
#define ENGINE_BYPASS -1

// For backward compatibility, create EngineID enum that uses the unified definitions
enum EngineID {
    // Use the unified engine type definitions from EngineTypes.h
    BYPASS = ENGINE_BYPASS
    // All other engine types are defined in EngineTypes.h
};