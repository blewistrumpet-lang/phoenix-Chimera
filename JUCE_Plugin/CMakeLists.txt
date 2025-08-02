cmake_minimum_required(VERSION 3.15)
project(ChimeraPhoenix VERSION 3.0.0)

# Add JUCE as a subdirectory (you'll need to download JUCE)
# add_subdirectory(JUCE)

# For now, let's create a basic structure for manual compilation
set(CMAKE_CXX_STANDARD 17)

# Define the plugin target
add_library(ChimeraPhoenix MODULE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/EngineFactory.cpp
    Source/DefaultParameterValues.cpp
    Source/CompleteEngineMetadata.cpp
    Source/EngineMetadataInit.cpp
    Source/KStyleOverdrive.cpp
    Source/TapeEcho.cpp
    Source/PlateReverb.cpp
    # Add other engine source files as needed
)

# Define the test executable
add_executable(QualityTestRunner
    Source/QualityTestRunner.cpp
    Source/EngineQualityTest.cpp
    Source/EngineFactory.cpp
    Source/DefaultParameterValues.cpp
    Source/CompleteEngineMetadata.cpp
    Source/EngineMetadataInit.cpp
    # Add all engine source files
    Source/KStyleOverdrive.cpp
    Source/TapeEcho.cpp
    Source/PlateReverb.cpp
    # Add other engine source files as needed
)

# Include directories
target_include_directories(ChimeraPhoenix PRIVATE
    Source
)

target_include_directories(QualityTestRunner PRIVATE
    Source
)

# macOS specific settings for AU
if(APPLE)
    set_target_properties(ChimeraPhoenix PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "component"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.chimera.phoenix"
        MACOSX_BUNDLE_BUNDLE_NAME "Chimera Phoenix"
        MACOSX_BUNDLE_BUNDLE_VERSION "3.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "3.0.0"
    )
endif()