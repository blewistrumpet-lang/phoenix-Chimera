# UI Redesign Plan: From "Windows 98" to Modern Professional

## Current Problems:
1. **Oversized Controls**: Bypass/Solo buttons and Mix knob taking 117px (25+32+60) of vertical space
2. **Poor Space Utilization**: Only ~200px left for actual parameters in a 480px slot
3. **Dated Aesthetic**: Beveled edges, heavy gradients, oversized elements
4. **Bad Hierarchy**: Mix control same size as main parameters

## New Modern Design:

### Layout Changes:
```
BEFORE (480px height):
- Slot Label: 25px
- Engine Selector + Bypass: 32px  
- Mix + Solo: 60px
- Parameters: ~200px (cramped!)

AFTER (480px height):
- Compact Header: 24px (slot + engine selector inline)
- Mini Controls Bar: 20px (bypass, solo, mix as small icons/mini knob)
- Parameters: 430px (DOUBLE the space!)
```

### Visual Design:
1. **Flat Modern Style**:
   - Remove ALL bevels and 3D effects
   - Flat backgrounds with subtle shadows only
   - Single pixel borders, not thick frames

2. **Compact Controls**:
   - Bypass/Solo: 16x16px icon buttons (not 60px monsters!)
   - Mix: 24x24px mini knob in corner
   - Engine selector: Sleek dropdown integrated in header

3. **Modern Color Palette**:
   ```
   Background: #0A0B0D (near black)
   Panels: #12141A (subtle lift)
   Borders: #1E2028 (barely visible)
   Text: #A0A6B8 (readable gray)
   Accent: #00D4FF (electric blue, not cyan)
   Active: #FF006E (magenta for bypass/solo)
   ```

4. **Parameter Knobs**:
   - 36x36px (not 45x45)
   - Thin line indicator (not thick glowing bar)
   - Value text integrated, not separate label
   - Modern sans-serif font (Inter, not system)

### Implementation Priority:
1. Fix resized() to use compact header
2. Make bypass/solo tiny icon buttons
3. Move mix to corner as mini control
4. Maximize parameter space
5. Remove all 3D effects
6. Update color scheme