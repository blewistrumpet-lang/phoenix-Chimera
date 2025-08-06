// PhaseAlign_Platinum.cpp - Professional Phase Alignment Implementation
#include "PhaseAlign_Platinum.h"
#include <cmath>
#include <algorithm>
#include <numeric>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

namespace {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    constexpr float EPSILON = 1e-10f;
}

//==============================================================================
// Implementation Class
//==============================================================================
class PhaseAlign_Platinum::Impl {
public:
    //==========================================================================
    // All-pass Filter for Phase Rotation
    //==========================================================================
    class AllpassFilter {
        float a1 = 0.0f;
        float a2 = 0.0f;
        float b0 = 0.0f;
        float b1 = 0.0f;
        float b2 = 0.0f;
        
        // State variables (per channel)
        struct State {
            float x1 = 0.0f, x2 = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;
        };
        
        State states[2];  // Stereo
        
    public:
        void setPhaseShift(float phaseRadians, float frequencyHz, float sampleRate) {
            // Design all-pass filter for specific phase shift at frequency
            const float omega = TWO_PI * frequencyHz / sampleRate;
            const float phi = phaseRadians;
            
            // Calculate filter coefficients
            const float alpha = std::sin(omega) / 2.0f;
            const float cosw = std::cos(omega);
            
            // All-pass design
            const float norm = 1.0f / (1.0f + alpha);
            
            b0 = (1.0f - alpha) * norm;
            b1 = -2.0f * cosw * norm;
            b2 = (1.0f + alpha) * norm;
            a1 = b1;  // All-pass property
            a2 = b0;  // All-pass property
        }
        
        float process(float input, int channel) {
            State& s = states[channel];
            
            // Direct Form II
            const float w = input - a1 * s.y1 - a2 * s.y2;
            const float y = b0 * w + b1 * s.x1 + b2 * s.x2;
            
            // Update states
            s.x2 = s.x1;
            s.x1 = w;
            s.y2 = s.y1;
            s.y1 = y;
            
            // Denormal prevention
            if (std::abs(s.y1) < EPSILON) s.y1 = 0.0f;
            if (std::abs(s.y2) < EPSILON) s.y2 = 0.0f;
            
            return y;
        }
        
        void reset() {
            for (auto& state : states) {
                state.x1 = state.x2 = 0.0f;
                state.y1 = state.y2 = 0.0f;
            }
        }
    };
    
    //==========================================================================
    // Linkwitz-Riley Crossover for Band Splitting
    //==========================================================================
    class CrossoverFilter {
        struct Biquad {
            float b0, b1, b2, a1, a2;
            float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
            
            float process(float input) {
                const float y = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                x2 = x1; x1 = input;
                y2 = y1; y1 = y;
                return y;
            }
            
            void reset() {
                x1 = x2 = y1 = y2 = 0.0f;
            }
        };
        
        Biquad lowpass[2][2];   // [channel][stage]
        Biquad highpass[2][2];  // [channel][stage]
        
    public:
        void setFrequency(float freq, float sampleRate) {
            // Butterworth coefficients for LR4 (cascade of 2x LR2)
            const float omega = TWO_PI * freq / sampleRate;
            const float cosw = std::cos(omega);
            const float sinw = std::sin(omega);
            const float sqrt2 = std::sqrt(2.0f);
            const float alpha = sinw / sqrt2;
            
            const float norm = 1.0f / (1.0f + alpha);
            
            // Low-pass coefficients
            const float lpb0 = (1.0f - cosw) * 0.5f * norm;
            const float lpb1 = (1.0f - cosw) * norm;
            const float lpb2 = lpb0;
            const float lpa1 = -2.0f * cosw * norm;
            const float lpa2 = (1.0f - alpha) * norm;
            
            // High-pass coefficients
            const float hpb0 = (1.0f + cosw) * 0.5f * norm;
            const float hpb1 = -(1.0f + cosw) * norm;
            const float hpb2 = hpb0;
            const float hpa1 = lpa1;
            const float hpa2 = lpa2;
            
            // Set coefficients for all stages
            for (int ch = 0; ch < 2; ++ch) {
                for (int stage = 0; stage < 2; ++stage) {
                    lowpass[ch][stage].b0 = lpb0;
                    lowpass[ch][stage].b1 = lpb1;
                    lowpass[ch][stage].b2 = lpb2;
                    lowpass[ch][stage].a1 = lpa1;
                    lowpass[ch][stage].a2 = lpa2;
                    
                    highpass[ch][stage].b0 = hpb0;
                    highpass[ch][stage].b1 = hpb1;
                    highpass[ch][stage].b2 = hpb2;
                    highpass[ch][stage].a1 = hpa1;
                    highpass[ch][stage].a2 = hpa2;
                }
            }
        }
        
