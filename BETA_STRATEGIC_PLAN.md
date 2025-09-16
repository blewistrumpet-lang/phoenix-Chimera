# Beta Strategic Plan - Chimera Phoenix v3.0

## Philosophy
**"Working First, Quality Later, Architecture Always"**

We'll implement simple but functional solutions with proper interfaces so we can upgrade components without breaking everything.

## Strategic Quick Proper Fix - 3 Phase Approach

### Phase 1: Architecture (Today)
Create the RIGHT interfaces even if implementations are simple:

```cpp
// PitchShiftStrategy.h - Proper interface for future upgrades
class IPitchShiftStrategy {
public:
    virtual ~IPitchShiftStrategy() = default;
    virtual void prepare(double sampleRate, int blockSize) = 0;
    virtual void reset() = 0;
    virtual void process(const float* in, float* out, int samples, float pitchRatio) = 0;
    virtual int getLatencySamples() const = 0;
    virtual const char* getName() const = 0;
    virtual bool isHighQuality() const = 0;
};

// SimplePitchShift.h - Beta quality implementation
class SimplePitchShift : public IPitchShiftStrategy {
    // Our working delay-buffer implementation
    int getLatencySamples() const override { return 0; }
    const char* getName() const override { return "Simple (Beta)"; }
    bool isHighQuality() const override { return false; }
};

// Future implementations (post-beta)
class SignalsmithPitchShift : public IPitchShiftStrategy { };  // With latency compensation
class PSOLAPitchShift : public IPitchShiftStrategy { };        // Higher quality
class RubberBandPitchShift : public IPitchShiftStrategy { };   // Professional
```

### Phase 2: Implementation (Next 2-3 Days)

#### Day 1: Core Infrastructure
1. Create IPitchShiftStrategy interface
2. Implement SimplePitchShift (we already have the code)
3. Create PitchShiftFactory for easy swapping

#### Day 2: Fix Engines
1. **PitchShifter**: Use IPitchShiftStrategy
2. **IntelligentHarmonizer**: Refactor to use IPitchShiftStrategy
3. **ShimmerReverb**: 
   - Use IPitchShiftStrategy for shimmer
   - Fix feedback range (0.4-0.55 → 0.4-0.95)

#### Day 3: Testing & Documentation
1. Test all pitch engines
2. Document beta limitations
3. Create upgrade path documentation

### Phase 3: Beta Polish (Next Week)

1. **Engine Audit**: Test all 57 engines, mark as:
   - ✅ Production Ready
   - 🎯 Beta Functional  
   - ⚠️ Known Issues
   - ❌ Not Working

2. **Documentation**:
   ```markdown
   ## Beta Status
   
   ### Pitch Engines
   - Quality: Beta (simple interpolation)
   - Latency: Zero
   - Artifacts: Some aliasing at extreme settings
   - Upgrade Path: IPitchShiftStrategy allows drop-in replacement
   ```

3. **User Expectations**:
   - Add "BETA" badge to UI for affected engines
   - Clear documentation of limitations
   - Promise of future upgrades

## Implementation Order

### Immediate (Today):
1. ✅ Stop trying to fix Signalsmith
2. ✅ Accept simple pitch shifting for beta
3. 🔄 Create proper interface architecture
4. 🔄 Implement SimplePitchShift with interface

### Tomorrow:
1. Fix PitchShifter
2. Fix ShimmerReverb reverb decay
3. Fix ShimmerReverb shimmer
4. Test and verify

### This Week:
1. Audit all 57 engines
2. Fix any other broken engines with simple solutions
3. Document everything honestly

## Success Metrics for Beta

### Must Have:
- ✅ All engines produce audio (any quality)
- ✅ No crashes or hangs
- ✅ Parameters respond predictably
- ✅ Clean architecture for upgrades

### Nice to Have:
- Good quality (can wait for v3.1)
- Zero latency (mostly achieved with simple approach)
- Professional features (post-beta)

### Not Required for Beta:
- Studio quality algorithms
- Latency compensation
- Advanced features

## Code Quality Standards

### For Beta:
```cpp
// ACCEPTABLE for Beta
class SimplePitchShift {
    // Simple, working, documented limitations
    // Clear upgrade path via interface
};
```

### NOT Acceptable:
```cpp
// BAD - No upgrade path
void processAudio(float* data) {
    // Hardcoded simple algorithm with no abstraction
}
```

## Risk Mitigation

1. **Use Interfaces**: Even if implementation is simple
2. **Document Limitations**: Be honest about beta quality
3. **Test Everything**: Better to know what's broken
4. **Version Control**: Tag beta releases clearly
5. **User Communication**: Set expectations properly

## Timeline

- **Week 1**: Core fixes (pitch engines, reverb)
- **Week 2**: Full engine audit and fixes
- **Week 3**: Documentation and beta release
- **Future**: Upgrade implementations via interfaces

## The Promise

By using proper interfaces now, we can:
1. Ship a working beta quickly
2. Upgrade quality without breaking changes
3. Keep architecture clean
4. Not waste time on complex implementations that aren't working

## Next Concrete Step

Create IPitchShiftStrategy.h with:
1. Proper virtual interface
2. SimplePitchShift implementation
3. Factory for easy instantiation
4. Clear documentation of limitations

Ready to proceed with this approach?