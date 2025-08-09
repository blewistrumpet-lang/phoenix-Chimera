// FormantFilter.cpp
#include "FormantFilter.h"
#include "DspEngineUtilities.h"
#include <algorithm>

// Professional vowel formant data with realistic Q ranges (2-20)
const FormantFilter::FormantData FormantFilter::VOWEL_A = {
    700, 1220, 2600,   // F1, F2, F3
    5.0, 7.0, 10.0,    // Q1, Q2, Q3
    1.0, 0.5, 0.25     // A1, A2, A3
};

const FormantFilter::FormantData FormantFilter::VOWEL_E = {
    530, 1840, 2480,
    5.0, 8.0, 10.0,
    1.0, 0.4, 0.2
};

const FormantFilter::FormantData FormantFilter::VOWEL_I = {
    400, 1920, 2650,
    5.0, 9.0, 10.0,
    1.0, 0.35, 0.15
};

const FormantFilter::FormantData FormantFilter::VOWEL_O = {
    570, 840, 2410,
    5.0, 6.0, 10.0,
    1.0, 0.45, 0.2
};

const FormantFilter::FormantData FormantFilter::VOWEL_U = {
    440, 1020, 2240,
    5.0, 6.0, 10.0,
    1.0, 0.3, 0.15
};

FormantFilter::FormantFilter() {
    m_vowelPosition.target = 0.0f;  m_vowelPosition.current = 0.0;
    m_formantShift.target = 0.5f;   m_formantShift.current = 0.5;
    m_resonance.target = 0.4f;      m_resonance.current = 0.4;
    m_morph.target = 0.0f;          m_morph.current = 0.0;
    m_drive.target = 0.0f;          m_drive.current = 0.0;
    m_mix.target = 0.8f;            m_mix.current = 0.8;
}

void FormantFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_blockSize = samplesPerBlock;
    
    // Parameter smoothing times
    m_vowelPosition.setSmoothingTime(50, sampleRate);
    m_formantShift.setSmoothingTime(30, sampleRate);
    m_resonance.setSmoothingTime(20, sampleRate);
    m_morph.setSmoothingTime(100, sampleRate);
    m_drive.setSmoothingTime(10, sampleRate);
    m_mix.setSmoothingTime(10, sampleRate);
    
    // This will be called from the message thread, so we can query channel count
    // For now, allocate for stereo
    m_formantFilters.resize(2);
    m_dcBlockers.resize(2);
    
    // Initialize filters
    for (auto& channel : m_formantFilters) {
        for (auto& formant : channel) {
            formant.reset(sampleRate);
        }
    }
    
    reset();
}

void FormantFilter::reset() {
    for (auto& channel : m_formantFilters) {
        for (auto& formant : channel) {
            formant.reset(m_sampleRate);
        }
    }
    
    for (auto& dc : m_dcBlockers) {
        dc.reset();
    }
    
    m_thermalModel.thermalNoise = 0.0;
    m_thermalModel.noiseFilter = 0.0;
}

void FormantFilter::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Dynamically resize if needed (rare case)
    if (numChannels > m_formantFilters.size()) {
        m_formantFilters.resize(numChannels);
        m_dcBlockers.resize(numChannels);
        for (int ch = 2; ch < numChannels; ++ch) {
            for (auto& formant : m_formantFilters[ch]) {
                formant.reset(m_sampleRate);
            }
        }
    }
    
    // Process using actual block size
    const int blockSize = std::min(m_blockSize, 64); // Cap at 64 for cache efficiency
    
    for (int offset = 0; offset < numSamples; offset += blockSize) {
        int samplesToProcess = std::min(blockSize, numSamples - offset);
        
        // Update parameters once per block
        m_vowelPosition.updateBlock();
        m_formantShift.updateBlock();
        m_resonance.updateBlock();
        m_morph.updateBlock();
        m_drive.updateBlock();
        m_mix.updateBlock();
        m_thermalModel.update(m_sampleRate);
        
        // Determine if we need oversampling for this block
        m_useOversampling = m_drive.current > 0.3;
        
        // Process each channel
        for (int ch = 0; ch < numChannels; ++ch) {
            float* data = buffer.getWritePointer(ch);
            
            for (int i = 0; i < samplesToProcess; ++i) {
                // All processing in double precision
                double input = static_cast<double>(data[offset + i]);
                double output = processSample(input, ch);
                data[offset + i] = static_cast<float>(output);
            }
        }
    }
    
    scrubBuffer(buffer);
}

