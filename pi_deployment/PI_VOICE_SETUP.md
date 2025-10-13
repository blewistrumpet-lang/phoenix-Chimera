# ChimeraPhoenix Pi - Voice Input Setup

## Overview

The Raspberry Pi version uses **voice commands** for Trinity AI preset generation via:
- **OpenAI Whisper** (speech-to-text)
- **Trinity AI Server** (preset generation)
- **JACK Audio** (UAD Volt CC routing)

---

## 1. OpenAI API Key Setup

### Option A: Environment Variable (Recommended for testing)
```bash
export OPENAI_API_KEY="sk-your-actual-key-here"
```

### Option B: Config File (Recommended for production)
Create `~/.chimera_config`:
```bash
echo "OPENAI_API_KEY=sk-your-actual-key-here" > ~/.chimera_config
chmod 600 ~/.chimera_config  # Secure the file
```

---

## 2. UAD Volt CC Audio Routing

### Hardware Connections:
- **Input 1**: Main audio source (guitar, synth, etc.) → DSP effects
- **Input 2**: USB microphone → Voice prompts for Trinity AI
- **Output 1**: Processed audio output

### JACK Configuration:

1. **Start JACK with UAD Volt CC:**
```bash
jackd -R -dalsa -dhw:Volt -r48000 -p512 -n3 -i2 -o2 &
```

2. **Verify JACK is running:**
```bash
jack_lsp
```

You should see:
```
system:capture_1  (Input 1 - Main audio)
system:capture_2  (Input 2 - Voice mic)
system:playback_1 (Output 1 - Main out)
system:playback_2 (Output 2 - Secondary)
```

3. **Run ChimeraPhoenix:**
```bash
cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build
./ChimeraPhoenix_Pi
```

### Audio Flow:
```
Input 1 → ChimeraPhoenix (DSP) → Output 1
Input 2 → Voice Recorder → Whisper API → Trinity → Preset Applied
```

---

## 3. Trinity AI Server

### Option A: Run on Mac (Recommended)
On your Mac:
```bash
cd ~/AI_Server
python3 server_trinity_complete.py
```

On Pi, update Trinity URL in the plugin:
- Default is `http://localhost:8000`
- Change to `http://YOUR_MAC_IP:8000` if running on Mac

### Option B: Run on Pi
```bash
cd ~/phoenix-Chimera/pi_deployment/AI_Server
pip3 install -r requirements.txt
export OPENAI_API_KEY="sk-your-key"
python3 server_trinity_complete.py
```

---

## 4. Voice Command Usage

1. **Press "HOLD TO SPEAK" button**
2. **Speak your prompt** (e.g., "warm analog tape saturation")
3. **Button turns red** - Recording (5 second max)
4. **Release or wait** - Processing starts
5. **"Transcribing..."** - Whisper converts speech to text
6. **"Heard: ..."** - Shows what you said
7. **"Trinity: Generating..."** - AI creates preset
8. **Progress bar animates** - `[========    ]`
9. **"Preset Applied!"** - Done!
10. **Preset name displays** at top of screen

---

## 5. Testing the Setup

### Test 1: Check API Key
```bash
# Should return your key
cat ~/.chimera_config
# OR
echo $OPENAI_API_KEY
```

### Test 2: Test JACK Audio
```bash
# Record 5 seconds from Input 2
arecord -D hw:Volt,0 -c 1 -f S16_LE -r 48000 -d 5 test_voice.wav
aplay test_voice.wav  # Should hear your voice
```

### Test 3: Test Whisper API (manual)
```bash
curl https://api.openai.com/v1/audio/transcriptions \
  -H "Authorization: Bearer $OPENAI_API_KEY" \
  -H "Content-Type: multipart/form-data" \
  -F file="@test_voice.wav" \
  -F model="whisper-1"
```

Should return:
```json
{
  "text": "your spoken words here"
}
```

### Test 4: Test Trinity Server
```bash
curl -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt":"warm analog sound"}'
```

Should return Trinity preset JSON.

---

## 6. Troubleshooting

### "Error: No OpenAI API key"
- Check `~/.chimera_config` exists
- OR set `export OPENAI_API_KEY=...`
- Restart the plugin

### "Error: No audio recorded"
- Check JACK is running: `jack_lsp`
- Verify Input 2 connected: `jack_lsp -c`
- Check mic is plugged into UAD Volt Input 2

### "Whisper error"
- Check internet connection
- Verify API key is valid
- Check audio file size > 0 bytes

### "Trinity: Error"
- Check Trinity server is running
- Verify URL in plugin matches server location
- Check server logs for errors

---

## 7. Performance Tips

- **Use wired Ethernet** (not WiFi) for stable API calls
- **Keep prompts under 5 seconds** for best results
- **Speak clearly** 6-12 inches from mic
- **Minimize background noise** for accurate transcription
- **Run Trinity on Mac** for faster generation (optional)

---

## 8. Advanced: Launcher Script

Create `/usr/local/bin/chimera-pi`:
```bash
#!/bin/bash

# Start JACK if not running
if ! pgrep -x "jackd" > /dev/null; then
    echo "Starting JACK..."
    jackd -R -dalsa -dhw:Volt -r48000 -p512 -n3 -i2 -o2 &
    sleep 2
fi

# Load API key
if [ -f ~/.chimera_config ]; then
    source ~/.chimera_config
    export OPENAI_API_KEY
fi

# Launch ChimeraPhoenix Pi
cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build
./ChimeraPhoenix_Pi
```

Make executable:
```bash
chmod +x /usr/local/bin/chimera-pi
```

Then just run:
```bash
chimera-pi
```

---

## Summary

**Required**:
1. OpenAI API key (Whisper access)
2. JACK audio running with UAD Volt CC
3. Trinity AI server accessible (Mac or Pi)
4. Microphone on Input 2

**Voice Workflow**:
Press button → Speak → Whisper transcribes → Trinity generates → Preset applied!
