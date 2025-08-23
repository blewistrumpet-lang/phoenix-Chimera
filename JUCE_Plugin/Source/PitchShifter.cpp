#include "PitchShifter.h"
#include <cmath>
#include <algorithm>

PitchShifter::PitchShifter() {
    // Initialize with sensible defaults
    modeParam.set(0.0f);      // Start in Gender mode
    control1Param.set(0.5f);   // Neutral position
    control2Param.set(0.5f);   // Neutral position
    control3Param.set(1.0f);   // Full mix for immediate effect
    
    reset();
}

PitchShifter::~PitchShifter() = default;

void PitchShifter::prepareToPlay(double sr, int samplesPerBlock) {
    sampleRate = sr;
    currentBlockSize = samplesPerBlock;
    
    // Initialize Signalsmith stretchers for each channel
    for (auto& stretcher : stretchers) {
        stretcher.presetCheaper(1, (float)sampleRate);
        stretcher.reset();
        stretcher.setTransposeFactor(1.0f);
    }
    
    // Set appropriate smoothing speeds
    modeParam.setSmoothingSpeed(0.05f);     // Fast for mode switches
    control1Param.setSmoothingSpeed(0.01f); // Smooth for audio
    control2Param.setSmoothingSpeed(0.01f);
    control3Param.setSmoothingSpeed(0.01f);
    
    reset();
}

void PitchShifter::reset() {
    // Reset processors
    glitchProcessor.reset();
    alienProcessor.pitchAccumulation = 1.0f;
    alienProcessor.lfoPhase = 0.0f;
    
    for (auto& stretcher : stretchers) {
        stretcher.reset();
    }
    
    for (auto& detector : transientDetectors) {
        detector.envelope = 0.0f;
    }
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Get smoothed parameters
    float modeValue = modeParam.tick();
    float control1 = control1Param.tick();
    float control2 = control2Param.tick();
    float control3 = control3Param.tick();
    
    // Determine current mode
    currentMode = getCurrentMode(modeValue);
    
    // Process each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        
        // Store dry signal for mixing
        std::vector<float> dry(channelData, channelData + numSamples);
        
        // Mode-specific processing
        switch (currentMode) {
            case MODE_GENDER: {
                // Gender Bender mode - process with formant/pitch shifting
                float formantRatio = 1.0f;
                float pitchRatio = 1.0f;
                
                genderProcessor.process(formantRatio, pitchRatio, control1, control2, control3);
                
                // Apply pitch/formant shifting if needed
                if (std::abs(pitchRatio - 1.0f) > 0.001f || std::abs(formantRatio - 1.0f) > 0.001f) {
                    stretchers[ch].setTransposeFactor(pitchRatio / formantRatio);
                    
                    std::vector<float> tempOutput(numSamples);
                    const float* inputChannels[1] = { channelData };
                    float* outputChannels[1] = { tempOutput.data() };
                    
                    stretchers[ch].process(inputChannels, numSamples, outputChannels, numSamples);
                    
                    // Apply formant compensation
                    float compensation = genderProcessor.calculateCompensation(formantRatio);
                    for (int i = 0; i < numSamples; ++i) {
                        channelData[i] = tempOutput[i] * compensation;
                    }
                }
                
                // Mix with dry (control3 is intensity in Gender mode)
                float wetAmount = control3;
                for (int i = 0; i < numSamples; ++i) {
                    channelData[i] = dry[i] * (1.0f - wetAmount) + channelData[i] * wetAmount;
                }
                break;
            }
            
            case MODE_GLITCH: {
                // Glitch Machine mode - rhythmic stutters
                glitchProcessor.updateSliceSize(control1, sampleRate);
                bool freeze = control3 > 0.5f; // Control3 is freeze button in Glitch mode
                
                for (int i = 0; i < numSamples; ++i) {
                    // Detect transients for smarter glitching
                    bool isTransient = transientDetectors[ch].detectTransient(channelData[i]);
                    
                    // Don't glitch on transients unless frozen
                    if (!isTransient || freeze) {
                        channelData[i] = glitchProcessor.process(channelData[i], ch, control2, freeze);
                    }
                }
                break;
            }
            
            case MODE_ALIEN: {
                // Alien Transform mode - creative mangling
                float formantRatio = 1.0f;
                float pitchRatio = 1.0f;
                
                alienProcessor.process(formantRatio, pitchRatio, control1, control2, control3, sampleRate);
                
                // Apply alien transformation
                if (std::abs(pitchRatio - 1.0f) > 0.001f || std::abs(formantRatio - 1.0f) > 0.001f) {
                    stretchers[ch].setTransposeFactor(pitchRatio / formantRatio);
                    
                    std::vector<float> tempOutput(numSamples);
                    const float* inputChannels[1] = { channelData };
                    float* outputChannels[1] = { tempOutput.data() };
                    
                    stretchers[ch].process(inputChannels, numSamples, outputChannels, numSamples);
                    std::copy(tempOutput.begin(), tempOutput.end(), channelData);
                }
                
                // Apply spiral feedback if dimension > 0
                if (control3 > 0.1f) {
                    for (int i = 0; i < numSamples; ++i) {
                        channelData[i] = alienProcessor.processSpiral(channelData[i], ch);
                    }
                }
                
                // Mix with dry based on dimension
                for (int i = 0; i < numSamples; ++i) {
                    channelData[i] = dry[i] * (1.0f - control3) + channelData[i] * control3;
                }
                break;
            }
        }
        
        // Soft clipping for safety
        for (int i = 0; i < numSamples; ++i) {
            if (std::abs(channelData[i]) > 0.95f) {
                channelData[i] = std::tanh(channelData[i] * 0.7f) * 1.43f;
            }
        }
    }
}

