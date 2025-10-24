# Pi 1 Migration Plan - ChimeraPhoenix_Pi → phoenix-Chimera
**Date:** October 18, 2025
**Objective:** Migrate Pi 1 (HiFiBerry production) to use phoenix-Chimera repo with clean hifiberrypi branch

---

## 🎯 Goal

**Consolidate to Single Source of Truth:**
- All three devices (Mac, Pi 1, Pi 2) use same repo: `phoenix-Chimera`
- All on same branch: `hifiberrypi`
- Delete old sketchy `hifiberrypi-jack-audio-fix` branch everywhere

---

## 📋 Pre-Migration Checklist

### Current State:
- ✅ Pi 1 running from `/home/branden/ChimeraPhoenix_Pi/`
- ✅ Pi 1 on old `hifiberrypi-jack-audio-fix` branch (1 commit, the sketchy one)
- ✅ Pi 1 binary: 113MB from Oct 17 17:25 (working)
- ✅ Pi 1 has preset_namer.py (untracked, added this morning)
- ✅ Local Mac and Pi 2 both on clean `hifiberrypi` branch in `phoenix-Chimera`
- ✅ GitHub has clean `hifiberrypi` branch with full history

### What We're Preserving:
1. **Working binary** - Will rebuild from clean source
2. **preset_namer.py** - Will copy to new location
3. **AI_Server/visionary_complete.py modifications** - Will copy
4. **Running process** - Will carefully stop/restart

### What We're Discarding:
1. **Old ChimeraPhoenix_Pi repo** - Backed up, then removed
2. **hifiberrypi-jack-audio-fix branch** - Deleted everywhere
3. **Duplicate editor files** - Clean repo has organized structure

---

## 🔒 Safety Measures

### Rollback Points:
1. **Backup old repo** - Renamed to `ChimeraPhoenix_Pi_OLD_BACKUP` (stays on disk)
2. **Keep process running** - Don't kill until new binary tested
3. **Binary backup** - Copy working binary to `/tmp/` before building new one

### Verification Tests:
1. Binary exists and is executable
2. Launch script works
3. JACK connects properly
4. Trinity AI server responds
5. Audio routing verified

---

## 📝 Step-by-Step Migration Plan

### **Phase 1: Preparation (No Service Interruption)**

#### Step 1.1: Backup Current Working Binary
```bash
ssh hifiberrypi "cp /home/branden/ChimeraPhoenix_Pi/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix /tmp/ChimeraPhoenix_working_backup_oct17"
```
**Verification:** Check backup exists and is 113MB

#### Step 1.2: Save Modified Files
```bash
# Copy preset_namer.py
ssh hifiberrypi "cp /home/branden/ChimeraPhoenix_Pi/AI_Server/preset_namer.py /tmp/"

# Copy modified visionary_complete.py
ssh hifiberrypi "cp /home/branden/ChimeraPhoenix_Pi/AI_Server/visionary_complete.py /tmp/"

# Copy intelligent naming doc
ssh hifiberrypi "cp /home/branden/ChimeraPhoenix_Pi/AI_Server/INTELLIGENT_NAMING_SYSTEM.md /tmp/ 2>/dev/null || true"
```
**Verification:** Check all files copied to /tmp/

#### Step 1.3: Document Current Process Info
```bash
ssh hifiberrypi "ps aux | grep ChimeraPhoenix | grep -v grep > /tmp/process_info.txt"
ssh hifiberrypi "pgrep -a trinity_server_pi > /tmp/trinity_info.txt"
ssh hifiberrypi "jack_lsp > /tmp/jack_connections.txt"
```
**Verification:** Files created with current state

---

### **Phase 2: Clone New Repository (Service Still Running)**

#### Step 2.1: Rename Old Repository
```bash
ssh hifiberrypi "mv /home/branden/ChimeraPhoenix_Pi /home/branden/ChimeraPhoenix_Pi_OLD_BACKUP"
```
**Verification:** Old directory renamed, process still running from memory

#### Step 2.2: Clone phoenix-Chimera from GitHub
```bash
ssh hifiberrypi "cd /home/branden && git clone git@github.com:blewistrumpet-lang/phoenix-Chimera.git"
```
**Expected Output:** Cloning into 'phoenix-Chimera'... done.

#### Step 2.3: Checkout hifiberrypi Branch
```bash
ssh hifiberrypi "cd /home/branden/phoenix-Chimera && git checkout hifiberrypi"
```
**Verification:** On branch hifiberrypi, up to date with origin/hifiberrypi

#### Step 2.4: Verify Repository Structure
```bash
ssh hifiberrypi "cd /home/branden/phoenix-Chimera && ls -d pi_deployment DEVELOPMENT_BRANCHES.md launch_chimera_hifiberry.sh"
```
**Expected:** All three exist

---

### **Phase 3: Restore Custom Work**

#### Step 3.1: Copy preset_namer.py
```bash
ssh hifiberrypi "cp /tmp/preset_namer.py /home/branden/phoenix-Chimera/AI_Server/"
```

