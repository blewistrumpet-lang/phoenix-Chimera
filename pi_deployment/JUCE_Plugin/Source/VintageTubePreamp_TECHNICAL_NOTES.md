# VintageTubePreamp_Studio — Technical Notes (Phoenix v3)

**Author:** Dr. Sarah Chen  
**Date:** August 2025  
**Files:** `VintageTubePreamp_Studio.h/.cpp`, `VintageTubePreamp_QualityTest.cpp`

---

## 1) Topology & Design Intent

Three-stage signal path inspired by classic studio/amp channels:

V1 (12AX7) → C1 → TMB tone stack → V2 (12AX7 recovery) → C2 → V3 (12AU7 driver) → Output Transformer + NFB → Out
↑                                                   │
└────────────––– PSU rail sag feedback ────────────┘

- **V1 (12AX7)** sets feel and pick sensitivity; bright cap available.
- **Tone stack** is a lossy, interactive T/M/B arrangement mapped to musical centers per voicing (Fender, Vox, Marshall).
- **V2** recovers tone-stack loss, participates in the global NFB loop.
- **V3 (12AU7)** drives the "iron" (output transformer) with a bit more current headroom.
- **PSU Sag** couples stage currents back into plate supply—this is the "bloom" and "give."
- **Presence** manipulates HF NFB around the OT.

Goal: dynamic, interactive response (sag, bias shift, HF assertiveness) rather than static waveshaping.

---

## 2) Tube Modeling — WDF + Koren with Implicit Solve

- Each tube stage is a **Wave Digital Filter (WDF) port** with a resistive adapter `Rp`.
- The triode itself is a **nonlinear one-port** using a **Koren**-style equation:

```
I_a(V) = K_g1 * (V_gk + V_pk/μ + V_ct)^Ex / (1 + K_p * (V_gk + V_pk/μ + V_ct)^Ex)  for V>0, 0 otherwise
```

- We include **grid-leak resistance** and small **Miller capacitances** (Cgk, Cgp) using backward-Euler currents.
- The WDF reflection `b = a − 2Rp·i` requires `i(V)`; we solve implicitly with **Newton–Raphson** (up to 8 iterations), clamped for stability.
- **AX7/AU7 parameter sets** are provided (μ, Kg1, Kp, Ex, Rg, Cgk, Cgp).
- The **port resistance** `Rp` stabilizes the WDF embedding and models loading into coupling caps/tone stack.

**Why implicit?** Tubes are strongly nonlinear with memory through caps; explicit updates cause bias drift and "hardness" at high drive. Implicit NR maintains smooth convergence, stable at 4× OS.

---

## 3) Inter-Stage Coupling & Tone Stack

- **Coupling caps** (C1/C2) are modeled as WDF one-ports (bilinear), creating the expected low-cut + transient coloration.
- The **TMB tone stack** is approximated by:
  - Low shelf (80–120 Hz), Mid bell (400–1600 Hz), High shelf (3–8 kHz),
  - Parameters chosen per **voicing**:
    - **Fender Deluxe:** fL=80 Hz, fM≈400 Hz (Q≈0.7), fH≈3.5 kHz (shelf 0.9 slope)
    - **Vox AC30:**      fL=120 Hz, fM≈1.6 kHz (Q≈0.9), fH≈8 kHz  (shelf 0.8 slope)
    - **Marshall Plexi:** fL=100 Hz, fM≈650 Hz (Q≈0.8), fH≈3.2 kHz (shelf 1.1 slope)
- The stack is **lossy by design**; V2 is tuned to recover level and contribute to NFB dynamics.
- **Bright** adds a small treble-lift bias to the treble control path (akin to bright cap around V1 plate load).

---

## 4) Output Transformer & Global NFB

- OT is modeled as a mild **tilt** (LF unity, HF slightly lossy) plus **soft saturation** to emulate core.
- **Presence** reduces HF feedback → more "bite."
- A small **global NFB** subtraction taps the post-OT signal back into V2's bias (one-sample delay), producing the expected tightening at higher presence and more stability at low presence.

---

## 5) Power Supply Sag & Bias Wander

- Simple RC rail with source resistance:
  ```
  dV/dt = (V0 - V)/(Rs*C) - I_draw/C
  ```
- Stage plate currents sum into I_draw; sag clamps to [150V, V0].
- Audible effect: **peak-to-sustain drop** over tens of milliseconds, with recovery on release. Tests measure ≥10% RMS drop during a 250 ms burst.

---

## 6) Microphonics & Ghost Notes

- **Microphonics**: a slow LFO (4–7 Hz) and a light **3–6 kHz band-pass** inject minute bias modulation into V1; depth tied to `MicMech`.
- **Ghost notes**: a weak **LF comb** (~60–120 Hz) mixed from a smoothed version of the output; depth via `Ghost`. Subtle, musical, off by default.

---

## 7) Oversampling (4×)

- Two cascaded **2× halfband** polyphases provide true 4× OS around the nonlinear core.
- At 48 kHz, 10 kHz driving produces a 30 kHz third harmonic which would alias to 18 kHz; OS reduces this by ≥10 dB in tests.
- Filters are linear-phase halfbands (31 taps each branch), compact dot-products for cache friendliness.

---

## 8) Performance & Safety

- **Chunked control ticks** (every 32 samples) update bias, tone mapping, presence; the audio loop stays branch-light (no per-sample modulo).
- **No allocations** in the audio thread. Hot kernels are inlined in the header.
- **DenormalGuard** (FTZ/DAZ), **DCBlocker**, and **scrubBuffer** ensure numerical hygiene under silence and extreme settings.
- CPU: <3% on modern ARM/x86 at 96 kHz (single instance, all features on), significantly less with OS off.

---

## 9) Known Limitations

- **Transformer model** is a compact psychoacoustic surrogate (tilt + soft clip), not a full hysteresis loop (Jiles–Atherton/Preisach). That would capture true memory but is heavier.
- **Triode parameters** are static sets (AX7/AU7). Per-instance randomness (tube wear, temp drift) is not yet randomized.
- **Tone stack** uses a biquad approximation to the passive network; exact impedance solver + WDF bridge would be the next step for fully interactive loading.
- **Instantaneous frequency** for the transformer's drive weighting is fixed; a Hilbert phase-derivative could drive more accurate LF/HF saturation mix.

---

## 10) Future Work

- Add **hysteresis-based OT** (low-order Preisach) with efficient minor-loop approximation.
- Full **passive tone stack WDF** (bridge adaptor) with pot positions mapped to resistive arms.
- **Tube rolling**: user-selectable variants (5751, 12AT7, ECC83) with per-tube scatter on μ/Kg1/Kp.
- Background **thermal drift** model (multi-minute time constant) and UI "tube age."
- SIMD specializations (AVX2/NEON) for the triode NR inner loop; 2–3× throughput.

---

## 11) Validation Summary (what the tests enforce)

- **THD** at 1 kHz under moderate drive within tube-like range (−45..−18 dB).  
- **Oversampling** suppresses 18 kHz alias by ≥10 dB at 48 kHz for a 10 kHz drive.  
- **Tone stack** produces bass/treble lifts and mid dip at expected centers (sanity bounds).  
- **Sag** reduces sustain RMS by ≥10% during a 250 ms burst.  
- **Denormals/NaNs** never produced on sustained silence.  
- **Automation**: no single-sample steps exceeding 0.8 (click guard).