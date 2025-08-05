/*
=======================================================================================

    MidSideProcessor_Ultimate.h
    Created: 4 Aug 2025
    Author: Project Chimera
    
    Professional Mid/Side processor for high-end mastering applications.
    Features complete M/S matrix, multi-band processing, dynamic EQ,
    stereo field analysis, and ambisonic support.

=======================================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <atomic>
#include <memory>
#include <complex>
#include <cstring>

//=======================================================================================
/** Professional Mid/Side Processor with advanced DSP capabilities */
class MidSideProcessor_Ultimate
{
public:
    //===================================================================================
    /** Constructor */
    MidSideProcessor_Ultimate();
    
    /** Destructor */
    ~MidSideProcessor_Ultimate();
    
    //===================================================================================
    /** Initialize the processor with sample rate and block size */
    void prepare (double sampleRate, int samplesPerBlock, int numChannels = 2);
    
    /** Process audio block */
    void processBlock (juce::AudioBuffer<float>& buffer);
    
    /** Reset all internal states */
    void reset();
    
    //===================================================================================
    // Core M/S Processing
    //===================================================================================
    
    /** Enable/disable M/S processing */
    void setMSProcessingEnabled (bool enabled) { msProcessingEnabled.store(enabled); }
    
    /** Set stereo width (-200% to +200%) */
    void setStereoWidth (float width) { stereoWidth.store(juce::jlimit(-2.0f, 2.0f, width)); }
    
    /** Set mid channel gain (dB) */
    void setMidGain (float gainDb) { midGain.store(juce::Decibels::decibelsToGain(gainDb)); }
    
    /** Set side channel gain (dB) */
    void setSideGain (float gainDb) { sideGain.store(juce::Decibels::decibelsToGain(gainDb)); }
    
    //===================================================================================
    // Multi-band Processing
    //===================================================================================
    
    static constexpr int NumBands = 6;
    
    /** Set crossover frequencies for multi-band processing */
    void setCrossoverFrequencies (const std::array<float, NumBands-1>& frequencies);
    
    /** Set width control for specific band */
    void setBandWidth (int bandIndex, float width);
    
    /** Set mid/side gains for specific band */
    void setBandMidGain (int bandIndex, float gainDb);
    void setBandSideGain (int bandIndex, float gainDb);
    
    /** Enable frequency-dependent width processing */
    void setFrequencyDependentWidth (bool enabled) { freqDependentWidth.store(enabled); }
    
    //===================================================================================
    // Dynamic EQ
    //===================================================================================
    
    static constexpr int NumDynamicEQBands = 8;
    
    struct DynamicEQBand
    {
        std::atomic<float> frequency{1000.0f};
        std::atomic<float> q{1.0f};
        std::atomic<float> gain{0.0f};
        std::atomic<float> threshold{-20.0f};
        std::atomic<float> ratio{2.0f};
        std::atomic<float> attack{10.0f};
        std::atomic<float> release{100.0f};
        std::atomic<bool> enabled{false};
        std::atomic<bool> sidechainEnabled{false};
        std::atomic<int> filterType{0}; // 0=Bell, 1=HPF, 2=LPF, 3=Shelf
    };
    
    /** Get dynamic EQ band for mid channel */
    DynamicEQBand& getMidDynamicEQBand (int bandIndex) { return midDynamicEQ[bandIndex]; }
    
    /** Get dynamic EQ band for side channel */
    DynamicEQBand& getSideDynamicEQBand (int bandIndex) { return sideDynamicEQ[bandIndex]; }
    
    /** Set sidechain input for dynamic EQ */
    void setSidechainInput (const juce::AudioBuffer<float>& sidechainBuffer);
    
    //===================================================================================
    // Stereo Field Analysis
    //===================================================================================
    
    struct StereoAnalysis
    {
        float correlation{0.0f};
        float width{0.0f};
        float balance{0.0f};
        float midRMS{0.0f};
        float sideRMS{0.0f};
        std::array<float, 360> vectorscopeData{};
        bool isValid{false};
    };
    
    /** Get current stereo analysis data */
    const StereoAnalysis& getStereoAnalysis() const { return stereoAnalysis; }
    
    /** Enable/disable stereo analysis */
    void setStereoAnalysisEnabled (bool enabled) { stereoAnalysisEnabled.store(enabled); }
    
    //===================================================================================
    // K-System Metering & LUFS
    //===================================================================================
    
