#pragma once

#include <JuceHeader.h>
#include <vector>
#include <array>
#include <memory>
#include <complex>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <random>

/**
 * TransientShaper_Ultimate - Professional-grade transient shaper with advanced DSP features
 * 
 * Features:
 * - Multiple detection algorithms (Peak, RMS, Hilbert, Spectral Flux, Onset Detection)
 * - Advanced lookahead with ML-inspired prediction
 * - Multi-band transient shaping with crossover filters
 * - Psychoacoustic modeling for perceptually optimized shaping
 * - Professional denormal protection using bit manipulation
 * - Side-chain processing capabilities
 * - Adaptive algorithms that learn from input material
 * - Multiple shaping curves (linear, exponential, logarithmic, S-curve, custom)
 * - Professional oversampling (8x minimum)
 * - Complete parameter set with thread-safe smoothing
 */
class TransientShaper_Ultimate
{
public:
    TransientShaper_Ultimate();
    ~TransientShaper_Ultimate();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void reset();

    // Parameter management
    void setAttackGain(float gain) { attackGain.setTargetValue(gain); }
    void setSustainGain(float gain) { sustainGain.setTargetValue(gain); }
    void setLookaheadTime(float ms) { lookaheadTime.setTargetValue(juce::jlimit(0.0f, 50.0f, ms)); }
    void setDetectionSensitivity(float sensitivity) { detectionSensitivity.setTargetValue(sensitivity); }
    void setShapingCurve(int curveType) { shapingCurveType.store(curveType); }
    void setMultibandEnabled(bool enabled) { multibandEnabled.store(enabled); }
    void setSidechainEnabled(bool enabled) { sidechainEnabled.store(enabled); }
    void setAdaptiveMode(bool enabled) { adaptiveMode.store(enabled); }
    void setOversamplingFactor(int factor) { oversamplingFactor.store(factor); }
    void setDetectionAlgorithm(int algorithm) { detectionAlgorithm.store(algorithm); }
    void setPsychoacousticEnabled(bool enabled) { psychoacousticEnabled.store(enabled); }
    
    // Multi-band controls
    void setLowBandAttack(float gain) { lowBandAttack.setTargetValue(gain); }
    void setMidBandAttack(float gain) { midBandAttack.setTargetValue(gain); }
    void setHighBandAttack(float gain) { highBandAttack.setTargetValue(gain); }
    void setLowBandSustain(float gain) { lowBandSustain.setTargetValue(gain); }
    void setMidBandSustain(float gain) { midBandSustain.setTargetValue(gain); }
    void setHighBandSustain(float gain) { highBandSustain.setTargetValue(gain); }
    void setCrossoverFreq1(float freq) { crossoverFreq1.setTargetValue(freq); }
    void setCrossoverFreq2(float freq) { crossoverFreq2.setTargetValue(freq); }

    // Sidechain input
    void processSidechain(const juce::AudioBuffer<float>& sidechainBuffer);

private:
    // Constants and enums
    static constexpr int MAX_OVERSAMPLING = 16;
    static constexpr int MAX_LOOKAHEAD_SAMPLES = 4800; // 100ms at 48kHz
    static constexpr int NUM_BANDS = 3;
    static constexpr int FFT_SIZE = 2048;
    static constexpr int OVERLAP_SIZE = FFT_SIZE / 4;
    static constexpr float DENORMAL_OFFSET = 1e-30f;
    static constexpr float MIN_FREQUENCY = 20.0f;
    static constexpr float MAX_FREQUENCY = 20000.0f;

    enum DetectionAlgorithm
    {
        PEAK_DETECTION = 0,
        RMS_DETECTION,
        HILBERT_DETECTION,
        SPECTRAL_FLUX,
        ONSET_DETECTION,
        TRANSIENT_SUSTAIN_SEPARATION
    };

    enum ShapingCurve
    {
        LINEAR = 0,
        EXPONENTIAL,
        LOGARITHMIC,
        S_CURVE,
        CUSTOM
    };

