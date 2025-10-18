# Chimera Phoenix - Development Branch Strategy

**Last Updated:** October 18, 2025
**Repository:** https://github.com/blewistrumpet-lang/phoenix-Chimera

---

## 🎯 Sources of Truth

### **Desktop/macOS Plugin**
- **Branch:** `main`
- **Platform:** macOS (Xcode), Windows (Visual Studio)
- **Build Targets:** VST3, AU, Standalone
- **UI:** Full desktop interface (1200×800)
- **Latest Commit:** `ace4604` - "Docs: Add comprehensive reorganization status report"
- **Status:** ⚠️ 3 commits ahead of `origin/main` (unpushed)

### **Raspberry Pi Embedded System**
- **Branch:** `hifiberrypi`
- **Platform:** Raspberry Pi 4/5 with HiFiBerry DAC+ADC Pro
- **Build Targets:** Standalone Linux binary
- **UI:** Touch-optimized interface (800×480)
- **Latest Commit:** `44c4fc9` - "Merge JACK audio fix - meters now working on Pi with HiFiBerry"
- **Status:** ✅ Synced with `origin/hifiberrypi`
- **Working Snapshot:** `v3.0.1-pi-working` tag

---

## 📂 Branch Topology

```
main (Desktop/macOS Plugin)
├─ Xcode/Visual Studio builds
├─ VST3/AU/Standalone
├─ Full desktop UI (PluginEditor.cpp)
└─ Trinity AI integration (optional)

hifiberrypi (Raspberry Pi Embedded)
├─ Linux Makefile build
├─ Standalone only
├─ Pi-optimized UI (PluginEditor_Pi.cpp - 800×480)
├─ JACK audio direct connection
├─ HiFiBerry DAC+ADC Pro support
├─ USB microphone voice recording
└─ Trinity AI required (localhost:8000)
```

---

## 🔀 Relationship Between Branches

### **Shared Code:**
Both branches share the **entire DSP engine library** (57 audio effects engines):
- `JUCE_Plugin/Source/Engine*.cpp/h` (all DSP engines)
- `PluginProcessor.cpp/h` (core audio processing)
- `EngineBase.h` (engine interface)
- `ParameterDefinitions.h` (parameter system)

### **Platform-Specific Code:**

#### **Desktop (`main`):**
- `JUCE_Plugin/Source/PluginEditor.cpp/h` - Desktop UI
- `JUCE_Plugin/Builds/MacOSX/` - Xcode project
- No JACK dependency
- No HiFiBerry-specific code

#### **Raspberry Pi (`hifiberrypi`):**
- `pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp/h` - Pi UI
- `pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/` - Linux build
- `pi_deployment/launch_chimera_hifiberry.sh` - Launch script
- JACK audio direct connection (`#ifdef __linux__`)
- `VoiceRecordButton.cpp` - USB mic integration
- `TrinityAIClient.cpp` - AI server communication

### **Why Separate Branches?**
1. **Different build systems** - Xcode vs. Makefile
2. **Different UIs** - Desktop (1200×800) vs. Pi (800×480)
3. **Different hardware** - Generic audio vs. HiFiBerry DAC+ADC
4. **Different use cases** - DAW plugin vs. standalone hardware
5. **Easier testing** - Changes to one don't affect the other

---

## 🏷️ Important Tags

| Tag | Commit | Description |
|-----|--------|-------------|
| `v3.0.1-pi-working` | `cd6a176` | ✅ Known-good Pi build with JACK audio fix working |
| `v3.0-golden-reference` | `f99a2af` | 🎯 DSP baseline for regression testing |
| `v3.0-pre-reorg` | `9157fd1` | 📸 Snapshot before codebase reorganization |
| `v0.1.0-pi-beta` | `f99a2af` | 🥧 First Raspberry Pi beta release |

---

## 🚀 Development Workflow

### **Working on Desktop Plugin:**
```bash
git checkout main
git pull origin main
# Make changes to JUCE_Plugin/Source/
git add .
git commit -m "feat: your change"
git push origin main
```

### **Working on Raspberry Pi:**
```bash
git checkout hifiberrypi
git pull origin hifiberrypi
# Make changes to pi_deployment/JUCE_Plugin/Source/
git add .
git commit -m "feat(pi): your change"
git push origin hifiberrypi
```

### **Syncing DSP Engine Improvements:**
If you fix a bug or improve a DSP engine on one branch, manually port the fix to the other:

#### **From `main` → `hifiberrypi`:**
```bash
# On main branch, identify the engine fix commit
git log --oneline JUCE_Plugin/Source/PlateReverb.cpp

# Switch to hifiberrypi
git checkout hifiberrypi

# Copy the fixed file
cp ../main-repo/JUCE_Plugin/Source/PlateReverb.cpp pi_deployment/JUCE_Plugin/Source/

# Test and commit
git add pi_deployment/JUCE_Plugin/Source/PlateReverb.cpp
git commit -m "fix(pi): Port PlateReverb fix from main branch"
git push origin hifiberrypi
```

