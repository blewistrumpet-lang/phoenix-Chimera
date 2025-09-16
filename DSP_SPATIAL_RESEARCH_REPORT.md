# DSP Research Report: Spatial Audio Processing, Spatialization, and Psychoacoustic Effects

## Executive Summary

This comprehensive research report covers spatial audio processing, stereo processing, spatialization techniques, and psychoacoustic effects, focusing on production-ready implementations with emphasis on mono compatibility and phase coherence. The research covers six key areas with detailed analysis of current implementations in the Chimera Phoenix codebase and modern industry standards.

## 1. MID-SIDE PROCESSING

### 1.1 Mathematical Foundations

**M/S Encoding Matrix:**
```
Mid = (L + R) / √2
Side = (L - R) / √2
```

**M/S Decoding Matrix:**
```
L = (Mid + Side) / √2
R = (Mid - Side) / √2
```

### 1.2 Implementation from Codebase

The `MidSideProcessor_Platinum` demonstrates sophisticated M/S processing:

```cpp
// Professional M/S Processing with double precision
class MidSideProcessor {
    // Encoding
    void encode(float left, float right, float& mid, float& side) {
        mid = (left + right) * 0.7071067811865476f;  // 1/√2
        side = (left - right) * 0.7071067811865476f;
    }
    
    // Decoding with width control
    void decode(float mid, float side, float& left, float& right, float width) {
        // Width: 0 = mono, 1 = normal, 2 = extra wide
        side *= width;
        
        left = (mid + side) * 0.7071067811865476f;
        right = (mid - side) * 0.7071067811865476f;
    }
    
    // Independent processing
    float processMid(float mid) {
        // Apply EQ, compression, etc. to center image
        return midEQ.process(midCompressor.process(mid));
    }
    
    float processSide(float side) {
        // Apply different processing to sides
        return sideEQ.process(sideEnhancer.process(side));
    }
};
```

### 1.3 Width Control Implementation

```cpp
class StereoWidthController {
    float bassMonoFreq = 120.0f;  // Hz
    ButterworthFilter bassMonoFilter;
    
public:
    void process(float& left, float& right, float width) {
        // Extract bass for mono processing
        float bassMono = bassMonoFilter.processLowpass(
            (left + right) * 0.5f, bassMonoFreq);
        
        // Remove bass from sides
        float leftHigh = left - bassMono;
        float rightHigh = right - bassMono;
        
        // Apply width to high frequencies only
        float mid = (leftHigh + rightHigh) * 0.5f;
        float side = (leftHigh - rightHigh) * 0.5f * width;
        
        // Reconstruct with mono bass
        left = bassMono + mid + side;
        right = bassMono + mid - side;
    }
};
```

### 1.4 Bass Mono Techniques

**Elliptical Filtering for Vinyl Compatibility:**
```cpp
class EllipticalFilter {
    // Prevents vertical stylus movement at low frequencies
    float cutoffFreq = 300.0f;  // Hz
    float ellipticity = 0.5f;   // 0 = circular, 1 = fully elliptical
    
    void process(float& left, float& right, double sampleRate) {
        float fc = cutoffFreq / sampleRate;
        float alpha = std::sin(M_PI * fc) / 
                     (std::sin(M_PI * fc) + std::cos(M_PI * fc));
        
        // Progressive mono-ing below cutoff
        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        
        // Reduce side content at low frequencies
        side = highpass(side, cutoffFreq, sampleRate);
        side *= (1.0f - ellipticity);
        
        left = mid + side;
        right = mid - side;
    }
};
```

### 1.5 Phase Correlation Monitoring