        void process(float input, float& low, float& high, int channel) {
            // Process through cascaded stages
            low = lowpass[channel][0].process(input);
            low = lowpass[channel][1].process(low);
            
            high = highpass[channel][0].process(input);
            high = highpass[channel][1].process(high);
        }
        
        void reset() {
            for (int ch = 0; ch < 2; ++ch) {
                for (int stage = 0; stage < 2; ++stage) {
                    lowpass[ch][stage].reset();
                    highpass[ch][stage].reset();
                }
            }
        }
    };
    
    //==========================================================================
    // Phase Correlation Analyzer
    //==========================================================================
    class CorrelationAnalyzer {
        static constexpr size_t WINDOW_SIZE = 1024;
        float bufferL[WINDOW_SIZE] = {};
        float bufferR[WINDOW_SIZE] = {};
        size_t writePos = 0;
        float correlation = 0.0f;
        
    public:
        void process(float left, float right) {
            bufferL[writePos] = left;
            bufferR[writePos] = right;
            writePos = (writePos + 1) % WINDOW_SIZE;
            
            // Update correlation every 64 samples
            if ((writePos % 64) == 0) {
                updateCorrelation();
            }
        }
        
        float getCorrelation() const { return correlation; }
        
    private:
        void updateCorrelation() {
            float sumL = 0.0f, sumR = 0.0f;
            float sumL2 = 0.0f, sumR2 = 0.0f;
            float sumLR = 0.0f;
            
            for (size_t i = 0; i < WINDOW_SIZE; ++i) {
                const float l = bufferL[i];
                const float r = bufferR[i];
                
                sumL += l;
                sumR += r;
                sumL2 += l * l;
                sumR2 += r * r;
                sumLR += l * r;
            }
            
            const float meanL = sumL / WINDOW_SIZE;
            const float meanR = sumR / WINDOW_SIZE;
            
            const float varL = sumL2 / WINDOW_SIZE - meanL * meanL;
            const float varR = sumR2 / WINDOW_SIZE - meanR * meanR;
            const float covar = sumLR / WINDOW_SIZE - meanL * meanR;
            
            const float denom = std::sqrt(varL * varR);
            correlation = (denom > EPSILON) ? (covar / denom) : 0.0f;
            correlation = std::clamp(correlation, -1.0f, 1.0f);
        }
    };
    
    //==========================================================================
    // Main Implementation
    //==========================================================================
    
    // Processing components
    CrossoverFilter crossovers[3];  // Low/LowMid, LowMid/HighMid, HighMid/High
    AllpassFilter phaseFilters[4];  // One per band
    CorrelationAnalyzer correlator;
    
    // Parameters
    struct Parameters {
        std::atomic<float> autoAlign{0.0f};
        std::atomic<float> reference{0.5f};
        std::atomic<float> lowPhase{0.0f};
        std::atomic<float> lowMidPhase{0.0f};
        std::atomic<float> highMidPhase{0.0f};
        std::atomic<float> highPhase{0.0f};
        std::atomic<float> lowFreq{0.2f};
        std::atomic<float> midFreq{0.5f};
        std::atomic<float> highFreq{0.8f};
        std::atomic<float> mix{1.0f};
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
        Smoother lowPhase, lowMidPhase, highMidPhase, highPhase;
        Smoother lowFreq, midFreq, highFreq;
        Smoother mix;
    } smoothers;
    
    // State
    float sampleRate = 48000.0f;
    bool bandSolo[4] = {false, false, false, false};
    bool bandMute[4] = {false, false, false, false};
    bool globalPolarity = false;
    std::atomic<float> currentCorrelation{0.0f};
    std::array<std::atomic<float>, 4> currentPhases{};
    
    //==========================================================================
    // Constructor
    //==========================================================================
    Impl() {
        #if HAS_SSE2
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
        #endif
    }
    
