// CRITICAL FIXES NEEDED FOR PITCHSHIFTER

/*
ISSUES FOUND:
1. Gain reduction when Mix = 100% - outputScale is wrong
2. Parameters not working - dynamic hop size breaks overlap-add
3. Mismatch between dynamic hop in processChannel and fixed hop in scatterFrame

ROOT CAUSE:
- Line 282: dynamicHopSize changes based on grain parameter
- Line 170: scatterFrame uses fixed HOP_SIZE
- Line 214: outputScale assumes fixed overlap factor
- This breaks the overlap-add reconstruction!

FIXES NEEDED:
*/

// FIX 1: Remove dynamic hop size (it's breaking everything)
// Replace line 280-286 with:
/*
    // Process frame when ready
    if (ch.hopCounter >= HOP_SIZE) {
        ch.hopCounter = 0;
        processSpectralFrame(ch, pitch, formant, gate, window);
    }
*/

// FIX 2: Fix output scaling
// Replace line 214 with:
/*
    outputScale = 1.0f / (FFT_SIZE * OVERLAP_FACTOR);  // Remove the 0.5f
*/

// FIX 3: Make grain parameter do something else (not hop size)
// The grain parameter should control grain window shape or crossfade, not hop size
// Dynamic hop size breaks the mathematics of overlap-add!

// TEMPORARY QUICK FIX - Just disable the broken grain parameter:
/*
Replace lines 280-286:

OLD (BROKEN):
    // Process frame when ready (using dynamic hop size based on grain)
    // Grain parameter controls hop size: 0.0 = tight (1/8), 1.0 = loose (1/2)
    const int dynamicHopSize = FFT_SIZE / (8 - static_cast<int>(grain * 6));
    if (ch.hopCounter >= dynamicHopSize) {
        ch.hopCounter = 0;
        processSpectralFrame(ch, pitch, formant, gate, window);
    }

NEW (FIXED):
    // Process frame when ready (fixed hop size for correct overlap-add)
    if (ch.hopCounter >= HOP_SIZE) {
        ch.hopCounter = 0;
        // Note: grain parameter temporarily disabled - was breaking overlap-add
        processSpectralFrame(ch, pitch, formant, gate, window);
    }
*/

// Also fix the output scale:
/*
Line 214:
OLD: outputScale = 1.0f / (FFT_SIZE * OVERLAP_FACTOR * 0.5f);
NEW: outputScale = 1.0f / (FFT_SIZE * OVERLAP_FACTOR * 2.0f);
*/