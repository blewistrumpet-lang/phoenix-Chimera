#include "MultibandSaturator.h"
#include <cmath>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Platform-specific SIMD support (should match header)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define HAS_SIMD 1
#else
    #define HAS_SIMD 0
#endif

// Enable FTZ/DAZ globally for denormal prevention
namespace {
    struct DenormalDisabler {
        DenormalDisabler() {
            #if HAS_SIMD && defined(__SSE__)
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
            #endif
        }
    } denormalDisabler;
}

//==============================================================================
// ButterworthSection Implementation
//==============================================================================
void MultibandSaturator::ButterworthSection::calculateCoefficients(double freq, double sampleRate, bool highpass) {
    double w = 2.0 * M_PI * freq / sampleRate;
    double cosw = std::cos(w);
    double sinw = std::sin(w);
    double sqrt2 = std::sqrt(2.0);
    double alpha = sinw / sqrt2;
    
    if (highpass) {
        double norm = 1.0 / (1.0 + alpha);
        b0 = (1.0 + cosw) / 2.0 * norm;
        b1 = -(1.0 + cosw) * norm;
        b2 = b0;
        a1 = -2.0 * cosw * norm;
        a2 = (1.0 - alpha) * norm;
    } else {
        double norm = 1.0 / (1.0 + alpha);
        b0 = (1.0 - cosw) / 2.0 * norm;
        b1 = (1.0 - cosw) * norm;
        b2 = b0;
        a1 = -2.0 * cosw * norm;
        a2 = (1.0 - alpha) * norm;
    }
}

//==============================================================================
// CrossoverNetwork Implementation
//==============================================================================
void MultibandSaturator::CrossoverNetwork::setup(double sampleRate) {
    lowLP.setup(LOW_CROSSOVER_FREQ, sampleRate, false);
    lowHP.setup(LOW_CROSSOVER_FREQ, sampleRate, true);
    midLP.setup(HIGH_CROSSOVER_FREQ, sampleRate, false);
    midHP.setup(HIGH_CROSSOVER_FREQ, sampleRate, true);
}

MultibandSaturator::CrossoverNetwork::BandOutputs 
MultibandSaturator::CrossoverNetwork::process(double input) noexcept {
    BandOutputs output;
    output.low = lowLP.process(input);
    double midHigh = lowHP.process(input);
    output.mid = midLP.process(midHigh);
    output.high = midHP.process(midHigh);
    return output;
}

void MultibandSaturator::CrossoverNetwork::reset() {
    lowLP.reset();
    lowHP.reset();
    midLP.reset();
    midHP.reset();
}

//==============================================================================
// PolyphaseOversampler Implementation
//==============================================================================
void MultibandSaturator::PolyphaseOversampler::prepare() {
    // Polyphase coefficients for 4x oversampling
    // These provide ~80dB stopband with minimal latency
    upPhase[0].setCoefficient(0.04104245150566);
    upPhase[1].setCoefficient(0.25486358142037);
    upPhase[2].setCoefficient(0.57406208636789);
    upPhase[3].setCoefficient(0.88149860099754);
    
    downPhase[0].setCoefficient(0.04104245150566);
    downPhase[1].setCoefficient(0.25486358142037);
    downPhase[2].setCoefficient(0.57406208636789);
    downPhase[3].setCoefficient(0.88149860099754);
}

void MultibandSaturator::PolyphaseOversampler::processUpsample(const double* input, double* output, int numSamples) noexcept {
    for (int i = 0; i < numSamples; ++i) {
        double sample = input[i];
        
        // Phase 0 (original sample)
        output[i * OVERSAMPLE_FACTOR] = sample;
        
        // Phases 1-3 (interpolated)
        double phase1 = upPhase[0].process(sample);
        double phase2 = upPhase[1].process(phase1);
        double phase3 = upPhase[2].process(phase2);
        double phase4 = upPhase[3].process(phase3);
        
        output[i * OVERSAMPLE_FACTOR + 1] = (phase1 + phase4) * 0.5;
        output[i * OVERSAMPLE_FACTOR + 2] = phase2;
        output[i * OVERSAMPLE_FACTOR + 3] = (phase3 + sample) * 0.5;
    }
}