    //==========================================================================
    // Processing
    //==========================================================================
    void process(juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        if (numChannels < 2 || numSamples == 0) return;
        
        // Update smoothers
        smoothers.lowPhase.setTarget(params.lowPhase.load() * 360.0f - 180.0f);
        smoothers.lowMidPhase.setTarget(params.lowMidPhase.load() * 360.0f - 180.0f);
        smoothers.highMidPhase.setTarget(params.highMidPhase.load() * 360.0f - 180.0f);
        smoothers.highPhase.setTarget(params.highPhase.load() * 360.0f - 180.0f);
        smoothers.mix.setTarget(params.mix.load());
        
        // Update crossover frequencies
        const float lowFreqHz = 20.0f * std::pow(25.0f, params.lowFreq.load());
        const float midFreqHz = 200.0f * std::pow(25.0f, params.midFreq.load());
        const float highFreqHz = 1000.0f * std::pow(15.0f, params.highFreq.load());
        
        crossovers[0].setFrequency(lowFreqHz, sampleRate);
        crossovers[1].setFrequency(midFreqHz, sampleRate);
        crossovers[2].setFrequency(highFreqHz, sampleRate);
        
        // Update phase filters
        const float lowPhaseDeg = smoothers.lowPhase.tick();
        const float lowMidPhaseDeg = smoothers.lowMidPhase.tick();
        const float highMidPhaseDeg = smoothers.highMidPhase.tick();
        const float highPhaseDeg = smoothers.highPhase.tick();
        
        phaseFilters[0].setPhaseShift(lowPhaseDeg * DEG_TO_RAD, lowFreqHz * 0.5f, sampleRate);
        phaseFilters[1].setPhaseShift(lowMidPhaseDeg * DEG_TO_RAD, (lowFreqHz + midFreqHz) * 0.5f, sampleRate);
        phaseFilters[2].setPhaseShift(highMidPhaseDeg * DEG_TO_RAD, (midFreqHz + highFreqHz) * 0.5f, sampleRate);
        phaseFilters[3].setPhaseShift(highPhaseDeg * DEG_TO_RAD, highFreqHz * 2.0f, sampleRate);
        
        // Store current phases
        currentPhases[0].store(lowPhaseDeg);
        currentPhases[1].store(lowMidPhaseDeg);
        currentPhases[2].store(highMidPhaseDeg);
        currentPhases[3].store(highPhaseDeg);
        
        // Process audio
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        
        for (int i = 0; i < numSamples; ++i) {
            const float dryL = left[i];
            const float dryR = right[i];
            
            // Determine reference channel
            const float ref = params.reference.load();
            const float refSignal = (ref < 0.25f) ? dryL : 
                                   (ref > 0.75f) ? dryR : 
                                   (dryL + dryR) * 0.5f;
            
            // Split into bands
            float bands[2][4];  // [channel][band]
            
            for (int ch = 0; ch < 2; ++ch) {
                const float input = (ch == 0) ? dryL : dryR;
                
                // First split: Low vs Rest
                float low, rest;
                crossovers[0].process(input, low, rest, ch);
                bands[ch][0] = low;
                
                // Second split: LowMid vs HighRest
                float lowMid, highRest;
                crossovers[1].process(rest, lowMid, highRest, ch);
                bands[ch][1] = lowMid;
                
                // Third split: HighMid vs High
                float highMid, high;
                crossovers[2].process(highRest, highMid, high, ch);
                bands[ch][2] = highMid;
                bands[ch][3] = high;
            }
            
            // Apply phase shifts to each band
            for (int band = 0; band < 4; ++band) {
                if (!bandMute[band]) {
                    for (int ch = 0; ch < 2; ++ch) {
                        bands[ch][band] = phaseFilters[band].process(bands[ch][band], ch);
                    }
                }
            }
            
            // Solo/mute logic and reconstruction
            float wetL = 0.0f, wetR = 0.0f;
            bool anySolo = false;
            
            for (int band = 0; band < 4; ++band) {
                if (bandSolo[band]) anySolo = true;
            }
            
            for (int band = 0; band < 4; ++band) {
                if (!bandMute[band] && (!anySolo || bandSolo[band])) {
                    wetL += bands[0][band];
                    wetR += bands[1][band];
                }
            }
            
            // Apply global polarity if needed
            if (globalPolarity) {
                wetL = -wetL;
                wetR = -wetR;
            }
            
            // Mix dry/wet
            const float mixAmt = smoothers.mix.tick();
            left[i] = dryL * (1.0f - mixAmt) + wetL * mixAmt;
            right[i] = dryR * (1.0f - mixAmt) + wetR * mixAmt;
            
            // Update correlation analyzer
            correlator.process(left[i], right[i]);
        }
        
        // Store correlation
        currentCorrelation.store(correlator.getCorrelation());
    }
    
