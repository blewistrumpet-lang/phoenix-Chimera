# Parameter System Issue Analysis
## Critical Problems Found

---

## 🔴 MAJOR DISCOVERY: Parameter Routing Issues

### 1. **PitchShifter Parameters NOT WORKING**

Testing revealed that the PitchShifter's main parameters have NO EFFECT:
- **Pitch (param 0)**: Changes from -2 octaves to +2 octaves = NO AUDIO CHANGE
- **Formant (param 1)**: NO EFFECT
- **Gate (param 4)**: NO EFFECT  
- **Grain (param 5)**: NO EFFECT
- **Feedback (param 6)**: NO EFFECT
- **Width (param 7)**: NO EFFECT

Only Mix and Window parameters work!

### 2. **Root Cause Analysis**

The DSP code IS correct and DOES use the pitch parameter:
```cpp
// Line 409 in PitchShifter.cpp
const double shiftedFreq = ch.frequency[bin] * pitch;
```

But something is preventing the parameters from reaching the DSP or being processed correctly.

### 3. **Possible Issues**

#### A. FFT Not Initialized?
The FFT needs to be created in prepareToPlay:
```cpp
void prepareToPlay(double sr, int) {
    // Are the FFT objects being created?
    for (auto& ch : channels) {
        ch.fft = std::make_unique<juce::dsp::FFT>(FFT_ORDER);
    }
}
```

#### B. Formant Shift Breaking Magnitude Array?
Line 398-402: The formant shift populates shiftedMag, but if formant != 1.0, bins might not get copied:
```cpp
for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
    const int targetBin = static_cast<int>(bin * formant + 0.5f);
    if (targetBin >= 0 && targetBin <= FFT_SIZE/2) {
        shiftedMag[targetBin] += ch.magnitude[bin];  // What if targetBin != bin?
    }
}
```

#### C. Phase Vocoder Not Working?
The phase vocoder might not be accumulating phases correctly, causing silent output.

#### D. Window Functions Wrong?
If the windows aren't normalized properly, the overlap-add might cancel out.

---

## 🔍 OTHER ENGINE PARAMETER ISSUES

### Engines with Non-Functional Parameters:
Based on the test results, many engines have parameters that don't affect the audio:

1. **PitchShifter**: 6 of 8 parameters broken
2. **IntelligentHarmonizer**: Likely same issues as PitchShifter
3. **Many modulation effects**: Some parameters show "SAME AT 0 AND 1"

### Engines That Work:
- **ClassicCompressor**: All parameters functional
- **ParametricEQ**: Most parameters working
- **BitCrusher**: Parameters affect audio

---

## 🎯 CRITICAL FIX NEEDED

### For PitchShifter Specifically:

1. **Check FFT initialization**
```cpp
// In prepareToPlay, ensure FFT is created:
ch.fft = std::make_unique<juce::dsp::FFT>(FFT_ORDER);
```

2. **Fix formant shift logic**
```cpp
// If formant == 1.0, just copy magnitudes directly
if (std::abs(formant - 1.0f) < 0.001f) {
    shiftedMag = ch.magnitude;
} else {
    // Do the formant shift
}
```

3. **Debug pitch parameter flow**
```cpp
// Add logging to verify pitch value:
DBG("Pitch ratio: " << pitch);
```

4. **Check initial parameter values**
The default values might be causing issues:
```cpp
pitchRatio.setImmediate(1.0f);  // This is unison, no shift
```

---

## 🔧 IMMEDIATE ACTION ITEMS

### 1. Fix PitchShifter FFT Initialization
Check if FFT objects are being created properly.

### 2. Test with Simple Pitch Shift
Bypass the formant shift and test just pitch:
```cpp
// Simplified test - just shift frequency
for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
    const double shiftedFreq = ch.frequency[bin] * pitch;
    // Reconstruct directly without formant
}
```

### 3. Add Parameter Value Logging
Log what values are actually reaching the DSP:
```cpp
void updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        DBG("Param " << index << " = " << value);
    }
}
```

### 4. Check Parameter Display in UI
The UI might be sending wrong values or the display might be wrong.

---

## 💡 WHY THIS MATTERS

Without working pitch parameters:
- Users can't actually pitch shift audio
- The "Pitch Shifter" engine is useless
- Musical interval mapping we fixed doesn't matter
- The plugin appears broken in Logic

This is a **CRITICAL BUG** that makes major features non-functional.

---

## 🚨 TESTING REVEALS

Our test shows clear evidence:
```
=== Testing PitchShifter (ID: 31) ===
  Param 0 (Pitch): SAME AT 0 AND 1  <-- BROKEN!
  Param 2 (Mix): OK (change: 74.18 -> 260.79)  <-- WORKING
```

When pitch goes from 0.0 to 1.0 (which should be -24 to +24 semitones), there's NO CHANGE in the audio. This confirms the user's report that "the numerical values for the pitch shifting are meaningless."

---

*The parameter system has fundamental bugs that prevent core functionality from working.*