# PARAMETER INTERACTION TESTING REPORT

**Chimera Phoenix v3.0 - Deep Parameter Interaction Analysis**

*Generated: Sat Oct 11 13:32:49 2025
*

---

## Executive Summary

- **Total Engines Tested:** 56
- **Engines with Issues:** 11
- **Total Danger Zones Identified:** 190
- **Total Sweet Spots Identified:** 3342

---

## Detailed Reports by Engine Category

### Dynamics & Compression

#### [1] Opto Compressor

**Parameters:** 7

**Known Parameter Interactions:**

- **P0 + P1** (coupled): Attack & Release: Fast attack + fast release can cause pumping
- **P2 + P3** (synergistic): Threshold & Ratio: Higher ratio needs higher threshold for transparency
- **P0 + P2** (conflicting): Attack & Threshold: Very fast attack with low threshold causes distortion

**Test Results:**
- Total Tests: 105
- Passed: 105 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.668591, RMS: 0.305005)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.712805, RMS: 0.367927)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.638619, RMS: 0.283258)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.601425, RMS: 0.342519)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.614037, RMS: 0.332927)

---

#### [2] VCA Compressor

**Parameters:** 7

**Known Parameter Interactions:**

- **P0 + P1** (coupled): Attack & Release: Fast attack + fast release can cause pumping
- **P2 + P3** (synergistic): Threshold & Ratio: Higher ratio needs higher threshold for transparency
- **P0 + P2** (conflicting): Attack & Threshold: Very fast attack with low threshold causes distortion

**Test Results:**
- Total Tests: 105
- Passed: 105 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 1.110081, RMS: 0.443495)
- `P0_P1_Both_Low`: Good output levels (Peak: 1.087176, RMS: 0.313117)
- `P0_P1_Both_Max`: Good output levels (Peak: 1.118182, RMS: 0.734953)
- `P0_P1_Both_Mid`: Good output levels (Peak: 1.087176, RMS: 0.342340)
- `P0_P1_Both_Min`: Good output levels (Peak: 1.079591, RMS: 0.548734)

---

#### [3] Transient Shaper

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 62 (88%)
- Failed: 8
- Unstable: 8

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_Low`: Good output levels (Peak: 0.342257, RMS: 0.170509)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.700000, RMS: 0.491801)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.118709, RMS: 0.036221)
- `P0_P2_Both_High`: Good output levels (Peak: 1.346583, RMS: 0.536707)
- `P0_P2_Both_Low`: Good output levels (Peak: 0.679619, RMS: 0.469465)

**⚠️  Danger Zones (Avoid These Combinations):**

- `P0_P1_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P1_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P2_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P3_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P3_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 

---

#### [4] Noise Gate

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.773367, RMS: 0.441982)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.773379, RMS: 0.454999)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.696797, RMS: 0.414438)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.773370, RMS: 0.448895)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.773420, RMS: 0.470575)

---

#### [5] Mastering Limiter

**Parameters:** 6

**Test Results:**
- Total Tests: 105
- Passed: 105 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.720387, RMS: 0.496263)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.310910, RMS: 0.125673)
- `P0_P1_Both_Max`: Good output levels (Peak: 1.396676, RMS: 0.974806)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.449518, RMS: 0.249404)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.219173, RMS: 0.045470)

---

#### [6] Dynamic EQ

**Parameters:** 7

**Test Results:**
- Total Tests: 105
- Passed: 92 (87%)
- Failed: 13
- Unstable: 13

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_Low`: Good output levels (Peak: 1.434257, RMS: 0.762294)
- `P0_P1_Both_Min`: Good output levels (Peak: 1.540396, RMS: 0.802843)
- `P0_P1_P1_Max_P2_Min`: Good output levels (Peak: 1.336835, RMS: 0.570485)
- `P0_P1_P1_Min_P2_Max`: Good output levels (Peak: 1.395150, RMS: 0.926170)
- `P0_P2_Both_Low`: Good output levels (Peak: 1.527911, RMS: 0.780105)

**⚠️  Danger Zones (Avoid These Combinations):**

- `P0_P2_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P2_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P3_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P5_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P5_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P3_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P3_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P4_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P2_P5_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P5_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 

---

### Filters & EQ

#### [7] Parametric EQ

**Parameters:** 7

