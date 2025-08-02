#include "SpectralFreeze.h"
#include <algorithm>

SpectralFreeze::SpectralFreeze() : m_rng(std::random_device{}()) {
    m_channels.resize(2);
}

void SpectralFreeze::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize channels with maximum FFT size
    for (auto& channel : m_channels) {
        channel.init(MAX_FFT_SIZE);
        channel.reset();
    }

void SpectralFreeze::reset() {
    // Reset all internal state
    // TODO: Implement specific reset logic for SpectralFreeze
}

}

void SpectralFreeze::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update FFT size based on resolution parameter
    int newFftSize = getFFTSizeFromParameter(m_resolution);
    if (newFftSize != m_fftSize) {
        m_fftSize = newFftSize;
        m_hopSize = m_fftSize / 4; // 75% overlap
        
        // Regenerate windows
        for (auto& channel : m_channels) {
            generateHannWindow(channel.window, m_fftSize);
        }
    }
    
    // Detect freeze trigger edge
    bool freezeTriggered = (m_freezeTrigger > 0.5f && m_prevFreezeTrigger <= 0.5f);
    bool freezeReleased = (m_freezeTrigger <= 0.5f && m_prevFreezeTrigger > 0.5f);
    m_prevFreezeTrigger = m_freezeTrigger;
    
    // Process each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        auto& state = m_channels[ch % m_channels.size()];
        
        // Handle freeze state changes
        if (freezeTriggered) {
            state.isFrozen = true;
            state.freezeFadeIn = 0.0f;
        } else if (freezeReleased) {
            state.isFrozen = false;
        }
        
        // Process samples
        for (int i = 0; i < numSamples; ++i) {
            // Fill input buffer
            state.inputBuffer[state.inputPos] = channelData[i];
            state.inputPos++;
            
            // Process frame when we have enough samples
            if (state.inputPos >= m_hopSize) {
                processSpectralFrame(state);
                state.inputPos = 0;
            }
            
            // Read from output buffer
            if (state.outputPos < state.outputBuffer.size()) {
                float output = state.outputBuffer[state.outputPos];
                
                // Crossfade between dry and frozen signal
                if (state.isFrozen) {
                    state.freezeFadeIn = std::min(1.0f, state.freezeFadeIn + 0.001f);
                    channelData[i] = channelData[i] * (1.0f - state.freezeFadeIn) + 
                                    output * state.freezeFadeIn;
                } else {
                    channelData[i] = output;
                }
                
                state.outputPos++;
            } else {
                // Shouldn't happen, but safety fallback
                channelData[i] = 0.0f;
            }
        }
    }
}

void SpectralFreeze::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    m_freezeTrigger = getParam(0, 0.0f);
    m_resolution = getParam(1, 0.5f);
    m_crystalline = getParam(2, 0.5f);
    m_morph = getParam(3, 0.0f);
}

juce::String SpectralFreeze::getParameterName(int index) const {
    switch (index) {
        case 0: return "Freeze";
        case 1: return "Resolution";
        case 2: return "Crystalline";
        case 3: return "Morph";
        default: return "";
    }
}

void SpectralFreeze::ChannelState::init(int maxFftSize) {
    inputBuffer.resize(maxFftSize);
    outputBuffer.resize(maxFftSize);
    fftData.resize(maxFftSize);
    frozenSpectrum.resize(maxFftSize);
    window.resize(maxFftSize);
    overlapBuffer.resize(maxFftSize);
}

void SpectralFreeze::ChannelState::reset() {
    std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
    std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
    std::fill(overlapBuffer.begin(), overlapBuffer.end(), 0.0f);
    inputPos = 0;
    outputPos = 0;
    isFrozen = false;
    freezeFadeIn = 0.0f;
}

