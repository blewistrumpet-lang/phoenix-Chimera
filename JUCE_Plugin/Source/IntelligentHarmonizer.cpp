#include "IntelligentHarmonizer.h"
#include <cmath>
#include <algorithm>

// Initialize scale intervals
constexpr int IntelligentHarmonizer::SCALE_INTERVALS[NUM_SCALES][12];

IntelligentHarmonizer::IntelligentHarmonizer() {
    // Initialize smoothed parameters with defaults
    m_interval.reset(0.5f);      // Center = no transposition
    m_key.reset(0.0f);           // C
    m_scale.reset(0.0f);         // Major
    m_voiceCount.reset(0.0f);    // 1 voice
    m_spread.reset(0.3f);        // Moderate spread
    m_humanize.reset(0.0f);      // No humanization
    m_formant.reset(0.0f);       // No formant correction
    m_mix.reset(0.5f);           // 50% wet
}

void IntelligentHarmonizer::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_maxBlockSize = samplesPerBlock;
    
    // Resize wet buffer to handle the maximum block size
    m_wetBuffer.resize(samplesPerBlock);
    
    // Set parameter smoothing times
    float fastSmoothingTime = 20.0f;   // 20ms for most parameters
    float slowSmoothingTime = 100.0f;  // 100ms for key/scale changes
    
    m_interval.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_key.setSmoothingTime(slowSmoothingTime, sampleRate);
    m_scale.setSmoothingTime(slowSmoothingTime, sampleRate);
    m_voiceCount.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_spread.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_humanize.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_formant.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_mix.setSmoothingTime(fastSmoothingTime, sampleRate);
    
    // Prepare channel states
    for (auto& channel : m_channelStates) {
        channel.prepare(sampleRate);
    }
}

void IntelligentHarmonizer::reset() {
    for (auto& channel : m_channelStates) {
        channel.inputDC.reset();
        channel.outputDC.reset();
        channel.antiAliasFilter.reset();
        for (auto& voice : channel.voices) {
            voice.prepare(m_sampleRate);
        }
    }
}

void IntelligentHarmonizer::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Ensure we don't exceed pre-allocated buffer
    jassert(numSamples <= m_maxBlockSize);
    if (numSamples > m_wetBuffer.size()) {
        // Safety fallback - resize if needed (shouldn't happen)
        m_wetBuffer.resize(numSamples);
    }
    
    // Update all smoothed parameters
    m_interval.update();
    m_key.update();
    m_scale.update();
    m_voiceCount.update();
    m_spread.update();
    m_humanize.update();
    m_formant.update();
    m_mix.update();
    
    // Calculate base interval in semitones
    int baseInterval = static_cast<int>((m_interval.current - 0.5f) * MAX_INTERVAL_SEMITONES); // -24 to +24
    
    // Clamp to safe range
    baseInterval = std::max(-24, std::min(24, baseInterval));
    
    // Get active voice count
    int activeVoices = getActiveVoices();
    
    // Get scale type and root key
    ScaleType scaleType = static_cast<ScaleType>(
        std::min(static_cast<int>(m_scale.current * NUM_SCALES), NUM_SCALES - 1)
    );
    int rootKey = static_cast<int>(m_key.current * 12.0f) % 12; // 0-11 (C-B)
    
    // Pitch detection on first channel only (moved outside main loop)
    if (numChannels > 0) {
        auto& detector = m_channelStates[0].pitchDetector;
        const float* firstChannel = buffer.getReadPointer(0);
        
        // Decimate by PITCH_DETECT_DECIMATION for efficiency
        for (int i = 0; i < numSamples; i += PITCH_DETECT_DECIMATION) {
            detector.addSample(firstChannel[i]);
        }
        
        float detectedFreq = detector.detectPitch(m_sampleRate);
        if (detectedFreq > 0.0f && detector.confidence > 0.5f) {
            m_currentDetectedNote = frequencyToNote(detectedFreq);
        }
    }
    
    // Pre-calculate pan gains for each voice (once per block)
    std::array<std::pair<float, float>, 4> panGains;
    if (numChannels == 2 && activeVoices > 1) {
        for (int voice = 0; voice < activeVoices; ++voice) {
            float pan = (voice - (activeVoices - 1) * 0.5f) / 
                       std::max(1.0f, activeVoices - 1.0f);
            pan *= m_spread.current;
            
            // Constant power panning
            float angle = (pan + 1.0f) * 0.25f * M_PI;
            panGains[voice] = {std::cos(angle), std::sin(angle)};
        }
    } else {
        // Mono or single voice - no panning
        for (int i = 0; i < 4; ++i) {
            panGains[i] = {1.0f, 1.0f};
        }
    }
    
    // Process each channel
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Clear wet buffer for this channel
        std::fill(m_wetBuffer.begin(), m_wetBuffer.begin() + numSamples, 0.0f);
        
        // Process each sample
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            
            // Remove DC
            input = state.inputDC.process(input);
            
            // Process each active voice
            float harmonizedOutput = 0.0f;
            
            for (int voice = 0; voice < activeVoices; ++voice) {
                auto& harmonizer = state.voices[voice];
                
                // Calculate harmony interval for this voice
                int voiceInterval = baseInterval;
                calculateHarmonyIntervals(baseInterval, scaleType, voice, activeVoices, voiceInterval);
                
                // Quantize to scale if not chromatic
                if (scaleType != CHROMATIC) {
                    voiceInterval = quantizeToScale(voiceInterval, scaleType, rootKey);
                }
                
                // Clamp final interval to safe range
                voiceInterval = std::max(-36, std::min(36, voiceInterval));
                
                // Calculate pitch ratio
                float pitchRatio = std::pow(2.0f, voiceInterval / 12.0f);
                
                // Process voice
                float voiceOut = harmonizer.process(input, pitchRatio, m_formant.current, 
                                                  m_humanize.current, m_sampleRate);
                
                // Apply pre-calculated panning
                float gain = (channel == 0) ? panGains[voice].first : panGains[voice].second;
                voiceOut *= gain;
                
                // Mix voices with equal power
                harmonizedOutput += voiceOut / std::sqrt(static_cast<float>(activeVoices));
            }
            
            // Anti-aliasing
            harmonizedOutput = state.antiAliasFilter.process(harmonizedOutput);
            
            // Remove DC from output
            harmonizedOutput = state.outputDC.process(harmonizedOutput);
            
            // Store wet signal
            m_wetBuffer[sample] = harmonizedOutput;
        }
        
        // Apply dry/wet mix
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = channelData[sample] * (1.0f - m_mix.current) + 
                                 m_wetBuffer[sample] * m_mix.current;
        }
    }
}

