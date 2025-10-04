#include "NonUniformPartitionedConvolution.h"
#include <algorithm>
#include <cmath>

//==============================================================================
// NonUniformPartitionedConvolution Implementation
//==============================================================================

NonUniformPartitionedConvolution::NonUniformPartitionedConvolution() {
    // Reserve space for segments
    m_segments.reserve(8);
}

NonUniformPartitionedConvolution::~NonUniformPartitionedConvolution() = default;

void NonUniformPartitionedConvolution::prepare(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    m_maxBlockSize = maxBlockSize;
    
    // Allocate buffers
    m_inputBuffer.resize(maxBlockSize * 2);
    m_outputBuffer.resize(maxBlockSize * 2);
    m_accumulator.resize(maxBlockSize);
    
    std::fill(m_inputBuffer.begin(), m_inputBuffer.end(), 0.0f);
    std::fill(m_outputBuffer.begin(), m_outputBuffer.end(), 0.0f);
    
    reset();
}

void NonUniformPartitionedConvolution::reset() {
    for (auto& segment : m_segments) {
        segment->reset();
    }
    
    std::fill(m_inputBuffer.begin(), m_inputBuffer.end(), 0.0f);
    std::fill(m_outputBuffer.begin(), m_outputBuffer.end(), 0.0f);
    std::fill(m_accumulator.begin(), m_accumulator.end(), 0.0f);
    
    m_bufferIndex = 0;
}

void NonUniformPartitionedConvolution::loadImpulseResponse(const float* ir, size_t irLength, bool normalize) {
    if (irLength == 0 || ir == nullptr) {
        m_isReady = false;
        return;
    }
    
    // Create a copy for normalization
    std::vector<float> irCopy(ir, ir + irLength);
    
    if (normalize) {
        normalizeImpulseResponse(irCopy);
    }
    
    // Create partition scheme
    createPartitionScheme(irLength);
    
    // Compute FFTs for all partitions
    computePartitionFFTs(irCopy.data(), irLength);
    
    m_isReady = true;
}

void NonUniformPartitionedConvolution::createPartitionScheme(size_t irLength) {
    m_segments.clear();
    
    // Non-uniform partition scheme:
    // - First 512 samples: 64-sample partitions (8 partitions) - Ultra low latency
    // - Next 2048 samples: 256-sample partitions (8 partitions) - Low latency
    // - Next 8192 samples: 1024-sample partitions (8 partitions) - Medium latency
    // - Next 32768 samples: 4096-sample partitions (8 partitions) - High efficiency
    // - Remainder: 8192-sample partitions - Maximum efficiency
    
    struct PartitionPlan {
        size_t partitionSize;
        size_t coverageSamples;
        size_t maxPartitions;
    };
    
    std::vector<PartitionPlan> plans = {
        {64,    512,    8},   // 8 * 64 = 512 samples
        {256,   2048,   8},   // 8 * 256 = 2048 samples  
        {1024,  8192,   8},   // 8 * 1024 = 8192 samples
        {4096,  32768,  8},   // 8 * 4096 = 32768 samples
        {8192,  SIZE_MAX, 16} // Remainder with 8192-sample partitions
    };
    
    size_t currentOffset = 0;
    
    for (const auto& plan : plans) {
        if (currentOffset >= irLength) break;
        
        size_t remainingSamples = irLength - currentOffset;
        size_t samplesForThisSegment = std::min(plan.coverageSamples, remainingSamples);
        size_t numPartitions = (samplesForThisSegment + plan.partitionSize - 1) / plan.partitionSize;
        numPartitions = std::min(numPartitions, plan.maxPartitions);
        
        if (numPartitions > 0) {
            auto segment = std::make_unique<ConvolutionSegment>();
            segment->prepare(plan.partitionSize, static_cast<int>(numPartitions));
            m_segments.push_back(std::move(segment));
            
            currentOffset += numPartitions * plan.partitionSize;
        }
    }
}

