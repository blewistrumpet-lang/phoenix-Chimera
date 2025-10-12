// ConvolutionReverb_Algorithmic.cpp - Using algorithmic IR generation
// Avoids WAV file dependencies while still using JUCE convolution engine

#include "ConvolutionReverb.h"
#include <cmath>
#include <algorithm>
#include <memory>
#include <random>

class ConvolutionReverb::Impl {
public:
    // JUCE's stereo convolution engine
    juce::dsp::Convolution convolution;
    
    // Pre-delay lines
    juce::dsp::DelayLine<float> predelayL{44100};
    juce::dsp::DelayLine<float> predelayR{44100};
    
    // Filters
    juce::dsp::StateVariableTPTFilter<float> lowCutL;
    juce::dsp::StateVariableTPTFilter<float> lowCutR;
    juce::dsp::StateVariableTPTFilter<float> highCutL;
    juce::dsp::StateVariableTPTFilter<float> highCutR;
    
    // Parameters
    float mixParam = 0.5f;
    float irSelectParam = 0.0f;
    float sizeParam = 1.0f;
    float predelayParam = 0.0f;
    float dampingParam = 0.0f;
    float reverseParam = 0.0f;
    float earlyLateParam = 0.5f;
    float lowCutParam = 0.0f;
    float highCutParam = 1.0f;
    float widthParam = 1.0f;
    
    // State
    double sampleRate = 44100.0;
    int currentIR = -1;
    bool isReversed = false;
    bool needsIRReload = true;
    bool isInitialized = false;
    bool isReloading = false;  // CRITICAL: Prevent concurrent IR reloads
    
    void init(double sr, int samplesPerBlock) {
        sampleRate = sr;

        // CRITICAL: Initialize convolution engine FIRST
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2; // Stereo processing

        convolution.prepare(spec);
        convolution.reset();

        // Initialize pre-delay with stereo spec
        predelayL.prepare(spec);
        predelayR.prepare(spec);
        predelayL.setMaximumDelayInSamples(static_cast<int>(0.2 * sr)); // 200ms max
        predelayR.setMaximumDelayInSamples(static_cast<int>(0.2 * sr));

        // Initialize filters with mono spec
        spec.numChannels = 1; // Filters are mono
        lowCutL.prepare(spec);
        lowCutR.prepare(spec);
        highCutL.prepare(spec);
        highCutR.prepare(spec);

        lowCutL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        lowCutR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        highCutL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        highCutR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

        // Load default algorithmic IR
        isInitialized = true;
        loadImpulseResponse();
    }
    