void MultibandSaturator::PolyphaseOversampler::processDownsample(const double* input, double* output, int numSamples) noexcept {
    for (int i = 0; i < numSamples; ++i) {
        // Anti-alias then decimate
        double sum = 0.0;
        for (int j = 0; j < OVERSAMPLE_FACTOR; ++j) {
            double sample = input[i * OVERSAMPLE_FACTOR + j];
            sum += downPhase[j].process(sample);
        }
        output[i] = sum * 0.25; // Average
    }
}

void MultibandSaturator::PolyphaseOversampler::reset() {
    for (auto& phase : upPhase) phase.reset();
    for (auto& phase : downPhase) phase.reset();
}

//==============================================================================
// ChannelProcessor Implementation
//==============================================================================
void MultibandSaturator::ChannelProcessor::prepare(double sampleRate, int maxBlockSize) {
    crossover.setup(sampleRate);
    
    for (auto& oversampler : oversamplers) {
        oversampler.prepare();
    }
    
    // Pre-allocate all buffers with alignment
    int maxSamples = maxBlockSize;
    int oversampleSize = maxSamples * OVERSAMPLE_FACTOR;
    
    inputBuffer.resize(maxSamples);
    lowBand.resize(maxSamples);
    midBand.resize(maxSamples);
    highBand.resize(maxSamples);
    oversampledBuffer.resize(oversampleSize);
    oversampledOutput.resize(oversampleSize);
    
    // Ensure buffers are zero-initialized
    std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0);
    std::fill(lowBand.begin(), lowBand.end(), 0.0);
    std::fill(midBand.begin(), midBand.end(), 0.0);
    std::fill(highBand.begin(), highBand.end(), 0.0);
    std::fill(oversampledBuffer.begin(), oversampledBuffer.end(), 0.0);
    std::fill(oversampledOutput.begin(), oversampledOutput.end(), 0.0);
}

void MultibandSaturator::ChannelProcessor::reset() {
    crossover.reset();
    for (auto& oversampler : oversamplers) {
        oversampler.reset();
    }
    inputDC.reset();
    outputDC.reset();
    satStates.reset();
}

//==============================================================================
// MultibandSaturator Implementation
//==============================================================================
MultibandSaturator::MultibandSaturator() {
    // Initialize parameters with musical defaults
    m_lowDrive.setImmediate(1.0);
    m_midDrive.setImmediate(1.0);
    m_highDrive.setImmediate(1.0);
    m_saturationType.setImmediate(0.0);
    m_harmonicCharacter.setImmediate(0.5);
    m_outputGain.setImmediate(1.0);
    m_mix.setImmediate(1.0);
    
    // Initialize with 2 channels
    m_channelProcessors.resize(2);
    for (auto& processor : m_channelProcessors) {
        processor = std::make_unique<ChannelProcessor>();
    }
}

MultibandSaturator::~MultibandSaturator() = default;

void MultibandSaturator::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_samplesPerBlock = samplesPerBlock;
    
    // Set parameter smoothing rates (5ms)
    m_lowDrive.setSmoothingCoeff(5.0, sampleRate);
    m_midDrive.setSmoothingCoeff(5.0, sampleRate);
    m_highDrive.setSmoothingCoeff(5.0, sampleRate);
    m_saturationType.setSmoothingCoeff(10.0, sampleRate);
    m_harmonicCharacter.setSmoothingCoeff(10.0, sampleRate);
    m_outputGain.setSmoothingCoeff(5.0, sampleRate);
    m_mix.setSmoothingCoeff(5.0, sampleRate);
    
    // Prepare all channel processors
    for (auto& processor : m_channelProcessors) {
        processor->prepare(sampleRate, samplesPerBlock);
        processor->reset();
    }
    
    reset();
}

void MultibandSaturator::reset() {
    // Reset all processors
    for (auto& processor : m_channelProcessors) {
        processor->reset();
    }
    
    // Reset parameters to current values
    m_lowDrive.setImmediate(m_lowDrive.current);
    m_midDrive.setImmediate(m_midDrive.current);
    m_highDrive.setImmediate(m_highDrive.current);
    m_saturationType.setImmediate(m_saturationType.current);
    m_harmonicCharacter.setImmediate(m_harmonicCharacter.current);
    m_outputGain.setImmediate(m_outputGain.current);
    m_mix.setImmediate(m_mix.current);
}

