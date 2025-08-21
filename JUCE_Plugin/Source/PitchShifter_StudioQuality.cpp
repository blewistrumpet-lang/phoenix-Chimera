// Studio-Quality Pitch Shifter Implementation
// Using Laroche-Dolson Phase Vocoder with proper phase coherence
// References:
// - Laroche & Dolson 1999: "Improved Phase Vocoder Time-Scale Modification of Audio"
// - Bernsee: "Pitch Shifting Using The Fourier Transform"

#include "PitchShifter.h"
#include "DspEngineUtilities.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <array>
#include <algorithm>
#include <complex>

// Studio-quality implementation structure
struct PitchShifter::Impl {
    static constexpr int FFT_ORDER = 13;  // 8192 for better frequency resolution
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    static constexpr int OVERLAP_FACTOR = 8;  // 87.5% overlap for smoother processing
    static constexpr int HOP_SIZE = FFT_SIZE / OVERLAP_FACTOR;
    static constexpr int MAX_CHANNELS = 8;
    
    // Lock-free parameters
    std::atomic<float> pitchRatio{1.0f};
    std::atomic<float> formantShift{0.5f};
    std::atomic<float> mixAmount{1.0f};
    std::atomic<float> windowWidth{0.5f};
    std::atomic<float> spectralGate{0.0f};
    std::atomic<float> grainSize{0.5f};
    std::atomic<float> feedback{0.0f};
    std::atomic<float> stereoWidth{0.5f};
    std::atomic<float> snappedPitchValue{0.5f};
    
    // Per-channel state for studio quality
    struct ChannelState {
        // FFT buffers
        alignas(32) std::array<float, FFT_SIZE * 2> inputBuffer{};
        alignas(32) std::array<float, FFT_SIZE * 2> outputBuffer{};
        alignas(32) std::array<std::complex<float>, FFT_SIZE> spectrum{};
        alignas(32) std::array<float, FFT_SIZE> window{};
        
        // Phase vocoder state (double precision for accuracy)
        alignas(32) std::array<double, FFT_SIZE/2 + 1> lastPhase{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> sumPhase{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> magnitude{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> frequency{};
        
        // Laroche-Dolson phase locking
        alignas(32) std::array<int, FFT_SIZE/2 + 1> peakBins{};
        alignas(32) std::array<bool, FFT_SIZE/2 + 1> isPeak{};
        alignas(32) std::array<int, FFT_SIZE/2 + 1> closestPeak{};
        
        // Buffer positions
        int inputPos{0};
        int outputPos{0};
        int hopCounter{0};
        
        // FFT object
        std::unique_ptr<juce::dsp::FFT> fft;
        
        // DC blocker
        DCBlocker dcBlocker;
        
        void reset() {
            inputBuffer.fill(0.0f);
            outputBuffer.fill(0.0f);
            lastPhase.fill(0.0);
            sumPhase.fill(0.0);
            magnitude.fill(0.0);
            frequency.fill(0.0);
            peakBins.fill(-1);
            isPeak.fill(false);
            closestPeak.fill(-1);
            inputPos = 0;
            outputPos = 0;
            hopCounter = 0;
            dcBlocker.reset();
        }
    };
    
    std::array<ChannelState, MAX_CHANNELS> channels;
    int activeChannels{0};
    double sampleRate{44100.0};
    
    // Pre-computed constants
    double binFrequency{0.0};
    double expectedPhaseInc{0.0};
    double freqPerBin{0.0};
    double outputScale{0.0};
    
    Impl() {
        pitchRatio.store(1.0f);
        formantShift.store(0.5f);
        mixAmount.store(1.0f);
        windowWidth.store(0.5f);
        spectralGate.store(0.0f);
        grainSize.store(0.5f);
        feedback.store(0.0f);
        stereoWidth.store(0.5f);
    }
    
    void prepareToPlay(double sr, int /*samplesPerBlock*/) {
        sampleRate = sr;
        
        // Pre-compute constants
        binFrequency = sr / FFT_SIZE;
        freqPerBin = sr / FFT_SIZE;
        expectedPhaseInc = 2.0 * M_PI * HOP_SIZE / FFT_SIZE;
        
        // Output scaling for 87.5% overlap
        outputScale = 1.0 / (FFT_SIZE * OVERLAP_FACTOR * 0.375);
        
        // Initialize channels
        for (auto& ch : channels) {
            ch.fft = std::make_unique<juce::dsp::FFT>(FFT_ORDER);
            createWindow(ch.window);
            ch.reset();
        }
    }
    
    void createWindow(std::array<float, FFT_SIZE>& window) {
        // Hann window for phase vocoder
        for (int i = 0; i < FFT_SIZE; ++i) {
            const double t = static_cast<double>(i) / (FFT_SIZE - 1);
            window[i] = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * M_PI * t));
        }
    }
    
