# GPIO Development Strategy - Chimera Phoenix Pi

**Decision Date:** October 20, 2025
**Strategy:** Unified Build with Hardware Auto-Detection
**Status:** ✅ Active

---

## Executive Summary

**Decision: Use a single unified codebase (`hifiberrypi` branch) that automatically detects and adapts to available hardware on both Raspberry Pi devices.**

This strategy allows rapid GPIO/UI development on Pi .65 (no audio hardware) while maintaining full audio+GPIO integration testing capability on Pi .68 (with HiFiBerry DAC+ADC Pro).

---

## The Two Devices

### **Pi .65 (192.168.68.65) - "Development Workbench"**
- **Hostname:** `raspberrypi`
- **Hardware:** Standard Raspberry Pi 4/5
- **Audio:** None (uses JACK dummy driver)
- **GPIO:** 3 encoders + 3 switches (physical hardware)
- **Purpose:** Primary GPIO/UI development
- **Repository:** `~/phoenix-Chimera`
- **Branch:** `hifiberrypi`
- **Launch Script:** `~/launch_chimera_dev.sh`

### **Pi .68 (192.168.68.68) - "Integration Test Bed"**
- **Hostname:** `HifiBerryPi`
- **Hardware:** Raspberry Pi 4/5 + HiFiBerry DAC+ADC Pro
- **Audio:** Professional audio I/O + USB microphone
- **GPIO:** Same as .65 (code detects automatically)
- **Purpose:** Full system integration testing
- **Repository:** `~/ChimeraPhoenix_Pi`
- **Branch:** `hifiberrypi`
- **Launch Script:** `~/launch_chimera_hifiberry.sh`

---

## Why Unified Build? (Option A)

### ✅ **Advantages:**

1. **Single Source of Truth**
   - One codebase to maintain
   - Changes automatically propagate
   - No risk of branches diverging

2. **Smart Hardware Detection**
   - Code detects available hardware at runtime
   - Gracefully handles missing components
   - Same binary works on both devices

3. **Faster Development**
   - Work on .65 without worrying about breaking audio
   - Push once, works everywhere
   - Less git complexity

4. **Production Ready**
   - Final product will be one binary
   - Works on any Pi configuration
   - Users can add/remove hardware

### 🎯 **How It Works:**

The code uses conditional compilation and runtime detection:

```cpp
// GPIO Hardware (compile-time flag + runtime detection)
#if ENABLE_GPIO_HARDWARE && defined(__linux__)
    try {
        hardwareController = std::make_unique<HardwareController>();
        if (hardwareController->initialize()) {
            DBG("✓ GPIO hardware detected and initialized");
            // Enable encoder/switch controls
        }
    } catch (...) {
        DBG("⚠ GPIO hardware not available - continuing without");
        hardwareController.reset();
        // Plugin works normally, just no physical controls
    }
#endif

// Audio Hardware (runtime detection)
if (HiFiBerryDetected()) {
    initJackDirect();  // Use real audio I/O
} else {
    initJackDummy();   // Use dummy driver for development
}
```

**Result:** Same code, different capabilities based on hardware present.

---

## Development Workflow

### **Daily Development Cycle (Mon-Thu)**

#### On Pi .65 (Development):

```bash
# 1. SSH to development Pi
ssh branden@192.168.68.65
cd ~/phoenix-Chimera

# 2. Pull latest (if Mac or .68 made changes)
git pull origin hifiberrypi

# 3. Work on GPIO/UI code
# Edit files:
#   - pi_deployment/JUCE_Plugin/Source/HardwareController.cpp
#   - pi_deployment/JUCE_Plugin/Source/HardwareDisplayComponents.h
#   - pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp
#   - pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.h

# 4. Build
cd pi_deployment/JUCE_Plugin/Builds/LinuxMakefile
make clean && make -j4

# 5. Test with dummy audio (no HiFiBerry needed)
~/launch_chimera_dev.sh

# 6. Test GPIO functionality:
#    - Turn encoders → see position updates
#    - Press encoder buttons → trigger actions
#    - Flip switches → mode changes
#    - Check UI displays encoder/switch states
#    - Verify parameter control (if mapped)

# 7. Commit when working
cd ~/phoenix-Chimera
git add pi_deployment/JUCE_Plugin/Source/Hardware*
git add pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.*
git commit -m "feat(hardware): description of what you did"

# 8. Push to GitHub (makes it available to .68 and Mac)
git push origin hifiberrypi
```