**Test Results:**
- Total Tests: 105
- Passed: 101 (96%)
- Failed: 4
- Unstable: 4

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.774685, RMS: 0.474077)
- `P0_P1_Both_Low`: Good output levels (Peak: 1.532304, RMS: 0.498067)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.781622, RMS: 0.450359)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.746634, RMS: 0.490743)
- `P0_P1_P1_Min_P2_Max`: Good output levels (Peak: 0.869444, RMS: 0.482502)

**⚠️  Danger Zones (Avoid These Combinations):**

- `P1_P4_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P4_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 

---

#### [8] Vintage Console EQ

**Parameters:** 6

**Test Results:**
- Total Tests: 105
- Passed: 105 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.889859, RMS: 0.433291)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.936710, RMS: 0.599266)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.755886, RMS: 0.489703)
- `P0_P1_Both_Min`: Good output levels (Peak: 1.279782, RMS: 0.800206)
- `P0_P1_P1_Max_P2_Min`: Good output levels (Peak: 0.848738, RMS: 0.485711)

---

#### [9] Ladder Filter

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Frequency & Q/Resonance: High Q at low freq can cause booming
- **P0 + P1** (coupled): Frequency & Q: Self-oscillation at max Q + any frequency

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.342885, RMS: 0.139021)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.351152, RMS: 0.180206)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.228523, RMS: 0.105469)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.364932, RMS: 0.174387)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.126418, RMS: 0.036193)

---

#### [10] State Variable Filter

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Frequency & Q/Resonance: High Q at low freq can cause booming
- **P0 + P1** (coupled): Frequency & Q: Self-oscillation at max Q + any frequency

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P2_P3_Both_High`: Good output levels (Peak: 1.710931, RMS: 1.175357)
- `P2_P3_Both_Low`: Good output levels (Peak: 1.706137, RMS: 1.172188)
- `P2_P3_Both_Mid`: Good output levels (Peak: 1.705930, RMS: 1.171939)
- `P2_P3_P1_Max_P2_Min`: Good output levels (Peak: 1.706128, RMS: 1.167765)
- `P2_P4_Both_High`: Good output levels (Peak: 1.718546, RMS: 1.184393)

---

#### [11] Formant Filter

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Frequency & Q/Resonance: High Q at low freq can cause booming
- **P0 + P1** (coupled): Frequency & Q: Self-oscillation at max Q + any frequency

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.988635, RMS: 0.535601)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.985218, RMS: 0.508463)
- `P0_P1_Both_Max`: Good output levels (Peak: 1.010378, RMS: 0.489253)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.990022, RMS: 0.529834)
- `P0_P1_Both_Min`: Good output levels (Peak: 1.005332, RMS: 0.631105)

---

#### [12] Envelope Filter

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Frequency & Q/Resonance: High Q at low freq can cause booming
- **P0 + P1** (coupled): Frequency & Q: Self-oscillation at max Q + any frequency

**Test Results:**
- Total Tests: 70
- Passed: 59 (84%)
- Failed: 11
- Unstable: 11

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.905612, RMS: 0.493650)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.829120, RMS: 0.493411)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.900896, RMS: 0.494383)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.851762, RMS: 0.493700)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.875409, RMS: 0.493548)

**⚠️  Danger Zones (Avoid These Combinations):**

- `P0_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_Min`: Signal growing beyond control. Excessive output level. 

---

#### [13] Comb Resonator

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Frequency & Q/Resonance: High Q at low freq can cause booming
- **P0 + P1** (coupled): Frequency & Q: Self-oscillation at max Q + any frequency

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.741168, RMS: 0.494661)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.646264, RMS: 0.424748)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.876454, RMS: 0.595968)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.668087, RMS: 0.440776)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.630232, RMS: 0.428380)

---

