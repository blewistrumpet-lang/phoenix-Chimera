# Trinity AI Pipeline Architecture
## Chimera Phoenix Pi - Voice-to-Preset System

**Last Updated:** October 12, 2025
**Version:** trinity_server_pi.py (Oracle-free, optimized for Raspberry Pi)

---

## Overview

The Trinity AI Pipeline transforms voice/text prompts into professional audio presets through three intelligent stages:

```
Voice/Text → Visionary → Calculator → Alchemist → Complete Preset
   (Whisper)    (GPT-4)     (GPT-4)   (Validation)    (Plugin)
```

**Key Features:**
- 57 boutique DSP engines available
- Professional gain staging analysis
- Musical parameter intelligence
- Real-time progress tracking
- Raspberry Pi optimized (reduced API calls)

---

## System Architecture

### Audio Configuration

**HiFiBerry DAC+ADC Pro (Card 3)**
- ALSA Device: `hw:sndrpihifiberry`
- Purpose: Main audio I/O for plugin processing
- Mode: Direct ALSA (no JACK required)
- Configuration: `~/.config/ChimeraPhoenix.settings`

**USB Microphone (Card 2)**
- Device: "USB PnP Sound Device"
- Purpose: Voice input for Whisper transcription
- API Endpoint: `POST /transcribe`

### Server Components

**Trinity Server:** `trinity_server_pi.py`
- Port: 8000
- Progress tracking: `/tmp/trinity_progress/`
- Health check: `GET /health`

**API Key:** OpenAI API configured in `.env`

---

## Pipeline Stages

### Stage 0: Voice Transcription (Optional)

**Endpoint:** `POST /transcribe`

**Process:**
1. User records voice via USB mic
2. Plugin sends WAV file to server
3. OpenAI Whisper API transcribes audio
4. Returns text prompt for generation

**Implementation:**
```python
client.audio.transcriptions.create(
    model="whisper-1",
    file=audio_file
)
```

---

### Stage 1: VISIONARY 🎨

**Purpose:** Creative engine selection and preset design

**File:** `visionary_complete.py`
**Class:** `CompleteVisionary`

#### Process Flow

**1. Prompt Analysis**
```python
context = analyze_prompt_context(prompt)
# Extracts: character, intensity, spatial qualities
```

Character types:
- **Warm:** Golden, velvet, rich tones
- **Harsh:** Scorched, grinding, aggressive
- **Dark:** Obsidian, shadow, mysterious
- **Ethereal:** Celestial, crystalline, airy
- **Mechanical:** Industrial, synthetic, precise

**2. Routing Strategy**

Two intelligent selection modes:

**A) Rule-Based (Fast, Coherent)**
- Uses `engine_selector.py` with hardcoded sonic knowledge
- Respects `FORBIDDEN_COMBINATIONS`
  - Example: No dual reverbs (PlateReverb + ShimmerReverb)
- Uses `SONIC_SIGNATURES` database
- Best for: Character-based prompts ("warm pad", "harsh lead")

**B) Hybrid (Creative, Deep Knowledge)**
- **GPT-4o API call** with complete engine catalog
- System prompt includes all 57 engines:
  ```
  Engine 39: PlateReverb
    Function: Smooth metallic reverb
    Character: Warm, spacious
    Parameters: 15 (Mix at index 14)
  ```
- Returns structured JSON with reasoning
- Best for: Complex creative prompts

**3. Preset Naming**

`PresetNameGenerator` creates evocative names:
```python
adjectives = {
    "harsh": ["Scorched", "Shattered", "Grinding"],
    "warm": ["Golden", "Velvet", "Amber"],
    "dark": ["Obsidian", "Shadow", "Midnight"]
}
nouns = {
    "spatial": ["Cathedral", "Chamber", "Expanse"],
    "elemental": ["Ember", "Frost", "Thunder"]
}
```

Examples:
- "Scorched Cathedral" (harsh + spatial)
- "Golden Reverie" (warm + emotional)
- "Obsidian Expanse" (dark + spatial)

**4. Output Format**
```json
{
  "name": "Celestial Shimmer",
  "slots": [
    {
      "slot": 1,
      "engine_id": 39,
      "parameters": [
        {"index": 0, "value": 0.5, "name": "PreDelay"},
        {"index": 1, "value": 0.7, "name": "Decay"}
      ]
    }
  ],
  "reasoning": "PlateReverb provides spatial foundation with medium decay..."
}
```