**What to develop on .65:**
- ✅ GPIO encoder logic
- ✅ GPIO switch logic
- ✅ UI display components
- ✅ Parameter mapping (encoder → slot parameters)
- ✅ Visual feedback
- ✅ Button debouncing improvements
- ✅ Encoder acceleration curves
- ✅ Mode switching logic

---

### **Weekly Integration Testing (Friday)**

#### On Pi .68 (Integration):

```bash
# 1. SSH to integration Pi
ssh hifiberrypi
cd ~/ChimeraPhoenix_Pi

# 2. Pull all the GPIO work from .65
git pull origin hifiberrypi

# 3. Rebuild with audio support
cd pi_deployment/JUCE_Plugin/Builds/LinuxMakefile
make clean && make -j2

# 4. Launch with FULL system (audio + GPIO + voice + Trinity)
~/launch_chimera_hifiberry.sh

# 5. Comprehensive integration test:

## Audio Path:
#  - Verify HiFiBerry audio input working
#  - Verify HiFiBerry audio output working
#  - Check JACK connections (jack_lsp -c)
#  - Verify zero dropouts (check logs)

## Voice Recording:
#  - Press voice button
#  - Record voice prompt
#  - Verify Whisper transcription
#  - Check Trinity preset generation
#  - Confirm preset loads correctly

## GPIO Integration:
#  - Use encoders to control loaded preset parameters
#  - Use switches to change modes/bypass slots
#  - Verify encoder changes affect audio in real-time
#  - Check parameter smoothing (no clicks/pops)

## Trinity AI:
#  - Test text-based preset generation
#  - Test voice-based preset generation
#  - Verify preset loading via GPIO controls
#  - Check progress indicators

## UI/Display:
#  - Verify all displays update correctly
#  - Check preset name shown
#  - Verify engine slot indicators
#  - Check level meters

# 6. If bugs found:
#    - Note which subsystem (GPIO/Audio/Voice/Trinity)
#    - Fix on appropriate device (.65 for GPIO, .68 for audio)
#    - Commit and push fix
#    - Re-test on other device
```

**Integration test checklist:**
- [ ] JACK audio running without xruns
- [ ] HiFiBerry input/output working
- [ ] USB microphone recording working
- [ ] Whisper transcription successful
- [ ] Trinity preset generation working
- [ ] GPIO encoders controlling parameters
- [ ] GPIO switches changing modes
- [ ] UI updating in real-time
- [ ] No audio glitches when using GPIO
- [ ] Level meters showing signal
- [ ] Preset name displaying correctly

---

### **When Things Break**

#### GPIO Issues (encoders, switches, display):
**Fix on:** Pi .65
```bash
ssh branden@192.168.68.65
# Debug and fix GPIO code
# Test locally
# Commit and push
# Pull on .68 to verify fix
```

#### Audio Issues (JACK, HiFiBerry, voice recording):
**Fix on:** Pi .68
```bash
ssh hifiberrypi
# Debug and fix audio code
# Test locally
# Commit and push
# Pull on .65 (won't affect GPIO work)
```

#### Integration Issues (GPIO affecting audio, or vice versa):
**Fix on:** Pi .68 (test environment has all hardware)
```bash
# Reproduce issue
# Fix in code
# Test full integration
# Commit and push
# Verify on .65 (GPIO still works)
```

---

## File Organization

### **GPIO-Specific Files** (develop on Pi .65):

