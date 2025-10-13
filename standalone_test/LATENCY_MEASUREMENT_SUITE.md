# Latency Measurement Suite

## Overview

Comprehensive latency testing tool for ChimeraPhoenix audio engines. Measures the exact delay from input impulse to first output for all pitch shifters, reverbs, and time-based effects.

## Purpose

- **Precise Latency Measurement**: Determines exact sample count and milliseconds from impulse to first output
- **Complete Coverage**: Tests all 14 engines across pitch, reverb, and delay categories
- **Stability Testing**: Verifies output stability and detects problematic engines
- **Consistency Analysis**: Checks if latency varies with parameter changes
- **Documentation**: Generates detailed reports for plugin delay compensation (PDC)

## Engines Tested

### Pitch Shifters (4 engines)
- **Engine 31**: Detune Doubler
- **Engine 32**: Pitch Shifter
- **Engine 33**: Intelligent Harmonizer
- **Engine 49**: Pitch Shifter (Alt)

### Reverbs (5 engines)
- **Engine 39**: Convolution Reverb
- **Engine 40**: Shimmer Reverb
- **Engine 41**: Plate Reverb
- **Engine 42**: Spring Reverb
- **Engine 43**: Gated Reverb

### Delays / Time-based Effects (5 engines)
- **Engine 34**: Tape Echo
- **Engine 35**: Digital Delay
- **Engine 36**: Magnetic Drum Echo
- **Engine 37**: Bucket Brigade Delay
- **Engine 38**: Buffer Repeat Platinum

## Measurement Methodology

### Detection Algorithm

1. **Impulse Generation**: Creates a unit impulse (1.0 at sample 0)
2. **Processing**: Processes through engine in blocks (512 samples)
3. **Threshold Detection**: Scans output for first sample > -60dB (0.001 linear)
4. **Latency Calculation**: Measures samples from impulse to threshold crossing
5. **Verification**: Confirms stability, checks for NaN/Inf

### Test Parameters

- **Sample Rate**: 48,000 Hz
- **Block Size**: 512 samples
- **Detection Threshold**: -60 dB (0.001 linear amplitude)
- **Max Detection Window**: 1 second (48,000 samples)
- **Precision**: Sample-accurate (±1 sample)

### Parameter Setup

**Reverbs:**
- Mix = 100% wet
- Decay/Size = 50%
- Damping = 30%
- Other params = default

**Delays:**
- Time = 20% (short for clear detection)
- Feedback = 0% (no repeats)
- Mix = 100% wet

**Pitch Shifters:**
- Pitch = 50% (unity/no shift)
- Mix = 100% wet
- Other params = default

## Building

```bash
cd standalone_test
./build_latency_suite.sh
```

### Build Requirements

- macOS with Xcode command line tools
- clang++ compiler
- JUCE framework (path: `/Users/Branden/JUCE`)
- ChimeraPhoenix engine sources
- `required_engines.txt` file listing engine sources

### Build Process

The build script:
1. Compiles JUCE modules (cached if available)
2. Compiles all required engine sources
3. Compiles latency measurement suite
4. Links executable with frameworks and dependencies

## Running the Test

```bash
cd standalone_test
./build/latency_measurement_suite
```

### Expected Runtime

- **Total Time**: ~30-60 seconds
- **Per Engine**: ~2-4 seconds
- **Output**: Terminal report + CSV file

## Output Files

### Console Report

Detailed text report with:
- Real-time measurement for each engine
- Category statistics (min/max/avg latency)
- Overall summary
- Notable cases (lowest/highest latency)
- Problematic engines (if any)

### CSV Report: `latency_report.csv`

Comma-separated values with columns:
- `EngineID`: Numeric engine identifier
- `EngineName`: Human-readable engine name
- `Category`: Pitch/Reverb/Delay
- `LatencySamples`: Latency in samples
- `LatencyMs`: Latency in milliseconds
- `HasOutput`: Yes/No output detection
- `IsStable`: Yes/No (no NaN/Inf)
- `IsConstant`: Yes/No/Variable (parameter consistency)
- `FirstPeakAmp`: Amplitude of first detected sample
- `Notes`: Additional observations

