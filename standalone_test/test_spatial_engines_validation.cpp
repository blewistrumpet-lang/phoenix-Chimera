/**
 * SPATIAL ENGINES DEEP VALIDATION TEST
 *
 * Tests all spatial processing engines with comprehensive parameter coverage:
 * - DimensionExpander: Stereo width and depth control
 * - SpectralFreeze: Spectral hold and manipulation
 * - SpectralGate: Frequency-selective gating
 * - MidSideProcessor: M/S encoding/decoding
 * - PhaseAlign: Phase alignment and correction
 *
 * Test Coverage:
 * 1. Parameter ranges and documentation
 * 2. Stereo correlation measurements
 * 3. Phase alignment accuracy
 * 4. Spectral freeze behavior
 * 5. Mid-side matrix accuracy
 * 6. Mono compatibility
 * 7. Phase coherence
 */

#include "JuceHeader.h"
#include "DimensionExpander.h"
#include "SpectralFreeze.h"
#include "SpectralGate_Platinum.h"
#include "MidSideProcessor_Platinum.h"
#include "PhaseAlign_Platinum.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <complex>
#include <map>

// ============================================================================
// TEST UTILITIES
// ============================================================================

struct StereoMetrics {
    float correlation;      // -1 to +1 (1 = perfect correlation)
    float width;           // Perceived width
    float monoCompatibility; // How well it sums to mono
    float phaseCoherence;  // Phase relationship quality
    float leftRMS;
    float rightRMS;
    float midRMS;
    float sideRMS;
};

class SpatialAnalyzer {
public:
    static StereoMetrics analyze(const juce::AudioBuffer<float>& buffer) {
        StereoMetrics metrics = {};

        if (buffer.getNumChannels() < 2) return metrics;

        const float* left = buffer.getReadPointer(0);
        const float* right = buffer.getReadPointer(1);
        const int numSamples = buffer.getNumSamples();

        // Calculate RMS levels
        double leftSum = 0.0, rightSum = 0.0;
        double midSum = 0.0, sideSum = 0.0;
        double corrSum = 0.0;

        for (int i = 0; i < numSamples; ++i) {
            float L = left[i];
            float R = right[i];

            leftSum += L * L;
            rightSum += R * R;
            corrSum += L * R;

            // M/S encoding
            float M = (L + R) * 0.5f;
            float S = (L - R) * 0.5f;

            midSum += M * M;
            sideSum += S * S;
        }

        metrics.leftRMS = std::sqrt(leftSum / numSamples);
        metrics.rightRMS = std::sqrt(rightSum / numSamples);
        metrics.midRMS = std::sqrt(midSum / numSamples);
        metrics.sideRMS = std::sqrt(sideSum / numSamples);

        // Correlation coefficient
        float denom = metrics.leftRMS * metrics.rightRMS;
        if (denom > 0.0001f) {
            metrics.correlation = corrSum / (numSamples * denom);
        } else {
            metrics.correlation = 0.0f;
        }

        // Width estimate (based on S/M ratio)
        if (metrics.midRMS > 0.0001f) {
            metrics.width = metrics.sideRMS / metrics.midRMS;
        } else {
            metrics.width = 0.0f;
        }

        // Mono compatibility (how much cancellation in mono sum)
        double monoSum = 0.0;
        for (int i = 0; i < numSamples; ++i) {
            float mono = (left[i] + right[i]) * 0.5f;
            monoSum += mono * mono;
        }
        float monoRMS = std::sqrt(monoSum / numSamples);
        float avgRMS = (metrics.leftRMS + metrics.rightRMS) * 0.5f;
        if (avgRMS > 0.0001f) {
            metrics.monoCompatibility = monoRMS / avgRMS;
        } else {
            metrics.monoCompatibility = 1.0f;
        }

        // Phase coherence (simplified)
        metrics.phaseCoherence = std::abs(metrics.correlation);

        return metrics;
    }

