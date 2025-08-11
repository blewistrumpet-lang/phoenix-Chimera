#include "VintageConsoleEQ_Studio.h"
#include <cstring>
#include <cmath>

// ---------- Static filter helpers ----------

void VintageConsoleEQ_Studio::bellOrfanidis(BiquadCoeffs& out, double fs, double f0, double Q, double gainDB) {
    const double A = std::pow(10.0, gainDB * 0.025); // sqrt for bell
    const double w0 = 2.0 * M_PI * f0 / fs;
    const double sinw = std::sin(w0);
    const double cosw = std::cos(w0);
    const double alpha = sinw / (2.0 * Q);
    
    const double b0 = 1.0 + alpha * A;
    const double b1 = -2.0 * cosw;
    const double b2 = 1.0 - alpha * A;
    const double a0 = 1.0 + alpha / A;
    const double a1 = -2.0 * cosw;
    const double a2 = 1.0 - alpha / A;
    
    const double ia0 = 1.0 / a0;
    out.b0 = (float)(b0 * ia0);
    out.b1 = (float)(b1 * ia0);
    out.b2 = (float)(b2 * ia0);
    out.a1 = (float)(a1 * ia0);
    out.a2 = (float)(a2 * ia0);
}

void VintageConsoleEQ_Studio::shelfOrfanidis(BiquadCoeffs& out, double fs, double f0, double slope, double gainDB, bool highShelf) {
    const double A = std::pow(10.0, gainDB * 0.05);
    const double w0 = 2.0 * M_PI * f0 / fs;
    const double sinw = std::sin(w0);
    const double cosw = std::cos(w0);
    const double S = slope;
    const double beta = std::sqrt(A) / S;
    
    double b0, b1, b2, a0, a1, a2;
    
    if (highShelf) {
        b0 = A * ((A + 1) + (A - 1) * cosw + beta * sinw);
        b1 = -2 * A * ((A - 1) + (A + 1) * cosw);
        b2 = A * ((A + 1) + (A - 1) * cosw - beta * sinw);
        a0 = (A + 1) - (A - 1) * cosw + beta * sinw;
        a1 = 2 * ((A - 1) - (A + 1) * cosw);
        a2 = (A + 1) - (A - 1) * cosw - beta * sinw;
    } else {
        b0 = A * ((A + 1) - (A - 1) * cosw + beta * sinw);
        b1 = 2 * A * ((A - 1) - (A + 1) * cosw);
        b2 = A * ((A + 1) - (A - 1) * cosw - beta * sinw);
        a0 = (A + 1) + (A - 1) * cosw + beta * sinw;
        a1 = -2 * ((A - 1) + (A + 1) * cosw);
        a2 = (A + 1) + (A - 1) * cosw - beta * sinw;
    }
    
    const double ia0 = 1.0 / a0;
    out.b0 = (float)(b0 * ia0);
    out.b1 = (float)(b1 * ia0);
    out.b2 = (float)(b2 * ia0);
    out.a1 = (float)(a1 * ia0);
    out.a2 = (float)(a2 * ia0);
}

float VintageConsoleEQ_Studio::magAtW(const BiquadCoeffs& c, float w) {
    const std::complex<float> z(std::cos(w), std::sin(w));
    const std::complex<float> num = c.b0 + c.b1 * std::conj(z) + c.b2 * std::conj(z) * std::conj(z);
    const std::complex<float> den = 1.0f + c.a1 * std::conj(z) + c.a2 * std::conj(z) * std::conj(z);
    return std::abs(num / den);
}

// ---------- Constructor / Destructor ----------

VintageConsoleEQ_Studio::VintageConsoleEQ_Studio() {
    loadCenters();
}

VintageConsoleEQ_Studio::~VintageConsoleEQ_Studio() = default;

// ---------- EngineBase overrides ----------

