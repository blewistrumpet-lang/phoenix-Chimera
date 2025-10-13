# Chimera Phoenix - Hardware Integration Complete

**Date:** October 13, 2025  
**Hardware:** Raspberry Pi with HiFiBerry DAC+ADC Pro + USB Microphone  
**Status:** ✅ READY FOR TESTING

---

## Audio Hardware Configuration

### Primary Audio I/O (HiFiBerry DAC+ADC Pro)
- **Device:** `hw:0` (sndrpihifiberry)
- **Role:** Main DSP processing audio I/O
- **Connection:** Connected to JACK server
- **Sample Rate:** 48kHz
- **Channels:** 2 in, 2 out
- **Buffer Size:** 512 samples

### Voice Input (USB PnP Sound Device)
- **Device:** `hw:1` (USB PnP Sound Device)
- **Role:** Voice recording for AI transcription
- **Connection:** Direct ALSA (bypasses JACK)
- **Sample Rate:** 48kHz
- **Channels:** 1 (mono)
- **Use Case:** VoiceRecordButton → Whisper transcription

---

## Software Architecture

### Audio Routing Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    AUDIO INPUT PATHS                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────┐           ┌──────────────────┐       │
│  │  HiFiBerry ADC   │           │   USB Microphone │       │
│  │    (hw:0)        │           │     (hw:1)       │       │
│  └────────┬─────────┘           └────────┬─────────┘       │
│           │                              │                  │
│           │ ALSA                         │ ALSA (direct)   │
│           ▼                              ▼                  │
│  ┌────────────────┐            ┌──────────────────┐        │
│  │  JACK Server   │            │ VoiceRecordButton│        │
│  │  (48kHz/512)   │            │    (JUCE/ALSA)   │        │
│  └────────┬───────┘            └────────┬─────────┘        │
│           │                              │                  │
│           ▼                              ▼                  │
│  ┌─────────────────┐           ┌──────────────────┐        │
│  │ ChimeraPhoenix  │           │  Whisper API     │        │
│  │   DSP Plugin    │           │  (OpenAI)        │        │
│  │  (57 engines)   │           └────────┬─────────┘        │
│  └────────┬────────┘                    │                  │
│           │                             │                  │
│           │                             ▼                  │
│           │                    ┌──────────────────┐        │
│           │                    │  Trinity Server  │        │
│           │                    │  (AI Pipeline)   │        │
│           │                    └──────────────────┘        │
│           ▼                                                 │
│  ┌────────────────┐                                        │
│  │  JACK Server   │                                        │
│  │  (output)      │                                        │
│  └────────┬───────┘                                        │
│           │                                                 │
│           ▼                                                 │
│  ┌────────────────┐                                        │
│  │ HiFiBerry DAC  │                                        │
│  │    (hw:0)      │                                        │
│  └────────────────┘                                        │
└─────────────────────────────────────────────────────────────┘
```

---

## Trinity AI Pipeline

### Current Production Version
**File:** `~/phoenix-Chimera/AI_Server/trinity_server_pi_CURRENT_USE.py`  
**Symlink:** `trinity_server_pi.py` → `trinity_server_pi_CURRENT_USE.py`

### Pipeline Flow
```
Voice → Whisper (OpenAI) → Visionary → Calculator → Alchemist → Preset
                             ↓           ↓           ↓
                         GPT-4o-mini  GPT-4o    Python
                         + Rules    + Gain    Validation
