// RotarySpeaker.cpp
#include "RotarySpeaker.h"

namespace AudioDSP {

RotarySpeaker::RotarySpeaker() {
    // Initialize components
    for (int i = 0; i < 2; ++i) {
        m_crossover[i] = std::make_unique<CrossoverFilter>();
        m_hornDoppler[i] = std::make_unique<DopplerProcessor>();
        m_drumDoppler[i] = std::make_unique<DopplerProcessor>();
        m_hornAM[i] = std::make_unique<AmplitudeModulator>();
        m_drumAM[i] = std::make_unique<AmplitudeModulator>();
        m_preamp[i] = std::make_unique<TubePreamp>();
    }
    
    m_cabinet = std::make_unique<CabinetSimulator>();
    
    // Initialize parameters
    m_speedParam = std::make_unique<ParameterSmoother>();
    m_mixParam = std::make_unique<ParameterSmoother>();
    m_driveParam = std::make_unique<ParameterSmoother>();
    m_micDistanceParam = std::make_unique<ParameterSmoother>();
    m_stereoWidthParam = std::make_unique<ParameterSmoother>();
    
    // Set defaults
    m_speedParam->reset(0.5f);
    m_mixParam->reset(1.0f);
    m_driveParam->reset(0.3f);
    m_micDistanceParam->reset(0.6f);
    m_stereoWidthParam->reset(0.8f);
}

RotarySpeaker::~RotarySpeaker() = default;

void RotarySpeaker::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Configure components
    for (int i = 0; i < 2; ++i) {
        m_crossover[i]->setSampleRate(sampleRate);
        m_crossover[i]->setCrossoverFrequency(800.0); // Standard Leslie crossover
        
        m_hornDoppler[i]->setSampleRate(sampleRate);
        m_drumDoppler[i]->setSampleRate(sampleRate);
        
        m_preamp[i]->setSampleRate(sampleRate);
    }
    
    m_cabinet->setSampleRate(sampleRate);
    
    // Configure smoothers
    m_speedParam->setSampleRate(sampleRate);
    m_speedParam->setSmoothingTime(50.0f); // 50ms for speed changes
    
    m_mixParam->setSampleRate(sampleRate);
    m_mixParam->setSmoothingTime(10.0f);
    
    m_driveParam->setSampleRate(sampleRate);
    m_driveParam->setSmoothingTime(10.0f);
    
    m_micDistanceParam->setSampleRate(sampleRate);
    m_micDistanceParam->setSmoothingTime(20.0f);
    
    m_stereoWidthParam->setSampleRate(sampleRate);
    m_stereoWidthParam->setSmoothingTime(20.0f);
    
    reset();
}

void RotarySpeaker::reset() {
    // Reset all components
    for (int i = 0; i < 2; ++i) {
        m_crossover[i]->reset();
        m_hornDoppler[i]->reset();
        m_drumDoppler[i]->reset();
        m_hornAM[i]->reset();
        m_drumAM[i]->reset();
        m_preamp[i]->reset();
    }
    
    m_cabinet->reset();
    
    // Reset rotor states
    m_hornRotor.angle = 0.0;
    m_hornRotor.velocity = 0.0;
    m_drumRotor.angle = 0.0;
    m_drumRotor.velocity = 0.0;
    
    updateRotorSpeeds();
}

