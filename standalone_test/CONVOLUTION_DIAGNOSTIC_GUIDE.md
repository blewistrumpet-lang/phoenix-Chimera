# ConvolutionReverb Diagnostic Output Guide

This guide explains the diagnostic output from ConvolutionReverb and how to interpret it.

---

## Normal Operation

### IR Generation Output
```
ConvolutionReverb: Final IR - Length=144000, Peak=0.78, RMS=0.023, NonZero=95.3%
```

**What this means:**
- `Length=144000`: IR is 3 seconds at 48kHz (144000 samples) - GOOD
- `Peak=0.78`: Maximum sample value is 0.78 - GOOD (0.5-0.9 is ideal)
- `RMS=0.023`: Average energy is 0.023 - GOOD (0.01-0.05 is typical)
- `NonZero=95.3%`: 95.3% of samples have content - EXCELLENT (>80% is good)

### Processing Output
```
ConvolutionReverb: Input=0.5, Output=0.42, Latency=256
```

**What this means:**
- `Input=0.5`: Input signal peak is 0.5 - GOOD
- `Output=0.42`: Output after convolution is 0.42 - GOOD (similar to input)
- `Latency=256`: Convolution introduces 256 samples delay - NORMAL

**Healthy ratio:** Output should be 0.5-1.5x of input (accounting for reverb energy)

---

## Problem Indicators

### Issue #1: Weak IR Generation
```
ConvolutionReverb ERROR: Generated IR is too weak or empty! Peak=0.0001, RMS=0.00001
```

**Cause:** IR generation failed completely
**Action:** Emergency impulse fallback activated
**Impact:** Will work but no reverb character

### Issue #2: Destroyed IR
```
ConvolutionReverb ERROR: Final IR is destroyed! Using emergency impulse.
ConvolutionReverb: Final IR - Length=144000, Peak=0.00005, RMS=0.000001, NonZero=0.1%
```

**Cause:** IR filtering (damping/brightness) destroyed the IR
**Action:** Emergency exponential decay fallback activated
**Impact:** Basic reverb but not full algorithmic character

**What to check:**
- Is damping parameter too high? (>0.9)
- Are multiple filters stacked?
- Check brightness parameter

### Issue #3: Zero Output During Processing
```
ConvolutionReverb WARNING: Input present but output is zero!
ConvolutionReverb: Input=0.5, Output=0.00001, Latency=256
```

**Cause:** Convolution engine is not producing output
**Possible reasons:**
1. IR wasn't loaded properly
2. Convolution engine not initialized
3. IR has no content

**Action:**
1. Check earlier IR generation messages
2. Verify IR NonZero% > 80%
3. Restart engine (call reset())

---

## Parameter Effects on Diagnostics

### Damping Parameter (0.0 - 1.0)

| Damping | Expected IR NonZero% | Expected Peak | Notes |
|---------|---------------------|---------------|-------|
| 0.0 | 95-99% | 0.7-0.9 | No filtering, full spectrum |
| 0.5 | 90-95% | 0.6-0.8 | Moderate HF roll-off |
| 1.0 | 85-90% | 0.5-0.7 | Heavy HF roll-off |

**Red flag:** If damping > 0.8 and NonZero% < 70%, filter is too aggressive.

### Size Parameter (0.0 - 1.0)

| Size | IR Length (48kHz) | Expected RT60 | Notes |
|------|------------------|---------------|-------|
| 1.0 | 144000 (3s) | 2-3s | Full reverb tail |
| 0.5 | 72000 (1.5s) | 1-1.5s | Medium tail |
| 0.2 | 28800 (0.6s) | 0.3-0.6s | Short tail |

**Red flag:** If Length < 10000 samples, may sound more like delay than reverb.

### IR Select (0-3)

| Index | IR Type | Expected Length | Expected Density |
|-------|---------|----------------|------------------|
| 0 | Concert Hall | 144000 (3s) | 80% (sparse early) |
| 1 | EMT Plate | 96000 (2s) | 95% (very dense) |
| 2 | Stairwell | 192000 (4s) | 60% (very sparse) |
| 3 | Cloud Chamber | 240000 (5s) | 70% (moderate) |

---

## Diagnostic Timeline

### During Initialization
```
1. ConvolutionReverb: Initializing...
2. ConvolutionReverb: Loading IR...
3. ConvolutionReverb: Final IR - Length=144000, Peak=0.78, RMS=0.023, NonZero=95.3%
```

**Good:** All 3 messages appear in order

**Bad:** Missing message #3 = IR wasn't validated/loaded

### During Parameter Change
```
1. ConvolutionReverb: Loading IR...  (triggered by parameter change)
2. ConvolutionReverb: Final IR - Length=72000, Peak=0.65, RMS=0.019, NonZero=92.1%
```

**Note:** Length may change if size parameter changed

### During Processing (Every ~10 seconds)
```
ConvolutionReverb: Input=0.5, Output=0.42, Latency=256
ConvolutionReverb: Input=0.3, Output=0.28, Latency=256
ConvolutionReverb: Input=0.6, Output=0.51, Latency=256
```

**Good:** Output roughly tracks input (ratio 0.5-1.5)

**Bad:** Output always 0.0 = convolution not working

---

## Troubleshooting Guide

### Symptom: No audio output at all

**Check:**
1. Is mix parameter > 0? (Parameter 0)
2. Does IR have content? (NonZero% > 80%?)
3. Is input signal present? (Check Input= value)

