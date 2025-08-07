#!/bin/bash

# Chimera DSP Engine Test Builder and Runner
# This script builds and runs comprehensive tests for Chimera DSP engines

echo "==================================================================="
echo "Chimera DSP Engine Test Builder and Runner"
echo "==================================================================="

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="${SCRIPT_DIR}/Source"
BUILD_DIR="${SCRIPT_DIR}/engine_test_build"
TEST_EXECUTABLE="${BUILD_DIR}/test_all_engines"
JUCE_DIR="${SCRIPT_DIR}/JUCE"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if JUCE is available (simple check)
check_juce() {
    if [ ! -d "$JUCE_DIR" ]; then
        print_warning "JUCE directory not found at $JUCE_DIR"
        
        # Try to find JUCE in common locations
        POSSIBLE_JUCE_PATHS=(
            "/Applications/JUCE"
            "/usr/local/JUCE"
            "$HOME/JUCE"
            "${SCRIPT_DIR}/../JUCE"
            "${SCRIPT_DIR}/../../JUCE"
        )
        
        for juce_path in "${POSSIBLE_JUCE_PATHS[@]}"; do
            if [ -d "$juce_path" ]; then
                JUCE_DIR="$juce_path"
                print_success "Found JUCE at: $JUCE_DIR"
                return 0
            fi
        done
        
        print_error "JUCE not found. Please install JUCE or set JUCE_DIR environment variable"
        return 1
    fi
    return 0
}

# Create minimal JUCE header for testing
create_minimal_juce() {
    print_status "Creating minimal JUCE compatibility layer..."
    
    mkdir -p "${BUILD_DIR}/juce_minimal"
    
    cat > "${BUILD_DIR}/juce_minimal/JuceHeader.h" << 'EOF'
#pragma once

// Minimal JUCE compatibility layer for engine testing
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>
#include <chrono>

namespace juce {
    template<typename FloatType>
    struct MathConstants {
        static constexpr FloatType pi = static_cast<FloatType>(3.141592653589793238);
    };
    
    class String {
        std::string str;
    public:
        String() = default;
        String(const char* s) : str(s) {}
        String(const std::string& s) : str(s) {}
        
        const char* toRawUTF8() const { return str.c_str(); }
        std::string toStdString() const { return str; }
    };
    
    template<typename SampleType>
    class AudioBuffer {
        std::vector<std::vector<SampleType>> channels;
        int numChannels_, numSamples_;
        
    public:
        AudioBuffer(int channels, int samples) 
            : channels(channels, std::vector<SampleType>(samples, 0)), 
              numChannels_(channels), numSamples_(samples) {}
        
        int getNumChannels() const { return numChannels_; }
        int getNumSamples() const { return numSamples_; }
        
        SampleType* getWritePointer(int channel) { 
            return channels[channel].data(); 
        }
        const SampleType* getReadPointer(int channel) const { 
            return channels[channel].data(); 
        }
        
        void clear() {
            for (auto& ch : channels) {
                std::fill(ch.begin(), ch.end(), SampleType(0));
            }
        }
        
        void setSample(int channel, int sample, SampleType value) {
            if (channel < numChannels_ && sample < numSamples_) {
                channels[channel][sample] = value;
            }
        }
    };
    
    class Random {
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist;
    public:
        Random() : rng(std::chrono::steady_clock::now().time_since_epoch().count()), 
                   dist(0.0f, 1.0f) {}
        float nextFloat() { return dist(rng); }
    };
    
    class JUCEApplication {
    public:
        virtual ~JUCEApplication() = default;
        virtual const String getApplicationName() = 0;
        virtual const String getApplicationVersion() = 0;
        virtual void initialise(const String&) = 0;
        virtual void shutdown() = 0;
        void quit() { /* stub */ }
    };
}

#define START_JUCE_APPLICATION(AppClass) \
int main(int argc, char* argv[]) { \
    AppClass app; \
    app.initialise(""); \
    return 0; \
}
EOF

    print_success "Minimal JUCE compatibility layer created"
}

