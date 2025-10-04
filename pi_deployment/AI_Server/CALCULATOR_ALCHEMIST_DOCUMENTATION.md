# Calculator and Alchemist Component Documentation

## Overview

The Calculator and Alchemist are two critical components in the Trinity AI pipeline that work together to transform raw preset selections into polished, safe, and musically useful presets.

## Component Verification Results ✅

### Calculator Component - VERIFIED WORKING
- **Keyword-based nudging**: Successfully applies parameter adjustments based on prompt keywords
- **Character-based nudging**: Applies additional nudges based on engine character descriptions
- **Parameter clamping**: Ensures all nudged values stay within 0.0-1.0 range
- **Nudge tracking**: Records which parameters were modified for transparency

### Alchemist Component - VERIFIED WORKING
- **Parameter validation**: Clamps all values to valid ranges
- **Safety checks**: Prevents audio issues (clipping, feedback runaway, silent output)
- **Structure completion**: Ensures all required parameters exist with sensible defaults
- **Creative naming**: Generates contextual preset names based on vibe
- **Warning system**: Identifies potential issues without blocking functionality

---

## Calculator Component

### Purpose
The Calculator applies intelligent parameter nudges based on the user's prompt and the Visionary's blueprint. It acts as the "fine-tuning" stage that personalizes presets to match user intent.

### Key Features

#### 1. Keyword-Based Nudging
Recognizes descriptive keywords in prompts and applies corresponding parameter adjustments:

```python
# Example keyword rules:
"dark": {
    "slot1_param2": -0.2,  # Reduce tone/brightness
    "slot2_param1": 0.1    # Increase reverb size
}
"aggressive": {
    "slot1_param1": 0.3,   # Increase drive
    "slot1_param3": 0.1    # Increase level
}
```

**Supported Keywords**:
- **Mood**: dark, bright, warm, vintage, modern
- **Intensity**: aggressive, subtle, punchy, tight
- **Space**: spacious, ambient, ethereal

#### 2. Character-Based Nudging
Applies nudges based on engine character descriptions from the Visionary:

```python
"warm": +0.1 drive, -0.05 tone
"aggressive": +0.2 drive, +0.1 level
"spacious": +0.15 size, +0.1 mix
```

#### 3. Nudge Tracking
All modified parameters are tracked in `calculator_nudges` array for transparency.

### Test Results
- ✅ Keyword detection and application working
- ✅ Parameter clamping prevents out-of-range values
- ✅ Multiple keywords can be applied cumulatively
- ✅ Character-based nudging supplements keyword nudging

---

## Alchemist Component

### Purpose
The Alchemist performs final validation, safety checks, and creative enhancements to ensure presets are production-ready. It's the "quality assurance" stage of the pipeline.

### Key Features

#### 1. Parameter Validation
- **Range Clamping**: All parameters clamped to 0.0-1.0
- **Type Checking**: Engine selectors converted to integers
- **Engine-Specific Limits**: Special ranges for sensitive parameters (e.g., feedback < 0.95)

#### 2. Safety Checks

**Total Gain Limiting**:
- Prevents clipping by limiting combined slot gains to 2.0
- Scales down mix levels proportionally if needed

**Feedback Prevention**:
- Limits feedback parameters to 0.95 maximum
- Applies to delay/echo engines (IDs: 1, 8, 9, 53)

**Active Slot Guarantee**:
- Ensures at least one slot is active
- Activates slot 1 with safe defaults if all bypassed

#### 3. Structure Completion
Ensures every preset has:
- All 78 slot parameters (6 slots × 13 params)
- Master controls (input, output, mix)
- Required metadata fields

#### 4. Creative Name Generation

**Vibe-Based Naming**:
```
"aggressive" → "Crushing Generator", "Brutal Machine"
"warm" → "Golden Creator", "Smooth Engine"
"spacious" → "Infinite Echo", "Vast Chamber"
"experimental" → "Sonic Laboratory", "Sound Discovery"
```

**Name Structure**: `[Adjective] [Noun] [Optional Suffix]`

#### 5. Validation Warnings
Non-blocking warnings for:
- Very high parameters (> 0.9)
- Very low parameters (< 0.1)
- Potential feedback risks
- Unusual parameter combinations

### Test Results
- ✅ Out-of-range parameters correctly clamped
- ✅ Total gain limiting prevents clipping
- ✅ All slots bypassed scenario handled
- ✅ Complete parameter structure ensured
- ✅ Creative names generated based on vibe
- ✅ Validation warnings correctly identified

---

## Integration with Trinity Pipeline

### Data Flow
1. **Oracle** → Selects base preset
2. **Calculator** → Applies intelligent nudges based on prompt
3. **Alchemist** → Validates, ensures safety, adds creative touches
4. **Result** → Production-ready preset with proper naming

### Example Workflow

**Input Prompt**: "Make a dark aggressive preset with vintage character"

1. **Calculator Stage**:
   - Detects keywords: "dark", "aggressive", "vintage"
   - Applies nudges:
     - `slot1_param2`: -0.2 (darker tone)
     - `slot1_param1`: +0.3 (more drive)
     - `slot2_param4`: +0.2 (vintage wow)

2. **Alchemist Stage**:
   - Validates all parameters in range
   - Checks total gain (scales if needed)
   - Generates name: "Brutal Vintage Machine"
   - Adds validation metadata

**Output**: Safe, customized preset ready for the plugin

---

## Configuration Files

### nudge_rules.json
Contains keyword mappings and engine-specific adjustments. Can be modified to customize behavior without code changes.

### Safety Limits (Hardcoded)
- `max_total_gain`: 2.0
- `max_feedback`: 0.95
- `min_output_level`: 0.1

---

## Conclusion

Both Calculator and Alchemist components are functioning correctly and provide essential preset customization and safety features. They work seamlessly together to transform raw preset selections into polished, user-tailored results while preventing potential audio issues.