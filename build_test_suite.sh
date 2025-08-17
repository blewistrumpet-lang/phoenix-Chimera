#!/bin/bash

# Build script for comprehensive engine test suite

echo "Building Comprehensive Engine Test Suite..."

# Set paths
JUCE_PATH="/Users/Branden/JUCE/modules"
PLUGIN_SOURCE="JUCE_Plugin/Source"
BUILD_DIR="build_tests"

# Create build directory
mkdir -p $BUILD_DIR

# Compile flags
CXX="g++"
CXXFLAGS="-std=c++17 -O2 -DDEBUG=1"
INCLUDES="-I$JUCE_PATH -I$PLUGIN_SOURCE -IJUCE_Plugin/JuceLibraryCode"
FRAMEWORKS="-framework CoreAudio -framework CoreFoundation -framework Accelerate"
FRAMEWORKS="$FRAMEWORKS -framework AudioToolbox -framework AudioUnit -framework CoreAudioKit"
FRAMEWORKS="$FRAMEWORKS -framework CoreMIDI -framework Cocoa -framework Carbon"
FRAMEWORKS="$FRAMEWORKS -framework QuartzCore -framework IOKit -framework Security"
FRAMEWORKS="$FRAMEWORKS -framework WebKit -framework Metal -framework MetalKit"

DEFINES="-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
DEFINES="$DEFINES -DJUCE_MODULE_AVAILABLE_juce_core=1"
DEFINES="$DEFINES -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1"
DEFINES="$DEFINES -DJUCE_MODULE_AVAILABLE_juce_audio_devices=1"
DEFINES="$DEFINES -DJUCE_MODULE_AVAILABLE_juce_audio_formats=1"
DEFINES="$DEFINES -DJUCE_MODULE_AVAILABLE_juce_audio_processors=1"
DEFINES="$DEFINES -DJUCE_MODULE_AVAILABLE_juce_audio_utils=1"
DEFINES="$DEFINES -DJUCE_MODULE_AVAILABLE_juce_dsp=1"
DEFINES="$DEFINES -DJUCE_STANDALONE_APPLICATION=1"

echo "Compiling test runners..."

# Compile test suite
$CXX $CXXFLAGS $INCLUDES $DEFINES \
    engine_test_runner.cpp \
    comprehensive_engine_test_suite.cpp \
    -o $BUILD_DIR/engine_test_suite \
    $FRAMEWORKS \
    JUCE_Plugin/Builds/MacOSX/build/Debug/libChimeraPhoenix.a \
    2>&1 | tee $BUILD_DIR/build.log

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo ""
    echo "Test suite executables created in $BUILD_DIR/"
    echo ""
    echo "Usage:"
    echo "  ./$BUILD_DIR/engine_test_suite --all              # Test all 56 engines"
    echo "  ./$BUILD_DIR/engine_test_suite --engine SpringReverb  # Test single engine"
    echo "  ./$BUILD_DIR/engine_test_suite --category reverb      # Test category"
    echo ""
    echo "Categories: reverb, pitch, eq, dynamics, delay, distortion, spatial"
else
    echo "❌ Build failed. Check $BUILD_DIR/build.log for details"
    exit 1
fi

# Create test runner script
cat > run_tests.sh << 'EOF'
#!/bin/bash

# Convenience script to run engine tests

SUITE="./build_tests/engine_test_suite"

case "$1" in
    all)
        echo "Running all engine tests..."
        $SUITE --all
        ;;
    engine)
        if [ -z "$2" ]; then
            echo "Usage: $0 engine <EngineName>"
            exit 1
        fi
        echo "Testing engine: $2"
        $SUITE --engine "$2"
        ;;
    category)
        if [ -z "$2" ]; then
            echo "Usage: $0 category <category>"
            echo "Categories: reverb, pitch, eq, dynamics, delay, distortion, spatial"
            exit 1
        fi
        echo "Testing category: $2"
        $SUITE --category "$2"
        ;;
    quick)
        echo "Running quick smoke tests..."
        $SUITE --engine SpringReverb
        $SUITE --engine DigitalDelay
        $SUITE --engine ParametricEQ
        $SUITE --engine ClassicCompressor
        ;;
    *)
        echo "Usage: $0 {all|engine <name>|category <cat>|quick}"
        echo ""
        echo "Examples:"
        echo "  $0 all                    # Test all 56 engines"
        echo "  $0 engine SpringReverb    # Test single engine"
        echo "  $0 category reverb        # Test all reverbs"
        echo "  $0 quick                  # Quick smoke test"
        exit 1
        ;;
esac
EOF

chmod +x run_tests.sh

echo "Created helper script: run_tests.sh"
echo ""
echo "Quick start:"
echo "  ./run_tests.sh quick    # Run quick smoke tests"
echo "  ./run_tests.sh all      # Run full test suite"