    void prepareToPlay(double sr, int samplesPerBlock) {
        sampleRate = static_cast<float>(sr);
        
        // Setup smoothers
        smoothers.lowPhase.setCoeff(sr, 20.0f);
        smoothers.lowMidPhase.setCoeff(sr, 20.0f);
        smoothers.highMidPhase.setCoeff(sr, 20.0f);
        smoothers.highPhase.setCoeff(sr, 20.0f);
        smoothers.lowFreq.setCoeff(sr, 50.0f);
        smoothers.midFreq.setCoeff(sr, 50.0f);
        smoothers.highFreq.setCoeff(sr, 50.0f);
        smoothers.mix.setCoeff(sr, 20.0f);
        
        reset();
    }
    
    void reset() {
        for (auto& crossover : crossovers) {
            crossover.reset();
        }
        for (auto& filter : phaseFilters) {
            filter.reset();
        }
    }
};

//==============================================================================
// Public Interface Implementation
//==============================================================================

PhaseAlign_Platinum::PhaseAlign_Platinum() : pImpl(std::make_unique<Impl>()) {}
PhaseAlign_Platinum::~PhaseAlign_Platinum() = default;

void PhaseAlign_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->prepareToPlay(sampleRate, samplesPerBlock);
}

void PhaseAlign_Platinum::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void PhaseAlign_Platinum::reset() {
    pImpl->reset();
}

void PhaseAlign_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (static_cast<ParamID>(index)) {
            case ParamID::AUTO_ALIGN:     pImpl->params.autoAlign.store(value); break;
            case ParamID::REFERENCE:      pImpl->params.reference.store(value); break;
            case ParamID::LOW_PHASE:      pImpl->params.lowPhase.store(value); break;
            case ParamID::LOW_MID_PHASE:  pImpl->params.lowMidPhase.store(value); break;
            case ParamID::HIGH_MID_PHASE: pImpl->params.highMidPhase.store(value); break;
            case ParamID::HIGH_PHASE:     pImpl->params.highPhase.store(value); break;
            case ParamID::LOW_FREQ:       pImpl->params.lowFreq.store(value); break;
            case ParamID::MID_FREQ:       pImpl->params.midFreq.store(value); break;
            case ParamID::HIGH_FREQ:      pImpl->params.highFreq.store(value); break;
            case ParamID::MIX:            pImpl->params.mix.store(value); break;
        }
    }
}

juce::String PhaseAlign_Platinum::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::AUTO_ALIGN:     return "Auto Align";
        case ParamID::REFERENCE:      return "Reference";
        case ParamID::LOW_PHASE:      return "Low Phase";
        case ParamID::LOW_MID_PHASE:  return "Low-Mid Phase";
        case ParamID::HIGH_MID_PHASE: return "High-Mid Phase";
        case ParamID::HIGH_PHASE:     return "High Phase";
        case ParamID::LOW_FREQ:       return "Low Freq";
        case ParamID::MID_FREQ:       return "Mid Freq";
        case ParamID::HIGH_FREQ:      return "High Freq";
        case ParamID::MIX:            return "Mix";
        default:                      return "";
    }
}

float PhaseAlign_Platinum::getPhaseCorrelation() const {
    return pImpl->currentCorrelation.load();
}

std::array<float, 4> PhaseAlign_Platinum::getBandPhases() const {
    return {
        pImpl->currentPhases[0].load(),
        pImpl->currentPhases[1].load(),
        pImpl->currentPhases[2].load(),
        pImpl->currentPhases[3].load()
    };
}

void PhaseAlign_Platinum::setBandSolo(int band, bool solo) {
    if (band >= 0 && band < 4) {
        pImpl->bandSolo[band] = solo;
    }
}

void PhaseAlign_Platinum::setBandMute(int band, bool mute) {
    if (band >= 0 && band < 4) {
        pImpl->bandMute[band] = mute;
    }
}

void PhaseAlign_Platinum::setGlobalPolarity(bool invert) {
    pImpl->globalPolarity = invert;
}

void PhaseAlign_Platinum::triggerAutoAlign() {
    // Auto-alignment would analyze phase relationships and set optimal values
    // This is a placeholder for the full implementation
}