    juce::AudioBuffer<float> generateAlgorithmicIR(int type, double sr) {
        // Generate different IR characteristics based on type
        int irLength = 0;
        float decay = 0.0f;
        float density = 0.0f;
        float brightness = 0.0f;
        
        switch (type) {
            case 0: // Concert Hall
                irLength = static_cast<int>(sr * 3.0); // 3 seconds
                decay = 0.95f;
                density = 0.8f;
                brightness = 0.7f;
                break;
            case 1: // EMT Plate
                irLength = static_cast<int>(sr * 2.0); // 2 seconds
                decay = 0.93f;
                density = 0.95f;
                brightness = 0.9f;
                break;
            case 2: // Stairwell
                irLength = static_cast<int>(sr * 4.0); // 4 seconds
                decay = 0.96f;
                density = 0.6f;
                brightness = 0.5f;
                break;
            case 3: // Cloud Chamber
                irLength = static_cast<int>(sr * 5.0); // 5 seconds
                decay = 0.97f;
                density = 0.7f;
                brightness = 0.6f;
                break;
            default:
                irLength = static_cast<int>(sr * 2.0);
                decay = 0.94f;
                density = 0.7f;
                brightness = 0.7f;
        }
        
        // Create stereo IR buffer
        juce::AudioBuffer<float> ir(2, irLength);
        ir.clear();
        
        std::mt19937 rng(type + 12345); // Seed for reproducibility
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        // Generate early reflections (first 100ms)
        int earlyLength = static_cast<int>(0.1 * sr);
        int numEarlyReflections = static_cast<int>(density * 20);
        
        for (int i = 0; i < numEarlyReflections; i++) {
            int delay = static_cast<int>((earlyLength * i) / numEarlyReflections);
            float gain = std::pow(0.8f, i) * 0.5f;
            
            // Add to both channels with slight variation
            if (delay < irLength) {
                ir.setSample(0, delay, ir.getSample(0, delay) + gain * dist(rng));
                ir.setSample(1, delay, ir.getSample(1, delay) + gain * dist(rng));
            }
        }
        
        // Generate late reverb tail using exponential decay with noise
        float decayRate = -std::log(0.001f) / irLength; // Decay to -60dB
        
        for (int ch = 0; ch < 2; ch++) {
            float* data = ir.getWritePointer(ch);
            
            // Start from after early reflections
            for (int i = earlyLength; i < irLength; i++) {
                float envelope = std::exp(-decayRate * i * (2.0f - decay));
                float noise = dist(rng) * 0.1f;
                
                // Apply density modulation
                if ((i % static_cast<int>(10 / density)) == 0) {
                    noise *= density;
                }
                
                data[i] += noise * envelope;
            }
            
            // Apply brightness filtering (simple lowpass)
            // Use in-place lowpass filter to avoid temporary buffer allocation
            if (brightness < 0.99f) {
                // Use simple one-pole lowpass filter (no memory allocation)
                float coeff = brightness; // 0.99 = very bright, 0.0 = very dark
                float state = data[0];

                for (int i = 1; i < irLength; i++) {
                    state = data[i] * (1.0f - coeff) + state * coeff;
                    data[i] = state;
                }
            }
            
            // Normalize to prevent clipping
            float maxSample = 0.0f;
            for (int i = 0; i < irLength; i++) {
                maxSample = std::max(maxSample, std::abs(data[i]));
            }
            if (maxSample > 0.0f) {
                float normFactor = 0.8f / maxSample;
                for (int i = 0; i < irLength; i++) {
                    data[i] *= normFactor;
                }
            }
        }
        
        // Add stereo width variation through simple all-pass decorrelation
        // Apply a small delay offset to right channel for decorrelation
        // CRITICAL FIX: Process in-place to avoid memory allocation
        for (int ch = 0; ch < 2; ch++) {
            float* data = ir.getWritePointer(ch);

            // Offset by 7 or 11 samples (prime numbers for less periodicity)
            int offset = (ch == 0) ? 7 : 11;

            // Process backwards to avoid overwriting data we need
            for (int i = irLength - 1; i >= offset; i--) {
                float delayed = data[i - offset];
                data[i] = data[i] * 0.9f + delayed * 0.1f;
            }
        }
        
        return ir;
    }
    