#### Progress Updates

```
Stage: "visionary"
Percent: 5% → 40%
Message: "Starting creative generation..." → "Creative generation complete: [preset name]"
```

---

### Stage 2: CALCULATOR 🧮

**Purpose:** Professional parameter optimization with musical intelligence

**File:** `calculator_max_intelligence.py`
**Class:** `MaxIntelligenceCalculator`

#### Knowledge Base

Loads `trinity_engine_knowledge_COMPLETE.json`:
```json
{
  "engines": {
    "39": {
      "name": "PlateReverb",
      "category": "Reverb",
      "parameters": [
        {
          "name": "PreDelay",
          "default": 0.0,
          "min": 0.0,
          "max": 1.0,
          "description": "Pre-delay time before reverb",
          "units": "ms"
        }
      ]
    }
  }
}
```

#### Core Features

**1. Unified Intelligent Optimization** (Pi-optimized)

Single GPT-4 API call combines three operations:
- Style parameter optimization
- Conflict resolution
- Gain staging fixes

**Sends to GPT-4:**
```
USER PROMPT: "warm vintage delay with shimmer"

ENGINES IN CHAIN:
Engine 34: TapeEcho
  param1: DelayTime
  param2: Feedback
  param5: Mix

Engine 42: ShimmerReverb
  param1: Decay
  param8: ShimmerAmount

CURRENT STATE:
Slot 1: Engine 34
  param1: 0.5
  param2: 0.5

GAIN STAGING ANALYSIS:
- Cumulative gain: +2.1dB
- Headroom available: -23.9dB (SAFE)
- No warnings

INSTRUCTIONS: Optimize parameters for musical coherence and professional sound.
```

**GPT-4 Returns:**
```json
{
  "style_parameters": {
    "34": {
      "param1": 0.5,
      "param2": 0.6,
      "param5": 0.4
    },
    "42": {
      "param1": 0.7,
      "param8": 0.5,
      "param14": 0.3
    }
  },
  "conflict_fixes": {},
  "reasoning": "TapeEcho feedback set to 60% for vintage character. ShimmerReverb decay extended to 70% for spaciousness. Mix levels balanced to prevent masking."
}
```

**2. Professional Gain Staging Analysis** ⚡

`GainStagingAnalyzer` class provides studio-grade signal flow analysis:

**Gain Profiles by Category:**
```python
gain_profiles = {
    "Distortion": {"typical_gain": +6, "range": (0, +18), "can_clip": True},
    "Dynamics": {"typical_gain": -3, "range": (-12, +6)},
    "EQ": {"typical_gain": +3, "range": (-12, +12)},
    "Reverb": {"typical_gain": 0, "range": (-3, +3)}
}
```

**Analysis Example:**
```
Signal Chain:
[Slot 1: MuffFuzz +12dB] → [Slot 2: ParametricEQ +3dB] → [Slot 3: PlateReverb 0dB]

Cumulative Gain: +15dB
Headroom: -21dB (SAFE - no warnings)
```

**Warning System:**
```
⚠️  WARNING: Cumulative gain +9dB at slot 3
    Approaching clipping threshold

📊 RECOMMENDATION: Reduce MuffFuzz drive to 60% (slot 1)
    Expected result: +6dB gain reduction
```

**Frequency Masking Detection:**
```python
frequency_bands = {
    "sub": (20, 60),
    "bass": (60, 250),
    "low_mid": (250, 500),
    "mid": (500, 2000),
    "high_mid": (2000, 4000),
    "presence": (4000, 6000),
    "brilliance": (6000, 20000)
}
```

Detects buildup in specific frequency ranges and recommends EQ adjustments.

**3. Musical Parameter Parsing** 🎵

Intelligent parsing of musical terminology:

**Time Subdivisions:**
```python
"slow tremolo at 1/4 dotted notes"
→ subdivision = 0.375 (dotted quarter)

"fast delay at 1/16th notes"
→ subdivision = 0.0625 (sixteenth)
```

**Frequency Values:**
```python
"bright 3.5kHz shelf boost"
→ frequency_param = normalize_frequency(3500, min=20, max=20000)

"remove low end at 80Hz"
→ frequency_param = normalize_frequency(80, min=20, max=20000)
```

**Percentages:**
```python
"70% mix" → 0.7
"subtle 20% effect" → 0.2
"full 100% wet" → 1.0
```

