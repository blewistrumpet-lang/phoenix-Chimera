#include "PhasedVocoder.h"
#include <cmath>
#include <algorithm>

PhasedVocoder::PhasedVocoder() = default;

void PhasedVocoder::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_channelStates.clear();
    
    int numChannels = 2;
    m_channelStates.resize(numChannels);
    
    const int bufferSize = FFT_SIZE * MAX_STRETCH * 2;
    
    for (auto& state : m_channelStates) {
        state.inputBuffer.resize(bufferSize);
        state.outputBuffer.resize(bufferSize);
        state.grainBuffer.resize(FFT_SIZE);
        state.fftBuffer.resize(FFT_SIZE);
        state.window.resize(FFT_SIZE);
        state.magnitude.resize(FFT_SIZE / 2 + 1);
        state.phase.resize(FFT_SIZE / 2 + 1);
        state.lastPhase.resize(FFT_SIZE / 2 + 1);
        state.phaseAccum.resize(FFT_SIZE / 2 + 1);
        state.trueBinFreq.resize(FFT_SIZE / 2 + 1);
        state.freezeMagnitude.resize(FFT_SIZE / 2 + 1);
        state.freezePhase.resize(FFT_SIZE / 2 + 1);
        
        std::fill(state.inputBuffer.begin(), state.inputBuffer.end(), 0.0f);
        std::fill(state.outputBuffer.begin(), state.outputBuffer.end(), 0.0f);
        std::fill(state.lastPhase.begin(), state.lastPhase.end(), 0.0f);
        std::fill(state.phaseAccum.begin(), state.phaseAccum.end(), 0.0f);
        
        createWindow(state.window);
        
        state.readPos = 0.0f;
        state.writePos = 0;
        state.outputReadPos = 0;
        state.hopCounter = 0;
        state.isFrozen = false;
    }
}

void PhasedVocoder::reset() {
    // Reset all internal state
    for (auto& state : m_channelStates) {
        // Clear all buffers
        std::fill(state.inputBuffer.begin(), state.inputBuffer.end(), 0.0f);
        std::fill(state.outputBuffer.begin(), state.outputBuffer.end(), 0.0f);
        std::fill(state.grainBuffer.begin(), state.grainBuffer.end(), 0.0f);
        std::fill(state.fftBuffer.begin(), state.fftBuffer.end(), std::complex<float>(0.0f, 0.0f));
        
        // Clear magnitude and phase arrays
        std::fill(state.magnitude.begin(), state.magnitude.end(), 0.0f);
        std::fill(state.phase.begin(), state.phase.end(), 0.0f);
        std::fill(state.lastPhase.begin(), state.lastPhase.end(), 0.0f);
        std::fill(state.phaseAccum.begin(), state.phaseAccum.end(), 0.0f);
        std::fill(state.trueBinFreq.begin(), state.trueBinFreq.end(), 0.0f);
        
        // Clear freeze buffers
        std::fill(state.freezeMagnitude.begin(), state.freezeMagnitude.end(), 0.0f);
        std::fill(state.freezePhase.begin(), state.freezePhase.end(), 0.0f);
        state.isFrozen = false;
        
        // Reset position tracking
        state.readPos = 0.0f;
        state.writePos = 0;
        state.outputReadPos = 0;
        state.hopCounter = 0;
        
        // Reset transient detection state
        state.envelopeFollower = 0.0f;
        state.lastMagnitudeSum = 0.0f;
    }
}

