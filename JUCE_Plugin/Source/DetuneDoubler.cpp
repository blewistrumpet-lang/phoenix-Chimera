// DetuneDoubler.cpp
#include "DetuneDoubler.h"

namespace AudioDSP {

DetuneDoubler::DetuneDoubler() {
    // Initialize voices with shared RNG
    for (auto& voice : m_voices) {
        voice.pitchShifter = std::make_unique<PitchShifter>(m_randomGen);
        voice.delay = std::make_unique<DelayLine>();
        voice.phaseNetwork = std::make_unique<AllPassNetwork>(m_randomGen);
        voice.modulator = std::make_unique<ModulationGenerator>(m_randomGen);
        voice.tapeFilter = std::make_unique<BiquadFilter>();
    }
    
    // Initialize parameters
    m_detuneParam = std::make_unique<ParameterSmoother>();
    m_delayParam = std::make_unique<ParameterSmoother>();
    m_widthParam = std::make_unique<ParameterSmoother>();
    m_thicknessParam = std::make_unique<ParameterSmoother>();
    m_mixParam = std::make_unique<ParameterSmoother>();
    
    // Set defaults
    m_detuneParam->reset(0.3f);
    m_delayParam->reset(0.15f);
    m_widthParam->reset(0.7f);
    m_thicknessParam->reset(0.3f);
    m_mixParam->reset(0.5f);
}

DetuneDoubler::~DetuneDoubler() = default;

void DetuneDoubler::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Configure all voices
    for (int i = 0; i < 4; ++i) {
        m_voices[i].pitchShifter->setSampleRate(sampleRate);
        
        // Set different modulation rates for each voice
        m_voices[i].modulator->setSampleRate(sampleRate);
        m_voices[i].modulator->setRates(0.1f + i * 0.03f);
        
        // Configure tape filter with slight high-frequency boost
        m_voices[i].tapeFilter->setHighShelf(8000.0, sampleRate, 2.0);
        
        // Randomize all-pass networks for each voice
        m_voices[i].phaseNetwork->randomize();
    }
    
    // Configure smoothers
    m_detuneParam->setSampleRate(sampleRate);
    m_detuneParam->setSmoothingTime(20.0f);
    
    m_delayParam->setSampleRate(sampleRate);
    m_delayParam->setSmoothingTime(30.0f);
    
    m_widthParam->setSampleRate(sampleRate);
    m_widthParam->setSmoothingTime(20.0f);
    
    m_thicknessParam->setSampleRate(sampleRate);
    m_thicknessParam->setSmoothingTime(20.0f);
    
    m_mixParam->setSampleRate(sampleRate);
    m_mixParam->setSmoothingTime(10.0f);
    
    // Reset smoothers to their current values
    m_detuneParam->reset(m_detuneParam->getCurrentValue());
    m_delayParam->reset(m_delayParam->getCurrentValue());
    m_widthParam->reset(m_widthParam->getCurrentValue());
    m_thicknessParam->reset(m_thicknessParam->getCurrentValue());
    m_mixParam->reset(m_mixParam->getCurrentValue());
    
    reset();
}

void DetuneDoubler::reset() {
    for (auto& voice : m_voices) {
        voice.reset();
    }
}

void DetuneDoubler::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numSamples == 0) return;
    
    if (numChannels >= 2) {
        processStereo(buffer.getWritePointer(0),
                     buffer.getWritePointer(1),
                     numSamples);
    } else if (numChannels == 1) {
        // Process mono as dual mono
        float* data = buffer.getWritePointer(0);
        processStereo(data, data, numSamples);
    }
}