```cpp
class PhaseCorrelationMeter {
    static constexpr int WINDOW_SIZE = 4096;
    CircularBuffer<float> leftBuffer;
    CircularBuffer<float> rightBuffer;
    
public:
    float calculateCorrelation() {
        float correlation = 0.0f;
        float leftPower = 0.0f;
        float rightPower = 0.0f;
        
        for (int i = 0; i < WINDOW_SIZE; ++i) {
            float l = leftBuffer.read(i);
            float r = rightBuffer.read(i);
            
            correlation += l * r;
            leftPower += l * l;
            rightPower += r * r;
        }
        
        float denominator = std::sqrt(leftPower * rightPower);
        if (denominator < 1e-6f) return 0.0f;
        
        return correlation / denominator;  // -1 to +1
    }
    
    std::string getStatus(float correlation) {
        if (correlation > 0.5f) return "Mono-compatible";
        if (correlation > 0.0f) return "Good stereo";
        if (correlation > -0.5f) return "Wide stereo";
        return "Phase issues - check mono compatibility!";
    }
};
```

## 2. HAAS EFFECT & PRECEDENCE

### 2.1 Psychoacoustic Principles

**Key Parameters:**
- Minimum detectable ITD: 10 microseconds (10° azimuth shift)
- Maximum natural ITD: ~650 microseconds (head width ~22cm)
- Minimum detectable ILD: 1dB difference
- Haas effect window: 1-30ms delay maintains precedence

### 2.2 Implementation

From the `DimensionExpander` engine:

```cpp
class HaasProcessor {
    static constexpr float MAX_DELAY_MS = 30.0f;
    DelayLine leftDelay;
    DelayLine rightDelay;
    
public:
    void process(float& left, float& right, float delayMs, float crossfeed) {
        // Apply Haas delay (1-30ms)
        float delayedLeft = leftDelay.read(delayMs);
        float delayedRight = rightDelay.read(delayMs);
        
        // Crossfeed for natural width
        float crossfeedGain = crossfeed * 0.3f;  // -10dB typical
        left += delayedRight * crossfeedGain;
        right += delayedLeft * crossfeedGain;
        
        // Update delay lines
        leftDelay.write(left);
        rightDelay.write(right);
    }
    
    void processMovement(float& left, float& right, float lfoPhase) {
        // Dynamic Haas effect for movement
        float delayL = 10.0f + 5.0f * std::sin(lfoPhase);
        float delayR = 10.0f + 5.0f * std::sin(lfoPhase + M_PI);
        
        left = leftDelay.read(delayL);
        right = rightDelay.read(delayR);
    }
};
```

### 2.3 Crossfeed Implementation (Bauer Method)

```cpp
class BauerCrossfeed {
    // Simulates natural acoustic crossfeed between ears
    struct Channel {
        DelayLine delay;
        ButterworthFilter lpf;
        float delayMs = 0.3f;      // Typical IRTD
        float attenuation = -4.5f;  // dB
        float cutoff = 700.0f;      // Hz (head shadow)
    };
    
    Channel leftToRight;
    Channel rightToLeft;
    
public:
    void process(float& left, float& right, double sampleRate) {
        // Each ear hears opposite channel delayed and filtered
        float l2r = leftToRight.delay.read(leftToRight.delayMs);
        l2r = leftToRight.lpf.processLowpass(l2r, leftToRight.cutoff, sampleRate);
        l2r *= dBToLinear(leftToRight.attenuation);
        
        float r2l = rightToLeft.delay.read(rightToLeft.delayMs);
        r2l = rightToLeft.lpf.processLowpass(r2l, rightToLeft.cutoff, sampleRate);
        r2l *= dBToLinear(rightToLeft.attenuation);
        
        // Mix crossfeed with direct signal
        float outputLeft = left + r2l;
        float outputRight = right + l2r;
        
        // Update delay lines
        leftToRight.delay.write(left);
        rightToLeft.delay.write(right);
        
        left = outputLeft;
        right = outputRight;
    }
};
```

### 2.4 Blumlein Shuffler