#### Step 3.2: Copy Modified visionary_complete.py
```bash
ssh hifiberrypi "cp /tmp/visionary_complete.py /home/branden/phoenix-Chimera/AI_Server/"
```

#### Step 3.3: Copy Documentation
```bash
ssh hifiberrypi "cp /tmp/INTELLIGENT_NAMING_SYSTEM.md /home/branden/phoenix-Chimera/AI_Server/ 2>/dev/null || true"
```

#### Step 3.4: Git Status Check
```bash
ssh hifiberrypi "cd /home/branden/phoenix-Chimera && git status --short"
```
**Expected:** Shows preset_namer.py and visionary_complete.py as modified/untracked

---

### **Phase 4: Build New Binary (Service Still Running)**

#### Step 4.1: Verify Build Dependencies
```bash
ssh hifiberrypi "which make && which g++ && pkg-config --modversion jack"
```
**Expected:** All tools found

#### Step 4.2: Build Release Binary
```bash
ssh hifiberrypi "cd /home/branden/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile && make clean && make CONFIG=Release -j4"
```
**Expected:** Compilation completes, binary created
**Time:** ~5-10 minutes on Pi

#### Step 4.3: Verify Binary Created
```bash
ssh hifiberrypi "ls -lh /home/branden/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix"
```
**Expected:** Binary exists, size ~100-120MB

#### Step 4.4: Make Binary Executable (should already be)
```bash
ssh hifiberrypi "chmod +x /home/branden/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix"
```

---

### **Phase 5: Service Cutover (Brief Interruption)**

#### Step 5.1: Stop Current Services
```bash
ssh hifiberrypi "pkill -TERM -f ChimeraPhoenix && sleep 2"
ssh hifiberrypi "pkill -TERM -f trinity_server_pi && sleep 2"
ssh hifiberrypi "pkill -TERM jackd && sleep 2"
```
**Verification:** Check no processes remain:
```bash
ssh hifiberrypi "pgrep -a ChimeraPhoenix || echo 'Stopped'"
```

#### Step 5.2: Kill Any Remaining Processes
```bash
ssh hifiberrypi "pkill -9 -f ChimeraPhoenix 2>/dev/null || true"
ssh hifiberrypi "pkill -9 -f trinity_server_pi 2>/dev/null || true"
ssh hifiberrypi "pkill -9 jackd 2>/dev/null || true"
```

#### Step 5.3: Start New System
```bash
ssh hifiberrypi "cd /home/branden/phoenix-Chimera && ./launch_chimera_hifiberry.sh"
```
**Expected Output:**
```
✅ Chimera Phoenix Running with HiFiBerry DAC+ADC Pro
✓ JACK Server: PID XXXXX
✓ Trinity Server: PID XXXXX
✓ Plugin: PID XXXXX
```

---

### **Phase 6: Verification**

#### Step 6.1: Verify All Services Running
```bash
ssh hifiberrypi "ps aux | grep -E 'jackd|trinity_server_pi|ChimeraPhoenix' | grep -v grep"
```
**Expected:** 3 processes found

#### Step 6.2: Check Binary Path
```bash
ssh hifiberrypi "pgrep -a ChimeraPhoenix"
```
**Expected:** Shows `/home/branden/phoenix-Chimera/pi_deployment/...` path (NEW path)

#### Step 6.3: Test Trinity AI
```bash
ssh hifiberrypi "curl -s http://localhost:8000/health"
```
**Expected:** {"status":"healthy"}

#### Step 6.4: Test JACK Audio
```bash
ssh hifiberrypi "jack_lsp | grep -c system"
```
**Expected:** Shows system ports

#### Step 6.5: Check Logs for Errors
```bash
ssh hifiberrypi "tail -20 /home/branden/phoenix-Chimera/logs/plugin.log"
ssh hifiberrypi "tail -20 /home/branden/phoenix-Chimera/logs/trinity.log"
```
**Look for:** No critical errors, clean startup

---

### **Phase 7: Cleanup**

#### Step 7.1: Test System Stability (Wait 5 minutes)
```bash
# Let it run for 5 minutes, monitor logs
ssh hifiberrypi "tail -f /home/branden/phoenix-Chimera/logs/plugin.log"
# Ctrl+C after observing
```

#### Step 7.2: Commit New Files to Git
```bash
ssh hifiberrypi "cd /home/branden/phoenix-Chimera && git add AI_Server/preset_namer.py AI_Server/visionary_complete.py AI_Server/INTELLIGENT_NAMING_SYSTEM.md"
ssh hifiberrypi "cd /home/branden/phoenix-Chimera && git commit -m 'feat(pi): Add intelligent naming system to production Pi'"
ssh hifiberrypi "cd /home/branden/phoenix-Chimera && git push origin hifiberrypi"
```

#### Step 7.3: Delete Old Branch Locally (Mac)
```bash
git branch -d hifiberrypi-jack-audio-fix
```

#### Step 7.4: Delete Old Branch on GitHub
```bash
git push origin --delete hifiberrypi-jack-audio-fix
```

