#include "IntelligentHarmonizer.h"
#include <cmath>
#include <algorithm>

// Initialize scale intervals
constexpr int IntelligentHarmonizer::SCALE_INTERVALS[NUM_SCALES][12];

IntelligentHarmonizer::IntelligentHarmonizer() {
    // Initialize smoothed parameters with proper defaults
    m_interval.reset(0.5f);
    m_key.reset(0.0f);
    m_scale.reset(0.0f);
    m_voiceCount.reset(0.0f);
    m_spread.reset(0.3f);
    m_humanize.reset(0.3f);
    m_formant.reset(0.5f);
    m_mix.reset(0.5f);
}

void IntelligentHarmonizer::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    float fastSmoothingTime = 100.0f; // 100ms for harmonic parameters
    float slowSmoothingTime = 200.0f; // 200ms for key/scale changes
    
    m_interval.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_key.setSmoothingTime(slowSmoothingTime, sampleRate);
    m_scale.setSmoothingTime(slowSmoothingTime, sampleRate);
    m_voiceCount.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_spread.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_humanize.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_formant.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_mix.setSmoothingTime(fastSmoothingTime, sampleRate);
    
    for (auto& channel : m_channelStates) {
        // Initialize voices with boutique preparation
        for (auto& voice : channel.voices) {
            voice.prepare(sampleRate);
        }
    }
}

void IntelligentHarmonizer::reset() {
    // Reset all internal state
    // TODO: Implement specific reset logic for IntelligentHarmonizer
}

void IntelligentHarmonizer::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update all smoothed parameters
    m_interval.update();
    m_key.update();
    m_scale.update();
    m_voiceCount.update();
    m_spread.update();
    m_humanize.update();
    m_formant.update();
    m_mix.update();
    
    // Calculate interval in semitones using smoothed parameter
    int intervalSemitones = static_cast<int>((m_interval.current - 0.5f) * 48.0f); // -24 to +24
    
    // Get active voice count using smoothed parameter
    int activeVoices = getActiveVoices();
    
    // Get scale type using smoothed parameter
    ScaleType scaleType = static_cast<ScaleType>(
        std::min(static_cast<int>(m_scale.current * NUM_SCALES), NUM_SCALES - 1)
    );
    
    // Get root key using smoothed parameter
    int rootKey = static_cast<int>(m_key.current * 12.0f) % 12; // 0-11 (C-B)
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Process pitch detection on a decimated signal
        static int detectCounter = 0;
        if (++detectCounter >= 64) { // Detect every 64 samples
            detectCounter = 0;
            
            // Add samples to pitch detector
            for (int i = 0; i < std::min(64, numSamples); i += 4) {
                state.pitchDetector.addSample(channelData[i]);
            }
            
            // Detect pitch
            float detectedFreq = state.pitchDetector.detectPitch(m_sampleRate);
            float detectedNote = frequencyToNote(detectedFreq);
        }
        
        // Temporary buffer for voice mixing
        std::vector<float> voiceBuffer(numSamples, 0.0f);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float drySignal = input;
            
            // Apply input DC blocking and thermal/aging compensation
            input = state.inputDCBlocker.process(input);
            
            // Update thermal and aging models
            float processingLoad = std::min(1.0f, numSamples / 512.0f);
            state.thermalModel.update(processingLoad);
            state.componentAging.update();
            
            float thermalDrift = state.thermalModel.getTemperatureDrift();
            float agingFactor = state.componentAging.getAgingFactor();
            
            // Apply thermal compensation to input
            input *= (1.0f + thermalDrift) * agingFactor;
            
            // ZDF high-pass filter to remove DC and very low frequencies
            float highpassed = state.dcBlockingFilter.processHighpass(input);
            
            // Process each active voice
            float harmonizedOutput = 0.0f;
            
            for (int voice = 0; voice < activeVoices; ++voice) {
                auto& harmonizer = state.voices[voice];
                
                // Calculate harmony interval for this voice
                int voiceInterval = intervalSemitones;
                
                // For multiple voices, create chord intervals
                if (activeVoices > 1) {
                    switch (voice) {
                        case 0: break; // Use base interval
                        case 1: voiceInterval += (scaleType == MAJOR) ? 4 : 3; break; // 3rd
                        case 2: voiceInterval += 7; break; // 5th
                        case 3: voiceInterval += (scaleType == MAJOR) ? 11 : 10; break; // 7th
                    }
                }
                
                // Quantize to scale if not chromatic
                if (scaleType != CHROMATIC) {
                    voiceInterval = quantizeToScale(voiceInterval, scaleType, rootKey);
                }
                
                // Calculate pitch ratio
                float pitchRatio = std::pow(2.0f, voiceInterval / 12.0f);
                
                // Add enhanced humanization using smoothed parameter
                if (m_humanize.current > 0.0f) {
                    // Multiple LFOs for natural variation
                    harmonizer.pitchLFO += 0.1f / m_sampleRate;
                    harmonizer.vibratoPhase += 2.0f * M_PI * 4.5f / m_sampleRate; // 4.5Hz vibrato
                    harmonizer.chorusPhase += 2.0f * M_PI * 0.3f / m_sampleRate; // 0.3Hz chorus
                    
                    if (harmonizer.pitchLFO > 1.0f) harmonizer.pitchLFO -= 1.0f;
                    if (harmonizer.vibratoPhase > 2.0f * M_PI) harmonizer.vibratoPhase -= 2.0f * M_PI;
                    if (harmonizer.chorusPhase > 2.0f * M_PI) harmonizer.chorusPhase -= 2.0f * M_PI;
                    
                    // Complex pitch modulation with multiple sources
                    float vibrato = std::sin(harmonizer.vibratoPhase) * m_humanize.current * 0.03f; // ±3 cents
                    float chorus = std::sin(harmonizer.chorusPhase) * m_humanize.current * 0.02f; // ±2 cents
                    float drift = std::sin(harmonizer.pitchLFO * 2.0f * M_PI * 0.1f) * m_humanize.current * 0.01f; // ±1 cent drift
                    
                    float totalPitchMod = vibrato + chorus + drift;
                    pitchRatio *= std::pow(2.0f, totalPitchMod / 12.0f);
                }
                
                // Process voice with enhanced parameters
                float voiceOut = harmonizer.process(highpassed, pitchRatio, m_formant.current, 
                                                  m_humanize.current, m_sampleRate);
                
                // Apply stereo spread using smoothed parameter
                if (numChannels == 2 && activeVoices > 1) {
                    float pan = (voice - (activeVoices - 1) * 0.5f) / 
                               std::max(1.0f, activeVoices - 1.0f);
                    pan *= m_spread.current;
                    
                    // Simple stereo panning
                    if (channel == 0) {
                        voiceOut *= (1.0f - pan * 0.5f); // Left
                    } else {
                        voiceOut *= (1.0f + pan * 0.5f); // Right
                    }
                }
                
                harmonizedOutput += voiceOut / std::sqrt(static_cast<float>(activeVoices));
            }
            
            // Apply boutique anti-aliasing and DC blocking
            harmonizedOutput = state.antiAliasingFilter.processLowpass(harmonizedOutput);
            harmonizedOutput = state.outputDCBlocker.process(harmonizedOutput);
            
            // Add subtle analog noise for realism
            harmonizedOutput += state.voices[0].addAnalogNoise(0.0f);
            
            // Mix with dry signal using smoothed parameter
            channelData[sample] = drySignal * (1.0f - m_mix.current) + harmonizedOutput * m_mix.current;
        }
    }
}