void PitchShifter::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (index) {
            case kMode: 
                modeParam.set(value);
                DBG("Vocal Destroyer: Mode " << getCurrentMode(value));
                break;
            case kControl1: 
                control1Param.set(value);
                break;
            case kControl2: 
                control2Param.set(value);
                break;
            case kControl3: 
                control3Param.set(value);
                break;
        }
    }
}

juce::String PitchShifter::getParameterName(int index) const {
    if (index == kMode) return "Mode";
    return getModeParameterName(index);
}

juce::String PitchShifter::getModeParameterName(int paramIndex) const {
    switch (currentMode) {
        case MODE_GENDER:
            switch (paramIndex) {
                case kControl1: return "Gender";
                case kControl2: return "Age";
                case kControl3: return "Intensity";
            }
            break;
            
        case MODE_GLITCH:
            switch (paramIndex) {
                case kControl1: return "Slice";
                case kControl2: return "Scatter";
                case kControl3: return "Freeze";
            }
            break;
            
        case MODE_ALIEN:
            switch (paramIndex) {
                case kControl1: return "Species";
                case kControl2: return "Evolution";
                case kControl3: return "Dimension";
            }
            break;
    }
    return "";
}

juce::String PitchShifter::getParameterDisplayString(int index, float value) const {
    if (index == kMode) {
        Mode mode = getCurrentMode(value);
        switch (mode) {
            case MODE_GENDER: return "Gender Bender";
            case MODE_GLITCH: return "Glitch Machine";
            case MODE_ALIEN: return "Alien Transform";
        }
    }
    
    return getModeParameterDisplay(index, value);
}

