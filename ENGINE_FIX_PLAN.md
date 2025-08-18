# Step-by-Step Plan to Fix 12 Problematic Engines

## Priority Order (Easiest to Hardest)

### PHASE 1: Quick Wins (Should take < 1 hour each)

---

## 1. GainUtility (ID 54) - HANGING
**Difficulty: TRIVIAL**
**Issue**: Simple gain adjustment hanging

### Steps to Fix:
```cpp
1. Check process() method for infinite loops
2. Likely issues:
   - Missing buffer size check
   - Incorrect loop termination
   - Denormal numbers causing CPU spike
   
3. Fix pattern:
   void process(AudioBuffer<float>& buffer) {
       DenormalGuard guard; // Add this
       
       // Check for valid buffer
       if (buffer.getNumSamples() == 0) return;
       
       // Simple gain with bounds checking
       for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
           float* data = buffer.getWritePointer(ch);
           for (int i = 0; i < buffer.getNumSamples(); ++i) {
               data[i] *= gain;
               // Add safety clamp
               data[i] = juce::jlimit(-1.0f, 1.0f, data[i]);
           }
       }
   }
```

---

## 2. MonoMaker (ID 55) - HANGING
**Difficulty: TRIVIAL**
**Issue**: Stereo summing causing hang

### Steps to Fix:
```cpp
1. Check channel count handling
2. Likely issues:
   - Not handling mono input
   - Incorrect channel indexing
   - Missing null checks

3. Fix pattern:
   void process(AudioBuffer<float>& buffer) {
       DenormalGuard guard;
       
       // Handle mono - just return
       if (buffer.getNumChannels() < 2) return;
       
       // Sum to mono
       float* left = buffer.getWritePointer(0);
       float* right = buffer.getWritePointer(1);
       
       for (int i = 0; i < buffer.getNumSamples(); ++i) {
           float mono = (left[i] + right[i]) * 0.5f;
           left[i] = mono;
           right[i] = mono;
       }
   }
```

---

### PHASE 2: Numerical Stability Fixes (1-2 hours each)

---

## 3. VintageOptoCompressor_Platinum (ID 2) - NaN
**Difficulty: MODERATE**
**Issue**: Division by zero in compression calculation

### Steps to Fix:
```cpp
1. Find division operations in compression ratio calculation
2. Common issues:
   - RMS calculation dividing by zero
   - Log of zero/negative in dB conversion
   - Ratio calculation with zero threshold

3. Fix pattern:
   // In gain calculation
   float inputLevel = std::max(1e-10f, rmsLevel); // Prevent zero
   float dB = 20.0f * std::log10(inputLevel);
   
   // In ratio calculation  
   float threshold = std::max(0.001f, m_threshold);
   if (dB > threshold) {
       float excess = dB - threshold;
       float compressedExcess = excess / std::max(1.0f, m_ratio);
       // ...
   }
```

---

## 4. KStyleOverdrive (ID 17) - NaN
**Difficulty: MODERATE**  
**Issue**: Uninitialized variables in distortion

### Steps to Fix:
```cpp
1. Check all member variables are initialized
2. Look for:
   - Uninitialized filter states
   - Missing reset() implementation
   - Tanh/atan of extreme values

3. Fix pattern:
   // In constructor
   KStyleOverdrive() : m_drive(1.0f), m_tone(0.5f), m_level(0.5f) {
       reset();
   }
   
   void reset() override {
       m_lowpass.reset();
       m_highpass.reset();
       m_prevSample = 0.0f;
       // Initialize ALL state variables
   }
   
   // In process - add safety
   float clipped = std::tanh(sample * drive);
   if (\!std::isfinite(clipped)) clipped = 0.0f;
```

---

## 5. DimensionExpander (ID 47) - NaN
**Difficulty: MODERATE**
**Issue**: NaN in spatial calculations

### Steps to Fix:
```cpp
1. Check Haas delay calculations
2. Look for:
   - Square root of negative numbers
   - Division in phase calculations
   - Uninitialized delay buffers

3. Fix pattern:
   // In spatial calculation
   float distance = std::max(0.0f, m_width * m_depth);
   float delay = std::sqrt(distance) * 0.001f;
   
   // In phase correlation
   float correlation = numerator / std::max(1e-10f, denominator);
   correlation = juce::jlimit(-1.0f, 1.0f, correlation);
```

---

## 6. PhaseAlign_Platinum (ID 53) - Infinity
**Difficulty: MODERATE**
**Issue**: Infinity in phase calculations

### Steps to Fix:
```cpp
1. Check FFT phase unwrapping
2. Look for:
   - atan2 with both args as zero
   - Division by magnitude in polar conversion
   - Accumulating phase without wrapping

3. Fix pattern:
   // In FFT processing
   float magnitude = std::sqrt(real*real + imag*imag);
   if (magnitude < 1e-10f) {
       phase = 0.0f;
   } else {
       phase = std::atan2(imag, real);
   }
   
   // Wrap phase to [-π, π]
   while (phase > M_PI) phase -= 2*M_PI;
   while (phase < -M_PI) phase += 2*M_PI;
```

---

### PHASE 3: Complex Algorithm Fixes (2-4 hours each)

---

## 7. SpectralFreeze (ID 49) - HANGING
**Difficulty: HARD**
**Issue**: STFT processing deadlock

