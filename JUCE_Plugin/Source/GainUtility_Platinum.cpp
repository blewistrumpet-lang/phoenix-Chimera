// GainUtility_Platinum.cpp - Professional Gain Control Implementation
#include "JuceHeader.h"
#include "GainUtility_Platinum.h"
#include "DspEngineUtilities.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <deque>

namespace {
    constexpr float EPSILON = 1e-10f;
    constexpr float DB_TO_LINEAR = 0.05776226504666210911810267678818f;  // ln(10)/20
    constexpr float LINEAR_TO_DB = 17.312340490667560888319096172023f;  // 20/ln(10)
    constexpr float MINUS_INF_DB = -144.0f;
    
    inline float dbToLinear(float db) {
        return (db > MINUS_INF_DB) ? std::exp(db * DB_TO_LINEAR) : 0.0f;
    }
    
    inline float linearToDb(float linear) {
        return (linear > EPSILON) ? std::log(linear) * LINEAR_TO_DB : MINUS_INF_DB;
    }
}

//==============================================================================
// Implementation Class
//==============================================================================
class GainUtility_Platinum::Impl {
public:
    //==========================================================================
    // True Peak Detector with Oversampling
    //==========================================================================
    class TruePeakDetector {
        static constexpr int OVERSAMPLE_FACTOR = 4;
        static constexpr int FIR_LENGTH = 32;
        
        // Polyphase FIR coefficients for 4x oversampling
        float firCoeffs[OVERSAMPLE_FACTOR][FIR_LENGTH / OVERSAMPLE_FACTOR];
        float delayLine[FIR_LENGTH] = {};
        int delayIndex = 0;
        float truePeak = 0.0f;
        
    public:
        TruePeakDetector() {
            // Initialize sinc interpolation filter
            for (int phase = 0; phase < OVERSAMPLE_FACTOR; ++phase) {
                for (int tap = 0; tap < FIR_LENGTH / OVERSAMPLE_FACTOR; ++tap) {
                    const float n = tap * OVERSAMPLE_FACTOR + phase - FIR_LENGTH / 2.0f;
                    if (std::abs(n) < EPSILON) {
                        firCoeffs[phase][tap] = 1.0f;
                    } else {
                        const float x = n * 3.14159f / OVERSAMPLE_FACTOR;
                        const float sinc = std::sin(x) / x;
                        const float window = 0.5f + 0.5f * std::cos(2.0f * 3.14159f * n / FIR_LENGTH);
                        firCoeffs[phase][tap] = sinc * window;
                    }
                }
            }
        }
        
        float process(float input) {
            // Update delay line
            delayLine[delayIndex] = input;
            delayIndex = (delayIndex + 1) % FIR_LENGTH;
            
            // Interpolate at 4x rate
            float maxSample = std::abs(input);
            
            for (int phase = 0; phase < OVERSAMPLE_FACTOR; ++phase) {
                float interpolated = 0.0f;
                
                for (int tap = 0; tap < FIR_LENGTH / OVERSAMPLE_FACTOR; ++tap) {
                    const int delayTap = (delayIndex + tap * OVERSAMPLE_FACTOR) % FIR_LENGTH;
                    interpolated += delayLine[delayTap] * firCoeffs[phase][tap];
                }
                
                maxSample = std::max(maxSample, std::abs(interpolated));
            }
            
            // Update peak with fast attack, slow release
            const float attack = 0.0f;    // Instant attack
            const float release = 0.9999f; // ~3 second release at 48kHz
            
            if (maxSample > truePeak) {
                truePeak = maxSample;
            } else {
                truePeak *= release;
            }
            
            return truePeak;
        }
        
        float getTruePeak() const { return truePeak; }
        void reset() { truePeak = 0.0f; std::fill(std::begin(delayLine), std::end(delayLine), 0.0f); }
    };
    
    //==========================================================================
    // RMS Meter
    //==========================================================================
    class RMSMeter {
        static constexpr size_t WINDOW_SIZE = 8192;  // ~170ms at 48kHz
        float buffer[WINDOW_SIZE] = {};
        size_t writePos = 0;
        float sum = 0.0f;
        float currentRMS = 0.0f;
        