double FormantFilter::processSample(double input, int channel) {
    // DC blocking
    double x = m_dcBlockers[channel].process(input);
    
    // Get interpolated formant data
    FormantData D = interpolateVowels(m_vowelPosition.current, m_morph.current);
    
    // Update filter parameters
    updateFormantFilters(channel, D);
    
    // Apply drive if needed
    double driven = x;
    if (m_drive.current > 0.01) {
        driven = analogSaturation(x * (1.0 + m_drive.current * 2.0), m_drive.current);
    }
    
    // Process through formant bank
    double output = processFormantBank(driven, channel, m_drive.current);
    
    // Makeup gain based on resonance
    output *= (1.0 + m_resonance.current * 0.3);
    
    // Soft limiting
    if (std::abs(output) > 0.8) {
        output = std::tanh(output * 0.9) / 0.9;
    }
    
    // Mix dry/wet
    return input * (1.0 - m_mix.current) + output * m_mix.current;
}

FormantFilter::FormantData FormantFilter::interpolateVowels(double pos, double morph) const {
    const FormantData *v1, *v2;
    double f;
    
    // Determine vowel pair
    if (pos < 0.25) { 
        v1 = &VOWEL_A; v2 = &VOWEL_E; f = pos * 4.0; 
    } else if (pos < 0.5) { 
        v1 = &VOWEL_E; v2 = &VOWEL_I; f = (pos - 0.25) * 4.0; 
    } else if (pos < 0.75) { 
        v1 = &VOWEL_I; v2 = &VOWEL_O; f = (pos - 0.5) * 4.0; 
    } else { 
        v1 = &VOWEL_O; v2 = &VOWEL_U; f = (pos - 0.75) * 4.0; 
    }
    
    // Apply morph
    double mf = std::clamp(f + morph * 0.5, 0.0, 1.0);
    
    FormantData R;
    // Interpolate all parameters
    R.f1 = v1->f1 + mf * (v2->f1 - v1->f1);
    R.f2 = v1->f2 + mf * (v2->f2 - v1->f2);
    R.f3 = v1->f3 + mf * (v2->f3 - v1->f3);
    
    R.q1 = v1->q1 + mf * (v2->q1 - v1->q1);
    R.q2 = v1->q2 + mf * (v2->q2 - v1->q2);
    R.q3 = v1->q3 + mf * (v2->q3 - v1->q3);
    
    R.a1 = v1->a1 + mf * (v2->a1 - v1->a1);
    R.a2 = v1->a2 + mf * (v2->a2 - v1->a2);
    R.a3 = v1->a3 + mf * (v2->a3 - v1->a3);
    
    // Apply formant shift
    double shift = 0.5 + m_formantShift.current; // 0.5x to 1.5x
    R.f1 = std::clamp(R.f1 * shift, 80.0, 1000.0);
    R.f2 = std::clamp(R.f2 * shift, 200.0, 4000.0);
    R.f3 = std::clamp(R.f3 * shift, 1000.0, 8000.0);
    
    return R;
}