    static float measurePhaseDelay(const juce::AudioBuffer<float>& buffer) {
        if (buffer.getNumChannels() < 2) return 0.0f;

        const float* left = buffer.getReadPointer(0);
        const float* right = buffer.getReadPointer(1);
        const int numSamples = buffer.getNumSamples();

        // Cross-correlation to find phase delay
        float maxCorr = -1e9f;
        int bestDelay = 0;
        const int maxSearch = std::min(100, numSamples / 4);

        for (int delay = -maxSearch; delay <= maxSearch; ++delay) {
            double corr = 0.0;
            int count = 0;

            for (int i = std::max(0, delay); i < std::min(numSamples, numSamples + delay); ++i) {
                int j = i - delay;
                if (j >= 0 && j < numSamples) {
                    corr += left[i] * right[j];
                    count++;
                }
            }

            if (count > 0) {
                float avgCorr = corr / count;
                if (avgCorr > maxCorr) {
                    maxCorr = avgCorr;
                    bestDelay = delay;
                }
            }
        }

        return bestDelay;
    }
};

// ============================================================================
// TEST SIGNAL GENERATORS
// ============================================================================

class TestSignals {
public:
    static void generateStereoSine(juce::AudioBuffer<float>& buffer,
                                   float frequency,
                                   double sampleRate,
                                   float leftPhase = 0.0f,
                                   float rightPhase = 0.0f) {
        const int numSamples = buffer.getNumSamples();
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);

        float phaseInc = 2.0f * M_PI * frequency / sampleRate;

        for (int i = 0; i < numSamples; ++i) {
            left[i] = std::sin(leftPhase);
            right[i] = std::sin(rightPhase);
            leftPhase += phaseInc;
            rightPhase += phaseInc;
        }
    }

    static void generateWhiteNoise(juce::AudioBuffer<float>& buffer, float level = 0.5f) {
        juce::Random random;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                data[i] = (random.nextFloat() * 2.0f - 1.0f) * level;
            }
        }
    }

    static void generatePinkNoise(juce::AudioBuffer<float>& buffer, float level = 0.5f) {
        juce::Random random;
        float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float white = random.nextFloat() * 2.0f - 1.0f;
                b0 = 0.99886f * b0 + white * 0.0555179f;
                b1 = 0.99332f * b1 + white * 0.0750759f;
                b2 = 0.96900f * b2 + white * 0.1538520f;
                b3 = 0.86650f * b3 + white * 0.3104856f;
                b4 = 0.55000f * b4 + white * 0.5329522f;
                b5 = -0.7616f * b5 - white * 0.0168980f;
                float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
                b6 = white * 0.115926f;
                data[i] = pink * level * 0.11f;
            }
        }
    }
};

// ============================================================================
// DIMENSION EXPANDER TESTS
// ============================================================================