```
pi_deployment/JUCE_Plugin/Source/
├── HardwareController.cpp         ← Core GPIO logic
├── HardwareController.h
├── HardwareDisplayComponents.h    ← UI components for encoders/switches
├── PluginEditor_Pi.cpp            ← GPIO integration into UI
└── PluginEditor_Pi.h

pi_deployment/
├── GPIO_QUICK_REFERENCE.md        ← GPIO pin mappings
└── HARDWARE_INTEGRATION.md        ← Integration guide
```

### **Audio-Specific Files** (test on Pi .68):

```
pi_deployment/JUCE_Plugin/Source/
├── PluginProcessor.cpp            ← JACK audio fix (initJackDirect)
├── PluginProcessor.h
├── VoiceRecordButton.cpp          ← USB microphone recording
└── TrinityAIClient.cpp            ← Trinity AI communication

pi_deployment/
└── launch_chimera_hifiberry.sh    ← Full system launch script
```

### **Shared Files** (test on both):

```
pi_deployment/JUCE_Plugin/Source/
├── All Engine files (57 DSP engines)
├── EngineFactory.cpp
├── EngineBase.h
└── ParameterDefinitions.h

pi_deployment/AI_Server/
├── trinity_server_pi.py           ← Trinity AI server
├── visionary_complete.py
├── calculator_max_intelligence.py
└── alchemist_complete.py
```

---

## Git Strategy

### **Branch Structure:**

```
main (Desktop/macOS Plugin)
  │
  └─ hifiberrypi (All Pi Work) ← SINGLE BRANCH FOR BOTH PIs
      ├─ Pi .65 develops here
      └─ Pi .68 tests here
```

### **Commit Message Format:**

```bash
# GPIO/Hardware work:
git commit -m "feat(hardware): add encoder acceleration for faster parameter changes"
git commit -m "fix(hardware): debounce switch inputs to prevent jitter"
git commit -m "feat(ui): add visual feedback for encoder position"

# Audio work:
git commit -m "fix(audio): improve JACK connection stability"
git commit -m "feat(voice): add noise gate to USB microphone input"

# Integration:
git commit -m "feat(integration): map encoders to slot parameters"
git commit -m "fix(integration): prevent audio glitches during parameter changes"

# Documentation:
git commit -m "docs: update GPIO pin mapping guide"
```

### **Daily Sync Commands:**

**On Pi .65 (after development session):**
```bash
git add .
git commit -m "feat(hardware): what you did today"
git push origin hifiberrypi
```

**On Pi .68 (before testing):**
```bash
git pull origin hifiberrypi
# Build and test
```

**On Mac (to stay current):**
```bash
git checkout hifiberrypi
git pull origin hifiberrypi
```

---

## Development Phases

### **Phase 1: GPIO Foundation** (Current - Week 1)
**Status:** ✅ Complete

- [x] HardwareController implementation
- [x] EncoderDisplay components
- [x] SwitchDisplay components
- [x] Basic UI integration
- [x] Thread-safe polling
- [x] Debouncing logic
- [x] Graceful fallback

### **Phase 2: Parameter Mapping** (Week 2-3)
**Primary Device:** Pi .65

**Goals:**
- [ ] Map Encoder 1 → Slot selection (0-5)
- [ ] Map Encoder 2 → Parameter selection (1-15)
- [ ] Map Encoder 3 → Parameter value adjustment
- [ ] Map encoder buttons → Special functions (reset, bypass)
- [ ] Map switches → Mode selection (preset browser, manual control, etc.)
- [ ] Add parameter value displays to UI
- [ ] Implement parameter smoothing for clean audio

**Success Criteria:**
- Can select any slot using Encoder 1
- Can select any parameter in that slot using Encoder 2
- Can adjust parameter value using Encoder 3
- Changes affect audio in real-time (test on .68)
- No audio clicks/pops during parameter changes

### **Phase 3: Advanced Features** (Week 4-5)
**Primary Device:** Pi .65, validate on .68