void VintageConsoleEQ_Studio::prepareToPlay(double sampleRate, int samplesPerBlock) {
    fs_ = sampleRate;
    blockSize_ = samplesPerBlock;
    
    // Initialize bands
    bands_[LOW].isShelf = true;
    bands_[HIGH].isShelf = true;
    bands_[LM].isShelf = false;
    bands_[HM].isShelf = false;
    
    // Reset all filters
    for (int b = 0; b < NBANDS; ++b) {
        for (int ch = 0; ch < kMaxChannels; ++ch) {
            bands_[b].filt[ch].reset();
        }
    }
    
    // Initialize nonlinear stages
    xform_.reset();
    inductor_.reset();
    
    // DC blockers
    for (int ch = 0; ch < kMaxChannels; ++ch) {
        dc_[ch].reset();
    }
    
    // Halfband
    hb_.reset();
    
    ctrlPhase_ = 0;
    noiseSeed_ = 0x13579BDFu;
}

void VintageConsoleEQ_Studio::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int nCh = std::min(buffer.getNumChannels(), (int)kMaxChannels);
    const int N = buffer.getNumSamples();
    
    if (bypass_) {
        scrubBuffer(buffer);
        return;
    }
    
    // Control rate update
    if (ctrlPhase_ == 0) {
        controlTick();
        ctrlPhase_ = kCtrlInterval;
    }
    
    // Determine if oversampling needed
    const bool needOS = (osMode_ == 1) || (osMode_ == 0 && fs_ < 96000.0);
    
    // Process audio
    for (int ch = 0; ch < nCh; ++ch) {
        float* data = buffer.getWritePointer(ch);
        
        for (int n = 0; n < N; ++n) {
            // Update control phase
            if (--ctrlPhase_ <= 0) {
                controlTick();
                ctrlPhase_ = kCtrlInterval;
            }
            
            float x = data[n];
            
            // Input drive/transformer stage
            if (drive_ > 0.01f) {
                x = xform_.process(x, 1000.0f, (float)fs_); // Assume 1kHz for now
            }
            
            // EQ bands (with coupling)
            float bandOuts[NBANDS];
            for (int b = 0; b < NBANDS; ++b) {
                bandOuts[b] = bands_[b].filt[ch].process(x) * dbToLin(gainsEffective_[b]);
            }
            
            // Mix bands
            float y = x;
            for (int b = 0; b < NBANDS; ++b) {
                y += (bandOuts[b] - x) * 0.25f; // Partial mix to avoid excessive gain
            }
            
            // Inductor resonance (subtle)
            if (drive_ > 0.01f) {
                y = inductor_.process(ch, y, 2000.0f, (float)fs_, drive_ * 0.3f);
            }
            
            // Output trim
            y *= dbToLin(outputTrim_);
            
            // Vintage noise
            if (noiseOn_) {
                noiseSeed_ = noiseSeed_ * 1664525u + 1013904223u;
                float noise = ((noiseSeed_ >> 16) & 0x7fff) * (1.0f / 32768.0f) - 1.0f;
                y += noise * 0.00001f; // Very subtle
            }
            
            // DC blocking
            y = dc_[ch].process(y);
            
            data[n] = y;
        }
    }
    
    scrubBuffer(buffer);
}

void VintageConsoleEQ_Studio::reset() {
    for (int b = 0; b < NBANDS; ++b) {
        bands_[b].haveLast = false;
        for (int ch = 0; ch < kMaxChannels; ++ch) {
            bands_[b].filt[ch].reset();
        }
    }
    
    xform_.reset();
    inductor_.reset();
    
    for (int ch = 0; ch < kMaxChannels; ++ch) {
        dc_[ch].reset();
    }
    
    hb_.reset();
    ctrlPhase_ = 0;
}

