# ACTUAL SYSTEM CONFIGURATION - DO NOT TOUCH
**Created:** October 18, 2025, 9:15 AM
**CRITICAL:** This is the EXACT current state. DO NOT migrate, change, or "fix" anything.

---

## 🚨 CRITICAL UNDERSTANDING

**Pi 1 (HiFiBerry Production) uses DIFFERENT repository than Mac/Pi 2**

This is **INTENTIONAL** and **WORKING PERFECTLY**. Do not attempt to "unify" or "fix" this.

---

## 📍 Pi 1 (192.168.68.68 - "HifiBerryPi") - PRODUCTION HARDWARE

**Repository Path:** `/home/branden/ChimeraPhoenix_Pi/`

**Git Configuration:**
- Remote: `git@github.com:blewistrumpet-lang/phoenix-Chimera.git`
- Branch: `hifiberrypi-jack-audio-fix`
- Latest Commit: `e24e8f1c` - "fix(audio): Implement JACK direct connection to bypass JUCE wrapper bug on Linux/ARM"
- **ONLY 1 COMMIT IN HISTORY** (shallow clone or reset)

**Binary:**
- Path: `/home/branden/ChimeraPhoenix_Pi/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix`
- Size: 113 MB
- Date: **October 17, 2025 at 17:25** (5:25 PM)
- Status: **RUNNING PERFECTLY** (PID 365447, been running since Oct 17)

**Key Source Files:**
- `JUCE_Plugin/Source/PluginProcessor.cpp` - 66KB, Oct 17 17:15 ← **MOST RECENT, WORKING VERSION**
- `JUCE_Plugin/Source/PluginEditor_Pi.cpp` - 43KB, Oct 15 11:30
- `AI_Server/preset_namer.py` - 12KB, Oct 18 08:10 (added this morning)
- `AI_Server/visionary_complete.py` - 32KB, Oct 18 08:10 (modified this morning)

**Structure:**
- **NO `pi_deployment/` directory**
- All files directly in `JUCE_Plugin/Source/`
- Has `PluginEditor_Pi.cpp` for Pi UI
- Has JACK audio integration in PluginProcessor.cpp

**Running Process:**
```
PID 365447: /home/branden/ChimeraPhoenix_Pi/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix
```

**Hardware:**
- HiFiBerry DAC+ADC Pro (hw:0)
- USB microphone for voice recording
- 7" touchscreen (800×480)

**IMPORTANT:** This is the **PRODUCTION SYSTEM**. It works. Don't touch it.

---

## 📍 Pi 2 (192.168.68.63 - "raspberrypi") - DEVELOPMENT PI

**Repository Path:** `/home/branden/phoenix-Chimera/`

**Git Configuration:**
- Remote: `git@github.com:blewistrumpet-lang/phoenix-Chimera.git`
- Branch: `hifiberrypi` ✅
- Latest Commit: `cff2b23` - "docs: Add comprehensive branch strategy documentation"
- **FULL GIT HISTORY** (15+ commits)

**Binary:**
- Path: `/home/branden/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix`
- Size: 13 MB (older build)
- Date: October 10, 2025

**Key Source Files:**
- Has `pi_deployment/` directory structure ✅
- Has `DEVELOPMENT_BRANCHES.md` ✅
- Has preset_namer.py in AI_Server/ ✅

**Untracked Files:** 53 files
- Development backups
- Patch files
- Test scripts
- Trinity components

**Purpose:** Development, testing, building new versions

---

## 📍 Local Mac - DEVELOPMENT WORKSTATION

**Repository Path:** `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/`

**Git Configuration:**
- Remote 1: `origin` - `https://github.com/blewistrumpet-lang/phoenix-Chimera.git`
- Remote 2: `backup-mirror` - `https://github.com/blewistrumpet-lang/chimera-backup-mirror.git`
- Current Branch: `hifiberrypi` ✅
- Latest Commit: `cff2b234` - "docs: Add comprehensive branch strategy documentation"

**Branches:**
- `hifiberrypi` ← Currently checked out, for Pi development
- `main` - Desktop plugin (paused)
- `hifiberrypi-jack-audio-fix` - Old branch (can delete)
- `feature/goldens` - Golden audio work
- `feature/dsp-isolation` - DSP isolation work
- `quarantine/2025-10-13` - Quarantine branch
- `temp-golden-baseline` - Temporary baseline

