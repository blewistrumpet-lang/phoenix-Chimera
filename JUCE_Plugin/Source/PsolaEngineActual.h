#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>

/**
 * ACTUAL TD-PSOLA Implementation
 * Direct port of sannawag/TD-PSOLA working Python code
 * 
 * Algorithm:
 * 1. Find peaks in input signal
 * 2. Create new peak positions: spread them out for pitch down, compress for pitch up
 * 3. For each new peak, copy windowed segment from nearest original peak
 */

class PsolaEngineActual {
public:
    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }
    
    void reset() {
        inputBuffer_.clear();
        inputBuffer_.resize(65536, 0.0f);
        writePos_ = 0;
        peaks_.clear();
    }
    
    // Main PSOLA processing
    void process(const float* input, float* output, int numSamples, float pitchRatio) {
        // Store input
        for (int i = 0; i < numSamples; ++i) {
            inputBuffer_[(writePos_ + i) % inputBuffer_.size()] = input[i];
        }
        
        // Find peaks in the input (simple approach - find local maxima)
        std::vector<int> localPeaks = findPeaks(input, numSamples);
        
        // Add to global peak list
        for (int peak : localPeaks) {
            peaks_.push_back(writePos_ + peak);
        }
        
        // Keep only recent peaks
        while (peaks_.size() > 200) {
            peaks_.erase(peaks_.begin());
        }
        
        writePos_ += numSamples;
        
        // Clear output
        std::memset(output, 0, numSamples * sizeof(float));
        
        if (peaks_.size() < 4) {
            // Not enough peaks - pass through
            std::memcpy(output, input, numSamples * sizeof(float));
            return;
        }
        
        // PSOLA synthesis (based on Python implementation)
        synthesize(output, numSamples, pitchRatio);
    }
    
private:
    std::vector<int> findPeaks(const float* signal, int numSamples) {
        std::vector<int> peaks;
        
        // Simple peak detection - find local maxima
        // Look for peaks every ~220 samples (for 220Hz at 48kHz)
        int expectedPeriod = (int)(sampleRate_ / 220.0f);
        int searchWindow = expectedPeriod / 4;
        
        for (int i = expectedPeriod; i < numSamples - expectedPeriod; i += expectedPeriod) {
            // Find local maximum
            int peakIdx = i;
            float peakVal = signal[i];
            
            for (int j = i - searchWindow; j <= i + searchWindow; ++j) {
                if (j >= 0 && j < numSamples && signal[j] > peakVal) {
                    peakVal = signal[j];
                    peakIdx = j;
                }
            }
            
            // Only add if it's a significant peak
            if (peakVal > 0.1f) {
                if (peaks.empty() || peakIdx - peaks.back() > expectedPeriod * 0.5) {
                    peaks.push_back(peakIdx);
                }
            }
        }
        
        return peaks;
    }
    
    void synthesize(float* output, int numSamples, float pitchRatio) {
        // Based on Python: new_peaks_ref = np.linspace(0, len(peaks) - 1, len(peaks) * f_ratio)
        // This creates NEW peak positions by spreading/compressing the original peaks
        
        int numOrigPeaks = peaks_.size();
        int numNewPeaks = (int)(numOrigPeaks * pitchRatio);
        
        if (numNewPeaks < 2) return;
        
        // Calculate new peak positions (interpolated from original)
        std::vector<float> newPeakRefs;
        for (int i = 0; i < numNewPeaks; ++i) {
            float ref = (float)i * (numOrigPeaks - 1) / (numNewPeaks - 1);
            newPeakRefs.push_back(ref);
        }
        
        // Convert references to actual sample positions
        std::vector<int> newPeaks;
        for (float ref : newPeakRefs) {
            int left = (int)std::floor(ref);
            int right = (int)std::ceil(ref);
            float weight = ref - left;
            
            if (left < 0) left = 0;
            if (right >= peaks_.size()) right = peaks_.size() - 1;
            
            int newPeak = (int)(peaks_[left] * (1 - weight) + peaks_[right] * weight);
            newPeaks.push_back(newPeak);
        }
        
        // PSOLA overlap-add
        for (size_t j = 0; j < newPeaks.size(); ++j) {
            // Find corresponding original peak (nearest)
            int nearestOrigIdx = 0;
            int minDist = std::abs(peaks_[0] - newPeaks[j]);
            
            for (size_t i = 1; i < peaks_.size(); ++i) {
                int dist = std::abs(peaks_[i] - newPeaks[j]);
                if (dist < minDist) {
                    minDist = dist;
                    nearestOrigIdx = i;
                }
            }
            
            // Get window size (distance to adjacent new peaks)
            int leftDist = (j == 0) ? 100 : (newPeaks[j] - newPeaks[j-1]) / 2;
            int rightDist = (j == newPeaks.size()-1) ? 100 : (newPeaks[j+1] - newPeaks[j]) / 2;
            
            // Ensure we don't exceed buffer bounds
            int origPeak = peaks_[nearestOrigIdx];
            leftDist = std::min(leftDist, origPeak);
            rightDist = std::min(rightDist, (int)inputBuffer_.size() - origPeak - 1);
            
            // Linear window (triangular)
            std::vector<float> window;
            
            // Rising edge
            for (int i = 0; i < leftDist; ++i) {
                window.push_back((float)i / leftDist);
            }
            // Peak
            window.push_back(1.0f);
            // Falling edge
            for (int i = 1; i <= rightDist; ++i) {
                window.push_back(1.0f - (float)i / (rightDist + 1));
            }
            
            // Apply windowed segment to output
            int newPeakInBlock = newPeaks[j] - (writePos_ - numSamples);
            
            for (int i = -leftDist; i <= rightDist; ++i) {
                int outIdx = newPeakInBlock + i;
                int winIdx = i + leftDist;
                
                if (outIdx >= 0 && outIdx < numSamples && winIdx < window.size()) {
                    int srcIdx = (origPeak + i) % inputBuffer_.size();
                    if (srcIdx >= 0 && srcIdx < inputBuffer_.size()) {
                        output[outIdx] += window[winIdx] * inputBuffer_[srcIdx];
                    }
                }
            }
        }
    }
    
    double sampleRate_ = 48000.0;
    std::vector<float> inputBuffer_;
    int writePos_ = 0;
    std::vector<int> peaks_;
};