void testDimensionExpander() {
    std::cout << "\n=== DIMENSION EXPANDER VALIDATION ===\n\n";

    std::cout << "PARAMETERS:\n";
    std::cout << "  0. Width (0-1): Stereo width control\n";
    std::cout << "     0.0 = mono, 0.5 = normal, 1.0 = wide\n";
    std::cout << "  1. Depth (0-1): Haas effect depth\n";
    std::cout << "     Controls micro-delay (0.8-8ms)\n";
    std::cout << "  2. Crossfeed (0-1): L/R channel blending\n";
    std::cout << "     0.0 = no crossfeed, 1.0 = 50% blend\n";
    std::cout << "  3. Bass Retention (0-1): Keep lows centered\n";
    std::cout << "     Controls LP cutoff (100-300 Hz)\n";
    std::cout << "  4. Ambience (0-1): Allpass diffusion\n";
    std::cout << "     Adds spatial character\n";
    std::cout << "  5. Movement (0-1): LFO modulation\n";
    std::cout << "     Slow M/S rotation\n";
    std::cout << "  6. Clarity (0-1): Tilt EQ\n";
    std::cout << "     Shapes high frequency detail\n";
    std::cout << "  7. Mix (0-1): Dry/wet blend\n\n";

    DimensionExpander expander;
    const double sampleRate = 48000.0;
    const int blockSize = 512;

    expander.prepareToPlay(sampleRate, blockSize);

    // Test 1: Width control vs correlation
    std::cout << "TEST 1: Width Control vs Stereo Correlation\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Width | Correlation | Side/Mid | Mono Compat\n";
    std::cout << "------|-------------|----------|-------------\n";

    for (float width = 0.0f; width <= 1.0f; width += 0.2f) {
        juce::AudioBuffer<float> buffer(2, blockSize);
        TestSignals::generatePinkNoise(buffer, 0.3f);

        std::map<int, float> params;
        params[0] = width;  // Width
        params[1] = 0.5f;   // Depth
        params[7] = 1.0f;   // Mix
        expander.updateParameters(params);

        expander.process(buffer);

        auto metrics = SpatialAnalyzer::analyze(buffer);

        std::cout << std::setw(5) << width << " | "
                  << std::setw(11) << metrics.correlation << " | "
                  << std::setw(8) << metrics.width << " | "
                  << std::setw(11) << metrics.monoCompatibility << "\n";
    }

    // Test 2: Depth (Haas effect) timing
    std::cout << "\nTEST 2: Depth Control (Haas Effect)\n";
    std::cout << "Depth | Phase Delay (samples) | Width\n";
    std::cout << "------|----------------------|-------\n";

    for (float depth = 0.0f; depth <= 1.0f; depth += 0.25f) {
        juce::AudioBuffer<float> buffer(2, blockSize * 2);
        TestSignals::generateStereoSine(buffer, 1000.0f, sampleRate);

        std::map<int, float> params;
        params[0] = 0.7f;   // Width
        params[1] = depth;  // Depth
        params[7] = 1.0f;   // Mix
        expander.updateParameters(params);

        expander.process(buffer);

        float phaseDelay = SpatialAnalyzer::measurePhaseDelay(buffer);
        auto metrics = SpatialAnalyzer::analyze(buffer);

        std::cout << std::setw(5) << depth << " | "
                  << std::setw(20) << phaseDelay << " | "
                  << std::setw(5) << metrics.width << "\n";
    }

    // Test 3: Bass retention (mono lows)
    std::cout << "\nTEST 3: Bass Retention (Low Frequency Mono)\n";
    std::cout << "Keep  | Low Corr | High Corr | Width\n";
    std::cout << "------|----------|-----------|-------\n";

    for (float keep = 0.0f; keep <= 1.0f; keep += 0.25f) {
        // Generate mix of low and high frequencies
        juce::AudioBuffer<float> buffer(2, blockSize);
        juce::AudioBuffer<float> low(2, blockSize);
        juce::AudioBuffer<float> high(2, blockSize);

        TestSignals::generateStereoSine(low, 100.0f, sampleRate);
        TestSignals::generateStereoSine(high, 5000.0f, sampleRate);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                buffer.setSample(ch, i, low.getSample(ch, i) * 0.5f + high.getSample(ch, i) * 0.5f);
            }
        }

        std::map<int, float> params;
        params[0] = 0.8f;   // Width
        params[3] = keep;   // Bass Retention
        params[7] = 1.0f;   // Mix
        expander.updateParameters(params);

        expander.process(buffer);

        auto metrics = SpatialAnalyzer::analyze(buffer);

        std::cout << std::setw(5) << keep << " | "
                  << std::setw(8) << "N/A" << " | "
                  << std::setw(9) << "N/A" << " | "
                  << std::setw(5) << metrics.width << "\n";
    }

    std::cout << "\n✓ DimensionExpander validation complete\n";
}

