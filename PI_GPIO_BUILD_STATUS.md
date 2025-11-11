# Raspberry Pi GPIO Build Status & Recovery Guide
**Date:** November 10, 2024
**Current State:** Working GPIO binary exists, but build environment needs fixing

## System Overview

### Pi 2 (192.168.68.107 - "raspberrypi")
- **Purpose:** Development Pi with newly soldered GPIO hardware
- **SSH Access:** `ssh -i ~/.ssh/id_ed25519 branden@192.168.68.107`
- **Has HiFiBerry Studio DAC/ADC HAT**
- **GPIO Hardware:** Newly soldered, confirmed working (SW1 detected but bouncing)

### Pi 1 (192.168.68.68 - "HifiBerryPi")
- **Purpose:** Production Pi (may or may not have GPIO)
- **SSH Access:** `ssh -i ~/.ssh/id_ed25519_hifiberrypi branden@192.168.68.68`
- **Has older build from October 17**

## Working Binaries on Pi 2

### 1. Most Recent Working Binary (October 27, 2024)
```
Location: /home/branden/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix
Size: 119 MB
Built: October 27, 2024
Status: DOES NOT EXIST (was listed but not actually there)
```

### 2. HiFiBerry Build (October 20, 2024)
```
Location: /home/branden/ChimeraPhoenix_Build_FromHiFiBerry/ChimeraPhoenix
Size: 113 MB
Built: October 20, 2024
Status: WORKS, GPIO responds (tested today)
Features: Detects SW1 switch (but shows bouncing)
```

### 3. Older Phoenix-Chimera Build
```
Location: /home/branden/phoenix-Chimera/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix
Size: 90 MB
Built: October 7, 2024
Status: Unknown GPIO support
```

## Git Repository State on Pi 2

```bash
Repository: /home/branden/phoenix-Chimera/
Branch: fix/phase2-preset-ab-coalesce
Latest Commit: 2389934 - "feat(gpio): Week 3 Phase 1 - Add WARMTH/SIZE/PUNCH macros and SW3 BYPASS control"
Modified Files: 31 (various including HardwareController.cpp/.h)
Stashed Changes: 2 stashes with GPIO modifications
```

## Build Configuration Requirements

### Critical Build Flags
```bash
CPPFLAGS='-DCHIMERA_PI=1 -DENABLE_GPIO=1'
```

- **CHIMERA_PI=1** - Selects ChimeraAudioProcessorEditor_Pi instead of other editors
- **ENABLE_GPIO=1** - Enables GPIO hardware support

### Key Source Files for GPIO

1. **HardwareController.cpp/.h** - Main GPIO interface
2. **GPIOPresetManager.h** - Preset management via GPIO
3. **PluginEditor_Pi.cpp/.h** - Pi-specific UI
4. **PluginEditor_Pi_Components.h** - UI components
5. **HardwareDisplayComponents.h** - Referenced but missing
6. **EventBus.h** - Referenced but missing
7. **ControlState.h** - Exists

## Build Issues Encountered

### 1. Missing Files in Includes
```cpp
// In PluginEditor_Pi.h:
#include "HardwareDisplayComponents.h"  // File doesn't exist
#include "EventBus.h"                    // File doesn't exist
```

### 2. Editor Class Mismatch
```cpp
// PluginProcessor.cpp line 864 tries to create:
return new ChimeraAudioProcessorEditorRefined(*this);
// But PluginEditorRefined.cpp/.h are empty (0 bytes)
```

### 3. VocalFormantFilter.cpp Compilation Error
```cpp
// Line 27: Conflicting DenormGuard declaration
static struct DenormGuard {
    // Conflicts with another DenormGuard elsewhere
} g_denormGuard;
```

### 4. HardwareController Not in Build
- HardwareController.cpp exists but wasn't in the .jucer project file
- Not included in Makefile OBJECTS list
- Causes undefined reference linker errors

### 5. Projucer Version Mismatch
```
"This project was last saved using an outdated version of the Projucer!"
```
- Using JUCE_7_backup Projucer at: `~/JUCE_7_backup/extras/Projucer/Builds/LinuxMakefile/build/Projucer`

## What Works

1. **GPIO Hardware:** Confirmed working - SW1 switch detected (needs debouncing)
2. **Existing Binary:** `/home/branden/ChimeraPhoenix_Build_FromHiFiBerry/ChimeraPhoenix` runs with GPIO
3. **Source Code:** All GPIO source files exist in `~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/`

## Recovery Steps for Clean Build

### Step 1: Prepare Clean Environment
```bash
cd ~/phoenix-Chimera
git stash  # Save any local changes
git checkout 2389934  # Week 3 GPIO commit
```

### Step 2: Fix Known Issues
```bash
# 1. Comment out EventBus and HardwareDisplayComponents includes
sed -i 's/#include "HardwareDisplayComponents.h"/\/\/#include "HardwareDisplayComponents.h"/' \
    pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.h
sed -i 's/#include "EventBus.h"/\/\/#include "EventBus.h"/' \
    pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.h

# 2. Fix VocalFormantFilter.cpp - comment out conflicting DenormGuard
sed -i '27,34s/^/\/\/ /' pi_deployment/JUCE_Plugin/Source/VocalFormantFilter.cpp

# 3. Add HardwareController to .jucer file
# Edit ChimeraPhoenix.jucer and add:
# <FILE id="hw1234" name="HardwareController.cpp" compile="1" resource="0" file="Source/HardwareController.cpp"/>
```

### Step 3: Regenerate Project Files
```bash
cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin
~/JUCE_7_backup/extras/Projucer/Builds/LinuxMakefile/build/Projucer --resave ChimeraPhoenix.jucer
```

### Step 4: Build with Correct Flags
```bash
cd Builds/LinuxMakefile
make clean
make CONFIG=Release CPPFLAGS='-DCHIMERA_PI=1 -DENABLE_GPIO=1' -j4
```

## Alternative: Use Working Build Configuration

Since `/home/branden/ChimeraPhoenix_Build_FromHiFiBerry/ChimeraPhoenix` works:
1. Check if there's a Makefile in that directory
2. Copy the build configuration from there
3. Use it as reference for fixing the main build

## Next Steps for v3.0 Integration

1. **Get basic build working** with GPIO support
2. **Test GPIO hardware** thoroughly (fix switch bouncing)
3. **Integrate v3.0 safety system** changes
4. **Add new UI components** as needed
5. **Test complete system** with all 57 engines

## Important Notes

- The working binary proves GPIO hardware and basic software work
- Main issue is build configuration, not code functionality
- SW1 bouncing suggests need for:
  - Software debouncing
  - Pull-up resistor configuration
  - Possible hardware check (loose connection?)

## Quick Test Command

To test if GPIO is working with existing binary:
```bash
sudo /home/branden/ChimeraPhoenix_Build_FromHiFiBerry/ChimeraPhoenix
# Watch console output for SW1, SW2, SW3 detection
# Press Ctrl+C to exit
```

## Files to Backup Before Starting Fresh

```bash
# Backup working binary
cp /home/branden/ChimeraPhoenix_Build_FromHiFiBerry/ChimeraPhoenix ~/chimera_gpio_working_backup

# Backup current source with modifications
cd ~/phoenix-Chimera
tar czf ~/chimera_source_backup_$(date +%Y%m%d).tar.gz pi_deployment/JUCE_Plugin/Source/
```

---

**Summary:** You have working GPIO hardware and a working binary. The challenge is the build environment has inconsistencies between the .jucer file, Makefile, and source code. The path forward is to either fix these inconsistencies or extract the working build configuration from the October 20 HiFiBerry build.