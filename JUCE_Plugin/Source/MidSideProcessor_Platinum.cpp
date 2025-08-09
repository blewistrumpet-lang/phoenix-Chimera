#include "JuceHeader.h"
#include "MidSideProcessor_Platinum.h"
#include "DspEngineUtilities.h"
#include <cmath>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Use denormal prevention from DspEngineUtilities

//==============================================================================
// Private Implementation
//==============================================================================
struct MidSideProcessor_Platinum::Impl {
    // Sample rate and processing info
    float sampleRate = 44100.0f;
    int samplesPerBlock = 512;
    
    // Thread-safe parameters with smoothing
    struct SmoothParam {
        std::atomic<float> target{0.5f};
        float current = 0.5f;
        float smooth = 0.995f;
        
        void setSmoothingTime(float milliseconds, float sr) {
            float freq = 1000.0f / (2.0f * M_PI * milliseconds);
            smooth = std::exp(-2.0f * M_PI * freq / sr);
        }
        
        float getNext() {
            float t = target.load(std::memory_order_relaxed);
            current = t + (current - t) * smooth;
            current = DSPUtils::flushDenorm(current);
            return current;
        }
        
        void reset(float value) {
            target.store(value, std::memory_order_relaxed);
            current = value;
        }
    };
    
    // Parameters
    SmoothParam midGain;
    SmoothParam sideGain;
    SmoothParam width;
    SmoothParam midLow;
    SmoothParam midHigh;
    SmoothParam sideLow;
    SmoothParam sideHigh;
    SmoothParam bassMono;
    SmoothParam soloMode;
    SmoothParam presence;
    
    // High-precision shelving filter
    struct ShelfFilter {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;
        