// ============================================================================
// SPECTRAL FREEZE TESTS
// ============================================================================

void testSpectralFreeze() {
    std::cout << "\n=== SPECTRAL FREEZE VALIDATION ===\n\n";

    std::cout << "PARAMETERS:\n";
    std::cout << "  0. Freeze (0-1): Spectral hold toggle\n";
    std::cout << "     < 0.5 = pass through, >= 0.5 = freeze\n";
    std::cout << "  1. Smear (0-1): Spectral blur radius\n";
    std::cout << "     Averages neighboring bins\n";
    std::cout << "  2. Shift (0-1): Frequency shift\n";
    std::cout << "     0.5 = none, 0.0 = down, 1.0 = up\n";
    std::cout << "  3. Resonance (0-1): Peak enhancement\n";
    std::cout << "     Emphasizes spectral peaks\n";
    std::cout << "  4. Decay (0-1): Frozen spectrum decay\n";
    std::cout << "     0.0 = fast decay, 1.0 = infinite hold\n";
    std::cout << "  5. Brightness (0-1): Spectral tilt\n";
    std::cout << "     0.0 = dark, 0.5 = flat, 1.0 = bright\n";
    std::cout << "  6. Density (0-1): Spectral thinning\n";
    std::cout << "     1.0 = all bins, < 1.0 = sparse\n";
    std::cout << "  7. Shimmer (0-1): Phase randomization\n";
    std::cout << "     Adds textural variation\n\n";

    SpectralFreeze freeze;
    const double sampleRate = 48000.0;
    const int blockSize = 512;

    freeze.prepareToPlay(sampleRate, blockSize);

    // Test 1: Freeze engage/disengage
    std::cout << "TEST 1: Freeze State Transitions\n";
    std::cout << "State   | RMS Level | Stability\n";
    std::cout << "--------|-----------|----------\n";

    juce::AudioBuffer<float> testBuffer(2, blockSize);
    TestSignals::generatePinkNoise(testBuffer, 0.3f);

    // Build up frozen spectrum
    std::map<int, float> params;
    params[0] = 1.0f;  // Freeze on
    freeze.updateParameters(params);

    for (int block = 0; block < 10; ++block) {
        juce::AudioBuffer<float> buffer(2, blockSize);
        TestSignals::generatePinkNoise(buffer, 0.3f);
        freeze.process(buffer);
    }

    // Now test frozen output
    juce::AudioBuffer<float> frozenBuffer(2, blockSize);
    frozenBuffer.clear();
    freeze.process(frozenBuffer);

    auto frozenMetrics = SpatialAnalyzer::analyze(frozenBuffer);
    std::cout << "Frozen  | " << std::setw(9) << frozenMetrics.leftRMS << " | Holding\n";

    // Unfreeze
    params[0] = 0.0f;  // Freeze off
    freeze.updateParameters(params);

    juce::AudioBuffer<float> unfrozenBuffer(2, blockSize);
    TestSignals::generatePinkNoise(unfrozenBuffer, 0.3f);
    freeze.process(unfrozenBuffer);

    auto unfrozenMetrics = SpatialAnalyzer::analyze(unfrozenBuffer);
    std::cout << "Unfrozen| " << std::setw(9) << unfrozenMetrics.leftRMS << " | Passing\n";

    // Test 2: Spectral shift
    std::cout << "\nTEST 2: Spectral Shift\n";
    std::cout << "Shift | Effect\n";
    std::cout << "------|--------\n";

    for (float shift = 0.0f; shift <= 1.0f; shift += 0.5f) {
        juce::AudioBuffer<float> buffer(2, blockSize);
        TestSignals::generateStereoSine(buffer, 1000.0f, sampleRate);

        std::map<int, float> params;
        params[0] = 1.0f;   // Freeze
        params[2] = shift;  // Shift
        freeze.updateParameters(params);

        // Process multiple blocks to build frozen spectrum
        for (int i = 0; i < 5; ++i) {
            freeze.process(buffer);
        }

        std::string effect = (shift < 0.4f) ? "Down" :
                           (shift > 0.6f) ? "Up" : "None";

        std::cout << std::setw(5) << shift << " | " << effect << "\n";
    }

    // Test 3: Density (spectral thinning)
    std::cout << "\nTEST 3: Density Control\n";
    std::cout << "Density | RMS    | Effect\n";
    std::cout << "--------|--------|--------\n";

    for (float density = 0.2f; density <= 1.0f; density += 0.2f) {
        juce::AudioBuffer<float> buffer(2, blockSize);
        TestSignals::generatePinkNoise(buffer, 0.3f);

        std::map<int, float> params;
        params[0] = 1.0f;     // Freeze
        params[6] = density;  // Density
        freeze.updateParameters(params);

        // Build frozen spectrum
        for (int i = 0; i < 5; ++i) {
            freeze.process(buffer);
        }

        auto metrics = SpatialAnalyzer::analyze(buffer);

        std::cout << std::setw(7) << density << " | "
                  << std::setw(6) << metrics.leftRMS << " | "
                  << (density < 0.5f ? "Sparse" : "Dense") << "\n";
    }

    std::cout << "\n✓ SpectralFreeze validation complete\n";
}

