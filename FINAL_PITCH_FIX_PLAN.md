# Final Fix for PitchShifter Parameters

## ❌ Current Status: STILL BROKEN

Despite fixing the formant parameter mapping and default values, the PitchShifter parameters still don't work. The test shows:
- Pitch parameter: NO EFFECT
- Formant parameter: NO EFFECT  
- Only Mix and Window work

## 🔍 Deep Dive Analysis

### The Real Problem
Looking at the `shiftSpectrum` function (line 392-429 in PitchShifter.cpp):

```cpp
void shiftSpectrum(ChannelState& ch, float pitch, float formant) {
    alignas(16) std::array<std::complex<float>, FFT_SIZE> shifted{};
    alignas(16) std::array<float, FFT_SIZE/2 + 1> shiftedMag{};
    
    // Formant shift populates shiftedMag
    for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
        const int targetBin = static_cast<int>(bin * formant + 0.5f);
        if (targetBin >= 0 && targetBin <= FFT_SIZE/2) {
            shiftedMag[targetBin] += ch.magnitude[bin];
        }
    }
    
    // Pitch shift only processes bins with magnitude
    for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
        if (shiftedMag[bin] > 1e-10f) {  // <-- PROBLEM!
            // Pitch shifting happens here
        }
    }
}
```

### The Bug
If `formant != 1.0`, the bins get remapped. For example:
- If formant = 1.25, bin 100 maps to bin 125
- But then the pitch shift loop checks shiftedMag[100] which is now empty!
- The pitch shifting code never runs for most bins

## 🔴 CRITICAL FIX NEEDED

The formant and pitch operations are intertwined incorrectly. We need to separate them:

```cpp
void shiftSpectrum(ChannelState& ch, float pitch, float formant) {
    alignas(16) std::array<std::complex<float>, FFT_SIZE> shifted{};
    
    // First apply pitch shift to phase
    for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
        if (ch.magnitude[bin] > 1e-10f) {
            // Calculate pitched frequency
            const double shiftedFreq = ch.frequency[bin] * pitch;
            ch.phaseSum[bin] += 2.0 * M_PI * shiftedFreq * HOP_SIZE / sampleRate;
            ch.phaseSum[bin] = std::fmod(ch.phaseSum[bin] + M_PI, 2.0 * M_PI) - M_PI;
        }
    }
    
    // Then apply formant shift to magnitude envelope
    for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
        const int sourceBin = static_cast<int>(bin / formant + 0.5f);
        if (sourceBin >= 0 && sourceBin <= FFT_SIZE/2) {
            float mag = ch.magnitude[sourceBin];
            float phase = static_cast<float>(ch.phaseSum[sourceBin]);
            shifted[bin] = std::polar(mag, phase);
            
            if (bin > 0 && bin < FFT_SIZE/2) {
                shifted[FFT_SIZE - bin] = std::conj(shifted[bin]);
            }
        }
    }
    
    ch.spectrum = shifted;
}
```

## 🎯 Alternative Quick Fix

For immediate testing, just bypass formant shift when it's 1.0:

```cpp
void shiftSpectrum(ChannelState& ch, float pitch, float formant) {
    alignas(16) std::array<std::complex<float>, FFT_SIZE> shifted{};
    
    if (std::abs(formant - 1.0f) < 0.001f) {
        // No formant shift - just do pitch
        for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
            if (ch.magnitude[bin] > 1e-10f) {
                const double shiftedFreq = ch.frequency[bin] * pitch;
                ch.phaseSum[bin] += 2.0 * M_PI * shiftedFreq * HOP_SIZE / sampleRate;
                ch.phaseSum[bin] = std::fmod(ch.phaseSum[bin] + M_PI, 2.0 * M_PI) - M_PI;
                
                const float mag = ch.magnitude[bin];
                const float phase = static_cast<float>(ch.phaseSum[bin]);
                shifted[bin] = std::polar(mag, phase);
                
                if (bin > 0 && bin < FFT_SIZE/2) {
                    shifted[FFT_SIZE - bin] = std::conj(shifted[bin]);
                }
            }
        }
    } else {
        // Original code for formant shift
        // ... existing code ...
    }
    
    ch.spectrum = shifted;
}
```

## 📋 Action Items

1. **Fix the shiftSpectrum function** - Separate pitch and formant operations
2. **Test with formant = 1.0** - Ensure pitch works when formant is disabled
3. **Add debug output** - Log pitch values to verify they're reaching DSP
4. **Check phase accumulator** - Ensure phaseSum is being updated correctly
5. **Verify FFT reconstruction** - Make sure spectrum is being rebuilt properly

## 💡 Why This Is Hard

The phase vocoder implementation is complex:
- Pitch shifting changes frequency (phase increment)
- Formant shifting changes spectral envelope (magnitude distribution)
- These should be independent operations but are mixed together
- The current code makes formant shift affect which bins get pitch shifted

## 🚨 User Impact

Without this fix:
- PitchShifter doesn't shift pitch at all
- IntelligentHarmonizer likely has same issue
- Musical interval display is meaningless
- Users think the plugin is broken

**This is the #1 priority bug to fix!**