juce::String PitchShifter::getModeParameterDisplay(int paramIndex, float value) const {
    switch (currentMode) {
        case MODE_GENDER:
            switch (paramIndex) {
                case kControl1: { // Gender
                    int percent = static_cast<int>((value - 0.5f) * 200.0f);
                    if (percent < -50) return "Male";
                    else if (percent > 50) return "Female";
                    else if (percent > 0) return "+" + juce::String(percent) + "%";
                    else if (percent < 0) return juce::String(percent) + "%";
                    else return "Neutral";
                }
                case kControl2: { // Age
                    if (value < 0.25f) return "Child";
                    else if (value < 0.45f) return "Teen";
                    else if (value < 0.55f) return "Adult";
                    else if (value < 0.75f) return "Middle Age";
                    else return "Elderly";
                }
                case kControl3: // Intensity
                    return juce::String(static_cast<int>(value * 100)) + "%";
            }
            break;
            
        case MODE_GLITCH:
            switch (paramIndex) {
                case kControl1: { // Slice
                    if (value < 0.2f) return "1/32";
                    else if (value < 0.4f) return "1/16";
                    else if (value < 0.6f) return "1/8";
                    else if (value < 0.8f) return "1/4";
                    else return "1/2";
                }
                case kControl2: // Scatter
                    return juce::String(static_cast<int>(value * 100)) + "%";
                case kControl3: // Freeze
                    return value > 0.5f ? "Frozen" : "Live";
            }
            break;
            
        case MODE_ALIEN:
            switch (paramIndex) {
                case kControl1: { // Species
                    if (value < 0.2f) return "Martian";
                    else if (value < 0.4f) return "Reptilian";
                    else if (value < 0.6f) return "Insectoid";
                    else if (value < 0.8f) return "Crystalline";
                    else return "Void Being";
                }
                case kControl2: { // Evolution
                    if (value < 0.01f) return "Static";
                    else if (value < 0.5f) return "Slow";
                    else return "Rapid";
                }
                case kControl3: // Dimension
                    return juce::String(static_cast<int>(value * 100)) + "%";
            }
            break;
    }
    return "";
}

// --- Implementation of processor methods ---

void PitchShifter::GenderProcessor::process(float& formantRatio, float& pitchRatio, 
                                           float control1, float control2, float control3) {
    // Gender (control1): -100% male to +100% female
    float gender = (control1 - 0.5f) * 2.0f;
    formantRatio = std::pow(2.0f, gender * 0.5f); // ±0.5 octave formant shift
    
    // Age (control2): affects both pitch and formant
    float age = control2;
    if (age < 0.5f) {
        // Younger = higher pitch and formant
        float youngness = (0.5f - age) * 2.0f;
        pitchRatio *= std::pow(2.0f, youngness * 0.25f); // Up to +3 semitones
        formantRatio *= std::pow(2.0f, youngness * 0.15f);
    } else {
        // Older = lower pitch, neutral formant
        float oldness = (age - 0.5f) * 2.0f;
        pitchRatio *= std::pow(2.0f, -oldness * 0.15f); // Down to -2 semitones
    }
    
    // Intensity (control3) is handled in main process as mix
    intensity = control3;
}

float PitchShifter::GenderProcessor::calculateCompensation(float formantRatio) {
    // Compensate for perceived volume changes with formant shifting
    if (formantRatio > 1.0f) {
        return 1.0f / std::sqrt(formantRatio); // Reduce gain for higher formants
    } else {
        return std::sqrt(2.0f - formantRatio); // Boost gain for lower formants
    }
}

void PitchShifter::GlitchProcessor::updateSliceSize(float control1, double sampleRate) {
    // Map to musical divisions
    float division;
    if (control1 < 0.2f) division = 32.0f;
    else if (control1 < 0.4f) division = 16.0f;
    else if (control1 < 0.6f) division = 8.0f;
    else if (control1 < 0.8f) division = 4.0f;
    else division = 2.0f;
    
    // Assume 120 BPM for now (could sync to host tempo)
    float beatsPerSecond = 120.0f / 60.0f;
    float sliceTime = 1.0f / (beatsPerSecond * division);
    sliceSize = static_cast<int>(sliceTime * sampleRate);
    sliceSize = std::max(64, std::min(sliceSize, MAX_BUFFER_SIZE / 2));
}

