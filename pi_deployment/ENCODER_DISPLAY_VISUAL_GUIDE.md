# EncoderDisplay Visual Comparison Guide

## Before vs After

### BEFORE: Raw Encoder Position
```
┌──────────────────────┐
│                      │
│      ENC 1           │  ← Generic label
│                      │
│       -24            │  ← Meaningless integer
│                      │
│        ●             │
└──────────────────────┘
```
**Problems:**
- User sees "-24" - what does this mean?
- No context about what parameter is being controlled
- No units or meaningful values
- No indication of bank state (A or B)

---

### AFTER: Meaningful Parameter Display

#### Example 1: Input Gain (Bank A, at -6 dB)
```
┌──────────────────────┐
│ A                    │  ← Bank indicator (cyan)
│                      │
│      Input           │  ← Parameter name
│                      │
│    -6.0 dB           │  ← Formatted value with units
│                      │
│        ●             │
└──────────────────────┘
```

#### Example 2: Mix Control (Bank B, at 75%)
```
┌──────────────────────┐
│ B                    │  ← Bank indicator (orange)
│                      │
│       Mix            │  ← Parameter name
│                      │
│       75%            │  ← Percentage display
│                      │
│        ●             │
└──────────────────────┘
```

#### Example 3: Output Level (Bank A, at +3 dB)
```
┌──────────────────────┐
│ A                    │  ← Bank indicator (cyan)
│                      │
│     Output           │  ← Parameter name
│                      │
│    +3.0 dB           │  ← Positive gain shown with +
│                      │
│        ●             │
└──────────────────────┘
```

## Color Coding