```

### Components
1. **Whisper** (OpenAI): Voice transcription
2. **Visionary** (visionary_complete.py): 
   - Hybrid intelligence (80% rules, 20% AI)
   - Engine selection via engine_selector.py
   - Musical intent metadata generation
3. **Calculator** (calculator_max_intelligence.py):
   - GPT-4o for parameter optimization
   - GainStagingAnalyzer for professional audio
   - Musical time subdivisions
4. **Alchemist** (alchemist_complete.py):
   - Pure Python validation
   - Parameter clamping
   - Safety checks

### Health Endpoint
```bash
curl http://localhost:8000/health
```

**Expected Response:**
```json
{
    "status": "healthy",
    "components": {
        "visionary": "ready",
        "calculator": "intelligent",
        "alchemist": "ready",
        "whisper": "ready",
        "oracle": "removed",
        "corpus": "not_needed"
    }
}
```

---

## Launch Script

### Location
`~/phoenix-Chimera/launch_chimera_hifiberry.sh`

### What It Does
1. **Pre-flight Checks:**
   - Verifies HiFiBerry and USB mic detection
   - Checks for existing lock file
   - Validates API key configuration

2. **Cleanup:**
   - Kills conflicting audio processes (PulseAudio, PipeWire)
   - Stops existing JACK/Trinity/Plugin instances

3. **Service Startup:**
   - Starts JACK server with HiFiBerry
   - Starts Trinity AI server
   - Launches ChimeraPhoenix plugin

4. **Verification:**
   - Confirms all services are running
   - Displays comprehensive status summary

### Usage
```bash
cd ~/phoenix-Chimera
./launch_chimera_hifiberry.sh
```

### Stop All Services
```bash
pkill -f 'jackd|trinity_server_pi|ChimeraPhoenix'
rm /tmp/chimera_plugin.lock
```

---

## Plugin Build Details

### Binary Location
`~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix`

### Build Info
- **Last Built:** October 13, 2025 15:12
- **Size:** 113 MB (debug build with symbols)
- **Architecture:** ARM aarch64 (Raspberry Pi)
- **Type:** ELF 64-bit LSB pie executable
- **Debug Info:** Included (not stripped)

### VoiceRecordButton Updates
The plugin now includes USB microphone detection on Linux:
- Automatically searches for USB/PnP audio devices
- Configures ALSA to use USB mic for voice input
- Separate from JACK audio routing
- Direct ALSA connection for low latency

**Code Location:**  
`VoiceRecordButton.cpp:175-189`

---

## Testing Checklist

### ✅ Completed Tests
1. USB microphone detected (hw:1)
2. HiFiBerry DAC+ADC detected (hw:0)
3. USB mic audio capture working (3s test recording successful)
4. JACK configuration verified (~/.jackdrc)
5. Trinity server health endpoint responding
6. Plugin rebuilt with VoiceRecordButton USB mic support
7. Launch script created and deployed

### 🔄 Ready for Testing
1. Launch complete system with new script
2. Verify JACK audio routing
3. Test voice recording → Whisper transcription
4. Test preset generation with voice input
5. Verify audio DSP processing through HiFiBerry
6. End-to-end integration test

---

## Key Files and Locations

### Configuration Files
- **JACK Config:** `~/.jackdrc`
- **Trinity Env:** `~/phoenix-Chimera/AI_Server/.env`
- **Launch Script:** `~/phoenix-Chimera/launch_chimera_hifiberry.sh`

### Source Code
- **Plugin Source:** `~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/`
- **Trinity Server:** `~/phoenix-Chimera/AI_Server/trinity_server_pi_CURRENT_USE.py`
- **Voice Recording:** `~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/VoiceRecordButton.cpp`

### Log Files
- **JACK:** `~/phoenix-Chimera/logs/jack.log`
- **Trinity:** `~/phoenix-Chimera/logs/trinity.log`
- **Plugin:** `~/phoenix-Chimera/logs/plugin.log`

### Documentation
- **Trinity Version Control:** `~/phoenix-Chimera/AI_Server/TRINITY_VERSION_CONTROL.md`
- **Oracle Removal Notice:** `~/phoenix-Chimera/pi_deployment/ORACLE_CORPUS_REMOVAL_NOTICE.md`
- **This Document:** `~/phoenix-Chimera/HARDWARE_INTEGRATION_COMPLETE.md`

---

## Troubleshooting

### If JACK won't start
```bash
# Kill conflicting processes
pkill -9 pulseaudio pipewire pipewire-pulse

# Check HiFiBerry is detected
aplay -l | grep hifiberry

# Test JACK manually
jackd -R -d alsa -d hw:sndrpihifiberry -r 48000 -p 512
```

### If USB mic not working
```bash
# Verify USB mic is detected
arecord -l | grep USB

# Test recording
arecord -D hw:1,0 -f S16_LE -r 48000 -c 1 -d 3 test.wav
```

### If Trinity server fails
```bash
# Check logs
tail -f ~/phoenix-Chimera/logs/trinity.log

# Verify API key
grep OPENAI_API_KEY ~/phoenix-Chimera/AI_Server/.env

# Test import
cd ~/phoenix-Chimera/AI_Server
python3 -c "import trinity_server_pi_CURRENT_USE"
```

---

## Performance Metrics

### Expected Response Times
- **Whisper Transcription:** 1-2 seconds
- **Visionary (Hybrid):** 800-1200ms (instant for rule-based)
- **Calculator:** 1000-1500ms (1 GPT call)
- **Alchemist:** 50-100ms (local)
- **Total Generation:** 2-4 seconds

### Audio Performance
- **Sample Rate:** 48 kHz
- **Buffer Size:** 512 samples
- **Latency:** ~10.7 ms (512/48000)
- **Bit Depth:** 24-bit (HiFiBerry)

---

## Next Steps

1. **Run Launch Script:**
   ```bash
   cd ~/phoenix-Chimera
   ./launch_chimera_hifiberry.sh
   ```

2. **Monitor System Status:**
   ```bash
   # Watch all logs
   tail -f ~/phoenix-Chimera/logs/*.log
   
   # Check process status
   ps aux | grep -E 'jackd|trinity|ChimeraPhoenix'
   
   # Verify JACK ports
   jack_lsp
   ```

3. **Test Voice Input:**
   - Click voice record button in plugin
   - Speak a preset description
   - Verify Whisper transcription
   - Confirm preset generation

4. **Test Audio Processing:**
   - Load a preset into chain
   - Route audio through HiFiBerry → JACK → Plugin → JACK → HiFiBerry
   - Verify DSP engines processing correctly

---

**Status:** All hardware integration tasks complete. System ready for end-to-end testing.
