// SpringReverb_Platinum.cpp - Professional Physical Spring Reverb Implementation
// Ultra-realistic spring modeling with chirp, boing, and saturation

#include "SpringReverb_Platinum.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <chrono>

// Platform-specific SIMD headers with detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
    #if defined(__AVX2__)
        #define HAS_AVX2 1
    #else
        #define HAS_AVX2 0
    #endif
#else
    #define HAS_SSE2 0
    #define HAS_AVX2 0
#endif

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr size_t MAX_SPRING_DELAY = 8192;
constexpr size_t DISPERSION_STAGES = 4;
constexpr float EPSILON = 1e-10f;

//==============================================================================
// Helper Functions
//==============================================================================

// Fast approximation of exp(-x) for x >= 0
inline float fastExp(float x) noexcept {
    // Padé approximation
    x = 1.0f + x * (-0.9998684f + x * (0.4982926f + x * (-0.1595332f + x * 0.0293641f)));
    x *= x; x *= x; x *= x; x *= x;
    return 1.0f / x;
}

// Soft clipping function
inline float softClip(float x) noexcept {
    const float ax = std::abs(x);
    if (ax < 0.5f) return x;
    if (ax > 2.0f) return x > 0.0f ? 1.0f : -1.0f;
    const float x2 = x * x;
    return x * (1.5f - 0.25f * x2);
}

//==============================================================================
// Implementation Class
//==============================================================================

class SpringReverb_Platinum::Impl {
public:
    //==========================================================================
    // Spring Model - Physical simulation of spring behavior
    //==========================================================================
    class SpringModel {
        static constexpr size_t NUM_MODES = 50;  // Number of resonant modes
        
        struct Mode {
            float frequency = 0.0f;
            float decay = 0.0f;
            float amplitude = 0.0f;
            float phase = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;  // State variables
        };
        
        alignas(32) Mode modes[NUM_MODES];
        float sampleRate = 48000.0f;
        float tension = 0.5f;
        float damping = 0.01f;
        float baseFreq = 10.0f;
        
    public:
        void prepare(double sr) {
            sampleRate = static_cast<float>(sr);
            updateModes();
        }
        
        void reset() {
            for (auto& mode : modes) {
                mode.y1 = mode.y2 = 0.0f;
            }
        }
        
        void setParameters(float tens, float damp) {
            tension = tens;
            damping = damp;
            updateModes();
        }
        
        float process(float input) noexcept {
            float output = 0.0f;
            
            // Excite all modes with input
            for (auto& mode : modes) {
                // 2nd order resonator
                const float w = mode.frequency * TWO_PI / sampleRate;
                const float cosw = std::cos(w);
                const float sinw = std::sin(w);
                const float r = fastExp(mode.decay / sampleRate);
                
                const float a1 = -2.0f * r * cosw;
                const float a2 = r * r;
                
                const float y0 = input * mode.amplitude * sinw + 
                                a1 * mode.y1 + a2 * mode.y2;
                
                output += y0;
                mode.y2 = mode.y1;
                mode.y1 = y0;
            }
            
            return output * 0.1f;  // Scale down
        }
        
    private:
        void updateModes() {
            // Physical modeling of spring modes
            const float tensionFactor = 0.2f + tension * 1.8f;
            baseFreq = 5.0f * tensionFactor;
            
            for (size_t i = 0; i < NUM_MODES; ++i) {
                const float n = static_cast<float>(i + 1);
                
                // Frequency with slight inharmonicity
                const float inharmonicity = 1.0f + 0.001f * n * n;
                modes[i].frequency = baseFreq * n * inharmonicity;
                
                // Decay time decreases with frequency
                modes[i].decay = damping * (1.0f + 0.1f * n);
                
                // Amplitude decreases with mode number
                modes[i].amplitude = 1.0f / (n * std::sqrt(n));
                
                // Random phase for natural sound
                modes[i].phase = static_cast<float>(rand()) / RAND_MAX * TWO_PI;
            }
        }
    };
    
