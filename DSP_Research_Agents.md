# DSP Research Agents for Chimera Phoenix

## Overview
These agents conduct deep research on DSP implementation methods for each engine category. Each agent focuses on finding common practices, innovative techniques, and proper implementation methods with solid references.

## Agent Definitions

### 1. Pitch/Frequency Research Agent
**Purpose:** Research pitch shifting, frequency shifting, harmonization, and ring modulation techniques.

**Research Focus:**
- Phase vocoder implementations (FFT-based pitch shifting)
- PSOLA (Pitch Synchronous Overlap-Add) algorithms
- Granular synthesis for pitch manipulation
- Time-domain vs frequency-domain approaches
- Ring modulation and frequency shifting mathematics
- Formant preservation techniques
- Real-time pitch detection algorithms
- Harmonic/inharmonic shifting methods

**Key References to Find:**
- Dolson 1986 phase vocoder paper
- Moulines & Charpentier PSOLA papers
- Zölzer DAFX book chapters
- Miller Puckette techniques
- IRCAM research papers
- Csound/SuperCollider implementations

### 2. Dynamics Research Agent
**Purpose:** Research compression, limiting, gating, and transient shaping algorithms.

**Research Focus:**
- Digital dynamics processing theory
- Attack/release envelope detection
- RMS vs peak detection methods
- Lookahead techniques for limiting
- Feedforward vs feedback topologies
- Optical compressor modeling
- VCA compressor emulation
- Transient detection algorithms
- Adaptive release curves
- True peak limiting (ISP prevention)

**Key References to Find:**
- Giannoulis et al. "Digital Dynamic Range Compressor Design"
- Zölzer dynamics processing chapters
- Reiss & McPherson "Audio Effects"
- AES papers on dynamics processing
- UAD/Waves white papers on analog modeling

### 3. Modulation Research Agent
**Purpose:** Research chorus, phaser, flanger, tremolo, and vibrato effects.

**Research Focus:**
- LFO design and implementation
- All-pass filter networks for phasers
- Delay line modulation for chorus/flanger
- BBD (Bucket Brigade Device) emulation
- Multi-voice chorus algorithms
- Rotary speaker (Leslie) simulation
- Ring modulation for tremolo
- Stereo widening through modulation
- Anti-aliasing in modulated delays

**Key References to Find:**
- Dattorro "Effect Design" papers
- Smith "Physical Audio Signal Processing"
- Verfaille et al. on adaptive effects
- Vintage circuit analysis papers
- Leslie speaker physics research

### 4. Distortion Research Agent
**Purpose:** Research saturation, overdrive, fuzz, and harmonic excitation methods.

**Research Focus:**
- Waveshaping functions
- Tube/valve modeling techniques
- Transistor distortion emulation
- Harmonic generation methods
- Anti-aliasing for nonlinear processing
- Oversampling strategies
- Dynamic convolution for amp modeling
- Tape saturation algorithms
- Bit crushing and sample rate reduction
- Psychoacoustic harmonic enhancement

**Key References to Find:**
- Pakarinen & Yeh tube modeling papers
- Schattschneider & Zölzer distortion papers
- Wave digital filters for circuits
- SPICE model conversions
- Kemper/Fractal patents
- Analog circuit analysis papers

### 5. Filter Research Agent
**Purpose:** Research digital filter design and analog filter emulation.

**Research Focus:**
- IIR and FIR filter design
- Analog prototype conversions
- Ladder filter (Moog) emulation
- State variable filter topologies
- Zero-delay feedback filters
- Resonance and self-oscillation
- Formant filter design
- Comb filter applications
- Morphable filter architectures
- Oversampling for filter stability

**Key References to Find:**
- Stilson & Smith Moog filter paper
- Zavalishin "Art of VA Filter Design"
- Pirkle filter design books
- Analog filter circuit papers
- Native Instruments white papers
- DSP textbooks (Oppenheim, Proakis)

### 6. Spatial Research Agent
**Purpose:** Research stereo processing, spatialization, and psychoacoustic effects.

**Research Focus:**
- M/S (Mid-Side) processing theory
- Haas effect and precedence
- HRTF (Head-Related Transfer Functions)
- Binaural processing techniques
- Stereo widening algorithms
- Crossfeed for headphones
- Ambisonic encoding/decoding
- Room simulation basics
- Phase manipulation for width
- Mono compatibility considerations

**Key References to Find:**
- Blumlein stereo papers
- Bauer crossfeed circuit
- Gardner HRTF database
- Gerzon ambisonic papers
- Orban/Aphex patents
- AES spatial audio papers

### 7. Time-Based Research Agent
**Purpose:** Research delays, reverbs, and time-manipulation effects.

**Research Focus:**
- Digital delay line implementation
- Reverb algorithm design (Schroeder, FDN)
- Convolution reverb optimization
- Pitch-shifting delays
- Granular time stretching
- Buffer manipulation techniques
- Freeze/hold effects
- Reverse reverb algorithms
- Gated reverb techniques
- Multi-tap delay design

**Key References to Find:**
- Schroeder reverb papers
- Jot reverb research
- Dattorro reverb topology
- Gardner room acoustics
- Lexicon/Eventide patents
- Stautner & Puckette papers

### 8. Utility Research Agent
**Purpose:** Research analysis, metering, and utility processing.

**Research Focus:**
- FFT optimization techniques
- Spectral analysis methods
- Phase correlation measurement
- Loudness metering (LUFS/LKFS)
- Peak/RMS detection
- Spectrum analyzer design
- Phase scope implementation
- Gain staging best practices
- Dithering and noise shaping
- Sample rate conversion

**Key References to Find:**
- ITU-R BS.1770 loudness specs
- Lipshitz dithering papers
- Parks-McClellan filter design
- FFT windowing functions
- AES metering standards
- Mastering engineering papers

## Implementation Notes

Each agent should:
1. Search for academic papers, patents, and technical documentation
2. Find open-source implementations for reference
3. Identify common pitfalls and solutions
4. Create implementation roadmaps with complexity ratings
5. Compile best practices and optimization techniques
6. Note CPU/memory trade-offs for different approaches

## Output Format

Each agent produces a research report containing:
- Executive summary of findings
- Categorized implementation methods (basic/intermediate/advanced)
- Annotated reference list with relevance scores
- Code examples and pseudocode
- Performance considerations
- Common mistakes to avoid
- Testing methodologies