# Find all engine source files
find_engine_sources() {
    print_status "Searching for engine source files..."
    
    ENGINE_SOURCES=""
    REQUIRED_SOURCES=(
        "EngineFactory.cpp"
        "CompleteEngineMetadata.cpp"
        "EngineMetadataInit.cpp"
    )
    
    # Add required core files
    for src in "${REQUIRED_SOURCES[@]}"; do
        if [ -f "${SOURCE_DIR}/${src}" ]; then
            ENGINE_SOURCES="${ENGINE_SOURCES} ${SOURCE_DIR}/${src}"
            print_status "Added core file: ${src}"
        else
            print_warning "Required file not found: ${src}"
        fi
    done
    
    # Find all engine implementation files
    ENGINE_FILES=(
        "PlateReverb.cpp"
        "ClassicCompressor.cpp" 
        "RodentDistortion.cpp"
        "TapeEcho.cpp"
        "KStyleOverdrive.cpp"
        "MuffFuzz.cpp"
        "BitCrusher.cpp"
        "WaveFolder.cpp"
        "AnalogPhaser.cpp"
        "StereoChorus.cpp"
        "HarmonicExciter.cpp"
        "VintageOptoCompressor.cpp"
        "VintageTubePreamp.cpp"
        "NoiseGate.cpp"
        "ParametricEQ.cpp"
        "LadderFilter.cpp"
        "StateVariableFilter.cpp"
        "FormantFilter.cpp"
        "EnvelopeFilter.cpp"
        "CombResonator.cpp"
        "RotarySpeaker.cpp"
        "PitchShifter.cpp"
        "FrequencyShifter.cpp"
        "GranularCloud.cpp"
        "SpectralFreeze.cpp"
        "BufferRepeat.cpp"
        "ChaosGenerator.cpp"
        "IntelligentHarmonizer.cpp"
        "GatedReverb.cpp"
        "DetuneDoubler.cpp"
        "PhasedVocoder.cpp"
        "SpectralGate.cpp"
        "DigitalDelay.cpp"
        "BucketBrigadeDelay.cpp"
        "MagneticDrumEcho.cpp"
        "SpringReverb.cpp"
        "ConvolutionReverb.cpp"
        "ShimmerReverb.cpp"
        "StereoWidener.cpp"
        "StereoImager.cpp"
        "DimensionExpander.cpp"
        "FeedbackNetwork.cpp"
        "VocalFormantFilter.cpp"
        "HarmonicTremolo.cpp"
        "ClassicTremolo.cpp"
        "AnalogRingModulator.cpp"
        "ResonantChorus.cpp"
        "MultibandSaturator.cpp"
        "DynamicEQ.cpp"
        "VintageConsoleEQ.cpp"
        "TransientShaper_Platinum.cpp"
        "MasteringLimiter_Platinum.cpp"
        "NoiseGate_Platinum.cpp"
        "ParametricEQ_Platinum.cpp"
        "VintageConsoleEQ_Platinum.cpp"
        "VintageOptoCompressor_Platinum.cpp"
        "HarmonicExciter_Platinum.cpp"
        "ResonantChorus_Platinum.cpp"
        "RotarySpeaker_Platinum.cpp"
        "SpringReverb_Platinum.cpp"
        "SpectralGate_Platinum.cpp"
        "ChaosGenerator_Platinum.cpp"
        "BufferRepeat_Platinum.cpp"
        "GainUtility_Platinum.cpp"
        "MidSideProcessor_Platinum.cpp"
        "MonoMaker_Platinum.cpp"
        "PhaseAlign_Platinum.cpp"
        "PlatinumRingModulator.cpp"
    )
    
    local found_engines=0
    for engine in "${ENGINE_FILES[@]}"; do
        if [ -f "${SOURCE_DIR}/${engine}" ]; then
            ENGINE_SOURCES="${ENGINE_SOURCES} ${SOURCE_DIR}/${engine}"
            found_engines=$((found_engines + 1))
            print_status "Found engine: ${engine}"
        fi
    done
    
    print_success "Found ${found_engines} engine source files"
    return 0
}

# Build the test executable
build_test() {
    print_status "Building engine test executable..."
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    
    # Compiler settings
    CXX="clang++"
    CXXFLAGS="-std=c++17 -O2 -Wall -Wextra"
    INCLUDES="-I${SOURCE_DIR} -I${BUILD_DIR}/juce_minimal"
    DEFINES="-DJUCE_STANDALONE_APPLICATION=1"
    
    # macOS specific flags
    if [[ "$OSTYPE" == "darwin"* ]]; then
        CXXFLAGS="${CXXFLAGS} -stdlib=libc++"
        LDFLAGS="-framework Accelerate -framework AudioToolbox -framework AudioUnit -framework CoreAudio -framework CoreFoundation -framework CoreServices"
    else
        LDFLAGS="-lm -lpthread"
    fi
    
    print_status "Compiler: $CXX"
    print_status "Flags: $CXXFLAGS"
    print_status "Includes: $INCLUDES"
    
    # Build command
    BUILD_CMD="$CXX $CXXFLAGS $DEFINES $INCLUDES ${SOURCE_DIR}/test_all_engines.cpp $ENGINE_SOURCES $LDFLAGS -o $TEST_EXECUTABLE"
    
    print_status "Executing build command..."
    echo "$BUILD_CMD" > "${BUILD_DIR}/build_command.txt"
    
    if eval "$BUILD_CMD"; then
        print_success "Build completed successfully!"
        print_success "Test executable: $TEST_EXECUTABLE"
        return 0
    else
        print_error "Build failed!"
        return 1
    fi
}

# Run the tests
run_tests() {
    print_status "Running engine tests..."
    
    if [ ! -f "$TEST_EXECUTABLE" ]; then
        print_error "Test executable not found: $TEST_EXECUTABLE"
        return 1
    fi
    
    # Make executable
    chmod +x "$TEST_EXECUTABLE"
    
    # Run tests and capture output
    print_status "Starting test execution..."
    echo ""
    
    if "$TEST_EXECUTABLE"; then
        print_success "Tests completed successfully!"
        
        # Check if detailed report was generated
        if [ -f "chimera_engine_test_report.txt" ]; then
            print_success "Detailed test report generated: chimera_engine_test_report.txt"
        fi
        
        return 0
    else
        print_error "Tests failed or executable crashed!"
        return 1
    fi
}

# Clean build artifacts
clean_build() {
    print_status "Cleaning build artifacts..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned"
    fi
}

# Main execution flow
main() {
    print_status "Starting Chimera DSP Engine test process..."
    
    case "${1:-build_and_run}" in
        "clean")
            clean_build
            ;;
        "build")
            create_minimal_juce || exit 1
            find_engine_sources || exit 1
            build_test || exit 1
            ;;
        "run")
            run_tests || exit 1
            ;;
        "build_and_run"|"")
            create_minimal_juce || exit 1
            find_engine_sources || exit 1
            build_test || exit 1
            echo ""
            run_tests || exit 1
            ;;
        *)
            echo "Usage: $0 [clean|build|run|build_and_run]"
            echo "  clean        - Clean build artifacts"
            echo "  build        - Build test executable only"
            echo "  run          - Run tests only"
            echo "  build_and_run - Build and run tests (default)"
            exit 1
            ;;
    esac
    
    print_success "Process completed!"
}

# Execute main function with all arguments
main "$@"