    public:
        float process(float input) {
            const float squared = input * input;
            
            // Update running sum
            sum -= buffer[writePos];
            sum += squared;
            buffer[writePos] = squared;
            writePos = (writePos + 1) % WINDOW_SIZE;
            
            // Calculate RMS
            currentRMS = std::sqrt(sum / WINDOW_SIZE);
            return currentRMS;
        }
        
        float getRMS() const { return currentRMS; }
        
        void reset() {
            std::fill(std::begin(buffer), std::end(buffer), 0.0f);
            sum = 0.0f;
            currentRMS = 0.0f;
            writePos = 0;
        }
    };
    
    //==========================================================================
    // LUFS Meter (ITU-R BS.1770-4)
    //==========================================================================
    class LUFSMeter {
        // K-weighting filter coefficients
        struct KWeightingFilter {
            // Stage 1: Shelving filter
            float b0_1 = 1.53091f, b1_1 = -2.69169f, b2_1 = 1.19839f;
            float a1_1 = -1.69065f, a2_1 = 0.73248f;
            float x1_1 = 0.0f, x2_1 = 0.0f, y1_1 = 0.0f, y2_1 = 0.0f;
            
            // Stage 2: High-pass filter
            float b0_2 = 1.0f, b1_2 = -2.0f, b2_2 = 1.0f;
            float a1_2 = -1.99004f, a2_2 = 0.99007f;
            float x1_2 = 0.0f, x2_2 = 0.0f, y1_2 = 0.0f, y2_2 = 0.0f;
            
            float process(float input) {
                // Stage 1
                float y1 = b0_1 * input + b1_1 * x1_1 + b2_1 * x2_1 - a1_1 * y1_1 - a2_1 * y2_1;
                x2_1 = x1_1; x1_1 = input;
                y2_1 = y1_1; y1_1 = y1;
                
                // Stage 2
                float y2 = b0_2 * y1 + b1_2 * x1_2 + b2_2 * x2_2 - a1_2 * y1_2 - a2_2 * y2_2;
                x2_2 = x1_2; x1_2 = y1;
                y2_2 = y1_2; y1_2 = y2;
                
                return y2;
            }
            
            void setSampleRate(float sr) {
                // Update coefficients based on sample rate
                // These are pre-calculated for common rates
                if (sr < 50000) { // 44.1/48kHz
                    // Already set to defaults
                } else if (sr < 100000) { // 88.2/96kHz
                    b0_1 = 1.53660026f; b1_1 = -2.68908427f; b2_1 = 1.16158667f;
                    a1_1 = -1.68859930f; a2_1 = 0.69708464f;
                    a1_2 = -1.99517455f; a2_2 = 0.99520193f;
                }
            }
            
            void reset() {
                x1_1 = x2_1 = y1_1 = y2_1 = 0.0f;
                x1_2 = x2_2 = y1_2 = y2_2 = 0.0f;
            }
        };
        
        KWeightingFilter kFilterL, kFilterR;
        
        // Gating windows
        std::deque<float> momentaryWindow;  // 400ms
        std::deque<float> shortTermWindow;  // 3s
        std::vector<float> integratedBlocks; // All 400ms blocks
        
        size_t momentarySize = 19200;   // 400ms at 48kHz
        size_t shortTermSize = 144000;  // 3s at 48kHz
        
        float momentaryLoudness = -70.0f;
        float shortTermLoudness = -70.0f;
        float integratedLoudness = -70.0f;
        
        float sampleRate = 48000.0f;
        size_t sampleCounter = 0;
        float blockSum = 0.0f;
        
    public:
        void setSampleRate(float sr) {
            sampleRate = sr;
            momentarySize = static_cast<size_t>(0.4f * sr);
            shortTermSize = static_cast<size_t>(3.0f * sr);
            
            kFilterL.setSampleRate(sr);
            kFilterR.setSampleRate(sr);
        }
        
