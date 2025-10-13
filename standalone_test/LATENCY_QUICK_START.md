# Latency Measurement Suite - Quick Start Guide

## TL;DR - Run the Test

```bash
cd standalone_test
./build_latency_suite.sh    # Build (1-2 minutes)
./build/latency_measurement_suite    # Run test (~30 seconds)
```

**Output**: Terminal report + `latency_report.csv`

---

## What It Tests

✓ **4 Pitch Shifters** (31, 32, 33, 49)
✓ **5 Reverbs** (39, 40, 41, 42, 43)
✓ **5 Delays** (34, 35, 36, 37, 38)

**Total**: 14 engines tested for latency

---

## What You Get

### Console Output
```
[Measuring Engine 31: Detune Doubler]
  Latency: 512 samples (10.667 ms)
  First peak: 0.950000 at sample 512
  Latency is constant across parameters

[Measuring Engine 32: Pitch Shifter]
  Latency: 2048 samples (42.667 ms)
  First peak: 0.890000 at sample 2048
  Latency is constant across parameters
```

### Final Report
```
================================================================================
Pitch ENGINES
================================================================================

ID  Engine Name                      Samples      ms    Constant  Status
--------------------------------------------------------------------------------
31  Detune Doubler                       512   10.667         Yes  ✓ OK
32  Pitch Shifter                       2048   42.667         Yes  ✓ OK
33  Intelligent Harmonizer              2048   42.667         Yes  ✓ OK
49  Pitch Shifter (Alt)                 2048   42.667         Yes  ✓ OK

Category Statistics:
  Min Latency: 512 samples (10.667 ms)
  Max Latency: 2048 samples (42.667 ms)
  Avg Latency: 1664.0 samples (34.667 ms)
```

### CSV File: `latency_report.csv`
```csv
EngineID,EngineName,Category,LatencySamples,LatencyMs,HasOutput,IsStable,IsConstant,FirstPeakAmp,Notes
31,"Detune Doubler","Pitch",512,10.666667,Yes,Yes,Yes,9.500000e-01,""
32,"Pitch Shifter","Pitch",2048,42.666668,Yes,Yes,Yes,8.900000e-01,""
```

---

## Understanding the Results

### Latency Categories

| Range | Classification | Typical Engines | Use Case |
|-------|---------------|-----------------|----------|
| 0-50 samples (0-1ms) | **Very Low** | Digital Delays, Simple FX | Real-time performance |
| 50-512 samples (1-11ms) | **Low** | BBD, Basic Reverbs | Live tracking |
| 512-2048 samples (11-43ms) | **Moderate** | Pitch Shifters, FFT FX | Studio mixing |
| >2048 samples (>43ms) | **High** | Convolution Reverb | Post-production |

### Status Indicators

| Symbol | Meaning | Action |
|--------|---------|--------|
| **✓ OK** | Working perfectly | Use as-is |
| **✗ NO OUTPUT** | Not producing audio | Fix engine |
| **⚠ UNSTABLE** | NaN/Inf in output | Debug processing |

### Consistency

- **Constant**: Latency doesn't change with parameters (good for PDC)
- **Variable**: Latency varies with settings (document this behavior)

---

## Common Use Cases

### 1. Plugin Delay Compensation (PDC)

```cpp
int MyPlugin::getLatencySamples() {
    // From latency_report.csv
    switch (currentEngine) {
        case 31: return 512;   // Detune Doubler
        case 32: return 2048;  // Pitch Shifter
        case 39: return 4096;  // Convolution Reverb
        default: return 0;
    }
}
```

### 2. User Manual Documentation

```markdown
## Latency Information

This plugin introduces processing latency depending on the selected effect:

- **Detune Doubler**: 10.7ms (512 samples @ 48kHz)
- **Pitch Shifter**: 42.7ms (2048 samples @ 48kHz)
- **Convolution Reverb**: 85.3ms (4096 samples @ 48kHz)

Your DAW will automatically compensate for this delay.
```

### 3. Performance Benchmarking

Track latency over development:
```bash
# Run and save with date
./build/latency_measurement_suite
cp latency_report.csv results/latency_$(date +%Y%m%d).csv

# Compare with previous
diff results/latency_baseline.csv latency_report.csv
```

### 4. Quality Assurance

Check all engines pass:
```bash
./build/latency_measurement_suite > /dev/null
if grep -q "NO OUTPUT\|UNSTABLE" latency_report.csv; then
    echo "❌ FAIL: Some engines have issues"
    exit 1
fi
echo "✅ PASS: All engines working"
```

---

## Troubleshooting

### Build Fails

**Problem**: `required_engines.txt not found`
```bash
# Create file listing engine sources
ls ../JUCE_Plugin/Source/*Engine*.cpp > required_engines.txt
ls ../JUCE_Plugin/Source/EngineFactory.cpp >> required_engines.txt
```

**Problem**: `JUCE modules not found`
```bash
# Edit build_latency_suite.sh, update JUCE_DIR
JUCE_DIR="/your/path/to/JUCE"
```

### No Output for Engine

**Possible causes**:
1. Engine not processing impulse
2. Mix parameter too low
3. Engine requires specific parameter setup

**Fix**: Edit `latency_measurement_suite.cpp`, adjust parameter values in `measureEngineLatency()`

### Variable Latency Warning

**Is it a problem?**
- No, if it's a delay effect (tape speed changes delay time)
- Yes, if it's a pitch shifter or reverb (should be constant)

**Action**:
- Document if intentional
- Fix if unintentional

---

## Expected Results Summary

### Typical Latency Values (@ 48kHz)

**Pitch Shifters:**
- Detune Doubler: ~10-15ms
- Pitch Shifter: ~40-45ms
- Harmonizer: ~40-45ms

**Reverbs:**
- Spring: ~5-10ms
- Plate: ~10-15ms
- Shimmer: ~20-25ms
- Convolution: ~80-90ms

**Delays:**
- Digital: ~0-2ms
- BBD: ~10-15ms
- Tape: ~2-5ms (variable)

---

## Files Created

After running the test:

```
standalone_test/
├── build/
│   └── latency_measurement_suite          # Executable
├── latency_report.csv                     # CSV data
├── latency_measurement_suite.cpp          # Source code
├── build_latency_suite.sh                 # Build script
├── LATENCY_MEASUREMENT_SUITE.md           # Full docs
└── LATENCY_QUICK_START.md                 # This file
```

---

## Integration with Other Tests

This latency suite complements other test suites:

1. **THD Tests** → Audio quality
2. **CPU Benchmark** → Performance
3. **Pitch Tests** → Pitch accuracy
4. **Latency Tests** → Delay characteristics ← You are here

All together provide complete engine validation.

---

## Next Steps

1. ✅ Run the test
2. ✅ Review the report
3. ✅ Check CSV file for specifics
4. ✅ Document latencies in user manual
5. ✅ Implement PDC in plugin
6. ✅ Track latency in CI/CD

---

## Questions?

- Full documentation: `LATENCY_MEASUREMENT_SUITE.md`
- Source code: `latency_measurement_suite.cpp`
- Build script: `build_latency_suite.sh`

**Author**: Claude (Anthropic)
**Date**: October 11, 2025
**Project**: ChimeraPhoenix v3.0
