#pragma once
#include <atomic>
#include <chrono>
#include <string>
#include <sstream>
#include <cmath>

/**
 * Real-time safe quality metrics tracking
 */
class QualityMetrics {
public:
    QualityMetrics() = default;
    
    void setSampleRate(double sr) noexcept {
        sampleRate = sr;
    }
    
    void reset() noexcept {
        cpuUsage.store(0.0f);
        peakLevel.store(0.0f);
        rmsLevel.store(0.0f);
        denormalCount.store(0);
        totalSamples.store(0);
        totalBlocks.store(0);
    }
    
    void startBlock() noexcept {
        blockStart = std::chrono::high_resolution_clock::now();
    }
    
    void endBlock(int numSamples, int numChannels) noexcept {
        auto blockEnd = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(blockEnd - blockStart).count();
        
        // Calculate theoretical time for this block
        double theoreticalTime = static_cast<double>(numSamples) / sampleRate;
        
        // CPU usage as percentage
        float usage = static_cast<float>((duration / theoreticalTime) * 100.0);
        cpuUsage.store(usage);
        
        totalSamples.fetch_add(numSamples * numChannels);
        totalBlocks.fetch_add(1);
    }
    
    void updatePeakRMS(const float* data, int numSamples) noexcept {
        float peak = 0.0f;
        float sum = 0.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            float sample = data[i];
            peak = std::max(peak, std::abs(sample));
            sum += sample * sample;
        }
        
        // Update atomic values
        float currentPeak = peakLevel.load();
        while (peak > currentPeak && !peakLevel.compare_exchange_weak(currentPeak, peak));
        
        float rms = std::sqrt(sum / numSamples);
        rmsLevel.store(rms);
    }
    
    void checkDenormals(const float* data, int numSamples) noexcept {
        for (int i = 0; i < numSamples; ++i) {
            if (data[i] != 0.0f && std::abs(data[i]) < 1e-30f) {
                denormalCount.fetch_add(1);
            }
        }
    }
    
    float getCPUUsage() const noexcept {
        return cpuUsage.load();
    }
    
    float getDynamicRangeDB() const noexcept {
        float peak = peakLevel.load();
        float rms = rmsLevel.load();
        
        if (rms > 0.0f && peak > 0.0f) {
            return 20.0f * std::log10(peak / rms);
        }
        return 144.0f; // Default high dynamic range
    }
    
    std::string getReport() const {
        std::stringstream ss;
        ss << "CPU: " << getCPUUsage() << "%\n";
        ss << "Dynamic Range: " << getDynamicRangeDB() << " dB\n";
        ss << "Peak Level: " << 20.0f * std::log10(peakLevel.load() + 1e-10f) << " dBFS\n";
        ss << "RMS Level: " << 20.0f * std::log10(rmsLevel.load() + 1e-10f) << " dBFS\n";
        ss << "Denormals: " << denormalCount.load() << "\n";
        ss << "Total Samples: " << totalSamples.load() << "\n";
        ss << "Total Blocks: " << totalBlocks.load() << "\n";
        return ss.str();
    }
    
private:
    std::atomic<float> cpuUsage{0.0f};
    std::atomic<float> peakLevel{0.0f};
    std::atomic<float> rmsLevel{0.0f};
    std::atomic<uint64_t> denormalCount{0};
    std::atomic<uint64_t> totalSamples{0};
    std::atomic<uint64_t> totalBlocks{0};
    
    double sampleRate{48000.0};
    std::chrono::high_resolution_clock::time_point blockStart;
};