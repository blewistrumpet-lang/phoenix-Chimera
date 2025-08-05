#include "PitchShifter.h"
#include <cmath>

PitchShifter::PitchShifter() {
    // Initialize smooth parameters
    m_pitchRatio.setImmediate(1.0f);
    m_formantShift.setImmediate(1.0f);
    m_mixAmount.setImmediate(1.0f);
    m_windowWidth.setImmediate(0.5f);
    m_spectralGate.setImmediate(0.0f);
    m_grainSize.setImmediate(0.5f);
    m_feedback.setImmediate(0.0f);
    m_stereoWidth.setImmediate(0.5f);
    
    // Set smoothing rates (faster for more responsive parameters)
    m_pitchRatio.setSmoothingRate(0.990f);
    m_formantShift.setSmoothingRate(0.992f);
    m_mixAmount.setSmoothingRate(0.995f);
    m_windowWidth.setSmoothingRate(0.998f);
    m_spectralGate.setSmoothingRate(0.995f);
    m_grainSize.setSmoothingRate(0.998f);
    m_feedback.setSmoothingRate(0.995f);
    m_stereoWidth.setSmoothingRate(0.995f);
}

void PitchShifter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_channelStates.clear();
    
    int numChannels = 2;
    m_channelStates.resize(numChannels);
    
    // Initialize DC blockers
    m_inputDCBlockers.resize(numChannels);
    m_outputDCBlockers.resize(numChannels);
    
    // Prepare oversampler
    if (m_useOversampling) {
        m_oversampler.prepare(samplesPerBlock);
    }
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    
    for (auto& state : m_channelStates) {
        state.inputBuffer.resize(FFT_SIZE * 2);
        state.outputBuffer.resize(FFT_SIZE * 2);
        state.windowBuffer.resize(FFT_SIZE);
        state.analysisWindow.resize(FFT_SIZE);
        state.synthesisWindow.resize(FFT_SIZE);
        state.spectrum.resize(FFT_SIZE);
        state.phaseLast.resize(FFT_SIZE / 2 + 1);
        state.phaseSum.resize(FFT_SIZE / 2 + 1);
        state.magnitude.resize(FFT_SIZE / 2 + 1);
        state.frequency.resize(FFT_SIZE / 2 + 1);
        state.feedbackBuffer.resize(samplesPerBlock * 4);
        
        std::fill(state.inputBuffer.begin(), state.inputBuffer.end(), 0.0f);
        std::fill(state.outputBuffer.begin(), state.outputBuffer.end(), 0.0f);
        std::fill(state.phaseLast.begin(), state.phaseLast.end(), 0.0f);
        std::fill(state.phaseSum.begin(), state.phaseSum.end(), 0.0f);
        std::fill(state.feedbackBuffer.begin(), state.feedbackBuffer.end(), 0.0f);
        
        createWindows(state.analysisWindow, state.synthesisWindow);
    }
    
    // Reset thermal model
    m_thermalModel = ThermalModel();
}