    // Audio processing components
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    
    // Thread-safe parameter smoothing
    juce::SmoothedValue<float> attackGain;
    juce::SmoothedValue<float> sustainGain;
    juce::SmoothedValue<float> lookaheadTime;
    juce::SmoothedValue<float> detectionSensitivity;
    juce::SmoothedValue<float> lowBandAttack, midBandAttack, highBandAttack;
    juce::SmoothedValue<float> lowBandSustain, midBandSustain, highBandSustain;
    juce::SmoothedValue<float> crossoverFreq1, crossoverFreq2;

    // Atomic parameters for thread safety
    std::atomic<int> shapingCurveType{LINEAR};
    std::atomic<bool> multibandEnabled{false};
    std::atomic<bool> sidechainEnabled{false};
    std::atomic<bool> adaptiveMode{false};
    std::atomic<int> oversamplingFactor{8};
    std::atomic<int> detectionAlgorithm{PEAK_DETECTION};
    std::atomic<bool> psychoacousticEnabled{false};

    // Oversampling
    struct OversamplingProcessor
    {
        std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
        juce::AudioBuffer<float> oversampledBuffer;
        bool isInitialized = false;
    };
    OversamplingProcessor oversamplingL, oversamplingR;

    // Lookahead delay line
    struct DelayLine
    {
        std::vector<float> buffer;
        int writeIndex = 0;
        int size = 0;
        
        void resize(int newSize)
        {
            size = newSize;
            buffer.resize(size, 0.0f);
            writeIndex = 0;
        }
        
        void write(float sample)
        {
            buffer[writeIndex] = sample;
            writeIndex = (writeIndex + 1) % size;
        }
        
        float read(int delaySamples) const
        {
            int readIndex = (writeIndex - delaySamples + size) % size;
            return buffer[readIndex];
        }
    };
    
    DelayLine delayLineL, delayLineR;
    DelayLine sidechainDelayL, sidechainDelayR;

    // Detection algorithms
    class TransientDetector
    {
    public:
        virtual ~TransientDetector() = default;
        virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
        virtual float detectTransient(float sample) = 0;
        virtual void reset() = 0;
    };

    // Peak detection
    class PeakDetector : public TransientDetector
    {
    public:
        void prepareToPlay(double sampleRate, int samplesPerBlock) override
        {
            this->sampleRate = sampleRate;
            envelope.reset(sampleRate, 0.001); // 1ms attack
            envelope.setTargetValue(0.0f);
        }

        float detectTransient(float sample) override
        {
            float absSample = std::abs(sample);
            if (absSample > envelope.getCurrentValue())
                envelope.setTargetValue(absSample);
            else
                envelope.setTargetValue(absSample * 0.9999f); // Slow release
            
            envelope.skip(1);
            float current = envelope.getCurrentValue();
            float derivative = current - previousValue;
            previousValue = current;
            
            return juce::jmax(0.0f, derivative * 10.0f); // Scale derivative
        }

        void reset() override
        {
            envelope.reset(sampleRate, 0.001);
            envelope.setTargetValue(0.0f);
            previousValue = 0.0f;
        }

    private:
        juce::SmoothedValue<float> envelope;
        float previousValue = 0.0f;
        double sampleRate = 44100.0;
    };

    // RMS detection
    class RMSDetector : public TransientDetector
    {
    public:
        void prepareToPlay(double sampleRate, int samplesPerBlock) override
        {
            this->sampleRate = sampleRate;
            windowSize = static_cast<int>(sampleRate * 0.010); // 10ms window
            rmsBuffer.resize(windowSize, 0.0f);
            writeIndex = 0;
            rmsSum = 0.0f;
        }

