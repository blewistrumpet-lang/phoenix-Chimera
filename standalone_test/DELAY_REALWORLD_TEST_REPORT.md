# REAL-WORLD DELAY ENGINE TEST REPORT

**Test Date:** 11 Oct 2025 5:55:05pm
**Sample Rate:** 48000 Hz
**Test Duration:** 10 seconds per engine

## Executive Summary

Comprehensive real-world testing of delay engines with musical materials.

| Engine | Grade | Timing | Feedback | Stereo | Status |
|--------|-------|--------|----------|--------|--------|
| Digital_Delay | D | 2/5 | 6/6 | 0.00 | NOT RECOMMENDED - Significant issues detected |
| Bucket_Brigade_Delay | F | 0/5 | 6/6 | 0.00 | NOT RECOMMENDED - Significant issues detected |

## Detailed Results

### Digital_Delay

**Grade:** D

**Production Readiness:** NOT RECOMMENDED - Significant issues detected

#### Timing Accuracy

| Target (ms) | Measured (ms) | Error (ms) | Error (%) | Status |
|-------------|---------------|------------|-----------|--------|
| 50.0 | 51.0 | 1.00 | 2.00 | PASS |
| 250.0 | 251.2 | 1.23 | 0.49 | FAIL |
| 500.0 | 501.8 | 1.77 | 0.35 | FAIL |
| 1000.0 | 999.7 | -0.31 | -0.03 | PASS |
| 2000.0 | -2.1 | -2002.08 | -100.10 | FAIL |

#### Feedback Stability

| Feedback (%) | Stable | Max Peak | Avg Energy | Notes |
|--------------|--------|----------|------------|-------|
| 0.00 | YES | 0.105 | 0.000 |  |
| 25.000 | YES | 0.105 | 0.000 |  |
| 50.000 | YES | 0.105 | 0.000 |  |
| 75.000 | YES | 0.105 | 0.000 |  |
| 90.000 | YES | 0.105 | 0.000 |  |
| 95.000 | YES | 0.105 | 0.000 |  |

#### Audio Quality

- **Stereo Width:** 0.000
- **Parameter Clicks:** None
- **Filter Character:** 

---

### Bucket_Brigade_Delay

**Grade:** F

**Production Readiness:** NOT RECOMMENDED - Significant issues detected

#### Timing Accuracy

| Target (ms) | Measured (ms) | Error (ms) | Error (%) | Status |
|-------------|---------------|------------|-----------|--------|
| 50.0 | 21.3 | -28.69 | -57.38 | FAIL |
| 250.0 | 21.3 | -228.69 | -91.47 | FAIL |
| 500.0 | 21.3 | -478.69 | -95.74 | FAIL |
| 1000.0 | 21.3 | -978.69 | -97.87 | FAIL |
| 2000.0 | 21.3 | -1978.69 | -98.93 | FAIL |

#### Feedback Stability

| Feedback (%) | Stable | Max Peak | Avg Energy | Notes |
|--------------|--------|----------|------------|-------|
| 0.00 | YES | 0.072 | 0.000 |  |
| 25.000 | YES | 0.072 | 0.000 |  |
| 50.000 | YES | 0.072 | 0.000 |  |
| 75.000 | YES | 0.072 | 0.000 |  |
| 90.000 | YES | 0.072 | 0.000 |  |
| 95.000 | YES | 0.072 | 0.000 |  |

#### Audio Quality

- **Stereo Width:** 0.000
- **Parameter Clicks:** None
- **Filter Character:** 

---

## Audio Test Files

Audio test files saved to: `/standalone_test/delay_audio_tests/`

Files generated for each engine:
- `[engine]_guitar.wav` - Clean picked guitar with delay
- `[engine]_vocals.wav` - Rhythmic vocals with delay
- `[engine]_drums.wav` - Drum pattern with delay

## Conclusions

**Best Performing Engine:** Digital_Delay (Grade: D)

**Key Findings:**
- Timing accuracy is critical for musical applications
- Feedback stability must be rock-solid up to 95%
- Parameter smoothing prevents clicks
- Stereo imaging enhances spatial perception

