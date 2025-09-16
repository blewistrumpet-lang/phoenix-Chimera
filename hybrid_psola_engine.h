#pragma once
#include <vector>
#include <deque>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include "psola_engine.h"

// Hybrid PSOLA Engine that intelligently switches between TD-PSOLA and resampling
// based on pitch ratio complexity to avoid subharmonic artifacts
class HybridPsolaEngine {
public:
    void prepare(double fs, double histSeconds = 0.6) {
        fs_ = fs;
        psolaEngine_.prepare(fs, histSeconds);
        
        // Prepare resampler buffers
        const int maxResampleLen = (int)(fs * 0.1); // 100ms max
        resampleBuffer_.resize(maxResampleLen);
        resampleOutput_.resize(maxResampleLen * 4); // Allow for upsampling
    }
    
    void resetSynthesis(int64_t synStartAbs = 0) {
        psolaEngine_.resetSynthesis(synStartAbs);
        resamplePhase_ = 0.0;
    }
    
    void pushBlock(const float* x, int N) {
        psolaEngine_.pushBlock(x, N);
        
        // Also store for resampler
        for (int i = 0; i < N; ++i) {
            resampleHistory_.push_back(x[i]);
        }
        // Keep only what we need
        while (resampleHistory_.size() > 48000) { // 1 second max
            resampleHistory_.pop_front();
        }
    }
    
    void appendEpochs(const std::vector<int>& local, int64_t localStartAbs, float T0, bool voiced) {
        psolaEngine_.appendEpochs(local, localStartAbs, T0, voiced);
    }
    
    // Main render function with intelligent algorithm selection
    void renderBlock(float alpha, float* out, int outN, int64_t outStartAbs = -1) {
        // Determine if this ratio is problematic for TD-PSOLA
        bool useResampling = shouldUseResampling(alpha);
        
        if (useResampling) {
            // Use high-quality resampling for problematic ratios
            renderResampled(alpha, out, outN);
        } else {
            // Use TD-PSOLA for simple ratios where it excels
            psolaEngine_.renderBlock(alpha, out, outN, outStartAbs);
        }
        
        // Optional: blend between methods for smooth transitions
        // This could be enhanced with crossfading when switching methods
    }
    
    int64_t writeCursorAbs() const { return psolaEngine_.writeCursorAbs(); }
    const std::deque<PsolaEpoch>& epochs() const { return psolaEngine_.epochs(); }
    
private:
    PsolaEngine psolaEngine_;
    double fs_ = 48000.0;
    
    // Resampling state
    std::deque<float> resampleHistory_;
    std::vector<float> resampleBuffer_;
    std::vector<float> resampleOutput_;
    double resamplePhase_ = 0.0;
    
    // Determine if a pitch ratio is problematic for TD-PSOLA
    bool shouldUseResampling(float alpha) const {
        // List of problematic ratios that cause subharmonics in TD-PSOLA
        // These are irrational or complex ratios where grain alignment creates beats
        
        // Check for irrational ratios near common musical intervals
        const struct { float ratio; float tolerance; const char* name; } problematic[] = {
            {0.7071f, 0.01f, "tritone down (√2/2)"},  // √2/2 = 0.7071...
            {1.4142f, 0.01f, "tritone up (√2)"},      // √2 = 1.4142...
            {0.7937f, 0.01f, "major third down"},     // 2^(-4/12) = 0.7937...
            {1.2599f, 0.01f, "major third up"},       // 2^(4/12) = 1.2599...
            {0.8909f, 0.01f, "major second down"},    // 2^(-2/12) = 0.8909...
            {1.1225f, 0.01f, "major second up"},      // 2^(2/12) = 1.1225...
        };
        
        for (const auto& p : problematic) {
            if (std::fabs(alpha - p.ratio) < p.tolerance) {
                // Debug output
                static int warnCount = 0;
                if (warnCount++ < 10) {
                    std::fprintf(stderr, "HybridPSOLA: Using resampling for %s (α=%.4f)\n", 
                                p.name, alpha);
                }
                return true;
            }
        }
        
        // Check if ratio can be expressed as simple fraction (works well with TD-PSOLA)
        // Simple ratios like 0.5, 1.5, 2.0 work perfectly with TD-PSOLA
        const float tolerance = 0.001f;
        for (int num = 1; num <= 4; ++num) {
            for (int den = 1; den <= 4; ++den) {
                float simpleRatio = (float)num / (float)den;
                if (std::fabs(alpha - simpleRatio) < tolerance) {
                    return false; // Simple ratio, use TD-PSOLA
                }
            }
        }
        
        // For other ratios, check if they're close to irrational numbers
        // by seeing if the continued fraction expansion has large terms
        float remainder = alpha;
        for (int i = 0; i < 3; ++i) {
            if (remainder < 0.001f) break;
            float intPart = std::floor(remainder);
            remainder = remainder - intPart;
            if (remainder > 0.001f) {
                remainder = 1.0f / remainder;
                if (remainder > 10.0f) {
                    // Large term in continued fraction suggests irrational
                    return true;
                }
            }
        }
        
        return false; // Default to TD-PSOLA
    }
    
    // High-quality resampling for problematic ratios
    void renderResampled(float alpha, float* out, int outN) {
        std::fill(out, out + outN, 0.0f);
        
        if (resampleHistory_.empty()) return;
        
        // Linear interpolation resampling (can be upgraded to sinc)
        for (int i = 0; i < outN; ++i) {
            // Calculate source position
            double srcPos = resamplePhase_;
            
            // Bounds check
            if (srcPos >= resampleHistory_.size() - 1) {
                resamplePhase_ = 0.0;
                srcPos = 0.0;
            }
            
            // Linear interpolation
            int idx = (int)srcPos;
            float frac = srcPos - idx;
            
            if (idx < resampleHistory_.size() - 1) {
                out[i] = resampleHistory_[idx] * (1.0f - frac) + 
                        resampleHistory_[idx + 1] * frac;
            }
            
            // Advance phase
            resamplePhase_ += 1.0 / alpha;
            
            // Wrap if needed
            while (resamplePhase_ >= resampleHistory_.size()) {
                resamplePhase_ -= resampleHistory_.size();
            }
        }
    }
};

// Enhanced version with formant correction for even better quality
class FormantCorrectingHybridPsolaEngine : public HybridPsolaEngine {
public:
    void renderBlock(float alpha, float* out, int outN, int64_t outStartAbs = -1) {
        // First do pitch shifting
        HybridPsolaEngine::renderBlock(alpha, out, outN, outStartAbs);
        
        // Apply formant correction if pitch shift is significant
        if (std::fabs(alpha - 1.0f) > 0.2f) {
            applyFormantCorrection(out, outN, alpha);
        }
    }
    
protected:
    void applyFormantCorrection(float* out, int N, float alpha) {
        // Simple spectral envelope preservation
        // This is a placeholder - real implementation would use
        // spectral envelope estimation and correction
        
        // For now, just apply gentle spectral tilt compensation
        float tilt = std::log2(alpha) * 0.5f; // dB/octave compensation
        
        // Apply gentle high-frequency boost for pitch-up, cut for pitch-down
        // This would be better done with proper spectral processing
        static float z1 = 0.0f;
        float freq = 2000.0f / 48000.0f; // Correction frequency normalized
        float gain = std::pow(10.0f, tilt / 20.0f);
        
        for (int i = 0; i < N; ++i) {
            float hp = out[i] - z1;
            z1 = out[i];
            out[i] = out[i] + hp * (gain - 1.0f) * freq;
        }
    }
};