#### Step 7.5: Delete Old Branch on Pi 2
```bash
ssh branden@192.168.68.63 "cd ~/phoenix-Chimera && git branch -d hifiberrypi-jack-audio-fix 2>/dev/null || echo 'Already deleted'"
```

#### Step 7.6: Optional - Remove Old Backup (After 1 Week)
```bash
# Wait 1 week, verify everything stable, then:
# ssh hifiberrypi "rm -rf /home/branden/ChimeraPhoenix_Pi_OLD_BACKUP"
```

---

## 🚨 Emergency Rollback Procedure

**If new binary fails to start or has critical issues:**

### Rollback Step 1: Stop New System
```bash
ssh hifiberrypi "pkill -9 -f ChimeraPhoenix && pkill -9 -f trinity_server_pi && pkill -9 jackd"
```

### Rollback Step 2: Restore Old Repository
```bash
ssh hifiberrypi "mv /home/branden/phoenix-Chimera /home/branden/phoenix-Chimera_FAILED"
ssh hifiberrypi "mv /home/branden/ChimeraPhoenix_Pi_OLD_BACKUP /home/branden/ChimeraPhoenix_Pi"
```

### Rollback Step 3: Restore Old Binary (if needed)
```bash
ssh hifiberrypi "cp /tmp/ChimeraPhoenix_working_backup_oct17 /home/branden/ChimeraPhoenix_Pi/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix"
ssh hifiberrypi "chmod +x /home/branden/ChimeraPhoenix_Pi/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix"
```

### Rollback Step 4: Restart Old System
```bash
ssh hifiberrypi "cd /home/branden/ChimeraPhoenix_Pi && ./launch_chimera_hifiberry.sh"
```

**Time to Rollback:** ~2 minutes

---

## ✅ Success Criteria

Migration is successful when ALL of these are true:

1. ✅ Pi 1 running from `/home/branden/phoenix-Chimera/` path
2. ✅ Pi 1 on `hifiberrypi` branch (not hifiberrypi-jack-audio-fix)
3. ✅ Binary is ~100-120MB and working
4. ✅ JACK audio routing works (meters moving)
5. ✅ Trinity AI responds to requests
6. ✅ preset_namer.py integrated and working
7. ✅ No errors in plugin.log or trinity.log
8. ✅ System stable for 5+ minutes
9. ✅ Changes committed and pushed to GitHub
10. ✅ Old `hifiberrypi-jack-audio-fix` branch deleted everywhere

---

## 📊 Post-Migration State

### All Three Devices Unified:

**Local Mac (192.168.1.x):**
- Path: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix`
- Branch: `hifiberrypi`
- Role: Edit code locally, push to GitHub

**Pi 2 (192.168.68.63 - "raspberrypi"):**
- Path: `/home/branden/phoenix-Chimera`
- Branch: `hifiberrypi`
- Role: Build/test/development

**Pi 1 (192.168.68.68 - "HifiBerryPi"):**
- Path: `/home/branden/phoenix-Chimera` ← NEW
- Branch: `hifiberrypi` ← NEW
- Role: Production HiFiBerry hardware

**GitHub:**
- Branch: `hifiberrypi` (clean history, no sketchy commits)
- Branch: `main` (desktop development, paused)

---

## 🎯 Development Workflow After Migration

```bash
# On Mac: Edit code
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix
git checkout hifiberrypi
git pull origin hifiberrypi
# Edit pi_deployment/ files
git add .
git commit -m "feat(pi): your change"
git push origin hifiberrypi

# On Pi 1: Pull and rebuild
ssh hifiberrypi "cd ~/phoenix-Chimera && git pull origin hifiberrypi"
ssh hifiberrypi "cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile && make CONFIG=Release -j4"

# Restart services
ssh hifiberrypi "pkill -9 ChimeraPhoenix && sleep 2 && ~/phoenix-Chimera/launch_chimera_hifiberry.sh"
```

Clean, simple, single source of truth. ✅

---

## ⏱️ Estimated Timeline

- **Phase 1 (Backup):** 2 minutes
- **Phase 2 (Clone):** 3 minutes
- **Phase 3 (Restore work):** 1 minute
- **Phase 4 (Build):** 8-10 minutes
- **Phase 5 (Cutover):** 2 minutes
- **Phase 6 (Verification):** 5 minutes
- **Phase 7 (Cleanup):** 3 minutes

**Total Time:** ~25-30 minutes
**Service Downtime:** ~2 minutes (Phase 5 only)

---

## 🔐 Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Build fails | Low | Medium | Rollback to backup binary |
| Binary won't start | Low | High | Full rollback procedure |
| JACK connection fails | Low | Medium | Check launch script, verify hw:sndrpihifiberry |
| Trinity AI fails | Low | Medium | Check .env file, verify API key |
| Git clone fails | Very Low | Low | Check SSH keys, retry |
| Data loss | Very Low | N/A | All code on GitHub, no user data |

**Overall Risk:** LOW - Full rollback capability maintained

---

**Ready to execute?** Say "proceed" and I'll begin Phase 1.
