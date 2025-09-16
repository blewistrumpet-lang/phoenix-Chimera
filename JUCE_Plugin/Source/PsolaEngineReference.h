#pragma once
#include <vector>
#include <deque>
#include <cmath>
#include <algorithm>
#include <cstring>

/**
 * TD-PSOLA Implementation based on working reference code
 * From sannawag/TD-PSOLA and terrykong/Phase-Vocoder
 * 
 * Key insights from working implementations:
 * 1. Analysis shift = pitch period in samples
 * 2. Synthesis shift = analysis shift / pitch_ratio (NOT * ratio)
 * 3. New peak positions are INTERPOLATED from original peaks
 * 4. Triangular/linear windows for smooth transitions
 */

class PsolaEngineReference {
public:
    PsolaEngineReference() : sampleRate_(48000.0) {
        reset();
    }
    
    void setSampleRate(double fs) {
        sampleRate_ = fs;
    }
    
    void reset() {
        inputBuffer_.resize(65536);
        std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);
        outputBuffer_.resize(4096);
        std::fill(outputBuffer_.begin(), outputBuffer_.end(), 0.0f);
        writePos_ = 0;
        peaks_.clear();
    }
    
    // Store input and detect peaks
    void analyzeInput(const float* input, int numSamples, float pitchHz) {
        // Store input
        for (int i = 0; i < numSamples; ++i) {
            inputBuffer_[(writePos_ + i) % inputBuffer_.size()] = input[i];
        }
        
        // Calculate period
        float period = sampleRate_ / pitchHz;
        
        // Find peaks using simple maximum detection
        // Based on sannawag/TD-PSOLA approach
        for (int i = (int)period; i < numSamples - (int)period; i += (int)(period * 0.8f)) {
            // Find local maximum within ±5% of expected period
            int searchStart = i - (int)(period * 0.05f);
            int searchEnd = i + (int)(period * 0.05f);
            
            int peakPos = i;
            float peakVal = input[i];
            
            for (int j = searchStart; j <= searchEnd && j < numSamples; ++j) {
                if (j >= 0 && input[j] > peakVal) {
                    peakVal = input[j];
                    peakPos = j;
                }
            }
            
            // Store absolute position
            int64_t absPeak = writePos_ + peakPos;
            if (peaks_.empty() || absPeak - peaks_.back() > period * 0.7f) {
                peaks_.push_back(absPeak);
            }
        }
        
        // Keep only recent peaks
        while (peaks_.size() > 100) {
            peaks_.pop_front();
        }
        
        writePos_ += numSamples;
        currentPeriod_ = period;
    }
    
    // PSOLA synthesis based on working implementations
    void synthesize(float* output, int numSamples, float pitchRatio) {
        std::memset(output, 0, numSamples * sizeof(float));
        
        if (peaks_.size() < 3) {
            return;
        }
        
        // From terrykong implementation:
        // scalingFactor = 1 + (inputPitch - desiredPitch)/desiredPitch
        // But for ratio-based: scalingFactor = 1/pitchRatio
        
        // Analysis shift (distance between original peaks)
        float analysisShift = currentPeriod_;
        
        // Synthesis shift (distance between output peaks)
        // THIS IS THE KEY: synthesis shift = analysis shift / ratio
        float synthesisShift = analysisShift / pitchRatio;
        
        // Create interpolated peak positions (from sannawag implementation)
        std::vector<float> newPeakPositions;
        float currentPos = 0;
        while (currentPos < numSamples) {
            newPeakPositions.push_back(currentPos);
            currentPos += synthesisShift;
        }
        
        // For each synthesis peak position
        for (float synthPos : newPeakPositions) {
            // Map to analysis position
            // Key: synthesis advances slower for pitch down, faster for pitch up
            float analysisPos = synthPos * pitchRatio;
            
            // Find the corresponding input peak by interpolation
            int peakIndex = (int)(analysisPos / analysisShift);
            if (peakIndex < 0 || peakIndex >= (int)peaks_.size() - 1) {
                continue;
            }
            
            // Linear interpolation weight
            float weight = (analysisPos / analysisShift) - peakIndex;
            
            // Interpolate between adjacent peaks
            int64_t peakPos = (int64_t)(peaks_[peakIndex] * (1 - weight) + 
                                        peaks_[peakIndex + 1] * weight);
            
            // Create grain centered at this peak
            int grainSize = (int)(currentPeriod_ * 2);
            
            // Apply triangular (Bartlett) window and copy to output
            for (int i = 0; i < grainSize; ++i) {
                int outIdx = (int)synthPos + i - grainSize/2;
                if (outIdx < 0 || outIdx >= numSamples) continue;
                
                // Triangular window
                float window;
                if (i < grainSize/2) {
                    window = (float)i / (grainSize/2);
                } else {
                    window = (float)(grainSize - i) / (grainSize/2);
                }
                
                // Read from input buffer
                int64_t readPos = peakPos + i - grainSize/2;
                float sample = 0;
                if (readPos >= 0 && readPos < writePos_) {
                    sample = inputBuffer_[readPos % inputBuffer_.size()];
                }
                
                output[outIdx] += sample * window;
            }
        }
        
        // Normalize based on overlap
        float overlap = currentPeriod_ / synthesisShift;
        if (overlap > 1.5f) {
            float norm = 1.0f / std::sqrt(overlap * 0.5f);
            for (int i = 0; i < numSamples; ++i) {
                output[i] *= norm;
            }
        }
    }
    
    // Combined process function
    void process(const float* input, float* output, int numSamples, 
                 float pitchRatio, float detectedPitchHz) {
        analyzeInput(input, numSamples, detectedPitchHz);
        synthesize(output, numSamples, pitchRatio);
    }
    
private:
    double sampleRate_;
    std::vector<float> inputBuffer_;
    std::vector<float> outputBuffer_;
    int64_t writePos_;
    std::deque<int64_t> peaks_;
    float currentPeriod_;
};