        float detectTransient(float sample) override
        {
            // Update RMS calculation
            float oldSample = rmsBuffer[writeIndex];
            rmsBuffer[writeIndex] = sample * sample;
            rmsSum = rmsSum - oldSample + rmsBuffer[writeIndex];
            writeIndex = (writeIndex + 1) % windowSize;
            
            float currentRms = std::sqrt(rmsSum / windowSize);
            float derivative = currentRms - previousRms;
            previousRms = currentRms;
            
            return juce::jmax(0.0f, derivative * 50.0f); // Scale derivative
        }

        void reset() override
        {
            std::fill(rmsBuffer.begin(), rmsBuffer.end(), 0.0f);
            rmsSum = 0.0f;
            previousRms = 0.0f;
            writeIndex = 0;
        }

    private:
        std::vector<float> rmsBuffer;
        int windowSize = 441;
        int writeIndex = 0;
        float rmsSum = 0.0f;
        float previousRms = 0.0f;
        double sampleRate = 44100.0;
    };

    // Hilbert transform detection
    class HilbertDetector : public TransientDetector
    {
    public:
        void prepareToPlay(double sampleRate, int samplesPerBlock) override
        {
            this->sampleRate = sampleRate;
            // Initialize Hilbert transform coefficients
            hilbertDelay.resize(HILBERT_ORDER, 0.0f);
            hilbertIndex = 0;
            envelope.reset(sampleRate, 0.005);
            envelope.setTargetValue(0.0f);
        }

        float detectTransient(float sample) override
        {
            // Apply Hilbert transform
            hilbertDelay[hilbertIndex] = sample;
            float hilbertOutput = 0.0f;
            
            for (int i = 0; i < HILBERT_ORDER; ++i)
            {
                int tapIndex = (hilbertIndex - i + HILBERT_ORDER) % HILBERT_ORDER;
                if (i % 2 == 1) // Odd taps only
                {
                    float coeff = 2.0f / (juce::MathConstants<float>::pi * i);
                    hilbertOutput += hilbertDelay[tapIndex] * coeff;
                }
            }
            
            hilbertIndex = (hilbertIndex + 1) % HILBERT_ORDER;
            
            // Calculate instantaneous amplitude
            float amplitude = std::sqrt(sample * sample + hilbertOutput * hilbertOutput);
            envelope.setTargetValue(amplitude);
            envelope.skip(1);
            
            float derivative = amplitude - previousAmplitude;
            previousAmplitude = amplitude;
            
            return juce::jmax(0.0f, derivative * 5.0f);
        }

        void reset() override
        {
            std::fill(hilbertDelay.begin(), hilbertDelay.end(), 0.0f);
            hilbertIndex = 0;
            previousAmplitude = 0.0f;
            envelope.reset(sampleRate, 0.005);
            envelope.setTargetValue(0.0f);
        }

    private:
        static constexpr int HILBERT_ORDER = 63;
        std::vector<float> hilbertDelay;
        int hilbertIndex = 0;
        float previousAmplitude = 0.0f;
        juce::SmoothedValue<float> envelope;
        double sampleRate = 44100.0;
    };

