#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <random>

class BufferRepeat : public EngineBase {
public:
    BufferRepeat();
    ~BufferRepeat() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Buffer Repeat"; }
    
private:
    // Smoothed parameters for boutique quality
    struct SmoothParam {
        float target = 0.5f;
        float current = 0.5f;
        float smoothing = 0.995f;
        
        void update() {
            current = target + (current - target) * smoothing;
        }
        
        void reset(float value) {
            target = current = value;
        }
        
        void setSmoothingTime(float timeMs, float sampleRate) {
            float samples = timeMs * 0.001f * sampleRate;
            smoothing = std::exp(-1.0f / samples);
        }
    };
    
    SmoothParam m_division;       // Beat division (1/64 to 4 bars)
    SmoothParam m_probability;    // Repeat probability
    SmoothParam m_feedback;       // Feedback amount
    SmoothParam m_filter;         // Filter cutoff
    SmoothParam m_pitch;          // Pitch shift amount
    SmoothParam m_reverse;        // Reverse probability
    SmoothParam m_stutter;        // Stutter/gate effect
    SmoothParam m_mix;            // Dry/wet mix
    
    // Buffer settings
    static constexpr int MAX_BUFFER_SIZE = 192000; // ~4 seconds at 48kHz
    static constexpr int MIN_SLICE_SIZE = 64;
    
    // Beat divisions
    enum Division {
        DIV_64TH,
        DIV_32ND,
        DIV_16TH,
        DIV_8TH,
        DIV_QUARTER,
        DIV_HALF,
        DIV_BAR,
        DIV_2BARS,
        DIV_4BARS
    };
    
    // Slice playback state
    struct SlicePlayer {
        std::vector<float> buffer;
        int bufferSize = 0;
        int writePos = 0;
        float readPos = 0.0f;
        bool isPlaying = false;
        bool isReversed = false;
        float pitchRatio = 1.0f;
        float feedback = 0.0f;
        int sliceStart = 0;
        int sliceLength = 0;
        int repeatCount = 0;
        
        void startSlice(int start, int length, bool reverse, float pitch) {
            sliceStart = start;
            sliceLength = std::max(MIN_SLICE_SIZE, length);
            isReversed = reverse;
            pitchRatio = pitch;
            readPos = reverse ? static_cast<float>(sliceLength - 1) : 0.0f;
            isPlaying = true;
            repeatCount = 0;
        }
        
        float getNextSample() {
            if (!isPlaying || sliceLength == 0) return 0.0f;
            
            // Calculate actual buffer position
            int bufferPos = (sliceStart + static_cast<int>(readPos)) % bufferSize;
            
            // Linear interpolation for smooth playback
            float frac = readPos - std::floor(readPos);
            int pos0 = bufferPos;
            int pos1 = (bufferPos + 1) % bufferSize;
            
            float sample = buffer[pos0] * (1.0f - frac) + buffer[pos1] * frac;
            
            // Update read position with pitch adjustment
            if (isReversed) {
                readPos -= pitchRatio;
                if (readPos < 0.0f) {
                    // Loop or stop
                    readPos += sliceLength;
                    repeatCount++;
                    if (feedback <= 0.01f && repeatCount > 0) {
                        isPlaying = false;
                    }
                }
            } else {
                readPos += pitchRatio;
                if (readPos >= sliceLength) {
                    // Loop or stop
                    readPos -= sliceLength;
                    repeatCount++;
                    if (feedback <= 0.01f && repeatCount > 0) {
                        isPlaying = false;
                    }
                }
            }
            
            // Apply feedback attenuation
            float gain = std::pow(feedback, static_cast<float>(repeatCount));
            return sample * gain;
        }
        
        void reset() {
            isPlaying = false;
            readPos = 0.0f;
            repeatCount = 0;
        }
    };
    
    // Stutter gate
    struct StutterGate {
        float phase = 0.0f;
        float rate = 8.0f; // Hz
        