void IntelligentHarmonizer::HarmonizerVoice::prepare(double sampleRate) {
    buffer.fill(0.0f);
    writeIndex = 0;
    
    for (auto& grain : grains) {
        grain.reset();
    }
    nextGrain = 0;
    grainCounter = 0;
    
    currentPitch = 1.0f;
    targetPitch = 1.0f;
    
    pitchSmoother.setCutoff(50.0f, sampleRate);
    pitchSmoother.reset();
    
    formantFilter.setCutoff(1000.0f, sampleRate);
    formantFilter.reset();
    
    vibratoPhase = 0.0f;
    driftPhase = 0.0f;
}

float IntelligentHarmonizer::HarmonizerVoice::process(float input, float pitchRatio, 
                                                     float formantAmount, float humanization, 
                                                     double sampleRate) {
    // Write input to circular buffer
    buffer[writeIndex] = input;
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;
    
    // Smooth pitch changes
    targetPitch = pitchRatio;
    currentPitch = pitchSmoother.process(targetPitch);
    
    // Add humanization
    if (humanization > 0.0f) {
        // Vibrato (4-6 Hz)
        vibratoPhase += 2.0f * M_PI * 5.0f / sampleRate;
        if (vibratoPhase > 2.0f * M_PI) vibratoPhase -= 2.0f * M_PI;
        
        // Slow drift (0.1-0.3 Hz)
        driftPhase += 2.0f * M_PI * 0.2f / sampleRate;
        if (driftPhase > 2.0f * M_PI) driftPhase -= 2.0f * M_PI;
        
        float vibrato = std::sin(vibratoPhase) * humanization * 0.02f; // ±2 cents
        float drift = std::sin(driftPhase) * humanization * 0.01f;     // ±1 cent
        float random = noise(rng) * humanization * 0.005f;             // ±0.5 cents
        
        float totalMod = vibrato + drift + random;
        currentPitch *= std::pow(2.0f, totalMod / 12.0f);
    }
    
    // Grain scheduling - create new grain every GRAIN_SIZE/GRAIN_OVERLAP_FACTOR samples for overlap
    grainCounter++;
    if (grainCounter >= GRAIN_SIZE / GRAIN_OVERLAP_FACTOR) {
        grainCounter = 0;
        
        // Find inactive grain
        for (int i = 0; i < MAX_GRAINS; ++i) {
            if (!grains[i].active) {
                grains[i].active = true;
                grains[i].age = 0;
                grains[i].fadeIn = 0.0f;
                grains[i].fadeOut = 1.0f;
                // Start reading from a safe position behind write head
                grains[i].readPos = writeIndex - GRAIN_SIZE * 2;
                if (grains[i].readPos < 0) grains[i].readPos += BUFFER_SIZE;
                break;
            }
        }
    }
    
    // Process all active grains
    float output = 0.0f;
    int activeGrains = 0;
    
    for (auto& grain : grains) {
        if (!grain.active) continue;
        
        // Calculate grain envelope position (0 to 1)
        float grainPos = static_cast<float>(grain.age) / GRAIN_SIZE;
        
        // Apply window function
        float window = windowFunction(grainPos);
        
        // Read from buffer with interpolation
        int readInt = static_cast<int>(grain.readPos);
        float readFrac = grain.readPos - readInt;
        
        int idx0 = readInt % BUFFER_SIZE;
        int idx1 = (readInt + 1) % BUFFER_SIZE;
        
        // Linear interpolation
        float sample = buffer[idx0] * (1.0f - readFrac) + buffer[idx1] * readFrac;
        
        // Apply window
        output += sample * window;
        activeGrains++;
        
        // Update grain
        grain.readPos += currentPitch;
        while (grain.readPos >= BUFFER_SIZE) grain.readPos -= BUFFER_SIZE;
        
        grain.age++;
        if (grain.age >= GRAIN_SIZE) {
            grain.active = false;
        }
    }
    
    // Normalize by active grain count
    if (activeGrains > 0) {
        output /= std::sqrt(static_cast<float>(activeGrains));
    }
    
    // Formant preservation
    if (formantAmount > 0.0f && std::abs(currentPitch - 1.0f) > 0.01f) {
        // Simple formant shift by filtering
        float formantRatio = 1.0f / currentPitch;
        float filteredOutput = formantFilter.process(output);
        output = output * (1.0f - formantAmount) + filteredOutput * formantAmount * formantRatio;
    }
    
    return output;
}

