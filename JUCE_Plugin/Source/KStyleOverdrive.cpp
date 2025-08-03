#include "KStyleOverdrive.h"
#include <cmath>

KStyleOverdrive::KStyleOverdrive() {
    // Initialize parameters with musical defaults
    m_drive.reset(0.3f);  // 30% - warm but not distorted
    m_tone.reset(0.5f);   // 50% - balanced tone
    m_level.reset(0.5f);  // Unity gain
    m_mix.reset(1.0f);    // 100% wet by default for overdrive
}

void KStyleOverdrive::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Reset all filter states
    for (auto& state : m_filterStates) {
        state.reset();
    }
    
    // Calculate smoothing coefficient for ~10ms at current sample rate
    float smoothingTime = 0.01f; // 10ms
    float samples = smoothingTime * sampleRate;
    m_drive.smoothing = std::exp(-1.0f / samples);
    m_tone.smoothing = m_drive.smoothing;
    m_level.smoothing = m_drive.smoothing;
    m_mix.smoothing = m_drive.smoothing;
    
    updateFilterCoefficients();
}

void KStyleOverdrive::reset() {
    // Reset distortion state
    // TODO: Implement proper reset for KStyleOverdrive if needed
}

void KStyleOverdrive::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < numChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update smoothed parameters
            m_drive.update();
            m_tone.update();
            m_level.update();
            m_mix.update();
            
            // Store dry signal
            float dry = channelData[sample];
            
            // Process wet signal
            float wet = processSample(channelData[sample], channel);
            
            // Mix dry and wet
            channelData[sample] = dry * (1.0f - m_mix.current) + wet * m_mix.current;
        }
    }
}

float KStyleOverdrive::processSample(float input, int channel) {
    auto& state = m_filterStates[channel];
    
    // Input gain staging
    float sample = input * 0.7f; // Headroom for processing
    
    // Stage 1: Highpass filter to remove DC and tighten low end (80Hz)
    float hp_freq = 80.0f / m_sampleRate;
    float hp_a = std::exp(-2.0f * M_PI * hp_freq);
    float hp_out = sample - state.hp_z1;
    state.hp_z1 = sample - hp_out * hp_a;
    sample = hp_out;
    
    // Stage 2: Pre-emphasis filter to brighten signal before clipping
    float pre_freq = 720.0f / m_sampleRate;
    float pre_gain = 1.0f + m_drive.current * 0.5f; // More pre-emphasis with more drive
    float pre_a = std::exp(-2.0f * M_PI * pre_freq);
    float pre_out = sample + (sample - state.pre_z1) * pre_gain * (1.0f - pre_a);
    state.pre_z1 = sample;
    sample = pre_out;
    
    // Stage 3: Upsample for cleaner distortion
    float upsampled1 = upsample(sample, state.up_z1);
    float upsampled2 = upsample(sample, state.up_z1);
    
    // Process both upsampled samples
    float processed1 = processUpsampled(upsampled1, state);
    float processed2 = processUpsampled(upsampled2, state);
    
    // Downsample
    downsample(processed1, state.down_z1);
    sample = downsample(processed2, state.down_z1);
    
    // Stage 4: Tone stack
    sample = applyToneStack(sample, state);
    
    // Stage 5: DC blocker (essential after asymmetric clipping)
    float dc_freq = 20.0f / m_sampleRate;
    float dc_a = 1.0f - (2.0f * M_PI * dc_freq);
    float dc_out = sample - state.dc_z1_in + state.dc_z1_out * dc_a;
    state.dc_z1_in = sample;
    state.dc_z1_out = dc_out;
    sample = dc_out;
    
    // Output level with soft limiting
    float output = sample * m_level.current * 1.4f; // Compensate for processing loss
    
    // Soft output limiting to prevent harsh digital clipping
    if (std::abs(output) > 0.95f) {
        output = 0.95f * std::tanh(output);
    }
    
    return output;
}

float KStyleOverdrive::processUpsampled(float input, FilterStage& state) {
    // Apply tube-style clipping at higher sample rate
    float driven = input * (1.0f + m_drive.current * 15.0f); // 1x to 16x gain
    return applyTubeStage(driven, m_drive.current);
}