**Debug steps:**
```bash
# Test with 100% wet
./standalone_test --engine 41 --parameter 0:1.0 --duration 5.0

# Check logs for:
# - "Final IR" message with good stats
# - "Input=" and "Output=" messages
```

### Symptom: Output very weak

**Check:**
1. IR Peak value (should be > 0.5)
2. Damping parameter (try 0.0)
3. Mix parameter (try 1.0 for testing)

**Debug steps:**
```bash
# Test without filtering
./standalone_test --engine 41 \
    --parameter 0:1.0 \
    --parameter 4:0.0 \
    --duration 5.0
```

### Symptom: No reverb tail (sounds like delay)

**Check:**
1. Size parameter (should be > 0.5 for noticeable tail)
2. IR length in debug output (should be > 48000 samples)
3. RT60 measurement (use external tool)

**Debug steps:**
```bash
# Test with full size
./standalone_test --engine 41 \
    --parameter 0:1.0 \
    --parameter 2:1.0 \
    --duration 10.0
```

### Symptom: "IR destroyed" messages

**Cause:** Damping or brightness filtering too aggressive

**Fix:**
1. Reduce damping parameter to < 0.8
2. Check brightness parameter in IR generation
3. Verify moving average window size

**Prevention:**
The updated code has emergency fallbacks, but optimal performance requires:
- Damping < 0.9
- Reasonable window sizes
- Proper IR validation

---

## Performance Monitoring

### CPU Usage
- IR generation: ~10-50ms (one-time per parameter change)
- Convolution: ~1-5% CPU per instance (depends on IR length)

**Red flag:** If CPU > 20% per instance, IR may be too long

### Memory Usage
- Per instance: ~2-10 MB (depends on IR length)
- IR buffer: Length × 2 channels × 4 bytes

**Example:** 144000 samples × 2 channels × 4 bytes = 1.15 MB

### Latency
- Typical: 128-512 samples (depends on buffer size)
- Reported in debug output: `Latency=256`

**Red flag:** If latency > 2048 samples, check IR length and convolution settings

---

## Expected Values Reference

### IR Statistics (Healthy)
| Metric | Min | Typical | Max | Unit |
|--------|-----|---------|-----|------|
| Peak | 0.5 | 0.7-0.8 | 0.9 | amplitude |
| RMS | 0.01 | 0.02-0.03 | 0.05 | amplitude |
| NonZero% | 80% | 90-95% | 99% | percentage |
| Length | 24000 | 96000-144000 | 240000 | samples |

### Processing (Healthy)
| Metric | Min | Typical | Max | Unit |
|--------|-----|---------|-----|------|
| Output/Input Ratio | 0.5 | 0.8-1.0 | 1.5 | ratio |
| Latency | 64 | 256-512 | 2048 | samples |

---

## Quick Reference: Message Severity

### INFO (Normal Operation)
```
✓ ConvolutionReverb: Final IR - Length=..., Peak=..., RMS=..., NonZero=...%
✓ ConvolutionReverb: Input=..., Output=..., Latency=...
```

### WARNING (Sub-optimal but Working)
```
⚠ ConvolutionReverb WARNING: Input present but output is zero!
⚠ [NonZero% < 80%]
⚠ [Peak < 0.3]
```

### ERROR (Fallback Activated)
```
✗ ConvolutionReverb ERROR: Generated IR is too weak or empty!
✗ ConvolutionReverb ERROR: Final IR is destroyed! Using emergency impulse.
```

---

## Testing Checklist

Use this checklist when verifying ConvolutionReverb:

- [ ] IR generation shows Peak > 0.5
- [ ] IR generation shows RMS > 0.01
- [ ] IR generation shows NonZero% > 80%
- [ ] No "ERROR" messages in logs
- [ ] Processing shows Output > 0 when Input > 0
- [ ] Output/Input ratio between 0.5-1.5
- [ ] All 4 IR types work (parameter 1: 0, 0.25, 0.5, 0.75)
- [ ] Damping parameter works (0.0, 0.5, 1.0)
- [ ] Size parameter works (0.5, 1.0)
- [ ] Output has reverb tail (RT60 > 1 second)

---

## Log Analysis Script

Save this as `analyze_convolution_log.sh`:

```bash
#!/bin/bash
# Analyze ConvolutionReverb diagnostic output

if [ $# -eq 0 ]; then
    echo "Usage: $0 <logfile>"
    exit 1
fi

LOGFILE=$1

echo "=== IR Generation Stats ==="
grep "Final IR" "$LOGFILE" | tail -1

echo ""
echo "=== Processing Stats (last 5) ==="
grep "Input=" "$LOGFILE" | tail -5

echo ""
echo "=== Errors/Warnings ==="
grep -E "ERROR|WARNING" "$LOGFILE"

if [ $? -ne 0 ]; then
    echo "No errors or warnings found ✓"
fi

echo ""
echo "=== Summary ==="
ERRORS=$(grep -c "ERROR" "$LOGFILE")
WARNINGS=$(grep -c "WARNING" "$LOGFILE")
echo "Errors: $ERRORS"
echo "Warnings: $WARNINGS"

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo "Status: PASS ✓"
else
    echo "Status: ISSUES DETECTED ✗"
fi
```

**Usage:**
```bash
chmod +x analyze_convolution_log.sh
./analyze_convolution_log.sh convolution_test.log
```
