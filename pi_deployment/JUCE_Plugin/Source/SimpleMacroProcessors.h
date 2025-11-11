#pragma once

#include "JuceHeader.h"
#include <array>

/**
 * Simple, efficient macro processors for Trinity GPIO control
 * These are always-on, fixed processors that provide predictable control
 */

//==============================================================================
/**
 * SimpleTiltEQ - Controls overall brightness/darkness
 * Uses a simple high-shelf + low-shelf to tilt the frequency spectrum
 */
class SimpleTiltEQ {
public:
    SimpleTiltEQ() = default;

    void prepare(double sampleRate, int samplesPerBlock) {
        currentSampleRate = sampleRate;

        // Initialize filters for each channel
        for (auto& channelFilters : filters) {
            for (auto& filter : channelFilters) {
                filter.reset();
            }
        }

        updateFilters();
    }

    void setWarmth(float warmth) {
        // 0.0 = dark, 0.5 = neutral, 1.0 = bright
        tiltAmount = (warmth - 0.5f) * 2.0f;  // -1 to +1
        updateFilters();
    }

    void process(juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int ch = 0; ch < numChannels && ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);

            // Process through low shelf then high shelf
            for (int i = 0; i < numSamples; ++i) {
                float sample = data[i];
                sample = filters[ch][0].processSample(sample);  // Low shelf
                sample = filters[ch][1].processSample(sample);  // High shelf
                data[i] = sample;
            }
        }
    }

private:
    void updateFilters() {
        if (currentSampleRate <= 0) return;

        // Low shelf at 200 Hz
        // Dark: boost +6dB, Bright: cut -6dB
        float lowGainDb = -tiltAmount * 6.0f;

        // High shelf at 4000 Hz
        // Dark: cut -6dB, Bright: boost +6dB
        float highGainDb = tiltAmount * 6.0f;

        for (int ch = 0; ch < 2; ++ch) {
            // Low shelf
            filters[ch][0].setCoefficients(
                juce::IIRCoefficients::makeLowShelf(
                    currentSampleRate, 200.0, 0.7,
                    juce::Decibels::decibelsToGain(lowGainDb)
                )
            );

            // High shelf
            filters[ch][1].setCoefficients(
                juce::IIRCoefficients::makeHighShelf(
                    currentSampleRate, 4000.0, 0.7,
                    juce::Decibels::decibelsToGain(highGainDb)
                )
            );
        }
    }

    double currentSampleRate = 44100.0;
    float tiltAmount = 0.0f;  // -1 to +1

    // 2 channels, 2 filters per channel (low shelf, high shelf)
    std::array<std::array<juce::IIRFilter, 2>, 2> filters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleTiltEQ)
};

//==============================================================================
/**
 * SimpleReverb - Adds spatial dimension
 * Basic algorithmic reverb with just size control
 */
class SimpleReverb {
public:
    SimpleReverb() {
        reverb.setSampleRate(44100.0);
        updateParameters();
    }

    void prepare(double sampleRate, int samplesPerBlock) {
        reverb.setSampleRate(sampleRate);
        updateParameters();
    }

    void setSize(float size) {
        // 0.0 = tight/small, 0.5 = medium, 1.0 = spacious/large
        roomSize = size;
        updateParameters();
    }

    void process(juce::AudioBuffer<float>& buffer) {
        if (wetLevel < 0.001f) return;  // Skip if no reverb needed

        // Create wet buffer for reverb
        wetBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
        wetBuffer.makeCopyOf(buffer);

        // Process reverb
        if (buffer.getNumChannels() == 1) {
            reverb.processMono(wetBuffer.getWritePointer(0), wetBuffer.getNumSamples());
        } else {
            reverb.processStereo(
                wetBuffer.getWritePointer(0),
                wetBuffer.getWritePointer(1),
                wetBuffer.getNumSamples()
            );
        }

        // Mix wet reverb with dry signal
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.addFrom(ch, 0, wetBuffer, ch, 0, buffer.getNumSamples(), wetLevel);
        }
    }

private:
    void updateParameters() {
        juce::Reverb::Parameters params;

        // Map size (0-1) to reverb parameters
        params.roomSize = 0.3f + (roomSize * 0.6f);      // 0.3 to 0.9
        params.damping = 0.7f - (roomSize * 0.3f);       // 0.7 to 0.4 (less damping for larger)
        params.wetLevel = 0.0f + (roomSize * 0.25f);     // 0.0 to 0.25 (subtle)
        params.dryLevel = 1.0f;
        params.width = 0.5f + (roomSize * 0.5f);         // 0.5 to 1.0
        params.freezeMode = 0.0f;

        reverb.setParameters(params);
        wetLevel = params.wetLevel;
    }

    juce::Reverb reverb;
    juce::AudioBuffer<float> wetBuffer;
    float roomSize = 0.5f;
    float wetLevel = 0.1f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleReverb)
};

