// Proposed fix for ChaosGenerator_Platinum.cpp
// Add this to the ModTarget enum and processBlock switch statement

// In the enum ModTarget section (around line 350):
enum ModTarget { 
    ModPitch=0, 
    ModFilter, 
    ModAmp, 
    ModPan, 
    ModGenerate,  // NEW: Direct audio generation
    ModTargetCount 
};

// In processBlock, replace the switch statement (around line 458):
switch (target) {
    case ModPitch:
        // Existing pitch modulation
        wet = dry * (1.0f + 0.05f * depth * mod);
        break;

    case ModFilter: {
        // Existing filter modulation
        const float cutoff = std::clamp(0.5f + 0.4f * depth * mod, 0.05f, 0.98f);
        wet = onePoleLP[ch].process(dry, cutoff);
        break;
    }

    case ModAmp:
        // Existing amplitude modulation
        wet = dry * (1.0f + depth * mod);
        break;

    case ModPan:
        // Existing pan modulation
        if (numCh == 1) {
            wet = dry;
        } else {
            const float pan = 0.5f * depth * mod;
            const float gl = std::clamp(1.0f - pan, 0.0f, 2.0f);
            const float gr = std::clamp(1.0f + pan, 0.0f, 2.0f);
            wet = dry * (ch==0 ? gl : gr);
        }
        break;

    case ModGenerate:
        // NEW: Generate audio directly from chaos
        // This creates sound even from silence!
        wet = dry + (depth * mod * 0.3f);  // Add chaos signal to input
        // OR for pure generation (replace input):
        // wet = depth * mod * 0.3f;  // Pure chaos output
        break;
}

// Also update the parameter mapping in updateParameters (around line 540):
// kModTarget (0..1 -> 0..4 instead of 0..3)
if (params.count(kModTarget)) {
    const float v = std::clamp(get(kModTarget, 0.0f), 0.0f, 1.0f);
    const int t = std::clamp(int(v * (int)Impl::ModTargetCount - 1e-4f), 0, (int)Impl::ModTargetCount-1);
    pImpl->pTarget.store(t, std::memory_order_relaxed);
}