    void loadImpulseResponse() {
        if (!isInitialized) {
            return;
        }

        // CRITICAL FIX: Prevent concurrent reloads that cause memory accumulation
        if (isReloading) {
            return;
        }

        // Determine which IR to load
        int irIndex = static_cast<int>(irSelectParam * 3.99f); // 0-3
        irIndex = std::clamp(irIndex, 0, 3);

        // Check if we need to reload
        if (irIndex == currentIR && isReversed == (reverseParam > 0.5f) && !needsIRReload) {
            return;
        }

        // Set reloading flag to prevent re-entry
        isReloading = true;
        currentIR = irIndex;
        isReversed = (reverseParam > 0.5f);
        needsIRReload = false;

        // Generate algorithmic IR
        juce::AudioBuffer<float> processedIR = generateAlgorithmicIR(irIndex, sampleRate);

        // DIAGNOSTIC: Validate IR after generation
        float initialPeak = processedIR.getMagnitude(0, processedIR.getNumSamples());
        float initialRMS = processedIR.getRMSLevel(0, 0, processedIR.getNumSamples());

        if (initialPeak < 0.0001f || initialRMS < 0.00001f) {
            DBG("ConvolutionReverb ERROR: Generated IR is too weak or empty! Peak=" << initialPeak << ", RMS=" << initialRMS);
            // Generate a simple impulse as fallback
            processedIR.clear();
            processedIR.setSample(0, 0, 0.5f);
            processedIR.setSample(1, 0, 0.5f);
        }

        // Apply size parameter (truncate or full)
        int targetSize = static_cast<int>(processedIR.getNumSamples() * sizeParam);
        targetSize = std::max(1024, targetSize); // Minimum size
        
        if (targetSize < processedIR.getNumSamples()) {
            // Apply fade out before truncating
            int fadeLength = std::min(512, targetSize / 4);
            for (int ch = 0; ch < processedIR.getNumChannels(); ch++) {
                float* data = processedIR.getWritePointer(ch);
                for (int i = 0; i < fadeLength; i++) {
                    int pos = targetSize - fadeLength + i;
                    float gain = 1.0f - (float)i / fadeLength;
                    data[pos] *= gain * gain;
                }
            }
            processedIR.setSize(processedIR.getNumChannels(), targetSize, true);
        }
        
        // Apply damping (lowpass filter to reduce high frequencies in IR)
        // CRITICAL FIX: Use in-place one-pole lowpass to avoid memory allocation
        if (dampingParam > 0.01f) {
            for (int ch = 0; ch < processedIR.getNumChannels(); ch++) {
                float* data = processedIR.getWritePointer(ch);

                // Use simple one-pole lowpass filter (no memory allocation)
                // Coefficient increases with damping (more filtering)
                float coeff = 0.5f + (dampingParam * 0.49f); // 0.5 to 0.99
                float state = data[0];

                for (int i = 1; i < processedIR.getNumSamples(); i++) {
                    state = data[i] * (1.0f - coeff) + state * coeff;
                    data[i] = state;
                }
            }
        }
        
        // Apply early/late balance
        int earlySize = static_cast<int>(0.08f * sampleRate); // First 80ms
        float earlyGain = 1.0f + (1.0f - earlyLateParam);
        float lateGain = 1.0f + earlyLateParam;
        
        for (int ch = 0; ch < processedIR.getNumChannels(); ch++) {
            float* data = processedIR.getWritePointer(ch);
            for (int i = 0; i < processedIR.getNumSamples(); i++) {
                if (i < earlySize) {
                    data[i] *= earlyGain;
                } else {
                    data[i] *= lateGain;
                }
            }
        }
        
        // Apply reverse if needed
        if (isReversed) {
            for (int ch = 0; ch < processedIR.getNumChannels(); ch++) {
                float* data = processedIR.getWritePointer(ch);
                std::reverse(data, data + processedIR.getNumSamples());

                // Apply fade-in to avoid click
                int fadeInSamples = std::min(256, processedIR.getNumSamples() / 4);
                for (int i = 0; i < fadeInSamples; i++) {
                    float fade = (float)i / fadeInSamples;
                    data[i] *= fade * fade;
                }
            }
        }

        // FINAL VALIDATION: Check IR before loading
        float finalPeak = processedIR.getMagnitude(0, processedIR.getNumSamples());
        float finalRMS = processedIR.getRMSLevel(0, 0, processedIR.getNumSamples());

        // Count non-zero samples to ensure IR has content
        int nonZeroCount = 0;
        for (int ch = 0; ch < processedIR.getNumChannels(); ch++) {
            const float* data = processedIR.getReadPointer(ch);
            for (int i = 0; i < processedIR.getNumSamples(); i++) {
                if (std::abs(data[i]) > 0.0001f) {
                    nonZeroCount++;
                }
            }
        }

        float nonZeroPercent = 100.0f * nonZeroCount / (processedIR.getNumSamples() * processedIR.getNumChannels());

        DBG("ConvolutionReverb: Final IR - Length=" << processedIR.getNumSamples()
            << ", Peak=" << finalPeak
            << ", RMS=" << finalRMS
            << ", NonZero=" << nonZeroPercent << "%");

        if (finalPeak < 0.0001f || nonZeroCount < 100) {
            DBG("ConvolutionReverb ERROR: Final IR is destroyed! Using emergency impulse.");
            // Emergency fallback - create simple but valid IR
            processedIR.clear();
            // Create a simple exponential decay
            for (int ch = 0; ch < processedIR.getNumChannels(); ch++) {
                float* data = processedIR.getWritePointer(ch);
                data[0] = 0.8f; // Initial impulse
                for (int i = 1; i < std::min(4800, processedIR.getNumSamples()); i++) {
                    data[i] = data[i-1] * 0.9995f; // Simple decay
                }
            }
        }

        // Load into convolution engine using stereo processing
        // NOTE: Using Normalise::no to preserve our carefully crafted IR energy
        convolution.loadImpulseResponse(std::move(processedIR),
                                       sampleRate,
                                       juce::dsp::Convolution::Stereo::yes,
                                       juce::dsp::Convolution::Trim::yes,
                                       juce::dsp::Convolution::Normalise::no);

        // Clear reloading flag after successful load
        isReloading = false;
    }
    