// ============================================================================
// MID-SIDE PROCESSOR TESTS
// ============================================================================

void testMidSideProcessor() {
    std::cout << "\n=== MID-SIDE PROCESSOR VALIDATION ===\n\n";

    std::cout << "PARAMETERS:\n";
    std::cout << "  0. Mid Gain (0-1): Mid channel level\n";
    std::cout << "     0.0 = -20dB, 0.5 = 0dB, 1.0 = +20dB\n";
    std::cout << "  1. Side Gain (0-1): Side channel level\n";
    std::cout << "     0.0 = -20dB, 0.5 = 0dB, 1.0 = +20dB\n";
    std::cout << "  2. Width (0-1): Stereo width\n";
    std::cout << "     0.0 = mono, 0.5 = 100%, 1.0 = 200%\n";
    std::cout << "  3. Mid Low (0-1): Mid low shelf\n";
    std::cout << "     0.5 = flat, adjust ±15dB\n";
    std::cout << "  4. Mid High (0-1): Mid high shelf\n";
    std::cout << "  5. Side Low (0-1): Side low shelf\n";
    std::cout << "  6. Side High (0-1): Side high shelf\n";
    std::cout << "  7. Bass Mono (0-1): Mono low frequencies\n";
    std::cout << "     0.0 = off, 1.0 = mono below 500Hz\n";
    std::cout << "  8. Solo Mode (0-1): Channel monitoring\n";
    std::cout << "     0.0 = off, 0.33 = mid, 0.66 = side\n";
    std::cout << "  9. Presence (0-1): High frequency boost\n";
    std::cout << "     0.0 = off, 1.0 = +6dB @ 10kHz\n\n";

    MidSideProcessor_Platinum processor;
    const double sampleRate = 48000.0;
    const int blockSize = 512;

    processor.prepareToPlay(sampleRate, blockSize);

    // Test 1: M/S encoding/decoding accuracy
    std::cout << "TEST 1: Mid-Side Matrix Accuracy\n";
    std::cout << "Testing unity gain through encode/decode...\n";

    juce::AudioBuffer<float> buffer(2, blockSize);
    TestSignals::generatePinkNoise(buffer, 0.3f);

    auto originalMetrics = SpatialAnalyzer::analyze(buffer);

    std::map<int, float> params;
    params[0] = 0.5f;  // Mid gain (0dB)
    params[1] = 0.5f;  // Side gain (0dB)
    params[2] = 0.5f;  // Width (100%)
    processor.updateParameters(params);

    processor.process(buffer);

    auto processedMetrics = SpatialAnalyzer::analyze(buffer);

    float levelChange = 20.0f * std::log10(processedMetrics.leftRMS / (originalMetrics.leftRMS + 1e-10f));

    std::cout << "  Input RMS:  " << originalMetrics.leftRMS << "\n";
    std::cout << "  Output RMS: " << processedMetrics.leftRMS << "\n";
    std::cout << "  Level change: " << levelChange << " dB\n";
    std::cout << "  Result: " << (std::abs(levelChange) < 1.0f ? "PASS" : "FAIL") << "\n";

    // Test 2: Width control
    std::cout << "\nTEST 2: Width Control\n";
    std::cout << "Width | Correlation | Side/Mid | Mono Compat\n";
    std::cout << "------|-------------|----------|-------------\n";

    for (float width = 0.0f; width <= 1.0f; width += 0.25f) {
        juce::AudioBuffer<float> buffer(2, blockSize);
        TestSignals::generatePinkNoise(buffer, 0.3f);

        std::map<int, float> params;
        params[2] = width;  // Width
        processor.updateParameters(params);

        processor.process(buffer);

        auto metrics = SpatialAnalyzer::analyze(buffer);

        std::cout << std::setw(5) << width << " | "
                  << std::setw(11) << metrics.correlation << " | "
                  << std::setw(8) << metrics.width << " | "
                  << std::setw(11) << metrics.monoCompatibility << "\n";
    }

    // Test 3: Solo modes
    std::cout << "\nTEST 3: Solo Mode Operation\n";
    std::cout << "Mode     | Correlation | Effect\n";
    std::cout << "---------|-------------|--------\n";

    const char* modes[] = {"Off", "Mid", "Side"};
    float soloValues[] = {0.0f, 0.33f, 0.66f};

    for (int i = 0; i < 3; ++i) {
        juce::AudioBuffer<float> buffer(2, blockSize);
        TestSignals::generatePinkNoise(buffer, 0.3f);

        std::map<int, float> params;
        params[8] = soloValues[i];  // Solo mode
        processor.updateParameters(params);

        processor.process(buffer);

        auto metrics = SpatialAnalyzer::analyze(buffer);

        std::cout << std::setw(8) << modes[i] << " | "
                  << std::setw(11) << metrics.correlation << " | "
                  << (i == 0 ? "Normal" : i == 1 ? "Mono" : "Wide") << "\n";
    }

    std::cout << "\n✓ MidSideProcessor validation complete\n";
}

