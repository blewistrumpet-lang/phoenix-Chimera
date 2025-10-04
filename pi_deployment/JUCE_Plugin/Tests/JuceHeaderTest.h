/*
    JUCE Header for Test Compilation
    
    This header uses the correct paths to the local JUCE installation
    for compiling standalone test applications.
    
    Unlike the auto-generated JuceHeader.h which expects system-installed
    JUCE headers, this version points to the local JUCE modules.
*/

#pragma once

// Include AppConfig first - this is required by JUCE modules
#include "AppConfig.h"

// Use relative paths to the local JUCE modules
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../JUCE/modules/juce_audio_devices/juce_audio_devices.h"
#include "../../JUCE/modules/juce_audio_formats/juce_audio_formats.h"
#include "../../JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "../../JUCE/modules/juce_audio_utils/juce_audio_utils.h"
#include "../../JUCE/modules/juce_data_structures/juce_data_structures.h"
#include "../../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../../JUCE/modules/juce_events/juce_events.h"
#include "../../JUCE/modules/juce_graphics/juce_graphics.h"
#include "../../JUCE/modules/juce_gui_basics/juce_gui_basics.h"
#include "../../JUCE/modules/juce_gui_extra/juce_gui_extra.h"

// Project Info for tests
#if ! JUCE_DONT_DECLARE_PROJECTINFO
namespace ProjectInfo
{
    const char* const  projectName    = "ChimeraPhoenix";
    const char* const  companyName    = "Chimera Audio";
    const char* const  versionString  = "1.0.0";
    const int          versionNumber  = 0x10000;
}
#endif