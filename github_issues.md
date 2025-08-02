# GitHub Issues to Create

Copy and paste these issues into GitHub Issues at https://github.com/blewistrumpet-lang/phoenix-Chimera/issues

---

## Issue 1: Fix thread safety violations (remove static variables)

**Labels:** `bug`, `high-priority`

### Description
Multiple engines use static variables in their audio processing threads, which can cause crashes in multi-threaded hosts.

### Affected Files
- `ClassicCompressor.cpp:150`
- `StereoChorus.cpp:89`
- `RodentDistortion.cpp:126`

### Solution
Replace static variables with instance members or use `thread_local` storage.

### Acceptance Criteria
- [ ] All static variables removed from audio processing paths
- [ ] Replaced with thread-safe alternatives
- [ ] Tested in multi-threaded host environment
- [ ] No audio artifacts or crashes

### Time Estimate
4 hours

---

## Issue 2: Fix memory leaks - convert raw pointers to smart pointers

**Labels:** `bug`, `high-priority`, `memory-leak`

### Description
Raw pointer usage with `new` without corresponding `delete` can cause memory leaks.

### Affected Files
- `PresetManager.cpp:145`
- `PluginProcessor.cpp:89`

### Solution
Replace raw pointers with `std::unique_ptr` or `std::shared_ptr` as appropriate.

### Acceptance Criteria
- [ ] All raw `new` calls replaced with smart pointers
- [ ] Memory leak detection shows no leaks
- [ ] RAII principles properly implemented
- [ ] No dangling pointers

### Time Estimate
3 hours

---

## Issue 3: Register StereoWidener engine in factory

**Labels:** `bug`, `quick-fix`

### Description
StereoWidener engine exists but is not registered in EngineFactory, making it unavailable for use.

### Affected Files
- `EngineFactory.cpp:54-210`
- `ParameterDefinitions.h`

### Solution
1. Add `ENGINE_STEREO_WIDENER` case to EngineFactory::createEngine()
2. Add `#include "StereoWidener.h"`
3. Update engine count if needed

### Acceptance Criteria
- [ ] StereoWidener can be instantiated via EngineFactory
- [ ] Engine appears in UI engine selector
- [ ] All parameters work correctly

### Time Estimate
30 minutes

---

## Issue 4: Update EngineFactory to use unified engine types

**Labels:** `refactoring`, `code-quality`

### Description
EngineFactory should use the unified EngineTypes.h definitions instead of local definitions.

### Affected Files
- `EngineFactory.cpp`
- `EngineFactory.h`

### Solution
1. Include EngineTypes.h
2. Remove any duplicate engine type definitions
3. Update all switch cases to use unified constants

### Acceptance Criteria
- [ ] EngineFactory uses only EngineTypes.h constants
- [ ] No duplicate engine type definitions
- [ ] All engines can still be created correctly

### Time Estimate
1 hour

---

## Issue 5: Generate FAISS index from exported JSON presets

**Labels:** `enhancement`, `ai-system`

### Description
Need to generate the FAISS index from the 30 exported Golden Corpus presets for the Oracle AI system.

### Tasks
1. Run `oracle_faiss_indexer.py` on exported presets
2. Verify index contains all 30 presets
3. Test search functionality

### Acceptance Criteria
- [ ] FAISS index successfully generated
- [ ] All 30 presets searchable
- [ ] Oracle service can retrieve presets by similarity
- [ ] Performance is acceptable (<100ms search time)

### Time Estimate
1 hour

---

## Issue 6: Create remaining Golden Corpus presets (GC_031 - GC_250)

**Labels:** `enhancement`, `content-creation`, `golden-corpus`

### Description
Complete the remaining 220 handcrafted presets for the Golden Corpus.

### Guidelines
Each preset should include:
- Scientific/cultural inspiration
- Detailed parameter rationale  
- Sonic and emotional profiles
- Musical context metadata
- Quality score ≥ 90%