float PitchShifter::GlitchProcessor::process(float input, int channel, float scatter, bool freeze) {
    // Write to buffer
    if (!freeze) {
        buffers[channel][writePos] = input;
    }
    
    // Calculate read position with scatter
    int readPos = writePos;
    if (scatter > 0.01f && !freeze) {
        // Random jump within slice
        int scatterAmount = static_cast<int>(scatter * sliceSize * 0.5f);
        readPos = (writePos - (rand() % scatterAmount) + MAX_BUFFER_SIZE) % MAX_BUFFER_SIZE;
    }
    
    // Read from buffer (with wrapping)
    float output = buffers[channel][readPos];
    
    // Advance write position
    if (!freeze) {
        writePos = (writePos + 1) % sliceSize;
        if (writePos == 0) {
            // New slice
            currentSlice = (currentSlice + 1) % 4;
        }
    }
    
    return output;
}

void PitchShifter::GlitchProcessor::reset() {
    for (auto& buffer : buffers) {
        buffer.fill(0.0f);
    }
    writePos = 0;
    currentSlice = 0;
    frozen = false;
    crossfadePos = 1.0f;
}

void PitchShifter::AlienProcessor::process(float& formantRatio, float& pitchRatio,
                                          float control1, float control2, float control3,
                                          double sampleRate) {
    // Species (control1): Different alien characteristics
    float species = control1;
    if (species < 0.2f) {
        // Martian - deep and resonant
        formantRatio = 0.6f;
        pitchRatio = 0.8f;
    } else if (species < 0.4f) {
        // Reptilian - hissing, sibilant
        formantRatio = 1.3f;
        pitchRatio = 1.1f;
    } else if (species < 0.6f) {
        // Insectoid - buzzing, high frequency
        formantRatio = 1.8f;
        pitchRatio = 1.5f;
    } else if (species < 0.8f) {
        // Crystalline - metallic, harmonic
        formantRatio = 1.0f;
        pitchRatio = 2.0f;
    } else {
        // Void Being - otherworldly
        formantRatio = 0.4f;
        pitchRatio = 0.5f;
    }
    
    // Evolution (control2): Add modulation
    if (control2 > 0.01f) {
        lfoPhase += (control2 * 10.0f) / sampleRate;
        if (lfoPhase > 2.0f * M_PI) lfoPhase -= 2.0f * M_PI;
        
        float lfo = std::sin(lfoPhase);
        formantRatio *= (1.0f + lfo * control2 * 0.3f);
        pitchRatio *= (1.0f + lfo * control2 * 0.1f);
        
        // Add some randomness
        if (rand() % 100 < control2 * 50) {
            formantRatio *= (1.0f + dist(rng) * 0.1f);
        }
    }
    
    // Dimension (control3): Handled as feedback in main process
    dimension = control3;
}

float PitchShifter::AlienProcessor::processSpiral(float input, int channel) {
    // Simple feedback delay for dimensional warping
    float delayed = spiralBuffers[channel][spiralPos[channel]];
    spiralBuffers[channel][spiralPos[channel]] = input + delayed * 0.5f * dimension;
    spiralPos[channel] = (spiralPos[channel] + 1) % spiralBuffers[channel].size();
    
    return input + delayed * dimension;
}

bool PitchShifter::TransientDetector::detectTransient(float input) {
    float rectified = std::abs(input);
    
    // Envelope follower
    float attack = std::exp(-1.0f / (attackTime * 44100));
    float release = std::exp(-1.0f / (releaseTime * 44100));
    
    if (rectified > envelope) {
        envelope = rectified + (envelope - rectified) * attack;
    } else {
        envelope = rectified + (envelope - rectified) * release;
    }
    
    // Detect sudden increases
    float threshold = 0.3f;
    bool isTransient = (rectified > envelope * (1.0f + threshold));
    
    return isTransient;
}

float PitchShifter::applyMusicalCurve(float input, CurveType type) const {
    switch (type) {
        case EXPONENTIAL:
            return input * input;
        case LOGARITHMIC:
            return std::sqrt(input);
        case S_CURVE:
            return input * input * (3.0f - 2.0f * input);
        case LINEAR:
        default:
            return input;
    }
}