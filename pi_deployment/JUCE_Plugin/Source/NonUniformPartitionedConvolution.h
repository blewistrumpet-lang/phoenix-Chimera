#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <complex>
#include <array>

/**
 * Non-Uniform Partitioned Convolution Engine
 * 
 * Uses different partition sizes for different parts of the impulse response:
 * - Small partitions (64-256 samples) for early reflections (low latency)
 * - Medium partitions (512-2048 samples) for mid-range
 * - Large partitions (4096-8192 samples) for reverb tail (high efficiency)
 * 
 * This provides a good balance between latency and CPU efficiency.
 */
class NonUniformPartitionedConvolution {
public:
    NonUniformPartitionedConvolution();
    ~NonUniformPartitionedConvolution();
    
    // Configuration
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void loadImpulseResponse(const float* ir, size_t irLength, bool normalize = true);
    
    // Processing
    void process(const float* input, float* output, int numSamples);
    void processBlock(juce::AudioBuffer<float>& buffer);
    
    // Getters
    int getLatency() const { return m_minPartitionSize; }
    bool isReady() const { return m_isReady; }
    
private:
    // Partition structure
    struct Partition {
        size_t size;                           // Partition size in samples
        size_t offset;                         // Offset in IR
        std::vector<std::complex<float>> fftData; // FFT of this partition
        std::unique_ptr<juce::dsp::FFT> fft;  // FFT processor for this size
    };
    
    // Convolution segment for a specific partition size
    struct ConvolutionSegment {
        size_t partitionSize;
        size_t fftSize;
        int numPartitions;
        
        // FFT processors
        std::unique_ptr<juce::dsp::FFT> fft;
        
        // Buffers
        std::vector<std::vector<std::complex<float>>> partitionFFTs; // FFT of each IR partition
        std::vector<std::complex<float>> inputFFT;                   // Current input FFT
        std::vector<std::vector<std::complex<float>>> inputHistory;  // History of input FFTs
        std::vector<float> overlapBuffer;                            // Overlap-add buffer
        std::vector<float> fftBuffer;                                // Temp buffer for FFT
        
        // State
        int historyIndex = 0;
        int overlapIndex = 0;
        
        void prepare(size_t partSize, int numParts);
        void reset();
        void processPartition(const float* input, float* output, int numSamples);
    };
    
    // Processing state
    bool m_isReady = false;
    double m_sampleRate = 48000.0;
    int m_maxBlockSize = 512;
    
    // Partition configuration
    static constexpr size_t m_minPartitionSize = 64;    // Minimum partition (latency)
    static constexpr size_t m_maxPartitionSize = 8192;  // Maximum partition
    
    // Segments for different partition sizes
    std::vector<std::unique_ptr<ConvolutionSegment>> m_segments;
    
    // Input/output buffers
    std::vector<float> m_inputBuffer;
    std::vector<float> m_outputBuffer;
    int m_bufferIndex = 0;
    
    // Processing accumulator
    std::vector<float> m_accumulator;
    
    // Helper functions
    void createPartitionScheme(size_t irLength);
    void computePartitionFFTs(const float* ir, size_t irLength);
    size_t nextPowerOfTwo(size_t n) const;
    void normalizeImpulseResponse(std::vector<float>& ir);
};

/**
 * Optimized convolution segment implementation
 */
class OptimizedConvolutionSegment {
public:
    OptimizedConvolutionSegment(size_t partitionSize, size_t numPartitions);
    
    void loadIRPartitions(const float* ir, size_t offset, size_t numPartitions);
    void process(const float* input, float* output, size_t numSamples);
    void reset();
    
private:
    size_t m_partitionSize;
    size_t m_fftSize;
    size_t m_numPartitions;
    
    // SIMD-aligned buffers
    std::vector<float, juce::dsp::SIMDRegister<float>::AlignedAllocator<float>> m_fftWorkspace;
    std::vector<std::complex<float>, juce::dsp::SIMDRegister<std::complex<float>>::AlignedAllocator<std::complex<float>>> m_frequencyDomain;
    
    // Partition management
    struct PartitionData {
        std::vector<std::complex<float>> spectrum;
        float gain = 1.0f;
    };
    std::vector<PartitionData> m_partitions;
    
    // Ring buffer for input history
    std::vector<std::vector<std::complex<float>>> m_inputSpectrumHistory;
    size_t m_historyWritePos = 0;
    
    // Overlap-save state
    std::vector<float> m_overlapBuffer;
    size_t m_overlapPos = 0;
    
    // FFT
    std::unique_ptr<juce::dsp::FFT> m_fft;
    
    // SIMD-optimized complex multiply-accumulate
    void complexMultiplyAccumulate(std::complex<float>* result, 
                                  const std::complex<float>* a, 
                                  const std::complex<float>* b, 
                                  size_t count);
};