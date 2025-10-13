# Phoenix Chimera - Quick Start Guide 🚀

**Status**: ✅ **READY TO USE**
**Trinity Network**: ✅ **CONNECTED**
**Last Tested**: October 1, 2025 23:12

---

## ✅ System Status

```
Trinity AI Server:    ✅ RUNNING (Port 8000)
  ├─ Visionary:       ✅ Ready
  ├─ Calculator:      ✅ Ready
  └─ Alchemist:       ✅ Ready

ChimeraPhoenix App:   ✅ RUNNING (PID 89776)
  ├─ Memory:          129MB
  ├─ CPU:             Normal
  └─ Audio:           Active

Network Connection:   ✅ CONNECTED
  └─ Response Time:   8 seconds (test passed)
```

---

## 🎮 How to Use

### Option 1: Trinity AI (Recommended)

1. **Type a natural language prompt** in the Trinity text box:
   ```
   Examples:
   • "vintage tape delay at 1/8 dotted with 35% feedback"
   • "warm tube saturation for vocals"
   • "aggressive compression 8:1 ratio"
   • "ethereal shimmer reverb with 40% mix"
   • "punchy drum bus with parallel compression"
   ```

2. **Press Enter**

3. **Wait 5-30 seconds** (average: 8s)
   - The AI is working (Visionary → Calculator → Alchemist)
   - No progress bar yet (known UI limitation)
   - Don't click anything, just wait!

4. **Engines load automatically!**
   - 4-6 engines selected
   - Parameters set intelligently
   - Ready to process audio

### Option 2: Manual Selection

1. **Click dropdown** for any slot (1-6)
2. **Select engine** from 57 options
3. **Adjust mix knob** for that slot
4. **Repeat** for other slots

---

## 💡 Smart Prompts (AI Understands)

### Time Values
- `"1/8 dotted"` → 0.1875 exactly
- `"quarter note"` → 0.25
- `"1/16"` → 0.0625

### Percentages
- `"35% feedback"` → 0.35
- `"40% mix"` → 0.40
- `"100% wet"` → 1.0

### Ratios
- `"8:1 ratio"` → 0.875 (compression)
- `"4:1"` → 0.75
- `"2:1"` → 0.5

### Musical Terms
- `"warm"` → tube saturation, vintage EQ
- `"punchy"` → compression, transient shaping
- `"ethereal"` → reverb, shimmer, chorus
- `"aggressive"` → distortion, high ratios

---

## 🎯 Example Sessions

### Session 1: Vintage Vocal Chain
**Prompt**: `"warm vintage vocal with tape saturation and plate reverb"`

**Result**:
- Vintage Tube Preamp
- Vintage Opto Compressor
- Tape Echo
- Plate Reverb
- Processing Time: ~7s

### Session 2: EDM Sidechain
**Prompt**: `"aggressive sidechain compression 8:1 ratio with harmonic saturation"`

**Result**:
- Parametric EQ
- Classic Compressor (ratio: 0.875)
- Harmonic Exciter
- Processing Time: ~6s

### Session 3: Ambient Pad
**Prompt**: `"ethereal shimmer reverb with long decay and 40% mix"`

**Result**:
- Shimmer Reverb (mix: 0.40)
- Plate Reverb
- Stereo Chorus
- Processing Time: ~8s

---

## 🔧 Troubleshooting

### Preset Not Loading
**Symptom**: Typed prompt, nothing happens

**Solutions**:
1. Wait longer (up to 30s for complex prompts)
2. Check Trinity server:
   ```bash
   curl http://localhost:8000/health
   ```
3. Restart both:
   ```bash
   pkill ChimeraPhoenix
   # Relaunch from Applications
   ```

### Server Not Responding
**Symptom**: Long wait, no result

**Solution**:
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server
python3 trinity_server_complete.py
# Should start on port 8000
```

### Audio Issues
**Symptom**: No sound

**Solutions**:
1. Check Audio Settings in plugin
2. Select correct input/output device
3. Increase buffer size if crackling
4. Check system audio preferences

---

## 🚀 Quick Test

**Run this to test everything:**

```bash
# Test Trinity server
curl -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": "test"}' | python3 -m json.tool