```cpp
class BlumleinShuffler {
    // Frequency-dependent stereo width adjustment
    AllpassFilter leftAP;
    AllpassFilter rightAP;
    
public:
    void process(float& left, float& right, float amount) {
        // Create frequency-dependent phase differences
        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        
        // Apply allpass to side signal
        side = leftAP.process(side) - rightAP.process(side);
        side *= amount;  // Width control
        
        // Reconstruct
        left = mid + side;
        right = mid - side;
    }
};
```

## 3. STEREO WIDENING

### 3.1 Phase Manipulation Techniques

**All-Pass Network Decorrelation:**
```cpp
class StereoWidener {
    struct AllpassChain {
        static constexpr int NUM_STAGES = 4;
        std::array<AllpassFilter, NUM_STAGES> filters;
        std::array<float, NUM_STAGES> coefficients = {
            0.731f, 0.633f, 0.547f, 0.451f  // Schroeder coefficients
        };
        
        float process(float input) {
            float output = input;
            for (int i = 0; i < NUM_STAGES; ++i) {
                output = filters[i].process(output, coefficients[i]);
            }
            return output;
        }
    };
    
    AllpassChain leftChain;
    AllpassChain rightChain;
    
public:
    void widen(float& left, float& right, float amount) {
        // Extract M/S
        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        
        // Decorrelate side signal
        float sideL = leftChain.process(side);
        float sideR = rightChain.process(side);
        
        // Mix decorrelated with original
        side = side * (1.0f - amount) + (sideL - sideR) * amount;
        
        // Reconstruct
        left = mid + side;
        right = mid - side;
    }
};
```

### 3.2 Comb Filter Approaches

```cpp
class CombWidener {
    DelayLine leftDelay;
    DelayLine rightDelay;
    
public:
    void process(float& left, float& right, float widthMs) {
        // Opposite delays for each channel
        float delayedLeft = leftDelay.read(widthMs);
        float delayedRight = rightDelay.read(widthMs);
        
        // Create width through comb filtering
        float outputLeft = left + delayedRight * 0.5f;
        float outputRight = right + delayedLeft * 0.5f;
        
        // Check mono compatibility
        float mono = (outputLeft + outputRight) * 0.5f;
        float originalMono = (left + right) * 0.5f;
        
        // Ensure mono signal is preserved
        if (std::abs(mono - originalMono) > 0.1f) {
            // Adjust to maintain mono compatibility
            float correction = originalMono - mono;
            outputLeft += correction;
            outputRight += correction;
        }
        
        left = outputLeft;
        right = outputRight;
    }
};
```

### 3.3 Frequency-Dependent Processing

```cpp
class MultibandWidener {
    struct Band {
        float lowFreq;
        float highFreq;
        float widthAmount;
        LinkwitzRileyFilter lpf;
        LinkwitzRileyFilter hpf;
    };
    
    std::array<Band, 3> bands = {{
        {0, 250, 0.0f},      // Bass - no widening
        {250, 2500, 0.5f},   // Mids - moderate widening
        {2500, 20000, 1.0f}  // Highs - full widening
    }};
    
public:
    void process(float& left, float& right, double sampleRate) {
        float outputLeft = 0.0f;
        float outputRight = 0.0f;
        
        for (auto& band : bands) {
            // Split into bands
            float bandL = band.hpf.process(
                band.lpf.process(left, band.highFreq, sampleRate),
                band.lowFreq, sampleRate);
            float bandR = band.hpf.process(
                band.lpf.process(right, band.highFreq, sampleRate),
                band.lowFreq, sampleRate);
            
            // Apply width per band
            applyWidth(bandL, bandR, band.widthAmount);
            
            outputLeft += bandL;
            outputRight += bandR;
        }
        
        left = outputLeft;
        right = outputRight;
    }
};
```

## 4. BINAURAL PROCESSING

### 4.1 HRTF Implementation