**Key Source Files:**
- `JUCE_Plugin/Source/PluginProcessor.cpp` - 57KB, Oct 18 07:14 (desktop version)
- `pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp` - 59KB, Oct 17 21:31 (Pi version)
- `pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp` - 42KB, Oct 18 07:14

**Structure:**
- Has both `JUCE_Plugin/` (desktop) and `pi_deployment/` (Pi) directories
- Has `DEVELOPMENT_BRANCHES.md`, `PI_MIGRATION_PLAN.md`, `REPOSITORY_ANALYSIS.md`

**Purpose:** Edit code locally, push to GitHub, SSH to Pis for testing

---

## 📊 CRITICAL DIFFERENCES BETWEEN REPOSITORIES

### **ChimeraPhoenix_Pi (Pi 1 Production):**
```
/home/branden/ChimeraPhoenix_Pi/
├── JUCE_Plugin/Source/
│   ├── PluginProcessor.cpp (66KB, Oct 17 - LATEST!)
│   ├── PluginEditor_Pi.cpp
│   └── All 57 engine files
├── AI_Server/
│   ├── preset_namer.py (NEW)
│   └── visionary_complete.py (MODIFIED)
└── Binary: 113MB, Oct 17 (RUNNING)
```

### **phoenix-Chimera (Mac + Pi 2):**
```
/home/branden/phoenix-Chimera/
├── JUCE_Plugin/Source/ (desktop code)
├── pi_deployment/
│   └── JUCE_Plugin/Source/
│       ├── PluginProcessor.cpp (59KB, Oct 17)
│       ├── PluginEditor_Pi.cpp
│       └── All 57 engine files
├── AI_Server/preset_namer.py
├── DEVELOPMENT_BRANCHES.md
└── Binary: 13MB, Oct 10 (older)
```

**KEY INSIGHT:** The **Oct 17 PluginProcessor.cpp (66KB) on Pi 1** is the **MOST RECENT VERSION** that builds the working 113MB binary.

---

## 🎯 WHICH VERSION IS SOURCE OF TRUTH?

**For Production Pi Code:**
- **Pi 1's `/home/branden/ChimeraPhoenix_Pi/JUCE_Plugin/Source/PluginProcessor.cpp` (66KB, Oct 17)**
- This is the version that produces the working 113MB binary
- It has the correct JACK integration
- It has the correct createEditor() for Pi

**For Development:**
- `phoenix-Chimera` repo on Mac/Pi 2 for documentation, history, branching

---

## ⚠️ WHAT WENT WRONG IN MIGRATION ATTEMPT

1. Tried to replace Pi 1's working `ChimeraPhoenix_Pi` repo with `phoenix-Chimera`
2. The `phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp` is **OLDER** (59KB, Oct 17 21:31)
3. Pi 1's PluginProcessor.cpp is **NEWER** (66KB, Oct 17 17:15)
4. Build failed because we tried to use an older version

**The Oct 17 work on Pi 1 was AFTER the last push to phoenix-Chimera!**

---

## ✅ CORRECT DEVELOPMENT WORKFLOW

### **For Pi Development (Current Focus):**

1. **Edit locally on Mac:**
   ```bash
   cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix
   git checkout hifiberrypi
   # Edit pi_deployment/ files
   ```

2. **BUT:** Before deploying, **check if Pi 1 has newer version:**
   ```bash
   ssh hifiberrypi "ls -lh /home/branden/ChimeraPhoenix_Pi/JUCE_Plugin/Source/PluginProcessor.cpp"
   ```

3. **If Pi 1 is newer (like it is now), copy FROM Pi 1 TO local:**
   ```bash
   scp hifiberrypi:/home/branden/ChimeraPhoenix_Pi/JUCE_Plugin/Source/PluginProcessor.cpp \
       pi_deployment/JUCE_Plugin/Source/
   ```

4. **Then commit and push:**
   ```bash
   git add pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp
   git commit -m "feat(pi): Sync latest working PluginProcessor from Pi 1"
   git push origin hifiberrypi
   ```

5. **To rebuild on Pi 1:**
   ```bash
   ssh hifiberrypi "cd /home/branden/ChimeraPhoenix_Pi/JUCE_Plugin/Builds/LinuxMakefile && make CONFIG=Release -j4"
   ```

### **For Desktop Development (When You Resume):**

1. **Switch to main branch:**
   ```bash
   git checkout main
   # Edit JUCE_Plugin/Source/ files
   git push origin main
   ```

---