        void process(float left, float right) {
            // Apply K-weighting
            const float kLeft = kFilterL.process(left);
            const float kRight = kFilterR.process(right);
            
            // Calculate mean square
            const float meanSquare = (kLeft * kLeft + kRight * kRight) * 0.5f;
            
            // Add to windows
            if (momentaryWindow.size() >= momentarySize) {
                momentaryWindow.pop_front();
            }
            momentaryWindow.push_back(meanSquare);
            
            if (shortTermWindow.size() >= shortTermSize) {
                shortTermWindow.pop_front();
            }
            shortTermWindow.push_back(meanSquare);
            
            // Accumulate for integrated measurement
            blockSum += meanSquare;
            sampleCounter++;
            
            // Every 100ms, calculate loudness values
            if (sampleCounter >= sampleRate * 0.1f) {
                updateLoudness();
                sampleCounter = 0;
            }
        }
        
        float getMomentary() const { return momentaryLoudness; }
        float getShortTerm() const { return shortTermLoudness; }
        float getIntegrated() const { return integratedLoudness; }
        
        void reset() {
            kFilterL.reset();
            kFilterR.reset();
            momentaryWindow.clear();
            shortTermWindow.clear();
            integratedBlocks.clear();
            blockSum = 0.0f;
            sampleCounter = 0;
            momentaryLoudness = shortTermLoudness = integratedLoudness = -70.0f;
        }
        
    private:
        void updateLoudness() {
            // Momentary loudness
            if (!momentaryWindow.empty()) {
                float sum = std::accumulate(momentaryWindow.begin(), momentaryWindow.end(), 0.0f);
                float meanSquare = sum / momentaryWindow.size();
                momentaryLoudness = -0.691f + 10.0f * std::log10(meanSquare + EPSILON);
            }
            
            // Short-term loudness
            if (!shortTermWindow.empty()) {
                float sum = std::accumulate(shortTermWindow.begin(), shortTermWindow.end(), 0.0f);
                float meanSquare = sum / shortTermWindow.size();
                shortTermLoudness = -0.691f + 10.0f * std::log10(meanSquare + EPSILON);
            }
            
            // Integrated loudness (gated)
            if (sampleCounter > 0 && blockSum / sampleCounter > EPSILON) {
                float blockLoudness = -0.691f + 10.0f * std::log10(blockSum / sampleCounter);
                
                // Only include blocks above -70 LUFS (absolute gate)
                if (blockLoudness > -70.0f) {
                    integratedBlocks.push_back(blockLoudness);
                }
                
                // Calculate gated loudness
                if (!integratedBlocks.empty()) {
                    // First pass: calculate ungated mean
                    float sum = 0.0f;
                    for (float block : integratedBlocks) {
                        sum += std::pow(10.0f, block * 0.1f);
                    }
                    float ungatedMean = 10.0f * std::log10(sum / integratedBlocks.size());
                    
                    // Second pass: relative gate at -10 LU
                    float relativeGate = ungatedMean - 10.0f;
                    sum = 0.0f;
                    int count = 0;
                    
                    for (float block : integratedBlocks) {
                        if (block > relativeGate) {
                            sum += std::pow(10.0f, block * 0.1f);
                            count++;
                        }
                    }
                    
                    if (count > 0) {
                        integratedLoudness = 10.0f * std::log10(sum / count);
                    }
                }
                
                blockSum = 0.0f;
            }
        }
    };
    
    //==========================================================================
    // A/B State Storage
    //==========================================================================
    struct State {
        float gain = 0.0f;
        float gainL = 0.0f;
        float gainR = 0.0f;
        float gainMid = 0.0f;
        float gainSide = 0.0f;
        float mode = 0.0f;
        bool phaseL = false;
        bool phaseR = false;
        bool channelSwap = false;
        bool autoGain = false;
    };
    
    //==========================================================================
    // Main Implementation
    //==========================================================================
    
    // Processing components
    TruePeakDetector truePeakL, truePeakR;
    RMSMeter rmsL, rmsR;
    LUFSMeter lufsMeter;
    