        void calculateLowShelf(double freq, double gain, double sr) {
            double omega = 2.0 * M_PI * freq / sr;
            double sinw = std::sin(omega);
            double cosw = std::cos(omega);
            double A = std::pow(10.0, gain / 40.0);
            double beta = std::sqrt(A) / 0.7071; // Q = 0.7071 for Butterworth
            
            b0 = A * ((A + 1.0) - (A - 1.0) * cosw + beta * sinw);
            b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw);
            b2 = A * ((A + 1.0) - (A - 1.0) * cosw - beta * sinw);
            double a0 = (A + 1.0) + (A - 1.0) * cosw + beta * sinw;
            a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosw) / a0;
            a2 = ((A + 1.0) + (A - 1.0) * cosw - beta * sinw) / a0;
            
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
        }
        
        void calculateHighShelf(double freq, double gain, double sr) {
            double omega = 2.0 * M_PI * freq / sr;
            double sinw = std::sin(omega);
            double cosw = std::cos(omega);
            double A = std::pow(10.0, gain / 40.0);
            double beta = std::sqrt(A) / 0.7071;
            
            b0 = A * ((A + 1.0) + (A - 1.0) * cosw + beta * sinw);
            b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw);
            b2 = A * ((A + 1.0) + (A - 1.0) * cosw - beta * sinw);
            double a0 = (A + 1.0) - (A - 1.0) * cosw + beta * sinw;
            a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosw) / a0;
            a2 = ((A + 1.0) - (A - 1.0) * cosw - beta * sinw) / a0;
            
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
        }
        
        double process(double input) {
            double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = input;
            y2 = y1;
            y1 = output;
            
            output = DSPUtils::flushDenorm(output);
            y1 = DSPUtils::flushDenorm(y1);
            y2 = DSPUtils::flushDenorm(y2);
            
            return output;
        }
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0;
        }
    };
    
    // Elliptical filter for bass mono (4th order)
    struct EllipticalFilter {
        ShelfFilter stage1, stage2;
        bool bypassed = true;
        
        void setFrequency(double freq, double sr) {
            if (freq < 20.0) {
                bypassed = true;
            } else {
                bypassed = false;
                // 4th order Butterworth highpass
                double q = 0.5412; // Butterworth Q for 2nd order sections
                calculateHighpass(freq, q, sr);
            }
        }
        
        void calculateHighpass(double freq, double q, double sr) {
            double omega = 2.0 * M_PI * freq / sr;
            double sinw = std::sin(omega);
            double cosw = std::cos(omega);
            double alpha = sinw / (2.0 * q);
            
            stage1.b0 = (1.0 + cosw) / 2.0;
            stage1.b1 = -(1.0 + cosw);
            stage1.b2 = (1.0 + cosw) / 2.0;
            double a0 = 1.0 + alpha;
            stage1.a1 = -2.0 * cosw / a0;
            stage1.a2 = (1.0 - alpha) / a0;
            
            stage1.b0 /= a0;
            stage1.b1 /= a0;
            stage1.b2 /= a0;
            
            // Copy to stage2
            stage2 = stage1;
        }
        
        double process(double input) {
            if (bypassed) return input;
            return stage2.process(stage1.process(input));
        }
        
        void reset() {
            stage1.reset();
            stage2.reset();
        }
    };
    
    // Presence filter (bell at 10kHz)
    struct PresenceFilter {
        ShelfFilter filter;
        bool active = false;
        
        void setGain(double gain, double sr) {
            if (gain < 0.1) {
                active = false;
            } else {
                active = true;
                calculateBell(10000.0, gain * 6.0, 2.0, sr);
            }
        }
        
        void calculateBell(double freq, double gain, double q, double sr) {
            double omega = 2.0 * M_PI * freq / sr;
            double sinw = std::sin(omega);
            double cosw = std::cos(omega);
            double A = std::pow(10.0, gain / 40.0);
            double alpha = sinw / (2.0 * q);
            
            filter.b0 = 1.0 + alpha * A;
            filter.b1 = -2.0 * cosw;
            filter.b2 = 1.0 - alpha * A;
            double a0 = 1.0 + alpha / A;
            filter.a1 = -2.0 * cosw / a0;
            filter.a2 = (1.0 - alpha / A) / a0;
            
            filter.b0 /= a0;
            filter.b1 /= a0;
            filter.b2 /= a0;
        }
        
        double process(double input) {
            return active ? filter.process(input) : input;
        }
        
        void reset() {
            filter.reset();
        }
    };
    
    // EQ filters
    ShelfFilter midLowShelf, midHighShelf;
    ShelfFilter sideLowShelf, sideHighShelf;
    EllipticalFilter bassMonoFilterL, bassMonoFilterR;
    PresenceFilter presenceFilter;
    
    // Metering
    struct Metering {
        float midRMS = 0.0f;
        float sideRMS = 0.0f;
        float correlation = 0.0f;
        float balance = 0.0f;
        
        // RMS detectors
        float midSquareSum = 0.0f;
        float sideSquareSum = 0.0f;
        float leftSquareSum = 0.0f;
        float rightSquareSum = 0.0f;
        int sampleCount = 0;
        static constexpr int RMS_WINDOW = 2048;
        
        void process(float mid, float side, float left, float right) {
            midSquareSum += mid * mid;
            sideSquareSum += side * side;
            leftSquareSum += left * left;
            rightSquareSum += right * right;
            
            if (++sampleCount >= RMS_WINDOW) {
                midRMS = std::sqrt(midSquareSum / RMS_WINDOW);
                sideRMS = std::sqrt(sideSquareSum / RMS_WINDOW);
                float leftRMS = std::sqrt(leftSquareSum / RMS_WINDOW);
                float rightRMS = std::sqrt(rightSquareSum / RMS_WINDOW);
                
                // Balance calculation
                float totalRMS = leftRMS + rightRMS;
                if (totalRMS > 0.0001f) {
                    balance = (rightRMS - leftRMS) / totalRMS;
                }
                
                // Phase correlation (simplified)
                correlation = 1.0f - (sideRMS / (midRMS + 0.0001f));
                correlation = std::max(-1.0f, std::min(1.0f, correlation));
                
                // Reset accumulators
                midSquareSum = sideSquareSum = 0.0f;
                leftSquareSum = rightSquareSum = 0.0f;
                sampleCount = 0;
            }
        }
    };
    
    Metering metering;
    
    // Mid-Side matrix operations
    void encodeMidSide(float left, float right, float& mid, float& side) {
        const float scale = 0.7071f; // 1/sqrt(2)
        mid = (left + right) * scale;
        side = (left - right) * scale;
    }
    
    void decodeMidSide(float mid, float side, float& left, float& right) {
        const float scale = 0.7071f;
        left = (mid + side) * scale;
        right = (mid - side) * scale;
    }
    
    // Constructor
    Impl() {
        // Set default smoothing
        const float smoothTime = 20.0f; // milliseconds
        midGain.setSmoothingTime(smoothTime, sampleRate);
        sideGain.setSmoothingTime(smoothTime, sampleRate);
        width.setSmoothingTime(smoothTime, sampleRate);
        midLow.setSmoothingTime(smoothTime, sampleRate);
        midHigh.setSmoothingTime(smoothTime, sampleRate);
        sideLow.setSmoothingTime(smoothTime, sampleRate);
        sideHigh.setSmoothingTime(smoothTime, sampleRate);
        bassMono.setSmoothingTime(smoothTime, sampleRate);
        soloMode.setSmoothingTime(smoothTime, sampleRate);
        presence.setSmoothingTime(smoothTime, sampleRate);
        
        reset();
    }
    
    void reset() {
        midLowShelf.reset();
        midHighShelf.reset();
        sideLowShelf.reset();
        sideHighShelf.reset();
        bassMonoFilterL.reset();
        bassMonoFilterR.reset();
        presenceFilter.reset();
        
        // Reset parameters
        midGain.reset(0.5f);     // 0dB
        sideGain.reset(0.5f);     // 0dB
        width.reset(0.5f);        // 100%
        midLow.reset(0.5f);       // 0dB
        midHigh.reset(0.5f);      // 0dB
        sideLow.reset(0.5f);      // 0dB
        sideHigh.reset(0.5f);     // 0dB
        bassMono.reset(0.0f);     // Off
        soloMode.reset(0.0f);     // Off
        presence.reset(0.0f);     // Off
    }
    
    SoloMode getCurrentSoloMode() const {
        float solo = soloMode.current;
        if (solo < 0.2f) return SoloMode::OFF;
        else if (solo < 0.5f) return SoloMode::MID_ONLY;
        else return SoloMode::SIDE_ONLY;
    }
    
    float getWidthPercentage() const {
        return width.current * 200.0f; // 0-200%
    }
    
    StereoMetering getMetering() const {
        StereoMetering result;
        result.midLevel = metering.midRMS;
        result.sideLevel = metering.sideRMS;
        result.correlation = metering.correlation;
        result.balance = metering.balance;
        return result;
    }
};

