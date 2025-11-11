# Remaining Tasks Strategy - 3 Plans Per Task

## Task 1: Remove Test Files (6 remaining)

### Plan A: Simple Deletion
```bash
rm -f test_*.cpp Test*.cpp Test*.h
```
**Pros:** Quick, simple
**Cons:** No backup, might lose useful test code

### Plan B: Move to Tests Directory
```bash
mkdir -p ../Tests/preserved
mv test_*.cpp Test*.cpp Test*.h ../Tests/preserved/
```
**Pros:** Preserves tests for future use
**Cons:** Still clutters repository

### Plan C: Archive and Document ✅ **BEST**
```bash
# Document what each test does
grep -H "^//" test_*.cpp Test*.cpp > docs/test_file_purposes.txt
# Archive for reference
tar czf ../test_files_backup.tar.gz test_*.cpp Test*.cpp Test*.h
# Remove from source
rm -f test_*.cpp Test*.cpp Test*.h
```
**Pros:** Clean removal with documentation and backup
**Cons:** Takes slightly longer
**Why Best:** Preserves knowledge while cleaning codebase

## Task 2: Fix 9 Broken Engines

### Plan A: Fix All Simultaneously
- Open all 9 engine files
- Apply similar fixes across all
- Test together
**Pros:** Might find common patterns
**Cons:** Complex, hard to track issues

### Plan B: Fix by Severity
1. Critical: IntelligentHarmonizer, SMBPitchShift (complex algorithms)
2. Medium: BitCrusher, PitchShifter_Platinum (buffer issues)
3. Simple: BufferRepeat, Others (timing/parameter issues)
**Pros:** Tackles hard problems first
**Cons:** Might get stuck on complex ones

### Plan C: Fix by Category with Isolation Testing ✅ **BEST**
```cpp
Group 1 - Denormal/Smoothing Issues (Quick fixes):
  - BitCrusher (add denormal protection)
  - KStyleOverdrive_Platinum (smoothing parameters)

Group 2 - Buffer/Timing Issues (Medium):
  - BufferRepeat & BufferRepeat_Platinum (buffer logic)
  - PitchShifter_Platinum (buffer management)

Group 3 - Algorithm Issues (Complex):
  - IntelligentHarmonizer (PSOLA algorithm)
  - SMBPitchShift (pitch detection)
  - HarmonicExciter_Platinum (harmonic generation)
  - ChaosGenerator_Platinum (stability)
```
**Why Best:** Groups similar issues, easiest fixes first build confidence, each fix is testable

## Task 3: Apply Safety Wrappers to All 57 Engines

### Plan A: Manual Wrapping
- Edit each engine file individually
- Change base class to SafeEngineBase
- Add error handling manually
**Pros:** Full control
**Cons:** Time-consuming, error-prone

### Plan B: Script-Based Conversion
```python
# Python script to auto-convert
for engine_file in engine_files:
    replace("public EngineBase", "public SafeEngineBase")
    add_safety_checks()
```
**Pros:** Fast, consistent
**Cons:** Might break unique implementations

### Plan C: Incremental Safety with Validation ✅ **BEST**
```cpp
Phase 1: Create SafeEngineAdapter (non-invasive wrapper)
class SafeEngineAdapter : public SafeEngineBase {
    std::unique_ptr<EngineBase> wrappedEngine;
    void process(AudioBuffer& buffer) override {
        // Safety checks
        if (validateBuffer(buffer)) {
            wrappedEngine->process(buffer);
            sanitizeOutput(buffer);
        }
    }
};

Phase 2: Test each engine through adapter
Phase 3: Gradually migrate engines to inherit SafeEngineBase directly
Phase 4: Remove adapter once all migrated
```
**Why Best:** Non-breaking, testable at each step, can rollback if issues

## Task 4: Organize File Structure

### Plan A: Immediate Full Reorganization
```bash
mkdir -p Core Engines/{Dynamics,Reverb,Delay,Filter,Modulation,Distortion,Pitch,Spatial,Utility} UI GPIO Utils
# Move all files immediately
```
**Pros:** Clean structure immediately
**Cons:** Breaks all includes, massive change

### Plan B: Keep Flat Structure
- Leave all files in Source/
- Use naming conventions only
**Pros:** No include changes needed
**Cons:** Messy, hard to navigate

### Plan C: Gradual Migration with Symlinks ✅ **BEST**
```bash
# Step 1: Create structure
mkdir -p Core Engines UI GPIO Utils

# Step 2: Move and symlink back (maintains compatibility)
mv PluginProcessor.* Core/
ln -s Core/PluginProcessor.* .

# Step 3: Update includes gradually
# Step 4: Remove symlinks once all updated
```
**Why Best:** Non-breaking, can be done incrementally, maintains working builds

## Recommended Execution Order

### Day 1: Foundation (Today)
1. **Archive test files** (Plan C) - 15 minutes
2. **Start file organization** (Plan C) - 1 hour
3. **Fix Group 1 engines** (BitCrusher, KStyleOverdrive) - 2 hours

### Day 2: Core Fixes
1. **Fix Group 2 engines** (Buffer/Timing) - 3 hours
2. **Create SafeEngineAdapter** - 2 hours
3. **Test adapter with 5 engines** - 1 hour

### Day 3: Complex Fixes
1. **Fix Group 3 engines** (Complex algorithms) - 4 hours
2. **Apply adapter to all working engines** - 2 hours

### Day 4: Integration
1. **Complete file organization** - 2 hours
2. **Start migrating engines to SafeEngineBase** - 3 hours
3. **Build and test on Mac** - 1 hour

### Day 5: Validation
1. **Build and test on Pi** - 2 hours
2. **Performance profiling** - 2 hours
3. **Final documentation** - 1 hour

## Success Metrics
- [ ] Zero test files in Source/
- [ ] All 57 engines compile
- [ ] All 57 engines process audio without crashes
- [ ] Safety system reports no critical errors
- [ ] CPU usage < 20% with 4 engines
- [ ] Clean builds on Mac and Pi
- [ ] Organized file structure

## Risk Mitigation
- **Backup before each major change**
- **Test after each engine fix**
- **Keep adapter pattern as fallback**
- **Document all changes in git commits**

## Let's Begin!
Starting with Task 1, Plan C - Archive and document test files...