    // Parameters
    struct Parameters {
        std::atomic<float> gain{0.0f};
        std::atomic<float> gainL{0.0f};
        std::atomic<float> gainR{0.0f};
        std::atomic<float> gainMid{0.0f};
        std::atomic<float> gainSide{0.0f};
        std::atomic<float> mode{0.0f};
        std::atomic<float> phaseL{0.0f};
        std::atomic<float> phaseR{0.0f};
        std::atomic<float> channelSwap{0.0f};
        std::atomic<float> autoGain{0.0f};
    } params;
    
    // Parameter smoothing
    struct Smoother {
        float current = 0.0f;
        float target = 0.0f;
        float coeff = 0.0f;
        
        void setCoeff(double sampleRate, float timeMs) {
            coeff = std::exp(-1.0f / (sampleRate * timeMs * 0.001f));
        }
        
        void setTarget(float t) { target = t; }
        void reset(float value) { current = target = value; }
        
        float tick() {
            current += (target - current) * (1.0f - coeff);
            return current;
        }
    };
    
    struct {
        Smoother gain, gainL, gainR, gainMid, gainSide;
    } smoothers;
    
    // State management
    State stateA, stateB;
    State* currentState = &stateA;
    
    // Metering data
    std::atomic<float> peakL{0.0f}, peakR{0.0f};
    std::atomic<float> currentRmsL{0.0f}, currentRmsR{0.0f};
    std::atomic<float> currentTruePeakL{0.0f}, currentTruePeakR{0.0f};
    std::atomic<float> outputGainDb{0.0f};
    
    float sampleRate = 48000.0f;
    
    //==========================================================================
    // Constructor
    //==========================================================================
    Impl() {
        // Denormal prevention is now handled by DenormalGuard
    }
    
    //==========================================================================
    // Processing
    //==========================================================================
    void process(juce::AudioBuffer<float>& buffer) {
        DenormalGuard guard;
        
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        if (numChannels < 2 || numSamples == 0) return;
        
        // Update mode
        const float modeValue = params.mode.load();
        const bool isStereo = modeValue < 0.33f;
        const bool isMidSide = modeValue > 0.33f && modeValue < 0.67f;
        const bool isMono = modeValue > 0.67f;
        
        // Update smoothers
        const float mainGainDb = params.gain.load() * 48.0f - 24.0f;  // -24 to +24 dB
        smoothers.gain.setTarget(dbToLinear(mainGainDb));
        
        if (isStereo) {
            const float gainLDb = (params.gainL.load() - 0.5f) * 24.0f;  // -12 to +12 dB
            const float gainRDb = (params.gainR.load() - 0.5f) * 24.0f;
            smoothers.gainL.setTarget(dbToLinear(gainLDb));
            smoothers.gainR.setTarget(dbToLinear(gainRDb));
        } else if (isMidSide) {
            const float gainMidDb = (params.gainMid.load() - 0.5f) * 24.0f;
            const float gainSideDb = (params.gainSide.load() - 0.5f) * 24.0f;
            smoothers.gainMid.setTarget(dbToLinear(gainMidDb));
            smoothers.gainSide.setTarget(dbToLinear(gainSideDb));
        }
        
        // Phase and swap settings
        const bool invertL = params.phaseL.load() > 0.5f;
        const bool invertR = params.phaseR.load() > 0.5f;
        const bool swap = params.channelSwap.load() > 0.5f;
        
        // Process audio
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        
        // Peak tracking
        float maxL = 0.0f, maxR = 0.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            float inL = left[i];
            float inR = right[i];
            
            // Channel swap if enabled
            if (swap) {
                std::swap(inL, inR);
            }
            
            // Apply main gain
            const float mainGain = smoothers.gain.tick();
            inL *= mainGain;
            inR *= mainGain;
            
            // Mode-specific processing
            if (isStereo) {
                // Stereo mode - independent L/R gains
                inL *= smoothers.gainL.tick();
                inR *= smoothers.gainR.tick();
            } else if (isMidSide) {
                // M/S mode
                const float mid = (inL + inR) * 0.5f;
                const float side = (inL - inR) * 0.5f;
                
                const float processedMid = mid * smoothers.gainMid.tick();
                const float processedSide = side * smoothers.gainSide.tick();
                
                inL = processedMid + processedSide;
                inR = processedMid - processedSide;
            } else if (isMono) {
                // Mono mode
                const float mono = (inL + inR) * 0.5f;
                inL = inR = mono;
            }
            
            // Phase inversion
            if (invertL) inL = -inL;
            if (invertR) inR = -inR;
            
            // Update meters (pre-limiter)
            maxL = std::max(maxL, std::abs(inL));
            maxR = std::max(maxR, std::abs(inR));
            
            // Safety limiter at 0dBFS
            inL = std::clamp(inL, -1.0f, 1.0f);
            inR = std::clamp(inR, -1.0f, 1.0f);
            
            // Write output
            left[i] = inL;
            right[i] = inR;
            
            // Update meters
            truePeakL.process(inL);
            truePeakR.process(inR);
            rmsL.process(inL);
            rmsR.process(inR);
            lufsMeter.process(inL, inR);
        }
        