## Interpretation

### Latency Values

**Low Latency (<50 samples / <1ms)**
- Typical for: Simple delays, some distortions
- Characteristics: Time-domain processing, minimal buffering
- Use case: Real-time performance, live monitoring

**Moderate Latency (50-2048 samples / 1-43ms)**
- Typical for: Most pitch shifters, basic reverbs
- Characteristics: FFT-based or granular processing
- Use case: Studio mixing, acceptable tracking delay

**High Latency (>2048 samples / >43ms)**
- Typical for: Convolution reverbs, complex pitch algorithms
- Characteristics: Large FFT windows, extensive processing
- Use case: Offline processing, post-production

### Consistency Analysis

**Constant Latency** (✓ Preferred)
- Latency doesn't change with parameters
- Enables accurate plugin delay compensation
- Predictable behavior across all settings

**Variable Latency** (⚠ Warning)
- Latency changes with parameters
- May require dynamic PDC or user notification
- Common in: Tape echo (tape speed), some delays

### Quality Indicators

**Good Engine:**
- ✓ Has output detected
- ✓ Stable (no NaN/Inf)
- ✓ Constant latency
- ✓ First peak amplitude > -40dB

**Problematic Engine:**
- ✗ No output detected
- ✗ Unstable output
- ⚠ Very low output level (<-80dB)

## Example Output

```
╔════════════════════════════════════════════════════════════════════════════╗
║              LATENCY MEASUREMENT REPORT - ALL ENGINES                      ║
╚════════════════════════════════════════════════════════════════════════════╝

Sample Rate: 48000 Hz
Detection Threshold: -60 dB (0.001 linear)

================================================================================
Pitch ENGINES
================================================================================

ID  Engine Name                            Samples          ms    Constant  Status
--------------------------------------------------------------------------------
31  Detune Doubler                             512      10.667         Yes  ✓ OK
32  Pitch Shifter                             2048      42.667         Yes  ✓ OK
33  Intelligent Harmonizer                    2048      42.667         Yes  ✓ OK
49  Pitch Shifter (Alt)                       2048      42.667         Yes  ✓ OK

  Category Statistics:
    Min Latency: 512 samples (10.667 ms)
    Max Latency: 2048 samples (42.667 ms)
    Avg Latency: 1664.0 samples (34.667 ms)

================================================================================
Reverb ENGINES
================================================================================

ID  Engine Name                            Samples          ms    Constant  Status
--------------------------------------------------------------------------------
39  Convolution Reverb                        4096      85.333         Yes  ✓ OK
40  Shimmer Reverb                            1024      21.333         Yes  ✓ OK
41  Plate Reverb                               512      10.667         Yes  ✓ OK
42  Spring Reverb                              256       5.333         Yes  ✓ OK
43  Gated Reverb                               512      10.667         Yes  ✓ OK

  Category Statistics:
    Min Latency: 256 samples (5.333 ms)
    Max Latency: 4096 samples (85.333 ms)
    Avg Latency: 1280.0 samples (26.667 ms)

================================================================================
Delay ENGINES
================================================================================

ID  Engine Name                            Samples          ms    Constant  Status
--------------------------------------------------------------------------------
34  Tape Echo                                  128       2.667      Variable  ✓ OK
      Notes: Variable latency
35  Digital Delay                                0       0.000         Yes  ✓ OK
36  Magnetic Drum Echo                         256       5.333         Yes  ✓ OK
37  Bucket Brigade Delay                       512      10.667         Yes  ✓ OK
38  Buffer Repeat Platinum                      64       1.333         Yes  ✓ OK

  Category Statistics:
    Min Latency: 0 samples (0.000 ms)
    Max Latency: 512 samples (10.667 ms)
    Avg Latency: 192.0 samples (4.000 ms)
```

## Use Cases

### Plugin Delay Compensation (PDC)

Use latency measurements to implement PDC in DAWs:

```cpp
// Example: Report latency to host
int getLatencyInSamples() {
    switch (currentEngine) {
        case 31: return 512;   // Detune Doubler
        case 32: return 2048;  // Pitch Shifter
        case 39: return 4096;  // Convolution Reverb
        // ... etc
    }
}
```

### User Documentation

Include latency values in user manual:
- "Convolution Reverb: 85ms latency at 48kHz"
- "Digital Delay: Zero-latency time-domain processing"

### Performance Optimization

Identify high-latency engines for optimization:
- Reduce FFT window sizes
- Implement look-ahead optimization
- Add low-latency modes

### Quality Assurance

Verify engines meet latency requirements:
- Real-time: <5ms preferred, <10ms acceptable
- Studio: <50ms acceptable
- Offline: No limit

## Troubleshooting

### No Output Detected

**Possible Causes:**
- Engine not producing output
- Mix parameter set to dry
- Parameter values causing silence

**Solutions:**
1. Check parameter setup in `measureEngineLatency()`
2. Verify engine processes impulse correctly
3. Lower detection threshold (currently -60dB)

### Unstable Output

**Possible Causes:**
- Uninitialized buffers
- Division by zero
- Numerical instability

**Solutions:**
1. Check engine initialization
2. Verify prepareToPlay() called
3. Review engine reset() behavior

### Variable Latency

**Possible Causes:**
- Time/speed parameters affect processing delay
- Buffer size changes with settings
- Intentional design (tape speed, etc.)

**Solutions:**
- Document as expected behavior
- Implement dynamic PDC
- Add user warning in documentation

### Build Errors

**Missing required_engines.txt:**
```bash
# Generate from existing test
ls ../JUCE_Plugin/Source/*.cpp | grep -E "(Engine|EngineFactory)" > required_engines.txt
```

**JUCE not found:**
```bash
# Update JUCE_DIR in build script
JUCE_DIR="/path/to/your/JUCE"
```

**Linking errors:**
```bash
# Check homebrew harfbuzz
brew install harfbuzz
```

## Technical Details

### Measurement Precision

- **Sample Accurate**: ±1 sample precision
- **Time Resolution**: ±0.021ms at 48kHz
- **Threshold**: -60dB prevents false detection from noise floor

### Processing Pipeline

```
Impulse (1.0) → [Engine Processing] → Output Buffer → Threshold Scan → Latency Count
       ↓                                      ↓
   Sample 0                         First sample > 0.001
```

### Consistency Test Algorithm

1. Test engine with 5 different parameter values (0%, 25%, 50%, 75%, 100%)
2. Measure latency for each configuration
3. Calculate min/max variation
4. Report as "Constant" if variation < 100 samples (~2ms)

### Error Handling

The suite includes comprehensive error handling:
- Try/catch around each engine test
- Graceful failure (continues to next engine)
- Error results included in report
- Exception messages captured in notes

## Integration with CI/CD

### Automated Testing

```bash
#!/bin/bash
# latency_regression_test.sh

./build_latency_suite.sh
./build/latency_measurement_suite

# Check for failures
if grep -q "NO OUTPUT\|UNSTABLE" latency_report.csv; then
    echo "FAIL: Latency test detected problems"
    exit 1
fi

echo "PASS: All engines have stable latency"
exit 0
```

### Benchmark Tracking

Track latency over time:
```bash
# Save historical data
cp latency_report.csv "latency_$(date +%Y%m%d).csv"

# Compare with baseline
diff latency_baseline.csv latency_report.csv
```

## Related Documentation

- **CATEGORY_COMPARISON_MATRIX.md**: Overall engine quality assessment
- **REVERB_QUALITY_ASSESSMENT.md**: Reverb-specific quality metrics
- **RUN_PITCH_TESTS.md**: Pitch accuracy and quality testing
- **MODULATION_TECHNICAL_SUMMARY.md**: Modulation effects analysis

## Credits

**Author**: Claude (Anthropic)
**Date**: October 11, 2025
**Version**: 1.0
**Project**: ChimeraPhoenix v3.0 Audio Effects Suite

## License

Part of the ChimeraPhoenix project. All rights reserved.