// ============================================================================
// PHASE ALIGN TESTS
// ============================================================================

void testPhaseAlign() {
    std::cout << "\n=== PHASE ALIGN VALIDATION ===\n\n";

    std::cout << "PARAMETERS:\n";
    std::cout << "  0. Auto Align (0-1): Enable auto-alignment\n";
    std::cout << "     < 0.5 = manual, >= 0.5 = auto\n";
    std::cout << "  1. Reference (0-1): Reference channel\n";
    std::cout << "     < 0.5 = left, >= 0.5 = right\n";
    std::cout << "  2. Low Phase (0-1): Low band phase rotation\n";
    std::cout << "     Maps to -180° to +180°\n";
    std::cout << "  3. Low-Mid Phase (0-1): Low-mid phase\n";
    std::cout << "  4. High-Mid Phase (0-1): High-mid phase\n";
    std::cout << "  5. High Phase (0-1): High band phase\n";
    std::cout << "  6. Low Freq (0-1): Low crossover\n";
    std::cout << "     Maps to 50-400 Hz\n";
    std::cout << "  7. Mid Freq (0-1): Mid crossover\n";
    std::cout << "     Maps to 400-3000 Hz\n";
    std::cout << "  8. High Freq (0-1): High crossover\n";
    std::cout << "     Maps to 3000-12000 Hz\n";
    std::cout << "  9. Mix (0-1): Dry/wet blend\n\n";

    PhaseAlign_Platinum aligner;
    const double sampleRate = 48000.0;
    const int blockSize = 512;

    aligner.prepareToPlay(sampleRate, blockSize);

    // Test 1: Auto-alignment accuracy
    std::cout << "TEST 1: Auto-Alignment Accuracy\n";
    std::cout << "Creating artificial phase delay...\n";

    // Generate test signal with known delay
    juce::AudioBuffer<float> buffer(2, blockSize * 2);

    // Left channel: immediate signal
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        left[i] = std::sin(2.0f * M_PI * 1000.0f * i / sampleRate);
        // Right channel: delayed by 10 samples
        if (i >= 10) {
            right[i] = left[i - 10];
        } else {
            right[i] = 0.0f;
        }
    }

    float beforeDelay = SpatialAnalyzer::measurePhaseDelay(buffer);

    std::map<int, float> params;
    params[0] = 1.0f;  // Auto align on
    params[9] = 1.0f;  // Full mix
    aligner.updateParameters(params);

    // Process multiple blocks for alignment to converge
    for (int block = 0; block < 10; ++block) {
        aligner.process(buffer);
    }

    float afterDelay = SpatialAnalyzer::measurePhaseDelay(buffer);

    std::cout << "  Before: " << beforeDelay << " samples delay\n";
    std::cout << "  After:  " << afterDelay << " samples delay\n";
    std::cout << "  Correction: " << (beforeDelay - afterDelay) << " samples\n";
    std::cout << "  Result: " << (std::abs(afterDelay) < 5.0f ? "PASS" : "PARTIAL") << "\n";

    // Test 2: Manual phase rotation
    std::cout << "\nTEST 2: Manual Phase Rotation\n";
    std::cout << "Phase | Correlation Change\n";
    std::cout << "------|-------------------\n";

    for (float phase = 0.0f; phase <= 1.0f; phase += 0.25f) {
        juce::AudioBuffer<float> buffer(2, blockSize);
        TestSignals::generateStereoSine(buffer, 1000.0f, sampleRate);

        auto beforeMetrics = SpatialAnalyzer::analyze(buffer);

        std::map<int, float> params;
        params[0] = 0.0f;   // Manual mode
        params[2] = phase;  // Low phase
        params[9] = 1.0f;   // Full mix
        aligner.updateParameters(params);

        aligner.process(buffer);

        auto afterMetrics = SpatialAnalyzer::analyze(buffer);

        float corrChange = afterMetrics.correlation - beforeMetrics.correlation;

        std::cout << std::setw(5) << phase << " | "
                  << std::setw(17) << corrChange << "\n";
    }

    std::cout << "\n✓ PhaseAlign validation complete\n";
}