void MultibandSaturator::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Process each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        auto& processor = *m_channelProcessors[ch % m_channelProcessors.size()];
        
        // Use pre-allocated buffers
        double* inputBuffer = processor.inputBuffer.data();
        double* lowBand = processor.lowBand.data();
        double* midBand = processor.midBand.data();
        double* highBand = processor.highBand.data();
        
        // Convert to double and apply input DC blocking
        for (int i = 0; i < numSamples; ++i) {
            inputBuffer[i] = processor.inputDC.process(static_cast<double>(channelData[i]));
        }
        
        // Process crossover network
        for (int i = 0; i < numSamples; ++i) {
            auto bands = processor.crossover.process(inputBuffer[i]);
            lowBand[i] = bands.low;
            midBand[i] = bands.mid;
            highBand[i] = bands.high;
        }
        
        // Process each band with per-sample parameter updates
        for (int i = 0; i < numSamples; ++i) {
            // Update parameters per sample
            double lowDrive = m_lowDrive.tick();
            double midDrive = m_midDrive.tick();
            double highDrive = m_highDrive.tick();
            double satTypeValue = m_saturationType.tick();
            double harmonics = m_harmonicCharacter.tick();
            double outputGain = m_outputGain.tick();
            double mixAmount = m_mix.tick();
            
            // Determine saturation type
            SaturationType satType;
            if (satTypeValue < 0.25) {
                satType = SaturationType::TUBE;
            } else if (satTypeValue < 0.5) {
                satType = SaturationType::TAPE;
            } else if (satTypeValue < 0.75) {
                satType = SaturationType::TRANSISTOR;
            } else {
                satType = SaturationType::DIODE;
            }
            
            // Store current samples for oversampling
            double lowSample = lowBand[i];
            double midSample = midBand[i];
            double highSample = highBand[i];
            
            // Process bands with oversampling (single sample at a time)
            processBand(&lowSample, 1, processor.oversamplers[0],
                       processor.oversampledBuffer.data(),
                       processor.oversampledOutput.data(),
                       satType, processor);
            lowBand[i] = lowSample * lowDrive;
            
            processBand(&midSample, 1, processor.oversamplers[1],
                       processor.oversampledBuffer.data(),
                       processor.oversampledOutput.data(),
                       satType, processor);
            midBand[i] = midSample * midDrive;
            
            processBand(&highSample, 1, processor.oversamplers[2],
                       processor.oversampledBuffer.data(),
                       processor.oversampledOutput.data(),
                       satType, processor);
            highBand[i] = highSample * highDrive;
            
            // Recombine bands
            double wet = lowBand[i] + midBand[i] + highBand[i];
            
            // Apply output DC blocking
            wet = processor.outputDC.process(wet);
            
            // Apply output gain
            wet *= outputGain;
            
            // Mix with dry signal
            double dry = inputBuffer[i];
            double output = dry * (1.0 - mixAmount) + wet * mixAmount;
            
            // Soft clipping for safety
            if (std::abs(output) > 0.95) {
                output = std::tanh(output);
            }
            
            // Convert back to float with denormal prevention
            channelData[i] = static_cast<float>(preventDenormal(output));
        }
    }
}

void MultibandSaturator::processBand(double* samples, int numSamples,
                                    PolyphaseOversampler& oversampler,
                                    double* oversampledBuffer,
                                    double* oversampledOutput,
                                    SaturationType type,
                                    ChannelProcessor& processor) noexcept {
    // Get current parameter values
    double drive = m_lowDrive.current; // Will be updated per-band in process()
    double harmonics = m_harmonicCharacter.current;
    
    // Upsample
    oversampler.processUpsample(samples, oversampledBuffer, numSamples);
    
    // Process at higher sample rate
    int oversampledSamples = numSamples * OVERSAMPLE_FACTOR;
    
    for (int i = 0; i < oversampledSamples; ++i) {
        double input = oversampledBuffer[i];
        double output = 0.0;
        
        // Apply saturation based on type
        switch (type) {
            case SaturationType::TUBE:
                output = saturateTube(input, drive, harmonics, processor.satStates);
                break;
                
            case SaturationType::TAPE:
                output = saturateTape(input, drive, harmonics, processor.satStates);
                break;
                
            case SaturationType::TRANSISTOR:
                output = saturateTransistor(input, drive, harmonics, processor.satStates);
                break;
                
            case SaturationType::DIODE:
                output = saturateDiode(input, drive, harmonics, processor.satStates);
                break;
        }
        
        oversampledOutput[i] = output;
    }
    
    // Downsample back to original rate
    oversampler.processDownsample(oversampledOutput, samples, numSamples);
}