        // Update peak meters with ballistics
        const float peakRelease = 0.99f;  // ~20ms at 48kHz
        
        float currentPeakL = peakL.load();
        float currentPeakR = peakR.load();
        
        if (maxL > currentPeakL) {
            currentPeakL = maxL;
        } else {
            currentPeakL *= peakRelease;
        }
        
        if (maxR > currentPeakR) {
            currentPeakR = maxR;
        } else {
            currentPeakR *= peakRelease;
        }
        
        peakL.store(currentPeakL);
        peakR.store(currentPeakR);
        
        // Update other meters
        currentRmsL.store(rmsL.getRMS());
        currentRmsR.store(rmsR.getRMS());
        currentTruePeakL.store(truePeakL.getTruePeak());
        currentTruePeakR.store(truePeakR.getTruePeak());
        
        // Calculate output gain for display
        outputGainDb.store(mainGainDb);
        
        // Apply final NaN/Inf cleanup
        scrubBuffer(buffer);
    }
    
    void prepareToPlay(double sr, int samplesPerBlock) {
        sampleRate = static_cast<float>(sr);
        
        // Setup smoothers (fast for precision control)
        smoothers.gain.setCoeff(sr, 5.0f);
        smoothers.gainL.setCoeff(sr, 5.0f);
        smoothers.gainR.setCoeff(sr, 5.0f);
        smoothers.gainMid.setCoeff(sr, 5.0f);
        smoothers.gainSide.setCoeff(sr, 5.0f);
        
        // Initialize smoothers
        smoothers.gain.reset(1.0f);
        smoothers.gainL.reset(1.0f);
        smoothers.gainR.reset(1.0f);
        smoothers.gainMid.reset(1.0f);
        smoothers.gainSide.reset(1.0f);
        
        // Setup meters
        lufsMeter.setSampleRate(sampleRate);
        
        reset();
    }
    
    void reset() {
        truePeakL.reset();
        truePeakR.reset();
        rmsL.reset();
        rmsR.reset();
        lufsMeter.reset();
        
        peakL.store(0.0f);
        peakR.store(0.0f);
        currentRmsL.store(0.0f);
        currentRmsR.store(0.0f);
        currentTruePeakL.store(0.0f);
        currentTruePeakR.store(0.0f);
    }
    
    void saveState(int slot) {
        State* targetState = (slot == 0) ? &stateA : &stateB;
        
        targetState->gain = params.gain.load();
        targetState->gainL = params.gainL.load();
        targetState->gainR = params.gainR.load();
        targetState->gainMid = params.gainMid.load();
        targetState->gainSide = params.gainSide.load();
        targetState->mode = params.mode.load();
        targetState->phaseL = params.phaseL.load() > 0.5f;
        targetState->phaseR = params.phaseR.load() > 0.5f;
        targetState->channelSwap = params.channelSwap.load() > 0.5f;
        targetState->autoGain = params.autoGain.load() > 0.5f;
    }
    
    void recallState(int slot) {
        const State* sourceState = (slot == 0) ? &stateA : &stateB;
        
        params.gain.store(sourceState->gain);
        params.gainL.store(sourceState->gainL);
        params.gainR.store(sourceState->gainR);
        params.gainMid.store(sourceState->gainMid);
        params.gainSide.store(sourceState->gainSide);
        params.mode.store(sourceState->mode);
        params.phaseL.store(sourceState->phaseL ? 1.0f : 0.0f);
        params.phaseR.store(sourceState->phaseR ? 1.0f : 0.0f);
        params.channelSwap.store(sourceState->channelSwap ? 1.0f : 0.0f);
        params.autoGain.store(sourceState->autoGain ? 1.0f : 0.0f);
    }
};