void PhasedVocoder::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= m_channelStates.size()) continue;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Handle freeze
        if (m_freeze > 0.5f && !state.isFrozen) {
            state.isFrozen = true;
            state.freezeMagnitude = state.magnitude;
            state.freezePhase = state.phase;
        } else if (m_freeze <= 0.5f) {
            state.isFrozen = false;
        }
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Write input to circular buffer
            state.inputBuffer[state.writePos] = channelData[sample];
            state.writePos = (state.writePos + 1) % state.inputBuffer.size();
            
            // Process frames at hop boundaries
            state.hopCounter++;
            if (state.hopCounter >= HOP_SIZE) {
                state.hopCounter = 0;
                processFrame(state);
            }
            
            // Read output
            float output = 0.0f;
            if (state.outputReadPos < state.outputBuffer.size()) {
                output = state.outputBuffer[state.outputReadPos];
                state.outputBuffer[state.outputReadPos] = 0.0f; // Clear after reading
                state.outputReadPos = (state.outputReadPos + 1) % state.outputBuffer.size();
            }
            
            // Mix with dry signal
            channelData[sample] = channelData[sample] * (1.0f - m_mixAmount) + output * m_mixAmount;
        }
    }
}

void PhasedVocoder::processFrame(ChannelState& state) {
    // Fill grain buffer from input at current read position
    for (int i = 0; i < FFT_SIZE; ++i) {
        int readIdx = static_cast<int>(state.readPos + i) % state.inputBuffer.size();
        state.grainBuffer[i] = state.inputBuffer[readIdx] * state.window[i];
    }
    
    // Advance read position based on time stretch
    float hopAdvance = HOP_SIZE / m_timeStretch;
    
    // Preserve transients by temporarily reducing stretch
    float transientAmount = detectTransient(state);
    if (transientAmount > 0.0f) {
        float transientMod = 1.0f - (transientAmount * m_transientPreserve * 0.9f);
        hopAdvance = HOP_SIZE / (m_timeStretch * transientMod);
    }
    
    state.readPos += hopAdvance;
    while (state.readPos >= state.inputBuffer.size()) {
        state.readPos -= state.inputBuffer.size();
    }
    
    // Analyze and synthesize
    analyzeFrame(state);
    applySpectralProcessing(state);
    synthesizeFrame(state);
}

void PhasedVocoder::analyzeFrame(ChannelState& state) {
    // Copy windowed grain to FFT buffer
    for (int i = 0; i < FFT_SIZE; ++i) {
        state.fftBuffer[i] = std::complex<float>(state.grainBuffer[i], 0.0f);
    }
    
    // Forward FFT
    state.fft.perform(state.fftBuffer.data(), state.fftBuffer.data(), false);
    
    // Extract magnitude and phase
    const float binFreq = static_cast<float>(m_sampleRate) / FFT_SIZE;
    const float expectedPhaseInc = 2.0f * M_PI * HOP_SIZE / FFT_SIZE;
    
    for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
        float real = state.fftBuffer[bin].real();
        float imag = state.fftBuffer[bin].imag();
        
        state.magnitude[bin] = std::sqrt(real * real + imag * imag);
        state.phase[bin] = std::atan2(imag, real);
        
        // Phase vocoder frequency analysis
        float phaseDiff = state.phase[bin] - state.lastPhase[bin];
        state.lastPhase[bin] = state.phase[bin];
        
        // Wrap phase difference
        while (phaseDiff > M_PI) phaseDiff -= 2.0f * M_PI;
        while (phaseDiff < -M_PI) phaseDiff += 2.0f * M_PI;
        
        // Calculate true frequency
        float deviation = phaseDiff - expectedPhaseInc * bin;
        state.trueBinFreq[bin] = binFreq * bin + deviation * m_sampleRate / (2.0f * M_PI * HOP_SIZE);
    }
}

void PhasedVocoder::applySpectralProcessing(ChannelState& state) {
    // Apply spectral gate
    if (m_spectralGate > 0.0f) {
        float threshold = m_spectralGate * m_spectralGate * 0.01f;
        for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
            if (state.magnitude[bin] < threshold) {
                state.magnitude[bin] = 0.0f;
            }
        }
    }
    
    // Apply spectral smear
    if (m_spectralSmear > 0.0f) {
        std::vector<float> smearedMag = state.magnitude;
        int smearWidth = static_cast<int>(m_spectralSmear * 10.0f + 1.0f);
        
        for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
            float sum = 0.0f;
            int count = 0;
            
            for (int offset = -smearWidth; offset <= smearWidth; ++offset) {
                int idx = bin + offset;
                if (idx >= 0 && idx <= FFT_SIZE / 2) {
                    sum += state.magnitude[idx];
                    count++;
                }
            }
            
            if (count > 0) {
                smearedMag[bin] = sum / count;
            }
        }
        
        state.magnitude = smearedMag;
    }
    
    // Use frozen spectrum if enabled
    if (state.isFrozen) {
        state.magnitude = state.freezeMagnitude;
        
        // Apply phase reset
        if (m_phaseReset > 0.0f) {
            for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
                float resetAmount = m_phaseReset;
                state.phase[bin] = state.freezePhase[bin] * (1.0f - resetAmount) + 
                                  state.phase[bin] * resetAmount;
            }
        }
    }
}