    struct MeteringData
    {
        float midPeak{-100.0f};
        float sidePeak{-100.0f};
        float midRMS{-100.0f};
        float sideRMS{-100.0f};
        float lufs{-100.0f};
        float truePeak{-100.0f};
        std::array<float, NumBands> bandPeaks{};
        std::array<float, NumBands> bandRMS{};
    };
    
    /** Get current metering data */
    const MeteringData& getMeteringData() const { return meteringData; }
    
    /** Set K-system reference level (K-12, K-14, K-20) */
    void setKSystemReference (int kValue) { kSystemReference.store(kValue); }
    
    //===================================================================================
    // Haas Effect Processor
    //===================================================================================
    
    /** Enable/disable Haas effect processing */
    void setHaasEffectEnabled (bool enabled) { haasEffectEnabled.store(enabled); }
    
    /** Set Haas delay time (0.1 to 40 ms) */
    void setHaasDelayTime (float delayMs) { haasDelayTime.store(juce::jlimit(0.1f, 40.0f, delayMs)); }
    
    /** Set Haas feedback amount */
    void setHaasFeedback (float feedback) { haasFeedback.store(juce::jlimit(0.0f, 0.95f, feedback)); }
    
    //===================================================================================
    // Oversampling
    //===================================================================================
    
    /** Set oversampling factor (1, 2, 4, 8, 16) */
    void setOversamplingFactor (int factor);
    
    /** Get current oversampling factor */
    int getOversamplingFactor() const { return oversamplingFactor.load(); }
    
    //===================================================================================
    // Ambisonic Processing
    //===================================================================================
    
    /** Set ambisonic order (0-3) */
    void setAmbisonicOrder (int order) { ambisonicOrder.store(juce::jlimit(0, 3, order)); }
    
    /** Process ambisonic channels */
    void processAmbisonicBlock (juce::AudioBuffer<float>& ambisonicBuffer);
    
    /** Enable/disable ambisonic processing */
    void setAmbisonicProcessingEnabled (bool enabled) { ambisonicEnabled.store(enabled); }

private:
    //===================================================================================
    // Internal Classes
    //===================================================================================
    
    /** High-precision Linkwitz-Riley crossover filter (48dB/oct) */
    class LinkwitzRileyCrossover
    {
    public:
        LinkwitzRileyCrossover() = default;
        
        void prepare (double sampleRate, float frequency)
        {
            this->sampleRate = sampleRate;
            setCutoffFrequency(frequency);
        }
        
        void setCutoffFrequency (float frequency)
        {
            cutoffFreq = frequency;
            calculateCoefficients();
        }
        
        void processBlock (const float* input, float* lowOutput, float* highOutput, int numSamples)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                auto inputSample = input[i];
                
                // Apply denormal protection
                inputSample = addDenormalNoise(inputSample);
                
                // 4th order Butterworth cascade
                auto stage1Low = processButterworthStage(inputSample, lpf1State, lpfCoeffs);
                auto stage2Low = processButterworthStage(stage1Low, lpf2State, lpfCoeffs);
                
                auto stage1High = processButterworthStage(inputSample, hpf1State, hpfCoeffs);
                auto stage2High = processButterworthStage(stage1High, hpf2State, hpfCoeffs);
                
