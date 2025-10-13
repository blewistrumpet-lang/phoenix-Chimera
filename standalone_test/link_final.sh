#!/bin/bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

# Collect all object files
OBJS=$(find build/obj -name "*.o" | sort)

# Link
clang++ -std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable \
    $OBJS \
    -framework Accelerate \
    -framework CoreAudio \
    -framework CoreFoundation \
    -framework AudioToolbox \
    -framework Cocoa \
    -framework IOKit \
    -framework Security \
    -framework QuartzCore \
    -framework CoreImage \
    -framework CoreGraphics \
    -framework CoreText \
    -framework WebKit \
    -framework DiscRecording \
    -L/opt/homebrew/lib -lharfbuzz \
    -o build/standalone_test

echo "Link complete: $?"
ls -lh build/standalone_test
