#pragma once
#include <vector>
#include <deque>
#include <cmath>
#include <algorithm>

/**
 * CORRECT TD-PSOLA Implementation
 * 
 * The key insight: for pitch ratio α (alpha):
 * - Synthesis marks are placed every T0/α samples
 * - At each synthesis mark, we select the nearest analysis epoch
 * - The φ (phi) mapping is: synthesis position -> analysis position
 * 
 * NO "surgical fixes" - just correct, simple PSOLA
 */

struct Epoch {
    int64_t position;  // Absolute sample position
    float period;      // Local period estimate
    float amplitude;   // Local RMS
};

class PsolaEngine_Correct {
public:
    PsolaEngine_Correct() { reset(); }
    
    void reset() {
        buffer_.clear();
        buffer_.resize(65536, 0.0f);
        writePos_ = 0;
        epochs_.clear();
        lastPeriod_ = 218.0f; // Default ~220Hz
    }
    
    void setSampleRate(double fs) {
        sampleRate_ = fs;
    }
    
    // Add input samples to buffer
    void pushSamples(const float* input, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            buffer_[writePos_ % buffer_.size()] = input[i];
            writePos_++;
        }
    }
    
    // Add detected pitch epochs
    void addEpochs(const std::vector<int>& localPositions, int blockStart, float period) {
        for (int pos : localPositions) {
            int64_t absPos = blockStart + pos;
            
            // Skip if too close to previous
            if (!epochs_.empty() && (absPos - epochs_.back().position) < period * 0.5f) {
                continue;
            }
            
            // Calculate local RMS
            float rms = 0.0f;
            int window = (int)(period * 0.5f);
            for (int j = -window; j <= window; ++j) {
                int idx = (absPos + j) % buffer_.size();
                if (idx >= 0 && idx < buffer_.size()) {
                    rms += buffer_[idx] * buffer_[idx];
                }
            }
            rms = std::sqrt(rms / (2 * window + 1));
            
            epochs_.push_back({absPos, period, rms});
        }
        
        // Keep only recent epochs (last 2 seconds)
        int64_t cutoff = writePos_ - (int64_t)(2.0 * sampleRate_);
        while (!epochs_.empty() && epochs_.front().position < cutoff) {
            epochs_.pop_front();
        }
        
        lastPeriod_ = period;
    }
    
    // Main PSOLA synthesis
    void synthesize(float* output, int numSamples, float pitchRatio, int outputStartPos) {
        // Clear output
        std::fill(output, output + numSamples, 0.0f);
        
        if (epochs_.size() < 3) {
            return; // Need epochs to synthesize
        }
        
        // Synthesis hop (spacing between synthesis marks)
        float synthesisHop = lastPeriod_ / pitchRatio;
        
        // Process synthesis marks
        float synthesisPos = outputStartPos;
        int outputIdx = 0;
        
        while (outputIdx < numSamples) {
            // Find analysis position using φ mapping
            // For pitch up: read faster (advance more in input)
            // For pitch down: read slower (advance less in input)
            // φ(t_syn) = t_syn / pitchRatio
            float analysisPos = synthesisPos / pitchRatio;
            
            // Find nearest epoch to analysis position
            int nearestEpoch = findNearestEpoch(analysisPos);
            if (nearestEpoch < 0) {
                synthesisPos += synthesisHop;
                outputIdx = (int)(synthesisPos - outputStartPos);
                continue;
            }
            
            const Epoch& epoch = epochs_[nearestEpoch];
            
            // Create grain centered at this epoch
            int grainSize = (int)(epoch.period * 2.0f);
            if (grainSize < 64) grainSize = 64;
            
            // Apply Hann window and copy to output
            for (int i = 0; i < grainSize; ++i) {
                int outputSample = outputIdx + i - grainSize/2;
                if (outputSample < 0 || outputSample >= numSamples) continue;
                
                // Hann window
                float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (grainSize - 1)));
                
                // Read from buffer at epoch position
                int64_t readPos = epoch.position + i - grainSize/2;
                float sample = 0.0f;
                if (readPos >= 0 && readPos < writePos_) {
                    sample = buffer_[readPos % buffer_.size()];
                }
                
                // Accumulate windowed grain
                output[outputSample] += sample * window;
            }
            
            // Move to next synthesis position
            synthesisPos += synthesisHop;
            outputIdx = (int)(synthesisPos - outputStartPos);
        }
        
        // Normalize output to prevent amplitude buildup
        float overlap = lastPeriod_ / synthesisHop;
        float gain = 1.0f / std::sqrt(overlap);
        
        for (int i = 0; i < numSamples; ++i) {
            output[i] *= gain;
        }
    }
    
private:
    int findNearestEpoch(float position) {
        if (epochs_.empty()) return -1;
        
        int best = -1;
        float minDist = 1e9f;
        
        for (size_t i = 0; i < epochs_.size(); ++i) {
            float dist = std::abs(position - epochs_[i].position);
            if (dist < minDist) {
                minDist = dist;
                best = i;
            }
        }
        
        // Only return if reasonably close
        if (minDist > lastPeriod_ * 2.0f) {
            return -1;
        }
        
        return best;
    }
    
    std::vector<float> buffer_;
    int64_t writePos_ = 0;
    std::deque<Epoch> epochs_;
    float lastPeriod_ = 218.0f;
    double sampleRate_ = 48000.0;
};