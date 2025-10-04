/*
    AppConfig.h for Chimera Phoenix Engine Tests
    
    This file defines the JUCE configuration for standalone test applications.
    It enables necessary JUCE modules and sets up build configuration.
    
    This serves as the global header that JUCE modules require.
*/

#pragma once

// Define that this is the global header file
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1

// Build configuration
#define DEBUG 1
#define JUCE_STANDALONE_APPLICATION 1

// JUCE module configuration
#define JUCE_STRICT_REFCOUNTEDPOINTER 1
#define JUCE_MODULE_AVAILABLE_juce_core 1
#define JUCE_MODULE_AVAILABLE_juce_data_structures 1
#define JUCE_MODULE_AVAILABLE_juce_events 1
#define JUCE_MODULE_AVAILABLE_juce_graphics 1
#define JUCE_MODULE_AVAILABLE_juce_gui_basics 1
#define JUCE_MODULE_AVAILABLE_juce_gui_extra 1
#define JUCE_MODULE_AVAILABLE_juce_audio_basics 1
#define JUCE_MODULE_AVAILABLE_juce_audio_devices 1
#define JUCE_MODULE_AVAILABLE_juce_audio_formats 1
#define JUCE_MODULE_AVAILABLE_juce_audio_processors 1
#define JUCE_MODULE_AVAILABLE_juce_audio_utils 1
#define JUCE_MODULE_AVAILABLE_juce_dsp 1

// Disable unused modules to speed compilation
#define JUCE_MODULE_AVAILABLE_juce_analytics 0
#define JUCE_MODULE_AVAILABLE_juce_animation 0
#define JUCE_MODULE_AVAILABLE_juce_box2d 0
#define JUCE_MODULE_AVAILABLE_juce_cryptography 0
#define JUCE_MODULE_AVAILABLE_juce_javascript 0
#define JUCE_MODULE_AVAILABLE_juce_midi_ci 0
#define JUCE_MODULE_AVAILABLE_juce_opengl 0
#define JUCE_MODULE_AVAILABLE_juce_osc 0
#define JUCE_MODULE_AVAILABLE_juce_product_unlocking 0
#define JUCE_MODULE_AVAILABLE_juce_video 0

// Audio processing configuration
#define JUCE_USE_FLAC 1
#define JUCE_USE_OGGVORBIS 1
#define JUCE_USE_MP3AUDIOFORMAT 1
#define JUCE_USE_LAME_AUDIO_FORMAT 0
#define JUCE_USE_WINDOWS_MEDIA_FORMAT 0

// DSP configuration
#define JUCE_DSP_USE_INTEL_MKL 0
#define JUCE_DSP_USE_SHARED_FFTW 0
#define JUCE_DSP_USE_STATIC_FFTW 0
#define JUCE_DSP_ENABLE_SNAP_TO_ZERO 1

// Audio device configuration
#define JUCE_USE_WINRT_MIDI 0
#define JUCE_USE_DIRECTSHOW 0
#define JUCE_USE_MEDIAFOUNDATION 0

// Graphics configuration  
#define JUCE_USE_COREIMAGE_LOADER 1
#define JUCE_USE_DIRECTWRITE 0
#define JUCE_DISABLE_COREGRAPHICS_FONT_SMOOTHING 0

// Network configuration
#define JUCE_USE_CURL 0

// Optimization settings
#define JUCE_CHECK_MEMORY_LEAKS 1
#define JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES 0
#define JUCE_INCLUDE_ZLIB_CODE 1
#define JUCE_USE_XSHM 1
#define JUCE_USE_XRENDER 1
#define JUCE_USE_XCURSOR 1

// Compiler warnings
#define JUCE_SILENCE_XCODE_15_LINKER_WARNING 1

// Application info
#define JucePlugin_Name "Chimera Phoenix Test"
#define JucePlugin_Manufacturer "Chimera Audio"
#define JucePlugin_Version "1.0.0"