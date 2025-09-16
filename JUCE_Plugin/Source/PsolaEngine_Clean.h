#pragma once
#include <vector>
#include <deque>
#include <cmath>
#include <algorithm>
#include <cstring>

/**
 * Clean TD-PSOLA implementation focused on artifact reduction
 * Key improvements:
 * - Smooth epoch transitions with proper windowing
 * - Careful boundary handling to prevent clicks
 * - RMS-based amplitude compensation
 * - Improved phase alignment
 */
class PsolaEngine_Clean {
public:
    explicit PsolaEngine_Clean(double sampleRate = 48000.0)
        : fs_(sampleRate) {
        setHistorySize(32768);
        reset();
    }
    
    void reset() {
        std::fill(hist_.begin(), hist_.end(), 0.0f);
        writePos_ = 0;
        readPos_ = 0.0;
        epochs_.clear();
        lastPeriod_ = 100.0f;
        targetRatio_ = 1.0f;
        currentRatio_ = 1.0f;
        rmsTracker_ = 0.0f;
        phaseAccum_ = 0.0;
    }
    
    void setHistorySize(int samples) {
        int pow2 = 1;
        while (pow2 < samples) pow2 <<= 1;
        histSize_ = pow2;
        histMask_ = pow2 - 1;
        hist_.resize(histSize_, 0.0f);
    }
    
    void setPitchRatio(float ratio) {
        targetRatio_ = std::max(0.25f, std::min(4.0f, ratio));
    }
    
    // Process block with pitch detection and synthesis
    void process(const float* input, float* output, int numSamples, 
                 const std::vector<int>& pitchMarks, float detectedPeriod) {
        
        // Update pitch ratio smoothly
        const float smoothTime = 0.995f;
        
        // Store input in circular buffer
        for (int i = 0; i < numSamples; ++i) {
            hist_[writePos_ & histMask_] = input[i];
            writePos_++;
        }
        
        // Update epoch markers
        if (!pitchMarks.empty() && detectedPeriod > 16.0f) {
            updateEpochs(pitchMarks, detectedPeriod);
            lastPeriod_ = detectedPeriod;
        }
        
        // Clear output
        std::memset(output, 0, numSamples * sizeof(float));
        
        // Need at least 3 epochs for synthesis
        if (epochs_.size() < 3) {
            // Passthrough with fade
            for (int i = 0; i < numSamples; ++i) {
                int readIdx = (writePos_ - numSamples + i) & histMask_;
                output[i] = hist_[readIdx] * 0.5f;
            }
            return;
        }
        
        // Synthesis using PSOLA
        synthesize(output, numSamples);
        
        // Update RMS tracker for amplitude compensation
        float blockRms = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            blockRms += input[i] * input[i];
        }
        blockRms = std::sqrt(blockRms / numSamples);
        rmsTracker_ = 0.95f * rmsTracker_ + 0.05f * blockRms;
        
        // Apply amplitude compensation
        float outputRms = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            outputRms += output[i] * output[i];
        }
        outputRms = std::sqrt(outputRms / numSamples);
        
        if (outputRms > 1e-6f && rmsTracker_ > 1e-6f) {
            float gain = rmsTracker_ / outputRms;
            gain = std::min(2.0f, gain); // Limit gain
            for (int i = 0; i < numSamples; ++i) {
                output[i] *= gain;
            }
        }
    }
    
private:
    struct Epoch {
        int position;      // Sample position in buffer
        float period;      // Local period estimate
        float amplitude;   // RMS amplitude around epoch
    };
    
    void updateEpochs(const std::vector<int>& marks, float period) {
        // Convert local marks to absolute positions
        int basePos = writePos_ - (int)hist_.size()/4; // Look back 1/4 buffer
        
        for (int mark : marks) {
            int absPos = basePos + mark;
            
            // Skip if too close to previous epoch
            if (!epochs_.empty()) {
                int lastPos = epochs_.back().position;
                if (std::abs(absPos - lastPos) < period * 0.5f) {
                    continue;
                }
            }
            
            // Calculate local RMS
            float rms = 0.0f;
            int windowSize = (int)(period * 0.5f);
            for (int i = -windowSize; i <= windowSize; ++i) {
                int idx = (absPos + i) & histMask_;
                float sample = hist_[idx];
                rms += sample * sample;
            }
            rms = std::sqrt(rms / (2 * windowSize + 1));
            
            epochs_.push_back({absPos, period, rms});
        }
        
        // Remove old epochs (keep last 100)
        while (epochs_.size() > 100) {
            epochs_.pop_front();
        }
    }
    
    void synthesize(float* output, int numSamples) {
        // Smooth ratio change
        const float alphaSmooth = 0.99f;
        currentRatio_ = alphaSmooth * currentRatio_ + (1.0f - alphaSmooth) * targetRatio_;
        
        float synthesisHop = lastPeriod_ / currentRatio_;
        int outputPos = 0;
        
        while (outputPos < numSamples) {
            // Find nearest epoch for current read position
            int epochIdx = findNearestEpoch(readPos_);
            if (epochIdx < 0 || epochIdx >= (int)epochs_.size()) {
                readPos_ += synthesisHop;
                outputPos++;
                continue;
            }
            
            const Epoch& epoch = epochs_[epochIdx];
            
            // Create synthesis grain
            int grainSize = (int)(epoch.period * 2.0f);
            if (grainSize < 32) grainSize = 32;
            
            // Apply Hann window and accumulate
            float centerPos = (float)epoch.position;
            for (int i = 0; i < grainSize; ++i) {
                int outIdx = outputPos + i - grainSize/2;
                if (outIdx < 0 || outIdx >= numSamples) continue;
                
                // Window function
                float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (grainSize - 1)));
                
                // Read from buffer with interpolation
                float readIdx = centerPos + i - grainSize/2;
                int idx0 = (int)readIdx;
                float frac = readIdx - idx0;
                
                float s0 = hist_[idx0 & histMask_];
                float s1 = hist_[(idx0 + 1) & histMask_];
                float sample = s0 + frac * (s1 - s0);
                
                // Accumulate with windowing
                output[outIdx] += sample * window * 0.5f; // Scale for overlap
            }
            
            // Advance read position
            readPos_ += synthesisHop;
            outputPos += (int)synthesisHop;
            
            // Phase accumulator for sub-sample precision
            phaseAccum_ += synthesisHop - (int)synthesisHop;
            if (phaseAccum_ >= 1.0) {
                phaseAccum_ -= 1.0;
                outputPos++;
            }
        }
    }
    
    int findNearestEpoch(float position) {
        if (epochs_.empty()) return -1;
        
        int best = 0;
        float minDist = std::abs(position - epochs_[0].position);
        
        for (int i = 1; i < (int)epochs_.size(); ++i) {
            float dist = std::abs(position - epochs_[i].position);
            if (dist < minDist) {
                minDist = dist;
                best = i;
            }
        }
        
        return best;
    }
    
    double fs_;
    int histSize_;
    int histMask_;
    std::vector<float> hist_;
    int writePos_;
    float readPos_;
    
    std::deque<Epoch> epochs_;
    float lastPeriod_;
    float targetRatio_;
    float currentRatio_;
    float rmsTracker_;
    double phaseAccum_;
};