**Ratios (Compression):**
```python
"4:1 ratio" → compression_ratio = 0.75
"gentle 2:1" → compression_ratio = 0.5
"brick wall 10:1" → compression_ratio = 0.9
```

**dB Values:**
```python
"+3dB boost" → gain = 0.65 (normalized)
"-6dB cut" → gain = 0.35 (normalized)
"0dB unity" → gain = 0.5
```

**4. Parameter Semantic Mapping**

Handles parameter name aliases:
```python
"feedback" = "regen" = "regeneration"
"time" = "delay"
"mix" = "wet" = "blend"
"drive" = "gain" = "intensity"
```

**5. Safety Validation**

```python
# Clamp all values to valid range
param_value = max(0.0, min(1.0, float(value)))

# Handle invalid responses
try:
    param_value = float(value)
except (ValueError, TypeError):
    logger.warning(f"Invalid value: {value}, using 0.5")
    param_value = 0.5
```

**6. Caching System**

Avoids duplicate GPT calls:
```python
cache_key = hashlib.md5(prompt.encode()).hexdigest()
if cache_key in self.intelligence_cache:
    return cached_result
```

#### Progress Updates

```
Stage: "calculator"
Percent: 40% → 80%
Message: "Calculating parameters for [preset name]..."
```

---

### Stage 3: ALCHEMIST ⚗️

**Purpose:** Format and validate for plugin consumption

**File:** `alchemist_complete.py`
**Class:** `CompleteAlchemist`

#### Process

**1. Validation**
```python
validation_report = {
    "valid": True/False,
    "errors": [],
    "warnings": []
}

# Checks:
- JSON structure integrity
- Required fields present
- Value ranges (0.0-1.0)
- Engine ID validity (0-56)
- Slot count (max 6)
```

**2. Format for Plugin**
```python
final_preset = {
    "name": "Preset Name",
    "slots": [
        {
            "slot": 1,
            "engine_id": 39,
            "parameters": [0.5, 0.7, 0.3, ...]  # Array of floats
        }
    ]
}
```

**3. Return Package**
```json
{
  "preset": { /* final preset */ },
  "debug": {
    "timestamp": "2025-10-12T...",
    "visionary": { /* reasoning */ },
    "calculator": { /* optimizations */ },
    "alchemist": { /* validation report */ }
  }
}
```

#### Progress Updates

```
Stage: "alchemist"
Percent: 80% → 100%
Message: "Formatting preset [preset name] for plugin..."
```

---

## Intelligence Features Summary

### 1. Gain Staging Analysis (Professional DSP)

**Example Safe Chain:**
```
[Distortion +6dB] → [EQ +3dB] → [Reverb 0dB]
Cumulative: +9dB
Headroom: -17dB remaining (SAFE)
```

**Example Warning:**
```
[MuffFuzz +12dB] → [HarmonicExciter +6dB] → [ParametricEQ +3dB]
Cumulative: +21dB ⚠️  DANGER
Headroom: -5dB (CLIPPING RISK)

RECOMMENDATION:
- Reduce MuffFuzz drive to 50% → +6dB gain
- Result: +15dB cumulative, -11dB headroom (SAFE)
```

### 2. Musical Parameter Intelligence

**Complex Prompt:**
```
"tape delay with dotted eighth notes at 120bpm,
 warm 2kHz boost,
 70% mix,
 gentle 3:1 compression on repeats"
```

**Calculator Parses:**
- Time: 0.1875 (dotted 8th subdivision)
- Frequency: 2000Hz normalized to EQ range
- Mix: 0.7
- Ratio: 0.67 (3:1 compression)

### 3. Conflict Detection

**Forbidden Combinations:**
```python
FORBIDDEN_COMBINATIONS = {
    (39, 42): "PlateReverb + ShimmerReverb creates mud",
    (27, 28): "FrequencyShifter + HarmonicTremolo phase conflicts"
}
```

**Resolution:**
- Reduces conflicting engine mix to 30%
- Adjusts parameters for coherence
- Logs warning in debug info

### 4. Collaborative Intelligence

**Rule-Based** (Fast path):
```python
prompt = "warm pad"
→ Use hardcoded sonic knowledge
→ Select: VintageConsoleEQ + PlateReverb + StereoWidener
→ No API call needed
```

