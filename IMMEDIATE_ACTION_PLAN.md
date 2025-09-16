# Immediate Action Plan - Chimera Phoenix Recovery

## Priority 1: Get Everything Working (Simple But Functional)

### Step 1: Create Shared Simple Pitch Shifter
Since IntelligentHarmonizer's low-latency mode WORKS, let's extract and reuse it:

```cpp
// SimplePitchShift.h - Shared implementation
class SimplePitchShift {
    static constexpr int BUFFER_SIZE = 8192;
    std::vector<float> delayBuffer;
    float readPos = 0.0f;
    int writePos = 0;
    
public:
    void reset() {
        std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
        readPos = 0.0f;
        writePos = 0;
    }
    
    void process(const float* input, float* output, int numSamples, float pitchRatio) {
        for (int i = 0; i < numSamples; ++i) {
            // Write to circular buffer
            delayBuffer[writePos] = input[i];
            writePos = (writePos + 1) % BUFFER_SIZE;
            
            // Read with interpolation
            int readIdx = static_cast<int>(readPos);
            float frac = readPos - readIdx;
            
            int idx0 = readIdx % BUFFER_SIZE;
            int idx1 = (readIdx + 1) % BUFFER_SIZE;
            
            output[i] = delayBuffer[idx0] * (1.0f - frac) + delayBuffer[idx1] * frac;
            
            // Advance read position based on pitch ratio
            readPos += 1.0f / pitchRatio;
            if (readPos >= BUFFER_SIZE) readPos -= BUFFER_SIZE;
        }
    }
};
```

### Step 2: Fix Each Engine Systematically

#### A. PitchShifter - Make It Work NOW
1. Add SimplePitchShift member
2. Use it instead of Signalsmith
3. Test that audio passes through
4. Worry about quality later

#### B. ShimmerReverb - Two Quick Fixes
1. **Fix Reverb Decay:**
   ```cpp
   // Change this:
   const float baseFeedback = 0.4f + 0.15f * size01;
   
   // To this:
   const float baseFeedback = 0.4f + 0.55f * size01;  // Up to 0.95 feedback
   ```

2. **Add Simple Pitch Shift for Shimmer:**
   - Replace Signalsmith with SimplePitchShift
   - Just make it work, optimize later

#### C. PhasedVocoder - Fix Latency
1. Report correct latency: `return FFT_SIZE;`
2. Prime with zeros on reset
3. Test thoroughly

### Step 3: Verification Protocol

For EACH engine fix:
1. **Before**: Run test, document failure
2. **Change**: Make ONE change
3. **After**: Run test, verify improvement
4. **Commit**: If better, commit immediately

### Step 4: Testing Commands

```bash
# Test individual engine
./test_engine_name

# Test all engines
./test_all_engines_status

# Test with real audio
./test_with_sine_sweep
```

## What NOT To Do Right Now

❌ **DON'T** try to fix Signalsmith - it's a rabbit hole
❌ **DON'T** optimize for quality yet
❌ **DON'T** refactor architecture 
❌ **DON'T** add new features
❌ **DON'T** change working engines

## Success Criteria for Today

✅ PitchShifter outputs audio (any quality)
✅ ShimmerReverb has audible reverb tail
✅ ShimmerReverb has some pitch shift effect
✅ No engines output silence
✅ All changes committed with clear messages

## Order of Operations

1. **Hour 1**: Create SimplePitchShift.h
2. **Hour 2**: Fix PitchShifter
3. **Hour 3**: Fix ShimmerReverb reverb
4. **Hour 4**: Fix ShimmerReverb shimmer
5. **Hour 5**: Test everything
6. **Hour 6**: Document and commit

## Fallback Plan

If any fix breaks things worse:
```bash
git stash              # Save attempted fix
git checkout HEAD~1    # Revert to previous
# Try different approach
```

## Remember

**Goal**: Everything works (even if poorly)
**Non-goal**: Everything works perfectly

Once we have a fully functional baseline, we can improve quality incrementally without breaking functionality.

---

Ready to proceed? Start with Step 1: Create SimplePitchShift.h