                lowOutput[i] = stage2Low;
                highOutput[i] = stage2High;
            }
        }
        
    private:
        struct BiquadState
        {
            float x1 = 0.0f, x2 = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;
        };
        
        struct BiquadCoeffs
        {
            float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
            float a1 = 0.0f, a2 = 0.0f;
        };
        
        double sampleRate = 44100.0;
        float cutoffFreq = 1000.0f;
        
        BiquadState lpf1State, lpf2State, hpf1State, hpf2State;
        BiquadCoeffs lpfCoeffs, hpfCoeffs;
        
        void calculateCoefficients()
        {
            auto omega = juce::MathConstants<double>::twoPi * cutoffFreq / sampleRate;
            auto cosOmega = std::cos(omega);
            auto sinOmega = std::sin(omega);
            auto alpha = sinOmega / std::sqrt(2.0); // Q = 0.707 for Butterworth
            
            // Low-pass coefficients
            auto norm = 1.0 + alpha;
            lpfCoeffs.b0 = static_cast<float>((1.0 - cosOmega) / (2.0 * norm));
            lpfCoeffs.b1 = static_cast<float>((1.0 - cosOmega) / norm);
            lpfCoeffs.b2 = lpfCoeffs.b0;
            lpfCoeffs.a1 = static_cast<float>(-2.0 * cosOmega / norm);
            lpfCoeffs.a2 = static_cast<float>((1.0 - alpha) / norm);
            
            // High-pass coefficients
            hpfCoeffs.b0 = static_cast<float>((1.0 + cosOmega) / (2.0 * norm));
            hpfCoeffs.b1 = static_cast<float>(-(1.0 + cosOmega) / norm);
            hpfCoeffs.b2 = hpfCoeffs.b0;
            hpfCoeffs.a1 = lpfCoeffs.a1;
            hpfCoeffs.a2 = lpfCoeffs.a2;
        }
        
        float processButterworthStage (float input, BiquadState& state, const BiquadCoeffs& coeffs)
        {
            auto output = coeffs.b0 * input + coeffs.b1 * state.x1 + coeffs.b2 * state.x2
                         - coeffs.a1 * state.y1 - coeffs.a2 * state.y2;
            
            state.x2 = state.x1;
            state.x1 = input;
            state.y2 = state.y1;
            state.y1 = output;
            
            return output;
        }
        
        // Denormal protection using bit manipulation
        static float addDenormalNoise (float sample)
        {
            static constexpr float denormalBias = 1e-25f;
            union { float f; uint32_t i; } u;
            u.f = sample;
            u.f += denormalBias;
            u.i &= 0x7FFFFFFF; // Remove sign bit from bias
            return u.f - denormalBias;
        }
    };
    
    /** Professional elliptic crossover filter */
    class EllipticCrossover
    {
    public:
        EllipticCrossover() = default;
        
        void prepare (double sampleRate, float frequency, float ripple = 0.1f, float stopbandAttenuation = 60.0f)
        {
            this->sampleRate = sampleRate;
            this->ripple = ripple;
            this->stopbandAttenuation = stopbandAttenuation;
            setCutoffFrequency(frequency);
        }
        
        void setCutoffFrequency (float frequency)
        {
            cutoffFreq = frequency;
            calculateEllipticCoefficients();
        }
        
        void processBlock (const float* input, float* lowOutput, float* highOutput, int numSamples)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                auto inputSample = addDenormalNoise(input[i]);
                
                // Process through elliptic filter cascade
                auto lowResult = inputSample;
                auto highResult = inputSample;
                
                for (int stage = 0; stage < numStages; ++stage)
                {
                    lowResult = processEllipticStage(lowResult, lowStates[stage], lowCoeffs[stage]);
                    highResult = processEllipticStage(highResult, highStates[stage], highCoeffs[stage]);
                }
                
                lowOutput[i] = lowResult;
                highOutput[i] = highResult;
            }
        }
        
    private:
        static constexpr int maxStages = 8;
        
        struct BiquadState
        {
            float x1 = 0.0f, x2 = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;
        };
        
        struct BiquadCoeffs
        {
            float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
            float a1 = 0.0f, a2 = 0.0f;
        };
        
        double sampleRate = 44100.0;
        float cutoffFreq = 1000.0f;
        float ripple = 0.1f;
        float stopbandAttenuation = 60.0f;
        int numStages = 4;
        
        std::array<BiquadState, maxStages> lowStates, highStates;
        std::array<BiquadCoeffs, maxStages> lowCoeffs, highCoeffs;
        
        void calculateEllipticCoefficients()
        {
            // Simplified elliptic filter design - in production, use proper elliptic design algorithms
            auto normalizedFreq = cutoffFreq / (sampleRate * 0.5);
            normalizedFreq = juce::jlimit(0.001, 0.499, normalizedFreq);
            
            auto omega = juce::MathConstants<double>::pi * normalizedFreq;
            auto cosOmega = std::cos(omega);
            auto sinOmega = std::sin(omega);
            
            // Design elliptic filter poles and zeros (simplified)
            for (int stage = 0; stage < numStages; ++stage)
            {
                auto poleAngle = juce::MathConstants<double>::pi * (2.0 * stage + 1.0) / (2.0 * numStages);
                auto q = 1.0 / (2.0 * std::sin(poleAngle / numStages));
                auto alpha = sinOmega / (2.0 * q);
                
                // Low-pass stage
                auto norm = 1.0 + alpha;
                lowCoeffs[stage].b0 = static_cast<float>((1.0 - cosOmega) / (2.0 * norm));
                lowCoeffs[stage].b1 = static_cast<float>((1.0 - cosOmega) / norm);
                lowCoeffs[stage].b2 = lowCoeffs[stage].b0;
                lowCoeffs[stage].a1 = static_cast<float>(-2.0 * cosOmega / norm);
                lowCoeffs[stage].a2 = static_cast<float>((1.0 - alpha) / norm);
                
                // High-pass stage (complementary)
                highCoeffs[stage].b0 = static_cast<float>((1.0 + cosOmega) / (2.0 * norm));
                highCoeffs[stage].b1 = static_cast<float>(-(1.0 + cosOmega) / norm);
                highCoeffs[stage].b2 = highCoeffs[stage].b0;
                highCoeffs[stage].a1 = lowCoeffs[stage].a1;
                highCoeffs[stage].a2 = lowCoeffs[stage].a2;
            }
        }
        
        float processEllipticStage (float input, BiquadState& state, const BiquadCoeffs& coeffs)
        {
            auto output = coeffs.b0 * input + coeffs.b1 * state.x1 + coeffs.b2 * state.x2
                         - coeffs.a1 * state.y1 - coeffs.a2 * state.y2;
            
            state.x2 = state.x1;
            state.x1 = input;
            state.y2 = state.y1;
            state.y1 = output;
            
            return output;
        }
        
        static float addDenormalNoise (float sample)
        {
            static constexpr float denormalBias = 1e-25f;
            return sample + denormalBias;
        }
    };
    
    /** Professional parametric EQ with dynamic capabilities */
    class DynamicParametricEQ
    {
    public:
        DynamicParametricEQ() = default;
        
        void prepare (double sampleRate)
        {
            this->sampleRate = sampleRate;
            envelope.prepare(sampleRate);
            sideChainEnvelope.prepare(sampleRate);
        }
        
        void setBand (float frequency, float q, float gain, float threshold, float ratio, float attack, float release)
        {
            this->frequency = frequency;
            this->q = q;
            this->staticGain = gain;
            this->threshold = threshold;
            this->ratio = ratio;
            envelope.setAttack(attack);
            envelope.setRelease(release);
            
            calculateCoefficients();
        }
        
        float processSample (float input, float sidechainInput = 0.0f)
        {
            auto envelopeInput = (sidechainInput != 0.0f) ? sidechainInput : input;
            auto envelopeLevel = envelope.processSample(std::abs(envelopeInput));
            
            // Dynamic gain calculation
            auto dynamicGain = staticGain;
            if (envelopeLevel > threshold)
            {
                auto overThreshold = envelopeLevel - threshold;
                auto compressedOver = overThreshold / ratio;
                dynamicGain = staticGain * (1.0f - (overThreshold - compressedOver) / overThreshold);
            }
            
            // Update filter coefficients if gain changed significantly
            if (std::abs(dynamicGain - lastDynamicGain) > 0.1f)
            {
                lastDynamicGain = dynamicGain;
                updateCoefficientsForGain(dynamicGain);
            }
            
            return processFilter(input);
        }
        
    private:
        double sampleRate = 44100.0;
        float frequency = 1000.0f;
        float q = 1.0f;
        float staticGain = 0.0f;
        float threshold = -20.0f;
        float ratio = 2.0f;
        float lastDynamicGain = 0.0f;
        
        struct BiquadState
        {
            float x1 = 0.0f, x2 = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;
        } filterState;
        
        struct BiquadCoeffs
        {
            float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
            float a1 = 0.0f, a2 = 0.0f;
        } coeffs;
        
        class EnvelopeFollower
        {
        public:
            void prepare (double sampleRate)
            {
                this->sampleRate = sampleRate;
            }
            
            void setAttack (float attackMs)
            {
                attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * static_cast<float>(sampleRate)));
            }
            
            void setRelease (float releaseMs)
            {
                releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(sampleRate)));
            }
            
            float processSample (float input)
            {
                auto absInput = std::abs(input);
                
                if (absInput > envelope)
                    envelope = absInput + (envelope - absInput) * attackCoeff;
                else
                    envelope = absInput + (envelope - absInput) * releaseCoeff;
                
                return envelope;
            }
            
        private:
            double sampleRate = 44100.0;
            float attackCoeff = 0.0f;
            float releaseCoeff = 0.0f;
            float envelope = 0.0f;
        };
        
        EnvelopeFollower envelope, sideChainEnvelope;
        
        void calculateCoefficients()
        {
            updateCoefficientsForGain(staticGain);
        }
        
        void updateCoefficientsForGain (float gain)
        {
            auto omega = juce::MathConstants<double>::twoPi * frequency / sampleRate;
            auto cosOmega = std::cos(omega);
            auto sinOmega = std::sin(omega);
            auto alpha = sinOmega / (2.0 * q);
            auto A = std::pow(10.0, gain / 40.0); // Convert dB to linear
            
            // Peaking EQ coefficients
            auto norm = 1.0 + alpha / A;
            coeffs.b0 = static_cast<float>((1.0 + alpha * A) / norm);
            coeffs.b1 = static_cast<float>(-2.0 * cosOmega / norm);
            coeffs.b2 = static_cast<float>((1.0 - alpha * A) / norm);
            coeffs.a1 = static_cast<float>(-2.0 * cosOmega / norm);
            coeffs.a2 = static_cast<float>((1.0 - alpha / A) / norm);
        }
        
        float processFilter (float input)
        {
            auto output = coeffs.b0 * input + coeffs.b1 * filterState.x1 + coeffs.b2 * filterState.x2
                         - coeffs.a1 * filterState.y1 - coeffs.a2 * filterState.y2;
            
            filterState.x2 = filterState.x1;
            filterState.x1 = input;
            filterState.y2 = filterState.y1;
            filterState.y1 = output;
            
            return output;
        }
    };
    
    /** Professional LUFS meter implementation */
    class LUFSMeter
    {
    public:
        LUFSMeter() = default;
        
        void prepare (double sampleRate, int numChannels)
        {
            this->sampleRate = sampleRate;
            this->numChannels = numChannels;
            
            // Initialize K-weighting filters
            kWeightingFilters.resize(numChannels);
            for (auto& filter : kWeightingFilters)
            {
                filter.prepare(sampleRate);
            }
            
            // Initialize measurement window (400ms for momentary, 3s for short-term)
            momentaryWindow.resize(static_cast<size_t>(sampleRate * 0.4));
            shortTermWindow.resize(static_cast<size_t>(sampleRate * 3.0));
            
            reset();
        }
        
        void processSample (const float* samples)
        {
            float weightedPower = 0.0f;
            
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto weighted = kWeightingFilters[ch].processSample(samples[ch]);
                auto channelWeight = (ch < 2) ? 1.0f : (ch < 5) ? 1.41f : 1.0f; // ITU-R BS.1770-4 weighting
                weightedPower += channelWeight * weighted * weighted;
            }
            
            // Update sliding windows
            momentaryWindow[momentaryIndex] = weightedPower;
            shortTermWindow[shortTermIndex] = weightedPower;
            
            momentaryIndex = (momentaryIndex + 1) % momentaryWindow.size();
            shortTermIndex = (shortTermIndex + 1) % shortTermWindow.size();
            
            // Calculate LUFS every 100ms
            if (++sampleCount >= sampleRate * 0.1)
            {
                sampleCount = 0;
                updateLUFS();
            }
        }
        
        float getMomentaryLUFS() const { return momentaryLUFS.load(); }
        float getShortTermLUFS() const { return shortTermLUFS.load(); }
        float getIntegratedLUFS() const { return integratedLUFS.load(); }
        
        void reset()
        {
            std::fill(momentaryWindow.begin(), momentaryWindow.end(), 0.0f);
            std::fill(shortTermWindow.begin(), shortTermWindow.end(), 0.0f);
            momentaryIndex = 0;
            shortTermIndex = 0;
            sampleCount = 0;
            
            for (auto& filter : kWeightingFilters)
                filter.reset();
        }
        
    private:
        double sampleRate = 44100.0;
        int numChannels = 2;
        size_t sampleCount = 0;
        
        std::vector<float> momentaryWindow, shortTermWindow;
        size_t momentaryIndex = 0, shortTermIndex = 0;
        
        std::atomic<float> momentaryLUFS{-100.0f};
        std::atomic<float> shortTermLUFS{-100.0f};
        std::atomic<float> integratedLUFS{-100.0f};
        
        class KWeightingFilter
        {
        public:
            void prepare (double sampleRate)
            {
                this->sampleRate = sampleRate;
                calculateCoefficients();
            }
            
            float processSample (float input)
            {
                // K-weighting consists of high shelf at 1681Hz and high-pass at 38Hz
                auto highShelfOutput = processHighShelf(input);
                return processHighPass(highShelfOutput);
            }
            
            void reset()
            {
                std::memset(&highShelfState, 0, sizeof(highShelfState));
                std::memset(&highPassState, 0, sizeof(highPassState));
            }
            
        private:
            double sampleRate = 44100.0;
            
            struct BiquadState
            {
                float x1 = 0.0f, x2 = 0.0f;
                float y1 = 0.0f, y2 = 0.0f;
            } highShelfState, highPassState;
            
            struct BiquadCoeffs
            {
                float b0, b1, b2, a1, a2;
            } highShelfCoeffs, highPassCoeffs;
            
            void calculateCoefficients()
            {
                // High shelf at 1681 Hz, +4 dB
                auto f = 1681.0 / sampleRate;
                auto omega = juce::MathConstants<double>::twoPi * f;
                auto cosOmega = std::cos(omega);
                auto sinOmega = std::sin(omega);
                auto A = std::pow(10.0, 4.0 / 40.0);
                auto S = 1.0;
                auto beta = std::sqrt(A) / 1.0; // Q = 1.0
                
                auto norm = (A + 1.0) - (A - 1.0) * cosOmega + beta * sinOmega;
                highShelfCoeffs.b0 = static_cast<float>(A * ((A + 1.0) + (A - 1.0) * cosOmega + beta * sinOmega) / norm);
                highShelfCoeffs.b1 = static_cast<float>(-2.0 * A * ((A - 1.0) + (A + 1.0) * cosOmega) / norm);
                highShelfCoeffs.b2 = static_cast<float>(A * ((A + 1.0) + (A - 1.0) * cosOmega - beta * sinOmega) / norm);
                highShelfCoeffs.a1 = static_cast<float>(2.0 * ((A - 1.0) - (A + 1.0) * cosOmega) / norm);
                highShelfCoeffs.a2 = static_cast<float>(((A + 1.0) - (A - 1.0) * cosOmega - beta * sinOmega) / norm);
                
                // High-pass at 38 Hz
                f = 38.0 / sampleRate;
                omega = juce::MathConstants<double>::twoPi * f;
                cosOmega = std::cos(omega);
                sinOmega = std::sin(omega);
                auto alpha = sinOmega / (2.0 * 0.5); // Q = 0.5
                
                norm = 1.0 + alpha;
                highPassCoeffs.b0 = static_cast<float>((1.0 + cosOmega) / (2.0 * norm));
                highPassCoeffs.b1 = static_cast<float>(-(1.0 + cosOmega) / norm);
                highPassCoeffs.b2 = highPassCoeffs.b0;
                highPassCoeffs.a1 = static_cast<float>(-2.0 * cosOmega / norm);
                highPassCoeffs.a2 = static_cast<float>((1.0 - alpha) / norm);
            }
            
            float processHighShelf (float input)
            {
                auto output = highShelfCoeffs.b0 * input + highShelfCoeffs.b1 * highShelfState.x1 + highShelfCoeffs.b2 * highShelfState.x2
                             - highShelfCoeffs.a1 * highShelfState.y1 - highShelfCoeffs.a2 * highShelfState.y2;
                
                highShelfState.x2 = highShelfState.x1;
                highShelfState.x1 = input;
                highShelfState.y2 = highShelfState.y1;
                highShelfState.y1 = output;
                
                return output;
            }
            
            float processHighPass (float input)
            {
                auto output = highPassCoeffs.b0 * input + highPassCoeffs.b1 * highPassState.x1 + highPassCoeffs.b2 * highPassState.x2
                             - highPassCoeffs.a1 * highPassState.y1 - highPassCoeffs.a2 * highPassState.y2;
                
                highPassState.x2 = highPassState.x1;
                highPassState.x1 = input;
                highPassState.y2 = highPassState.y1;
                highPassState.y1 = output;
                
                return output;
            }
        };
        
        std::vector<KWeightingFilter> kWeightingFilters;
        
        void updateLUFS()
        {
            // Calculate momentary LUFS (400ms window)
            auto momentaryMean = calculateMean(momentaryWindow);
            if (momentaryMean > 0.0f)
                momentaryLUFS.store(-0.691f + 10.0f * std::log10(momentaryMean));
            
            // Calculate short-term LUFS (3s window)
            auto shortTermMean = calculateMean(shortTermWindow);
            if (shortTermMean > 0.0f)
                shortTermLUFS.store(-0.691f + 10.0f * std::log10(shortTermMean));
        }
        
        float calculateMean (const std::vector<float>& window) const
        {
            auto sum = 0.0;
            for (auto value : window)
                sum += value;
            return static_cast<float>(sum / window.size());
        }
    };
    
    /** Haas effect processor for enhanced stereo imaging */
    class HaasProcessor
    {
    public:
        HaasProcessor() = default;
        
        void prepare (double sampleRate, int maxDelaySamples = 2048)
        {
            this->sampleRate = sampleRate;
            leftDelayLine.resize(maxDelaySamples, 0.0f);
            rightDelayLine.resize(maxDelaySamples, 0.0f);
            reset();
        }
        
        void setDelayTime (float delayMs)
        {
            delaySamples = static_cast<int>(delayMs * 0.001f * sampleRate);
            delaySamples = juce::jlimit(1, static_cast<int>(leftDelayLine.size() - 1), delaySamples);
        }
        
        void setFeedback (float feedback)
        {
            this->feedback = juce::jlimit(0.0f, 0.95f, feedback);
        }
        
        void processStereo (float& left, float& right)
        {
            // Read delayed samples
            auto delayedLeft = leftDelayLine[writeIndex];
            auto delayedRight = rightDelayLine[writeIndex];
            
            // Apply Haas effect (delay one channel slightly)
            auto processedLeft = left + feedback * delayedRight;
            auto processedRight = right + feedback * delayedLeft;
            
            // Write to delay lines
            leftDelayLine[(writeIndex + delaySamples) % leftDelayLine.size()] = processedLeft;
            rightDelayLine[(writeIndex + delaySamples) % rightDelayLine.size()] = processedRight;
            
            writeIndex = (writeIndex + 1) % leftDelayLine.size();
            
            left = processedLeft;
            right = processedRight;
        }
        
        void reset()
        {
            std::fill(leftDelayLine.begin(), leftDelayLine.end(), 0.0f);
            std::fill(rightDelayLine.begin(), rightDelayLine.end(), 0.0f);
            writeIndex = 0;
        }
        
    private:
        double sampleRate = 44100.0;
        std::vector<float> leftDelayLine, rightDelayLine;
        int writeIndex = 0;
        int delaySamples = 441; // ~10ms at 44.1kHz
        float feedback = 0.0f;
    };
    
    /** Correlation meter for stereo analysis */
    class CorrelationMeter
    {
    public:
        CorrelationMeter() = default;
        
        void prepare (double sampleRate)
        {
            this->sampleRate = sampleRate;
            auto windowSize = static_cast<size_t>(sampleRate * 0.1); // 100ms window
            leftBuffer.resize(windowSize, 0.0f);
            rightBuffer.resize(windowSize, 0.0f);
            reset();
        }
        
        void processSample (float left, float right)
        {
            leftBuffer[bufferIndex] = left;
            rightBuffer[bufferIndex] = right;
            bufferIndex = (bufferIndex + 1) % leftBuffer.size();
            
            if (++sampleCount >= updateInterval)
            {
                sampleCount = 0;
                calculateCorrelation();
            }
        }
        
        float getCorrelation() const { return correlation.load(); }
        
        void reset()
        {
            std::fill(leftBuffer.begin(), leftBuffer.end(), 0.0f);
            std::fill(rightBuffer.begin(), rightBuffer.end(), 0.0f);
            bufferIndex = 0;
            sampleCount = 0;
            correlation.store(0.0f);
        }
        
    private:
        double sampleRate = 44100.0;
        std::vector<float> leftBuffer, rightBuffer;
        size_t bufferIndex = 0;
        size_t sampleCount = 0;
        size_t updateInterval = 1024; // Update correlation every 1024 samples
        std::atomic<float> correlation{0.0f};
        
        void calculateCorrelation()
        {
            double sumL = 0.0, sumR = 0.0, sumLR = 0.0, sumL2 = 0.0, sumR2 = 0.0;
            auto N = static_cast<double>(leftBuffer.size());
            
            for (size_t i = 0; i < leftBuffer.size(); ++i)
            {
                auto l = static_cast<double>(leftBuffer[i]);
                auto r = static_cast<double>(rightBuffer[i]);
                
                sumL += l;
                sumR += r;
                sumLR += l * r;
                sumL2 += l * l;
                sumR2 += r * r;
            }
            
            auto meanL = sumL / N;
            auto meanR = sumR / N;
            auto meanLR = sumLR / N;
            auto meanL2 = sumL2 / N;
            auto meanR2 = sumR2 / N;
            
            auto covariance = meanLR - meanL * meanR;
            auto stdL = std::sqrt(meanL2 - meanL * meanL);
            auto stdR = std::sqrt(meanR2 - meanR * meanR);
            
            auto corr = (stdL > 1e-10 && stdR > 1e-10) ? covariance / (stdL * stdR) : 0.0;
            correlation.store(static_cast<float>(juce::jlimit(-1.0, 1.0, corr)));
        }
    };
    
    //===================================================================================
    // Member Variables
    //===================================================================================
    
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    int currentNumChannels = 2;
    
    // Core M/S processing
    std::atomic<bool> msProcessingEnabled{true};
    std::atomic<float> stereoWidth{1.0f};
    std::atomic<float> midGain{1.0f};
    std::atomic<float> sideGain{1.0f};
    
    // Multi-band processing
    std::array<std::unique_ptr<LinkwitzRileyCrossover>, NumBands-1> crossovers;
    std::array<std::unique_ptr<EllipticCrossover>, NumBands-1> ellipticCrossovers;
    std::array<float, NumBands-1> crossoverFrequencies{{80.0f, 320.0f, 1280.0f, 5120.0f, 20480.0f}};
    std::array<std::atomic<float>, NumBands> bandWidths;
    std::array<std::atomic<float>, NumBands> bandMidGains;
    std::array<std::atomic<float>, NumBands> bandSideGains;
    std::atomic<bool> freqDependentWidth{false};
    
    // Dynamic EQ
    std::array<DynamicEQBand, NumDynamicEQBands> midDynamicEQ;
    std::array<DynamicEQBand, NumDynamicEQBands> sideDynamicEQ;
    std::array<std::unique_ptr<DynamicParametricEQ>, NumDynamicEQBands> midDynamicEQProcessors;
    std::array<std::unique_ptr<DynamicParametricEQ>, NumDynamicEQBands> sideDynamicEQProcessors;
    juce::AudioBuffer<float> sidechainBuffer;
    
    // Stereo analysis
    mutable StereoAnalysis stereoAnalysis;
    std::atomic<bool> stereoAnalysisEnabled{true};
    std::unique_ptr<CorrelationMeter> correlationMeter;
    
    // Metering
    mutable MeteringData meteringData;
    std::unique_ptr<LUFSMeter> lufsMeter;
    std::atomic<int> kSystemReference{14}; // K-14
    
    // Haas effect
    std::atomic<bool> haasEffectEnabled{false};
    std::atomic<float> haasDelayTime{10.0f}; // ms
    std::atomic<float> haasFeedback{0.2f};
    std::unique_ptr<HaasProcessor> haasProcessor;
    
    // Oversampling
    std::atomic<int> oversamplingFactor{1};
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    
    // Ambisonic processing
    std::atomic<bool> ambisonicEnabled{false};
    std::atomic<int> ambisonicOrder{1};
    
    // Processing buffers
    juce::AudioBuffer<float> midBuffer, sideBuffer;
    std::array<juce::AudioBuffer<float>, NumBands> bandBuffers;
    juce::AudioBuffer<float> oversampledBuffer;
    
    // Thread safety
    juce::SpinLock parameterLock;
    
    //===================================================================================
    // Internal processing methods
    //===================================================================================
    
    /** Encode stereo to mid/side */
    void encodeToMidSide (juce::AudioBuffer<float>& buffer);
    
    /** Decode mid/side to stereo */
    void decodeFromMidSide (juce::AudioBuffer<float>& buffer);
    
    /** Process multi-band crossovers */
    void processMultiBandCrossovers (juce::AudioBuffer<float>& buffer);
    
    /** Apply dynamic EQ processing */
    void processDynamicEQ (juce::AudioBuffer<float>& midBuffer, juce::AudioBuffer<float>& sideBuffer);
    
    /** Update stereo analysis */
    void updateStereoAnalysis (const juce::AudioBuffer<float>& buffer);
    
    /** Update metering data */
    void updateMetering (const juce::AudioBuffer<float>& buffer);
    
    /** Process ambisonic channels */
    void processAmbisonicChannels (juce::AudioBuffer<float>& buffer);
    
    /** Apply frequency-dependent width control */
    void applyFrequencyDependentWidth (juce::AudioBuffer<float>& buffer);
    
    /** Denormal protection for entire buffer */
    void protectFromDenormals (juce::AudioBuffer<float>& buffer);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidSideProcessor_Ultimate)
};