# PATENT DRAWINGS - DETAILED STEP-BY-STEP INSTRUCTIONS
## Complete Guide to Creating All 11 Figures for Chimera Phoenix Patent

**Tool:** PowerPoint (Mac or Windows) or Google Slides (free)
**Time:** 3-4 hours total (20-30 min per figure)
**Quality:** Simple block diagrams are PERFECT for provisional patents

---

## SETUP - DO THIS FIRST (5 minutes)

### PowerPoint Setup:

1. Open PowerPoint
2. **New Blank Presentation**
3. **Slide 1 = Figure 1, Slide 2 = Figure 2**, etc. (you'll create 11 slides)
4. **Page Setup:**
   - Click: Design → Slide Size → Custom Slide Size
   - Width: 11 inches
   - Height: 8.5 inches
   - Orientation: Landscape
   - Click OK

5. **Remove text boxes** from slide layout:
   - Click on "Click to add title" box → Delete
   - Click on "Click to add text" box → Delete
   - You now have blank canvas

**You're ready to draw!**

---

## FIGURE 1: OVERALL SYSTEM ARCHITECTURE

**Time:** 30 minutes
**What it shows:** Complete system from voice input to audio output

### Step-by-Step:

**Step 1: Create Component Boxes (6 boxes)**

1. **Insert → Shapes → Rectangle**
2. Draw rectangle in upper-left area
3. **Right-click rectangle → Format Shape:**
   - Fill: White or light gray
   - Border: Black, 2pt weight
   - Size: Width 2", Height 1"

4. **Click inside rectangle → Type:**
   ```
   Voice Input
   (USB Mic)
   ```

5. **Repeat to create 5 more boxes:**
   - Box 2: "Whisper API (Transcribe)"
   - Box 3: "Trinity AI Pipeline"
   - Box 4: "57 DSP Engines (6 Slots)"
   - Box 5: "Hardware Controls (Encoders)"
   - Box 6: "Audio Output (HiFiBerry)"

**Layout positions:**
```
Row 1 (top):
  [Voice Input]  [Whisper API]  [Trinity AI]

Row 2 (bottom):
  [Audio Output] [Hardware Controls] [DSP Engines]
```

**Step 2: Add Arrows**

1. **Insert → Shapes → Arrow (Block Arrow or Line Arrow)**
2. Draw arrow from "Voice Input" pointing to "Whisper API"
3. Repeat for all connections:
   - Voice Input → Whisper API
   - Whisper API → Trinity AI
   - Trinity AI → DSP Engines
   - DSP Engines → Hardware Controls
   - Hardware Controls → Audio Output

**Make arrows bold:**
- Select arrow → Format → Line Weight → 3pt
- Color: Black

**Step 3: Add Figure Label**

1. **Insert → Text Box**
2. Click at bottom center of slide
3. Type: `Figure 1: Overall System Architecture`
4. **Format:**
   - Font: Arial or Helvetica, 14pt, Bold
   - Align: Center

**Step 4: Export**

1. **File → Export**
2. Select format: **PDF**
3. Options: Export slide 1 only
4. Save as: `Figure_1_System_Architecture.pdf`

**Done ✅**

---

## FIGURE 2: THREE-TIER ROUTING FLOWCHART

**Time:** 25 minutes
**What it shows:** How requests are classified into Tier 1, 2, or 3

### Step-by-Step:

**Step 1: Create Starting Box**

1. New slide (Slide 2)
2. Insert → Shapes → Rectangle
3. Position at top center
4. Type inside: `User Natural Language Prompt`
5. Format: Fill = light blue, Border = black 2pt

**Step 2: Create Decision Diamond**

1. Insert → Shapes → **Diamond** (in Basic Shapes)
2. Position below starting box
3. Type inside: `Analyze Complexity`
4. Format: Fill = yellow, Border = black 2pt

**Step 3: Create Three Processing Paths**

Create 3 boxes in a row:

1. Insert → Shapes → Rectangle (3 times)
2. Position in row below decision diamond
3. **Box 1 (left):**
   ```
   TIER 1
   Exact Match
   (GPT-4o-mini)
   3-5 seconds
   ```
   Format: Fill = light green

4. **Box 2 (center):**
   ```
   TIER 2
   Guided Artistic
   (GPT-4o)
   8-12 seconds
   ```
   Format: Fill = light orange

5. **Box 3 (right):**
   ```
   TIER 3
   Full Artistic
   (GPT-4o)
   10-15 seconds
   ```
   Format: Fill = light red

**Step 4: Create Output Box**

1. Insert → Shapes → Rectangle
2. Position at bottom center
3. Type: `Generated Preset`
4. Format: Fill = light purple

**Step 5: Add Arrows and Labels**

1. Arrow from "User Prompt" to "Analyze Complexity"

2. Three arrows from "Analyze Complexity" to each Tier box:
   - **Left arrow:** Add text label "Simple?" next to arrow
   - **Center arrow:** Add text label "Artistic?" next to arrow
   - **Right arrow:** Add text label "Complex?" next to arrow

3. Arrows from each Tier box to "Generated Preset"

**To add text labels near arrows:**
- Insert → Text Box
- Type label text
- Position near arrow

**Step 6: Add Figure Label**

Bottom center: `Figure 2: Three-Tier Intelligent Routing System`

**Step 7: Export**

File → Export → PDF → Slide 2 only → `Figure_2_Three_Tier_Routing.pdf`

**Done ✅**

---

## FIGURE 3: TRINITY AI PIPELINE

**Time:** 25 minutes
**What it shows:** Three-stage processing (Visionary → Calculator → Alchemist)

### Step-by-Step:

**Step 1: Create Input/Output Boxes**

1. New slide (Slide 3)
2. Top box: `Natural Language Input ("warm reverb")`
3. Bottom box: `Final Preset JSON`

**Step 2: Create Three Agent Boxes**

Create 3 boxes vertically between input and output:

1. **Box 1 (VISIONARY):**
   ```
   VISIONARY
   Engine Selection
   (0% → 33% progress)
   ```
   Format: Fill = light blue

2. **Box 2 (CALCULATOR):**
   ```
   CALCULATOR
   Parameter Generation
   (33% → 66% progress)
   ```
   Format: Fill = light green

3. **Box 3 (ALCHEMIST):**
   ```
   ALCHEMIST
   Validation & Optimization
   (66% → 100% progress)
   ```
   Format: Fill = light yellow

**Step 3: Add Arrows**

Vertical flow:
```
[Natural Language Input]
         ↓
    [VISIONARY]
         ↓
    [CALCULATOR]
         ↓
    [ALCHEMIST]
         ↓
  [Final Preset JSON]
```

**Step 4: Add Side Annotations (Optional but Helpful)**

Add text boxes on the right side showing what each stage does:

- Next to VISIONARY: "Selects: Plate Reverb, Tube Preamp"
- Next to CALCULATOR: "Sets: reverb_size=0.7, tube_drive=0.4"
- Next to ALCHEMIST: "Validates: All params 0.0-1.0, Optimizes: Chain order"

**Step 5: Figure Label**

Bottom: `Figure 3: Trinity AI Pipeline Architecture`

**Step 6: Export**

File → Export → PDF → Slide 3 → `Figure_3_Trinity_Pipeline.pdf`

**Done ✅**

---

## FIGURE 4: HARDWARE CONTROL LAYOUT

**Time:** 30 minutes
**What it shows:** Physical layout of encoders, switches, and display

### Step-by-Step:

**Step 1: Create Device Outline**

1. New slide (Slide 4)
2. Insert → Shapes → Rectangle (rounded corners)
3. Make it large (represents device enclosure)
4. Format: Fill = light gray, Border = black 3pt
5. Size: Width 8", Height 5"

**Step 2: Add Encoders (3 circles with labels)**

1. **Insert → Shapes → Oval (Circle)**
2. Hold Shift while dragging to make perfect circle
3. Size: 1" diameter
4. Format: Fill = white, Border = black 2pt

5. **Position at top of device:**
   ```
   ┌─────────────────────────────────┐
   │                                  │
   │    ⊙         ⊙         ⊙        │  ← Encoders
   │   E1        E2        E3         │
   ```

6. **Add labels below each:**
   - Insert → Text Box below each circle
   - E1: "Preset/Warmth"
   - E2: "Mix/Size"
   - E3: "Output/Punch"

**Step 3: Add Display (Center)**

1. Insert → Shapes → Rectangle
2. Position in center of device
3. Size: Width 3", Height 2"
4. Format: Fill = black, Border = white 2pt
5. Add text inside: `OLED Display (480×320)`
6. Text color: White (so it shows on black background)

**Step 4: Add Switches (3 at bottom)**

1. Insert → Shapes → Rounded Rectangle (3 times)
2. Size: Width 0.8", Height 0.5"
3. Position at bottom:
   ```
   │                                  │
   │         [Display]                │
   │                                  │
   │   [SW1]    [SW2]    [SW3]       │
   └─────────────────────────────────┘
   ```

4. **Add labels:**
   - SW1: "Mode"
   - SW2: "Bank A/B"
   - SW3: "Bypass"

**Step 5: Add GPIO Pin Annotations (Optional)**

Small text boxes showing pin numbers:
- E1: "GPIO 5,6,26"
- E2: "GPIO 23,24,25"
- E3: "GPIO 17,27,22"

**Step 6: Figure Label**

Bottom: `Figure 4: Hardware Control Interface Layout`

**Step 7: Export**

File → Export → PDF → Slide 4 → `Figure_4_Hardware_Layout.pdf`

**Done ✅**

---

## FIGURE 5: AUDIO SIGNAL FLOW

**Time:** 20 minutes
**What it shows:** Audio processing chain through 6 slots

### Step-by-Step:

**Step 1: Create Signal Flow**

New slide (Slide 5)

Create boxes in horizontal flow:

```
[Input] → [Gain] → [Slot 1] → [Slot 2] → [Slot 3] → [Slot 4] → [Slot 5] → [Slot 6] → [Mix] → [Output Gain] → [Output]
```

**Each box:**
- Rectangle shape
- Width: 1", Height: 0.8"
- Fill: light blue
- Border: black 1pt

**Arrows between boxes:**
- Insert → Shapes → Arrow
- Connect each box

**Step 2: Add Labels**

Above the chain, add text:
- "Input Gain: 0 to +12dB"
- "6-Slot Serial Chain (Any of 57 Engines)"
- "Global Mix: Wet/Dry 0-100%"
- "Output: -∞ to +6dB"

**Step 3: Show Example**

Below the chain, show an example:
```
Example Chain:
Slot 1: Tube Preamp → Slot 2: Parametric EQ → Slot 3: Plate Reverb
```

**Step 4: Figure Label**

Bottom: `Figure 5: Audio Signal Flow and Processing Chain`

**Step 5: Export**

File → Export → PDF → Slide 5 → `Figure_5_Signal_Flow.pdf`

**Done ✅**

---

## FIGURE 6: A/B BANK COMPARISON SYSTEM

**Time:** 25 minutes
**What it shows:** How A/B switching works with parameter storage

### Step-by-Step:

**Step 1: Create Two Parameter Storage Boxes**

New slide (Slide 6)

**Left side - Bank A:**
```
┌──────────────────┐
│    BANK A        │
│   Parameters:    │
│  input_gain=1.0  │
│  mix_wetdry=0.4  │
│  output_level=0.8│
│  warmth=0.7      │
└──────────────────┘
```

**Right side - Bank B:**
```
┌──────────────────┐
│    BANK B        │
│   Parameters:    │
│  input_gain=1.0  │
│  mix_wetdry=0.8  │
│  output_level=0.6│
│  warmth=0.3      │
└──────────────────┘
```

**Step 2: Create Center Switch Box**

```
        ┌─────────┐
        │   SW2   │
        │ Switch  │
        │  A↔B    │
        └─────────┘
```

Position between Bank A and Bank B

**Step 3: Create Active Parameters Box**

Below the switch:
```
┌──────────────────────────────────┐
│     APVTS (Active Parameters)    │
│      Currently: Bank A           │
└──────────────────────────────────┘
```

**Step 4: Add Arrows**

**Bidirectional arrows:**
- Bank A ←→ SW2 Switch
- SW2 Switch ←→ Bank B
- SW2 Switch ↓ APVTS

**Arrow labels:**
- Above arrows: "Capture" and "Apply"
- Show the flow: Capture current → Switch → Apply new

**Step 5: Add Workflow Description**

Bottom text box:
```
Workflow:
1. Capture current bank parameters from APVTS
2. Switch active bank pointer (A→B or B→A)
3. Apply new bank parameters to APVTS
```

**Step 6: Figure Label**

Bottom: `Figure 6: A/B Bank Comparison System Architecture`

**Step 7: Export**

File → Export → PDF → Slide 6 → `Figure_6_AB_Bank_System.pdf`

**Done ✅**

---

## FIGURE 7: INTELLIGENT FALLBACK FLOWCHART

**Time:** 30 minutes
**What it shows:** Decision tree for cloud AI vs. local fallback

### Step-by-Step:

**Step 1: Create Flowchart**

New slide (Slide 7)

**Use Diamond shapes for decisions, Rectangles for actions:**

```
         [Voice Input]
              ↓
         ◇ Cloud Available? ◇
         YES ↓          ↓ NO
      [Trinity AI]      ↓
              ↓         ◇ Keyword Match? ◇
              ↓         YES ↓         ↓ NO
              ↓    [Use Matched    [Character
              ↓     Engine]          Detection]
              ↓         ↓                ↓
              ↓         ↓         [Select Template]
              ↓         ↓                ↓
         [Generated Preset]
```

**Step 2: Create Shapes**

**Top (input):**
- Rectangle: "Voice Input"

**First Decision:**
- Diamond: "Cloud Available?"
- Two branches: "YES" (left), "NO" (right)

**Left Path (Cloud):**
- Rectangle: "Trinity AI (2-4 seconds)"
- Arrow down to: "Generated Preset"

**Right Path (Fallback):**
- Diamond: "Keyword Match?" (e.g., "spring reverb")
- Branch YES: Rectangle "Use Matched Engine (<20ms)"
- Branch NO: Rectangle "Character Detection" → "Select Template"
- Both paths converge to: "Generated Preset"

**Step 3: Add Timing Labels**

Small text near paths:
- Cloud path: "2-4 seconds, AI cost $0.001-0.015"
- Fallback path: "<20ms, $0 cost, 90% accuracy"

**Step 4: Color Code**

- Cloud path boxes: Light blue (online processing)
- Fallback path boxes: Light green (offline processing)
- Diamonds: Yellow (decisions)

**Step 5: Figure Label**

Bottom: `Figure 7: Intelligent Fallback System Flowchart`

**Step 6: Export**

File → Export → PDF → Slide 7 → `Figure_7_Fallback_System.pdf`

**Done ✅**

---

## FIGURE 8: EMBEDDED PLATFORM BLOCK DIAGRAM

**Time:** 30 minutes
**What it shows:** Hardware components (Raspberry Pi + HiFiBerry + GPIO)

### Step-by-Step:

**Step 1: Create Main Pi Box**

New slide (Slide 8)

Large rectangle representing Raspberry Pi:
```
┌────────────────────────────────────────┐
│         Raspberry Pi 5                  │
│                                         │
│  ┌─────────┐      ┌──────────────┐    │
│  │   CPU   │─────▶│ GPIO Control │────┼──▶ Encoders
│  │ Cortex  │      │  (libgpiod)  │    │    Switches
│  │  A76    │      └──────────────┘    │
│  └────┬────┘                           │
│       │                                 │
│  ┌────▼──────────────────────────┐    │
│  │   JACK Audio Server (48kHz)    │    │
│  └────┬──────────────────────────┘    │
└───────┼────────────────────────────────┘
        │
   ┌────▼─────────┐
   │  HiFiBerry   │───▶ Audio I/O
   │  DAC+ADC Pro │     (XLR/TRS)
   └──────────────┘
```

**Step 2: Create Individual Components**

**Inside the Pi box:**

1. **CPU Box:**
   - Rectangle
   - Text: "CPU (Cortex-A76 Quad-Core 2.4GHz)"
   - Size: 2" × 1"

2. **GPIO Box:**
   - Rectangle
   - Text: "GPIO Control (libgpiod 1000Hz polling)"
   - Size: 2.5" × 1"

3. **JACK Box:**
   - Rectangle
   - Text: "JACK Audio Server (48kHz, 512 buffer)"
   - Size: 4" × 1"

**Outside the Pi box:**

4. **HiFiBerry Box:**
   - Rectangle
   - Text: "HiFiBerry DAC+ADC Pro (24-bit)"
   - Size: 2.5" × 1"

**Step 3: Add Connections**

- CPU → GPIO (arrow labeled "Thread-safe events")
- GPIO → External (arrow labeled "To Encoders/Switches")
- CPU → JACK (arrow)
- JACK → HiFiBerry (arrow labeled "I2S interface")
- HiFiBerry → External (arrow labeled "Audio I/O")

**Step 4: Add Component Details**

Small text annotations:
- Near CPU: "8GB RAM, aarch64"
- Near GPIO: "3 encoders, 3 switches"
- Near HiFiBerry: "THD+N: -93dB"

**Step 5: Figure Label**

Bottom: `Figure 8: Embedded Platform Architecture`

**Step 6: Export**

File → Export → PDF → Slide 8 → `Figure_8_Embedded_Platform.pdf`

**Done ✅**

---

## FIGURE 9: PARAMETER NUDGING ALGORITHM

**Time:** 20 minutes
**What it shows:** How semantic characteristics translate to parameter adjustments

### Step-by-Step:

**Step 1: Create Flow**

New slide (Slide 9)

**Top to bottom flow:**

```
[User Prompt: "warm vintage guitar"]
         ↓
[Extract Characteristics: warmth=0.7]
         ↓
[For Each Selected Engine:]
    ┌────────────────────────────┐
    │ IF engine has tube_drive:  │
    │   tube_drive += 0.1 × 0.7  │
    │                 = +0.07    │
    ├────────────────────────────┤
    │ IF engine has low_freq:    │
    │   low_freq += 0.15 × 0.7   │
    │                 = +0.105   │
    ├────────────────────────────┤
    │ IF engine has high_freq:   │
    │   high_freq -= 0.1 × 0.7   │
    │                 = -0.07    │
    └────────────────────────────┘
         ↓
[Adjusted Parameters Applied to Engine]
```

**Step 2: Create Boxes**

Use rectangles for each step, formatted like code blocks:
- Font: Courier New or Monaco (monospace)
- Fill: Light gray (code block appearance)

**Step 3: Add Example Values**

Show specific calculation:
- warmth value: 0.7
- Result: tube_drive increases by 0.07

**Step 4: Color Code**

- Input: Light blue
- Processing: Light gray (code block)
- Output: Light green

**Step 5: Figure Label**

Bottom: `Figure 9: Character-Based Parameter Nudging Algorithm`

**Step 6: Export**

File → Export → PDF → Slide 9 → `Figure_9_Parameter_Nudging.pdf`

**Done ✅**

---

## FIGURE 10: SIGNAL CHAIN OPTIMIZATION

**Time:** 25 minutes
**What it shows:** How effects are automatically ordered

### Step-by-Step:

**Step 1: Show Unordered Input**

New slide (Slide 10)

**Top - Unordered effects:**
```
┌──────────────────────────────────────┐
│   Unordered Effects (User/AI Gen)    │
├──────────────────────────────────────┤
│  • Reverb (39)                       │
│  • Compressor (2)                    │
│  • EQ (7)                            │
│  • Distortion (15)                   │
└──────────────────────────────────────┘
```

**Step 2: Show Sorting Algorithm**

Middle section:
```
┌──────────────────────────────────────┐
│    ALCHEMIST Optimization            │
├──────────────────────────────────────┤
│  Sort by Category Order:             │
│  1. Dynamics (Compressor)            │
│  2. Filter (EQ)                      │
│  3. Distortion                       │
│  4. Modulation                       │
│  5. Delay                            │
│  6. Reverb                           │
│  7. Spatial                          │
│  8. Utility                          │
└──────────────────────────────────────┘
```

**Step 3: Show Optimized Output**

Bottom - Optimized chain:
```
┌──────────────────────────────────────┐
│      Optimized Signal Chain          │
├──────────────────────────────────────┤
│  Slot 1: Compressor (2)              │
│     ↓                                 │
│  Slot 2: EQ (7)                      │
│     ↓                                 │
│  Slot 3: Distortion (15)             │
│     ↓                                 │
│  Slot 4: Reverb (39)                 │
└──────────────────────────────────────┘
```

**Step 4: Add Arrows**

Large downward arrows:
- Unordered → Optimization Algorithm
- Algorithm → Optimized Chain

**Step 5: Add "Why" Annotation**

Text box on right side:
```
Why This Order?
• Dynamics first: Control levels
• EQ second: Shape tone
• Distortion third: Add harmonics
• Reverb last: Spatial ambience
```

**Step 6: Figure Label**

Bottom: `Figure 10: Automatic Signal Chain Optimization`

**Step 7: Export**

File → Export → PDF → Slide 10 → `Figure_10_Chain_Optimization.pdf`

**Done ✅**

---

## FIGURE 11: VOICE CONTROL WORKFLOW

**Time:** 25 minutes
**What it shows:** Step-by-step user experience of voice control

### Step-by-Step:

**Step 1: Create Timeline Flow**

New slide (Slide 11)

**Vertical timeline showing user actions and system responses:**

```
         TIME
          ↓

    ┌──────────────┐
  1 │ User holds   │  ← USER ACTION
    │ SPEAK button │
    └──────┬───────┘
           │
    ┌──────▼───────┐
  2 │ Recording... │  ← SYSTEM STATE
    │ (0-10 sec)   │
    └──────┬───────┘
           │
    ┌──────▼───────┐
  3 │ User releases│  ← USER ACTION
    │    button    │
    └──────┬───────┘
           │
    ┌──────▼───────┐
  4 │ Send to      │  ← SYSTEM ACTION
    │ Whisper API  │  (1-2 seconds)
    └──────┬───────┘
           │
    ┌──────▼───────┐
  5 │ Trinity AI   │  ← SYSTEM ACTION
    │ Processing   │  (2-4 seconds)
    └──────┬───────┘
           │
    ┌──────▼───────┐
  6 │ Preset       │  ← SYSTEM ACTION
    │   Loaded     │
    └──────┬───────┘
           │
    ┌──────▼───────┐
  7 │ Audio plays  │  ← USER HEARS
    │  with effect │
    └──────────────┘
```

**Step 2: Create Numbered Steps**

Use circles with numbers:
1. Insert → Shapes → Oval
2. Make small circles (0.4" diameter)
3. Number each: 1, 2, 3, 4, 5, 6, 7

**Step 3: Add Timing Annotations**

Right side, show cumulative time:
- Step 1-3: "0-10 seconds (user recording)"
- Step 4: "+1-2 seconds (transcription)"
- Step 5: "+2-4 seconds (AI processing)"
- Step 6-7: "Immediate (preset loads)"
- **Total: 3-16 seconds** (depending on speaking time)

**Step 4: Color Code**

- User actions: Light blue
- System actions: Light green
- Final result: Light yellow

**Step 5: Figure Label**

Bottom: `Figure 11: Voice Control Workflow and User Experience`

**Step 6: Export**

File → Export → PDF → Slide 11 → `Figure_11_Voice_Workflow.pdf`

**Done ✅**

---

## REMAINING FIGURES (Simplified Instructions)

You now have the pattern. Here are quick templates for the remaining figures:

---

## FIGURE 12: ENCODER ACCUMULATOR SYSTEM (BONUS - Optional)

**Time:** 20 minutes
**What it shows:** How 1000Hz polling is converted to 30Hz parameter updates

```
[Hardware ISR (1000 Hz)]
         ↓
   Encoder Turn (+1)
         ↓
[Atomic Accumulator]
   accumulator += 1
   (repeated 33 times in 33ms)
         ↓
[UI Thread Drain (30 Hz)]
   delta = accumulator.exchange(0)
   // delta = 33 detents
         ↓
◇ Parameter Type? ◇
DISCRETE ↓      ↓ CONTINUOUS
    ±1 step    Apply full delta
         ↓           ↓
   [Preset steps] [Mix adjusts]
   cleanly 1→2    smoothly

Figure 12: Encoder Accumulator Rate-Matching System
```

---

## COMBINE ALL FIGURES INTO ONE PDF

### Method 1: PowerPoint Export

1. **File → Export**
2. Select: **"Create PDF/XPS Document"**
3. Options: **"All Slides"**
4. Save as: `Chimera_Phoenix_Drawings.pdf`

**This creates one PDF with all 11 figures.**

### Method 2: Mac Preview (Combine Separate PDFs)

1. Open `Figure_1.pdf` in Preview
2. **View → Thumbnails** (sidebar appears)
3. **Drag** `Figure_2.pdf` into thumbnail sidebar
4. **Drag** `Figure_3.pdf` into thumbnail sidebar
5. Repeat for all 11 figures
6. **File → Export as PDF**
7. Save as: `Chimera_Phoenix_Drawings.pdf`

### Method 3: Online PDF Merger

1. Go to: https://www.ilovepdf.com/merge_pdf
2. Upload all 11 PDFs
3. Click "Merge PDF"
4. Download: `Chimera_Phoenix_Drawings.pdf`

---

## QUALITY CHECKLIST

Before exporting final PDFs, verify:

**For Each Figure:**
- [ ] Clear and legible (text readable at 100% zoom)
- [ ] All components labeled
- [ ] Arrows show direction of data/signal flow
- [ ] Figure number and title at bottom
- [ ] Black and white or simple colors (avoid complex color schemes)
- [ ] No typos in labels

**For Combined PDF:**
- [ ] All 11 figures included
- [ ] In correct order (Figure 1 through Figure 11)
- [ ] Each figure on separate page
- [ ] File size under 25 MB (should be <5 MB for simple diagrams)
- [ ] Filename: `Chimera_Phoenix_Drawings.pdf` (lowercase extension)
- [ ] No password protection

---

## ALTERNATIVE - HAND-DRAWN FIGURES (Even Simpler)

**If PowerPoint feels too complex:**

1. **Draw on paper** with pen/marker and ruler
2. Label everything clearly
3. Number each figure
4. **Take photo** with your phone (good lighting, straight angle)
5. **Convert to PDF:**
   - Mac: Open in Preview → File → Export as PDF
   - iPhone: Save photo → Share → Print → Pinch to zoom → Share → Save as PDF
6. **This is 100% acceptable for provisional patents!**

**Time:** Potentially faster if you're comfortable drawing

---

## FIGURE DIFFICULTY RANKING (Easiest First)

If short on time, create in this order:

**EASIEST (Do First):**
1. **Figure 5:** Signal Flow (boxes in a row with arrows) - 15 min
2. **Figure 11:** Voice Workflow (vertical timeline) - 20 min
3. **Figure 3:** Trinity Pipeline (3 boxes with arrows) - 20 min

**MEDIUM:**
4. **Figure 1:** System Architecture (6 boxes with arrows) - 25 min
5. **Figure 9:** Parameter Nudging (text/code blocks) - 20 min
6. **Figure 10:** Chain Optimization (before/after comparison) - 20 min

**MOST COMPLEX:**
7. **Figure 2:** Three-Tier Routing (flowchart with diamond decisions) - 25 min
8. **Figure 7:** Fallback Flowchart (decision tree) - 30 min
9. **Figure 4:** Hardware Layout (device illustration) - 30 min
10. **Figure 6:** A/B Bank System (data flow diagram) - 25 min
11. **Figure 8:** Embedded Platform (nested boxes and connections) - 30 min

**Strategy:** Do easiest 3-5 figures first. If you run out of time, having SOME drawings is better than perfect drawings for all 11.

---

## TIME-SAVING TIPS

### Tip 1: Use Templates

**PowerPoint:**
- Start with Figure 1
- Copy entire slide
- Paste for Figure 2
- Modify (saves time recreating boxes/formatting)

### Tip 2: Reuse Elements

- Create one well-formatted box
- Copy/paste for all other boxes
- Just change text

### Tip 3: Don't Overthink Aesthetics

**Acceptable for provisional:**
- ✅ Simple rectangles and arrows
- ✅ Basic fonts (Arial, Helvetica)
- ✅ Black and white only
- ✅ Hand-drawn sketches
- ✅ Screenshots with annotations

**NOT required:**
- ❌ Professional CAD drawings
- ❌ Consistent color scheme
- ❌ Perfect alignment
- ❌ Engineering precision

### Tip 4: Focus on Clarity, Not Beauty

**Good enough:**
- Can you understand what each component is?
- Can you follow the arrows/flow?
- Are labels readable?

**If yes to all three: Your drawing is fine.**

---

## FINAL CHECKLIST - ALL 11 FIGURES

- [ ] Figure 1: Overall System Architecture
- [ ] Figure 2: Three-Tier Intelligent Routing
- [ ] Figure 3: Trinity AI Pipeline
- [ ] Figure 4: Hardware Control Interface Layout
- [ ] Figure 5: Audio Signal Flow
- [ ] Figure 6: A/B Bank Comparison System
- [ ] Figure 7: Intelligent Fallback Flowchart
- [ ] Figure 8: Embedded Platform Architecture
- [ ] Figure 9: Parameter Nudging Algorithm
- [ ] Figure 10: Signal Chain Optimization
- [ ] Figure 11: Voice Control Workflow

**All combined into:** `Chimera_Phoenix_Drawings.pdf`

---

## YOU'RE READY TO FILE WHEN YOU HAVE:

1. ✅ `Chimera_Phoenix_Specification.pdf` (with 3 added paragraphs)
2. ✅ `Chimera_Phoenix_Drawings.pdf` (all 11 figures)
3. ✅ `SB16_Cover_Sheet.pdf` (filled out)
4. ✅ USPTO account created
5. ✅ ID.me verified
6. ✅ Credit card ready ($65-130)

**Then follow Friday afternoon steps** from my earlier walkthrough:
- Log into Patent Center
- New Application → Utility Provisional
- Upload 3 files
- Pay fee
- Submit
- **Patent Pending ✅**

---

## ESTIMATED TIMELINE

**Monday:** 1 hour (USPTO account)
**Tuesday:** 30 min (update specification)
**Wednesday:** 2 hours (create figures 1-6)
**Thursday:** 2 hours (create figures 7-11)
**Friday AM:** 15 min (fill out SB/16)
**Friday PM:** 30 min (file online)

**Total:** 6 hours over 5 days

**You'll be Patent Pending by Friday 5pm.**

---

**Need help with a specific figure? Tell me which one and I can give you even more detailed step-by-step PowerPoint instructions (which buttons to click, exact coordinates, etc.).**