//==============================================================================
/**
 * SimpleCompressor - Controls dynamics/punch
 * Basic RMS compressor with threshold and ratio
 */
class SimpleCompressor {
public:
    SimpleCompressor() = default;

    void prepare(double sampleRate, int samplesPerBlock) {
        currentSampleRate = sampleRate;

        // Reset envelope followers
        for (auto& env : envelopes) {
            env = 0.0f;
        }

        updateParameters();
    }

    void setPunch(float punch) {
        // 0.0 = soft/gentle, 0.5 = neutral, 1.0 = aggressive/punchy
        punchAmount = punch;
        updateParameters();
    }

    void process(juce::AudioBuffer<float>& buffer) {
        if (punchAmount < 0.01f) return;  // Skip if no compression

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int ch = 0; ch < numChannels && ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i) {
                float input = data[i];
                float inputLevel = std::abs(input);

                // Simple envelope follower
                float attack = (inputLevel > envelopes[ch]) ? attackTime : releaseTime;
                envelopes[ch] = inputLevel + attack * (envelopes[ch] - inputLevel);

                // Calculate gain reduction
                float gainReduction = 1.0f;
                if (envelopes[ch] > threshold) {
                    float excess = envelopes[ch] - threshold;
                    float compressedExcess = excess / ratio;
                    gainReduction = (threshold + compressedExcess) / envelopes[ch];
                }

                // Apply compression with makeup gain
                data[i] = input * gainReduction * makeupGain;

                // Soft clip to prevent harsh distortion
                if (std::abs(data[i]) > 0.95f) {
                    data[i] = 0.95f * (data[i] > 0 ? 1.0f : -1.0f);
                }
            }
        }
    }

private:
    void updateParameters() {
        if (currentSampleRate <= 0) return;

        // Map punch (0-1) to compressor parameters
        if (punchAmount < 0.5f) {
            // Soft: gentle compression
            threshold = 0.7f;
            ratio = 2.0f;
            makeupGain = 1.1f;
        } else {
            // Aggressive: heavy compression for punch
            float aggressive = (punchAmount - 0.5f) * 2.0f;  // 0 to 1
            threshold = 0.7f - (aggressive * 0.4f);  // 0.7 to 0.3
            ratio = 2.0f + (aggressive * 6.0f);      // 2:1 to 8:1
            makeupGain = 1.1f + (aggressive * 0.4f); // 1.1 to 1.5
        }

        // Fixed time constants for punchy response
        attackTime = std::exp(-1.0f / (0.005f * currentSampleRate));  // 5ms attack
        releaseTime = std::exp(-1.0f / (0.050f * currentSampleRate)); // 50ms release
    }

    double currentSampleRate = 44100.0;
    float punchAmount = 0.5f;

    // Compressor parameters
    float threshold = 0.7f;
    float ratio = 2.0f;
    float makeupGain = 1.0f;
    float attackTime = 0.995f;
    float releaseTime = 0.9995f;

    // Envelope followers for each channel
    std::array<float, 2> envelopes = {0.0f, 0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleCompressor)
};

//==============================================================================
/**
 * SimpleMacroProcessors - Container for all three macro processors
 */
class SimpleMacroProcessors {
public:
    SimpleMacroProcessors() = default;

    void prepare(double sampleRate, int samplesPerBlock) {
        warmthEQ.prepare(sampleRate, samplesPerBlock);
        sizeReverb.prepare(sampleRate, samplesPerBlock);
        punchCompressor.prepare(sampleRate, samplesPerBlock);
    }

    void setWarmth(float value) {
        warmthEQ.setWarmth(value);
    }

    void setSize(float value) {
        sizeReverb.setSize(value);
    }

    void setPunch(float value) {
        punchCompressor.setPunch(value);
    }

    void processPreSlots(juce::AudioBuffer<float>& buffer) {
        // Dynamics go BEFORE the slot engines
        punchCompressor.process(buffer);
    }

    void processPostSlots(juce::AudioBuffer<float>& buffer) {
        // EQ and Reverb go AFTER the slot engines
        warmthEQ.process(buffer);
        sizeReverb.process(buffer);
    }

private:
    SimpleTiltEQ warmthEQ;
    SimpleReverb sizeReverb;
    SimpleCompressor punchCompressor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleMacroProcessors)
};