double MultibandSaturator::saturateTube(double input, double drive, double harmonics,
                                       ChannelProcessor::SaturationStates& states) const noexcept {
    const double preEmphCutoff = 0.15; // ~3kHz at 44.1kHz
    
    // Apply pre-emphasis
    double emphasized = input - states.tubePreEmphState;
    states.tubePreEmphState += emphasized * preEmphCutoff;
    states.tubePreEmphState = preventDenormal(states.tubePreEmphState);
    
    // Scale input
    double x = emphasized * drive;
    double absX = std::abs(x);
    
    // Tube transfer function with asymmetry
    double y = 0.0;
    
    if (absX < 0.7) {
        // Linear region with 2nd harmonic
        y = x * (1.0 + 0.15 * absX * (1.0 - harmonics * 0.5));
        
        // Add subtle 3rd harmonic
        if (absX > 0.3) {
            y += x * x * x * 0.02 * harmonics;
        }
    } else {
        // Soft saturation with asymmetry
        double excess = absX - 0.7;
        double saturation = 0.7 + std::tanh(excess * 2.0) * 0.3;
        
        // Different curves for positive/negative
        if (x > 0) {
            saturation *= (1.0 + 0.05 * (1.0 - harmonics));
        } else {
            saturation *= (1.0 - 0.03 * (1.0 - harmonics));
        }
        
        y = (x > 0) ? saturation : -saturation;
        
        // Add harmonics based on balance
        double h2 = y * y * ((y > 0) ? 1.0 : -1.0); // Even harmonics
        double h3 = y * y * y;                       // Odd harmonics
        double h5 = y * y * y * y * y;               // 5th harmonic
        
        y += h2 * 0.08 * (1.0 - harmonics);
        y += h3 * 0.12 * harmonics;
        y += h5 * 0.02 * harmonics;
    }
    
    // De-emphasis
    double output = y + states.tubeDeEmphState * (1.0 - preEmphCutoff);
    states.tubeDeEmphState = preventDenormal(y);
    
    return output * 0.7; // Headroom
}

double MultibandSaturator::saturateTape(double input, double drive, double harmonics,
                                       ChannelProcessor::SaturationStates& states) const noexcept {
    const double hystAmount = 0.1 * harmonics;
    double x = input * drive;
    
    // Tape compression curve
    const double threshold = 0.6;
    const double knee = 0.1;
    const double ratio = 3.0;
    
    double absX = std::abs(x);
    
    if (absX > threshold - knee) {
        // Soft knee compression
        if (absX < threshold + knee) {
            // Interpolate in knee region
            double kneePos = (absX - (threshold - knee)) / (2.0 * knee);
            double kneeFactor = kneePos * kneePos;
            double linearGain = 1.0;
            double compressedGain = (threshold + (absX - threshold) / ratio) / absX;
            double gain = linearGain + (compressedGain - linearGain) * kneeFactor;
            x *= gain;
        } else {
            // Full compression
            double over = absX - threshold;
            double compressed = threshold + over / ratio;
            x = (x > 0) ? compressed : -compressed;
        }
    }
    
    // Hysteresis modeling
    double hyst = x - states.tapeHystState;
    double hystDrive = 0.1 + 0.2 * (1.0 - harmonics);
    states.tapeHystState += hyst * hystDrive;
    states.tapeHystState = preventDenormal(states.tapeHystState);
    
    // Apply hysteresis with bias
    x += hyst * hystAmount;
    x += states.tapeHystState * 0.05 * (1.0 - harmonics);
    
    // Tape saturation curve
    double satX = x * 1.5;
    double tape = 0.0;
    
    if (std::abs(satX) < 1.0) {
        // Smooth S-curve
        tape = satX - (satX * satX * satX) / 3.0;
    } else {
        // Soft clipping
        tape = std::tanh(satX);
    }
    
    // Add tape warmth (even harmonics)
    tape += tape * tape * ((tape > 0) ? 1.0 : -1.0) * 0.05 * (1.0 - harmonics);
    
    // Frequency-dependent saturation (high frequency compression)
    double highFreq = tape - states.tapeHighState;
    states.tapeHighState += highFreq * 0.3;
    states.tapeHighState = preventDenormal(states.tapeHighState);
    
    // Reduce high frequency content when saturating
    if (std::abs(tape) > 0.7) {
        tape = states.tapeHighState + highFreq * (1.0 - 0.3 * (std::abs(tape) - 0.7));
    }
    
    return tape * 0.8;
}