void VintageConsoleEQ_Studio::updateParameters(const std::map<int, float>& params) {
    using namespace Dsp;
    
    // Extract parameters
    auto getParam = [&params](int id, float defaultVal) -> float {
        auto it = params.find(id);
        return (it != params.end()) ? it->second : defaultVal;
    };
    
    bypass_ = getParam(kBypass, 0.0f) > 0.5f;
    outputTrim_ = getParam(kOutputTrim_dB, 0.0f);
    drive_ = getParam(kDrive, 0.0f);
    osMode_ = (int)getParam(kOSOn, 0.0f);
    noiseOn_ = getParam(kNoiseOn, 0.0f) > 0.5f;
    
    // Console type
    int consoleType = (int)getParam(kConsoleType, 0.0f);
    console_ = (ConsoleType)std::clamp(consoleType, 0, 3);
    
    // Band indices
    bands_[LOW].idx = (int)getParam(kLow_Index, 0.0f);
    bands_[LM].idx = (int)getParam(kLM_Index, 0.0f);
    bands_[HM].idx = (int)getParam(kHM_Index, 0.0f);
    bands_[HIGH].idx = (int)getParam(kHigh_Index, 0.0f);
    
    // Band gains
    bands_[LOW].gainDB = getParam(kLow_Gain_dB, 0.0f);
    bands_[LM].gainDB = getParam(kLM_Gain_dB, 0.0f);
    bands_[HM].gainDB = getParam(kHM_Gain_dB, 0.0f);
    bands_[HIGH].gainDB = getParam(kHigh_Gain_dB, 0.0f);
    
    // Q bias
    float qBias = getParam(kQBias, 0.5f);
    for (int b = 0; b < NBANDS; ++b) {
        bands_[b].Qbias = qBias;
    }
}

juce::String VintageConsoleEQ_Studio::getParameterName(int index) const {
    switch (index) {
        case kBypass: return "Bypass";
        case kOutputTrim_dB: return "Output Trim";
        case kDrive: return "Drive";
        case kOSOn: return "Oversampling";
        case kNoiseOn: return "Vintage Noise";
        case kConsoleType: return "Console Type";
        case kLow_Index: return "Low Freq";
        case kLM_Index: return "Low Mid Freq";
        case kHM_Index: return "High Mid Freq";
        case kHigh_Index: return "High Freq";
        case kLow_Gain_dB: return "Low Gain";
        case kLM_Gain_dB: return "Low Mid Gain";
        case kHM_Gain_dB: return "High Mid Gain";
        case kHigh_Gain_dB: return "High Gain";
        case kQBias: return "Q Character";
        default: return "Param " + juce::String(index);
    }
}

void VintageConsoleEQ_Studio::selectConsole(ConsoleType type) {
    console_ = type;
    loadCenters();
}

// ---------- Private methods ----------

void VintageConsoleEQ_Studio::controlTick() {
    // Apply inter-band coupling
    float rawGains[NBANDS] = {
        bands_[LOW].gainDB,
        bands_[LM].gainDB,
        bands_[HM].gainDB,
        bands_[HIGH].gainDB
    };
    
    coupling_.apply(rawGains, gainsEffective_);
    
    // Update filter coefficients if needed
    for (int b = 0; b < NBANDS; ++b) {
        float centerHz = 1000.0f; // Default
        
        // Get frequency from stepped centers
        switch (b) {
            case LOW:
                if (bands_[b].idx < lowCount_) {
                    centerHz = lowCenters_[bands_[b].idx];
                }
                break;
            case LM:
                if (bands_[b].idx < lmCount_) {
                    centerHz = lmCenters_[bands_[b].idx];
                }
                break;
            case HM:
                if (bands_[b].idx < hmCount_) {
                    centerHz = hmCenters_[bands_[b].idx];
                }
                break;
            case HIGH:
                if (bands_[b].idx < highCount_) {
                    centerHz = highCenters_[bands_[b].idx];
                }
                break;
        }
        
        rebuildBandsIfNeeded(0, (Band)b, centerHz, bands_[b].isShelf);
    }
}