```cpp
class HRTFProcessor {
    struct HRTFData {
        static constexpr int IR_LENGTH = 512;
        std::array<float, IR_LENGTH> leftIR;
        std::array<float, IR_LENGTH> rightIR;
        float azimuth;
        float elevation;
        float distance;
    };
    
    std::vector<HRTFData> hrtfDatabase;
    ConvolutionEngine leftConv;
    ConvolutionEngine rightConv;
    
public:
    void positionSound(float input, float& left, float& right,
                      float azimuth, float elevation, float distance) {
        // Find nearest HRTF
        HRTFData* nearest = findNearestHRTF(azimuth, elevation);
        
        // Interpolate between neighboring HRTFs
        HRTFData interpolated = interpolateHRTF(azimuth, elevation);
        
        // Apply distance attenuation
        float distanceGain = 1.0f / (1.0f + distance);
        
        // Apply air absorption (high-frequency rolloff)
        float airAbsorption = std::exp(-0.01f * distance);
        
        // Convolve with HRTF
        left = leftConv.process(input * distanceGain, interpolated.leftIR);
        right = rightConv.process(input * distanceGain, interpolated.rightIR);
        
        // Apply air absorption filter
        left = applyAirAbsorption(left, airAbsorption);
        right = applyAirAbsorption(right, airAbsorption);
    }
    
private:
    HRTFData interpolateHRTF(float azimuth, float elevation) {
        // Find 4 nearest HRTFs for bilinear interpolation
        auto [h00, h01, h10, h11] = findFourNearest(azimuth, elevation);
        
        // Calculate interpolation weights
        float alpha = calculateAzimuthWeight(azimuth);
        float beta = calculateElevationWeight(elevation);
        
        HRTFData result;
        for (int i = 0; i < HRTFData::IR_LENGTH; ++i) {
            result.leftIR[i] = 
                h00.leftIR[i] * (1-alpha) * (1-beta) +
                h01.leftIR[i] * (1-alpha) * beta +
                h10.leftIR[i] * alpha * (1-beta) +
                h11.leftIR[i] * alpha * beta;
            
            result.rightIR[i] = 
                h00.rightIR[i] * (1-alpha) * (1-beta) +
                h01.rightIR[i] * (1-alpha) * beta +
                h10.rightIR[i] * alpha * (1-beta) +
                h11.rightIR[i] * alpha * beta;
        }
        
        return result;
    }
};
```

### 4.2 3D Positioning Algorithms

```cpp
class Spatializer3D {
    HRTFProcessor hrtf;
    ReverbProcessor reverb;
    
    struct Source {
        float x, y, z;
        float directGain;
        float reverbGain;
    };
    
public:
    void processSource(float input, float& left, float& right, 
                      const Source& source, const Listener& listener) {
        // Calculate relative position
        float dx = source.x - listener.x;
        float dy = source.y - listener.y;
        float dz = source.z - listener.z;
        
        // Convert to spherical coordinates
        float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        float azimuth = std::atan2(dx, dz);
        float elevation = std::asin(dy / distance);
        
        // Apply HRTF for direct sound
        float directL, directR;
        hrtf.positionSound(input, directL, directR, azimuth, elevation, distance);
        
        // Calculate reverb send based on distance
        float reverbSend = calculateReverbSend(distance);
        float reverbL, reverbR;
        reverb.process(input * reverbSend, reverbL, reverbR);
        
        // Mix direct and reverberant sound
        left = directL * source.directGain + reverbL * source.reverbGain;
        right = directR * source.directGain + reverbR * source.reverbGain;
    }
    
private:
    float calculateReverbSend(float distance) {
        // More reverb for distant sources
        return 1.0f - std::exp(-distance * 0.1f);
    }
};
```

### 4.3 Distance Modeling