float KStyleOverdrive::applyTubeStage(float input, float drive) {
    // Authentic tube-style processing with multiple stages
    
    // Stage 1: Soft knee compression (tube sag)
    float threshold = 0.5f;
    float ratio = 1.0f + drive * 3.0f; // 1:1 to 4:1
    if (std::abs(input) > threshold) {
        float excess = std::abs(input) - threshold;
        float compressed = threshold + excess / ratio;
        input = compressed * (input < 0 ? -1.0f : 1.0f);
    }
    
    // Stage 2: Asymmetric waveshaping for even harmonics
    float bias = m_tubeStage.bias * drive;
    float biased = input + bias;
    
    // Stage 3: Variable-knee saturation
    float knee = 0.1f + drive * 0.3f;
    float x = biased;
    float sign = x < 0 ? -1.0f : 1.0f;
    x = std::abs(x);
    
    float y;
    if (x < knee) {
        // Linear region
        y = x;
    } else if (x < 1.0f) {
        // Soft saturation region (modified tanh curve)
        float t = (x - knee) / (1.0f - knee);
        y = knee + (1.0f - knee) * std::tanh(t * 2.0f) / std::tanh(2.0f);
    } else {
        // Hard clipping region with slight rounding
        y = 1.0f - std::exp(-(x - 1.0f) * 5.0f) * 0.05f;
    }
    
    // Restore sign and remove bias
    float output = sign * y - bias * 0.7f;
    
    // Stage 4: Dynamic harmonic enhancement
    float harmonics = drive * 0.2f;
    float h2 = output * output * harmonics * 0.3f;         // 2nd harmonic
    float h3 = output * output * output * harmonics * 0.15f; // 3rd harmonic
    output += h2 - h3;
    
    return output * 0.7f; // Scale back to prevent overload
}

float KStyleOverdrive::applyToneStack(float input, FilterStage& state) {
    // Interactive 3-band tone stack simulation
    float sample = input;
    
    // Low shelf centered at 100Hz
    float low_freq = 100.0f / m_sampleRate;
    float low_gain = 1.0f - m_tone.current * 0.5f; // More bass when tone is low
    float low_a = std::exp(-2.0f * M_PI * low_freq);
    
    // Process low shelf
    float low_in = sample;
    float low_out = low_in + (state.low_z1 - low_in) * low_a;
    state.low_z1 = low_out;
    sample = low_in + (low_out - low_in) * low_gain;
    
    // Mid scoop/boost at 500Hz (interactive with tone control)
    float mid_freq = 500.0f / m_sampleRate;
    float mid_gain = 1.0f - std::abs(m_tone.current - 0.5f) * 0.3f; // Scoop at extremes
    float mid_a = std::exp(-2.0f * M_PI * mid_freq * 2.0f); // Wider Q
    
    float mid_in = sample;
    float mid_out = mid_in + (state.mid_z1 - mid_in) * mid_a;
    state.mid_z1 = mid_out;
    sample = mid_in + (mid_out - mid_in) * mid_gain;
    
    // High shelf at 3kHz
    float high_freq = 3000.0f / m_sampleRate;
    float high_gain = 0.7f + m_tone.current * 0.6f; // More treble when tone is high
    float high_a = std::exp(-2.0f * M_PI * high_freq);
    
    float high_in = sample;
    float high_out = high_in - (state.high_z1 - high_in) * high_a;
    state.high_z1 = high_in;
    sample = high_in + (high_out - high_in) * high_gain;
    
    return sample;
}

float KStyleOverdrive::upsample(float input, float& z1) {
    // Simple linear interpolation upsampling
    float output = (input + z1) * 0.5f;
    z1 = input;
    return output;
}

float KStyleOverdrive::downsample(float input, float& z1) {
    // Simple averaging downsampling with slight filtering
    float output = (input + z1) * 0.5f;
    z1 = input;
    return output;
}

void KStyleOverdrive::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) {
        m_drive.target = params.at(0);
    }
    if (params.find(1) != params.end()) {
        m_tone.target = params.at(1);
        updateFilterCoefficients();
    }
    if (params.find(2) != params.end()) {
        m_level.target = params.at(2);
    }
    if (params.find(3) != params.end()) {
        m_mix.target = params.at(3);
    }
}

juce::String KStyleOverdrive::getParameterName(int index) const {
    switch (index) {
        case 0: return "Drive";
        case 1: return "Tone";
        case 2: return "Output";
        case 3: return "Mix";
        default: return "";
    }
}

void KStyleOverdrive::updateFilterCoefficients() {
    // Update any sample-rate dependent coefficients if needed
    // Most coefficients are calculated in-place for efficiency
}