void NonUniformPartitionedConvolution::computePartitionFFTs(const float* ir, size_t irLength) {
    size_t offset = 0;
    
    for (auto& segment : m_segments) {
        size_t partitionSize = segment->partitionSize;
        size_t fftSize = segment->fftSize;
        
        // Prepare FFT buffer
        std::vector<float> fftBuffer(fftSize, 0.0f);
        
        for (int p = 0; p < segment->numPartitions; ++p) {
            // Copy IR segment
            size_t copySize = std::min(partitionSize, irLength - offset);
            if (copySize == 0) break;
            
            std::copy(ir + offset, ir + offset + copySize, fftBuffer.begin());
            std::fill(fftBuffer.begin() + copySize, fftBuffer.end(), 0.0f);
            
            // Compute FFT
            std::vector<std::complex<float>> fftData(fftSize);
            for (size_t i = 0; i < fftSize; ++i) {
                fftData[i] = std::complex<float>(fftBuffer[i], 0.0f);
            }
            
            segment->fft->performFrequencyOnlyForwardTransform(reinterpret_cast<float*>(fftData.data()));
            segment->partitionFFTs[p] = fftData;
            
            offset += partitionSize;
            if (offset >= irLength) break;
        }
    }
}

void NonUniformPartitionedConvolution::process(const float* input, float* output, int numSamples) {
    if (!m_isReady) {
        std::copy(input, input + numSamples, output);
        return;
    }
    
    // Clear accumulator
    std::fill(m_accumulator.begin(), m_accumulator.begin() + numSamples, 0.0f);
    
    // Process each segment
    for (auto& segment : m_segments) {
        segment->processPartition(input, m_accumulator.data(), numSamples);
    }
    
    // Copy result to output
    std::copy(m_accumulator.begin(), m_accumulator.begin() + numSamples, output);
}

void NonUniformPartitionedConvolution::processBlock(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        process(buffer.getReadPointer(ch), buffer.getWritePointer(ch), numSamples);
    }
}

size_t NonUniformPartitionedConvolution::nextPowerOfTwo(size_t n) const {
    size_t power = 1;
    while (power < n) {
        power *= 2;
    }
    return power;
}

void NonUniformPartitionedConvolution::normalizeImpulseResponse(std::vector<float>& ir) {
    // Find peak
    float peak = 0.0f;
    for (float sample : ir) {
        peak = std::max(peak, std::abs(sample));
    }
    
    // Normalize to 0.5 peak
    if (peak > 0.0f) {
        float scale = 0.5f / peak;
        for (float& sample : ir) {
            sample *= scale;
        }
    }
}

//==============================================================================
// ConvolutionSegment Implementation
//==============================================================================

void NonUniformPartitionedConvolution::ConvolutionSegment::prepare(size_t partSize, int numParts) {
    partitionSize = partSize;
    numPartitions = numParts;
    fftSize = partSize * 2; // For overlap-save method
    
    // Create FFT processor
    int fftOrder = 0;
    size_t testSize = 1;
    while (testSize < fftSize) {
        testSize *= 2;
        fftOrder++;
    }
    fft = std::make_unique<juce::dsp::FFT>(fftOrder);
    
    // Allocate buffers
    partitionFFTs.resize(numPartitions);
    for (auto& fftData : partitionFFTs) {
        fftData.resize(fftSize);
    }
    
    inputHistory.resize(numPartitions);
    for (auto& history : inputHistory) {
        history.resize(fftSize);
    }
    
    inputFFT.resize(fftSize);
    overlapBuffer.resize(fftSize);
    fftBuffer.resize(fftSize);
    
    reset();
}

void NonUniformPartitionedConvolution::ConvolutionSegment::reset() {
    for (auto& history : inputHistory) {
        std::fill(history.begin(), history.end(), std::complex<float>(0.0f, 0.0f));
    }
    
    std::fill(overlapBuffer.begin(), overlapBuffer.end(), 0.0f);
    std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
    
    historyIndex = 0;
    overlapIndex = 0;
}