**Goals:**
- [ ] Encoder acceleration (faster turns = bigger jumps)
- [ ] Long-press actions on encoder buttons
- [ ] Switch combinations for advanced functions
- [ ] Preset saving via GPIO
- [ ] Preset loading via GPIO
- [ ] Visual feedback improvements
- [ ] Status LEDs (if hardware supports)

### **Phase 4: Integration Polish** (Week 6)
**Primary Device:** Pi .68 (full system testing)

**Goals:**
- [ ] End-to-end workflow testing
- [ ] Voice → Trinity → GPIO parameter tweaking
- [ ] Performance optimization
- [ ] Audio quality verification
- [ ] Latency measurement
- [ ] CPU usage optimization
- [ ] Memory leak testing

### **Phase 5: Production Ready** (Week 7+)
**Both Devices**

**Goals:**
- [ ] Complete documentation
- [ ] User manual creation
- [ ] Installation guide
- [ ] Troubleshooting guide
- [ ] Video demonstrations
- [ ] Beta testing with real users
- [ ] Bug fixes from beta feedback

---

## Testing Strategy

### **Unit Testing (Pi .65):**

Test individual GPIO components in isolation:

```bash
# Test encoder reading
./test_encoder_basic

# Test switch reading
./test_switch_basic

# Test debouncing
./test_debounce_timing

# Test UI display
./test_display_components
```

### **Integration Testing (Pi .68):**

Test full system with all components:

```bash
# Launch full system
~/launch_chimera_hifiberry.sh

# Run through complete workflow:
# 1. Audio I/O test
# 2. Voice recording test
# 3. Trinity preset generation test
# 4. GPIO parameter control test
# 5. End-to-end user scenario test
```

### **Regression Testing (Both Devices):**

After any major change:

```bash
# On Pi .65 - verify GPIO still works
./test_gpio_regression

# On Pi .68 - verify audio still works
./test_audio_regression

# On Pi .68 - verify integration still works
./test_full_system_regression
```

---

## Performance Targets

### **Pi .65 (Development):**
- **Build Time:** < 5 minutes (with ccache)
- **GPIO Polling:** 1ms (1000 Hz)
- **UI Update Rate:** 30 fps
- **CPU Usage:** < 15% (without audio processing)

### **Pi .68 (Production):**
- **Build Time:** < 10 minutes (Pi 4), < 5 minutes (Pi 5)
- **Audio Latency:** ~10.7ms (512 samples @ 48kHz)
- **JACK Xruns:** Zero during normal operation
- **GPIO Response:** < 5ms from hardware change to audio effect
- **CPU Usage:** < 40% with 6 engines + GPIO + Trinity
- **Memory Usage:** < 200 MB total

---

## Troubleshooting

### **Problem: GPIO code broke audio on Pi .68**

**Solution:**
1. Git revert the problematic commit
2. Test audio works again
3. Fix GPIO code on .65 in isolation
4. Test GPIO on .65
5. Pull to .68 and test integration
6. Only push if both work

### **Problem: Audio changes broke GPIO on Pi .65**

**Solution:**
1. This should be rare (GPIO code is separate)
2. Check if PluginProcessor changes affected UI
3. Fix integration issue
4. Test on both devices
5. Commit fix

### **Problem: Builds work on .65 but fail on .68**

**Possible causes:**
- Different compiler versions
- Missing libraries on .68
- Different JUCE paths
- Architecture differences (Pi 4 vs Pi 5)

**Solution:**
```bash
# On .68, check build environment
uname -a
gcc --version
ls /usr/include/gpiod.h
ls ~/JUCE/modules

# Match build environment to .65
# Or add build guards for differences
```

### **Problem: Git conflicts between devices**

**Solution:**
```bash
# Pull before you start working
git pull origin hifiberrypi

# Commit frequently (at least daily)
git add .
git commit -m "wip: current work state"
git push origin hifiberrypi

# If conflict occurs:
git pull origin hifiberrypi
# Resolve conflicts manually
git add .
git commit -m "fix: resolve merge conflict"
git push origin hifiberrypi
```