**Hybrid** (Creative path):
```python
prompt = "evolving celestial texture with subtle pitch modulation"
→ Complex creative requirement
→ Call GPT-4 with full engine catalog
→ AI reasons about sonic architecture
```

---

## Data Flow Example

### Complete Pipeline Execution

**User Input:**
```
Voice: "warm vintage delay with shimmer"
```

**Stage 0: Transcription**
```
Whisper API → Text: "warm vintage delay with shimmer"
Progress: 0-5%
```

**Stage 1: Visionary**
```
GPT-4o Analysis:
- Character: "warm"
- Effects needed: delay, reverb
- Spatial quality: shimmer

Selected Engines:
- Slot 1: TapeEcho (34) - "vintage delay character"
- Slot 2: ShimmerReverb (42) - "shimmer effect"

Generated Name: "Golden Echo Cathedral"
Progress: 5-40%
```

**Stage 2: Calculator**
```
GPT-4 Optimization:
- TapeEcho:
  param1 (DelayTime): 0.5 (medium delay)
  param2 (Feedback): 0.6 (vintage character)
  param5 (Mix): 0.4 (balanced)

- ShimmerReverb:
  param1 (Decay): 0.7 (spacious)
  param8 (ShimmerAmount): 0.5 (noticeable but tasteful)
  param14 (Mix): 0.3 (subtle blend)

Gain Staging:
- TapeEcho: 0dB
- ShimmerReverb: 0dB
- Cumulative: 0dB
- Headroom: -26dB (EXCELLENT)

Progress: 40-80%
```

**Stage 3: Alchemist**
```
Validation: PASS
- All values in range
- No conflicts detected
- JSON structure valid

Final Format:
{
  "name": "Golden Echo Cathedral",
  "slots": [
    {"slot": 1, "engine_id": 34, "parameters": [0.5, 0.6, 0.5, 0.5, 0.4, ...]},
    {"slot": 2, "engine_id": 42, "parameters": [0.7, 0.5, 0.5, ..., 0.3]}
  ]
}

Progress: 80-100%
```

**Plugin Receives:**
- Complete preset loaded
- Parameters applied
- Audio processing begins

---

## API Endpoints

### POST /transcribe
**Purpose:** Convert voice to text
**Input:** WAV audio file (multipart/form-data)
**Output:**
```json
{
  "text": "warm vintage delay with shimmer",
  "request_id": "transcribe_1697123456789"
}
```

### POST /generate
**Purpose:** Generate preset from text prompt
**Input:**
```json
{
  "prompt": "warm vintage delay with shimmer",
  "request_id": "generate_1697123456789"
}
```
**Output:**
```json
{
  "preset": { /* complete preset */ },
  "debug": { /* pipeline details */ }
}
```

### GET /progress/{request_id}
**Purpose:** Real-time progress tracking
**Output:**
```json
{
  "stage": "calculator",
  "percent": 65,
  "message": "Calculating parameters for Golden Echo Cathedral...",
  "preset_name": "Golden Echo Cathedral",
  "timestamp": 1697123456.789
}
```

### GET /health
**Purpose:** Server health check
**Output:**
```json
{
  "status": "healthy",
  "timestamp": "2025-10-12T...",
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

## Progress Tracking System

### Implementation

**Location:** `/tmp/trinity_progress/`

**File Format:** `{request_id}.json`

**Structure:**
```json
{
  "stage": "visionary|calculator|alchemist|error",
  "percent": 0-100,
  "message": "Status message",
  "preset_name": "Generated preset name",
  "timestamp": 1697123456.789
}
```

**Stages:**
```
Initialization: 0-5%
Visionary: 5-40%
Calculator: 40-80%
Alchemist: 80-100%
```

**Cleanup:** Auto-removes files older than 1 hour

### Plugin Integration

**Polling Loop:**
```cpp
// Plugin polls every 100ms
while (generating) {
    response = httpGet("/progress/" + requestId);
    updateProgressBar(response.percent);
    updateStatusText(response.message);
    sleep(100ms);
}
```

---

## Performance Optimization (Pi-Specific)

### API Call Reduction

**Before (Desktop version):**
- Visionary: 1 GPT-4 call
- Calculator: 3 GPT-4 calls (style, conflicts, creative)
- Total: 4 calls (~20-30 seconds)

**After (Pi version):**
- Visionary: 1 GPT-4o call
- Calculator: 1 unified GPT-4 call
- Total: 2 calls (~10-15 seconds)

### Unified Optimization Call

Single GPT-4 call handles:
```
1. Style parameter optimization
   → Musical coherence
   → Sonic character matching