void IntelligentHarmonizer::PitchDetector::addSample(float sample) {
    buffer[bufferIndex] = sample;
    bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
}

float IntelligentHarmonizer::PitchDetector::detectPitch(double sampleRate) {
    // Enhanced zero-crossing pitch detection with octave error rejection
    int zeroCrossings = 0;
    float lastSample = 0.0f;
    float totalEnergy = 0.0f;
    std::vector<float> crossingPeriods;
    int lastCrossingIndex = -1;
    
    // Analyze buffer
    for (int i = 1; i < BUFFER_SIZE; ++i) {
        float currentSample = buffer[(bufferIndex + i) % BUFFER_SIZE];
        totalEnergy += currentSample * currentSample;
        
        // Detect upward zero crossing
        if (lastSample < 0.0f && currentSample >= 0.0f) {
            if (lastCrossingIndex >= 0) {
                float period = i - lastCrossingIndex;
                crossingPeriods.push_back(period);
            }
            lastCrossingIndex = i;
            zeroCrossings++;
        }
        
        lastSample = currentSample;
    }
    
    // Calculate confidence based on signal energy
    float rmsAmplitude = std::sqrt(totalEnergy / BUFFER_SIZE);
    confidence = std::min(1.0f, rmsAmplitude * 20.0f); // Scale for typical audio levels
    
    if (crossingPeriods.size() >= 3 && confidence > 0.1f) {
        // Use median period to reject octave errors
        float medianPeriod = findMedianPeriod(crossingPeriods);
        
        if (medianPeriod > 0) {
            float frequency = sampleRate / medianPeriod;
            
            // Clamp to reasonable frequency range
            frequency = std::max(80.0f, std::min(2000.0f, frequency));
            
            // Smooth the pitch estimate
            detectedPitch = pitchFilter.process(frequency);
        }
    }
    
    return detectedPitch;
}

float IntelligentHarmonizer::PitchDetector::medianPeriod() {
    // This is now handled inside detectPitch with the local crossingPeriods vector
    // Kept for backward compatibility if needed
    return 0.0f;
}