```cpp
class DistanceModel {
    enum Model {
        InverseDistance,      // 1/r
        InverseDistanceClamped,  // 1/max(r, refDistance)
        LinearDistance,       // 1 - α(r - refDistance)
        ExponentialDistance   // (r/refDistance)^(-rolloff)
    };
    
    Model model = InverseDistanceClamped;
    float refDistance = 1.0f;
    float maxDistance = 100.0f;
    float rolloffFactor = 1.0f;
    
public:
    float calculateGain(float distance) {
        switch (model) {
            case InverseDistance:
                return refDistance / (refDistance + 
                       rolloffFactor * (distance - refDistance));
                
            case InverseDistanceClamped:
                distance = std::clamp(distance, refDistance, maxDistance);
                return refDistance / (refDistance + 
                       rolloffFactor * (distance - refDistance));
                
            case LinearDistance:
                distance = std::clamp(distance, refDistance, maxDistance);
                return 1.0f - rolloffFactor * 
                       (distance - refDistance) / (maxDistance - refDistance);
                
            case ExponentialDistance:
                distance = std::clamp(distance, refDistance, maxDistance);
                return std::pow(distance / refDistance, -rolloffFactor);
        }
        return 1.0f;
    }
    
    float calculateLowpassCutoff(float distance) {
        // Air absorption increases with distance
        float maxCutoff = 20000.0f;
        float minCutoff = 2000.0f;
        
        float normalized = (distance - refDistance) / (maxDistance - refDistance);
        normalized = std::clamp(normalized, 0.0f, 1.0f);
        
        return maxCutoff - (maxCutoff - minCutoff) * normalized;
    }
};
```

## 5. AMBISONIC PROCESSING

### 5.1 B-Format Encoding

```cpp
class AmbisonicEncoder {
    // First-order B-format: W, X, Y, Z
    struct BFormat {
        float w;  // Omnidirectional
        float x;  // Figure-8 front/back
        float y;  // Figure-8 left/right  
        float z;  // Figure-8 up/down
    };
    
public:
    BFormat encode(float signal, float azimuth, float elevation) {
        BFormat result;
        
        // Spherical harmonic encoding
        result.w = signal * (1.0f / std::sqrt(2.0f));  // Y00
        result.x = signal * std::cos(elevation) * std::cos(azimuth);  // Y11
        result.y = signal * std::cos(elevation) * std::sin(azimuth);  // Y1-1
        result.z = signal * std::sin(elevation);  // Y10
        
        return result;
    }
    
    // Higher-order encoding (HOA)
    std::vector<float> encodeHOA(float signal, float azimuth, 
                                 float elevation, int order) {
        int numChannels = (order + 1) * (order + 1);
        std::vector<float> result(numChannels);
        
        int channel = 0;
        for (int l = 0; l <= order; ++l) {
            for (int m = -l; m <= l; ++m) {
                result[channel++] = signal * 
                    sphericalHarmonic(l, m, azimuth, elevation);
            }
        }
        
        return result;
    }
    
private:
    float sphericalHarmonic(int l, int m, float azimuth, float elevation) {
        // Compute Y_l^m(θ, φ) using associated Legendre polynomials
        float plm = associatedLegendre(l, std::abs(m), std::sin(elevation));
        float normalization = std::sqrt((2*l + 1) * factorial(l - std::abs(m)) / 
                                      (4 * M_PI * factorial(l + std::abs(m))));
        
        if (m > 0) {
            return normalization * plm * std::cos(m * azimuth) * std::sqrt(2);
        } else if (m < 0) {
            return normalization * plm * std::sin(-m * azimuth) * std::sqrt(2);
        } else {
            return normalization * plm;
        }
    }
};
```

### 5.2 Ambisonic Decoding

