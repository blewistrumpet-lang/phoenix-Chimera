#include "BitCrusher.h"
#include <cmath>

void BitCrusher::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;

    // Initialize smoothing with 10ms ramp time
    const float rampTimeMs = 10.0f;
    const int rampSamples = static_cast<int>(sampleRate * rampTimeMs * 0.001);

    m_bits.reset(sampleRate, rampSamples);
    m_bits.setCurrentAndTargetValue(16.0f);

    m_downsample.reset(sampleRate, rampSamples);
    m_downsample.setCurrentAndTargetValue(1.0f);

    m_mix.reset(sampleRate, rampSamples);
    m_mix.setCurrentAndTargetValue(1.0f);

    reset();
}

void BitCrusher::reset() {
    m_heldSampleL = 0.0f;
    m_heldSampleR = 0.0f;
    m_counterL = 0.0f;
    m_counterR = 0.0f;
}

void BitCrusher::process(juce::AudioBuffer<float>& buffer) {
    // Add denormal protection
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Process parameter smoothing once per block
    m_bits.skip(numSamples);
    m_downsample.skip(numSamples);
    m_mix.skip(numSamples);

    for (int ch = 0; ch < numChannels && ch < 2; ++ch) {
        float* data = buffer.getWritePointer(ch);
        float& heldSample = (ch == 0) ? m_heldSampleL : m_heldSampleR;
        float& counter = (ch == 0) ? m_counterL : m_counterR;

        // Reset smoothing for per-sample processing
        m_bits.skip(-numSamples);
        m_downsample.skip(-numSamples);
        m_mix.skip(-numSamples);

        for (int i = 0; i < numSamples; ++i) {
            // Get smoothed parameter values
            const float bits = m_bits.getNextValue();
            const float downsample = m_downsample.getNextValue();
            const float mix = m_mix.getNextValue();

            float input = data[i];
            const float dry = input;

            // Add tiny DC offset to prevent denormals
            input += DENORMAL_OFFSET;

            // 1. Bit depth reduction (quantize)
            if (bits < 23.9f) {  // Avoid processing at 24-bit (no effect)
                // Ensure we don't get zero levels
                const float levels = std::max(2.0f, std::pow(2.0f, bits));
                const float levelScale = levels * 0.5f;  // Scale to avoid overflow

                // Quantize with soft clipping to prevent harsh distortion
                float scaled = std::tanh(input) * levelScale;
                input = std::round(scaled) / levelScale;
            }

            // 2. Sample rate reduction (downsample with sample-and-hold)
            counter += 1.0f;
            if (counter >= downsample) {
                // Use modulo to prevent counter drift
                counter = std::fmod(counter, downsample);
                heldSample = input;
            }

            // 3. Apply mix
            const float wet = (downsample > 1.001f) ? heldSample : input;

            // Remove DC offset and apply mix
            data[i] = (dry * (1.0f - mix) + (wet - DENORMAL_OFFSET) * mix);

            // Final safety check
            if (!std::isfinite(data[i])) {
                data[i] = 0.0f;
            }
        }
    }
}

void BitCrusher::updateParameters(const std::map<int, float>& params) {
    auto it = params.find(0);
    if (it != params.end()) {
        // Bits: map 0-1 to useful bit depths with smooth transitions
        const float v = juce::jlimit(0.0f, 1.0f, it->second);
        float targetBits;

        if (v < 0.2f)      targetBits = 24.0f;  // Clean
        else if (v < 0.4f) targetBits = 12.0f;  // Vintage sampler
        else if (v < 0.6f) targetBits = 8.0f;   // 8-bit
        else if (v < 0.8f) targetBits = 4.0f;   // Crunchy
        else               targetBits = 2.0f;   // Destroyed (avoid 1-bit)

        m_bits.setTargetValue(targetBits);
    }

    it = params.find(1);
    if (it != params.end()) {
        // Downsample: use continuous mapping for smooth transitions
        const float v = juce::jlimit(0.0f, 1.0f, it->second);

        // Exponential mapping for more musical control
        const float targetDownsample = 1.0f + v * v * 15.0f;  // 1 to 16
        m_downsample.setTargetValue(targetDownsample);
    }

    it = params.find(2);
    if (it != params.end()) {
        const float v = juce::jlimit(0.0f, 1.0f, it->second);
        m_mix.setTargetValue(v);
    }
}

juce::String BitCrusher::getParameterName(int index) const {
    switch (index) {
        case 0: return "Bits";
        case 1: return "Downsample";
        case 2: return "Mix";
        default: return "";
    }
}