    //==========================================================================
    // Dispersion Network - Models frequency-dependent delay
    //==========================================================================
    class DispersionNetwork {
        struct AllpassStage {
            float buffer[512] = {};
            size_t writePos = 0;
            float feedback = 0.7f;
            size_t delay = 100;
            
            float process(float input) noexcept {
                const float delayed = buffer[writePos];
                const float output = -input + delayed;
                buffer[writePos] = input + delayed * feedback;
                writePos = (writePos + 1) % delay;
                return output;
            }
            
            void reset() {
                std::fill(std::begin(buffer), std::end(buffer), 0.0f);
                writePos = 0;
            }
        };
        
        AllpassStage stages[DISPERSION_STAGES];
        
    public:
        void prepare(double sampleRate) {
            // Set up cascade of allpass filters for dispersion
            const size_t delays[] = {113, 137, 151, 173};
            const float feedbacks[] = {0.7f, 0.65f, 0.6f, 0.55f};
            
            for (size_t i = 0; i < DISPERSION_STAGES; ++i) {
                stages[i].delay = delays[i];
                stages[i].feedback = feedbacks[i];
                stages[i].reset();
            }
        }
        
        void reset() {
            for (auto& stage : stages) {
                stage.reset();
            }
        }
        
        float process(float input) noexcept {
            float signal = input;
            for (auto& stage : stages) {
                signal = stage.process(signal);
            }
            return signal;
        }
    };
    
    //==========================================================================
    // Chirp Generator - Transient "boing" effect
    //==========================================================================
    class ChirpGenerator {
        float phase = 0.0f;
        float frequency = 0.0f;
        float targetFreq = 0.0f;
        float envelope = 0.0f;
        float sampleRate = 48000.0f;
        float chirpAmount = 0.5f;
        
    public:
        void prepare(double sr) {
            sampleRate = static_cast<float>(sr);
        }
        
        void reset() {
            phase = 0.0f;
            frequency = 0.0f;
            envelope = 0.0f;
        }
        
        void setAmount(float amount) {
            chirpAmount = amount;
        }
        
        void trigger(float velocity) {
            if (chirpAmount > 0.01f) {
                envelope = velocity * chirpAmount;
                frequency = 2000.0f;  // Start high
                targetFreq = 100.0f;  // Sweep down
            }
        }
        
        float process() noexcept {
            if (envelope < 0.001f) return 0.0f;
            
            // Generate chirp
            const float output = std::sin(phase) * envelope;
            phase += frequency * TWO_PI / sampleRate;
            if (phase > TWO_PI) phase -= TWO_PI;
            
            // Update frequency (exponential sweep)
            frequency += (targetFreq - frequency) * 0.05f;
            
            // Update envelope
            envelope *= 0.995f;
            
            return output;
        }
    };
    
    //==========================================================================
    // Modulation LFO - Spring wobble
    //==========================================================================
    class ModulationLFO {
        float phase = 0.0f;
        float rate = 0.3f;
        float depth = 0.0f;
        float sampleRate = 48000.0f;
        
    public:
        void prepare(double sr) {
            sampleRate = static_cast<float>(sr);
        }
        
        void reset() {
            phase = 0.0f;
        }
        
        void setParameters(float r, float d) {
            rate = r;
            depth = d;
        }
        
        float process() noexcept {
            const float mod = std::sin(phase) * depth;
            phase += rate * TWO_PI / sampleRate;
            if (phase > TWO_PI) phase -= TWO_PI;
            return 1.0f + mod * 0.002f;  // Slight pitch modulation
        }
    };
    
    //==========================================================================
    // Main Implementation
    //==========================================================================
    
    // Processing components
    struct Channel {
        SpringModel springs[3];              // Multiple springs
        DispersionNetwork dispersion;        // Frequency dispersion
        ChirpGenerator chirp;               // Transient effect
        float delayBuffer[MAX_SPRING_DELAY] = {};
        size_t writePos = 0;
        float lowpass = 0.0f;               // Damping filter state
        float highpass = 0.0f;              // DC blocker state
        
        void prepare(double sampleRate) {
            for (auto& spring : springs) {
                spring.prepare(sampleRate);
            }
            dispersion.prepare(sampleRate);
            chirp.prepare(sampleRate);
            reset();
        }
        