void RotarySpeaker::process(juce::AudioBuffer<float>& buffer) {
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

void RotarySpeaker::processStereo(float* left, float* right, int numSamples) {
    // Process in blocks
    const int blockSize = 64;
    
    for (int offset = 0; offset < numSamples; offset += blockSize) {
        int samplesToProcess = std::min(blockSize, numSamples - offset);
        
        // Update rotor speeds once per block
        updateRotorSpeeds();
        
        for (int i = 0; i < samplesToProcess; ++i) {
            int idx = offset + i;
            
            // Get smoothed parameters
            float speed = m_speedParam->getNextValue();
            float mix = m_mixParam->getNextValue();
            float drive = m_driveParam->getNextValue();
            float micDistance = m_micDistanceParam->getNextValue() * 0.5f; // 0-0.5 meters
            float stereoWidth = m_stereoWidthParam->getNextValue() * M_PI * 0.25f; // 0-45 degrees
            
            // Store dry signal
            float dryL = left[idx];
            float dryR = right[idx];
            
            // Sanitize inputs
            float inL = sanitizeInput(left[idx]);
            float inR = sanitizeInput(right[idx]);
            
            // Apply tube preamp
            float preL = m_preamp[0]->process(inL, drive);
            float preR = m_preamp[1]->process(inR, drive);
            
            // Update rotor positions
            double deltaTime = 1.0 / m_sampleRate;
            m_hornRotor.update(deltaTime);
            m_drumRotor.update(deltaTime);
            
            // Calculate mic positions
            double micAngleL = -stereoWidth;
            double micAngleR = stereoWidth;
            
            // Process each channel
            float hornL, drumL, hornR, drumR;
            
            // Left channel
            m_crossover[0]->process(preL, drumL, hornL);
            
            // Apply Doppler effect
            hornL = m_hornDoppler[0]->process(hornL, m_hornRotor.angle, 
                                             m_hornRotor.velocity, HORN_RADIUS,
                                             micAngleL, micDistance);
            drumL = m_drumDoppler[0]->process(drumL, m_drumRotor.angle,
                                             m_drumRotor.velocity, DRUM_RADIUS,
                                             micAngleL, micDistance);
            
            // Apply amplitude modulation
            hornL = m_hornAM[0]->process(hornL, m_hornRotor.angle, micAngleL, 0.3);
            drumL = m_drumAM[0]->process(drumL, m_drumRotor.angle, micAngleL, 0.2);
            
            // Right channel
            m_crossover[1]->process(preR, drumR, hornR);
            
            hornR = m_hornDoppler[1]->process(hornR, m_hornRotor.angle,
                                             m_hornRotor.velocity, HORN_RADIUS,
                                             micAngleR, micDistance);
            drumR = m_drumDoppler[1]->process(drumR, m_drumRotor.angle,
                                             m_drumRotor.velocity, DRUM_RADIUS,
                                             micAngleR, micDistance);
            
            hornR = m_hornAM[1]->process(hornR, m_hornRotor.angle, micAngleR, 0.3);
            drumR = m_drumAM[1]->process(drumR, m_drumRotor.angle, micAngleR, 0.2);
            
            // Combine bands
            float wetL = hornL + drumL;
            float wetR = hornR + drumR;
            
            // Apply cabinet simulation (mono processing for realism)
            float cabinetMono = m_cabinet->process((wetL + wetR) * 0.5f);
            wetL = wetL * 0.7f + cabinetMono * 0.3f;
            wetR = wetR * 0.7f + cabinetMono * 0.3f;
            
            // Mix dry/wet
            left[idx] = dryL * (1.0f - mix) + wetL * mix;
            right[idx] = dryR * (1.0f - mix) + wetR * mix;
            
            // Soft limiting
            left[idx] = std::tanh(left[idx] * 0.8f) * 1.25f;
            right[idx] = std::tanh(right[idx] * 0.8f) * 1.25f;
        }
    }
}

void RotarySpeaker::updateRotorSpeeds() {
    float speed = m_speedParam->getCurrentValue();
    
    // Stop button functionality
    if (speed < 0.05f) {
        m_hornRotor.targetVelocity = 0.0;
        m_drumRotor.targetVelocity = 0.0;
        m_isStopped = true;
        return;
    }
    
    m_isStopped = false;
    
    // Speed ranges (in rad/s)
    // Chorale: Horn 0.66 Hz (40 rpm), Drum 0.5 Hz (30 rpm)
    // Tremolo: Horn 5.66 Hz (340 rpm), Drum 6.66 Hz (400 rpm)
    
    const double choraleHornSpeed = 0.66 * 2.0 * M_PI;
    const double tremaloHornSpeed = 5.66 * 2.0 * M_PI;
    const double choraleDrumSpeed = 0.5 * 2.0 * M_PI;
    const double tremaloDrumSpeed = 6.66 * 2.0 * M_PI;
    
    // Interpolate speeds
    double t = (speed - 0.05) / 0.95; // Normalize to 0-1 range
    t = std::max(0.0, std::min(1.0, t));
    
    m_hornRotor.targetVelocity = choraleHornSpeed + t * (tremaloHornSpeed - choraleHornSpeed);
    m_drumRotor.targetVelocity = choraleDrumSpeed + t * (tremaloDrumSpeed - choraleDrumSpeed);
    
    // Set acceleration based on current state
    double acceleration = 2.5 + t * 2.5; // 2.5 to 5.0 rad/s²
    m_hornRotor.acceleration = acceleration;
    m_drumRotor.acceleration = acceleration * 0.8; // Drum accelerates slightly slower
}

void RotarySpeaker::updateParameters(const std::map<int, float>& params) {
    auto it = params.find(0);
    if (it != params.end()) m_speedParam->setTargetValue(it->second);
    
    it = params.find(1);
    if (it != params.end()) {
        // Acceleration affects rotor inertia
        float accel = it->second;
        m_hornRotor.acceleration = 1.0 + accel * 9.0; // 1-10 rad/s²
        m_drumRotor.acceleration = m_hornRotor.acceleration * 0.8;
    }
    
    it = params.find(2);
    if (it != params.end()) m_driveParam->setTargetValue(it->second);
    
    it = params.find(3);
    if (it != params.end()) m_micDistanceParam->setTargetValue(it->second);
    
    it = params.find(4);
    if (it != params.end()) m_stereoWidthParam->setTargetValue(it->second);
    
    it = params.find(5);
    if (it != params.end()) m_mixParam->setTargetValue(it->second);
}

juce::String RotarySpeaker::getParameterName(int index) const {
    switch (index) {
        case 0: return "Speed";
        case 1: return "Acceleration";
        case 2: return "Drive";
        case 3: return "Mic Distance";
        case 4: return "Stereo Width";
        case 5: return "Mix";
        default: return "";
    }
}

// ==================== Component Implementations ====================

// CrossoverFilter implementation
void CrossoverFilter::updateCoefficients() noexcept {
    // 4th order Linkwitz-Riley = 2x 2nd order Butterworth
    double omega = 2.0 * M_PI * m_frequency / m_sampleRate;
    double cos_w = std::cos(omega);
    double sin_w = std::sin(omega);
    double sqrt2 = std::sqrt(2.0);
    
    // Butterworth Q values for LR4
    double q1 = 1.0 / std::sqrt(2.0);
    
    // First stage coefficients
    double alpha1 = sin_w / (2.0 * q1);
    double norm1 = 1.0 / (1.0 + alpha1);
    
    // Lowpass stage 1
    m_lowpassStages[0].b0 = (1.0 - cos_w) * 0.5 * norm1;
    m_lowpassStages[0].b1 = (1.0 - cos_w) * norm1;
    m_lowpassStages[0].b2 = m_lowpassStages[0].b0;
    m_lowpassStages[0].a1 = -2.0 * cos_w * norm1;
    m_lowpassStages[0].a2 = (1.0 - alpha1) * norm1;
    
    // Second stage (same for Butterworth)
    m_lowpassStages[1] = m_lowpassStages[0];
    
    // Highpass coefficients
    m_highpassStages[0].b0 = (1.0 + cos_w) * 0.5 * norm1;
    m_highpassStages[0].b1 = -(1.0 + cos_w) * norm1;
    m_highpassStages[0].b2 = m_highpassStages[0].b0;
    m_highpassStages[0].a1 = m_lowpassStages[0].a1;
    m_highpassStages[0].a2 = m_lowpassStages[0].a2;
    
    m_highpassStages[1] = m_highpassStages[0];
}

// DopplerProcessor implementation
float DopplerProcessor::process(float input, double rotorAngle, double rotorVelocity, 
              double rotorRadius, double micAngle, double micDistance) noexcept {
    // Write input to buffer
    m_buffer[m_writePos] = input + 1e-25f; // Denormal prevention
    m_writePos = (m_writePos + 1) & BUFFER_MASK;
    
    // Calculate Doppler delay
    double speakerX = rotorRadius * std::cos(rotorAngle);
    double speakerY = rotorRadius * std::sin(rotorAngle);
    double micX = micDistance * std::cos(micAngle);
    double micY = micDistance * std::sin(micAngle);
    
    // Distance from rotor to mic
    double dx = speakerX - micX;
    double dy = speakerY - micY;
    double distance = std::sqrt(dx * dx + dy * dy);
    
    // Radial velocity component (towards mic)
    double velocityX = -rotorRadius * rotorVelocity * std::sin(rotorAngle);
    double velocityY = rotorRadius * rotorVelocity * std::cos(rotorAngle);
    
    // Project velocity onto line from speaker to mic
    double distanceInv = 1.0 / (distance + 1e-10);
    double radialVelocity = (velocityX * dx + velocityY * dy) * distanceInv;
    
    // Doppler shift formula
    const double SPEED_OF_SOUND = 343.0;
    double dopplerRatio = 1.0 / (1.0 - radialVelocity / SPEED_OF_SOUND);
    
    // Convert to delay
    double baseDelay = distance / SPEED_OF_SOUND * m_sampleRate;
    double totalDelay = baseDelay * dopplerRatio;
    
    // Clamp and smooth delay changes
    totalDelay = std::max(1.0, std::min(totalDelay, BUFFER_SIZE - 4.0));
    
    // Smooth delay transitions to prevent clicks
    double smoothingFactor = 0.995;
    m_delaySmooth = totalDelay + (m_delaySmooth - totalDelay) * smoothingFactor;
    
    // Read with interpolation
    double readPos = static_cast<double>(m_writePos) - m_delaySmooth;
    while (readPos < 0) readPos += BUFFER_SIZE;
    
    return cubicInterpolate(readPos);
}

float DopplerProcessor::cubicInterpolate(double position) const noexcept {
    int idx = static_cast<int>(position);
    float frac = static_cast<float>(position - idx);
    
    // Get 4 points for cubic interpolation
    int i0 = (idx - 1) & BUFFER_MASK;
    int i1 = idx & BUFFER_MASK;
    int i2 = (idx + 1) & BUFFER_MASK;
    int i3 = (idx + 2) & BUFFER_MASK;
    
    float y0 = m_buffer[i0];
    float y1 = m_buffer[i1];
    float y2 = m_buffer[i2];
    float y3 = m_buffer[i3];
    
    // Catmull-Rom spline
    float a0 = y1;
    float a1 = 0.5f * (y2 - y0);
    float a2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float a3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    
    return a0 + a1 * frac + a2 * frac * frac + a3 * frac * frac * frac;
}

// AmplitudeModulator implementation
float AmplitudeModulator::process(float input, double rotorAngle, double micAngle, double depth) noexcept {
    // Calculate amplitude based on directivity pattern
    double angleDiff = rotorAngle - micAngle;
    
    // Cardioid pattern (more realistic than simple cosine)
    double pattern = 0.5 + 0.5 * std::cos(angleDiff);
    
    // Add some higher harmonics for realism
    pattern += 0.1 * std::cos(2.0 * angleDiff);
    
    // Apply depth control
    double modulation = 1.0 - depth * (1.0 - pattern);
    
    // Smooth to prevent clicks
    float target = static_cast<float>(modulation);
    m_smoothState = target + (m_smoothState - target) * 0.99f;
    
    return input * m_smoothState;
}

// TubePreamp implementation
void TubePreamp::reset() noexcept {
    m_dcBlockerIn.reset();
    m_dcBlockerOut.reset();
    m_toneFilter.reset();
}

void TubePreamp::setSampleRate(double sampleRate) noexcept {
    m_sampleRate = sampleRate;
    updateToneFilter();
}

float TubePreamp::process(float input, float drive) noexcept {
    // Input stage DC blocking
    float blocked = m_dcBlockerIn.process(input);
    
    // Apply drive
    float driven = blocked * (1.0f + drive * 4.0f);
    
    // Asymmetric tube saturation
    float saturated = tubeSaturate(driven);
    
    // Tone shaping (high frequency roll-off)
    float shaped = m_toneFilter.process(saturated);
    
    // Output DC blocking and gain compensation
    float output = m_dcBlockerOut.process(shaped);
    return output / (1.0f + drive * 2.0f);
}

float TubePreamp::DCBlocker::process(float input) noexcept {
    double in = static_cast<double>(input);
    double out = in - x1 + 0.995 * y1;
    x1 = in;
    y1 = out;
    return static_cast<float>(out);
}

float TubePreamp::ToneFilter::process(float input) noexcept {
    double in = static_cast<double>(input);
    double out = b0 * in + b1 * x1 - a1 * y1;
    x1 = in;
    y1 = out;
    return static_cast<float>(out + 1e-20);
}

float TubePreamp::tubeSaturate(float x) const noexcept {
    // Asymmetric saturation characteristic
    float sign = (x > 0.0f) ? 1.0f : -1.0f;
    float abs_x = std::abs(x);
    
    // Different curves for positive and negative
    if (x > 0.0f) {
        // Softer clipping for positive
        if (abs_x < 0.7f) {
            return x;
        } else if (abs_x < 2.0f) {
            float t = (abs_x - 0.7f) / 1.3f;
            return 0.7f + 0.3f * (1.0f - t * t);
        } else {
            return 1.0f - 0.02f / (abs_x - 1.0f);
        }
    } else {
        // Harder clipping for negative (tube asymmetry)
        if (abs_x < 0.5f) {
            return x;
        } else if (abs_x < 1.5f) {
            float t = (abs_x - 0.5f);
            return -0.5f - 0.4f * std::tanh(2.0f * t);
        } else {
            return -0.9f - 0.05f / (abs_x - 0.5f);
        }
    }
}

void TubePreamp::updateToneFilter() noexcept {
    // Simple one-pole lowpass at ~3kHz
    double freq = 3000.0;
    double omega = 2.0 * M_PI * freq / m_sampleRate;
    double a = std::exp(-omega);
    
    m_toneFilter.b0 = 1.0 - a;
    m_toneFilter.b1 = 0.0;
    m_toneFilter.a1 = -a;
}

// CabinetSimulator implementation
CabinetSimulator::CabinetSimulator() {
    reset();
}

void CabinetSimulator::setSampleRate(double sampleRate) noexcept {
    m_sampleRate = sampleRate;
    updateResonances();
}

void CabinetSimulator::reset() noexcept {
    for (auto& resonance : m_resonances) {
        resonance.reset();
    }
    m_reflectionDelay.reset();
}

float CabinetSimulator::process(float input) noexcept {
    float output = input;
    
    // Add cabinet resonances
    for (auto& resonance : m_resonances) {
        output += resonance.process(input) * 0.05f;
    }
    
    // Add early reflections
    output += m_reflectionDelay.process(output) * 0.15f;
    
    return output;
}

void CabinetSimulator::Resonance::reset() noexcept {
    x1 = x2 = y1 = y2 = 0.0;
}

void CabinetSimulator::Resonance::updateCoefficients(double freq, double sampleRate, double q) noexcept {
    frequency = freq;
    this->q = q;
    
    double omega = 2.0 * M_PI * frequency / sampleRate;
    double sin_w = std::sin(omega);
    double cos_w = std::cos(omega);
    double alpha = sin_w / (2.0 * q);
    
    // Bandpass resonator
    double norm = 1.0 / (1.0 + alpha);
    b0 = alpha * norm;
    b1 = 0.0;
    b2 = -alpha * norm;
    a1 = -2.0 * cos_w * norm;
    a2 = (1.0 - alpha) * norm;
}

float CabinetSimulator::Resonance::process(float input) noexcept {
    double in = static_cast<double>(input);
    double out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    
    x2 = x1; x1 = in;
    y2 = y1; y1 = out;
    
    return static_cast<float>(out);
}

void CabinetSimulator::SimpleDelay::reset() noexcept {
    buffer.fill(0.0f);
    writePos = 0;
}

float CabinetSimulator::SimpleDelay::process(float input) noexcept {
    buffer[writePos] = input;
    size_t readPos = (writePos + MAX_DELAY - delayLength) % MAX_DELAY;
    writePos = (writePos + 1) % MAX_DELAY;
    return buffer[readPos];
}

void CabinetSimulator::updateResonances() noexcept {
    // Characteristic Leslie cabinet resonances
    m_resonances[0].updateCoefficients(97.0, m_sampleRate, 12.0);   // Low resonance
    m_resonances[1].updateCoefficients(185.0, m_sampleRate, 10.0);  // Low-mid
    m_resonances[2].updateCoefficients(380.0, m_sampleRate, 8.0);   // Mid
    m_resonances[3].updateCoefficients(760.0, m_sampleRate, 6.0);   // Upper-mid
    
    // Set reflection delay (~2.3ms)
    m_reflectionDelay.delayLength = static_cast<size_t>(0.0023 * m_sampleRate);
}

// ParameterSmoother implementation
void ParameterSmoother::setSampleRate(double sampleRate) noexcept {
    m_sampleRate = sampleRate;
}

void ParameterSmoother::setSmoothingTime(float milliseconds) noexcept {
    m_rampLengthSamples = static_cast<int>(milliseconds * 0.001 * m_sampleRate);
    m_rampLengthSamples = std::max(1, m_rampLengthSamples);
}

void ParameterSmoother::setTargetValue(float newTarget) noexcept {
    if (std::abs(newTarget - m_target) < 1e-8f) {
        m_current = m_target = newTarget;
        m_stepsRemaining = 0;
        return;
    }
    
    m_target = newTarget;
    m_stepSize = (m_target - m_current) / static_cast<float>(m_rampLengthSamples);
    m_stepsRemaining = m_rampLengthSamples;
}

float ParameterSmoother::getNextValue() noexcept {
    if (m_stepsRemaining > 0) {
        m_current += m_stepSize;
        --m_stepsRemaining;
        
        if (m_stepsRemaining == 0) {
            m_current = m_target;
        }
    }
    
    return m_current + 1e-15f;
}

void ParameterSmoother::reset(float value) noexcept {
    m_current = m_target = value;
    m_stepsRemaining = 0;
}

} // namespace AudioDSP