#### **From `hifiberrypi` → `main`:**
```bash
# Same process, but copy from pi_deployment/ to JUCE_Plugin/
git checkout main
cp ../hifiberrypi-repo/pi_deployment/JUCE_Plugin/Source/BitCrusher.cpp JUCE_Plugin/Source/
git add JUCE_Plugin/Source/BitCrusher.cpp
git commit -m "fix: Port BitCrusher fix from Pi branch"
git push origin main
```

---

## ⚠️ Critical Rules

### **DO:**
✅ Keep both branches in sync for DSP engine fixes
✅ Test on target platform before pushing
✅ Use descriptive commit messages with `feat:`, `fix:`, `docs:` prefixes
✅ Tag stable releases (`v3.x.x`)
✅ Create backups before major changes

### **DON'T:**
❌ Merge `hifiberrypi` into `main` (or vice versa) without careful planning
❌ Push directly to `main` or `hifiberrypi` without testing
❌ Commit compiled binaries, build artifacts, or temporary files
❌ Forget to update this document when branch strategy changes
❌ Delete tags or branches without team consensus

---

## 🔧 Build Instructions

### **Desktop (macOS):**
```bash
cd JUCE_Plugin/Builds/MacOSX
xcodebuild -project ChimeraPhoenix.xcodeproj -configuration Release
# Output: build/Release/ChimeraPhoenix.vst3 and ChimeraPhoenix.component
```

### **Raspberry Pi:**
```bash
cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile
make CONFIG=Release
# Output: build/ChimeraPhoenix (113 MB standalone binary)
```

**Launch Pi System:**
```bash
cd ~/phoenix-Chimera
./launch_chimera_hifiberry.sh
```

---

## 📊 Current State (October 18, 2025)

| Branch | Commits Ahead/Behind | Last Updated | Status |
|--------|---------------------|--------------|--------|
| `main` | +3 ahead of origin | Oct 13, 2025 | ⚠️ Unpushed commits |
| `hifiberrypi` | ✅ synced | Oct 18, 2025 | ✅ Production ready |
| `hifiberrypi-jack-audio-fix` | Merged into `hifiberrypi` | Oct 18, 2025 | 🗑️ Can be deleted |

---

## 🎯 Future Merge Strategy

**When both platforms are stable and production-ready**, we may merge into a unified codebase:

```cpp
// Unified PluginProcessor.cpp
#ifdef CHIMERA_PI
    #include <jack/jack.h>
    // Pi-specific JACK integration
#endif

juce::AudioProcessorEditor* ChimeraAudioProcessor::createEditor() {
#ifdef CHIMERA_PI
    return new ChimeraAudioProcessorEditor_Pi(*this);  // 800×480 UI
#else
    return new ChimeraAudioProcessorEditor(*this);      // Full desktop UI
#endif
}
```

**Benefits of unified codebase:**
- Single source of truth for all DSP engines
- Easier to maintain
- Automatic propagation of fixes

**When to merge:**
- Both platforms are feature-complete
- Comprehensive test coverage exists
- Golden audio regression tests pass on both platforms
- CI/CD pipeline is in place

---

## 📝 Version History

| Version | Date | Branch | Description |
|---------|------|--------|-------------|
| v3.0.1 | Oct 18, 2025 | `hifiberrypi` | JACK audio fix, meters working |
| v3.0.0 | Oct 13, 2025 | `hifiberrypi` | Premium UI, Trinity AI, HiFiBerry support |
| v0.1.0-beta | Oct 12, 2025 | `hifiberrypi` | First Pi beta release |

---

## 🆘 Troubleshooting

**Q: I made a change on `main` but it's not on `hifiberrypi`**
A: Branches are independent. Manually port the change (see "Syncing DSP Engine Improvements" above).

**Q: Can I merge `hifiberrypi` into `main`?**
A: Not recommended yet. The branches have different build systems and UIs. Wait until unified merge strategy is ready.

**Q: Which branch should I use for new DSP engine development?**
A: Develop on `main` (easier testing), then port to `hifiberrypi` when stable.

**Q: I accidentally pushed to the wrong branch**
A: Use `git revert <commit>` to undo, or contact team lead for history cleanup.

---

## 📞 Contacts

**Repository:** https://github.com/blewistrumpet-lang/phoenix-Chimera
**Issues:** https://github.com/blewistrumpet-lang/phoenix-Chimera/issues
**Documentation:** See `pi_deployment/CHIMERA_PHOENIX_PI_COMPLETE_ANALYSIS.md`

---

**Remember:** This document is the **source of truth** for development strategy. Keep it updated!