void NonUniformPartitionedConvolution::ConvolutionSegment::processPartition(
    const float* input, float* output, int numSamples) {
    
    // This is a simplified implementation
    // In practice, we'd need to handle block processing more carefully
    
    for (int sample = 0; sample < numSamples; ++sample) {
        // Add input to buffer
        fftBuffer[overlapIndex] = input[sample];
        
        // When buffer is full, process FFT
        if (++overlapIndex >= partitionSize) {
            overlapIndex = 0;
            
            // Prepare FFT input (zero-pad to fftSize)
            std::vector<std::complex<float>> fftInput(fftSize);
            for (size_t i = 0; i < fftSize; ++i) {
                fftInput[i] = (i < partitionSize) ? 
                    std::complex<float>(fftBuffer[i], 0.0f) : 
                    std::complex<float>(0.0f, 0.0f);
            }
            
            // Forward FFT
            fft->performFrequencyOnlyForwardTransform(reinterpret_cast<float*>(fftInput.data()));
            
            // Store in history
            inputHistory[historyIndex] = fftInput;
            
            // Convolution in frequency domain
            std::vector<std::complex<float>> fftOutput(fftSize, std::complex<float>(0.0f, 0.0f));
            
            for (int p = 0; p < numPartitions; ++p) {
                int histIdx = (historyIndex - p + numPartitions) % numPartitions;
                
                // Complex multiplication
                for (size_t k = 0; k < fftSize; ++k) {
                    fftOutput[k] += inputHistory[histIdx][k] * partitionFFTs[p][k];
                }
            }
            
            // Inverse FFT
            fft->performFrequencyOnlyInverseTransform(reinterpret_cast<float*>(fftOutput.data()));
            
            // Extract real part and scale
            float scale = 1.0f / fftSize;
            for (size_t i = 0; i < partitionSize; ++i) {
                overlapBuffer[i] = fftOutput[i].real() * scale;
            }
            
            // Update history index
            historyIndex = (historyIndex + 1) % numPartitions;
        }
        
        // Add from overlap buffer to output
        if (overlapIndex < partitionSize) {
            output[sample] += overlapBuffer[overlapIndex];
        }
    }
}

//==============================================================================
// OptimizedConvolutionSegment Implementation
//==============================================================================

OptimizedConvolutionSegment::OptimizedConvolutionSegment(size_t partitionSize, size_t numPartitions) 
    : m_partitionSize(partitionSize)
    , m_numPartitions(numPartitions) {
    
    // Calculate FFT size (next power of 2 >= 2 * partitionSize)
    m_fftSize = 1;
    while (m_fftSize < partitionSize * 2) {
        m_fftSize *= 2;
    }
    
    // Calculate FFT order
    int fftOrder = 0;
    size_t size = 1;
    while (size < m_fftSize) {
        size *= 2;
        fftOrder++;
    }
    
    // Create FFT processor
    m_fft = std::make_unique<juce::dsp::FFT>(fftOrder);
    
    // Allocate aligned buffers
    m_fftWorkspace.resize(m_fftSize * 2);
    m_frequencyDomain.resize(m_fftSize);
    m_overlapBuffer.resize(m_fftSize);
    
    // Allocate partition storage
    m_partitions.resize(numPartitions);
    for (auto& partition : m_partitions) {
        partition.spectrum.resize(m_fftSize);
    }
    
    // Allocate input history
    m_inputSpectrumHistory.resize(numPartitions);
    for (auto& spectrum : m_inputSpectrumHistory) {
        spectrum.resize(m_fftSize);
    }
    
    reset();
}

void OptimizedConvolutionSegment::reset() {
    std::fill(m_fftWorkspace.begin(), m_fftWorkspace.end(), 0.0f);
    std::fill(m_overlapBuffer.begin(), m_overlapBuffer.end(), 0.0f);
    
    for (auto& spectrum : m_inputSpectrumHistory) {
        std::fill(spectrum.begin(), spectrum.end(), std::complex<float>(0.0f, 0.0f));
    }
    
    m_historyWritePos = 0;
    m_overlapPos = 0;
}

void OptimizedConvolutionSegment::loadIRPartitions(const float* ir, size_t offset, size_t numPartitions) {
    for (size_t p = 0; p < numPartitions && p < m_numPartitions; ++p) {
        // Copy IR partition to workspace
        std::fill(m_fftWorkspace.begin(), m_fftWorkspace.end(), 0.0f);
        
        size_t irOffset = offset + p * m_partitionSize;
        for (size_t i = 0; i < m_partitionSize; ++i) {
            m_fftWorkspace[i * 2] = ir[irOffset + i]; // Real part
            m_fftWorkspace[i * 2 + 1] = 0.0f;        // Imaginary part
        }
        
        // Forward FFT
        m_fft->performFrequencyOnlyForwardTransform(m_fftWorkspace.data());
        
        // Store spectrum
        for (size_t i = 0; i < m_fftSize; ++i) {
            m_partitions[p].spectrum[i] = std::complex<float>(
                m_fftWorkspace[i * 2],
                m_fftWorkspace[i * 2 + 1]
            );
        }
    }
}

void OptimizedConvolutionSegment::complexMultiplyAccumulate(
    std::complex<float>* result,
    const std::complex<float>* a,
    const std::complex<float>* b,
    size_t count) {
    
    // SIMD-optimized complex multiply-accumulate
    // This would benefit from explicit SIMD intrinsics
    for (size_t i = 0; i < count; ++i) {
        result[i] += a[i] * b[i];
    }
}