void SpectralFreeze::processSpectralFrame(ChannelState& state) {
    // Copy input to FFT buffer with windowing
    for (int i = 0; i < m_fftSize; ++i) {
        int idx = (state.inputPos + i) % m_fftSize;
        state.fftData[i] = std::complex<float>(
            state.inputBuffer[idx] * state.window[i], 0.0f
        );
    }
    
    // Forward FFT
    performFFT(state.fftData, false);
    
    // If frozen, use frozen spectrum
    if (state.isFrozen) {
        // Capture spectrum on first frozen frame
        if (state.freezeFadeIn == 0.0f) {
            // Copy magnitude, randomize phase
            for (int i = 0; i < m_fftSize; ++i) {
                float mag = std::abs(state.fftData[i]);
                float phase = m_phaseDist(m_rng);
                state.frozenSpectrum[i] = std::polar(mag, phase);
            }
        }
        
        // Use frozen spectrum with crystalline filtering
        state.fftData = state.frozenSpectrum;
        applyCrystallineFilter(state.fftData);
        
        // Apply morphing if needed
        if (m_morph > 0.0f) {
            std::vector<std::complex<float>> morphed(m_fftSize);
            morphSpectra(morphed, state.fftData, state.frozenSpectrum, m_morph);
            state.fftData = morphed;
        }
    }
    
    // Inverse FFT
    performFFT(state.fftData, true);
    
    // Overlap-add
    state.outputPos = 0;
    for (int i = 0; i < m_fftSize; ++i) {
        float sample = state.fftData[i].real() * state.window[i];
        
        if (i < m_hopSize) {
            state.outputBuffer[i] = state.overlapBuffer[i] + sample;
            state.overlapBuffer[i] = 0.0f;
        } else {
            state.overlapBuffer[i - m_hopSize] += sample;
        }
    }
}

void SpectralFreeze::performFFT(std::vector<std::complex<float>>& data, bool inverse) {
    // Simple DFT implementation (replace with optimized FFT in production)
    int N = m_fftSize;
    std::vector<std::complex<float>> temp(N);
    
    float sign = inverse ? 1.0f : -1.0f;
    float scale = inverse ? 1.0f / N : 1.0f;
    
    for (int k = 0; k < N; ++k) {
        std::complex<float> sum(0.0f, 0.0f);
        for (int n = 0; n < N; ++n) {
            float angle = sign * 2.0f * M_PI * k * n / N;
            sum += data[n] * std::complex<float>(cos(angle), sin(angle));
        }
        temp[k] = sum * scale;
    }
    
    data = temp;
}

void SpectralFreeze::generateHannWindow(std::vector<float>& window, int size) {
    for (int i = 0; i < size; ++i) {
        window[i] = 0.5f * (1.0f - cos(2.0f * M_PI * i / (size - 1)));
    }
}

void SpectralFreeze::applyCrystallineFilter(std::vector<std::complex<float>>& spectrum) {
    // Spectral thinning/emphasis based on crystalline parameter
    float threshold = 1.0f - m_crystalline;
    
    // Find peak magnitude
    float maxMag = 0.0f;
    for (const auto& bin : spectrum) {
        maxMag = std::max(maxMag, std::abs(bin));
    }
    
    // Apply spectral filtering
    for (auto& bin : spectrum) {
        float mag = std::abs(bin);
        float phase = std::arg(bin);
        
        // Emphasize peaks, suppress valleys
        if (mag < maxMag * threshold) {
            mag *= 0.1f + 0.9f * (mag / (maxMag * threshold));
        } else {
            mag *= 1.0f + m_crystalline * 2.0f;
        }
        
        bin = std::polar(mag, phase);
    }
}

void SpectralFreeze::morphSpectra(std::vector<std::complex<float>>& result,
                                 const std::vector<std::complex<float>>& a,
                                 const std::vector<std::complex<float>>& b,
                                 float amount) {
    for (size_t i = 0; i < result.size(); ++i) {
        // Magnitude interpolation
        float magA = std::abs(a[i]);
        float magB = std::abs(b[i]);
        float mag = magA * (1.0f - amount) + magB * amount;
        
        // Phase interpolation (circular)
        float phaseA = std::arg(a[i]);
        float phaseB = std::arg(b[i]);
        float phaseDiff = phaseB - phaseA;
        
        // Wrap phase difference to [-pi, pi]
        while (phaseDiff > M_PI) phaseDiff -= 2.0f * M_PI;
        while (phaseDiff < -M_PI) phaseDiff += 2.0f * M_PI;
        
        float phase = phaseA + phaseDiff * amount;
        
        result[i] = std::polar(mag, phase);
    }
}

int SpectralFreeze::getFFTSizeFromParameter(float param) {
    // Map 0-1 to FFT sizes: 512, 1024, 2048, 4096, 8192
    if (param < 0.2f) return 512;
    else if (param < 0.4f) return 1024;
    else if (param < 0.6f) return 2048;
    else if (param < 0.8f) return 4096;
    else return 8192;
}