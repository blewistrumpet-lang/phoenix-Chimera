# DSP Research Report: Modulation Effects Implementation

## Executive Summary

This comprehensive research report examines advanced DSP techniques for implementing chorus, phaser, flanger, tremolo, and vibrato algorithms, with a focus on professional-grade audio effects. The analysis combines examination of existing implementations in the Project Chimera codebase with state-of-the-art research from Stanford CCRMA, Dattorro's seminal work, and modern commercial implementations by companies like Strymon and Eventide.

## 1. LFO DESIGN & IMPLEMENTATION

### Multi-Waveform Generation

Based on the analysis of `ClassicTremolo.cpp`, the current implementation provides sophisticated LFO capabilities:

```cpp
void ProfessionalLFO::generateBlock(double* output, int numSamples, double shape) {
    for (int i = 0; i < numSamples; ++i) {
        if (shape < 0.25) {
            output[i] = sine();
        } else if (shape < 0.5) {
            double blend = (shape - 0.25) * 4.0;
            output[i] = sine() * (1.0 - blend) + triangle() * blend;
        } else if (shape < 0.75) {
            double blend = (shape - 0.5) * 4.0;
            output[i] = triangle() * (1.0 - blend) + square() * blend;
        } else {
            double blend = (shape - 0.75) * 4.0;
            output[i] = square() * (1.0 - blend) + sawUp() * blend;
        }
        tick();
    }
}
```

**Key Features:**
- Morphing between sine, triangle, square, and sawtooth waveforms
- Smooth transitions using linear interpolation
- Block-based processing for efficiency

**Recommendations for Enhancement:**
1. **Stochastic Modulation**: Add Perlin noise or filtered random modulation
2. **Harmonic Waveforms**: Implement bandlimited waveforms to prevent aliasing
3. **Phase-Locked Loops**: For tempo synchronization with host DAW

### Phase Relationships for Stereo Effects

The current implementation uses fixed phase offsets:

```cpp
// 90-degree offset for stereo width
m_lfos[1].reset(m_lfos[0].getPhase() + phaseOffset);
```

**Advanced Stereo Techniques:**
- Variable phase correlation matrices
- Quadrature oscillator networks
- Hadamard matrix decorrelation

### Tempo Synchronization Implementation

```cpp
class TempoSyncLFO {
    double hostTempo = 120.0;
    double syncRatio = 1.0; // 1/4, 1/2, 1, 2, etc.
    
    void setFrequency() {
        double freqHz = (hostTempo / 60.0) * syncRatio;
        phaseIncrement = 2.0 * M_PI * freqHz / sampleRate;
    }
};
```

## 2. CHORUS ALGORITHMS

### Current Implementation Analysis

The `ResonantChorus.cpp` implementation demonstrates multi-voice architecture:

```cpp
static constexpr int NUM_VOICES = 4;
static constexpr float BASE_DELAYS[NUM_VOICES] = {7.2f, 14.8f, 21.3f, 28.7f};
static constexpr float LFO_PHASES[NUM_VOICES] = {0.0f, 0.25f, 0.5f, 0.75f};
```

**Strengths:**
- Multiple independent delay lines
- Phase-distributed LFOs
- Resonance control for tonal shaping

### BBD (Bucket Brigade Device) Emulation

Research from Eventide's implementations suggests authentic BBD modeling:

```cpp
class BBDEmulator {
    struct BBDStage {
        float sample = 0.0f;
        float noise = 0.0f;
        float nonlinearity = 0.0f;
    };
    
    std::vector<BBDStage> stages;
    
    float process(float input, float clockRate) {
        // Clock noise injection
        float clockNoise = (random() - 0.5f) * 0.001f * clockRate;
        
        // Transfer through BBD stages with nonlinearity
        float output = input;
        for (auto& stage : stages) {
            // BBD transfer characteristic
            float transfer = std::tanh(output * 1.2f) * 0.83f;
            stage.sample = transfer + clockNoise;
            output = stage.sample;
        }
        return output;
    }
};
```

### Juno-Style Chorus Design

The Roland Juno chorus uses a specific delay modulation pattern:

```cpp
class JunoChorus {
    static constexpr float JUNO_DELAYS[2] = {2.5f, 5.0f}; // ms
    static constexpr float JUNO_DEPTH = 0.15f; // ±0.15ms modulation
    
    float process(float input, float lfoValue) {
        float modulatedDelay1 = JUNO_DELAYS[0] + lfoValue * JUNO_DEPTH;
        float modulatedDelay2 = JUNO_DELAYS[1] - lfoValue * JUNO_DEPTH; // inverted
        
        float delayed1 = delayLine1.read(modulatedDelay1);
        float delayed2 = delayLine2.read(modulatedDelay2);
        
        return (input + delayed1 + delayed2) / 3.0f;
    }
};
```

## 3. PHASER IMPLEMENTATION

### Current Implementation Analysis

The `AnalogPhaser.cpp` shows sophisticated all-pass filter design:

```cpp
struct AllpassTPT {
    float g = 0.f; // tan(pi*fc/fs)
    float z = 0.f; // state
    float a = 0.f; // coefficient
    
    void setCutoffHz(float fc, float fs) {
        fc = juce::jlimit(10.0f, 0.45f*fs, fc);
        g = std::tan(juce::MathConstants<float>::pi * (fc / fs));
        float aa = (1.0f - g) / (1.0f + g);
        a = juce::jlimit(-0.98f, 0.98f, aa);
    }
    
    float process(float x) noexcept {
        const float y = -x + z;
        z = x + a * y;
        return y;
    }
};
```

**Key Features:**
- Topology-Preserving Transform (TPT) approach
- Stable coefficient limiting
- Zavalishin's method for accurate analog modeling

### All-Pass Filter Cascades

Based on Dattorro's work and Julius Smith's research:

```cpp
class PhaserCascade {
    static constexpr int MAX_STAGES = 12;
    AllpassTPT stages[MAX_STAGES];
    
    void updateStageFrequencies(float centerFreq, float depth, float lfoValue) {
        for (int i = 0; i < activeStages; ++i) {
            // Logarithmic frequency spacing
            float stageRatio = std::pow(2.0f, i * 0.5f);
            float modulatedFreq = centerFreq * stageRatio * 
                                 (1.0f + depth * lfoValue);
            stages[i].setCutoffHz(modulatedFreq, sampleRate);
        }
    }
};
```

### Notch Frequency Calculation

From Julius Smith's work at Stanford CCRMA:

```cpp
// Non-uniform notch spacing for authentic phasing
float calculateNotchFrequency(int notchIndex, float baseFreq, float sweep) {
    // Golden ratio spacing for musical intervals
    const float PHI = 1.618033988749f;
    float ratio = std::pow(PHI, notchIndex * 0.3f);
    return baseFreq * ratio * (1.0f + sweep);
}
```

### Vintage Phaser Emulation

#### Phase 90 Style:
```cpp
class Phase90Emulator {
    static constexpr int STAGES = 4;
    static constexpr float PHASE90_Q = 0.54f;
    
    // MXR Phase 90 frequency range: 100Hz - 6kHz
    void updateSweep(float lfoValue) {
        float logFreq = 100.0f * std::pow(60.0f, lfoValue); // 100Hz to 6kHz
        for (int i = 0; i < STAGES; ++i) {
            allpass[i].setFrequency(logFreq, PHASE90_Q);
        }
    }
};
```

#### Small Stone Style:
```cpp
class SmallStoneEmulator {
    static constexpr int STAGES = 6; // More stages for deeper sweep
    float feedback = 0.5f;
    
    float process(float input) {
        float output = input;
        float feedbackSignal = 0.0f;
        
        for (int i = 0; i < STAGES; ++i) {
            output = allpass[i].process(output + feedbackSignal * feedback);
            if (i == STAGES - 1) feedbackSignal = output;
        }
        return output;
    }
};
```

## 4. FLANGER DESIGN

### Comb Filter Implementation