// Helper function to find median (add this as a private method)
float IntelligentHarmonizer::PitchDetector::findMedianPeriod(std::vector<float>& periods) {
    if (periods.empty()) return 0.0f;
    
    // Sort periods
    std::sort(periods.begin(), periods.end());
    
    // Return median
    size_t n = periods.size();
    if (n % 2 == 0) {
        return (periods[n/2 - 1] + periods[n/2]) / 2.0f;
    } else {
        return periods[n/2];
    }
}

void IntelligentHarmonizer::calculateHarmonyIntervals(int baseInterval, ScaleType scale, 
                                                      int voiceIndex, int totalVoices, 
                                                      int& interval) {
    // Create proper chord voicings based on the scale
    if (totalVoices == 1) {
        interval = baseInterval;
    } else if (totalVoices == 2) {
        // Two voices: root and third or fifth
        switch (voiceIndex) {
            case 0: interval = baseInterval; break;
            case 1: 
                if (scale == MAJOR || scale == MIXOLYDIAN) {
                    interval = baseInterval + 4; // Major third
                } else {
                    interval = baseInterval + 3; // Minor third
                }
                break;
        }
    } else if (totalVoices == 3) {
        // Three voices: triad
        switch (voiceIndex) {
            case 0: interval = baseInterval; break;
            case 1: 
                if (scale == MAJOR || scale == MIXOLYDIAN) {
                    interval = baseInterval + 4; // Major third
                } else {
                    interval = baseInterval + 3; // Minor third
                }
                break;
            case 2: interval = baseInterval + 7; break; // Fifth
        }
    } else {
        // Four voices: seventh chord
        switch (voiceIndex) {
            case 0: interval = baseInterval; break;
            case 1: 
                if (scale == MAJOR || scale == MIXOLYDIAN) {
                    interval = baseInterval + 4; // Major third
                } else {
                    interval = baseInterval + 3; // Minor third
                }
                break;
            case 2: interval = baseInterval + 7; break; // Fifth
            case 3: 
                if (scale == MAJOR) {
                    interval = baseInterval + 11; // Major seventh
                } else if (scale == MIXOLYDIAN || scale == BLUES) {
                    interval = baseInterval + 10; // Dominant seventh
                } else {
                    interval = baseInterval + 10; // Minor seventh
                }
                break;
        }
    }
}

int IntelligentHarmonizer::quantizeToScale(int noteOffset, ScaleType scale, int rootKey) {
    // Get the absolute note number
    int absoluteNote = 60 + noteOffset; // Using middle C as reference
    
    // Calculate the note's position relative to the root
    // Proper modulo for negative numbers
    int noteFromRoot = ((absoluteNote - rootKey) % 12 + 12) % 12;
    
    // Find the closest scale degree
    int closestDegree = 0;
    int minDistance = 12;
    
    for (int i = 0; i < 12; ++i) {
        if (SCALE_INTERVALS[scale][i] == -1) break; // End of scale
        
        int distance = std::abs(noteFromRoot - SCALE_INTERVALS[scale][i]);
        // Also check distance with wraparound
        int distanceWrap = std::abs(12 - distance);
        distance = std::min(distance, distanceWrap);
        
        if (distance < minDistance) {
            minDistance = distance;
            closestDegree = SCALE_INTERVALS[scale][i];
        }
    }
    
    // Calculate the quantized note
    int octaveOffset = (absoluteNote - rootKey) / 12;
    if (absoluteNote < rootKey && (absoluteNote - rootKey) % 12 != 0) {
        octaveOffset--; // Handle negative octaves properly
    }
    
    int quantizedNote = rootKey + octaveOffset * 12 + closestDegree;
    
    // Return as offset from middle C
    return quantizedNote - 60;
}

float IntelligentHarmonizer::noteToFrequency(float note) {
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

float IntelligentHarmonizer::frequencyToNote(float frequency) {
    if (frequency <= 0.0f) return 60.0f; // Default to middle C
    return 69.0f + 12.0f * std::log2(frequency / 440.0f);
}

int IntelligentHarmonizer::getActiveVoices() const {
    if (m_voiceCount.current < 0.25f) return 1;
    else if (m_voiceCount.current < 0.5f) return 2;
    else if (m_voiceCount.current < 0.75f) return 3;
    else return 4;
}

void IntelligentHarmonizer::updateParameters(const std::map<int, float>& params) {
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