2. Conflict resolution
   → Frequency masking
   → Phase cancellation

3. Gain staging fixes
   → Headroom management
   → Clipping prevention
```

**Performance Gain:**
- 50% fewer API calls
- 40% faster generation
- Same quality output

---

## Configuration Files

### trinity_engine_knowledge_COMPLETE.json

Complete engine database:
```json
{
  "engines": {
    "0": {"name": "None", "category": "Utility"},
    "1": {"name": "VintageOptoCompressor", "category": "Dynamics"},
    ...
    "56": {"name": "PhaseAlign", "category": "Utility"}
  },
  "categories": ["Dynamics", "EQ", "Filter", "Distortion", ...],
  "total_engines": 57
}
```

### .env

Required configuration:
```bash
OPENAI_API_KEY=sk-proj-...
```

---

## Troubleshooting

### Server Won't Start

**Check API Key:**
```bash
cat ~/phoenix-Chimera/AI_Server/.env | grep OPENAI_API_KEY
```

**Check Python Environment:**
```bash
~/phoenix-Chimera/AI_Server/venv/bin/python3 -c "import openai; print(openai.__version__)"
# Should output: 1.3.5 or later
```

**Check Logs:**
```bash
tail -f /tmp/trinity_server.log
```

### Voice Transcription Fails

**Test USB Mic:**
```bash
arecord -l
# Should show: card 2: Device [USB PnP Sound Device]
```

**Test Recording:**
```bash
arecord -D hw:2,0 -f cd -d 3 test.wav
aplay test.wav
```

### Progress Not Updating

**Check Progress Directory:**
```bash
ls -la /tmp/trinity_progress/
# Should contain .json files during generation
```

**Test Progress Endpoint:**
```bash
curl http://localhost:8000/progress/test_123
```

### Audio Routing Issues

**Check HiFiBerry Configuration:**
```bash
cat ~/.config/ChimeraPhoenix.settings | grep deviceType
# Should show: deviceType="ALSA"

cat ~/.config/ChimeraPhoenix.settings | grep audioInputDeviceName
# Should show: audioInputDeviceName="hw:sndrpihifiberry"
```

**Test HiFiBerry:**
```bash
speaker-test -D hw:sndrpihifiberry -c 2 -t sine
```

---

## Development Notes

### Why Two GPT Calls?

**Separation of Concerns:**

1. **Visionary** = Engine *selection*
   - "Which effects should I use?"
   - Creative architecture decision
   - High-level sonic design

2. **Calculator** = Parameter *optimization*
   - "How should I configure them?"
   - Technical parameter tuning
   - Professional gain staging

**Benefits:**
- Better prompts (specialized for each task)
- Caching (don't re-select engines when tweaking)
- Fallback resilience (rule-based selection if GPT fails)
- Debugging clarity (isolate which stage failed)

### Rule-Based vs AI Selection

**Use Rule-Based When:**
- Character-based prompts ("warm pad", "harsh lead")
- Need speed (no API call)
- Want predictable results
- Budget-conscious (no API cost)

**Use AI When:**
- Complex creative prompts
- Need deep sonic reasoning
- Want unexpected combinations
- Quality over speed

**Use Hybrid When:**
- Best of both worlds
- Rules ensure coherence
- AI adds creativity

---

## Future Enhancements

### Planned Features

1. **Local LLM Support**
   - Run Llama 3.1 8B on Pi 5
   - Eliminate API dependency
   - Reduce latency to <5 seconds

2. **Preset Learning**
   - User feedback on generated presets
   - Fine-tune selection algorithms
   - Build personal style profiles

3. **Multi-Modal Input**
   - Audio reference matching ("sound like this")
   - Image-to-sound ("visualizer-driven presets")
   - Gesture control integration

4. **Advanced Routing**
   - Parallel processing chains
   - Feedback loops
   - Sidechain ducking

---

## Credits

**Trinity AI Pipeline:** Branden Lewis
**OpenAI Models:** GPT-4o, Whisper-1
**JUCE Framework:** ROLI Ltd.
**Raspberry Pi:** Raspberry Pi Foundation

---

🤖 **Generated with Claude Code** - Documentation Assistant
📅 **Last Updated:** October 12, 2025