    // Studio-quality pitch shifting using Laroche-Dolson phase vocoder
    void processChannel(ChannelState& ch, float* data, int numSamples) {
        const float pitch = pitchRatio.load();
        const float mix = mixAmount.load();
        const float formant = formantShift.load();
        
        for (int i = 0; i < numSamples; ++i) {
            const float drySignal = data[i];
            
            // Input to circular buffer
            ch.inputBuffer[ch.inputPos] = drySignal;
            ch.inputPos = (ch.inputPos + 1) % (FFT_SIZE * 2);
            ch.hopCounter++;
            
            // Process frame at hop boundary
            if (ch.hopCounter >= HOP_SIZE) {
                ch.hopCounter = 0;
                processFrame(ch, pitch, formant);
            }
            
            // Output from circular buffer
            float wetSignal = ch.outputBuffer[ch.outputPos];
            ch.outputBuffer[ch.outputPos] = 0.0f;  // Clear after reading
            ch.outputPos = (ch.outputPos + 1) % (FFT_SIZE * 2);
            
            // DC blocking and mix
            wetSignal = ch.dcBlocker.process(wetSignal);
            data[i] = drySignal * (1.0f - mix) + wetSignal * mix;
        }
    }
    
    void processFrame(ChannelState& ch, float pitch, float formant) {
        // 1. Analysis: Extract spectrum with windowing
        alignas(32) std::array<float, FFT_SIZE> frameData{};
        
        // Get windowed frame from input buffer
        int readPos = (ch.inputPos - FFT_SIZE + FFT_SIZE * 2) % (FFT_SIZE * 2);
        for (int i = 0; i < FFT_SIZE; ++i) {
            frameData[i] = ch.inputBuffer[readPos] * ch.window[i];
            readPos = (readPos + 1) % (FFT_SIZE * 2);
        }
        
        // Copy to complex buffer for FFT
        for (int i = 0; i < FFT_SIZE; ++i) {
            ch.spectrum[i] = std::complex<float>(frameData[i], 0.0f);
        }
        
        // Forward FFT
        ch.fft->perform(ch.spectrum.data(), ch.spectrum.data(), false);
        
        // 2. Phase Analysis (Bernsee method)
        analyzePhase(ch);
        
        // 3. Peak Detection for Laroche-Dolson phase locking
        detectPeaks(ch);
        
        // 4. Pitch Shifting with phase coherence
        shiftPitch(ch, pitch, formant);
        
        // 5. Synthesis: IFFT and overlap-add
        ch.fft->perform(ch.spectrum.data(), ch.spectrum.data(), true);
        
        // Overlap-add to output buffer
        int writePos = ch.outputPos;
        for (int i = 0; i < FFT_SIZE; ++i) {
            ch.outputBuffer[writePos] += ch.spectrum[i].real() * ch.window[i] * outputScale;
            writePos = (writePos + 1) % (FFT_SIZE * 2);
        }
    }
    
    void analyzePhase(ChannelState& ch) {
        for (int k = 0; k <= FFT_SIZE/2; ++k) {
            const auto& bin = ch.spectrum[k];
            
            // Magnitude and phase
            const double mag = std::abs(bin);
            const double phase = std::arg(bin);
            
            // Phase difference
            double phaseDiff = phase - ch.lastPhase[k];
            ch.lastPhase[k] = phase;
            
            // Princarg wrapping
            phaseDiff = phaseDiff - 2.0 * M_PI * std::round(phaseDiff / (2.0 * M_PI));
            
            // Frequency deviation from expected
            const double expectedPhase = k * expectedPhaseInc;
            const double deviation = phaseDiff - expectedPhase;
            const double wrappedDeviation = deviation - 2.0 * M_PI * std::round(deviation / (2.0 * M_PI));
            
            // True frequency
            ch.frequency[k] = (k + wrappedDeviation / (2.0 * M_PI) * FFT_SIZE / HOP_SIZE) * freqPerBin;
            ch.magnitude[k] = mag;
        }
    }
    