#### [14] Vocal Formant Filter

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Frequency & Q/Resonance: High Q at low freq can cause booming
- **P0 + P1** (coupled): Frequency & Q: Self-oscillation at max Q + any frequency

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P3_Both_High`: Good output levels (Peak: 0.134702, RMS: 0.061458)
- `P0_P3_Both_Low`: Good output levels (Peak: 0.120914, RMS: 0.055520)
- `P0_P3_Both_Max`: Good output levels (Peak: 0.104578, RMS: 0.045085)
- `P0_P3_Both_Mid`: Good output levels (Peak: 0.129749, RMS: 0.058599)
- `P0_P3_Both_Min`: Good output levels (Peak: 0.102434, RMS: 0.042401)

---

### Distortion & Saturation

#### [15] Vintage Tube

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Drive & Tone: High drive needs tone control to tame harshness
- **P0 + P2** (coupled): Drive & Output: Max drive requires output reduction to prevent clipping

**Test Results:**
- Total Tests: 70
- Passed: 69 (98%)
- Failed: 1
- Unstable: 1

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.700000, RMS: 0.491801)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.438739, RMS: 0.032402)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.700000, RMS: 0.491801)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.438739, RMS: 0.032402)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.439025, RMS: 0.032403)

**⚠️  Danger Zones (Avoid These Combinations):**

- `P2_P3_Both_Max`: Signal growing beyond control. Excessive output level. 

---

#### [16] Wave Folder

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Drive & Tone: High drive needs tone control to tame harshness
- **P0 + P2** (coupled): Drive & Output: Max drive requires output reduction to prevent clipping

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.248419, RMS: 0.200533)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.248419, RMS: 0.200533)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.248419, RMS: 0.200533)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.248419, RMS: 0.200533)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.248419, RMS: 0.200519)

---

#### [17] Harmonic Exciter

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Drive & Tone: High drive needs tone control to tame harshness
- **P0 + P2** (coupled): Drive & Output: Max drive requires output reduction to prevent clipping

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.673544, RMS: 0.436808)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.672544, RMS: 0.436384)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.673919, RMS: 0.437058)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.673115, RMS: 0.436609)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.672458, RMS: 0.435992)

---

#### [18] Bit Crusher

**Parameters:** 4

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Drive & Tone: High drive needs tone control to tame harshness
- **P0 + P2** (coupled): Drive & Output: Max drive requires output reduction to prevent clipping

**Test Results:**
- Total Tests: 42
- Passed: 42 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.693750, RMS: 0.487797)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.699971, RMS: 0.491699)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.600000, RMS: 0.443343)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.699609, RMS: 0.491029)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.700000, RMS: 0.491801)

---

#### [19] Multiband Saturator

**Parameters:** 6

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Drive & Tone: High drive needs tone control to tame harshness
- **P0 + P2** (coupled): Drive & Output: Max drive requires output reduction to prevent clipping

**Test Results:**
- Total Tests: 105
- Passed: 105 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.718410, RMS: 0.156483)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.707596, RMS: 0.134837)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.725448, RMS: 0.165477)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.713178, RMS: 0.147736)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.698842, RMS: 0.090747)

---

#### [20] Muff Fuzz

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Drive & Tone: High drive needs tone control to tame harshness
- **P0 + P2** (coupled): Drive & Output: Max drive requires output reduction to prevent clipping

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.693545, RMS: 0.463754)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.693545, RMS: 0.463754)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.693545, RMS: 0.463754)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.693545, RMS: 0.463754)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.693545, RMS: 0.463754)

---

#### [21] Rodent Distortion

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Drive & Tone: High drive needs tone control to tame harshness
- **P0 + P2** (coupled): Drive & Output: Max drive requires output reduction to prevent clipping

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.377075, RMS: 0.294530)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.368221, RMS: 0.290120)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.382514, RMS: 0.296664)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.366454, RMS: 0.291094)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.414756, RMS: 0.295892)

---

#### [22] K-Style Overdrive

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Drive & Tone: High drive needs tone control to tame harshness
- **P0 + P2** (coupled): Drive & Output: Max drive requires output reduction to prevent clipping

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.579523, RMS: 0.432049)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.576620, RMS: 0.427934)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.568058, RMS: 0.421732)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.579635, RMS: 0.431351)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.568555, RMS: 0.419097)

---

### Modulation

#### [23] Digital Chorus

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Rate & Depth: Slow rate with high depth = seasick, fast + shallow = shimmer
- **P1 + P2** (conflicting): Depth & Feedback: Max depth + max feedback can cause instability

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.889524, RMS: 0.398698)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.858609, RMS: 0.360427)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.798619, RMS: 0.331195)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.883611, RMS: 0.376278)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.758536, RMS: 0.311952)

---

#### [24] Resonant Chorus

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Rate & Depth: Slow rate with high depth = seasick, fast + shallow = shimmer
- **P1 + P2** (conflicting): Depth & Feedback: Max depth + max feedback can cause instability

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.603757, RMS: 0.251357)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.612191, RMS: 0.251980)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.612362, RMS: 0.260410)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.595504, RMS: 0.250664)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.614491, RMS: 0.260750)

---

#### [25] Analog Phaser

**Parameters:** 5

**Known Parameter Interactions:**

- **P0 + P1** (synergistic): Rate & Depth: Slow rate with high depth = seasick, fast + shallow = shimmer
- **P1 + P2** (conflicting): Depth & Feedback: Max depth + max feedback can cause instability

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.361086, RMS: 0.246880)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.361082, RMS: 0.246881)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.361088, RMS: 0.246879)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.361084, RMS: 0.246881)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.361079, RMS: 0.246882)

---

#### [26] Ring Modulator

**Parameters:** 4

**Test Results:**
- Total Tests: 42
- Passed: 42 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.596454, RMS: 0.199554)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.599029, RMS: 0.196191)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.608551, RMS: 0.195844)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.600556, RMS: 0.198820)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.639219, RMS: 0.191200)

---

#### [27] Frequency Shifter

**Parameters:** 4

**Test Results:**
- Total Tests: 42
- Passed: 42 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.530860, RMS: 0.253601)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.523747, RMS: 0.260147)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.530228, RMS: 0.260376)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.538122, RMS: 0.259206)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.510353, RMS: 0.260286)

---

#### [28] Harmonic Tremolo

**Parameters:** 4

**Known Parameter Interactions:**

- **P0 + P1** (independent): Rate & Depth: Independent controls, all combinations valid

**Test Results:**
- Total Tests: 42
- Passed: 42 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 1.000135, RMS: 0.533572)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.612728, RMS: 0.370117)
- `P0_P1_Both_Max`: Good output levels (Peak: 1.043599, RMS: 0.505385)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.967882, RMS: 0.555358)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.805732, RMS: 0.395433)

---

#### [29] Classic Tremolo

**Parameters:** 4

**Known Parameter Interactions:**

- **P0 + P1** (independent): Rate & Depth: Independent controls, all combinations valid

**Test Results:**
- Total Tests: 42
- Passed: 42 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.760645, RMS: 0.379892)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.759959, RMS: 0.373790)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.759580, RMS: 0.377540)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.759676, RMS: 0.376694)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.759954, RMS: 0.383607)

---

#### [30] Rotary Speaker

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.560465, RMS: 0.358677)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.560398, RMS: 0.389193)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.560213, RMS: 0.399679)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.560411, RMS: 0.365823)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.597903, RMS: 0.389027)

---

#### [31] Pitch Shifter

**Parameters:** 4

**Test Results:**
- Total Tests: 42
- Passed: 42 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.880639, RMS: 0.412708)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.843528, RMS: 0.341434)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.719935, RMS: 0.375802)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.700000, RMS: 0.444977)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.748228, RMS: 0.442506)

---

#### [32] Detune Doubler

**Parameters:** 4

**Test Results:**
- Total Tests: 42
- Passed: 42 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.814760, RMS: 0.259721)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.836212, RMS: 0.259193)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.778790, RMS: 0.263874)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.690064, RMS: 0.262823)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.696710, RMS: 0.257920)

---

#### [33] Intelligent Harmonizer

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.480987, RMS: 0.252093)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.461762, RMS: 0.250028)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.461762, RMS: 0.251878)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.480987, RMS: 0.252093)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.461762, RMS: 0.256624)

---

### Reverb & Delay

#### [34] Tape Echo

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.350000, RMS: 0.245901)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.350000, RMS: 0.245901)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.468092, RMS: 0.256963)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.350000, RMS: 0.245901)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.454854, RMS: 0.317118)

---

#### [35] Digital Delay

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.381452, RMS: 0.267963)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.605336, RMS: 0.328696)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.381452, RMS: 0.267963)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.693204, RMS: 0.280577)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.528112, RMS: 0.305863)

---

#### [36] Magnetic Drum Echo

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.634978, RMS: 0.245995)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.652095, RMS: 0.246031)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.670882, RMS: 0.246080)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.670977, RMS: 0.246213)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.658514, RMS: 0.246328)

---

#### [37] Bucket Brigade Delay

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.777943, RMS: 0.545131)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.773088, RMS: 0.542968)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.770266, RMS: 0.538283)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.773222, RMS: 0.542869)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.765572, RMS: 0.536927)

---

#### [38] Buffer Repeat

**Parameters:** 4

**Test Results:**
- Total Tests: 42
- Passed: 42 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.726565, RMS: 0.255619)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.776900, RMS: 0.326642)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.773458, RMS: 0.329431)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.726565, RMS: 0.255619)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.772263, RMS: 0.329440)

---

#### [39] Plate Reverb

**Parameters:** 6

**Known Parameter Interactions:**

- **P1 + P2** (synergistic): Size & Damping: Large size needs damping to avoid metallic tail
- **P1 + P3** (coupled): Size & Pre-delay: Large size + long pre-delay = extreme spaciousness
- **P2 + P4** (conflicting): Damping & Diffusion: Max damping + low diffusion = muddy reverb

**Test Results:**
- Total Tests: 105
- Passed: 105 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.397684, RMS: 0.163212)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.537775, RMS: 0.346126)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.388045, RMS: 0.085734)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.453941, RMS: 0.251796)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.700000, RMS: 0.491801)

---

#### [40] Spring Reverb

**Parameters:** 5

**Known Parameter Interactions:**

- **P1 + P2** (synergistic): Size & Damping: Large size needs damping to avoid metallic tail
- **P1 + P3** (coupled): Size & Pre-delay: Large size + long pre-delay = extreme spaciousness
- **P2 + P4** (conflicting): Damping & Diffusion: Max damping + low diffusion = muddy reverb

**Test Results:**
- Total Tests: 70
- Passed: 68 (97%)
- Failed: 2
- Unstable: 2

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_Min`: Good output levels (Peak: 0.700000, RMS: 0.491801)
- `P0_P1_P1_Min_P2_Max`: Good output levels (Peak: 0.700000, RMS: 0.491801)
- `P0_P2_Both_Min`: Good output levels (Peak: 0.700000, RMS: 0.491801)
- `P0_P2_P1_Min_P2_Max`: Good output levels (Peak: 0.700000, RMS: 0.491801)
- `P0_P3_Both_Min`: Good output levels (Peak: 0.700000, RMS: 0.491801)