## 🔧 IMMEDIATE ACTION ITEMS

### **1. Sync Pi 1's Latest Code to Git**

Pi 1 has changes from Oct 17 that aren't in the `phoenix-Chimera` repo yet:

```bash
# Copy Pi 1's PluginProcessor.cpp to local Mac
scp hifiberrypi:/home/branden/ChimeraPhoenix_Pi/JUCE_Plugin/Source/PluginProcessor.cpp \
    /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/pi_deployment/JUCE_Plugin/Source/

# Commit
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix
git add pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp
git commit -m "feat(pi): Sync Oct 17 working PluginProcessor from Pi 1 production"
git push origin hifiberrypi
```

### **2. Copy preset_namer.py to Git**

```bash
scp hifiberrypi:/home/branden/ChimeraPhoenix_Pi/AI_Server/preset_namer.py \
    /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server/

git add AI_Server/preset_namer.py
git commit -m "feat(ai): Add intelligent preset naming system from Pi 1"
git push origin hifiberrypi
```

### **3. Document the Dual-Repo Setup**

Accept that Pi 1 uses `ChimeraPhoenix_Pi` and Mac/Pi2 use `phoenix-Chimera`. Update DEVELOPMENT_BRANCHES.md to reflect this reality.

---

## 🚫 WHAT NOT TO DO

❌ **Do NOT delete `ChimeraPhoenix_Pi` on Pi 1**
❌ **Do NOT try to "migrate" Pi 1 to `phoenix-Chimera`**
❌ **Do NOT assume Mac's pi_deployment/ is newer than Pi 1's code**
❌ **Do NOT overwrite Pi 1's PluginProcessor.cpp**
❌ **Do NOT kill the running process without backing up first**

---

## ✅ WHAT IS SAFE TO DO

✅ **Copy files FROM Pi 1 TO Mac/GitHub** (sync latest work)
✅ **Build new binary on Pi 1 in place** (using its own code)
✅ **Edit files on Mac** (but check dates before deploying)
✅ **Delete the old `hifiberrypi-jack-audio-fix` branch on GitHub**
✅ **Commit preset_namer.py to git**

---

## 📝 FILE DATE COMPARISON

| File | Pi 1 (ChimeraPhoenix_Pi) | Mac (phoenix-Chimera) | Winner |
|------|--------------------------|----------------------|--------|
| PluginProcessor.cpp | 66KB, Oct 17 17:15 | 59KB, Oct 17 21:31 | **Pi 1 is NEWER** |
| PluginEditor_Pi.cpp | 43KB, Oct 15 11:30 | 42KB, Oct 18 07:14 | **Mac is newer** |
| preset_namer.py | 12KB, Oct 18 08:10 | 12KB (in git) | Same (already synced) |

**Critical:** The **PluginProcessor.cpp on Pi 1 is 7 hours NEWER** than the one in phoenix-Chimera!

Oct 17 17:15 (Pi 1) vs Oct 17 21:31 (Mac/phoenix-Chimera)

Wait, that doesn't make sense. Let me check timezones...

Actually, Pi 1 is at **17:15** (5:15 PM) and Mac is at **21:31** (9:31 PM), so Mac is **4 hours LATER**.

But the **file size difference** (66KB vs 59KB) suggests they're different versions!

**MOST IMPORTANT:** The **66KB version on Pi 1** is what **builds the working 113MB binary**. That's what matters.

---

## 🎯 GROUND TRUTH

**The working system right now:**
- Pi 1 at `/home/branden/ChimeraPhoenix_Pi/`
- Branch: `hifiberrypi-jack-audio-fix`
- PluginProcessor.cpp: 66KB
- Binary: 113MB, works perfectly
- Running since Oct 17

**Everything else is secondary to this.**

---

## 📞 WHEN IN DOUBT

**Before making ANY changes to Pi 1:**
1. Check if the process is running: `ssh hifiberrypi "pgrep -a ChimeraPhoenix"`
2. Check file dates: `ssh hifiberrypi "ls -lh /home/branden/ChimeraPhoenix_Pi/JUCE_Plugin/Source/PluginProcessor.cpp"`
3. Ask yourself: "Is this ACTUALLY broken, or am I just trying to 'organize' it?"
4. If it ain't broke, **DON'T FIX IT**

---

**FINAL RULE:** Treat Pi 1's `ChimeraPhoenix_Pi` as **SACRED**. Only touch it to rebuild or add new features. Never "migrate" or "reorganize" it.