    void detectPeaks(ChannelState& ch) {
        // Reset peak detection
        ch.isPeak.fill(false);
        ch.closestPeak.fill(-1);
        
        // Find spectral peaks (local maxima)
        for (int k = 2; k < FFT_SIZE/2 - 2; ++k) {
            const double mag = ch.magnitude[k];
            
            // Local maximum detection with wider window
            if (mag > ch.magnitude[k-1] * 1.1 &&
                mag > ch.magnitude[k+1] * 1.1 &&
                mag > ch.magnitude[k-2] * 1.05 &&
                mag > ch.magnitude[k+2] * 1.05 &&
                mag > 0.0001) {
                ch.isPeak[k] = true;
            }
        }
        
        // Assign each bin to its closest peak (for phase locking)
        for (int k = 0; k <= FFT_SIZE/2; ++k) {
            if (ch.isPeak[k]) {
                ch.closestPeak[k] = k;  // Peak is its own reference
            } else {
                // Find closest peak
                int closestDist = FFT_SIZE;
                int closestIdx = -1;
                
                for (int p = std::max(0, k - 50); p <= std::min(FFT_SIZE/2, k + 50); ++p) {
                    if (ch.isPeak[p]) {
                        int dist = std::abs(p - k);
                        if (dist < closestDist) {
                            closestDist = dist;
                            closestIdx = p;
                        }
                    }
                }
                
                ch.closestPeak[k] = closestIdx;
            }
        }
    }
    
    void shiftPitch(ChannelState& ch, float pitch, float formant) {
        // Temporary storage for shifted spectrum
        alignas(32) std::array<std::complex<float>, FFT_SIZE> shiftedSpectrum{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> shiftedPhase{};
        
        // Phase accumulator update with Laroche-Dolson improvements
        for (int k = 0; k <= FFT_SIZE/2; ++k) {
            // Shift frequency
            const double shiftedFreq = ch.frequency[k] * pitch;
            
            // Phase advance
            const double phaseAdvance = 2.0 * M_PI * shiftedFreq * HOP_SIZE / sampleRate;
            
            // Update phase accumulator
            ch.sumPhase[k] += phaseAdvance;
            
            // Princarg wrapping
            ch.sumPhase[k] = ch.sumPhase[k] - 2.0 * M_PI * std::round(ch.sumPhase[k] / (2.0 * M_PI));
            
            shiftedPhase[k] = ch.sumPhase[k];
        }
        
        // Laroche-Dolson vertical phase coherence
        // Lock phases of non-peak bins to maintain relationships
        for (int k = 0; k <= FFT_SIZE/2; ++k) {
            if (!ch.isPeak[k] && ch.closestPeak[k] >= 0) {
                int peakBin = ch.closestPeak[k];
                
                // Calculate expected phase relationship
                double expectedPhaseDiff = 2.0 * M_PI * (k - peakBin) * HOP_SIZE / FFT_SIZE;
                
                // Lock phase to peak
                shiftedPhase[k] = shiftedPhase[peakBin] + expectedPhaseDiff;
                
                // Wrap phase
                shiftedPhase[k] = shiftedPhase[k] - 2.0 * M_PI * std::round(shiftedPhase[k] / (2.0 * M_PI));
            }
        }
        
        // Magnitude interpolation and formant shifting
        for (int k = 0; k <= FFT_SIZE/2; ++k) {
            // Find source bin for magnitude
            float sourceBin = k / pitch;
            
            // Apply formant shift (spectral envelope manipulation)
            if (formant != 0.5f) {
                // Formant shifting by warping the spectral envelope
                float formantFactor = 1.0f + (formant - 0.5f) * 2.0f;  // 0 to 2
                sourceBin = k / formantFactor;
            }
            
            double mag = 0.0;
            
            if (sourceBin >= 0 && sourceBin < FFT_SIZE/2) {
                // High-quality sinc interpolation for magnitude
                int baseIdx = static_cast<int>(sourceBin);
                float frac = sourceBin - baseIdx;
                
                // 4-point sinc interpolation
                const int points = 4;
                for (int p = -points/2; p <= points/2; ++p) {
                    int idx = baseIdx + p;
                    if (idx >= 0 && idx <= FFT_SIZE/2) {
                        float x = frac - p;
                        float sinc = (std::abs(x) < 1e-6f) ? 1.0f : std::sin(M_PI * x) / (M_PI * x);
                        
                        // Blackman window for sinc
                        float n = (p + points/2) / static_cast<float>(points);
                        float window = 0.42f - 0.5f * std::cos(2.0f * M_PI * n) + 
                                      0.08f * std::cos(4.0f * M_PI * n);
                        
                        mag += ch.magnitude[idx] * sinc * window;
                    }
                }
            }
            
            // Create complex number with shifted magnitude and phase
            shiftedSpectrum[k] = std::polar(static_cast<float>(mag), static_cast<float>(shiftedPhase[k]));
        }
        
        // Maintain Hermitian symmetry for real output
        for (int k = 1; k < FFT_SIZE/2; ++k) {
            shiftedSpectrum[FFT_SIZE - k] = std::conj(shiftedSpectrum[k]);
        }
        
        ch.spectrum = shiftedSpectrum;
    }
    
    void processStereoWidth(float* left, float* right, int numSamples) {
        const float width = stereoWidth.load() * 2.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            const float mid = (left[i] + right[i]) * 0.5f;
            const float side = (left[i] - right[i]) * 0.5f * width;
            left[i] = mid + side;
            right[i] = mid - side;
        }
    }
};

// Public interface
PitchShifter::PitchShifter() : pimpl(std::make_unique<Impl>()) {}
PitchShifter::~PitchShifter() = default;

void PitchShifter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepareToPlay(sampleRate, samplesPerBlock);
}