### Bank A (Default)
- **Border Color:** Cyan (#00b6d4)
- **Bank Indicator:** "A" in cyan with 60% transparency
- **Use Case:** Primary sound/settings

### Bank B (Alternate)
- **Border Color:** Orange (#ffaa00)
- **Bank Indicator:** "B" in orange with 60% transparency
- **Use Case:** Alternate sound for A/B comparison

### Common Elements
- **Background:** Dark gray (#2a2a2a)
- **Parameter Name:** White, 12px bold
- **Parameter Value:** Matches border color, 16px bold
- **Button (Pressed):** Red (#ef4444)
- **Button (Idle):** Dark gray (#444444)

## Complete System View

### PRESET Mode - Bank A
```
Encoder 1            Encoder 2            Encoder 3
┌──────────┐        ┌──────────┐        ┌──────────┐
│ A        │        │ A        │        │ A        │
│          │        │          │        │          │
│  Input   │        │   Mix    │        │  Output  │
│          │        │          │        │          │
│ -12.0 dB │        │   50%    │        │ +0.0 dB  │
│          │        │          │        │          │
│    ●     │        │    ●     │        │    ●     │
└──────────┘        └──────────┘        └──────────┘
```

### PRESET Mode - Bank B
```
Encoder 1            Encoder 2            Encoder 3
┌──────────┐        ┌──────────┐        ┌──────────┐
│ B        │        │ B        │        │ B        │
│          │        │          │        │          │
│  Input   │        │   Mix    │        │  Output  │
│          │        │          │        │          │
│ -6.0 dB  │        │   100%   │        │ +3.0 dB  │
│          │        │          │        │          │
│    ●     │        │    ●     │        │    ●     │
└──────────┘        └──────────┘        └──────────┘
```
*Note: Orange borders on Bank B (not shown in ASCII)*

### AI Mode - Bank A
```
Encoder 1            Encoder 2            Encoder 3
┌──────────┐        ┌──────────┐        ┌──────────┐
│ A        │        │ A        │        │ A        │
│          │        │          │        │          │
│Complexity│        │  Refine  │        │  Evolve  │
│          │        │          │        │          │
│   33%    │        │   50%    │        │   66%    │
│          │        │          │        │          │
│    ●     │        │    ●     │        │    ●     │
└──────────┘        └──────────┘        └──────────┘
```

## Value Formatting Examples

### Input/Output Gain (dB Display)

| Normalized | Actual | Display    | Description         |
|------------|--------|------------|---------------------|
| 0.00       | 0.00   | -inf dB    | Complete silence    |
| 0.25       | 0.50   | -6.0 dB    | Half power          |
| 0.50       | 1.00   | 0.0 dB     | Unity gain (no change) |
| 0.75       | 1.50   | +3.5 dB    | 50% boost           |
| 1.00       | 2.00   | +6.0 dB    | Double power        |

**Formula:** `dB = 20 * log10(actualValue)`

### Mix Control (Percentage Display)

| Normalized | Display | Description         |
|------------|---------|---------------------|
| 0.00       | 0%      | 100% dry (bypass)   |
| 0.25       | 25%     | Mostly dry          |
| 0.50       | 50%     | Equal mix           |
| 0.75       | 75%     | Mostly wet          |
| 1.00       | 100%    | 100% wet (full FX)  |

**Formula:** `percentage = normalizedValue * 100`

## Button State Indication

### Idle State
```
┌──────────┐
│ A        │
│          │
│  Input   │
│          │
│ -6.0 dB  │
│          │
│    ○     │  ← Gray circle (not pressed)
└──────────┘
```

### Pressed State
```
┌──────────┐
│ A        │
│          │
│  Input   │
│          │
│ -6.0 dB  │
│          │
│    ●     │  ← Red circle (pressed)
└──────────┘
```

## Layout Specifications

### Component Dimensions
- **Total Size:** 100px × 80px
- **Border Radius:** 8px
- **Border Width:** 2px

### Text Layout
```
Row 1 (0-16px):   Bank indicator "A" or "B"
                  - Font: 10px bold
                  - Position: Top-left corner
                  - Alpha: 60%

Row 2 (16-34px):  Parameter name
                  - Font: 12px bold
                  - Color: White
                  - Alignment: Centered

Row 3 (34-60px):  Parameter value
                  - Font: 16px bold
                  - Color: Border color (cyan/orange)
                  - Alignment: Centered

Row 4 (60-80px):  Button indicator
                  - Ellipse: 50px × 12px
                  - Position: Bottom center
                  - Color: Red (pressed) / Gray (idle)
```

## Implementation Notes

### Update Frequency
- **Timer:** 30Hz (every 33ms)
- **Source:** Direct from APVTS parameters
- **Latency:** Minimal (~33ms worst case)

### Value Synchronization
1. Timer callback fires
2. Fetch parameter from APVTS using `param->getValue()`
3. Get parameter label from ControlState
4. Get bank indicator from ControlState variant
5. Call `setParameterInfo()` with all values
6. Display automatically updates via `repaint()`

### Performance Considerations
- **Efficient repainting:** Only repaints when values change
- **String formatting:** Cached until parameter changes
- **No blocking operations:** All updates on message thread
- **Minimal allocations:** Uses string formatting inline

## User Experience Benefits

### Before (Raw Position)
❌ User turns encoder, sees "-24" → Confused
❌ Doesn't know what parameter is being controlled
❌ Can't tell if value is good or bad
❌ No indication of bank state
❌ Must memorize encoder assignments

### After (Formatted Display)
✅ User turns encoder, sees "Input: -6.0 dB" → Clear understanding
✅ Knows exactly what parameter is being controlled
✅ Can see value in professional audio units
✅ Bank indicator shows A/B state instantly
✅ Display adapts to mode changes automatically
✅ Can compare values between banks visually

## Testing Scenarios

### Test 1: Gain Display Accuracy
1. Set Input Gain to minimum → Should show "-inf dB"
2. Set Input Gain to 0.5 normalized (1.0 actual) → Should show "0.0 dB"
3. Set Input Gain to maximum → Should show "+6.0 dB"

### Test 2: Mix Display Accuracy
1. Set Mix to minimum → Should show "0%"
2. Set Mix to center → Should show "50%"
3. Set Mix to maximum → Should show "100%"

### Test 3: Bank Switching
1. VARIANT switch DOWN → All displays show "A" with cyan border
2. VARIANT switch UP → All displays show "B" with orange border
3. Parameter values should reflect bank-specific settings

### Test 4: Mode Switching
1. MODE switch to PRESET → Shows "Input", "Mix", "Output"
2. MODE switch to MIX → Shows "Input", "Mix", "Output" (macros coming soon)
3. MODE switch to AI → Shows "Complexity", "Refine", "Evolve"

### Test 5: Button Feedback
1. Press encoder button → Circle turns red
2. Release encoder button → Circle returns to gray
3. No delay or lag in visual feedback

## Future Enhancement Ideas

### Visual Enhancements
- **Ring visualization:** Show encoder position as circular arc
- **Bar graph:** Vertical bar showing current value in range
- **Peak indicators:** Hold peak value for gain parameters
- **Animation:** Smooth transitions between values

### Information Enhancements
- **Value delta:** Show "+2 dB" when changing
- **Default indicator:** Highlight when at default value
- **Range indicators:** Show min/max values
- **Tooltip:** Extended info on long press

### Interaction Enhancements
- **Touch feedback:** Haptic response on encoder turn
- **Double-tap:** Reset to default value
- **Long press:** Open detailed parameter editor
- **Gesture support:** Swipe to change parameters

## Conclusion

The improved EncoderDisplay provides a professional, user-friendly interface that shows meaningful parameter information in real-time. Users can now see exactly what they're controlling and monitor values in proper audio units (dB, %) while maintaining awareness of bank state through visual indicators.