void FormantFilter::updateFormantFilters(int channel, const FormantData& D) {
    double thermal = m_thermalModel.getFactor();
    double resFactor = 1.0 + m_resonance.current * 3.0; // Scale Q by 1x to 4x
    
    // Ensure channel exists
    if (channel >= m_formantFilters.size()) return;
    
    // Update each formant with proper Q range (2-20)
    auto& f1 = m_formantFilters[channel][0];
    f1.freq = D.f1 * thermal;
    f1.q = std::clamp(D.q1 * resFactor, 2.0, 20.0);
    f1.gain = D.a1;
    f1.filter.setParameters(f1.freq, f1.q, m_sampleRate);
    
    auto& f2 = m_formantFilters[channel][1];
    f2.freq = D.f2 * thermal;
    f2.q = std::clamp(D.q2 * resFactor, 2.0, 20.0);
    f2.gain = D.a2;
    f2.filter.setParameters(f2.freq, f2.q, m_sampleRate);
    
    auto& f3 = m_formantFilters[channel][2];
    f3.freq = D.f3 * thermal;
    f3.q = std::clamp(D.q3 * resFactor, 2.0, 20.0);
    f3.gain = D.a3;
    f3.filter.setParameters(f3.freq, f3.q, m_sampleRate);
}

double FormantFilter::processFormantBank(double in, int channel, double drive) {
    double output = 0.0;
    
    // Hoist oversampling decision
    if (m_useOversampling) {
        // Process all formants with oversampling
        for (int i = 0; i < 3; ++i) {
            auto& f = m_formantFilters[channel][i];
            
            double up1, up2;
            f.oversampler.process(in, up1, up2);
            
            double out1 = f.filter.processBandpass(up1);
            double out2 = f.filter.processBandpass(up2);
            
            // Apply saturation at 2x rate if high drive
            if (drive > 0.5) {
                out1 = asymmetricSaturation(out1, drive * 0.3);
                out2 = asymmetricSaturation(out2, drive * 0.3);
            }
            
            double downsampled = f.oversampler.downsample(out1, out2);
            output += downsampled * f.gain;
        }
    } else {
        // Direct processing without oversampling
        for (int i = 0; i < 3; ++i) {
            auto& f = m_formantFilters[channel][i];
            double filtered = f.filter.processBandpass(in);
            output += filtered * f.gain;
        }
    }
    
    return output;
}

double FormantFilter::analogSaturation(double in, double amt) const {
    // Warm analog-style saturation
    return std::tanh(in * 0.8) / (0.8 * (1.0 + amt * 0.3));
}

double FormantFilter::asymmetricSaturation(double in, double amt) const {
    // Tube-like asymmetric saturation
    if (in > 0) {
        return std::tanh(in * 0.7) / (0.7 * (1.0 + amt * 0.2));
    } else {
        return std::tanh(in * 0.9) / (0.9 * (1.0 + amt * 0.1));
    }
}

void FormantFilter::updateParameters(const std::map<int, float>& params) {
    auto it = params.find(kVowelPosition);
    if (it != params.end()) m_vowelPosition.target = std::clamp(it->second, 0.0f, 1.0f);
    
    it = params.find(kFormantShift);
    if (it != params.end()) m_formantShift.target = std::clamp(it->second, 0.0f, 1.0f);
    
    it = params.find(kResonance);
    if (it != params.end()) m_resonance.target = std::clamp(it->second, 0.0f, 1.0f);
    
    it = params.find(kMorph);
    if (it != params.end()) m_morph.target = std::clamp(it->second, 0.0f, 1.0f);
    
    it = params.find(kDrive);
    if (it != params.end()) m_drive.target = std::clamp(it->second, 0.0f, 1.0f);
    
    it = params.find(kMix);
    if (it != params.end()) m_mix.target = std::clamp(it->second, 0.0f, 1.0f);
}

juce::String FormantFilter::getParameterName(int index) const {
    switch (index) {
        case kVowelPosition: return "Vowel";
        case kFormantShift: return "Shift";
        case kResonance: return "Resonance";
        case kMorph: return "Morph";
        case kDrive: return "Drive";
        case kMix: return "Mix";
        default: return "";
    }
}