### Breakdown
- [ ] GC_031 - GC_050 (20 presets)
- [ ] GC_051 - GC_100 (50 presets)
- [ ] GC_101 - GC_150 (50 presets)
- [ ] GC_151 - GC_200 (50 presets)
- [ ] GC_201 - GC_250 (50 presets)

### Time Estimate
40+ hours (10-15 minutes per preset)

---

## Issue 7: Fix sample rate inconsistencies

**Labels:** `bug`, `medium-priority`

### Description
Different parts of the system use different default sample rates.

### Problem
- Engine headers: 44100.0 Hz
- PresetValidator.h: 48000.0 Hz

### Solution
Standardize on one sample rate (recommend 44100.0) across all components.

### Acceptance Criteria
- [ ] All components use same default sample rate
- [ ] Sample rate changes handled correctly
- [ ] No audio artifacts from rate mismatches

### Time Estimate
1 hour

---

## Issue 8: Implement comprehensive unit tests

**Labels:** `testing`, `code-quality`

### Description
Add unit tests for critical audio processing paths.

### Test Coverage Needed
- [ ] All engine reset() functions
- [ ] Parameter validation 
- [ ] Audio processing edge cases
- [ ] Thread safety tests
- [ ] Memory leak tests
- [ ] Preset loading/saving

### Acceptance Criteria
- [ ] 80%+ code coverage for critical paths
- [ ] All tests pass in CI
- [ ] Tests run in <30 seconds
- [ ] Clear test documentation

### Time Estimate
8 hours

---

## Issue 9: Design and implement plugin UI

**Labels:** `enhancement`, `ui`, `major-feature`

### Description
Create the user interface for the plugin.

### Requirements
- 6-slot engine rack visualization
- Parameter controls for each engine
- Preset browser with search
- AI prompt input field
- Mix controls and meters
- Modern, intuitive design

### Acceptance Criteria
- [ ] UI responds smoothly to parameter changes
- [ ] All engines properly represented
- [ ] Preset browser functional
- [ ] AI integration working
- [ ] Scalable/resizable window

### Time Estimate
40+ hours

---

## Issue 10: Complete Trinity AI integration

**Labels:** `enhancement`, `ai-system`, `major-feature`

### Description
Integrate the Trinity AI pipeline with the plugin UI.

### Components
- [ ] Visionary: Connect to GPT-4 API
- [ ] Oracle: Wire up FAISS search
- [ ] Calculator: Implement blending algorithm
- [ ] Alchemist: Add refinement logic

### Acceptance Criteria
- [ ] Natural language prompts generate presets
- [ ] Preset morphing works smoothly
- [ ] Response time <3 seconds
- [ ] Graceful error handling

### Time Estimate
20+ hours

---

## Issue 11: Add parameter count validation

**Labels:** `bug`, `validation`

### Description
Some engines report incorrect parameter counts.

### Affected Files
- `AnalogPhaser.h` (reports 8 but may use 10)
- `StereoImager.h` (reports 8 but usage varies)

### Solution
Audit all engines and ensure getNumParameters() returns correct count.

### Acceptance Criteria
- [ ] All engines report correct parameter count
- [ ] No out-of-bounds parameter access
- [ ] Parameter UI shows correct number of controls

### Time Estimate
2 hours

---

## Issue 12: Improve build system and documentation

**Labels:** `documentation`, `build-system`

### Description
Improve build configuration and documentation.

### Tasks
- [ ] Create proper CMake configuration
- [ ] Add build instructions for all platforms
- [ ] Document all dependencies and versions
- [ ] Create developer setup guide
- [ ] Add API documentation

### Acceptance Criteria
- [ ] Clean build on Windows/Mac/Linux
- [ ] Clear step-by-step instructions
- [ ] All dependencies documented
- [ ] Troubleshooting guide included

### Time Estimate
4 hours