        float process(float input, float amount, double sampleRate) {
            if (amount < 0.01f) return input;
            
            // Generate gate pattern
            float gate = (std::sin(2.0f * M_PI * phase) > 0.0f) ? 1.0f : 0.0f;
            
            // Smooth the gate transitions
            static float smoothGate = 0.0f;
            float smoothing = 0.995f;
            smoothGate = smoothGate * smoothing + gate * (1.0f - smoothing);
            
            // Update phase
            phase += rate / sampleRate;
            if (phase >= 1.0f) phase -= 1.0f;
            
            // Apply gating
            return input * (1.0f - amount + amount * smoothGate);
        }
        
        void setRate(float division) {
            // Convert division to Hz (assuming 120 BPM base)
            rate = 2.0f * std::pow(2.0f, division * 8.0f); // 2Hz to 512Hz
        }
    };
    
    // Enhanced degradation filter with aging
    struct DegradationFilter {
        float state = 0.0f;
        float cutoff = 0.5f;
        
        // 2nd order filter for better sound quality
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;
        
        float processLowpass(float input, float aging = 0.0f) {
            // Enhanced filter with aging effects
            float adjustedCutoff = cutoff;
            if (aging > 0.01f) {
                adjustedCutoff *= (1.0f - aging * 0.1f); // HF rolloff with age
            }
            
            // 2nd order Butterworth lowpass
            float omega = adjustedCutoff * M_PI;
            float sin_omega = std::sin(omega);
            float cos_omega = std::cos(omega);
            float alpha = sin_omega / (2.0f * 0.707f);
            
            float a0 = (1.0f - cos_omega) * 0.5f;
            float a1 = 1.0f - cos_omega;
            float a2 = a0;
            float b1 = -2.0f * cos_omega;
            float b2 = 1.0f - alpha;
            
            float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
            
            x2 = x1; x1 = input;
            y2 = y1; y1 = output;
            
            return output;
        }
        
        float processHighpass(float input, float aging = 0.0f) {
            float lp = processLowpass(input, aging);
            return input - lp * 0.8f; // Gentler highpass
        }
        