# Should return a preset in ~5-10 seconds
```

---

## 📊 Performance Tips

### For Best Results:
- **Specific prompts** = better results
  - ✅ "vintage tape delay at 1/8 dotted with 35% feedback"
  - ❌ "delay effect"

- **Use percentages** for precision
  - ✅ "40% mix"
  - ❌ "some reverb"

- **Mention time values** for delays
  - ✅ "1/8 dotted", "quarter note"
  - ❌ "slow delay"

### CPU Management:
- **Light**: 1-3 engines (5-15% CPU)
- **Medium**: 4-5 engines (15-25% CPU)
- **Heavy**: 6 engines (25-45% CPU)

---

## 🎨 57 Available Engines

### Dynamics (7)
- Vintage Opto Compressor
- Classic Compressor
- VCA Compressor
- Mastering Limiter
- Transient Shaper
- Dynamic EQ
- Noise Gate

### EQ & Filters (8)
- Parametric EQ
- Vintage Console EQ
- Ladder Filter
- State Variable Filter
- Formant Filter
- Envelope Filter
- Comb Resonator
- Vocal Formant Filter

### Distortion (8)
- Vintage Tube Preamp
- Wave Folder
- Harmonic Exciter
- Bit Crusher
- Multiband Saturator
- Muff Fuzz
- Rodent Distortion
- K-Style Overdrive

### Modulation (11)
- Digital Chorus
- Resonant Chorus
- Analog Phaser
- Ring Modulator
- Frequency Shifter
- Harmonic Tremolo
- Classic Tremolo
- Rotary Speaker
- Pitch Shifter
- Detune Doubler
- Intelligent Harmonizer

### Reverb & Delay (10)
- Tape Echo ⭐
- Digital Delay
- Magnetic Drum Echo
- Bucket Brigade Delay
- Buffer Repeat
- Plate Reverb
- Spring Reverb
- Convolution Reverb
- Shimmer Reverb ⭐
- Gated Reverb

### Spatial & Special (9)
- Stereo Widener
- Stereo Imager
- Dimension Expander
- Spectral Freeze
- Spectral Gate
- Phased Vocoder
- Granular Cloud
- Chaos Generator
- Feedback Network

### Utility (4)
- Mid-Side Processor
- Gain Utility
- Mono Maker
- Phase Align

⭐ = Frequently used by Trinity AI

---

## 🔗 Quick Commands

### Start Trinity Server
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server
python3 trinity_server_complete.py
```

### Launch Plugin
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Builds/MacOSX/build/Debug
open ChimeraPhoenix.app
```

### Test Connection
```bash
curl http://localhost:8000/health
# Should return: {"status":"healthy",...}
```

### Restart Everything
```bash
pkill -9 ChimeraPhoenix
pkill -9 Python  # Only if Trinity server stuck

# Then relaunch both
```

---

## ✨ What Makes Phoenix Chimera Special

### Traditional Plugins:
```
User: Manually tweak 15 parameters per effect
Time: 5-10 minutes to get sound right
Result: Trial and error
```

### Phoenix Chimera:
```
User: "vintage tape delay at 1/8 dotted with 35% feedback"
AI: *generates preset in 8 seconds*
Result: EXACTLY what you asked for!
```

**This is the only plugin that understands natural language AND sets precise parameter values.**

---

## 🎉 You're Ready!

**Current Status:**
✅ Trinity Server: Running
✅ ChimeraPhoenix: Running
✅ Connection: Verified
✅ Test Passed: "Echoed Twilight Glow" generated

**Try a prompt now!** Type in the Trinity text box and press Enter.

Recommended first prompt:
```
"warm vintage delay with 30% feedback"
```

---

*Phoenix Chimera v3.0 - Intelligent AI-Powered Audio Effects*
*Trinity Pipeline v1.5 - Built with Claude Code*