void DetuneDoubler::processStereo(float* left, float* right, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        // Get smoothed parameters
        float detune = m_detuneParam->getNextValue();
        float delay = m_delayParam->getNextValue();
        float width = m_widthParam->getNextValue();
        float thickness = m_thicknessParam->getNextValue();
        float mix = m_mixParam->getNextValue();
        
        // Store dry signal
        float dryL = left[i];
        float dryR = right[i];
        
        // Calculate detune amounts for each voice
        float detuneCents = detune * MAX_DETUNE_CENTS;
        m_voices[0].pitchShifter->setPitchShift(detuneCents);
        m_voices[1].pitchShifter->setPitchShift(-detuneCents * 0.7f);
        m_voices[2].pitchShifter->setPitchShift(-detuneCents);
        m_voices[3].pitchShifter->setPitchShift(detuneCents * 0.7f);
        
        // Calculate delay times with modulation
        float baseDelayMs = MIN_DELAY_MS + delay * (MAX_DELAY_MS - MIN_DELAY_MS);
        float baseDelaySamples = baseDelayMs * m_sampleRate / 1000.0f;
        
        // Process each voice
        float voice1L, voice2L, voice1R, voice2R;
        
        // Left channel voices
        float mod1 = m_voices[0].modulator->generate();
        float delay1 = baseDelaySamples * (1.0f + mod1 * 0.02f);
        m_voices[0].delay->setDelay(delay1);
        
        voice1L = m_voices[0].pitchShifter->process(dryL);
        voice1L = m_voices[0].delay->process(voice1L);
        voice1L = m_voices[0].phaseNetwork->process(voice1L);
        voice1L = m_voices[0].tapeFilter->processSample(voice1L);
        
        float mod2 = m_voices[1].modulator->generate();
        float delay2 = baseDelaySamples * (1.0f + mod2 * 0.02f) * 1.1f;
        m_voices[1].delay->setDelay(delay2);
        
        voice2L = m_voices[1].pitchShifter->process(dryL);
        voice2L = m_voices[1].delay->process(voice2L);
        voice2L = m_voices[1].phaseNetwork->process(voice2L);
        voice2L = m_voices[1].tapeFilter->processSample(voice2L);
        
        // Right channel voices
        float mod3 = m_voices[2].modulator->generate();
        float delay3 = baseDelaySamples * (1.0f + mod3 * 0.02f) * 0.95f;
        m_voices[2].delay->setDelay(delay3);
        
        voice1R = m_voices[2].pitchShifter->process(dryR);
        voice1R = m_voices[2].delay->process(voice1R);
        voice1R = m_voices[2].phaseNetwork->process(voice1R);
        voice1R = m_voices[2].tapeFilter->processSample(voice1R);
        
        float mod4 = m_voices[3].modulator->generate();
        float delay4 = baseDelaySamples * (1.0f + mod4 * 0.02f) * 1.05f;
        m_voices[3].delay->setDelay(delay4);
        
        voice2R = m_voices[3].pitchShifter->process(dryR);
        voice2R = m_voices[3].delay->process(voice2R);
        voice2R = m_voices[3].phaseNetwork->process(voice2R);
        voice2R = m_voices[3].tapeFilter->processSample(voice2R);
        
        // Apply stereo width and thickness
        float centerL = (voice1L + voice2L) * 0.5f;
        float centerR = (voice1R + voice2R) * 0.5f;
        float sideL = (voice1L - voice2L) * 0.5f;
        float sideR = (voice1R - voice2R) * 0.5f;
        
        // Cross-mixing for width
        float crossAmount = width * 0.5f;
        float thickL = centerL + sideR * crossAmount + centerR * thickness * 0.3f;
        float thickR = centerR + sideL * crossAmount + centerL * thickness * 0.3f;
        
        // Mix with dry signal
        left[i] = dryL * (1.0f - mix) + thickL * mix * 0.7f;
        right[i] = dryR * (1.0f - mix) + thickR * mix * 0.7f;
        
        // Soft limiting
        left[i] = std::tanh(left[i] * 0.9f) * 1.1f;
        right[i] = std::tanh(right[i] * 0.9f) * 1.1f;
    }
}

void DetuneDoubler::updateParameters(const std::map<int, float>& params) {
    auto it = params.find(0);
    if (it != params.end()) m_detuneParam->setTargetValue(clamp01(it->second));
    
    it = params.find(1);
    if (it != params.end()) m_delayParam->setTargetValue(clamp01(it->second));
    
    it = params.find(2);
    if (it != params.end()) m_widthParam->setTargetValue(clamp01(it->second));
    
    it = params.find(3);
    if (it != params.end()) m_thicknessParam->setTargetValue(clamp01(it->second));
    
    it = params.find(4);
    if (it != params.end()) m_mixParam->setTargetValue(clamp01(it->second));
}

juce::String DetuneDoubler::getParameterName(int index) const {
    switch (index) {
        case 0: return "Detune Amount";
        case 1: return "Delay Time";
        case 2: return "Stereo Width";
        case 3: return "Thickness";
        case 4: return "Mix";
        default: return "";
    }
}

} // namespace AudioDSP