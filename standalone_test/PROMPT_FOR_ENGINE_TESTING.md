# PERFECT PROMPT FOR ENGINE TESTING

## Context
You are testing the Chimera Phoenix v3.0 audio plugin, which has 57 DSP engines (IDs 0-56). Previous testing was superficial and produced fabricated claims. You must now test each engine individually, thoroughly, and honestly.

---

## Your Mission

Test ALL 57 engines systematically using the ENGINE_TESTING_PROTOCOL.md specification.

**Constraints:**
1. **ONE ENGINE AT A TIME** - Never batch process
2. **EVIDENCE REQUIRED** - Every claim needs a corresponding output file
3. **REAL CODE ONLY** - Link against `../JUCE_Plugin/Builds/MacOSX/build/Release/libChimeraPhoenix.a`
4. **NO FABRICATION** - If a test didn't run, say so
5. **VERIFY EVERYTHING** - Check that output files contain expected data

---

## Process for Each Engine

### Step 1: Create Test Program
```
File: test_engine_[ID]_[NAME].cpp

Required structure:
- Include EngineFactory.h (real plugin code)
- Create engine: EngineFactory::createEngine(ID)
- Test basic functionality (crash, NaN, Inf, silence)
- Test ALL parameters (sweep 0.0 → 1.0)
- Measure audio quality (THD, frequency response)
- Measure performance (CPU, memory)
- Stress test (10,000 blocks)
- Output all results to test_engine_[ID]_output.txt
```

### Step 2: Compile
```bash
clang++ -std=c++17 -O2 \
    -I../JUCE_Plugin/Source \
    -I../JUCE_Plugin/JuceLibraryCode \
    -I/Users/Branden/JUCE/modules \
    -DJUCE_STANDALONE_APPLICATION=1 \
    test_engine_[ID]_[NAME].cpp \
    ../JUCE_Plugin/Builds/MacOSX/build/Release/libChimeraPhoenix.a \
    -framework CoreAudio -framework CoreFoundation \
    -framework Accelerate -framework AudioToolbox \
    -framework CoreMIDI -framework Cocoa -framework IOKit \
    -o test_engine_[ID]_[NAME]
```

### Step 3: Run Test
```bash
./test_engine_[ID]_[NAME] | tee test_engine_[ID]_output.txt
```

### Step 4: Verify Output
```bash
# Check that output file exists and has content
ls -lh test_engine_[ID]_output.txt
cat test_engine_[ID]_output.txt | wc -l  # Should be >100 lines

# Verify test sections ran
grep "BASIC FUNCTIONALITY" test_engine_[ID]_output.txt
grep "PARAMETER TEST" test_engine_[ID]_output.txt
grep "AUDIO QUALITY" test_engine_[ID]_output.txt
grep "STRESS TEST" test_engine_[ID]_output.txt
```

### Step 5: Extract Results
```
Parse output file for:
- Did engine create? YES/NO
- Did it crash? YES/NO
- Any NaN/Inf? YES/NO
- Produces output? YES/NO
- Parameter count: X
- Parameters tested: X
- THD measurement: X.XXX%
- CPU usage: X.XX%
- Stress test: PASS/FAIL
```

### Step 6: Grade
```
PASS    - All tests passed
MARGINAL - Minor issues, usable
FAIL    - Critical issues, broken
```

### Step 7: Update Protocol
```
Mark engine as [x] in ENGINE_TESTING_PROTOCOL.md
Update "Engines Tested" count
```

### Step 8: Report
```
Create short status update:
"Engine [ID] ([NAME]): [PASS/MARGINAL/FAIL]
- Basic: [result]
- Parameters: [X/X passed]
- THD: [X.XXX%]
- CPU: [X.XX%]
- Issues: [list if any]
- Evidence: test_engine_[ID]_output.txt ([size])"
```

---

## Critical Rules

### DO:
✅ Test one engine completely before moving to next
✅ Save ALL output to log files
✅ Verify log files exist before claiming results
✅ Report actual numbers from output files
✅ Say "test didn't run" if executable missing
✅ Re-run tests if results look wrong
✅ Ask for clarification if stuck

### DON'T:
❌ Test multiple engines in parallel
❌ Claim results without corresponding log file
❌ Use mock/stub implementations
❌ Skip parameter testing
❌ Fabricate measurements
❌ Assume tests passed if no output
❌ Create summary reports without individual test evidence

---

## Example Session

```
User: "Start testing the engines"