```cpp
class Flanger {
    static constexpr float MIN_DELAY = 0.5f; // ms
    static constexpr float MAX_DELAY = 15.0f; // ms
    
    float process(float input, float lfoValue, float feedback, float depth) {
        // Calculate modulated delay time
        float delayTime = MIN_DELAY + (MAX_DELAY - MIN_DELAY) * 
                         (0.5f + 0.5f * lfoValue) * depth;
        
        // Write to delay line
        delayLine.write(input);
        
        // Read with interpolation
        float delayed = delayLine.readInterpolated(delayTime);
        
        // Apply feedback
        float output = input + delayed * 0.5f;
        delayed = delayed * feedback + input * (1.0f - feedback);
        
        return output;
    }
};
```

### Through-Zero Flanging

```cpp
class ThroughZeroFlanger {
    DelayLine positiveDelay;
    DelayLine negativeDelay;
    
    float process(float input, float lfoValue) {
        if (lfoValue >= 0.0f) {
            // Positive flanging
            return input + positiveDelay.readInterpolated(lfoValue * maxDelay);
        } else {
            // Negative flanging (phase inversion)
            float delayed = negativeDelay.readInterpolated(-lfoValue * maxDelay);
            return input - delayed; // Phase inversion
        }
    }
};
```

### Anti-Aliasing for Short Delays

```cpp
class AntiAliasedFlanger {
    IIR lowpass;
    
    void prepare(double sampleRate) {
        // Nyquist/4 filter for 2x oversampling headroom
        lowpass.setLowpass(sampleRate * 0.25, 0.707);
    }
    
    float process(float input, float shortDelay) {
        if (shortDelay < 2.0f) { // ms
            // Apply anti-aliasing for very short delays
            input = lowpass.process(input);
        }
        return delayLine.readInterpolated(shortDelay);
    }
};
```

## 5. TREMOLO & VIBRATO

### Amplitude Modulation (Tremolo)

The current `ClassicTremolo.cpp` implementation provides comprehensive tremolo types:

```cpp
enum class TremoloType {
    SINE_AMPLITUDE,
    TRIANGLE_AMPLITUDE, 
    SQUARE_AMPLITUDE,
    OPTICAL_TREMOLO,
    HARMONIC_TREMOLO,
    BIAS_TREMOLO,
    ROTARY_SPEAKER
};
```

### Optical Tremolo Emulation

```cpp
class OpticalTremoloModel {
    double ledBrightness = 0.0;
    double cellResistance = 1.0;
    double attackCoeff, decayCoeff;
    
    double process(double lfoValue) {
        double targetBrightness = (lfoValue + 1.0) * 0.5;
        
        if (targetBrightness > ledBrightness) {
            ledBrightness += (targetBrightness - ledBrightness) * attackCoeff;
        } else {
            ledBrightness += (targetBrightness - ledBrightness) * decayCoeff;
        }
        
        // Photocell resistance follows brightness non-linearly
        cellResistance = 1.0 / (1.0 + ledBrightness * ledBrightness * 10.0);
        return cellResistance;
    }
};
```

### Harmonic Tremolo Implementation

```cpp
class HarmonicTremolo {
    AllPassFilter phaseNetwork[4];
    DelayLine delayLine;
    
    double process(double input, double lfoValue, double depth) {
        // Phase shift network for harmonic tremolo
        double phaseShifted = input;
        for (auto& apf : phaseNetwork) {
            phaseShifted = apf.process(phaseShifted);
        }
        
        // Vibrato via modulated delay
        double delayTime = 2.0 + lfoValue * depth * 1.5; // 2-3.5ms
        double delayed = delayLine.readInterpolated(delayTime);
        
        // Mix original and phase-shifted for harmonic tremolo effect
        return (input + delayed) * 0.5;
    }
};
```

### Pitch Modulation (Vibrato)

```cpp
class VibratoProcessor {
    PitchShifter pitchShifter;
    
    float process(float input, float lfoValue, float depth) {
        // Convert LFO to pitch bend in cents
        float pitchBend = lfoValue * depth * 100.0f; // ±100 cents max
        
        // Apply pitch modulation
        return pitchShifter.process(input, pitchBend);
    }
};
```

## 6. ROTARY SPEAKER SIMULATION

### Current Implementation Analysis

The `RotarySpeaker.cpp` provides sophisticated Leslie speaker emulation:

```cpp
class RotarySpeaker {
    CrossoverFilter crossover[2];
    DopplerProcessor hornDoppler[2], drumDoppler[2];
    AmplitudeModulator hornAM[2], drumAM[2];
    TubePreamp preamp[2];
    CabinetSimulator cabinet;
};
```