    void reset() {
        convolution.reset();
        predelayL.reset();
        predelayR.reset();
        lowCutL.reset();
        lowCutR.reset();
        highCutL.reset();
        highCutR.reset();
    }
    
    void updateCoefficients() {
        // Check if IR needs reloading (only when parameters actually change)
        int newIR = static_cast<int>(irSelectParam * 3.99f);
        bool newReverse = (reverseParam > 0.5f);

        // Only trigger reload if parameters actually changed
        if (newIR != currentIR || newReverse != isReversed) {
            if (!isReloading) {  // Don't set flag if already reloading
                needsIRReload = true;
            }
        }

        // Update predelay
        float predelayMs = predelayParam * 200.0f; // 0-200ms
        int predelaySamples = static_cast<int>(predelayMs * sampleRate / 1000.0f);
        predelayL.setDelay(predelaySamples);
        predelayR.setDelay(predelaySamples);

        // Update filters
        float lowCutFreq = 20.0f * std::pow(50.0f, lowCutParam); // 20Hz to 1kHz
        lowCutL.setCutoffFrequency(lowCutFreq);
        lowCutR.setCutoffFrequency(lowCutFreq);

        float highCutFreq = 1000.0f * std::pow(20.0f, highCutParam); // 1kHz to 20kHz
        highCutFreq = std::min(highCutFreq, (float)(sampleRate * 0.49));
        highCutL.setCutoffFrequency(highCutFreq);
        highCutR.setCutoffFrequency(highCutFreq);
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numChannels == 0 || numSamples == 0) return;

        // Safety check: ensure we're initialized
        if (!isInitialized) {
            return; // Pass through dry signal
        }

        // Reload IR if needed
        if (needsIRReload) {
            loadImpulseResponse();
        }

        // CRITICAL FIX: Store dry signal BEFORE any processing
        juce::AudioBuffer<float> dryBuffer(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ch++) {
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
        }

        // Ensure we have stereo for processing
        juce::AudioBuffer<float> stereoBuffer(2, numSamples);
        stereoBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
        if (numChannels > 1) {
            stereoBuffer.copyFrom(1, 0, buffer, 1, 0, numSamples);
        } else {
            stereoBuffer.copyFrom(1, 0, buffer, 0, 0, numSamples); // Duplicate mono to stereo
        }

        // Apply pre-delay if needed
        if (predelayParam > 0.01f) {
            juce::dsp::AudioBlock<float> block(stereoBuffer);

            auto leftBlock = block.getSingleChannelBlock(0);
            juce::dsp::ProcessContextReplacing<float> contextL(leftBlock);
            predelayL.process(contextL);

            auto rightBlock = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> contextR(rightBlock);
            predelayR.process(contextR);
        }

        // DIAGNOSTIC: Check input to convolution
        float inputPeak = stereoBuffer.getMagnitude(0, numSamples);