float IntelligentHarmonizer::HarmonizerVoice::process(float input, float pitchRatio, 
                                                    float formantAmount, float humanization, 
                                                    double sampleRate) {
    // Apply input DC blocking
    input = inputDCBlocker.process(input);
    
    // Add subtle analog noise
    input = addAnalogNoise(input);
    
    // Update thermal and aging models
    float processingLoad = std::abs(input) * 10.0f;
    thermalModel.update(processingLoad);
    componentAging.update();
    
    float thermalDrift = thermalModel.getTemperatureDrift();
    float agingFactor = componentAging.getAgingFactor();
    
    // Apply thermal compensation to pitch ratio
    pitchRatio *= (1.0f + thermalDrift * 0.1f) * agingFactor;
    
    // Enhanced pitch smoothing with thermal stability
    targetPitch = pitchRatio;
    float adaptiveSmoothingTime = pitchSmoothing * (1.0f + std::abs(thermalDrift) * 10.0f);
    currentPitch = currentPitch * adaptiveSmoothingTime + targetPitch * (1.0f - adaptiveSmoothingTime);
    
    // Write to buffer
    buffer[static_cast<int>(writePos)] = input;
    writePos += 1.0f;
    if (writePos >= BUFFER_SIZE) writePos -= BUFFER_SIZE;
    
    // Enhanced read position calculation with oversampling consideration
    float readIncrement = 1.0f / currentPitch;
    readPos += readIncrement;
    
    // Wrap read position
    while (readPos >= BUFFER_SIZE) readPos -= BUFFER_SIZE;
    while (readPos < 0) readPos += BUFFER_SIZE;
    
    // Ensure minimum delay to prevent overlap (adaptive based on pitch ratio)
    float minDelay = 512.0f * std::max(1.0f, 1.0f / currentPitch);
    float delay = writePos - readPos;
    if (delay < 0) delay += BUFFER_SIZE;
    if (delay < minDelay) {
        readPos = writePos - minDelay;
        if (readPos < 0) readPos += BUFFER_SIZE;
    }
    
    // Superior Hermite interpolation for boutique quality
    int idx0 = static_cast<int>(readPos);
    int idx1 = (idx0 + 1) % BUFFER_SIZE;
    int idx2 = (idx0 + 2) % BUFFER_SIZE;
    int idx3 = (idx0 + 3) % BUFFER_SIZE;
    
    float frac = readPos - idx0;
    float y0 = buffer[idx0];
    float y1 = buffer[idx1];
    float y2 = buffer[idx2];
    float y3 = buffer[idx3];
    
    // Hermite interpolation (superior to cubic for audio)
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    
    // Apply thermal drift to interpolation coefficients for analog character
    c1 *= (1.0f + thermalDrift * 0.05f);
    c2 *= agingFactor;
    
    float output = ((c3 * frac + c2) * frac + c1) * frac + c0;
    
    // Advanced formant preservation with spectral envelope correction
    if (formantAmount > 0.0f && std::abs(currentPitch - 1.0f) > 0.01f) {
        formantPreserver.preserveFormants(input, currentPitch);
        
        // Apply intelligent formant correction
        float formantCorrection = (1.0f - currentPitch) * formantAmount;
        output = formantPreserver.applyFormantCorrection(output, formantCorrection);
        
        // Add subtle formant-related harmonics for realism
        float harmonicContent = output * output * output * 0.005f * formantAmount;
        output += harmonicContent;
    }
    
    // Apply output DC blocking
    output = outputDCBlocker.process(output);
    
    // Add subtle saturation for analog character
    if (std::abs(output) > 0.7f) {
        output = std::tanh(output * 0.8f) * 1.25f;
    }
    
    return output;
}