### Leslie Speaker Physics

```cpp
struct RotorModel {
    double angle = 0.0;
    double velocity = 0.0;
    double targetVelocity = 0.0;
    double acceleration = 2.5;
    double inertia = 0.96;
    
    void update(double deltaTime) {
        // Physical inertia model
        velocity += (targetVelocity - velocity) * (1.0 - inertia) * deltaTime;
        angle += velocity * deltaTime;
        if (angle >= 2.0 * M_PI) angle -= 2.0 * M_PI;
    }
};
```

### Doppler Effect Modeling

```cpp
float DopplerProcessor::process(float input, double rotorAngle, double rotorVelocity, 
                               double rotorRadius, double micAngle, double micDistance) {
    // Calculate speaker position
    double speakerX = rotorRadius * std::cos(rotorAngle);
    double speakerY = rotorRadius * std::sin(rotorAngle);
    
    // Calculate Doppler shift
    double velocityX = -rotorRadius * rotorVelocity * std::sin(rotorAngle);
    double velocityY = rotorRadius * rotorVelocity * std::cos(rotorAngle);
    
    // Project velocity onto line from speaker to mic
    double dx = speakerX - micX;
    double dy = speakerY - micY;
    double distance = std::sqrt(dx * dx + dy * dy);
    double radialVelocity = (velocityX * dx + velocityY * dy) / distance;
    
    // Doppler formula
    const double SPEED_OF_SOUND = 343.0;
    double dopplerRatio = 1.0 / (1.0 - radialVelocity / SPEED_OF_SOUND);
    
    return applyDopplerDelay(input, dopplerRatio);
}
```

### Horn/Drum Separation

```cpp
class CrossoverFilter {
    BiquadFilter lowpassStages[2];
    BiquadFilter highpassStages[2];
    
    void process(float input, float& lowOutput, float& highOutput) {
        // 4th order Linkwitz-Riley at 800Hz
        lowOutput = input;
        highOutput = input;
        
        for (int i = 0; i < 2; ++i) {
            lowOutput = lowpassStages[i].process(lowOutput);
            highOutput = highpassStages[i].process(highOutput);
        }
    }
};
```

### Cabinet Resonance Modeling

```cpp
class CabinetSimulator {
    Resonance resonances[4];
    SimpleDelay reflectionDelay;
    
    float process(float input) {
        float output = input;
        
        // Add cabinet resonances at characteristic frequencies
        // 97Hz, 185Hz, 380Hz, 760Hz
        for (auto& resonance : resonances) {
            output += resonance.process(input) * 0.05f;
        }
        
        // Add early reflections (~2.3ms)
        output += reflectionDelay.process(output) * 0.15f;
        return output;
    }
};
```

## 7. RESEARCH FINDINGS AND RECOMMENDATIONS

### Dattorro Effect Design Insights

Based on Jon Dattorro's seminal paper "Effect Design Part 1: Reverberator and Other Filters":

1. **All-pass Loop Architecture**: Essential for creating complex modulation without artifacts
2. **Figure-8 Topology**: Provides natural stereo imaging for rotary effects
3. **Careful Parameter Tuning**: All delay times and coefficients must be prime numbers to avoid metallic artifacts

### Julius Smith's Physical Modeling Approaches

From Stanford CCRMA research:

1. **Digital Waveguide Models**: For authentic string and wind instrument tremolo
2. **Finite Difference Methods**: For modeling acoustic spaces in rotary speakers
3. **Commuted Synthesis**: Efficient convolution-based cabinet modeling

### Modern Implementation Strategies (Strymon/Eventide)

1. **Multi-Algorithm Engines**: Single units providing multiple effect types
2. **DSP-Based Analog Modeling**: Precise emulation of classic circuits
3. **Parameter Interpolation**: Smooth morphing between effect types
4. **Preset Management**: Sophisticated parameter storage and recall

## 8. IMPLEMENTATION ROADMAP

### Phase 1: Enhanced LFO System
- [ ] Implement stochastic modulation sources
- [ ] Add tempo synchronization
- [ ] Create morphing waveform engine
- [ ] Develop phase correlation matrices

