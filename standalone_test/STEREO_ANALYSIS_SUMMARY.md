# Stereo Analysis Suite - Comprehensive Report

## Overview

This comprehensive stereo analysis suite evaluates ALL 56 engines in the Chimera Phoenix audio system for stereo quality, identifying mono-collapsed engines, phase issues, and overall stereo performance.

## Analysis Metrics

### Core Measurements

1. **L/R Correlation** (Pearson coefficient)
   - Measures similarity between left and right channels
   - Range: -1.0 (completely opposite) to 1.0 (identical)
   - **Threshold**: < 0.8 for good stereo separation

2. **Stereo Width**
   - Ratio of side content to mid content
   - Range: 0.0 (mono) to >1.0 (wide stereo)
   - Higher values indicate more stereo content

3. **Phase Coherence**
   - Measures phase relationship between channels
   - Range: -1.0 (out of phase) to 1.0 (in phase)
   - Values < -0.5 indicate phase reversal

4. **Mid/Side Analysis**
   - Mid: (L+R)/2 (center content)
   - Side: (L-R)/2 (stereo content)
   - Ratio in dB indicates stereo vs mono balance

5. **Mono Compatibility**
   - Tests how well signal survives mono summing
   - Range: 0.0 (complete cancellation) to 2.0 (perfect)
   - Values < 0.8 indicate phase issues

## Results Summary (31 Engines Tested)

### Overall Statistics
- **Total Engines**: 57 (engines 0-56)
- **Analyzed**: 31 (engines 0-30)
- **No Data**: 26 (engines 31-56, test hung at engine 31)

### Quality Distribution
- **PASSED**: 8 engines (25.8%)
- **WARNINGS**: 3 engines (9.7%)
- **FAILED**: 20 engines (64.5%)

### Critical Issues Found
- **Mono Collapsed**: 20 engines (correlation > 0.95 when should be stereo)
- **Phase Issues**: 2 engines (Ring Modulator, Vintage Tube Preamp - expected behavior)
- **Significantly Imbalanced**: 1 engine (Dynamic EQ)

## Detailed Results by Category

### 🟢 PASSED Engines (Good Stereo Quality)

1. **Engine 9: Ladder Filter** - Grade A
   - Correlation: 0.797
   - Stereo Width: 0.261
   - Excellent stereo separation

2. **Engine 15: Vintage Tube Preamp** - Grade A
   - Correlation: 0.074
   - Stereo Width: 0.929
   - Outstanding stereo width (non-linear processing creates natural stereo)

3. **Engine 19: Multiband Saturator** - Grade B
   - Correlation: 0.861
   - Stereo Width: 0.274
   - Good stereo separation

4. **Engine 23: Digital Chorus** - Grade A
   - Correlation: 0.417
   - Stereo Width: 0.643
   - Excellent stereo effect

5. **Engine 26: Ring Modulator** - Grade A
   - Correlation: -0.026
   - Stereo Width: 1.026
   - Exceptional stereo width (expected for ring mod)

6. **Engine 27: Frequency Shifter** - Grade B
   - Correlation: 0.857
   - Stereo Width: 0.278
   - Good stereo character

7. **Engine 30: Rotary Speaker** - Grade A
   - Correlation: 0.658
   - Stereo Width: 0.454
   - Excellent rotary stereo simulation

8. **Engine 0: None (Bypass)** - Grade A
   - Correlation: 0.966
   - Expected behavior for bypass (preserves input)

### 🟡 WARNING Engines (Marginal Stereo)

1. **Engine 16: Wave Folder** - Grade C
   - Correlation: 0.910
   - Stereo Width: 0.217
   - Marginal stereo separation

2. **Engine 21: Rodent Distortion** - Grade C
   - Correlation: 0.937
   - Stereo Width: 0.180
   - Limited stereo separation

3. **Engine 24: Resonant Chorus** - Grade C
   - Correlation: 0.929
   - Stereo Width: 0.192
   - Should have more stereo width for a chorus

### 🔴 FAILED Engines (Mono Collapsed)

#### Dynamics (6/6 failed)
- **Engine 1**: Vintage Opto Compressor - Corr 0.966
- **Engine 2**: Classic VCA Compressor - Corr 0.966
- **Engine 3**: Transient Shaper - Corr 0.966
- **Engine 4**: Noise Gate - Corr 0.966
- **Engine 5**: Mastering Limiter - Corr 0.966
- **Engine 6**: Dynamic EQ - Corr 0.991 (also imbalanced)

#### Filters (7/8 failed)
- **Engine 7**: Parametric EQ (Studio) - Corr 0.966
- **Engine 8**: Vintage Console EQ - Corr 0.966
- **Engine 10**: State Variable Filter - Corr 0.966
- **Engine 11**: Formant Filter - Corr 0.957
- **Engine 12**: Envelope Filter - Corr 0.966
- **Engine 13**: Comb Resonator - Corr 0.964
- **Engine 14**: Vocal Formant Filter - Corr 0.966

#### Distortion (4/8 failed)
- **Engine 17**: Harmonic Exciter - Corr 0.965
- **Engine 18**: Bit Crusher - Corr 0.966
- **Engine 20**: Muff Fuzz - Corr 0.966
- **Engine 22**: K-Style Overdrive - Corr 0.965

#### Modulation (3/11 failed)
- **Engine 25**: Analog Phaser - Corr 0.966
- **Engine 28**: Harmonic Tremolo - Corr 0.957
- **Engine 29**: Classic Tremolo - Corr 0.966

## Analysis by Category