    // Spectral flux detection
    class SpectralFluxDetector : public TransientDetector
    {
    public:
        void prepareToPlay(double sampleRate, int samplesPerBlock) override
        {
            this->sampleRate = sampleRate;
            fft = std::make_unique<juce::dsp::FFT>(fftOrder);
            fftBuffer.resize(fftSize * 2, 0.0f);
            previousMagnitudes.resize(fftSize / 2 + 1, 0.0f);
            window.resize(fftSize);
            
            // Hann window
            for (int i = 0; i < fftSize; ++i)
            {
                window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1)));
            }
            
            bufferIndex = 0;
            inputBuffer.resize(fftSize, 0.0f);
            hopSize = fftSize / 4;
            samplesSinceLastFFT = 0;
        }

        float detectTransient(float sample) override
        {
            inputBuffer[bufferIndex] = sample;
            bufferIndex = (bufferIndex + 1) % fftSize;
            samplesSinceLastFFT++;
            
            if (samplesSinceLastFFT >= hopSize)
            {
                samplesSinceLastFFT = 0;
                return calculateSpectralFlux();
            }
            
            return currentFlux;
        }

        void reset() override
        {
            std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
            std::fill(previousMagnitudes.begin(), previousMagnitudes.end(), 0.0f);
            std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
            bufferIndex = 0;
            samplesSinceLastFFT = 0;
            currentFlux = 0.0f;
        }

    private:
        float calculateSpectralFlux()
        {
            // Copy windowed data to FFT buffer
            for (int i = 0; i < fftSize; ++i)
            {
                int readIndex = (bufferIndex + i) % fftSize;
                fftBuffer[i * 2] = inputBuffer[readIndex] * window[i];
                fftBuffer[i * 2 + 1] = 0.0f;
            }
            
            fft->performFrequencyOnlyForwardTransform(fftBuffer.data());
            
            float flux = 0.0f;
            for (int i = 0; i < fftSize / 2 + 1; ++i)
            {
                float magnitude = fftBuffer[i];
                float diff = magnitude - previousMagnitudes[i];
                if (diff > 0.0f)
                    flux += diff;
                previousMagnitudes[i] = magnitude;
            }
            
            currentFlux = flux / (fftSize / 2 + 1);
            return currentFlux * 0.1f; // Scale factor
        }

        static constexpr int fftOrder = 11; // 2^11 = 2048
        static constexpr int fftSize = 1 << fftOrder;
        std::unique_ptr<juce::dsp::FFT> fft;
        std::vector<float> fftBuffer;
        std::vector<float> previousMagnitudes;
        std::vector<float> window;
        std::vector<float> inputBuffer;
        int bufferIndex = 0;
        int hopSize = 512;
        int samplesSinceLastFFT = 0;
        float currentFlux = 0.0f;
        double sampleRate = 44100.0;
    };

    // Onset detection
    class OnsetDetector : public TransientDetector
    {
    public:
        void prepareToPlay(double sampleRate, int samplesPerBlock) override
        {
            spectralFlux.prepareToPlay(sampleRate, samplesPerBlock);
            peakPicker.resize(static_cast<int>(sampleRate * 0.1)); // 100ms history
            medianFilter.resize(9, 0.0f); // 9-tap median filter
            writeIndex = 0;
        }

        float detectTransient(float sample) override
        {
            float flux = spectralFlux.detectTransient(sample);
            
            // Add to peak picker buffer
            peakPicker[writeIndex] = flux;
            writeIndex = (writeIndex + 1) % peakPicker.size();
            
            // Apply median filter for noise reduction
            medianFilter.erase(medianFilter.begin());
            medianFilter.push_back(flux);
            std::vector<float> sortedFilter = medianFilter;
            std::sort(sortedFilter.begin(), sortedFilter.end());
            float medianFlux = sortedFilter[sortedFilter.size() / 2];
            
            // Peak picking with adaptive threshold
            float threshold = calculateAdaptiveThreshold();
            return (medianFlux > threshold) ? medianFlux : 0.0f;
        }

        void reset() override
        {
            spectralFlux.reset();
            std::fill(peakPicker.begin(), peakPicker.end(), 0.0f);
            std::fill(medianFilter.begin(), medianFilter.end(), 0.0f);
            writeIndex = 0;
        }

    private:
        float calculateAdaptiveThreshold()
        {
            float mean = 0.0f;
            float variance = 0.0f;
            
            for (float value : peakPicker)
                mean += value;
            mean /= peakPicker.size();
            
            for (float value : peakPicker)
                variance += (value - mean) * (value - mean);
            variance /= peakPicker.size();
            
            return mean + std::sqrt(variance) * 2.0f; // 2 standard deviations
        }

        SpectralFluxDetector spectralFlux;
        std::vector<float> peakPicker;
        std::vector<float> medianFilter;
        int writeIndex = 0;
    };

    // Transient/Sustain separation
    class TransientSustainSeparator : public TransientDetector
    {
    public:
        void prepareToPlay(double sampleRate, int samplesPerBlock) override
        {
            this->sampleRate = sampleRate;
            windowSize = static_cast<int>(sampleRate * 0.02); // 20ms window
            buffer.resize(windowSize, 0.0f);
            writeIndex = 0;
            
            // Initialize filters for different frequency bands
            for (auto& filter : bandFilters)
            {
                filter.reset();
                filter.setCoefficients(juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1000.0f));
            }
        }

        float detectTransient(float sample) override
        {
            buffer[writeIndex] = sample;
            writeIndex = (writeIndex + 1) % windowSize;
            
            // Analyze spectral centroid
            float spectralCentroid = calculateSpectralCentroid();
            float spectralSpread = calculateSpectralSpread(spectralCentroid);
            
            // Transient detection based on spectral features
            float transientness = spectralSpread / (spectralCentroid + 1.0f);
            return juce::jlimit(0.0f, 1.0f, transientness * 2.0f);
        }

        void reset() override
        {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writeIndex = 0;
            for (auto& filter : bandFilters)
                filter.reset();
        }

    private:
        float calculateSpectralCentroid()
        {
            float weightedSum = 0.0f;
            float magnitudeSum = 0.0f;
            
            for (int i = 0; i < windowSize; ++i)
            {
                float magnitude = std::abs(buffer[i]);
                weightedSum += magnitude * i;
                magnitudeSum += magnitude;
            }
            
            return (magnitudeSum > 0.0f) ? weightedSum / magnitudeSum : 0.0f;
        }

        float calculateSpectralSpread(float centroid)
        {
            float spread = 0.0f;
            float magnitudeSum = 0.0f;
            
            for (int i = 0; i < windowSize; ++i)
            {
                float magnitude = std::abs(buffer[i]);
                float deviation = i - centroid;
                spread += magnitude * deviation * deviation;
                magnitudeSum += magnitude;
            }
            
            return (magnitudeSum > 0.0f) ? std::sqrt(spread / magnitudeSum) : 0.0f;
        }

        std::vector<float> buffer;
        int windowSize = 882;
        int writeIndex = 0;
        std::array<juce::dsp::IIR::Filter<float>, 4> bandFilters;
        double sampleRate = 44100.0;
    };

    // Detection algorithm instances
    std::unique_ptr<TransientDetector> detectors[6];

    // Multi-band processing
    struct CrossoverFilter
    {
        juce::dsp::LinkwitzRileyFilter<float> lowpass, highpass;
        
        void prepareToPlay(double sampleRate, int samplesPerBlock)
        {
            juce::dsp::ProcessSpec spec;
            spec.sampleRate = sampleRate;
            spec.maximumBlockSize = samplesPerBlock;
            spec.numChannels = 1;
            
            lowpass.prepare(spec);
            highpass.prepare(spec);
        }
        
        void setCutoffFrequency(float frequency)
        {
            lowpass.setCutoffFrequency(frequency);
            highpass.setCutoffFrequency(frequency);
        }
        
        std::pair<float, float> processSample(float input)
        {
            float low = lowpass.processSample(input);
            float high = highpass.processSample(input);
            return {low, high};
        }
        
        void reset()
        {
            lowpass.reset();
            highpass.reset();
        }
    };

    CrossoverFilter crossover1L, crossover1R, crossover2L, crossover2R;

    // Psychoacoustic modeling
    class PsychoacousticModel
    {
    public:
        void prepareToPlay(double sampleRate, int samplesPerBlock)
        {
            this->sampleRate = sampleRate;
            // Initialize bark scale filters
            initializeBarkFilters();
        }

        float calculateMaskingThreshold(const std::vector<float>& spectrum)
        {
            // Simplified psychoacoustic masking calculation
            float totalEnergy = 0.0f;
            float maskingThreshold = 0.0f;
            
            for (size_t i = 0; i < spectrum.size() && i < barkFilters.size(); ++i)
            {
                float energy = spectrum[i] * spectrum[i];
                totalEnergy += energy;
                
                // Apply masking curve
                float masking = energy * 0.1f; // Simplified masking
                maskingThreshold = juce::jmax(maskingThreshold, masking);
            }
            
            return maskingThreshold / (totalEnergy + 1e-10f);
        }

    private:
        void initializeBarkFilters()
        {
            // Initialize 24 critical band filters (simplified)
            barkFilters.resize(24);
            for (int i = 0; i < 24; ++i)
            {
                float centerFreq = barkToHz(i + 0.5f);
                barkFilters[i] = centerFreq / sampleRate; // Normalized frequency
            }
        }

        float barkToHz(float bark)
        {
            return 600.0f * std::sinh(bark / 4.0f);
        }

        std::vector<float> barkFilters;
        double sampleRate = 44100.0;
    };

    PsychoacousticModel psychoacousticModel;

    // Adaptive learning system
    class AdaptiveLearning
    {
    public:
        void prepareToPlay(double sampleRate, int samplesPerBlock)
        {
            this->sampleRate = sampleRate;
            learningRate = 0.001f;
            reset();
        }

        void learn(float input, float transientStrength)
        {
            // Simple adaptive algorithm
            float error = transientStrength - predict(input);
            
            // Update weights using gradient descent
            for (auto& weight : weights)
            {
                weight += learningRate * error * input;
            }
            
            // Update bias
            bias += learningRate * error;
            
            // Decay learning rate over time
            learningRate *= 0.9999f;
            learningRate = juce::jmax(learningRate, 0.0001f);
        }

        float predict(float input) const
        {
            float output = bias;
            for (size_t i = 0; i < weights.size(); ++i)
            {
                output += weights[i] * std::pow(input, static_cast<float>(i + 1));
            }
            return juce::jlimit(0.0f, 1.0f, output);
        }

        void reset()
        {
            weights = {0.1f, 0.05f, 0.01f}; // Initialize with small random values
            bias = 0.0f;
            learningRate = 0.001f;
        }

    private:
        std::vector<float> weights;
        float bias = 0.0f;
        float learningRate = 0.001f;
        double sampleRate = 44100.0;
    };

    AdaptiveLearning adaptiveLearning;

    // Shaping curve functions
    float applyShapingCurve(float input, float gain, int curveType) const
    {
        if (gain == 1.0f) return input;
        
        switch (curveType)
        {
            case LINEAR:
                return input * gain;
                
            case EXPONENTIAL:
                return input * std::pow(gain, 2.0f);
                
            case LOGARITHMIC:
                return input * (1.0f + std::log(gain) * 0.5f);
                
            case S_CURVE:
            {
                float t = (gain - 1.0f) * 0.5f + 0.5f; // Normalize to 0-1
                float curve = t * t * (3.0f - 2.0f * t); // Smoothstep
                return input * (1.0f + (curve - 0.5f) * 2.0f);
            }
            
            case CUSTOM:
            {
                // Custom curve with user-defined control points
                float normalizedGain = (gain - 0.1f) / 1.9f; // Normalize to 0-1
                float curve = customCurveInterpolation(normalizedGain);
                return input * curve;
            }
            
            default:
                return input * gain;
        }
    }

    float customCurveInterpolation(float t) const
    {
        // Cubic spline interpolation through control points
        static const std::array<float, 5> controlPoints = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        if (t <= 0.0f) return controlPoints[0];
        if (t >= 1.0f) return controlPoints[4];
        
        float scaledT = t * 4.0f;
        int index = static_cast<int>(scaledT);
        float frac = scaledT - index;
        
        if (index >= 3) { index = 3; frac = 1.0f; }
        
        // Catmull-Rom spline interpolation
        float p0 = (index > 0) ? controlPoints[index - 1] : controlPoints[0];
        float p1 = controlPoints[index];
        float p2 = controlPoints[index + 1];
        float p3 = (index < 3) ? controlPoints[index + 2] : controlPoints[4];
        
        float t2 = frac * frac;
        float t3 = t2 * frac;
        
        return 0.5f * ((2.0f * p1) +
                      (-p0 + p2) * frac +
                      (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                      (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    }

    // ML-inspired prediction system
    class PredictiveEngine
    {
    public:
        void prepareToPlay(double sampleRate, int samplesPerBlock)
        {
            this->sampleRate = sampleRate;
            historySize = static_cast<int>(sampleRate * 0.05); // 50ms history
            inputHistory.resize(historySize, 0.0f);
            transientHistory.resize(historySize, 0.0f);
            historyIndex = 0;
            
            // Initialize neural network weights (simplified)
            hiddenWeights.resize(HIDDEN_NEURONS * INPUT_FEATURES, 0.0f);
            outputWeights.resize(HIDDEN_NEURONS, 0.0f);
            
            // Random initialization
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> dist(0.0f, 0.1f);
            
            for (auto& weight : hiddenWeights)
                weight = dist(gen);
            for (auto& weight : outputWeights)
                weight = dist(gen);
        }

        float predict(float currentInput, float currentTransient)
        {
            // Update history
            inputHistory[historyIndex] = currentInput;
            transientHistory[historyIndex] = currentTransient;
            historyIndex = (historyIndex + 1) % historySize;
            
            // Extract features from recent history
            std::array<float, INPUT_FEATURES> features = extractFeatures();
            
            // Forward pass through neural network
            std::array<float, HIDDEN_NEURONS> hiddenOutputs;
            for (int h = 0; h < HIDDEN_NEURONS; ++h)
            {
                float sum = 0.0f;
                for (int i = 0; i < INPUT_FEATURES; ++i)
                {
                    sum += features[i] * hiddenWeights[h * INPUT_FEATURES + i];
                }
                hiddenOutputs[h] = std::tanh(sum); // Activation function
            }
            
            // Output layer
            float prediction = 0.0f;
            for (int h = 0; h < HIDDEN_NEURONS; ++h)
            {
                prediction += hiddenOutputs[h] * outputWeights[h];
            }
            
            return juce::jlimit(0.0f, 1.0f, prediction);
        }

        void reset()
        {
            std::fill(inputHistory.begin(), inputHistory.end(), 0.0f);
            std::fill(transientHistory.begin(), transientHistory.end(), 0.0f);
            historyIndex = 0;
        }

    private:
        static constexpr int INPUT_FEATURES = 8;
        static constexpr int HIDDEN_NEURONS = 16;
        
        std::array<float, INPUT_FEATURES> extractFeatures()
        {
            std::array<float, INPUT_FEATURES> features{};
            
            // Feature 1: RMS of recent input
            float rms = 0.0f;
            for (int i = 0; i < std::min(100, historySize); ++i)
            {
                int idx = (historyIndex - i - 1 + historySize) % historySize;
                rms += inputHistory[idx] * inputHistory[idx];
            }
            features[0] = std::sqrt(rms / 100.0f);
            
            // Feature 2: Peak of recent input
            float peak = 0.0f;
            for (int i = 0; i < std::min(100, historySize); ++i)
            {
                int idx = (historyIndex - i - 1 + historySize) % historySize;
                peak = juce::jmax(peak, std::abs(inputHistory[idx]));
            }
            features[1] = peak;
            
            // Feature 3: Zero crossing rate
            int zeroCrossings = 0;
            for (int i = 1; i < std::min(100, historySize); ++i)
            {
                int idx1 = (historyIndex - i - 1 + historySize) % historySize;
                int idx2 = (historyIndex - i + historySize) % historySize;
                if ((inputHistory[idx1] >= 0.0f) != (inputHistory[idx2] >= 0.0f))
                    zeroCrossings++;
            }
            features[2] = static_cast<float>(zeroCrossings) / 99.0f;
            
            // Feature 4: Spectral centroid (simplified)
            features[3] = calculateSpectralCentroid();
            
            // Feature 5: Recent transient activity
            float recentTransients = 0.0f;
            for (int i = 0; i < std::min(50, historySize); ++i)
            {
                int idx = (historyIndex - i - 1 + historySize) % historySize;
                recentTransients += transientHistory[idx];
            }
            features[4] = recentTransients / 50.0f;
            
            // Feature 6: Input trend (first derivative)
            float trend = 0.0f;
            for (int i = 1; i < std::min(20, historySize); ++i)
            {
                int idx1 = (historyIndex - i - 1 + historySize) % historySize;
                int idx2 = (historyIndex - i + historySize) % historySize;
                trend += inputHistory[idx2] - inputHistory[idx1];
            }
            features[5] = trend / 19.0f;
            
            // Feature 7: Input acceleration (second derivative)
            float acceleration = 0.0f;
            for (int i = 2; i < std::min(20, historySize); ++i)
            {
                int idx1 = (historyIndex - i - 1 + historySize) % historySize;
                int idx2 = (historyIndex - i + historySize) % historySize;
                int idx3 = (historyIndex - i + 1 + historySize) % historySize;
                float deriv1 = inputHistory[idx2] - inputHistory[idx1];
                float deriv2 = inputHistory[idx3] - inputHistory[idx2];
                acceleration += deriv2 - deriv1;
            }
            features[6] = acceleration / 18.0f;
            
            // Feature 8: Periodicity measure
            features[7] = calculatePeriodicity();
            
            return features;
        }

        float calculateSpectralCentroid()
        {
            float weightedSum = 0.0f;
            float magnitudeSum = 0.0f;
            
            for (int i = 0; i < std::min(100, historySize); ++i)
            {
                int idx = (historyIndex - i - 1 + historySize) % historySize;
                float magnitude = std::abs(inputHistory[idx]);
                weightedSum += magnitude * i;
                magnitudeSum += magnitude;
            }
            
            return (magnitudeSum > 0.0f) ? weightedSum / magnitudeSum / 100.0f : 0.0f;
        }

        float calculatePeriodicity()
        {
            // Simple autocorrelation-based periodicity measure
            float maxCorrelation = 0.0f;
            int windowSize = std::min(200, historySize / 2);
            
            for (int lag = 10; lag < windowSize; ++lag)
            {
                float correlation = 0.0f;
                for (int i = 0; i < windowSize - lag; ++i)
                {
                    int idx1 = (historyIndex - i - 1 + historySize) % historySize;
                    int idx2 = (historyIndex - i - lag - 1 + historySize) % historySize;
                    correlation += inputHistory[idx1] * inputHistory[idx2];
                }
                maxCorrelation = juce::jmax(maxCorrelation, std::abs(correlation));
            }
            
            return maxCorrelation / (windowSize * windowSize);
        }

        std::vector<float> inputHistory;
        std::vector<float> transientHistory;
        std::vector<float> hiddenWeights;
        std::vector<float> outputWeights;
        int historySize = 2205;
        int historyIndex = 0;
        double sampleRate = 44100.0;
    };

    PredictiveEngine predictiveEngine;

    // Sidechain processing
    juce::AudioBuffer<float> sidechainBuffer;
    bool hasSidechainInput = false;

    // Denormal protection using bit manipulation
    inline float removeDenormals(float sample) const
    {
        // Use bit manipulation for efficient denormal removal
        union { float f; uint32_t i; } u;
        u.f = sample;
        if ((u.i & 0x7f800000) == 0) // Check if exponent is zero
            u.f = 0.0f;
        return u.f;
    }

    // Processing helpers
    void initializeDetectors();
    void processMultiband(juce::AudioBuffer<float>& buffer);
    void processSingleBand(juce::AudioBuffer<float>& buffer);
    void applyLookaheadProcessing(juce::AudioBuffer<float>& buffer);
    float calculateTransientStrength(float sample, int channel);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientShaper_Ultimate)
};