//==============================================================================
// Public Implementation
//==============================================================================
MidSideProcessor_Platinum::MidSideProcessor_Platinum()
    : pimpl(std::make_unique<Impl>()) {
}

MidSideProcessor_Platinum::~MidSideProcessor_Platinum() = default;

void MidSideProcessor_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->sampleRate = static_cast<float>(sampleRate);
    pimpl->samplesPerBlock = samplesPerBlock;
    
    // Update smoothing times
    const float smoothTime = 20.0f;
    pimpl->midGain.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->sideGain.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->width.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->midLow.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->midHigh.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->sideLow.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->sideHigh.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->bassMono.setSmoothingTime(smoothTime, pimpl->sampleRate);
    pimpl->soloMode.setSmoothingTime(0.0f, pimpl->sampleRate); // No smoothing for solo
    pimpl->presence.setSmoothingTime(smoothTime, pimpl->sampleRate);
    
    reset();
}

void MidSideProcessor_Platinum::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Require stereo input
    if (numChannels < 2 || numSamples == 0) return;
    
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample) {
        // Get current parameters
        float midGainValue = pimpl->midGain.getNext();
        float sideGainValue = pimpl->sideGain.getNext();
        float widthValue = pimpl->width.getNext();
        float bassMonoFreq = pimpl->bassMono.getNext() * 500.0f; // 0-500Hz
        
        // Convert gains from normalized to linear
        float midGainLinear = std::pow(10.0f, (midGainValue - 0.5f) * 40.0f / 20.0f); // ±20dB
        float sideGainLinear = std::pow(10.0f, (sideGainValue - 0.5f) * 40.0f / 20.0f);
        
        // Update EQ settings (only when changed)
        static float lastMidLow = -1.0f, lastMidHigh = -1.0f;
        static float lastSideLow = -1.0f, lastSideHigh = -1.0f;
        static float lastBassMono = -1.0f, lastPresence = -1.0f;
        
        float midLowValue = pimpl->midLow.getNext();
        float midHighValue = pimpl->midHigh.getNext();
        float sideLowValue = pimpl->sideLow.getNext();
        float sideHighValue = pimpl->sideHigh.getNext();
        float presenceValue = pimpl->presence.getNext();
        
        const float threshold = 0.01f;
        
        if (std::abs(midLowValue - lastMidLow) > threshold) {
            pimpl->midLowShelf.calculateLowShelf(200.0, (midLowValue - 0.5f) * 30.0f, pimpl->sampleRate);
            lastMidLow = midLowValue;
        }
        
        if (std::abs(midHighValue - lastMidHigh) > threshold) {
            pimpl->midHighShelf.calculateHighShelf(5000.0, (midHighValue - 0.5f) * 30.0f, pimpl->sampleRate);
            lastMidHigh = midHighValue;
        }
        
        if (std::abs(sideLowValue - lastSideLow) > threshold) {
            pimpl->sideLowShelf.calculateLowShelf(200.0, (sideLowValue - 0.5f) * 30.0f, pimpl->sampleRate);
            lastSideLow = sideLowValue;
        }
        
        if (std::abs(sideHighValue - lastSideHigh) > threshold) {
            pimpl->sideHighShelf.calculateHighShelf(5000.0, (sideHighValue - 0.5f) * 30.0f, pimpl->sampleRate);
            lastSideHigh = sideHighValue;
        }
        
        if (std::abs(bassMonoFreq - lastBassMono) > threshold) {
            pimpl->bassMonoFilterL.setFrequency(bassMonoFreq, pimpl->sampleRate);
            pimpl->bassMonoFilterR.setFrequency(bassMonoFreq, pimpl->sampleRate);
            lastBassMono = bassMonoFreq;
        }
        
        if (std::abs(presenceValue - lastPresence) > threshold) {
            pimpl->presenceFilter.setGain(presenceValue, pimpl->sampleRate);
            lastPresence = presenceValue;
        }
        
        // Get input samples
        float left = leftChannel[sample];
        float right = rightChannel[sample];
        
        // Apply bass mono if active
        if (bassMonoFreq > 20.0f) {
            float leftHigh = pimpl->bassMonoFilterL.process(left);
            float rightHigh = pimpl->bassMonoFilterR.process(right);
            float mono = (left + right) * 0.5f;
            float monoLow = mono - (leftHigh + rightHigh) * 0.5f;
            left = monoLow + leftHigh;
            right = monoLow + rightHigh;
        }
        
        // Encode to mid-side
        float mid, side;
        pimpl->encodeMidSide(left, right, mid, side);
        
        // Apply EQ to mid channel
        mid = static_cast<float>(pimpl->midLowShelf.process(mid));
        mid = static_cast<float>(pimpl->midHighShelf.process(mid));
        
        // Apply EQ to side channel
        side = static_cast<float>(pimpl->sideLowShelf.process(side));
        side = static_cast<float>(pimpl->sideHighShelf.process(side));
        
        // Apply gains
        mid *= midGainLinear;
        side *= sideGainLinear;
        
        // Apply width control
        side *= widthValue * 2.0f; // 0-200%
        
        // Solo monitoring
        SoloMode solo = pimpl->getCurrentSoloMode();
        if (solo == SoloMode::MID_ONLY) {
            side = 0.0f;
        } else if (solo == SoloMode::SIDE_ONLY) {
            mid = 0.0f;
        }
        
        // Decode back to left-right
        pimpl->decodeMidSide(mid, side, left, right);
        
        // Apply presence boost if active
        if (presenceValue > 0.01f) {
            float presence = static_cast<float>(pimpl->presenceFilter.process((left + right) * 0.5f));
            float presenceGain = presenceValue * 0.5f;
            left += (presence - (left + right) * 0.5f) * presenceGain;
            right += (presence - (left + right) * 0.5f) * presenceGain;
        }
        
        // Update metering
        pimpl->metering.process(mid, side, left, right);
        
        // Write output
        leftChannel[sample] = left;
        rightChannel[sample] = right;
    }
    
    // Apply final NaN/Inf cleanup
    scrubBuffer(buffer);
}