**⚠️  Danger Zones (Avoid These Combinations):**

- `P0_P3_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 

---

#### [41] Convolution Reverb

**Parameters:** 5

**Known Parameter Interactions:**

- **P1 + P2** (synergistic): Size & Damping: Large size needs damping to avoid metallic tail
- **P1 + P3** (coupled): Size & Pre-delay: Large size + long pre-delay = extreme spaciousness
- **P2 + P4** (conflicting): Damping & Diffusion: Max damping + low diffusion = muddy reverb

**Test Results:**
- Total Tests: 70
- Passed: 16 (22%)
- Failed: 54
- Unstable: 54

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_Min`: Good output levels (Peak: 0.700000, RMS: 0.491801)
- `P0_P1_P1_Min_P2_Max`: Good output levels (Peak: 0.700000, RMS: 0.491801)
- `P0_P2_Both_Low`: Good output levels (Peak: 1.139043, RMS: 0.454062)
- `P0_P2_Both_Min`: Good output levels (Peak: 0.700000, RMS: 0.491801)
- `P0_P2_P1_Min_P2_Max`: Good output levels (Peak: 0.700000, RMS: 0.491801)

**⚠️  Danger Zones (Avoid These Combinations):**

- `P0_P1_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P1_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P0_P1_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P1_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P0_P1_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P0_P2_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P3_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P3_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P0_P3_Both_Max`: Signal growing beyond control. Silent output detected. Excessive output level. 
- `P0_P3_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P0_P3_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P0_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_High`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P1_P2_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P2_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P3_Both_High`: Signal growing beyond control. Excessive output level. 
- `P1_P3_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P1_P3_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P3_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P1_P3_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P3_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P3_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_High`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P4_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P2_P3_Both_High`: Signal growing beyond control. Excessive output level. 
- `P2_P3_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P2_P3_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P2_P3_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P3_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_High`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P4_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_High`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P3_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P3_P4_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 

---

#### [42] Shimmer Reverb

**Parameters:** 6

**Known Parameter Interactions:**

- **P1 + P2** (synergistic): Size & Damping: Large size needs damping to avoid metallic tail
- **P1 + P3** (coupled): Size & Pre-delay: Large size + long pre-delay = extreme spaciousness
- **P2 + P4** (conflicting): Damping & Diffusion: Max damping + low diffusion = muddy reverb

**Test Results:**
- Total Tests: 105
- Passed: 105 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.368259, RMS: 0.169234)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.551761, RMS: 0.350194)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.332117, RMS: 0.077991)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.460781, RMS: 0.257552)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.700000, RMS: 0.491801)

---

#### [43] Gated Reverb

**Parameters:** 6

**Known Parameter Interactions:**

- **P1 + P2** (synergistic): Size & Damping: Large size needs damping to avoid metallic tail
- **P1 + P3** (coupled): Size & Pre-delay: Large size + long pre-delay = extreme spaciousness
- **P2 + P4** (conflicting): Damping & Diffusion: Max damping + low diffusion = muddy reverb

**Test Results:**
- Total Tests: 105
- Passed: 105 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.415564, RMS: 0.177750)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.556364, RMS: 0.352011)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.414359, RMS: 0.099990)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.466887, RMS: 0.261720)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.700000, RMS: 0.491801)

---

### Spatial & Special

#### [44] Stereo Widener

**Parameters:** 4

**Test Results:**
- Total Tests: 42
- Passed: 42 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.628443, RMS: 0.439429)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.628445, RMS: 0.439429)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.628447, RMS: 0.439429)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.628444, RMS: 0.439429)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.628443, RMS: 0.439429)

---

#### [45] Stereo Imager

**Parameters:** 4

**Test Results:**
- Total Tests: 42
- Passed: 42 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_Max`: Good output levels (Peak: 1.967244, RMS: 0.459443)
- `P0_P1_P1_Max_P2_Min`: Good output levels (Peak: 1.967391, RMS: 0.458807)
- `P0_P2_Both_Max`: Good output levels (Peak: 1.967295, RMS: 0.459235)
- `P0_P2_P1_Max_P2_Min`: Good output levels (Peak: 1.967395, RMS: 0.458431)
- `P0_P3_Both_Max`: Good output levels (Peak: 1.967294, RMS: 0.459529)