### Steps to Fix:
```cpp
1. Check STFT buffer management
2. Common issues:
   - Circular buffer index overflow
   - Waiting for FFT size samples that never come
   - Not advancing hop size correctly

3. Fix pattern:
   // Add timeout and buffer size checks
   void processBlock(AudioBuffer<float>& buffer) {
       const int numSamples = buffer.getNumSamples();
       
       for (int i = 0; i < numSamples; ++i) {
           m_inputBuffer[m_writePos] = input[i];
           m_writePos = (m_writePos + 1) % m_bufferSize;
           
           m_sampleCounter++;
           if (m_sampleCounter >= m_hopSize) {
               m_sampleCounter = 0;
               if (m_freezeEnabled) {
                   // Use frozen spectrum
                   processFFT(m_frozenSpectrum);
               } else {
                   // Normal processing
                   performFFT();
               }
           }
       }
   }
```

---

## 8. GranularCloud (ID 50) - HANGING
**Difficulty: HARD**
**Issue**: Grain scheduling infinite loop

### Steps to Fix:
```cpp
1. Check grain triggering logic
2. Common issues:
   - Grain density causing infinite spawning
   - Grain position never advancing
   - Circular buffer wraparound errors

3. Fix pattern:
   void scheduleGrains() {
       const int MAX_GRAINS = 128; // Hard limit
       int grainCount = 0;
       
       while (shouldSpawnGrain() && grainCount < MAX_GRAINS) {
           if (m_activeGrains.size() >= MAX_GRAINS) {
               removeOldestGrain();
           }
           
           spawnGrain();
           grainCount++;
           
           // Advance time
           m_nextGrainTime += m_grainInterval;
           if (m_nextGrainTime > m_bufferLength) {
               break; // Prevent infinite loop
           }
       }
   }
```

---

## 9. ChaosGenerator (ID 51) - HANGING
**Difficulty: HARD**
**Issue**: Lorenz attractor numerical explosion

### Steps to Fix:
```cpp
1. Check Lorenz equation implementation
2. Issues:
   - Time step too large causing instability
   - No bounds checking on attractor values
   - Accumulation without normalization

3. Fix pattern:
   void updateLorenz() {
       const float dt = 0.001f; // Small time step
       const float sigma = 10.0f;
       const float rho = 28.0f;
       const float beta = 8.0f/3.0f;
       
       // Lorenz equations with bounds
       float dx = sigma * (y - x) * dt;
       float dy = (x * (rho - z) - y) * dt;
       float dz = (x * y - beta * z) * dt;
       
       x += dx; y += dy; z += dz;
       
       // Prevent explosion
       const float MAX_VAL = 50.0f;
       x = juce::jlimit(-MAX_VAL, MAX_VAL, x);
       y = juce::jlimit(-MAX_VAL, MAX_VAL, y);
       z = juce::jlimit(-MAX_VAL, MAX_VAL, z);
       
       // Reset if NaN
       if (\!std::isfinite(x) || \!std::isfinite(y) || \!std::isfinite(z)) {
           x = 1.0f; y = 1.0f; z = 1.0f;
       }
   }
```

---

## 10. FeedbackNetwork (ID 52) - HANGING
**Difficulty: HARD**
**Issue**: Unstable feedback causing runaway

### Steps to Fix:
```cpp
1. Check feedback matrix stability
2. Issues:
   - Feedback gain > 1 causing explosion
   - No damping in feedback path
   - Matrix eigenvalues > 1

3. Fix pattern:
   void processFeedbackMatrix() {
       // Ensure stability
       float maxFeedback = 0.95f; // Never allow >= 1
       
       // Process with damping
       for (int i = 0; i < NUM_NODES; ++i) {
           float sum = 0.0f;
           for (int j = 0; j < NUM_NODES; ++j) {
               sum += m_nodeOutputs[j] * m_feedbackMatrix[i][j];
           }
           
           // Apply damping and limiting
           sum *= maxFeedback;
           sum = std::tanh(sum); // Soft limiting
           
           // Leak DC
           m_nodeStates[i] = sum * 0.995f;
           
           // Safety check
           if (\!std::isfinite(m_nodeStates[i])) {
               m_nodeStates[i] = 0.0f;
           }
       }
   }
```

---

## Testing Strategy After Each Fix

### 1. Unit Test Each Engine
```bash
# Create simple test for each engine
./test_single_engine [ENGINE_ID]
```

### 2. Check for Common Issues
- ✅ No NaN/Inf in output
- ✅ CPU usage < 100%
- ✅ Processes silence without issues
- ✅ Handles extreme parameter values
- ✅ reset() properly initializes state

### 3. Integration Test
```bash
# Run comprehensive test after fixing each engine
./comprehensive_engine_audit
```

---

## Summary Fix Order

### Day 1 (Quick Wins - 2 hours)
1. ✅ GainUtility
2. ✅ MonoMaker

### Day 2 (Numerical Fixes - 4 hours)
3. ✅ VintageOptoCompressor_Platinum
4. ✅ KStyleOverdrive  
5. ✅ DimensionExpander
6. ✅ PhaseAlign_Platinum

### Day 3-4 (Complex Algorithms - 8-12 hours)
7. ✅ SpectralFreeze
8. ✅ GranularCloud
9. ✅ ChaosGenerator
10. ✅ FeedbackNetwork

### Note on SpringReverb_Platinum
The test shows this in the NaN list, but we already added DenormalGuard today. This might already be fixed - test it first before making changes.

## Expected Result
After these fixes: **57/57 engines working (100%)**

## Common Fix Patterns Used
1. **DenormalGuard** - Prevents CPU spikes from tiny numbers
2. **Bounds checking** - juce::jlimit() for all parameters
3. **Division safety** - std::max(epsilon, divisor)
4. **NaN checking** - std::isfinite() with fallback values
5. **Loop limits** - MAX_ITERATIONS to prevent infinite loops
6. **Buffer validation** - Check sizes and channel counts
7. **State initialization** - Proper reset() implementation
EOF < /dev/null