void MidSideProcessor_Platinum::reset() {
    pimpl->reset();
}

void MidSideProcessor_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (index) {
            case MID_GAIN:
                pimpl->midGain.target.store(value, std::memory_order_relaxed);
                break;
            case SIDE_GAIN:
                pimpl->sideGain.target.store(value, std::memory_order_relaxed);
                break;
            case WIDTH:
                pimpl->width.target.store(value, std::memory_order_relaxed);
                break;
            case MID_LOW:
                pimpl->midLow.target.store(value, std::memory_order_relaxed);
                break;
            case MID_HIGH:
                pimpl->midHigh.target.store(value, std::memory_order_relaxed);
                break;
            case SIDE_LOW:
                pimpl->sideLow.target.store(value, std::memory_order_relaxed);
                break;
            case SIDE_HIGH:
                pimpl->sideHigh.target.store(value, std::memory_order_relaxed);
                break;
            case BASS_MONO:
                pimpl->bassMono.target.store(value, std::memory_order_relaxed);
                break;
            case SOLO_MODE:
                pimpl->soloMode.target.store(value, std::memory_order_relaxed);
                break;
            case PRESENCE:
                pimpl->presence.target.store(value, std::memory_order_relaxed);
                break;
        }
    }
}

juce::String MidSideProcessor_Platinum::getParameterName(int index) const {
    switch (index) {
        case MID_GAIN: return "Mid Gain";
        case SIDE_GAIN: return "Side Gain";
        case WIDTH: return "Width";
        case MID_LOW: return "Mid Low";
        case MID_HIGH: return "Mid High";
        case SIDE_LOW: return "Side Low";
        case SIDE_HIGH: return "Side High";
        case BASS_MONO: return "Bass Mono";
        case SOLO_MODE: return "Solo Mode";
        case PRESENCE: return "Presence";
        default: return "";
    }
}

MidSideProcessor_Platinum::SoloMode MidSideProcessor_Platinum::getCurrentSoloMode() const noexcept {
    return static_cast<SoloMode>(pimpl->getCurrentSoloMode());
}

float MidSideProcessor_Platinum::getWidthPercentage() const noexcept {
    return pimpl->getWidthPercentage();
}

MidSideProcessor_Platinum::StereoMetering MidSideProcessor_Platinum::getMetering() const noexcept {
    return pimpl->getMetering();
}