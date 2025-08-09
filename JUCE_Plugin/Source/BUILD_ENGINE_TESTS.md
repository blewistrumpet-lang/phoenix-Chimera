# Simplified Engine Test Harness - Build Instructions

This document explains how to build and run the simplified C++ test harness for Project Chimera's DSP engines.

## Files

- `SimplifiedEngineTestHarness.h` - Main test harness (header-only implementation)
- `RunSimplifiedEngineTests.cpp` - Simple runner with main() function
- This build guide

## Quick Start

### Option 1: Add to Existing JUCE Project

1. Include the test files in your existing JUCE plugin project
2. Add this to your main application or create a separate test target:

```cpp
#include "SimplifiedEngineTestHarness.h"

int main() {
    SimplifiedEngineTestHarness harness;
    harness.runAllTests();
    return 0;
}
```

### Option 2: Standalone Console Application

Create a simple JUCE console application:

```cpp
// main.cpp
#include <JuceHeader.h>
#include "SimplifiedEngineTestHarness.h"

class TestApplication : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Engine Tests"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    
    void initialise(const juce::String&) override {
        SimplifiedEngineTestHarness harness;
        harness.runAllTests();
        quit();
    }
    
    void shutdown() override {}
};

START_JUCE_APPLICATION(TestApplication)
```

## What the Tests Check

### 1. Engine Creation Test
- Verifies that each engine can be created via EngineFactory
- Checks basic method calls (getName(), getNumParameters())
- Ensures no crashes during instantiation

### 2. NaN/Infinity Handling
- Feeds NaN and Infinity values to each engine
- Verifies engines don't output NaN/Inf (critical safety issue)
- Tests robustness of internal calculations

### 3. Parameter Functionality
- Tests engines with default parameters (0.5 for all params)
- Tests with extreme parameter values (0.0 and 1.0)
- Verifies no crashes or invalid outputs

### 4. Audio Quality
- Tests with sine wave input signals
- Checks for excessive gain (>3x amplification)
- Detects potential clipping or distortion
- Identifies engines that produce silence (broken processing)

### 5. Thread Safety Basics
- Runs audio processing in one thread
- Updates parameters in another thread
- Checks for crashes, deadlocks, or invalid outputs
- Limited but practical concurrency test

## Output

The tests generate:

1. **Console Output** - Real-time progress and summary
2. **Detailed Report** - Text file with full results (default: `/tmp/simplified_engine_test_report.txt`)

### Sample Output

```
=== SUMMARY ===
Engines Passed: 52
Engines Failed: 5
Success Rate: 91.2%

=== ENGINES WITH ISSUES ===
Engine 18: Bit Crusher
  Creation: PASS
  NaN/Inf Handling: PASS
  Parameter Functionality: FAIL
  Audio Quality: FAIL
  Thread Safety: PASS
  Issues:
    - Engine outputs NaN/Inf with extreme parameters
    - Excessive gain detected - RMS gain: 4.23
```

## Integration Examples

### Quick Safety Check
```cpp
#include "SimplifiedEngineTestHarness.h"

bool areEnginesSafe() {
    return SimplifiedEngineTests::runQuickSafetyCheck();
}
```

### Test Specific Engine
```cpp
bool testEngine = SimplifiedEngineTests::testSpecificEngine(ENGINE_BIT_CRUSHER);
```

### Get Problem Engines
```cpp
auto problemEngines = SimplifiedEngineTests::getCriticallyFailedEngines();
```

## Customization

You can customize the test configuration:

```cpp
SimplifiedEngineTestHarness::TestConfig config;
config.sampleRate = 48000.0;           // Test sample rate
config.testBufferSize = 2048;          // Buffer size for tests
config.maxAcceptableGain = 2.0f;       // Max gain before flagging
config.threadTestIterations = 200;     // Thread safety test duration
config.reportPath = "my_test_report.txt";

SimplifiedEngineTestHarness harness(config);
harness.runAllTests();
```

## Expected Results

For a healthy codebase, you should see:
- **90%+ success rate** - Most engines should pass all tests
- **Zero critical failures** - No creation failures or NaN/Inf issues
- **Minor quality issues** - Some gain/parameter issues are acceptable

### Critical Issues (Fix Immediately)
- Engine creation failures
- NaN/Inf outputs (can crash DAWs)
- Thread safety crashes

### Quality Issues (Fix When Possible)  
- Excessive gain amplification
- Parameter handling quirks
- Minor audio quality problems

## Limitations

This is a *simplified* test harness focused on finding obvious problems:

- **Not comprehensive** - Real testing needs more extensive signal analysis
- **Basic thread safety** - Not a full concurrency test
- **Parameter assumptions** - Assumes 0.0-1.0 parameter ranges
- **Limited signals** - Only tests with sine waves and basic signals

For production use, consider more comprehensive testing with:
- Multiple signal types (noise, sweeps, music)
- Full parameter space exploration
- Extensive concurrency testing
- Audio analysis (THD, frequency response, etc.)
- Performance benchmarking

## Troubleshooting

### Compilation Issues
- Ensure all engine header files are included in your project
- Check that JUCE framework is properly configured
- Verify all engines compile individually

### Runtime Issues
- Check that `/tmp/` directory is writable (or change report path)
- Ensure sufficient memory for audio buffers
- Verify sample rate and buffer size settings match your system

### False Positives
- Some "failures" may be expected behavior (e.g., intentional distortion)
- Review individual engine documentation for expected behavior
- Adjust test parameters if needed for specific engine types