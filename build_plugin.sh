#!/bin/bash

echo "Project Chimera Phoenix - Build Script"
echo "======================================"

# Check if we need to download JUCE
if [ ! -d "JUCE" ]; then
    echo "JUCE framework not found. Please:"
    echo "1. Download JUCE from https://juce.com/get-juce/download"
    echo "2. Extract it to this directory as 'JUCE'"
    echo "3. Run this script again"
    exit 1
fi

# Check for Projucer
PROJUCER_PATH=""
if [ -f "JUCE/Projucer.app/Contents/MacOS/Projucer" ]; then
    PROJUCER_PATH="JUCE/Projucer.app/Contents/MacOS/Projucer"
elif [ -f "/Applications/JUCE/Projucer.app/Contents/MacOS/Projucer" ]; then
    PROJUCER_PATH="/Applications/JUCE/Projucer.app/Contents/MacOS/Projucer"
else
    echo "Projucer not found. Please ensure JUCE is properly installed."
    exit 1
fi

cd JUCE_Plugin

# Generate Xcode project
echo "Generating Xcode project..."
"$PROJUCER_PATH" --resave ChimeraPhoenix.jucer

# Build with xcodebuild
echo "Building AU plugin..."
cd Builds/MacOSX
xcodebuild -project ChimeraPhoenix.xcodeproj -configuration Release -target "ChimeraPhoenix - AU"

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo "Build successful!"
    
    # Copy to Audio Unit folder
    echo "Installing AU plugin..."
    AU_PATH="$HOME/Library/Audio/Plug-Ins/Components/"
    mkdir -p "$AU_PATH"
    cp -R build/Release/ChimeraPhoenix.component "$AU_PATH"
    
    echo "Running auval validation..."
    auval -v aufx Chmr Chim
    
    if [ $? -eq 0 ]; then
        echo "Validation successful!"
        echo "Opening Logic Pro..."
        open -a "Logic Pro"
    else
        echo "AU validation failed. Check the output above for errors."
    fi
else
    echo "Build failed. Check the output above for errors."
fi