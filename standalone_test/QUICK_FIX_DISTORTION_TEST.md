# QUICK FIX: Distortion Real-World Test

## Problem
Test won't link due to JUCE debug/release mode mismatch.

## Solution (Choose One)

### Option 1: Use Debug Mode (FASTEST - 1 minute)
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

# Edit build script
nano build_distortion_realworld.sh

# Change line 24 from:
CXXFLAGS="-std=c++17 -O3 -I$PROJECT_ROOT -I$JUCE_DIR/modules -I$STANDALONE_DIR -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1"

# To:
CXXFLAGS="-std=c++17 -O3 -I$PROJECT_ROOT -I$JUCE_DIR/modules -I$STANDALONE_DIR -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DDEBUG=1"

# Save and run
./build_distortion_realworld.sh
```

### Option 2: Use Existing Test Infrastructure (2 minutes)
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

# Use the existing distortion test that already works
./build_distortion_test.sh

# Or use test_distortion_15_19 which is proven
./build_test_15_19.sh
```

### Option 3: Manual Quick Test (5 minutes)
```bash
# 1. Generate materials
python3 generate_distortion_test_materials.py

# 2. Use existing working test with new materials
# Just need to modify test_distortion_15_19.cpp to load .raw files instead of generating signals

# 3. Or create minimal standalone test
cat > test_distortion_minimal.cpp << 'EOF'
#include <iostream>
#include "DistortionEngineFactory.h"

int main() {
    std::cout << "Testing distortion engine factory...\n";

    for (int id = 15; id <= 22; ++id) {
        auto engine = DistortionEngineFactory::createEngine(id);
        if (engine) {
            std::cout << "✅ Engine " << id << ": " << engine->getName() << "\n";
        } else {
            std::cout << "❌ Engine " << id << ": FAILED TO CREATE\n";
        }
    }

    return 0;
}
EOF

# Compile minimal test
clang++ -std=c++17 -O3 \
    -I../JUCE/modules -I../JUCE_Plugin/Source -I. \
    -DDEBUG=1 \
    test_distortion_minimal.cpp \
    DistortionEngineFactory.cpp \
    build/*.o \
    juce_core.o juce_audio_basics.o \
    -framework Accelerate -framework AudioToolbox -framework CoreAudio -framework CoreMIDI -framework Foundation \
    -o test_minimal

./test_minimal
```

## What Each Option Does

**Option 1**: Matches test compilation to existing JUCE debug objects
- Pros: Quick, uses all new code
- Cons: Slightly slower runtime (negligible)

**Option 2**: Uses proven working infrastructure
- Pros: Guaranteed to work
- Cons: May need code adaptation for new materials

**Option 3**: Creates minimal validation test
- Pros: Fastest to verify engines load
- Cons: Doesn't run full test suite

## Recommended: Option 1

Just change `-DNDEBUG=1` to `-DDEBUG=1` in the build script.

## After Fix

```bash
# Run test
./build_distortion_realworld.sh

# Results will be in:
#  - DISTORTION_REALWORLD_TEST_REPORT.md
#  - distortion_output_*.wav
#  - distortion_spectrum_*.csv
```

## Test Materials Already Generated

```bash
ls -lh distortion_test_*.raw
```

Should show:
- distortion_test_guitar_di.raw (375K)
- distortion_test_bass.raw (375K)
- distortion_test_drums.raw (375K)
- distortion_test_synth.raw (375K)

All ready to go!