```cpp
class AmbisonicDecoder {
    struct Speaker {
        float azimuth;
        float elevation;
        float gain;
    };
    
    std::vector<Speaker> speakerArray;
    
public:
    void decodeBFormat(const BFormat& bformat, std::vector<float>& speakers) {
        for (size_t i = 0; i < speakerArray.size(); ++i) {
            const Speaker& spk = speakerArray[i];
            
            // Basic decoding equation
            speakers[i] = bformat.w +
                         bformat.x * std::cos(spk.elevation) * std::cos(spk.azimuth) +
                         bformat.y * std::cos(spk.elevation) * std::sin(spk.azimuth) +
                         bformat.z * std::sin(spk.elevation);
            
            speakers[i] *= spk.gain;
        }
    }
    
    // Binaural decoding using HRTFs
    void decodeBinaural(const BFormat& bformat, float& left, float& right) {
        // Virtual speaker array for binaural rendering
        static constexpr int NUM_VIRTUAL_SPEAKERS = 8;
        std::array<float, NUM_VIRTUAL_SPEAKERS> virtualSpeakers;
        
        // Decode to virtual speakers
        for (int i = 0; i < NUM_VIRTUAL_SPEAKERS; ++i) {
            float azimuth = i * (2 * M_PI / NUM_VIRTUAL_SPEAKERS);
            
            virtualSpeakers[i] = bformat.w +
                                bformat.x * std::cos(azimuth) +
                                bformat.y * std::sin(azimuth);
        }
        
        // Apply HRTF to each virtual speaker
        left = 0.0f;
        right = 0.0f;
        
        for (int i = 0; i < NUM_VIRTUAL_SPEAKERS; ++i) {
            float azimuth = i * (2 * M_PI / NUM_VIRTUAL_SPEAKERS);
            float hrtfL, hrtfR;
            
            applyHRTF(virtualSpeakers[i], azimuth, 0, hrtfL, hrtfR);
            
            left += hrtfL;
            right += hrtfR;
        }
    }
};
```

## 6. PSYCHOACOUSTIC EFFECTS

### 6.1 Phantom Center Stability

```cpp
class PhantomCenterEnhancer {
    // Ensures stable center image in stereo playback
    
public:
    void process(float& left, float& right) {
        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        
        // Boost center slightly for clarity
        mid *= 1.05f;  // +0.4dB
        
        // Ensure precise level matching
        float leftLevel = calculateRMS(left);
        float rightLevel = calculateRMS(right);
        float imbalance = (leftLevel - rightLevel) / (leftLevel + rightLevel);
        
        // Compensate for imbalance
        if (std::abs(imbalance) > 0.01f) {  // 1% threshold
            float correction = 1.0f - imbalance;
            if (imbalance > 0) {
                right *= 1.0f + correction * 0.1f;
            } else {
                left *= 1.0f - correction * 0.1f;
            }
        }
        
        left = mid + side;
        right = mid - side;
    }
};
```

### 6.2 Depth Perception Cues

```cpp
class DepthProcessor {
    struct DepthCues {
        float directToReverbRatio;
        float highFrequencyContent;
        float dynamicRange;
        float spectralTilt;
    };
    
public:
    void addDepth(float& signal, float distance) {
        // Direct-to-reverberant ratio
        float direct = signal;
        float reverb = reverbProcessor.process(signal);
        float ratio = 1.0f / (1.0f + distance * 0.5f);
        signal = direct * ratio + reverb * (1.0f - ratio);
        
        // High-frequency air absorption
        float cutoff = 20000.0f * std::exp(-distance * 0.1f);
        signal = lowpassFilter.process(signal, cutoff);
        
        // Dynamic range compression for distance
        float threshold = 1.0f - distance * 0.3f;
        signal = softKneeCompressor.process(signal, threshold, 2.0f);
        
        // Spectral tilt
        float tilt = -3.0f * distance;  // dB/octave
        signal = spectralTiltFilter.process(signal, tilt);
    }
};
```

### 6.3 Envelopment Techniques

