# PitchShifter Musical Interval Parameter Values

## Current Mapping
- Parameter range: 0.000 to 1.000
- 0.500 = Unison (no pitch change)
- Range: -24 to +24 semitones (4 octaves total)

## Exact Parameter Values for Musical Intervals

### DOWN (below 0.500)

| Interval | Semitones | Parameter Value |
|----------|-----------|-----------------|
| Octave down | -12 | 0.250 |
| Major 7th down | -11 | 0.271 |
| Minor 7th down | -10 | 0.292 |
| Major 6th down | -9 | 0.313 |
| Minor 6th down | -8 | 0.333 |
| Perfect 5th down | -7 | 0.354 |
| Tritone down | -6 | 0.375 |
| Perfect 4th down | -5 | 0.396 |
| Major 3rd down | -4 | 0.417 |
| Minor 3rd down | -3 | 0.438 |
| Major 2nd down | -2 | 0.458 |
| Minor 2nd down | -1 | 0.479 |
| **UNISON** | **0** | **0.500** |

### UP (above 0.500)

| Interval | Semitones | Parameter Value |
|----------|-----------|-----------------|
| Minor 2nd up | +1 | 0.521 |
| Major 2nd up | +2 | 0.542 |
| Minor 3rd up | +3 | 0.563 |
| Major 3rd up | +4 | 0.583 |
| Perfect 4th up | +5 | 0.604 |
| Tritone up | +6 | 0.625 |
| Perfect 5th up | +7 | 0.646 |
| Minor 6th up | +8 | 0.667 |
| Major 6th up | +9 | 0.688 |
| Minor 7th up | +10 | 0.708 |
| Major 7th up | +11 | 0.729 |
| Octave up | +12 | 0.750 |

## Formula
```
Parameter Value = 0.5 + (semitones / 48)
```

Where semitones ranges from -24 to +24

## Common Intervals (Three Decimal Places)

| Interval | Parameter |
|----------|-----------|
| Octave down | 0.250 |
| Perfect 5th down | 0.354 |
| Perfect 4th down | 0.396 |
| Major 3rd down | 0.417 |
| Minor 3rd down | 0.438 |
| **Unison** | **0.500** |
| Minor 3rd up | 0.563 |
| Major 3rd up | 0.583 |
| Perfect 4th up | 0.604 |
| Perfect 5th up | 0.646 |
| Octave up | 0.750 |