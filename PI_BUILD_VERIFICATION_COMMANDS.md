# Raspberry Pi Build Verification Commands

## Quick Verification (Run on Pi via SSH)

```bash
ssh pi@192.168.68.63
```

Then execute these commands:

### 1. COMPILATION VERIFICATION

```bash
# Check binary exists and info
ls -lh ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi

# Verify it's an ELF executable
file ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi

# Check modification time
stat ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi
```

**Expected Results:**
- File size: >1MB (likely 5-15MB)
- File type: "ELF 64-bit LSB executable, ARM aarch64"
- Modified: Today (2025-10-05)

---

### 2. SYMBOL VERIFICATION

```bash
# Check for required symbols using nm
nm ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi | grep -E "updateUIFromProgress|stopProgressMonitoring|FileProgressMonitor|sendTrinityRequest"

# Alternative: use strings if nm doesn't work
strings ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi | grep -E "updateUIFromProgress|stopProgressMonitoring|FileProgressMonitor|sendTrinityRequest"
```

**Expected Results:**
All four symbols should be found:
- ✓ updateUIFromProgress
- ✓ stopProgressMonitoring
- ✓ FileProgressMonitor
- ✓ sendTrinityRequest

---

### 3. SOURCE CODE CONSISTENCY

```bash
# Verify header contains currentRequestId
grep "currentRequestId" ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.h

# Verify header contains progressMonitor
grep "progressMonitor" ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.h

# Count lines in PluginEditor_Pi.cpp
wc -l ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp
```

**Expected Results:**
- currentRequestId: Found (should show: `std::string currentRequestId;`)
- progressMonitor: Found (should show: `std::unique_ptr<FileProgressMonitor> progressMonitor;`)
- Line count: 936 lines (or close to it)

---

### 4. CRITICAL: PROGRESS FIELD NAMES

This is the most important check - verify the EXACT JSON field names used:

```bash
# Extract the updateUIFromProgress function
sed -n '/void.*updateUIFromProgress/,/^}/p' ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp

# Find all progress["..."] field accesses
grep -o 'progress\["[^"]*"\]' ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp | sort -u
```

**Expected Field Names:**
The code should use these EXACT field names:
- `progress["stage"]` - The current stage name
- `progress["overall_progress"]` - The percentage (0-100)
- `progress["message"]` - Status message
- `progress["preset_name"]` - Name of preset being generated

**CRITICAL ISSUE TO CHECK:**
- ❌ WRONG: `progress["percent"]`
- ✓ CORRECT: `progress["overall_progress"]`

If you see "percent" instead of "overall_progress", that's a bug!

---

### 5. FILE COUNT VERIFICATION

```bash
# Show exact line count
wc -l ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp

# Show file modification time
ls -l ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp
```

---

## Automated Verification Script

For a comprehensive automated check, run the verification script:

```bash
# Copy the script to Pi (from Mac)
scp /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/verify_pi_build.sh pi@192.168.68.63:~/

# On Pi, make executable and run
chmod +x ~/verify_pi_build.sh
~/verify_pi_build.sh
```

---

## Expected Build Success Output

When the build is correct, you should see:

```
BUILD STATUS: SUCCESS

✓ Binary exists
✓ File size: >1MB
✓ ELF executable (ARM aarch64)
✓ Modified today
✓ All symbols found
✓ currentRequestId declared
✓ progressMonitor declared
✓ Correct JSON field names
```

---

## Common Issues and Fixes

### Issue: Binary not found
```bash
# Check if build failed
cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile
make clean
make CONFIG=Release
```

### Issue: Wrong field names (using "percent" instead of "overall_progress")
The source file needs to be corrected. This is a code bug that needs fixing.

### Issue: Symbols not found
```bash
# Check if proper optimization was used
readelf -p .comment ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi
```

Should show: GCC with optimization flags

---

## JSON Field Name Verification - DETAILED

The AI server sends progress updates with these fields:

```json
{
  "stage": "Generating preset",
  "overall_progress": 75.5,
  "message": "Creating parameters...",
  "preset_name": "Warm Reverb"
}
```

The PluginEditor_Pi.cpp code MUST access fields using these EXACT names:
- ✓ `progress["stage"]`
- ✓ `progress["overall_progress"]`
- ✓ `progress["message"]`
- ✓ `progress["preset_name"]`

To verify:
```bash
# This should show ONLY these field names
grep 'progress\["' ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp | grep -v "//"
```

If you see different field names, the code has a mismatch with the server protocol.

---

## Binary Execution Test

After verification, test the binary:

```bash
# Check dependencies
ldd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi

# Run with debug output
~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi --verbose
```

---

## Summary Checklist

- [ ] Binary exists at correct path
- [ ] Binary is >1MB
- [ ] Binary is ELF ARM64 executable
- [ ] Binary modified today (2025-10-05)
- [ ] updateUIFromProgress symbol exists
- [ ] stopProgressMonitoring symbol exists
- [ ] FileProgressMonitor symbol exists
- [ ] sendTrinityRequest symbol exists
- [ ] currentRequestId in header
- [ ] progressMonitor in header
- [ ] PluginEditor_Pi.cpp is ~936 lines
- [ ] Uses progress["overall_progress"] (NOT "percent")
- [ ] Uses progress["stage"]
- [ ] Uses progress["message"]
- [ ] Uses progress["preset_name"]

All items should be checked ✓ for a valid build.