        void reset() {
            for (auto& spring : springs) {
                spring.reset();
            }
            dispersion.reset();
            chirp.reset();
            std::fill(std::begin(delayBuffer), std::end(delayBuffer), 0.0f);
            writePos = 0;
            lowpass = highpass = 0.0f;
        }
    };
    
    // State
    Channel channels[2];
    ModulationLFO modLFO;
    
    // Parameters (atomic for thread safety)
    struct Parameters {
        std::atomic<float> tension{0.5f};
        std::atomic<float> damping{0.5f};
        std::atomic<float> decay{0.5f};
        std::atomic<float> modulation{0.3f};
        std::atomic<float> chirp{0.5f};
        std::atomic<float> drive{0.3f};
        std::atomic<float> width{0.8f};
        std::atomic<float> mix{0.5f};
    } params;
    
    // Parameter smoothing
    struct Smoothers {
        struct OnePoleSmoother {
            float current = 0.0f;
            float target = 0.0f;
            float coeff = 0.0f;
            
            void setCoeff(double sampleRate, float timeMs) {
                coeff = std::exp(-1.0f / (sampleRate * timeMs * 0.001f));
            }
            
            void setTarget(float t) { target = t; }
            float getCurrentValue() const { return current; }
            void reset(float value) { current = target = value; }
            
            float tick() {
                current += (target - current) * (1.0f - coeff);
                return current;
            }
        };
        
        OnePoleSmoother tension, damping, decay, modulation;
        OnePoleSmoother chirp, drive, width, mix;
    } smoothers;
    
    // Configuration
    Config config;
    SpringType springType = SpringType::VINTAGE_LONG;
    
    // Processing state
    float sampleRate = 48000.0f;
    float invSampleRate = 1.0f / 48000.0f;
    
    // Metering
    std::atomic<float> inputLevel{0.0f};
    std::atomic<float> outputLevel{0.0f};
    std::atomic<float> springExcursion{0.0f};
    
    // Performance measurement
    std::atomic<uint32_t> denormalCount{0};
    
    //==========================================================================
    // Constructor
    //==========================================================================
    Impl() {
        #if HAS_SSE2
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
        #endif
    }
    
    //==========================================================================
    // Core Processing
    //==========================================================================
    void process(juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        if (numSamples == 0) return;
        
        // Update smoothers
        smoothers.tension.setTarget(params.tension.load());
        smoothers.damping.setTarget(params.damping.load());
        smoothers.decay.setTarget(params.decay.load());
        smoothers.modulation.setTarget(params.modulation.load());
        smoothers.chirp.setTarget(params.chirp.load());
        smoothers.drive.setTarget(params.drive.load());
        smoothers.width.setTarget(params.width.load());
        smoothers.mix.setTarget(params.mix.load());
        
        // Get smoothed parameters
        const float tension = smoothers.tension.tick();
        const float damping = smoothers.damping.tick();
        const float modDepth = smoothers.modulation.tick();
        const float chirpAmount = smoothers.chirp.tick();
        const float drive = smoothers.drive.tick();
        
        // Update components
        modLFO.setParameters(0.3f + modDepth * 2.0f, modDepth);
        for (int ch = 0; ch < 2; ++ch) {
            channels[ch].chirp.setAmount(chirpAmount);
            for (auto& spring : channels[ch].springs) {
                spring.setParameters(tension, damping * 0.1f);
            }
        }
        
        // Process audio
        if (numChannels == 1) {
            // Mono processing
            processMono(buffer.getWritePointer(0), numSamples, drive);
        } else if (numChannels >= 2) {
            // Stereo processing
            processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), 
                         numSamples, drive);
        }
        
        // Update metering
        float peakIn = 0.0f, peakOut = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                peakIn = std::max(peakIn, std::abs(data[i]));
                peakOut = std::max(peakOut, std::abs(data[i]));
            }
        }
        
        inputLevel.store(20.0f * std::log10(peakIn + EPSILON));
        outputLevel.store(20.0f * std::log10(peakOut + EPSILON));
    }
    