---

#### [46] Dimension Expander

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.603132, RMS: 0.309898)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.599944, RMS: 0.303083)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.597616, RMS: 0.287481)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.599954, RMS: 0.303121)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.591262, RMS: 0.283119)

---

#### [47] Spectral Freeze

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.661233, RMS: 0.461351)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.671649, RMS: 0.470588)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.700000, RMS: 0.486624)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.667845, RMS: 0.466946)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.700000, RMS: 0.491801)

---

#### [48] Spectral Gate

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.700000, RMS: 0.490727)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.700000, RMS: 0.490709)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.700000, RMS: 0.490702)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.700000, RMS: 0.490709)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.700000, RMS: 0.490711)

---

#### [49] Phased Vocoder

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 0 (0%)
- Failed: 70
- Unstable: 70

**⚠️  Danger Zones (Avoid These Combinations):**

- `P0_P1_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P1_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P0_P1_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P1_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P0_P1_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P1_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P1_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P2_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P2_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P3_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P3_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P0_P3_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P3_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P0_P3_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P3_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P3_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P4_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_High`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P2_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P2_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P3_Both_High`: Signal growing beyond control. Excessive output level. 
- `P1_P3_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P1_P3_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P3_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P1_P3_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P3_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P3_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_High`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P1_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P4_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P2_P3_Both_High`: Signal growing beyond control. Excessive output level. 
- `P2_P3_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P2_P3_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P2_P3_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P2_P3_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P3_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P3_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_High`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P2_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P2_P4_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_High`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P3_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P3_P4_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P3_P4_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 

---

#### [50] Granular Cloud

**Parameters:** 6

**Test Results:**
- Total Tests: 105
- Passed: 105 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.941955, RMS: 0.244938)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.800786, RMS: 0.226282)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.574019, RMS: 0.206667)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.808058, RMS: 0.242577)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.589575, RMS: 0.177973)

---

#### [51] Chaos Generator

**Parameters:** 5

**Test Results:**
- Total Tests: 70
- Passed: 70 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.439642, RMS: 0.223198)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.793050, RMS: 0.301202)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.790094, RMS: 0.492235)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.438052, RMS: 0.223490)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.795125, RMS: 0.497097)

---

#### [52] Feedback Network

**Parameters:** 6

**Test Results:**
- Total Tests: 105
- Passed: 100 (95%)
- Failed: 5
- Unstable: 5

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 0.350000, RMS: 0.245901)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.524995, RMS: 0.258396)
- `P0_P1_Both_Max`: Good output levels (Peak: 0.350000, RMS: 0.245901)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.515010, RMS: 0.247041)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.883010, RMS: 0.300416)

**⚠️  Danger Zones (Avoid These Combinations):**

- `P0_P1_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P2_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P4_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P4_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P5_Both_Min`: Signal growing beyond control. Excessive output level. 

---

### Utility

#### [53] Mid-Side Processor

**Parameters:** 4

**Test Results:**
- Total Tests: 42
- Passed: 41 (97%)
- Failed: 1
- Unstable: 1

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 1.758055, RMS: 1.200739)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.681613, RMS: 0.204579)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.699987, RMS: 0.491792)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.655075, RMS: 0.058243)
- `P0_P1_P1_Min_P2_Max`: Good output levels (Peak: 0.655075, RMS: 0.058243)

**⚠️  Danger Zones (Avoid These Combinations):**

- `P0_P3_Both_Max`: Signal growing beyond control. Excessive output level. 

---

#### [54] Gain Utility

**Parameters:** 2

**Test Results:**
- Total Tests: 7
- Passed: 7 (100%)
- Failed: 0
- Unstable: 0

**Sweet Spots (Recommended Settings):**

- `P0_P1_Both_High`: Good output levels (Peak: 1.000000, RMS: 0.671397)
- `P0_P1_Both_Low`: Good output levels (Peak: 0.649330, RMS: 0.184078)
- `P0_P1_Both_Max`: Good output levels (Peak: 1.000000, RMS: 0.892262)
- `P0_P1_Both_Mid`: Good output levels (Peak: 0.700000, RMS: 0.389677)
- `P0_P1_Both_Min`: Good output levels (Peak: 0.608231, RMS: 0.065162)

---

#### [55] Mono Maker

**Parameters:** 1

**Test Results:**
- Total Tests: 0
- Passed: 0 (0%)
- Failed: 0
- Unstable: 0

---

#### [56] Phase Align

**Parameters:** 3

**Test Results:**
- Total Tests: 21
- Passed: 0 (0%)
- Failed: 21
- Unstable: 21

**⚠️  Danger Zones (Avoid These Combinations):**

- `P0_P1_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P1_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P0_P1_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P1_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P0_P1_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P1_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P1_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_High`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P0_P2_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P2_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P0_P2_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_High`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_Low`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_Max`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_Mid`: Signal growing beyond control. Excessive output level. 
- `P1_P2_Both_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P2_P1_Max_P2_Min`: Signal growing beyond control. Excessive output level. 
- `P1_P2_P1_Min_P2_Max`: Signal growing beyond control. Excessive output level. 

---

## Appendix: Testing Methodology

### Parameter Interaction Tests

For each engine, the following parameter pair combinations were tested:

1. **Both Min** (0.0, 0.0) - Minimum values for both parameters
2. **Both Max** (1.0, 1.0) - Maximum values for both parameters
3. **P1 Min, P2 Max** (0.0, 1.0) - Extreme opposing values
4. **P1 Max, P2 Min** (1.0, 0.0) - Extreme opposing values
5. **Both Low** (0.3, 0.3) - Conservative low settings
6. **Both Mid** (0.5, 0.5) - Neutral mid-range settings
7. **Both High** (0.7, 0.7) - Conservative high settings

### Failure Criteria

- **NaN/Inf Output:** Audio buffer contains invalid floating-point values
- **Unstable:** Signal grows beyond 10.0 peak amplitude
- **Silent:** Output remains below -60dB after warmup period
- **Excessive Level:** Peak exceeds 5.0 (potential clipping)

### Test Conditions

- Sample Rate: 48kHz
- Block Size: 512 samples
- Test Signal: 440Hz sine wave at -3dB
- Processing Blocks: 50 per test

---

*End of Report*