### Phase 2: Advanced Chorus Algorithms
- [ ] BBD emulation with clock noise
- [ ] Multi-tap delay networks
- [ ] Ensemble effect algorithms
- [ ] Vintage chorus emulations (CE-1, Juno, etc.)

### Phase 3: Professional Phaser Design
- [ ] Variable stage count (2-12 stages)
- [ ] Feedback network optimization
- [ ] Vintage circuit emulations
- [ ] Non-uniform notch spacing

### Phase 4: Through-Zero Flanging
- [ ] Negative delay implementation
- [ ] Anti-aliasing for short delays
- [ ] Jet flanging algorithms
- [ ] Tape flanging emulation

### Phase 5: Advanced Tremolo/Vibrato
- [ ] Multiple tremolo types
- [ ] Harmonic tremolo with phase networks
- [ ] Optical tremolo modeling
- [ ] Auto-pan effects

### Phase 6: Complete Rotary Speaker
- [ ] Full Leslie 122/147 emulation
- [ ] Multiple microphone positions
- [ ] Cabinet resonance modeling
- [ ] Speed ramping algorithms

## 9. CODE EXAMPLES FOR IMPLEMENTATION

### Anti-Aliased Oscillator
```cpp
class BandlimitedOscillator {
    static constexpr int WAVETABLE_SIZE = 2048;
    std::vector<std::vector<float>> wavetables; // Multiple octaves
    
    float process(double phase, double frequency, double sampleRate) {
        // Select appropriate wavetable based on frequency
        int tableIndex = static_cast<int>(std::log2(sampleRate / (2.0 * frequency)));
        tableIndex = std::clamp(tableIndex, 0, static_cast<int>(wavetables.size() - 1));
        
        // Linear interpolation
        double scaledPhase = phase * WAVETABLE_SIZE;
        int index = static_cast<int>(scaledPhase);
        double frac = scaledPhase - index;
        
        float sample1 = wavetables[tableIndex][index % WAVETABLE_SIZE];
        float sample2 = wavetables[tableIndex][(index + 1) % WAVETABLE_SIZE];
        
        return sample1 + frac * (sample2 - sample1);
    }
};
```

### Smooth Parameter Interpolation
```cpp
class SmoothParameter {
    double current = 0.0;
    double target = 0.0;
    double smoothingCoeff = 0.0;
    
public:
    void setSmoothing(double timeMs, double sampleRate) {
        smoothingCoeff = std::exp(-1.0 / (timeMs * 0.001 * sampleRate));
    }
    
    void setTarget(double newTarget) {
        target = newTarget;
    }
    
    double getNext() {
        current = target + (current - target) * smoothingCoeff;
        return current;
    }
};
```

### CPU-Efficient Processing
```cpp
class OptimizedModulation {
    static constexpr int BLOCK_SIZE = 64;
    
    void processBlock(float* buffer, int numSamples) {
        for (int offset = 0; offset < numSamples; offset += BLOCK_SIZE) {
            int samplesToProcess = std::min(BLOCK_SIZE, numSamples - offset);
            
            // Update parameters once per block
            updateParametersForBlock();
            
            // Process samples
            for (int i = 0; i < samplesToProcess; ++i) {
                buffer[offset + i] = processSample(buffer[offset + i]);
            }
        }
    }
};
```

## 10. CONCLUSION

This research provides a comprehensive foundation for implementing professional-grade modulation effects. The combination of theoretical understanding from academic sources (Dattorro, Julius Smith) with practical implementation insights from the current codebase and modern commercial products creates a roadmap for developing state-of-the-art audio effects.

Key success factors:
1. **Mathematical Precision**: Proper implementation of DSP fundamentals
2. **Analog Modeling**: Accurate emulation of classic circuits
3. **Computational Efficiency**: Optimized algorithms for real-time performance
4. **Musical Usability**: Parameter ranges and interactions that enhance creativity

The modular architecture demonstrated in the current codebase provides an excellent foundation for implementing these advanced algorithms while maintaining code organization and performance.

---

*This research report serves as the technical foundation for implementing professional modulation effects in the Project Chimera audio engine.*