void VintageConsoleEQ_Studio::rebuildBandsIfNeeded(int ch, Band b, float centerHz, bool isShelf) {
    BandState& band = bands_[b];
    
    // Calculate Q from gain and console type
    float Q = propQ(band.gainDB, console_, band.Qbias);
    
    BiquadCoeffs newCoeffs;
    
    if (isShelf) {
        float slope = (console_ == ConsoleType::API_550A) ? 1.2f : 0.8f;
        shelfOrfanidis(newCoeffs, fs_, centerHz, slope, band.gainDB, (b == HIGH));
    } else {
        bellOrfanidis(newCoeffs, fs_, centerHz, Q, band.gainDB);
    }
    
    // Power-compensated crossfade
    if (band.haveLast) {
        float oldMag = magAtW(band.last, (float)(M_PI * centerHz / fs_));
        float newMag = magAtW(newCoeffs, (float)(M_PI * centerHz / fs_));
        
        XfadeGain g;
        g.set(oldMag, newMag);
        
        // Start crossfade
        for (int c = 0; c < kMaxChannels; ++c) {
            auto& f = band.filt[c];
            f.useA = !f.useA;
            if (f.useA) {
                f.A.c = newCoeffs;
            } else {
                f.B.c = newCoeffs;
            }
            f.xfadeCtr = kXfadeSamples;
            f.g = g;
        }
    } else {
        // Initial set
        for (int c = 0; c < kMaxChannels; ++c) {
            band.filt[c].A.c = newCoeffs;
            band.filt[c].B.c = newCoeffs;
        }
    }
    
    band.last = newCoeffs;
    band.haveLast = true;
}

void VintageConsoleEQ_Studio::loadCenters() {
    // Load frequency centers based on console type
    switch (console_) {
        case ConsoleType::NEVE_1073:
            // Neve 1073 stepped frequencies
            lowCenters_ = {35, 60, 110, 220};
            lowCount_ = 4;
            
            lmCenters_ = {360, 700, 1600, 3200, 4800, 7200};
            lmCount_ = 6;
            
            hmCenters_ = {1500, 3000, 4500, 6000, 8000};
            hmCount_ = 5;
            
            highCenters_ = {10000, 12000, 16000};
            highCount_ = 3;
            break;
            
        case ConsoleType::SSL_4000E:
            // SSL 4000E frequencies
            lowCenters_ = {30, 40, 60, 80, 100, 150, 200};
            lowCount_ = 7;
            
            lmCenters_ = {250, 500, 1000, 2000, 4000};
            lmCount_ = 5;
            
            hmCenters_ = {1500, 3000, 5000, 7000, 9000};
            hmCount_ = 5;
            
            highCenters_ = {8000, 10000, 12000, 16000, 20000};
            highCount_ = 5;
            break;
            
        case ConsoleType::API_550A:
            // API 550A frequencies
            lowCenters_ = {30, 40, 50, 100, 200, 300, 400};
            lowCount_ = 7;
            
            lmCenters_ = {200, 400, 600, 800, 1500, 3000, 5000};
            lmCount_ = 7;
            
            hmCenters_ = {800, 1500, 3000, 5000, 8000};
            hmCount_ = 5;
            
            highCenters_ = {5000, 7500, 10000, 12500, 15000, 20000};
            highCount_ = 6;
            break;
            
        default:
            // Custom - wide range
            lowCenters_ = {20, 30, 40, 60, 80, 100, 150, 200, 300};
            lowCount_ = 9;
            
            lmCenters_ = {200, 300, 500, 700, 1000, 1500, 2000, 3000};
            lmCount_ = 8;
            
            hmCenters_ = {1000, 2000, 3000, 4000, 5000, 6000, 8000};
            hmCount_ = 7;
            
            highCenters_ = {5000, 8000, 10000, 12000, 16000, 20000};
            highCount_ = 6;
            break;
    }
}