private:
    void processMono(float* data, int numSamples, float drive) {
        const float mix = smoothers.mix.tick();
        const float decay = smoothers.decay.tick();
        
        for (int i = 0; i < numSamples; ++i) {
            const float dry = data[i];
            
            // Input saturation
            float wet = softClip(dry * (1.0f + drive * 3.0f));
            
            // Detect transients for chirp
            static float prevInput = 0.0f;
            const float transient = std::abs(wet - prevInput);
            if (transient > 0.5f) {
                channels[0].chirp.trigger(transient);
            }
            prevInput = wet;
            
            // Add chirp
            wet += channels[0].chirp.process() * 0.3f;
            
            // Process through springs with modulation
            const float modFactor = modLFO.process();
            float springOut = 0.0f;
            
            for (auto& spring : channels[0].springs) {
                springOut += spring.process(wet) * modFactor;
            }
            
            // Dispersion
            springOut = channels[0].dispersion.process(springOut);
            
            // Decay control via feedback delay
            const size_t delayTime = static_cast<size_t>(1000.0f + decay * 7000.0f);
            const size_t readPos = (channels[0].writePos + MAX_SPRING_DELAY - delayTime) % MAX_SPRING_DELAY;
            
            const float delayed = channels[0].delayBuffer[readPos];
            springOut += delayed * decay * 0.7f;
            
            // High frequency damping
            const float dampingCoeff = 0.9f - damping * 0.3f;
            channels[0].lowpass = springOut + (channels[0].lowpass - springOut) * dampingCoeff;
            springOut = channels[0].lowpass;
            
            // Write to delay buffer
            channels[0].delayBuffer[channels[0].writePos] = springOut;
            channels[0].writePos = (channels[0].writePos + 1) % MAX_SPRING_DELAY;
            
            // DC blocking
            channels[0].highpass = springOut - prevInput + channels[0].highpass * 0.995f;
            springOut = channels[0].highpass;
            
            // Mix
            data[i] = dry * (1.0f - mix) + springOut * mix;
            
            // Update spring excursion metering
            springExcursion.store(std::abs(springOut));
        }
    }
    
    void processStereo(float* left, float* right, int numSamples, float drive) {
        const float mix = smoothers.mix.tick();
        const float width = smoothers.width.tick();
        const float decay = smoothers.decay.tick();
        
        for (int i = 0; i < numSamples; ++i) {
            const float dryL = left[i];
            const float dryR = right[i];
            
            // Convert to M/S for width control
            const float mid = (dryL + dryR) * 0.5f;
            const float side = (dryL - dryR) * 0.5f;
            
            // Process mid through channel 0
            float wetMid = softClip(mid * (1.0f + drive * 3.0f));
            
            // Detect transients
            static float prevMid = 0.0f;
            const float transient = std::abs(wetMid - prevMid);
            if (transient > 0.5f) {
                channels[0].chirp.trigger(transient);
                channels[1].chirp.trigger(transient * 0.7f);  // Slightly different
            }
            prevMid = wetMid;
            
            // Add chirp
            wetMid += channels[0].chirp.process() * 0.3f;
            
            // Process through springs
            const float modFactor = modLFO.process();
            float springMid = 0.0f;
            
            for (auto& spring : channels[0].springs) {
                springMid += spring.process(wetMid) * modFactor;
            }
            
            // Process side through channel 1 for stereo effect
            float wetSide = side * (1.0f + drive);
            wetSide += channels[1].chirp.process() * 0.2f;
            
            float springSide = 0.0f;
            for (auto& spring : channels[1].springs) {
                springSide += spring.process(wetSide) * modFactor * 1.1f;  // Slightly different
            }
            
            // Dispersion
            springMid = channels[0].dispersion.process(springMid);
            springSide = channels[1].dispersion.process(springSide);
            
            // Delay and feedback for both channels
            for (int ch = 0; ch < 2; ++ch) {
                float* signal = (ch == 0) ? &springMid : &springSide;
                const size_t delayTime = static_cast<size_t>(1000.0f + decay * 7000.0f + ch * 37);
                const size_t readPos = (channels[ch].writePos + MAX_SPRING_DELAY - delayTime) % MAX_SPRING_DELAY;
                
                const float delayed = channels[ch].delayBuffer[readPos];
                *signal += delayed * decay * 0.7f;
                
                // Damping
                const float dampingCoeff = 0.9f - damping * 0.3f;
                channels[ch].lowpass = *signal + (channels[ch].lowpass - *signal) * dampingCoeff;
                *signal = channels[ch].lowpass;
                
                // Write to delay
                channels[ch].delayBuffer[channels[ch].writePos] = *signal;
                channels[ch].writePos = (channels[ch].writePos + 1) % MAX_SPRING_DELAY;
                
                // DC block
                static float prev[2] = {0.0f, 0.0f};
                channels[ch].highpass = *signal - prev[ch] + channels[ch].highpass * 0.995f;
                *signal = channels[ch].highpass;
                prev[ch] = *signal;
            }
            
            // Convert back from M/S with width control
            const float wetL = springMid + springSide * width;
            const float wetR = springMid - springSide * width;
            
            // Final mix
            left[i] = dryL * (1.0f - mix) + wetL * mix;
            right[i] = dryR * (1.0f - mix) + wetR * mix;
            
            // Update metering
            springExcursion.store(std::max(std::abs(springMid), std::abs(springSide)));
        }
    }
};

