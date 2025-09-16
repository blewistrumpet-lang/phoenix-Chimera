// ConvolutionReverb.cpp - FINAL FIXED VERSION
// Proper initialization order with safety checks

#include "ConvolutionReverb.h"
#include "BinaryData.h"
#include <cmath>
#include <algorithm>
#include <memory>

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
    bool initialized = false;
    bool irsLoaded = false;
    
    // Pre-loaded IR buffers
    std::map<int, juce::AudioBuffer<float>> irBuffers;
    
    void init(double sr, int samplesPerBlock) {
        sampleRate = sr;
        
        // STEP 1: Prepare all DSP components FIRST
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2; // Stereo processing
        
        // Prepare convolution BEFORE any IR loading
        convolution.prepare(spec);
        convolution.reset();
        
        // Prepare delays
        predelayL.prepare(spec);
        predelayR.prepare(spec);
        predelayL.setMaximumDelayInSamples(static_cast<int>(0.2 * sr));
        predelayR.setMaximumDelayInSamples(static_cast<int>(0.2 * sr));
        
        // Prepare filters (mono)
        spec.numChannels = 1;
        lowCutL.prepare(spec);
        lowCutR.prepare(spec);
        highCutL.prepare(spec);
        highCutR.prepare(spec);
        
        lowCutL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        lowCutR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        highCutL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        highCutR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        
        // STEP 2: Set initialized flag
        initialized = true;
        
        // STEP 3: Don't load IRs here - will be done on first process() call
        // This avoids any initialization deadlock
    }
    
    void loadIRsIfNeeded() {
        if (irsLoaded) return;
        
        // Generate simple algorithmic IRs instead of loading WAV files
        // This avoids WAV parsing issues
        generateAlgorithmicIRs();
        
        irsLoaded = true;
        needsIRReload = true;
    }
    
    void generateAlgorithmicIRs() {
        // Generate 4 different algorithmic reverb IRs
        const int irLength = static_cast<int>(sampleRate * 2.0); // 2 seconds
        
        // Concert Hall
        irBuffers[0] = generateHallIR(irLength);
        
        // EMT Plate
        irBuffers[1] = generatePlateIR(irLength);
        
        // Stairwell
        irBuffers[2] = generateStairwellIR(irLength);
        
        // Cloud Chamber
        irBuffers[3] = generateCloudIR(irLength);
    }
    
    juce::AudioBuffer<float> generateHallIR(int numSamples) {
        juce::AudioBuffer<float> ir(2, numSamples);
        ir.clear();
        
        juce::Random random;
        
        // Early reflections
        const float times[] = {0.015f, 0.022f, 0.035f, 0.045f, 0.058f, 0.072f, 0.089f, 0.108f};
        const float gains[] = {0.7f, 0.65f, 0.6f, 0.55f, 0.5f, 0.45f, 0.4f, 0.35f};
        
        for (int i = 0; i < 8; i++) {
            int pos = static_cast<int>(times[i] * sampleRate);
            if (pos < numSamples) {
                ir.setSample(0, pos, gains[i] * (i % 2 ? 0.8f : -0.8f));
                ir.setSample(1, pos, gains[i] * (i % 2 ? -0.7f : 0.9f));
            }
        }
        
        // Diffuse tail
        for (int i = sampleRate / 10; i < numSamples; i++) {
            float t = i / static_cast<float>(sampleRate);
            float env = std::exp(-3.0f * t / 2.8f); // RT60 = 2.8s
            
            if (random.nextFloat() < 0.3f * env) {
                float sample = (random.nextFloat() * 2.0f - 1.0f) * env * 0.3f;
                ir.setSample(0, i, ir.getSample(0, i) + sample);
                ir.setSample(1, i, ir.getSample(1, i) + sample * 0.9f);
            }
        }
        
        return ir;
    }
    
    juce::AudioBuffer<float> generatePlateIR(int numSamples) {
        juce::AudioBuffer<float> ir(2, numSamples);
        ir.clear();
        
        juce::Random random;
        
        // Dense immediate onset
        for (int i = 1; i < numSamples; i++) {
            float t = i / static_cast<float>(sampleRate);
            float env = std::exp(-3.0f * t / 1.8f); // RT60 = 1.8s
            
            // Metallic character
            float metallic = std::sin(i * 0.00523f) * 0.3f +
                           std::sin(i * 0.00234f) * 0.25f;
            
            ir.setSample(0, i, metallic * env * 0.5f);
            ir.setSample(1, i, metallic * env * 0.48f);
        }
        
        return ir;
    }
    
    juce::AudioBuffer<float> generateStairwellIR(int numSamples) {
        juce::AudioBuffer<float> ir(2, numSamples);
        ir.clear();
        
        // Flutter echoes
        const int flutterPeriod = static_cast<int>(0.012f * sampleRate);
        
        for (int echo = 0; echo < 100; echo++) {
            int pos = echo * flutterPeriod;
            if (pos >= numSamples) break;
            
            float gain = std::pow(0.85f, static_cast<float>(echo));
            ir.setSample(0, pos, gain);
            ir.setSample(1, pos, gain * 0.95f);
        }
        
        // Apply overall decay
        for (int i = 0; i < numSamples; i++) {
            float t = i / static_cast<float>(sampleRate);
            float env = std::exp(-3.0f * t / 1.2f);
            ir.setSample(0, i, ir.getSample(0, i) * env);
            ir.setSample(1, i, ir.getSample(1, i) * env);
        }
        
        return ir;
    }
    
    juce::AudioBuffer<float> generateCloudIR(int numSamples) {
        juce::AudioBuffer<float> ir(2, numSamples);
        ir.clear();
        
        juce::Random random;
        
        // Granular texture
        for (int g = 0; g < 200; g++) {
            int pos = random.nextInt(numSamples * 0.8f);
            float gain = random.nextFloat() * 0.2f;
            int grainSize = 100 + random.nextInt(300);
            
            for (int i = 0; i < grainSize && (pos + i) < numSamples; i++) {
                float envelope = std::sin(static_cast<float>(i) / grainSize * M_PI);
                float sample = (random.nextFloat() * 2.0f - 1.0f) * envelope * gain;
                ir.setSample(0, pos + i, ir.getSample(0, pos + i) + sample);
                ir.setSample(1, pos + i, ir.getSample(1, pos + i) + sample * (0.5f + random.nextFloat() * 0.5f));
            }
        }
        
        // Long decay
        for (int i = 0; i < numSamples; i++) {
            float t = i / static_cast<float>(sampleRate);
            float env = std::exp(-3.0f * t / 4.5f); // RT60 = 4.5s
            ir.setSample(0, i, ir.getSample(0, i) * env);
            ir.setSample(1, i, ir.getSample(1, i) * env);
        }
        
        return ir;
    }
    
    void loadImpulseResponse() {
        if (!initialized || !irsLoaded) return;
        
        int irIndex = static_cast<int>(irSelectParam * 3.99f);
        irIndex = std::clamp(irIndex, 0, 3);
        
        if (irIndex == currentIR && isReversed == (reverseParam > 0.5f) && !needsIRReload) {
            return;
        }
        
        currentIR = irIndex;
        isReversed = (reverseParam > 0.5f);
        needsIRReload = false;
        
        if (irBuffers.find(irIndex) == irBuffers.end()) {
            return;
        }
        
        // Process the IR
        juce::AudioBuffer<float> processedIR = irBuffers[irIndex];
        
        // Apply size parameter
        int targetSize = static_cast<int>(processedIR.getNumSamples() * sizeParam);
        targetSize = std::max(1024, targetSize);
        
        if (targetSize < processedIR.getNumSamples()) {
            int fadeLength = std::min(512, targetSize / 4);
            for (int ch = 0; ch < processedIR.getNumChannels(); ch++) {
                float* data = processedIR.getWritePointer(ch);
                for (int i = 0; i < fadeLength; i++) {
                    int pos = targetSize - fadeLength + i;
                    float gain = 1.0f - static_cast<float>(i) / fadeLength;
                    data[pos] *= gain * gain;
                }
            }
            processedIR.setSize(processedIR.getNumChannels(), targetSize, true);
        }
        
        // Apply damping
        if (dampingParam > 0.01f) {
            float dampFreq = 20000.0f * (1.0f - dampingParam);
            float dampCoeff = std::exp(-2.0f * M_PI * dampFreq / sampleRate);
            
            for (int ch = 0; ch < processedIR.getNumChannels(); ch++) {
                float* data = processedIR.getWritePointer(ch);
                float state = 0.0f;
                for (int i = 0; i < processedIR.getNumSamples(); i++) {
                    state = data[i] * (1.0f - dampCoeff) + state * dampCoeff;
                    data[i] = state;
                }
            }
        }
        
        // Apply early/late balance
        int earlySize = static_cast<int>(0.08f * sampleRate);
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
        
        // Apply reverse
        if (isReversed) {
            for (int ch = 0; ch < processedIR.getNumChannels(); ch++) {
                float* data = processedIR.getWritePointer(ch);
                std::reverse(data, data + processedIR.getNumSamples());
                
                int fadeInSamples = std::min(256, processedIR.getNumSamples() / 4);
                for (int i = 0; i < fadeInSamples; i++) {
                    float fade = static_cast<float>(i) / fadeInSamples;
                    data[i] *= fade * fade;
                }
            }
        }
        
        // Load into convolution
        convolution.loadImpulseResponse(std::move(processedIR),
                                       sampleRate,
                                       juce::dsp::Convolution::Stereo::yes,
                                       juce::dsp::Convolution::Trim::yes,
                                       juce::dsp::Convolution::Normalise::yes);
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
        int newIR = static_cast<int>(irSelectParam * 3.99f);
        if (newIR != currentIR || isReversed != (reverseParam > 0.5f)) {
            needsIRReload = true;
        }
        
        // Update predelay
        float predelayMs = predelayParam * 200.0f;
        int predelaySamples = static_cast<int>(predelayMs * sampleRate / 1000.0f);
        predelayL.setDelay(predelaySamples);
        predelayR.setDelay(predelaySamples);
        
        // Update filters
        float lowCutFreq = 20.0f * std::pow(50.0f, lowCutParam);
        lowCutL.setCutoffFrequency(lowCutFreq);
        lowCutR.setCutoffFrequency(lowCutFreq);
        
        float highCutFreq = 1000.0f * std::pow(20.0f, highCutParam);
        highCutFreq = std::min(highCutFreq, static_cast<float>(sampleRate * 0.49));
        highCutL.setCutoffFrequency(highCutFreq);
        highCutR.setCutoffFrequency(highCutFreq);
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        // SAFETY CHECK: Ensure everything is initialized
        if (!initialized) {
            return; // Pass through dry signal
        }
        
        // Load IRs on first process call (lazy loading)
        if (!irsLoaded) {
            loadIRsIfNeeded();
        }
        
        // Reload IR if needed
        if (needsIRReload && irsLoaded) {
            loadImpulseResponse();
        }
        
        // If still no IRs, pass through with mix control
        if (!irsLoaded || irBuffers.empty()) {
            // Just apply dry mix
            if (mixParam < 1.0f) {
                buffer.applyGain(1.0f - mixParam);
            } else {
                buffer.clear(); // Full wet with no reverb = silence
            }
            return;
        }
        
        // Normal processing
        juce::AudioBuffer<float> stereoBuffer(2, numSamples);
        stereoBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
        if (numChannels > 1) {
            stereoBuffer.copyFrom(1, 0, buffer, 1, 0, numSamples);
        } else {
            stereoBuffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
        }
        
        juce::AudioBuffer<float> dryBuffer(stereoBuffer);
        
        // Apply pre-delay
        if (predelayParam > 0.01f) {
            juce::dsp::AudioBlock<float> block(stereoBuffer);
            
            auto leftBlock = block.getSingleChannelBlock(0);
            juce::dsp::ProcessContextReplacing<float> contextL(leftBlock);
            predelayL.process(contextL);
            
            auto rightBlock = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> contextR(rightBlock);
            predelayR.process(contextR);
        }
        
        // Process convolution
        {
            juce::dsp::AudioBlock<float> block(stereoBuffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            convolution.process(context);
        }
        
        // Apply filters
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
        
        // Apply width
        if (widthParam < 0.99f) {
            for (int i = 0; i < numSamples; i++) {
                float left = stereoBuffer.getSample(0, i);
                float right = stereoBuffer.getSample(1, i);
                
                float mid = (left + right) * 0.5f;
                float side = (left - right) * 0.5f * widthParam;
                
                stereoBuffer.setSample(0, i, mid + side);
                stereoBuffer.setSample(1, i, mid - side);
            }
        }
        
        // Mix dry and wet
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
        
        switch (index) {
            case 0: mixParam = value; break;
            case 1: irSelectParam = value; break;
            case 2: sizeParam = value; needsIRReload = true; break;
            case 3: predelayParam = value; break;
            case 4: dampingParam = value; needsIRReload = true; break;
            case 5: reverseParam = value; break;
            case 6: earlyLateParam = value; needsIRReload = true; break;
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

// Public interface
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