        void setCutoff(float normalized) {
            cutoff = std::max(0.001f, std::min(0.999f, normalized));
        }
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0f;
            state = 0.0f;
        }
    };
    
    // DC Blocking filter
    struct DCBlocker {
        float x1 = 0.0f, y1 = 0.0f;
        static constexpr float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
        
        void reset() { x1 = y1 = 0.0f; }
    };
    
    // Thermal modeling for analog drift simulation
    struct ThermalModel {
        float temperature = 25.0f;  // Celsius
        float thermalNoise = 0.0f;
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist{-0.5f, 0.5f};
        
        ThermalModel() : rng(std::random_device{}()) {}
        
        void update(double sampleRate) {
            // Slow thermal drift affecting buffer timing
            thermalNoise += (dist(rng) * 0.0007f) / sampleRate;
            thermalNoise = std::max(-0.012f, std::min(0.012f, thermalNoise));
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalNoise;
        }
    };
    
    // Component aging simulation
    struct ComponentAging {
        float age = 0.0f;
        float timingDrift = 0.0f;
        float feedbackDegradation = 0.0f;
        
        void update(float aging) {
            age = aging;
            timingDrift = aging * 0.01f;  // 1% timing drift
            feedbackDegradation = aging * 0.05f;  // Feedback path degradation
        }
        
        float applyTimingDrift(float value) const {
            return value * (1.0f + timingDrift);
        }
        
        float applyFeedbackDegradation(float value) const {
            return value * (1.0f - feedbackDegradation);
        }
    };
    
    // Enhanced channel state with boutique features
    struct ChannelState {
        // Main recording buffer with oversampling
        std::vector<float> recordBuffer;
        int writePos = 0;
        
        // Enhanced slice players for layered repeats
        static constexpr int NUM_PLAYERS = 6; // More players for complex patterns
        std::array<SlicePlayer, NUM_PLAYERS> slicePlayers;
        int currentPlayer = 0;
        
        // Enhanced effects
        StutterGate stutterGate;
        DegradationFilter filter;
        
        // DC blockers for input and output
        DCBlocker inputDCBlocker;
        DCBlocker outputDCBlocker;
        
        // Thermal and aging models
        ThermalModel thermalModel;
        ComponentAging componentAging;
        
        // Enhanced random number generation
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist{0.0f, 1.0f};
        
        // Timing with lookahead
        int samplesSinceLastSlice = 0;
        int nextSliceTime = 0;
        
        // Oversampling for high-quality processing
        struct Oversampler {
            static constexpr int OVERSAMPLE_FACTOR = 2;
            std::vector<float> upsampleBuffer;
            std::vector<float> downsampleBuffer;
            
            // Simple anti-aliasing filter
            struct AAFilter {
                float x1 = 0.0f, y1 = 0.0f;
                
                float process(float input) {
                    // Simple 1-pole lowpass at Fs/4
                    const float cutoff = 0.25f;
                    y1 += cutoff * (input - y1);
                    return y1;
                }
            };
            
            AAFilter upsampleFilter;
            AAFilter downsampleFilter;
            
            void prepare(int blockSize) {
                upsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
                downsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
            }
        };
        
        Oversampler oversampler;
        bool useOversampling = false; // Enable for critical processing
        
        // Noise floor simulation
        float noiseFloor = -84.0f; // dB
        
        // Enhanced pitch shifting with better quality
        struct EnhancedPitchShift {
            static constexpr int BUFFER_SIZE = 8192;
            std::vector<float> buffer;
            float writePos = 0.0f;
            float readPos = 0.0f;
            
            // Overlap-add for smoother pitch shifting
            static constexpr int OVERLAP_SIZE = 512;
            std::vector<float> overlapBuffer;
            std::vector<float> windowFunc;
            
            void prepare() {
                buffer.resize(BUFFER_SIZE);
                overlapBuffer.resize(OVERLAP_SIZE);
                windowFunc.resize(OVERLAP_SIZE);
                
                std::fill(buffer.begin(), buffer.end(), 0.0f);
                std::fill(overlapBuffer.begin(), overlapBuffer.end(), 0.0f);
                
                // Create Hann window
                for (int i = 0; i < OVERLAP_SIZE; ++i) {
                    windowFunc[i] = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / (OVERLAP_SIZE - 1));
                }
                
                writePos = 0.0f;
                readPos = 0.0f;
            }
            
            float process(float input, float pitchFactor, float aging = 0.0f) {
                // Apply aging effects to pitch stability
                float adjustedPitch = pitchFactor;
                if (aging > 0.03f) {
                    static thread_local juce::Random random;
                    static float pitchWobble = 0.0f;
                    pitchWobble += (random.nextFloat() - 0.5f) * aging * 0.0005f;
                    pitchWobble *= 0.9995f;  // Slow decay
                    adjustedPitch *= (1.0f + pitchWobble);
                }
                
                // Write to buffer
                buffer[static_cast<int>(writePos)] = input;
                writePos = std::fmod(writePos + 1.0f, BUFFER_SIZE);
                
                // Read with improved interpolation
                float output = 0.0f;
                
                // Cubic interpolation for smoother sound
                int idx0 = static_cast<int>(readPos);
                int idx1 = (idx0 + 1) % BUFFER_SIZE;
                int idx2 = (idx0 + 2) % BUFFER_SIZE;
                int idx3 = (idx0 + 3) % BUFFER_SIZE;
                
                float frac = readPos - std::floor(readPos);
                
                float y0 = buffer[idx0];
                float y1 = buffer[idx1];
                float y2 = buffer[idx2];
                float y3 = buffer[idx3];
                
                // Hermite interpolation
                float c0 = y1;
                float c1 = 0.5f * (y2 - y0);
                float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
                float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
                
                output = ((c3 * frac + c2) * frac + c1) * frac + c0;
                
                // Update read position
                readPos += adjustedPitch;
                if (readPos >= BUFFER_SIZE) readPos -= BUFFER_SIZE;
                if (readPos < 0) readPos += BUFFER_SIZE;
                
                return output;
            }
        };
        
        EnhancedPitchShift enhancedPitchShift;
        
        void prepare(double sampleRate) {
            recordBuffer.resize(MAX_BUFFER_SIZE);
            std::fill(recordBuffer.begin(), recordBuffer.end(), 0.0f);
            
            for (auto& player : slicePlayers) {
                player.buffer.resize(MAX_BUFFER_SIZE);
                player.bufferSize = MAX_BUFFER_SIZE;
                player.reset();
            }
            
            writePos = 0;
            currentPlayer = 0;
            samplesSinceLastSlice = 0;
            nextSliceTime = 0;
            
            // Initialize DC blockers
            inputDCBlocker.reset();
            outputDCBlocker.reset();
            
            // Initialize thermal model with unique seed
            rng.seed(std::random_device{}());
            thermalModel = ThermalModel();
            
            // Initialize component aging
            componentAging.update(0.0f);
            
            // Prepare enhanced pitch shift
            enhancedPitchShift.prepare();
            
            // Prepare oversampler
            oversampler.prepare(512);
            
            // Initialize filter
            filter.reset();
        }
        
        void triggerSlice(int sliceSize, float probability, bool reverse, float pitch) {
            if (dist(rng) > probability) return;
            
            // Find available player or use next in rotation
            SlicePlayer* player = nullptr;
            for (auto& p : slicePlayers) {
                if (!p.isPlaying) {
                    player = &p;
                    break;
                }
            }
            
            if (!player) {
                player = &slicePlayers[currentPlayer];
                currentPlayer = (currentPlayer + 1) % NUM_PLAYERS;
            }
            
            // Copy current buffer content to player
            std::copy(recordBuffer.begin(), recordBuffer.end(), player->buffer.begin());
            
            // Calculate slice position (going back from current write position)
            int sliceStart = (writePos - sliceSize + MAX_BUFFER_SIZE) % MAX_BUFFER_SIZE;
            
            // Start the slice
            player->startSlice(sliceStart, sliceSize, reverse, pitch);
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // Tempo sync with thermal drift
    float m_bpm = 120.0f;
    
    // Component aging tracking
    float m_componentAge = 0.0f;
    int m_sampleCount = 0;
    
    // Enhanced processing flags
    bool m_enableThermalModeling = true;
    bool m_enableComponentAging = true;
    bool m_enableOversampling = false;
    
    // Lookahead processing for better slice detection
    struct LookaheadProcessor {
        static constexpr int LOOKAHEAD_SAMPLES = 64;
        std::array<float, LOOKAHEAD_SAMPLES> buffer;
        int writePos = 0;
        
        void process(float input, float& delayedOutput, float& lookaheadPeak) {
            buffer[writePos] = input;
            
            // Get delayed output
            int readPos = (writePos - LOOKAHEAD_SAMPLES + LOOKAHEAD_SAMPLES) % LOOKAHEAD_SAMPLES;
            delayedOutput = buffer[readPos];
            
            // Calculate lookahead peak
            lookaheadPeak = 0.0f;
            for (float sample : buffer) {
                lookaheadPeak = std::max(lookaheadPeak, std::abs(sample));
            }
            
            writePos = (writePos + 1) % LOOKAHEAD_SAMPLES;
        }
    };
    
    std::array<LookaheadProcessor, 2> m_lookaheadProcessors;
    
    // Helper functions
    int getDivisionSamples(Division div) const;
    Division getDivisionFromParam(float param) const;
    float getPitchRatio(float param) const;
    
    // Enhanced helper methods for boutique functionality
    void updateAllSmoothParams();
    void updateComponentAging();
    float applyAnalogCharacter(float input, float thermalFactor, float aging);
    
    // Advanced buffer processing techniques
    void processEnhancedSlicing(ChannelState& state, float input, float thermalFactor, float aging);
    float applyVintageBufferCharacter(float input, float feedback, float aging);
    void optimizeSliceTimings(ChannelState& state, float lookaheadPeak);
};