void PitchShifter::reset() {
    for (auto& ch : pimpl->channels) {
        ch.reset();
    }
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = std::min(buffer.getNumChannels(), Impl::MAX_CHANNELS);
    const int numSamples = buffer.getNumSamples();
    
    pimpl->activeChannels = numChannels;
    
    // Process each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        pimpl->processChannel(pimpl->channels[ch], buffer.getWritePointer(ch), numSamples);
    }
    
    // Apply stereo width if stereo
    if (numChannels >= 2) {
        pimpl->processStereoWidth(buffer.getWritePointer(0), 
                                 buffer.getWritePointer(1), 
                                 numSamples);
    }
    
    // Scrub buffer for NaN/Inf protection
    scrubBuffer(buffer);
}

void PitchShifter::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (index) {
            case kPitch: {
                // Snap to musical intervals
                float snappedValue = value;
                const float snapPoints[] = {
                    0.250f, 0.354f, 0.396f, 0.417f, 0.438f, 0.479f,
                    0.500f, 0.521f, 0.563f, 0.583f, 0.604f, 0.646f, 0.750f
                };
                
                float minDistance = 1.0f;
                for (float snapPoint : snapPoints) {
                    float distance = std::abs(value - snapPoint);
                    if (distance < minDistance) {
                        minDistance = distance;
                        snappedValue = snapPoint;
                    }
                }
                
                pimpl->snappedPitchValue.store(snappedValue);
                float semitones = (snappedValue - 0.5f) * 48.0f;
                float ratio = std::pow(2.0f, semitones / 12.0f);
                pimpl->pitchRatio.store(ratio);
                break;
            }
            case kFormant:  pimpl->formantShift.store(value); break;
            case kMix:      pimpl->mixAmount.store(value); break;
            case kWindow:   pimpl->windowWidth.store(value); break;
            case kGate:     pimpl->spectralGate.store(value); break;
            case kGrain:    pimpl->grainSize.store(value); break;
            case kFeedback: pimpl->feedback.store(value * 0.9f); break;
            case kWidth:    pimpl->stereoWidth.store(value); break;
        }
    }
}

juce::String PitchShifter::getParameterName(int index) const {
    switch (index) {
        case kPitch:    return "Pitch";
        case kFormant:  return "Formant";
        case kMix:      return "Mix";
        case kWindow:   return "Window";
        case kGate:     return "Gate";
        case kGrain:    return "Grain";
        case kFeedback: return "Feedback";
        case kWidth:    return "Width";
        default:        return "";
    }
}

juce::String PitchShifter::getParameterText(int index, float /*value*/) const {
    if (index == kPitch) {
        float snappedValue = pimpl->snappedPitchValue.load();
        return juce::String(snappedValue, 3);
    }
    return "";
}