---

## Success Metrics

### **Development Velocity:**
- ✅ Can iterate on GPIO features daily
- ✅ No more than 1 day turnaround for full system test
- ✅ Can fix bugs without affecting other device

### **Code Quality:**
- ✅ Single source of truth (no duplicate code)
- ✅ Clean separation of concerns (GPIO / Audio / Integration)
- ✅ Comprehensive error handling
- ✅ Graceful degradation when hardware missing

### **User Experience:**
- ✅ Plugin works on any Pi (with or without HiFiBerry)
- ✅ GPIO enhances experience but not required
- ✅ Same binary for all configurations
- ✅ Clear feedback when hardware missing

---

## Future Considerations

### **Potential Third Device:**

If you get a third Pi for a specific purpose:

```
Pi .65 → GPIO development (no audio)
Pi .68 → Full integration (audio + GPIO)
Pi .XX → Production deployment (exact target hardware)
```

Same unified build strategy applies - code adapts to hardware present.

### **Hardware Variations:**

The unified build handles:
- ✅ Pi with GPIO but no audio
- ✅ Pi with audio but no GPIO
- ✅ Pi with both GPIO and audio
- ✅ Different Pi models (Pi 4 vs Pi 5)
- ✅ Different audio hardware (HiFiBerry variants)

### **Deployment:**

When shipping to users:
1. Build once on .68 (most complete hardware)
2. Test on .65 (verify works without audio)
3. Ship single binary
4. Include hardware detection guide
5. Document optional components

---

## Quick Reference

### **Where to Work:**

| Task | Device | Why |
|------|--------|-----|
| GPIO encoder logic | .65 | No audio needed, faster iteration |
| GPIO switch logic | .65 | No audio needed, faster iteration |
| UI displays | .65 | Visual only, no audio |
| Parameter smoothing | .68 | Need to hear audio quality |
| JACK audio fixes | .68 | Need HiFiBerry hardware |
| Voice recording | .68 | Need USB microphone |
| Trinity testing | Either | Network-based, works on both |
| Integration testing | .68 | Need all hardware |
| DSP engine work | Mac | Easier debugging, faster builds |

### **Daily Commands:**

**Start of day:**
```bash
# On any device
git pull origin hifiberrypi
```

**End of day:**
```bash
# On .65 (after GPIO work)
git add .
git commit -m "feat(hardware): today's progress"
git push origin hifiberrypi

# On .68 (after testing)
git add .
git commit -m "fix(integration): issues found during testing"
git push origin hifiberrypi
```

### **Status Check:**

```bash
# Check all devices are in sync
echo "=== MAC ===" && cd ~/branden/Project_Chimera_v3.0_Phoenix && git log -1 --oneline
echo "=== PI .65 ===" && ssh branden@192.168.68.65 "cd ~/phoenix-Chimera && git log -1 --oneline"
echo "=== PI .68 ===" && ssh hifiberrypi "cd ~/ChimeraPhoenix_Pi && git log -1 --oneline"
```

All three should show the same commit hash.

---

## Conclusion

**The unified build strategy with hardware auto-detection provides:**

1. **Rapid Development** - Iterate on GPIO without needing audio hardware
2. **Robust Integration** - Test complete system with all hardware
3. **Clean Codebase** - Single source of truth, no duplication
4. **Production Ready** - Same binary works in all configurations
5. **Future Proof** - Easy to add new hardware or features

**Success comes from:**
- Clear separation: .65 = develop, .68 = validate
- Frequent commits and pushes (daily minimum)
- Weekly integration testing
- Trust the auto-detection code

**This strategy scales as the project grows and ensures a maintainable, professional product.**

---

**Last Updated:** October 20, 2025
**Next Review:** November 1, 2025 (after Phase 2 completion)