void PitchShifter::reset() {
    // Reset all internal state
    for (auto& state : m_channelStates) {
        std::fill(state.inputBuffer.begin(), state.inputBuffer.end(), 0.0f);
        std::fill(state.outputBuffer.begin(), state.outputBuffer.end(), 0.0f);
        std::fill(state.phaseLast.begin(), state.phaseLast.end(), 0.0f);
        std::fill(state.phaseSum.begin(), state.phaseSum.end(), 0.0f);
        state.inputPos = 0;
        state.outputPos = 0;
    }
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_pitchRatio.update();
    m_formantShift.update();
    m_mixAmount.update();
    m_windowWidth.update();
    m_spectralGate.update();
    m_grainSize.update();
    m_feedback.update();
    m_stereoWidth.update();
    
    // Update thermal model
    m_thermalModel.update(m_sampleRate);
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update component aging (very slow)
    m_sampleCount += numSamples;
    if (m_sampleCount > m_sampleRate * 10) { // Every 10 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.0001f);
        m_sampleCount = 0;
    }
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= m_channelStates.size()) continue;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            
            // DC block input
            input = m_inputDCBlockers[channel].process(input);
            
            // Add feedback with thermal and aging modulation
            if (m_feedback.current > 0.0f) {
                float feedbackAmount = m_feedback.current * 0.7f * thermalFactor * (1.0f - m_componentAge * 0.1f);
                input += state.feedbackBuffer[state.feedbackPos] * feedbackAmount;
                state.feedbackPos = (state.feedbackPos + 1) % state.feedbackBuffer.size();
            }
            
            // Fill input buffer
            state.inputBuffer[state.inputPos] = input;
            state.inputPos++;
            
            // Process when we have enough samples
            if (state.inputPos >= HOP_SIZE) {
                processSpectralFrame(state);
                state.inputPos = 0;
                
                // Shift buffers
                std::copy(state.inputBuffer.begin() + HOP_SIZE, 
                         state.inputBuffer.end(), 
                         state.inputBuffer.begin());
                std::fill(state.inputBuffer.end() - HOP_SIZE, 
                         state.inputBuffer.end(), 0.0f);
            }
            
            // Get output
            float output = 0.0f;
            if (state.outputPos < state.outputBuffer.size()) {
                output = state.outputBuffer[state.outputPos];
                state.outputPos++;
                
                if (state.outputPos >= HOP_SIZE) {
                    // Shift output buffer
                    std::copy(state.outputBuffer.begin() + HOP_SIZE,
                             state.outputBuffer.end(),
                             state.outputBuffer.begin());
                    std::fill(state.outputBuffer.end() - HOP_SIZE,
                             state.outputBuffer.end(), 0.0f);
                    state.outputPos -= HOP_SIZE;
                }
            }
            
            // Store in feedback buffer
            if (m_feedback.current > 0.0f && state.feedbackPos < state.feedbackBuffer.size()) {
                state.feedbackBuffer[state.feedbackPos] = output;
            }
            
            // DC block output
            output = m_outputDCBlockers[channel].process(output);
            
            // Analog-style saturation with aging
            float saturation = 1.0f + m_componentAge * 0.05f;
            if (std::abs(output) > 0.8f) {
                output = std::tanh(output * saturation) / saturation;
            }
            
            // Mix with dry signal
            channelData[sample] = input * (1.0f - m_mixAmount.current) + output * m_mixAmount.current;
        }
    }
    
    // Apply stereo width with thermal modulation
    if (numChannels == 2 && m_stereoWidth.current != 0.5f) {
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        
        float width = m_stereoWidth.current * 2.0f * thermalFactor;
        float mid, side;
        
        for (int i = 0; i < numSamples; ++i) {
            mid = (left[i] + right[i]) * 0.5f;
            side = (left[i] - right[i]) * 0.5f * width;
            left[i] = mid + side;
            right[i] = mid - side;
        }
    }
}

void PitchShifter::processSpectralFrame(ChannelState& state) {
    // Window the input
    for (int i = 0; i < FFT_SIZE; ++i) {
        state.windowBuffer[i] = state.inputBuffer[i] * state.analysisWindow[i];
    }
    
    // Copy to complex array for FFT
    for (int i = 0; i < FFT_SIZE; ++i) {
        state.spectrum[i] = std::complex<float>(state.windowBuffer[i], 0.0f);
    }
    
    // Forward FFT
    state.fft.perform(state.spectrum.data(), state.spectrum.data(), false);
    
    // Analyze spectrum
    const float binFreq = static_cast<float>(m_sampleRate) / FFT_SIZE;
    const float expectedPhaseInc = 2.0f * M_PI * HOP_SIZE / FFT_SIZE;
    
    for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
        float real = state.spectrum[bin].real();
        float imag = state.spectrum[bin].imag();
        
        state.magnitude[bin] = std::sqrt(real * real + imag * imag);
        float phase = std::atan2(imag, real);
        
        // Phase vocoder analysis
        float phaseDiff = phase - state.phaseLast[bin];
        state.phaseLast[bin] = phase;
        
        // Wrap phase difference to [-pi, pi]
        while (phaseDiff > M_PI) phaseDiff -= 2.0f * M_PI;
        while (phaseDiff < -M_PI) phaseDiff += 2.0f * M_PI;
        
        // Calculate true frequency
        float deviation = phaseDiff - expectedPhaseInc * bin;
        float frequency = binFreq * bin + deviation * m_sampleRate / (2.0f * M_PI * HOP_SIZE);
        state.frequency[bin] = frequency;
    }
    
    // Apply spectral gate with thermal and aging effects
    if (m_spectralGate.current > 0.0f) {
        float thermalFactor = m_thermalModel.getThermalFactor();
        float threshold = m_spectralGate.current * m_spectralGate.current * 0.1f * thermalFactor;
        float agingSmooth = 1.0f - m_componentAge * 0.2f; // Aging reduces gate precision
        
        for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
            if (state.magnitude[bin] < threshold) {
                state.magnitude[bin] *= agingSmooth;
            }
        }
    }
    
    // Shift spectrum with thermal and aging modulation
    float thermalFactor = m_thermalModel.getThermalFactor();
    float pitchRatio = m_pitchRatio.current * thermalFactor * (1.0f - m_componentAge * 0.02f);
    float formantShift = m_formantShift.current * (1.0f + m_componentAge * 0.01f);
    
    shiftSpectrum(state.spectrum, state.magnitude, state.frequency, 
                  pitchRatio, formantShift);
    
    // Inverse FFT
    state.fft.perform(state.spectrum.data(), state.spectrum.data(), true);
    
    // Window and overlap-add
    const float scaleFactor = 1.0f / (FFT_SIZE * OVERLAP_FACTOR * 0.5f);
    for (int i = 0; i < FFT_SIZE; ++i) {
        state.outputBuffer[i] += state.spectrum[i].real() * state.synthesisWindow[i] * scaleFactor;
    }
}

