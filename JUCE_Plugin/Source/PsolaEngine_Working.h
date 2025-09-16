#pragma once
#include <vector>
#include <deque>
#include <cmath>
#include <cstring>
#include <algorithm>

/**
 * WORKING TD-PSOLA Implementation
 * Based on the standard algorithm from speech processing literature
 * 
 * Key concepts:
 * - Analysis epochs are detected pitch marks in the input
 * - Synthesis marks are placed at intervals of T0/α 
 * - Each synthesis mark copies a windowed grain from the nearest analysis epoch
 */

class PsolaEngine_Working {
public:
    void prepare(double sampleRate) {
        fs_ = sampleRate;
        bufferSize_ = 65536;
        buffer_.resize(bufferSize_);
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        writePos_ = 0;
        readPos_ = 0.0;
        epochs_.clear();
    }
    
    void reset() {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        writePos_ = 0;
        readPos_ = 0.0;
        epochs_.clear();
    }
    
    // Store input samples
    void pushSamples(const float* input, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            buffer_[(writePos_ + i) % bufferSize_] = input[i];
        }
        writePos_ += numSamples;
    }
    
    // Add pitch epochs (positions relative to current writePos_)
    void addEpochs(const std::vector<int>& relativePositions, float period) {
        for (int relPos : relativePositions) {
            int64_t absPos = writePos_ - 512 + relPos; // Assume we're analyzing recent samples
            
            // Skip if too close to previous
            if (!epochs_.empty() && (absPos - epochs_.back()) < period * 0.5f) {
                continue;
            }
            
            epochs_.push_back(absPos);
        }
        
        // Remove old epochs
        while (!epochs_.empty() && epochs_.front() < writePos_ - bufferSize_/2) {
            epochs_.pop_front();
        }
        
        period_ = period;
    }
    
    // PSOLA synthesis
    void process(const float* input, float* output, int numSamples, float pitchRatio) {
        // Store input
        pushSamples(input, numSamples);
        
        // Clear output
        std::memset(output, 0, numSamples * sizeof(float));
        
        if (epochs_.size() < 2) {
            // Not enough epochs - pass through
            std::memcpy(output, input, numSamples * sizeof(float));
            return;
        }
        
        // Synthesis parameters
        float analysisHop = period_;              // Distance between analysis epochs
        float synthesisHop = period_ / pitchRatio; // Distance between synthesis marks
        
        // Generate synthesis marks for this block
        int outputSample = 0;
        
        while (outputSample < numSamples) {
            // Current synthesis position
            double synthPos = readPos_ + outputSample;
            
            // Map to analysis position
            // Key insight: synthesis advances by 1 sample/sample
            //              analysis advances by 1/pitchRatio samples/sample
            double analysisPos = writePos_ - numSamples + (outputSample * pitchRatio);
            
            // Find nearest epoch
            int epochIdx = -1;
            double minDist = 1e9;
            for (size_t i = 0; i < epochs_.size(); ++i) {
                double dist = std::abs(analysisPos - epochs_[i]);
                if (dist < minDist) {
                    minDist = dist;
                    epochIdx = i;
                }
            }
            
            if (epochIdx < 0 || minDist > period_ * 2) {
                outputSample++;
                continue;
            }
            
            // Create grain at this epoch
            int64_t epochPos = epochs_[epochIdx];
            int grainSize = (int)(period_ * 2);
            if (grainSize < 64) grainSize = 64;
            
            // Copy windowed grain to output
            for (int i = 0; i < grainSize; ++i) {
                int outIdx = outputSample + i - grainSize/2;
                if (outIdx < 0 || outIdx >= numSamples) continue;
                
                // Hann window
                float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (grainSize - 1)));
                
                // Read from buffer
                int64_t readIdx = epochPos + i - grainSize/2;
                float sample = 0.0f;
                if (readIdx >= 0 && readIdx < writePos_) {
                    sample = buffer_[readIdx % bufferSize_];
                }
                
                output[outIdx] += sample * window;
            }
            
            // Advance to next synthesis mark
            outputSample += (int)synthesisHop;
        }
        
        // Normalize for overlap
        float overlapFactor = period_ / synthesisHop;
        if (overlapFactor > 1.0f) {
            float norm = 1.0f / std::sqrt(overlapFactor);
            for (int i = 0; i < numSamples; ++i) {
                output[i] *= norm;
            }
        }
        
        // Update read position
        readPos_ += numSamples;
    }
    
private:
    double fs_ = 48000.0;
    int bufferSize_ = 65536;
    std::vector<float> buffer_;
    int64_t writePos_ = 0;
    double readPos_ = 0.0;
    std::deque<int64_t> epochs_;
    float period_ = 218.0f; // Default for 220Hz
};