double MultibandSaturator::saturateTransistor(double input, double drive, double harmonics,
                                             ChannelProcessor::SaturationStates& states) const noexcept {
    double x = input * drive;
    
    // Crossover distortion modeling
    const double crossoverBase = 0.02;
    const double crossover = crossoverBase * (1.0 - harmonics * 0.5);
    const double crossoverSlope = 0.1;
    
    if (std::abs(x) < crossover) {
        // Dead zone with smooth transition
        double ratio = std::abs(x) / crossover;
        x *= ratio * ratio * crossoverSlope;
    } else {
        // Active region
        x = (x > 0) ? x - crossover * (1.0 - crossoverSlope) 
                    : x + crossover * (1.0 - crossoverSlope);
    }
    
    // Class AB push-pull modeling
    double positive = 0.0;
    double negative = 0.0;
    
    if (x > 0) {
        // Positive transistor
        positive = x * 1.2;
        if (positive > 0.7) {
            // Collector saturation
            double excess = positive - 0.7;
            positive = 0.7 + std::tanh(excess * 3.0) * 0.2;
        }
        // Beta variations
        positive *= (1.0 + 0.05 * harmonics);
    } else {
        // Negative transistor (slightly different characteristics)
        negative = x * 1.15;
        if (negative < -0.75) {
            double excess = -0.75 - negative;
            negative = -0.75 - std::tanh(excess * 3.0) * 0.25;
        }
        // Different beta
        negative *= (1.0 + 0.03 * harmonics);
    }
    
    x = positive + negative;
    
    // Add odd harmonics (transistor characteristic)
    double x3 = x * x * x;
    double x5 = x3 * x * x;
    double x7 = x5 * x * x;
    
    x += x3 * 0.1 * harmonics;
    x += x5 * 0.03 * harmonics;
    x += x7 * 0.01 * harmonics;
    
    // Output stage coupling
    double coupled = x - states.transistorCouplingState * 0.995;
    states.transistorCouplingState = preventDenormal(x);
    
    return coupled * 0.8;
}

double MultibandSaturator::saturateDiode(double input, double drive, double harmonics,
                                        ChannelProcessor::SaturationStates& states) const noexcept {
    double x = input * drive;
    
    // Diode parameters
    const double vf = 0.7;        // Forward voltage
    const double vr = 5.0;        // Reverse breakdown
    const double is = 1e-12;      // Saturation current
    const double n = 1.5;         // Ideality factor
    const double vt = 0.026;      // Thermal voltage
    
    // Shockley diode equation approximation
    double diode = 0.0;
    
    if (x > 0) {
        // Forward bias
        if (x < vf * 0.5) {
            // Exponential region
            diode = vf * (std::exp(x / (n * vt * 10.0)) - 1.0) * 0.01;
        } else {
            // Linear region with soft knee
            double excess = x - vf * 0.5;
            diode = vf * 0.5 + std::tanh(excess * 2.0 / vf) * vf * 0.5;
        }
    } else {
        // Reverse bias
        if (x > -vr) {
            // Leakage current
            diode = x * 0.001 * (1.0 + harmonics * 0.01);
        } else {
            // Breakdown region
            double breakdown = x + vr;
            diode = -vr + breakdown * 0.1;
        }
    }
    
    // Junction capacitance effect
    double capCutoff = 0.1 + harmonics * 0.2;
    double capEffect = diode - states.diodeCapState;
    states.diodeCapState += capEffect * capCutoff;
    states.diodeCapState = preventDenormal(states.diodeCapState);
    
    // Capacitance creates frequency-dependent response
    diode = states.diodeCapState + capEffect * (1.0 - capCutoff);
    
    // Temperature drift simulation (use RNG)
    std::uniform_real_distribution<double> dist(-0.001, 0.001);
    states.diodeTempDrift += dist(m_rng);
    states.diodeTempDrift = std::clamp(states.diodeTempDrift, -0.05, 0.05);
    states.diodeTempDrift = preventDenormal(states.diodeTempDrift);
    
    diode *= (1.0 + states.diodeTempDrift * harmonics);
    
    // Recovery time effects
    double recovery = diode - states.diodeRecoveryState;
    states.diodeRecoveryState += recovery * 0.7;
    states.diodeRecoveryState = preventDenormal(states.diodeRecoveryState);
    
    if (recovery * diode < 0) { // Sign change
        diode *= 0.8; // Recovery time softening
    }
    
    return diode * 0.9;
}