void IntelligentHarmonizer::PitchDetector::addSample(float sample) {
    buffer[bufferIndex] = sample;
    bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;
}

float IntelligentHarmonizer::PitchDetector::detectPitch(double sampleRate) {
    // Simple autocorrelation-based pitch detection
    float maxCorr = 0.0f;
    int bestLag = 0;
    
    // Search for fundamental frequency between 80Hz and 2000Hz
    int minLag = static_cast<int>(sampleRate / 2000.0);
    int maxLag = static_cast<int>(sampleRate / 80.0);
    
    for (int lag = minLag; lag < maxLag && lag < WINDOW_SIZE / 2; ++lag) {
        float corr = autocorrelation(lag);
        if (corr > maxCorr) {
            maxCorr = corr;
            bestLag = lag;
        }
    }
    
    confidence = maxCorr;
    
    if (bestLag > 0 && confidence > 0.5f) {
        detectedPitch = static_cast<float>(sampleRate) / bestLag;
    }
    
    return detectedPitch;
}

float IntelligentHarmonizer::PitchDetector::autocorrelation(int lag) {
    float sum = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;
    
    for (int i = 0; i < WINDOW_SIZE - lag; ++i) {
        int idx1 = (bufferIndex + i) % WINDOW_SIZE;
        int idx2 = (bufferIndex + i + lag) % WINDOW_SIZE;
        
        float a = buffer[idx1];
        float b = buffer[idx2];
        
        sum += a * b;
        normA += a * a;
        normB += b * b;
    }
    
    if (normA > 0.0f && normB > 0.0f) {
        return sum / std::sqrt(normA * normB);
    }
    
    return 0.0f;
}

int IntelligentHarmonizer::quantizeToScale(int noteOffset, ScaleType scale, int key) {
    // Convert offset to absolute note number
    int absoluteNote = 60 + noteOffset; // Middle C as reference
    
    // Get note within octave
    int noteInOctave = (absoluteNote - key + 1200) % 12; // +1200 to handle negative values
    int octave = (absoluteNote - key) / 12;
    
    // Find closest scale degree
    int closestDegree = 0;
    int minDistance = 12;
    
    for (int degree = 0; degree < 12; ++degree) {
        if (SCALE_INTERVALS[scale][degree] > 11) break; // End of scale
        
        int distance = std::abs(noteInOctave - SCALE_INTERVALS[scale][degree]);
        if (distance < minDistance) {
            minDistance = distance;
            closestDegree = degree;
        }
    }
    
    // Calculate quantized note
    int quantizedNote = key + octave * 12 + SCALE_INTERVALS[scale][closestDegree];
    
    return quantizedNote - 60; // Return as offset from middle C
}

float IntelligentHarmonizer::noteToFrequency(float note) {
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

float IntelligentHarmonizer::frequencyToNote(float frequency) {
    return 69.0f + 12.0f * std::log2(frequency / 440.0f);
}

int IntelligentHarmonizer::getActiveVoices() const {
    // Use smoothed parameter for voice count
    if (m_voiceCount.current < 0.25f) return 1;
    else if (m_voiceCount.current < 0.5f) return 2;
    else if (m_voiceCount.current < 0.75f) return 3;
    else return 4;
}

void IntelligentHarmonizer::updateParameters(const std::map<int, float>& params) {
    // Update target values for smoothed parameters
    if (params.count(0)) m_interval.target = params.at(0);
    if (params.count(1)) m_key.target = params.at(1);
    if (params.count(2)) m_scale.target = params.at(2);
    if (params.count(3)) m_voiceCount.target = params.at(3);
    if (params.count(4)) m_spread.target = params.at(4);
    if (params.count(5)) m_humanize.target = params.at(5);
    if (params.count(6)) m_formant.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String IntelligentHarmonizer::getParameterName(int index) const {
    switch (index) {
        case 0: return "Interval";
        case 1: return "Key";
        case 2: return "Scale";
        case 3: return "Voices";
        case 4: return "Spread";
        case 5: return "Humanize";
        case 6: return "Formant";
        case 7: return "Mix";
        default: return "";
    }
}