```cpp
class EnvelopmentProcessor {
    std::array<AllpassFilter, 8> decorrelators;
    std::array<float, 8> decorrelatorCoeffs = {
        0.773f, 0.802f, 0.753f, 0.733f,
        0.697f, 0.693f, 0.653f, 0.613f
    };
    
public:
    void createEnvelopment(float input, std::vector<float>& outputs) {
        // Generate decorrelated signals for multiple speakers
        for (size_t i = 0; i < outputs.size(); ++i) {
            // Each output gets differently decorrelated signal
            float decorrelated = decorrelators[i % 8].process(
                input, decorrelatorCoeffs[i % 8]);
            
            // Add subtle modulation for movement
            float modulation = 1.0f + 0.02f * std::sin(
                2 * M_PI * 0.1f * currentTime + i);
            
            outputs[i] = decorrelated * modulation;
        }
    }
    
    void processLateReflections(float input, float& left, float& right) {
        // Simulate late reflections for envelopment
        static constexpr int NUM_REFLECTIONS = 12;
        std::array<float, NUM_REFLECTIONS> delays = {
            23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71  // ms (primes)
        };
        
        float leftSum = 0.0f;
        float rightSum = 0.0f;
        
        for (int i = 0; i < NUM_REFLECTIONS; ++i) {
            float delayed = delayLine.read(delays[i]);
            float filtered = decorrelators[i % 8].process(
                delayed, decorrelatorCoeffs[i % 8]);
            
            // Pseudo-random panning
            float pan = ((i * 73) % 100) / 100.0f;  // 0 to 1
            leftSum += filtered * (1.0f - pan);
            rightSum += filtered * pan;
        }
        
        left = input + leftSum * 0.3f;   // 30% wet
        right = input + rightSum * 0.3f;
    }
};
```

## 7. KEY REFERENCES AND SOURCES

### Foundational Papers
1. **Alan Blumlein (1931)**: "Improvements in and relating to Sound-transmission" - UK Patent 394,325
2. **Benjamin Bauer (1961)**: "Stereophonic Earphones and Binaural Loudspeakers"
3. **Gardner & Martin (1994)**: "HRTF Measurements of a KEMAR Dummy-Head Microphone"
4. **Michael Gerzon (1970s)**: Various papers on Ambisonics and spatial audio
5. **Helmut Haas (1949)**: "The Influence of a Single Echo on the Audibility of Speech"

### Modern Research
1. **Jot (1999)**: "Real-time spatial processing of sounds for music, multimedia and interactive human-computer interfaces"
2. **Pulkki (2007)**: "Spatial Sound Reproduction with Directional Audio Coding"
3. **Zotter & Frank (2019)**: "Ambisonics: A Practical 3D Audio Theory for Recording, Studio Production, Sound Reinforcement, and Virtual Reality"

### Industry Standards
1. **ITU-R BS.775**: Multichannel stereophonic sound system with and without accompanying picture
2. **AES69-2015**: AES standard for file exchange - Spatial acoustic data file format (SOFA)
3. **ITU-R BS.2127**: Audio Definition Model (ADM) for object-based audio

### Modern Implementations
1. **Waves S1 Stereo Imager**: Professional stereo manipulation
2. **iZotope Ozone Imager**: Stereoize II technology
3. **Spatial Audio Framework (SAF)**: Open-source C library
4. **Facebook 360 Spatial Workstation**: VR/AR audio tools
5. **Apple Spatial Audio**: Hardware-accelerated 3D audio

## 8. PRODUCTION BEST PRACTICES

### Mono Compatibility Guidelines
1. Always check mix in mono
2. Keep bass frequencies centered
3. Avoid excessive anti-phase processing
4. Use correlation meters
5. Test on mono playback systems

### Phase Coherence
1. Maintain positive correlation above 0
2. Avoid excessive width on important elements
3. Use M/S processing carefully
4. Check phase at different frequencies
5. Consider broadcast requirements

### CPU Optimization
1. Use convolution efficiently (partitioned)
2. Implement SIMD where possible
3. Cache HRTF interpolations
4. Use LOD for distance sources
5. Optimize decorrelation filters

## CONCLUSION

Modern spatial audio processing requires careful balance between enhancement and compatibility. The Chimera Phoenix codebase demonstrates sophisticated implementations of these techniques, with particular attention to phase coherence and mono compatibility. The combination of traditional techniques (Blumlein, Haas) with modern processing (HRTF, Ambisonics) provides comprehensive spatial manipulation capabilities suitable for any production requirement.