//==============================================================================
// Public Interface Implementation
//==============================================================================

GainUtility_Platinum::GainUtility_Platinum() : pImpl(std::make_unique<Impl>()) {}
GainUtility_Platinum::~GainUtility_Platinum() = default;

void GainUtility_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->prepareToPlay(sampleRate, samplesPerBlock);
}

void GainUtility_Platinum::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void GainUtility_Platinum::reset() {
    pImpl->reset();
}

void GainUtility_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (static_cast<ParamID>(index)) {
            case ParamID::GAIN:         pImpl->params.gain.store(value); break;
            case ParamID::GAIN_L:       pImpl->params.gainL.store(value); break;
            case ParamID::GAIN_R:       pImpl->params.gainR.store(value); break;
            case ParamID::GAIN_MID:     pImpl->params.gainMid.store(value); break;
            case ParamID::GAIN_SIDE:    pImpl->params.gainSide.store(value); break;
            case ParamID::MODE:         pImpl->params.mode.store(value); break;
            case ParamID::PHASE_L:      pImpl->params.phaseL.store(value); break;
            case ParamID::PHASE_R:      pImpl->params.phaseR.store(value); break;
            case ParamID::CHANNEL_SWAP: pImpl->params.channelSwap.store(value); break;
            case ParamID::AUTO_GAIN:    pImpl->params.autoGain.store(value); break;
        }
    }
}

juce::String GainUtility_Platinum::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::GAIN:         return "Gain";
        case ParamID::GAIN_L:       return "Left Gain";
        case ParamID::GAIN_R:       return "Right Gain";
        case ParamID::GAIN_MID:     return "Mid Gain";
        case ParamID::GAIN_SIDE:    return "Side Gain";
        case ParamID::MODE:         return "Mode";
        case ParamID::PHASE_L:      return "Phase L";
        case ParamID::PHASE_R:      return "Phase R";
        case ParamID::CHANNEL_SWAP: return "Channel Swap";
        case ParamID::AUTO_GAIN:    return "Auto Gain";
        default:                    return "";
    }
}

GainUtility_Platinum::MeteringData GainUtility_Platinum::getMetering() const {
    MeteringData data;
    data.peakL = pImpl->peakL.load();
    data.peakR = pImpl->peakR.load();
    data.rmsL = pImpl->currentRmsL.load();
    data.rmsR = pImpl->currentRmsR.load();
    data.lufsM = pImpl->lufsMeter.getMomentary();
    data.lufsS = pImpl->lufsMeter.getShortTerm();
    data.lufsI = pImpl->lufsMeter.getIntegrated();
    data.truePeakL = pImpl->currentTruePeakL.load();
    data.truePeakR = pImpl->currentTruePeakR.load();
    data.gainReduction = 0.0f;  // No compression in gain utility
    data.outputGain = pImpl->outputGainDb.load();
    return data;
}

void GainUtility_Platinum::saveState(int slot) {
    pImpl->saveState(slot);
}

void GainUtility_Platinum::recallState(int slot) {
    pImpl->recallState(slot);
}

void GainUtility_Platinum::matchGain(int toSlot) {
    // This would analyze the loudness of the current signal
    // and adjust gain to match the saved state's loudness
    // Placeholder for full implementation
}

float GainUtility_Platinum::getIntegratedLoudness() const {
    return pImpl->lufsMeter.getIntegrated();
}

void GainUtility_Platinum::resetLoudnessMeters() {
    pImpl->lufsMeter.reset();
}

std::array<float, 2> GainUtility_Platinum::getPhaseCorrelation() const {
    // Would calculate L/R correlation
    // Placeholder returns perfect correlation
    return {1.0f, 1.0f};
}