//==============================================================================
// Public Interface Implementation
//==============================================================================

SpringReverb_Platinum::SpringReverb_Platinum() : pImpl(std::make_unique<Impl>()) {}
SpringReverb_Platinum::~SpringReverb_Platinum() = default;

void SpringReverb_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->sampleRate = static_cast<float>(sampleRate);
    pImpl->invSampleRate = 1.0f / pImpl->sampleRate;
    
    // Prepare channels
    for (auto& channel : pImpl->channels) {
        channel.prepare(sampleRate);
    }
    
    // Prepare modulation
    pImpl->modLFO.prepare(sampleRate);
    
    // Setup smoothers
    pImpl->smoothers.tension.setCoeff(sampleRate, 20.0f);
    pImpl->smoothers.damping.setCoeff(sampleRate, 20.0f);
    pImpl->smoothers.decay.setCoeff(sampleRate, 50.0f);
    pImpl->smoothers.modulation.setCoeff(sampleRate, 30.0f);
    pImpl->smoothers.chirp.setCoeff(sampleRate, 10.0f);
    pImpl->smoothers.drive.setCoeff(sampleRate, 10.0f);
    pImpl->smoothers.width.setCoeff(sampleRate, 30.0f);
    pImpl->smoothers.mix.setCoeff(sampleRate, 20.0f);
    
    // Initialize smoothers
    pImpl->smoothers.tension.reset(pImpl->params.tension);
    pImpl->smoothers.damping.reset(pImpl->params.damping);
    pImpl->smoothers.decay.reset(pImpl->params.decay);
    pImpl->smoothers.modulation.reset(pImpl->params.modulation);
    pImpl->smoothers.chirp.reset(pImpl->params.chirp);
    pImpl->smoothers.drive.reset(pImpl->params.drive);
    pImpl->smoothers.width.reset(pImpl->params.width);
    pImpl->smoothers.mix.reset(pImpl->params.mix);
    
    reset();
}

void SpringReverb_Platinum::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void SpringReverb_Platinum::reset() {
    for (auto& channel : pImpl->channels) {
        channel.reset();
    }
    pImpl->modLFO.reset();
}

void SpringReverb_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (index) {
            case 0: pImpl->params.tension.store(value); break;
            case 1: pImpl->params.damping.store(value); break;
            case 2: pImpl->params.decay.store(value); break;
            case 3: pImpl->params.modulation.store(value); break;
            case 4: pImpl->params.chirp.store(value); break;
            case 5: pImpl->params.drive.store(value); break;
            case 6: pImpl->params.width.store(value); break;
            case 7: pImpl->params.mix.store(value); break;
        }
    }
}

juce::String SpringReverb_Platinum::getParameterName(int index) const {
    switch (index) {
        case 0: return "Tension";
        case 1: return "Damping";
        case 2: return "Decay";
        case 3: return "Modulation";
        case 4: return "Chirp";
        case 5: return "Drive";
        case 6: return "Width";
        case 7: return "Mix";
        default: return "";
    }
}