        // Process through convolution (stereo processing)
        {
            juce::dsp::AudioBlock<float> block(stereoBuffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            convolution.process(context);
        }

        // DIAGNOSTIC: Check output from convolution
        float outputPeak = stereoBuffer.getMagnitude(0, numSamples);

        static int debugCounter = 0;
        if (debugCounter++ % 500 == 0) { // Log every 500 blocks (~10 seconds at 512 samples/block, 48kHz)
            DBG("ConvolutionReverb: Input=" << inputPeak << ", Output=" << outputPeak
                << ", Latency=" << convolution.getLatency());

            if (inputPeak > 0.01f && outputPeak < 0.0001f) {
                DBG("ConvolutionReverb WARNING: Input present but output is zero!");
            }
        }

        // Apply filters if needed
        if (lowCutParam > 0.01f) {
            juce::dsp::AudioBlock<float> block(stereoBuffer);

            auto leftBlock = block.getSingleChannelBlock(0);
            juce::dsp::ProcessContextReplacing<float> contextL(leftBlock);
            lowCutL.process(contextL);

            auto rightBlock = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> contextR(rightBlock);
            lowCutR.process(contextR);
        }

        if (highCutParam < 0.99f) {
            juce::dsp::AudioBlock<float> block(stereoBuffer);

            auto leftBlock = block.getSingleChannelBlock(0);
            juce::dsp::ProcessContextReplacing<float> contextL(leftBlock);
            highCutL.process(contextL);

            auto rightBlock = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> contextR(rightBlock);
            highCutR.process(contextR);
        }

        // Apply stereo width
        if (widthParam < 0.99f) {
            for (int i = 0; i < numSamples; i++) {
                float left = stereoBuffer.getSample(0, i);
                float right = stereoBuffer.getSample(1, i);

                float mid = (left + right) * 0.5f;
                float side = (left - right) * 0.5f;

                side *= widthParam;

                stereoBuffer.setSample(0, i, mid + side);
                stereoBuffer.setSample(1, i, mid - side);
            }
        }

        // Mix dry and wet signals
        for (int ch = 0; ch < numChannels; ch++) {
            for (int i = 0; i < numSamples; i++) {
                float dry = dryBuffer.getSample(ch, i);
                float wet = stereoBuffer.getSample(ch, i);
                buffer.setSample(ch, i, dry * (1.0f - mixParam) + wet * mixParam);
            }
        }
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);

        // CRITICAL FIX: Only reload IR if parameter actually changed significantly (> 5%)
        // This prevents constant IR regeneration during parameter automation
        const float changeThreshold = 0.05f;

        switch (index) {
            case 0: mixParam = value; break;
            case 1: irSelectParam = value; break;
            case 2:
                if (std::abs(sizeParam - value) > changeThreshold) {
                    sizeParam = value;
                    needsIRReload = true;
                } else {
                    sizeParam = value;
                }
                break;
            case 3: predelayParam = value; break;
            case 4:
                if (std::abs(dampingParam - value) > changeThreshold) {
                    dampingParam = value;
                    needsIRReload = true;
                } else {
                    dampingParam = value;
                }
                break;
            case 5: reverseParam = value; break;
            case 6:
                if (std::abs(earlyLateParam - value) > changeThreshold) {
                    earlyLateParam = value;
                    needsIRReload = true;
                } else {
                    earlyLateParam = value;
                }
                break;
            case 7: lowCutParam = value; break;
            case 8: highCutParam = value; break;
            case 9: widthParam = value; break;
        }

        updateCoefficients();
    }
    
    int getLatencySamples() const {
        return static_cast<int>(convolution.getLatency());
    }
};

// Public interface implementation
ConvolutionReverb::ConvolutionReverb() : pImpl(std::make_unique<Impl>()) {}
ConvolutionReverb::~ConvolutionReverb() = default;

void ConvolutionReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->init(sampleRate, samplesPerBlock);
}

void ConvolutionReverb::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void ConvolutionReverb::reset() {
    pImpl->reset();
}

void ConvolutionReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index < 10) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String ConvolutionReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Mix";
        case 1: return "IR Select";
        case 2: return "Size";
        case 3: return "Pre-Delay";
        case 4: return "Damping";
        case 5: return "Reverse";
        case 6: return "Early/Late";
        case 7: return "Low Cut";
        case 8: return "High Cut";
        case 9: return "Width";
        default: return "";
    }
}

int ConvolutionReverb::getNumParameters() const {
    return 10;
}

juce::String ConvolutionReverb::getName() const {
    return "Convolution Reverb";
}

int ConvolutionReverb::getLatencySamples() const noexcept {
    return pImpl ? pImpl->getLatencySamples() : 0;
}