void MultibandSaturator::updateParameters(const std::map<int, float>& params) {
    auto updateParam = [&params](int index, SmoothParam& param,
                                double min, double max, double defaultVal) {
        auto it = params.find(index);
        if (it != params.end()) {
            float normalized = it->second;
            double value = min + normalized * (max - min);
            param.target.store(value, std::memory_order_relaxed);
        }
    };
    
    // Update all parameters with appropriate ranges
    updateParam(0, m_lowDrive, 0.1, 10.0, 1.0);        // Low Drive: 0.1-10x
    updateParam(1, m_midDrive, 0.1, 10.0, 1.0);        // Mid Drive: 0.1-10x
    updateParam(2, m_highDrive, 0.1, 10.0, 1.0);       // High Drive: 0.1-10x
    updateParam(3, m_saturationType, 0.0, 1.0, 0.0);   // Saturation Type: 0-1
    updateParam(4, m_harmonicCharacter, 0.0, 1.0, 0.5); // Harmonics: 0-1
    updateParam(5, m_outputGain, 0.0, 2.0, 1.0);       // Output Gain: 0-2x
    updateParam(6, m_mix, 0.0, 1.0, 1.0);              // Mix: 0-100%
}

juce::String MultibandSaturator::getParameterName(int index) const {
    switch (index) {
        case 0: return "Low Drive";
        case 1: return "Mid Drive";
        case 2: return "High Drive";
        case 3: return "Saturation Type";
        case 4: return "Harmonic Character";
        case 5: return "Output Gain";
        case 6: return "Mix";
        default: return "";
    }
}

void MultibandSaturator::mixBandsSIMD(float* output, const double* low, const double* mid,
                                     const double* high, int numSamples) noexcept {
#if HAS_SIMD
    // Process 4 samples at a time using SSE
    int simdSamples = numSamples & ~3;
    
    for (int i = 0; i < simdSamples; i += 4) {
        // Load doubles and convert to float
        __m128d low1 = _mm_loadu_pd(&low[i]);
        __m128d low2 = _mm_loadu_pd(&low[i + 2]);
        __m128d mid1 = _mm_loadu_pd(&mid[i]);
        __m128d mid2 = _mm_loadu_pd(&mid[i + 2]);
        __m128d high1 = _mm_loadu_pd(&high[i]);
        __m128d high2 = _mm_loadu_pd(&high[i + 2]);
        
        // Sum bands
        __m128d sum1 = _mm_add_pd(_mm_add_pd(low1, mid1), high1);
        __m128d sum2 = _mm_add_pd(_mm_add_pd(low2, mid2), high2);
        
        // Convert to float
        __m128 result = _mm_movelh_ps(_mm_cvtpd_ps(sum1), _mm_cvtpd_ps(sum2));
        
        // Store
        _mm_storeu_ps(&output[i], result);
    }
    
    // Process remaining samples
    for (int i = simdSamples; i < numSamples; ++i) {
        output[i] = static_cast<float>(low[i] + mid[i] + high[i]);
    }
#else
    // Non-SIMD fallback
    for (int i = 0; i < numSamples; ++i) {
        output[i] = static_cast<float>(low[i] + mid[i] + high[i]);
    }
#endif
}