float SpringReverb_Platinum::getParameterValue(int index) const {
    switch (index) {
        case 0: return pImpl->params.tension.load();
        case 1: return pImpl->params.damping.load();
        case 2: return pImpl->params.decay.load();
        case 3: return pImpl->params.modulation.load();
        case 4: return pImpl->params.chirp.load();
        case 5: return pImpl->params.drive.load();
        case 6: return pImpl->params.width.load();
        case 7: return pImpl->params.mix.load();
        default: return 0.0f;
    }
}

void SpringReverb_Platinum::setParameterValue(int index, float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    switch (index) {
        case 0: pImpl->params.tension.store(value); break;
        case 1: pImpl->params.damping.store(value); break;
        case 2: pImpl->params.decay.store(value); break;
        case 3: pImpl->params.modulation.store(value); break;
        case 4: pImpl->params.chirp.store(value); break;
        case 5: pImpl->params.drive.store(value); break;
        case 6: pImpl->params.width.store(value); break;
        case 7: pImpl->params.mix.store(value); break;
    }
}

float SpringReverb_Platinum::getParameterDefaultValue(int index) const {
    switch (index) {
        case 0: return 0.5f;  // Tension
        case 1: return 0.5f;  // Damping
        case 2: return 0.5f;  // Decay
        case 3: return 0.3f;  // Modulation
        case 4: return 0.5f;  // Chirp
        case 5: return 0.3f;  // Drive
        case 6: return 0.8f;  // Width
        case 7: return 0.5f;  // Mix
        default: return 0.0f;
    }
}

juce::String SpringReverb_Platinum::getParameterText(int index) const {
    const float value = getParameterValue(index);
    
    switch (index) {
        case 0: return juce::String(value * 100.0f, 1) + "%";
        case 1: return juce::String(value * 100.0f, 1) + "%";
        case 2: return juce::String(0.1f + value * 4.9f, 2) + "s";
        case 3: return juce::String(value * 100.0f, 1) + "%";
        case 4: return juce::String(value * 100.0f, 1) + "%";
        case 5: return juce::String(value * 100.0f, 1) + "%";
        case 6: return juce::String(value * 100.0f, 1) + "%";
        case 7: return juce::String(value * 100.0f, 1) + "%";
        default: return "";
    }
}

void SpringReverb_Platinum::setSpringType(SpringType type) {
    pImpl->springType = type;
    
    // Configure based on type
    switch (type) {
        case SpringType::VINTAGE_LONG:
            pImpl->config.springLength = 0.4f;
            pImpl->config.numSprings = 3;
            break;
            
        case SpringType::VINTAGE_SHORT:
            pImpl->config.springLength = 0.2f;
            pImpl->config.numSprings = 2;
            break;
            
        case SpringType::MODERN_BRIGHT:
            pImpl->config.springLength = 0.3f;
            pImpl->config.numSprings = 4;
            break;
            
        case SpringType::WARM_DARK:
            pImpl->config.springLength = 0.5f;
            pImpl->config.numSprings = 2;
            break;
            
        case SpringType::EXPERIMENTAL:
            pImpl->config.springLength = 0.6f;
            pImpl->config.numSprings = 4;
            pImpl->config.enableChirp = true;
            pImpl->config.enableModulation = true;
            break;
    }
}

SpringReverb_Platinum::SpringType SpringReverb_Platinum::getSpringType() const {
    return pImpl->springType;
}

void SpringReverb_Platinum::setConfig(const Config& config) {
    pImpl->config = config;
}

SpringReverb_Platinum::Config SpringReverb_Platinum::getConfig() const {
    return pImpl->config;
}

float SpringReverb_Platinum::getInputLevel() const {
    return pImpl->inputLevel.load();
}

float SpringReverb_Platinum::getOutputLevel() const {
    return pImpl->outputLevel.load();
}

float SpringReverb_Platinum::getSpringExcursion() const {
    return pImpl->springExcursion.load();
}