void PitchShifter::shiftSpectrum(std::vector<std::complex<float>>& spectrum,
                                const std::vector<float>& magnitude,
                                const std::vector<float>& frequency,
                                float pitchRatio, float formantRatio) {
    std::vector<std::complex<float>> shiftedSpectrum(FFT_SIZE, std::complex<float>(0.0f, 0.0f));
    std::vector<float> shiftedMagnitude(FFT_SIZE / 2 + 1, 0.0f);
    
    // Shift magnitudes (formant shift)
    for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
        int targetBin = static_cast<int>(bin * formantRatio + 0.5f);
        if (targetBin > 0 && targetBin <= FFT_SIZE / 2) {
            shiftedMagnitude[targetBin] += magnitude[bin];
        }
    }
    
    // Reconstruct spectrum with shifted frequencies (pitch shift)
    // const float binFreq = static_cast<float>(m_sampleRate) / FFT_SIZE;
    for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
        if (shiftedMagnitude[bin] > 0.0f) {
            float shiftedFreq = frequency[bin] * pitchRatio;
            m_channelStates[0].phaseSum[bin] += 2.0f * M_PI * shiftedFreq * HOP_SIZE / m_sampleRate;
            
            float phase = m_channelStates[0].phaseSum[bin];
            shiftedSpectrum[bin] = std::polar(shiftedMagnitude[bin], phase);
            
            // Mirror for negative frequencies
            if (bin > 0 && bin < FFT_SIZE / 2) {
                shiftedSpectrum[FFT_SIZE - bin] = std::conj(shiftedSpectrum[bin]);
            }
        }
    }
    
    spectrum = shiftedSpectrum;
}

float PitchShifter::getWindow(int pos, int size) {
    // Hann window with adjustable width and thermal modulation
    float thermalFactor = m_thermalModel.getThermalFactor();
    float width = (0.5f + m_windowWidth.current * 0.45f) * thermalFactor;
    float t = static_cast<float>(pos) / (size - 1);
    return width - width * std::cos(2.0f * M_PI * t);
}

void PitchShifter::createWindows(std::vector<float>& analysis, std::vector<float>& synthesis) {
    for (int i = 0; i < FFT_SIZE; ++i) {
        analysis[i] = getWindow(i, FFT_SIZE);
        synthesis[i] = analysis[i];
    }
    
    // Normalize synthesis window for COLA
    std::vector<float> sum(FFT_SIZE, 0.0f);
    for (int i = 0; i < OVERLAP_FACTOR; ++i) {
        int offset = i * HOP_SIZE;
        for (int j = 0; j < FFT_SIZE; ++j) {
            int idx = (j + offset) % FFT_SIZE;
            sum[idx] += synthesis[j] * synthesis[j];
        }
    }
    
    for (int i = 0; i < FFT_SIZE; ++i) {
        if (sum[i] > 0.0f) {
            synthesis[i] /= std::sqrt(sum[i]);
        }
    }
}

void PitchShifter::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_pitchRatio.target = 0.25f + params.at(0) * 3.75f; // 0.25x to 4x
    if (params.count(1)) m_formantShift.target = 0.5f + params.at(1) * 1.5f; // 0.5x to 2x
    if (params.count(2)) m_mixAmount.target = params.at(2);
    if (params.count(3)) m_windowWidth.target = params.at(3);
    if (params.count(4)) m_spectralGate.target = params.at(4);
    if (params.count(5)) m_grainSize.target = params.at(5);
    if (params.count(6)) m_feedback.target = params.at(6) * 0.9f;
    if (params.count(7)) m_stereoWidth.target = params.at(7);
}

juce::String PitchShifter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Pitch";
        case 1: return "Formant";
        case 2: return "Mix";
        case 3: return "Window";
        case 4: return "Gate";
        case 5: return "Grain";
        case 6: return "Feedback";
        case 7: return "Width";
        default: return "";
    }
}