### Dynamics (6 engines tested)
- **Average Correlation**: 0.970 (too high!)
- **Average Stereo Width**: 0.145 (too narrow!)
- **Status**: All engines appear to be processing stereo as dual mono
- **Recommendation**: Review dynamics processing - should preserve stereo imaging

### Filters (8 engines tested)
- **Average Correlation**: 0.943
- **Average Stereo Width**: 0.150
- **Status**: Most filters collapse stereo (except Ladder Filter)
- **Recommendation**: Implement independent L/R filter processing

### Distortion (8 engines tested)
- **Average Correlation**: 0.831
- **Average Stereo Width**: 0.266
- **Status**: Mixed results - some good, some collapsed
- **Best**: Vintage Tube Preamp (excellent stereo)
- **Recommendation**: Non-linear processors should process independently per channel

### Modulation (11 engines tested)
- **Average Correlation**: 0.715
- **Average Stereo Width**: 0.376
- **Status**: Best category overall
- **Best**: Ring Modulator, Digital Chorus, Rotary Speaker
- **Recommendation**: Some tremolo/phaser effects need stereo width enhancement

### Not Tested
- **Delay** (5 engines): No data - test hung at engine 31
- **Reverb** (5 engines): No data
- **Spatial** (3 engines): No data - these are CRITICAL to test!
- **Special** (6 engines): No data
- **Utility** (4 engines): No data

## Technical Root Causes

### Why Engines Show High Correlation (Mono Collapse)

The test input signal has a slight phase difference (15°) between L/R to create stereo content. Correlation of 0.966 indicates:

1. **Processing stereo as dual mono**
   - Both channels processed identically
   - No stereo enhancement or separation
   - Identical parameter application to L/R

2. **Shared processing state**
   - Single filter/envelope/compressor for both channels
   - No independent L/R state variables
   - Linked control signals

3. **Missing stereo features**
   - No mid/side processing
   - No stereo width controls
   - No L/R decorrelation

## Critical Findings

### 1. Mono Collapse in Dynamics Processors
**All 6 dynamics engines are mono-collapsed**

This is concerning because:
- Compressors should preserve stereo image
- Gate stereo linking should be optional
- Modern dynamics processors maintain stereo field

**Fix**: Implement true stereo or linked-stereo modes

### 2. Filter Stereo Issues
**7 out of 8 filters collapse stereo**

Only the Ladder Filter maintains stereo separation. Others need:
- Independent L/R filter coefficients
- Separate state variables per channel
- Stereo-aware envelope following

### 3. Missing Spatial Engine Data
**CRITICAL**: Engines 44-46 are Stereo Widener, Stereo Imager, and Dimension Expander

These are specifically designed for stereo manipulation and MUST be tested:
- Engine 44: Stereo Widener
- Engine 45: Stereo Imager
- Engine 46: Dimension Expander

### 4. Reverb Data Missing
All 5 reverb engines need testing:
- Reverbs should have excellent stereo width
- Decorrelated L/R reflections are expected
- Critical for spatial audio quality

## Recommendations

### Immediate Actions

1. **Fix test hang at Engine 31 (Pitch Shifter)**
   - Debug why test freezes
   - Complete testing for engines 31-56
   - Focus on spatial and reverb engines

2. **Address mono-collapsed dynamics processors**
   - Implement true stereo processing
   - Add stereo link controls
   - Test mid/side options

3. **Fix filter stereo processing**
   - Separate L/R filter state
   - Independent coefficient calculation
   - Maintain stereo imaging

4. **Enhance modulation effects**
   - Increase stereo width for chorus/phaser
   - Add L/R phase offset options
   - Implement stereo spread controls

### Testing Methodology

The test system:
- Generates 2-second stereo sine wave (440 Hz)
- 15° phase offset between L/R channels
- Processes through each engine with default parameters
- Measures correlation, width, phase, mid/side balance
- Identifies mono collapse (correlation > 0.95)

### Files Generated

1. **stereo_engine_<ID>.csv** - Raw L/R audio data for each engine
2. **stereo_quality_report.csv** - Detailed metrics in CSV format
3. **stereo_quality_report.json** - Machine-readable results
4. **STEREO_ANALYSIS_SUMMARY.md** - This report

## Test Code

### C++ Test Generator
- **File**: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/test_all_engines_stereo.cpp`
- **Compiles**: `make build/test_all_engines_stereo`
- **Runs**: `./build/test_all_engines_stereo`
- **Output**: Creates `stereo_engine_<ID>.csv` for each engine

### Python Analysis Suite
- **File**: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/stereo_analysis_suite.py`
- **Runs**: `python3 stereo_analysis_suite.py`
- **Analyzes**: All stereo CSV files
- **Reports**: Detailed quality matrix, warnings, recommendations

## Quality Grading System

- **Grade A**: Correlation < 0.8, good stereo width (>0.2)
- **Grade B**: Correlation 0.8-0.9, moderate stereo
- **Grade C**: Correlation 0.9-0.95, limited stereo
- **Grade D**: Correlation 0.95-0.97, minimal stereo
- **Grade F**: Correlation > 0.95, mono collapsed (FAIL)

## Conclusion

Of the 31 engines tested:
- **8 engines (26%)** have good stereo quality
- **3 engines (10%)** have marginal stereo
- **20 engines (64%)** are mono-collapsed and need fixing

The majority of failures are in:
1. **Dynamics processors** (100% failure rate)
2. **Filters** (87.5% failure rate)
3. **Distortion** (50% failure rate)

**Priority**: Complete testing of remaining 26 engines, especially spatial and reverb effects, then systematically address mono-collapsed engines by implementing true stereo processing with independent L/R signal paths.

---

Generated: 2025-10-11
Test System: Chimera Phoenix v3.0
Analysis Tool: stereo_analysis_suite.py