void PhasedVocoder::synthesizeFrame(ChannelState& state) {
    // const float binFreq = static_cast<float>(m_sampleRate) / FFT_SIZE;
    
    // Reconstruct spectrum with pitch shift
    for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
        // Apply pitch shift to frequency
        float shiftedFreq = state.trueBinFreq[bin] * m_pitchShift;
        
        // Accumulate phase
        state.phaseAccum[bin] += 2.0f * M_PI * shiftedFreq * HOP_SIZE / m_sampleRate;
        
        // Reconstruct complex spectrum
        state.fftBuffer[bin] = std::polar(state.magnitude[bin], state.phaseAccum[bin]);
        
        // Mirror for negative frequencies
        if (bin > 0 && bin < FFT_SIZE / 2) {
            state.fftBuffer[FFT_SIZE - bin] = std::conj(state.fftBuffer[bin]);
        }
    }
    
    // Inverse FFT
    state.fft.perform(state.fftBuffer.data(), state.fftBuffer.data(), true);
    
    // Window and overlap-add to output
    const float scale = 1.0f / (FFT_SIZE * OVERLAP / 2.0f);
    for (int i = 0; i < FFT_SIZE; ++i) {
        int outIdx = (state.outputReadPos + i) % state.outputBuffer.size();
        state.outputBuffer[outIdx] += state.fftBuffer[i].real() * state.window[i] * scale;
    }
}

float PhasedVocoder::detectTransient(ChannelState& state) {
    // Simple transient detection based on spectral flux
    float magnitudeSum = 0.0f;
    for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
        magnitudeSum += state.magnitude[bin];
    }
    
    float flux = std::max(0.0f, magnitudeSum - state.lastMagnitudeSum);
    state.lastMagnitudeSum = magnitudeSum;
    
    // Envelope follower
    const float attack = 0.001f;
    const float release = 0.1f;
    
    if (flux > state.envelopeFollower) {
        state.envelopeFollower += (flux - state.envelopeFollower) * attack;
    } else {
        state.envelopeFollower += (flux - state.envelopeFollower) * release;
    }
    
    return std::min(1.0f, state.envelopeFollower * 10.0f);
}

void PhasedVocoder::createWindow(std::vector<float>& window) {
    // Hann window
    for (int i = 0; i < FFT_SIZE; ++i) {
        window[i] = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / (FFT_SIZE - 1));
    }
}

void PhasedVocoder::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_timeStretch = 0.25f + params.at(0) * 3.75f; // 0.25x to 4x
    if (params.count(1)) m_pitchShift = 0.5f + params.at(1) * 1.5f; // 0.5x to 2x
    if (params.count(2)) m_spectralSmear = params.at(2);
    if (params.count(3)) m_transientPreserve = params.at(3);
    if (params.count(4)) m_phaseReset = params.at(4);
    if (params.count(5)) m_spectralGate = params.at(5);
    if (params.count(6)) m_mixAmount = params.at(6);
    if (params.count(7)) m_freeze = params.at(7);
}

juce::String PhasedVocoder::getParameterName(int index) const {
    switch (index) {
        case 0: return "Stretch";
        case 1: return "Pitch";
        case 2: return "Smear";
        case 3: return "Transient";
        case 4: return "Phase";
        case 5: return "Gate";
        case 6: return "Mix";
        case 7: return "Freeze";
        default: return "";
    }
}