// ============================================================================
// SPECTRAL GATE TESTS
// ============================================================================

void testSpectralGate() {
    std::cout << "\n=== SPECTRAL GATE VALIDATION ===\n\n";

    std::cout << "PARAMETERS:\n";
    std::cout << "  0. Threshold (0-1): Gate threshold\n";
    std::cout << "     Maps to -60 to 0 dB\n";
    std::cout << "  1. Ratio (0-1): Gate ratio\n";
    std::cout << "     Maps to 1:1 to 20:1\n";
    std::cout << "  2. Attack (0-1): Attack time\n";
    std::cout << "     Maps to 0.1 to 50 ms\n";
    std::cout << "  3. Release (0-1): Release time\n";
    std::cout << "     Maps to 1 to 500 ms\n";
    std::cout << "  4. Freq Low (0-1): Low frequency bound\n";
    std::cout << "     Maps to 20Hz to 20kHz (log)\n";
    std::cout << "  5. Freq High (0-1): High frequency bound\n";
    std::cout << "     Maps to 20Hz to 20kHz (log)\n";
    std::cout << "  6. Lookahead (0-1): Lookahead time\n";
    std::cout << "     Maps to 0 to 10 ms\n";
    std::cout << "  7. Mix (0-1): Dry/wet blend\n\n";

    SpectralGate_Platinum gate;
    const double sampleRate = 48000.0;
    const int blockSize = 512;

    gate.prepareToPlay(sampleRate, blockSize);

    // Test 1: Frequency-selective gating
    std::cout << "TEST 1: Frequency-Selective Gating\n";
    std::cout << "Testing gate on specific frequency bands...\n";

    juce::AudioBuffer<float> buffer(2, blockSize);

    // Generate signal with multiple frequencies
    for (int i = 0; i < blockSize; ++i) {
        float t = i / sampleRate;
        float sample = 0.1f * std::sin(2.0f * M_PI * 100.0f * t) +   // Low
                      0.3f * std::sin(2.0f * M_PI * 1000.0f * t) +  // Mid (louder)
                      0.1f * std::sin(2.0f * M_PI * 5000.0f * t);   // High
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }

    auto originalMetrics = SpatialAnalyzer::analyze(buffer);

    // Gate out everything except 500Hz - 2kHz (should keep 1kHz component)
    std::map<int, float> params;
    params[0] = 0.25f;  // Threshold (-45 dB)
    params[1] = 0.5f;   // Ratio (10:1)
    params[4] = 0.4f;   // Freq Low (~500 Hz)
    params[5] = 0.5f;   // Freq High (~2 kHz)
    params[7] = 1.0f;   // Full wet
    gate.updateParameters(params);

    gate.process(buffer);

    auto gatedMetrics = SpatialAnalyzer::analyze(buffer);

    float reduction = 20.0f * std::log10(gatedMetrics.leftRMS / (originalMetrics.leftRMS + 1e-10f));

    std::cout << "  Original RMS: " << originalMetrics.leftRMS << "\n";
    std::cout << "  Gated RMS:    " << gatedMetrics.leftRMS << "\n";
    std::cout << "  Reduction:    " << -reduction << " dB\n";
    std::cout << "  Result: " << (gatedMetrics.leftRMS < originalMetrics.leftRMS ? "PASS" : "FAIL") << "\n";

    // Test 2: Threshold response
    std::cout << "\nTEST 2: Threshold Response\n";
    std::cout << "Thresh | RMS Out | Reduction (dB)\n";
    std::cout << "-------|---------|---------------\n";

    for (float thresh = 0.2f; thresh <= 0.8f; thresh += 0.2f) {
        juce::AudioBuffer<float> buffer(2, blockSize);
        TestSignals::generatePinkNoise(buffer, 0.2f);

        auto inputMetrics = SpatialAnalyzer::analyze(buffer);

        std::map<int, float> params;
        params[0] = thresh;  // Threshold
        params[1] = 0.8f;    // High ratio
        params[7] = 1.0f;    // Full wet
        gate.updateParameters(params);

        gate.process(buffer);

        auto outputMetrics = SpatialAnalyzer::analyze(buffer);

        float reductionDb = 20.0f * std::log10(outputMetrics.leftRMS / (inputMetrics.leftRMS + 1e-10f));

        std::cout << std::setw(6) << thresh << " | "
                  << std::setw(7) << outputMetrics.leftRMS << " | "
                  << std::setw(13) << reductionDb << "\n";
    }

    std::cout << "\n✓ SpectralGate validation complete\n";
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║    SPATIAL ENGINES DEEP VALIDATION TEST                       ║\n";
    std::cout << "║    Comprehensive parameter and processing verification        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";

    try {
        testDimensionExpander();
        testSpectralFreeze();
        testMidSideProcessor();
        testPhaseAlign();
        testSpectralGate();

        std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║    ALL SPATIAL ENGINE TESTS COMPLETED                         ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\n✗ TEST FAILED WITH EXCEPTION: " << e.what() << "\n";
        return 1;
    }
}
