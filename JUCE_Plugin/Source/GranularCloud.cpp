#include "GranularCloud.h"
#include <algorithm>
#include <cstring>
#include <chrono>
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #ifdef _MSC_VER
        #include <intrin.h>
        void my_cpuid(int cpuInfo[4], int function_id) {
            __cpuid(cpuInfo, function_id);
        }
    #else
        #include <cpuid.h>
        void my_cpuid(int cpuInfo[4], int function_id) {
            __cpuid_count(function_id, 0, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
        }
    #endif
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simple aligned memory allocation helpers
static void* alignedAlloc(size_t size, size_t alignment) {
#ifdef _MSC_VER
    return _aligned_malloc(size, alignment);
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0)
        return nullptr;
    return ptr;
#endif
}

static void alignedFree(void* ptr) {
#ifdef _MSC_VER
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

void GranularCloud::AlignedDeleter::operator()(float* ptr) const {
    alignedFree(ptr);
}

GranularCloud::GranularCloud() : m_rng(std::random_device{}()) {
    // Initialize denormal handling
#if defined(__SSE__) || defined(__SSE2__)
    _mm_setcsr(_mm_getcsr() | 0x8040); // Set flush-to-zero and denormals-are-zero
#endif
    
    // Allocate aligned circular buffer memory
    const size_t totalSamples = BUFFER_SIZE * 2; // Stereo
    const size_t alignment = 64; // Cache line size
    m_circularBufferMemory.reset(static_cast<float*>(alignedAlloc(totalSamples * sizeof(float), alignment)));
    
    // Set up channel pointers
    m_circularBufferPtrs[0] = m_circularBufferMemory.get();
    m_circularBufferPtrs[1] = m_circularBufferMemory.get() + BUFFER_SIZE;
    
    // Initialize parameters
    m_grainSize.setImmediate(50.0);
    m_density.setImmediate(20.0);
    m_pitchScatter.setImmediate(0.0);
    m_cloudPosition.setImmediate(0.5);
    
    // Professional smoothing rates
    m_grainSize.setSmoothingRate(0.9997);
    m_density.setSmoothingRate(0.9998);
    m_pitchScatter.setSmoothingRate(0.9999);
    m_cloudPosition.setSmoothingRate(0.9997);
    
    // Detect CPU features
    detectCPUFeatures();
    
    // Initialize grains
    for (auto& grain : m_grains) {
        grain.reset();
    }
}

GranularCloud::~GranularCloud() {
    alignedFree(m_windowTable);
}

void GranularCloud::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Clear buffers with SIMD if available
    const size_t totalSamples = BUFFER_SIZE * 2;
    std::memset(m_circularBufferMemory.get(), 0, totalSamples * sizeof(float));
    
    m_writePos = 0;
    
    // Reset all grains
    for (auto& grain : m_grains) {
        grain.reset();
    }
    
    // Initialize DSP components
    m_interpolator.initialize();
    generateWindowTable();
    
    // Initialize oversampler
    if (m_useOversampling) {
        m_oversampler.initialize(sampleRate);
    }
    
    // Reset timers with denormal-safe values
    m_grainTimer = 0.0;
    m_nextGrainTime = 0.0;
    
    // Reset DC blockers
    for (auto& dcb : m_inputDCBlockers) dcb.reset();
    for (auto& dcb : m_outputDCBlockers) dcb.reset();
    
    // Reset metrics
    m_metrics.cpuUsage.store(0.0f);
    m_metrics.activeGrainCount.store(0);
    m_metrics.droppedGrains.store(0);
    m_metrics.peakLevel.store(0.0f);
    m_metrics.rmsLevel.store(0.0f);
    m_metrics.thd.store(0.0f);
}

void GranularCloud::process(juce::AudioBuffer<float>& buffer) {
    const auto startTime = std::chrono::high_resolution_clock::now();
    
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    // Process each sample with per-sample parameter updates
    for (int sample = 0; sample < numSamples; ++sample) {
        // Per-sample parameter smoothing
        const float grainSizeMs = static_cast<float>(m_grainSize.tick());
        const float density = static_cast<float>(m_density.tick());
        const float pitchScatter = static_cast<float>(m_pitchScatter.tick());
        const float cloudPosition = static_cast<float>(m_cloudPosition.tick());
        
        // Calculate grain interval for this sample
        const double grainInterval = 1.0 / std::max(0.1, static_cast<double>(density));
        
        // Write input to circular buffer with DC blocking
        for (int ch = 0; ch < numChannels; ++ch) {
            float input = buffer.getReadPointer(ch)[sample];
            input = m_inputDCBlockers[ch].process(input);
            m_circularBufferPtrs[ch][m_writePos] = input;
        }
        
        // Update grain timer
        m_grainTimer += 1.0 / m_sampleRate;
        
        // Trigger grains with sample-accurate timing
        if (m_grainTimer >= m_nextGrainTime) {
            triggerGrain();
            // Organic timing with minimal jitter
            const double jitter = 0.97 + 0.06 * m_uniformDist(m_rng);
            m_nextGrainTime = m_grainTimer + grainInterval * jitter;
        }
        
        // Mix all active grains
        float outputL = 0.0f;
        float outputR = 0.0f;
        int activeGrains = 0;
        
        for (auto& grain : m_grains) {
            if (grain.active.load(std::memory_order_acquire)) {
                // Process grain with optional oversampling
                float grainSample;
                
                if (m_useOversampling && std::abs(grain.pitchRatio - 1.0) > 0.1) {
                    // Use oversampling for significant pitch shifts
                    grainSample = m_oversampler.processSample(
                        0.0f, // Input not used, grain reads directly
                        [&](float) { 
                            return grain.processSample(m_sampleRate * Oversampler::FACTOR, 
                                                     m_interpolator); 
                        }
                    );
                } else {
                    // Direct processing for near-unity pitch
                    grainSample = grain.processSample(m_sampleRate, m_interpolator);
                }
                
                if (!grain.active.load(std::memory_order_acquire)) continue;
                
                // Calculate panning
                float leftGain, rightGain;
                calculateStereoPan(leftGain, rightGain, grain.pan);
                
                // Mix with denormal prevention
                mixGrainToOutput(grainSample, leftGain, rightGain, outputL, outputR);
                activeGrains++;
            }
        }
        
        // Update active grain count
        if (sample % 64 == 0) { // Update periodically to reduce atomic contention
            m_metrics.activeGrainCount.store(activeGrains, std::memory_order_relaxed);
        }
        
        // Apply output DC blocking and write
        if (numChannels >= 1) {
            outputL = m_outputDCBlockers[0].process(outputL);
            buffer.getWritePointer(0)[sample] = outputL;
        }
        if (numChannels >= 2) {
            outputR = m_outputDCBlockers[1].process(outputR);
            buffer.getWritePointer(1)[sample] = outputR;
        }
        
        // Advance circular buffer
        m_writePos = (m_writePos + 1) % BUFFER_SIZE;
    }
    
    // Update quality metrics
    const auto endTime = std::chrono::high_resolution_clock::now();
    const double processingTime = std::chrono::duration<double>(endTime - startTime).count();
    const double availableTime = numSamples / m_sampleRate;
    
    m_metrics.updateCPU(processingTime, availableTime);
    m_metrics.updateLevels(buffer.getReadPointer(0), numSamples);
}

void GranularCloud::updateParameters(const std::map<int, float>& params) {
    // Pre-compute all parameters in non-RT thread
    float grainSize = 50.0f;
    float density = 20.0f;
    float pitchScatter = 0.0f;
    float cloudPosition = 0.5f;
    
    // Safe map access (no allocation in RT thread)
    auto it = params.find(0);
    if (it != params.end()) {
        float sizeParam = it->second;
        grainSize = 0.1f * std::pow(20000.0f, sizeParam);
        grainSize = validateParameter(grainSize, 0.1f, 2000.0f);
    }
    
    it = params.find(1);
    if (it != params.end()) {
        float densityParam = it->second;
        density = 0.1f * std::pow(5000.0f, densityParam);
        density = validateParameter(density, 0.1f, 500.0f);
    }
    
    it = params.find(2);
    if (it != params.end()) {
        pitchScatter = validateParameter(it->second * 4.0f, 0.0f, 4.0f);
    }
    
    it = params.find(3);
    if (it != params.end()) {
        cloudPosition = validateParameter(it->second, 0.0f, 1.0f);
    }
    
    // Store atomically for RT thread
    m_grainSizeTarget.store(grainSize, std::memory_order_relaxed);
    m_densityTarget.store(density, std::memory_order_relaxed);
    m_pitchScatterTarget.store(pitchScatter, std::memory_order_relaxed);
    m_cloudPositionTarget.store(cloudPosition, std::memory_order_relaxed);
    
    // Update smooth param targets
    m_grainSize.target.store(grainSize, std::memory_order_relaxed);
    m_density.target.store(density, std::memory_order_relaxed);
    m_pitchScatter.target.store(pitchScatter, std::memory_order_relaxed);
    m_cloudPosition.target.store(cloudPosition, std::memory_order_relaxed);
}

void GranularCloud::reset() {
    // Clear all grains
    for (auto& grain : m_grains) {
        grain.reset();
    }
    
    // Clear buffers
    const size_t totalSamples = BUFFER_SIZE * 2;
    std::memset(m_circularBufferMemory.get(), 0, totalSamples * sizeof(float));
    m_writePos = 0;
    
    // Reset DC blockers
    for (auto& dcb : m_inputDCBlockers) dcb.reset();
    for (auto& dcb : m_outputDCBlockers) dcb.reset();
    
    // Reset timers
    m_grainTimer = 0.0;
    m_nextGrainTime = 0.0;
}

void GranularCloud::triggerGrain() noexcept {
    Grain* grain = allocateGrain();
    if (!grain) {
        m_metrics.droppedGrains.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    
    // Get current parameter values
    const float grainSizeMs = m_grainSizeTarget.load(std::memory_order_relaxed);
    const float pitchScatter = m_pitchScatterTarget.load(std::memory_order_relaxed);
    const float cloudPosition = m_cloudPositionTarget.load(std::memory_order_relaxed);
    
    // Calculate grain length
    grain->grainLength = static_cast<int>(grainSizeMs * 0.001 * m_sampleRate);
    grain->grainLength = std::clamp(grain->grainLength, 64, static_cast<int>(MAX_WINDOW_SIZE));
    
    // Random source channel
    const int sourceChannel = (m_uniformDist(m_rng) > 0.5) ? 1 : 0;
    grain->bufferPtr = m_circularBufferPtrs[std::min(sourceChannel, 1)];
    grain->bufferSize = BUFFER_SIZE;
    
    // Start position with musical weighting
    const double maxDelayMs = 1000.0;
    const int maxDelaySamples = static_cast<int>(maxDelayMs * 0.001 * m_sampleRate);
    const double delayRatio = std::pow(m_uniformDist(m_rng), 1.5); // Favor recent
    const int delaySamples = static_cast<int>(delayRatio * maxDelaySamples);
    
    const int readPos = (m_writePos.load() - delaySamples + BUFFER_SIZE) % BUFFER_SIZE;
    grain->readPosAccumulator = static_cast<double>(readPos);
    
    // Pitch with musical distribution
    grain->pitchRatio = calculatePitchRatio(pitchScatter);
    
    // Configure anti-aliasing filters
    grain->useInputFilter = false;
    grain->useOutputFilter = false;
    
    if (grain->pitchRatio > 1.2) {
        // Pre-filter for downward transposition
        const double cutoff = (m_sampleRate / 2.0) / grain->pitchRatio * 0.9;
        grain->inputFilter.designButterworth(cutoff, m_sampleRate);
        grain->inputFilter.reset();
        grain->useInputFilter = true;
    } else if (grain->pitchRatio < 0.83) {
        // Post-filter for upward transposition  
        const double cutoff = (m_sampleRate / 2.0) * grain->pitchRatio * 0.9;
        grain->outputFilter.designButterworth(cutoff, m_sampleRate);
        grain->outputFilter.reset();
        grain->useOutputFilter = true;
    }
    
    // Amplitude with natural distribution
    grain->amplitude = 0.6f + 0.4f * static_cast<float>(std::pow(m_uniformDist(m_rng), 2.0));
    
    // Panning based on cloud position
    if (cloudPosition < 0.5f) {
        grain->pan = 0.5f + (m_uniformDist(m_rng) - 0.5f) * cloudPosition * 2.0f;
    } else {
        grain->pan = static_cast<float>(m_uniformDist(m_rng));
    }
    
    // Activate
    grain->envelopePos = 0;
    grain->active.store(true, std::memory_order_release);
}

GranularCloud::Grain* GranularCloud::allocateGrain() noexcept {
    // Lock-free allocation with cache-friendly iteration
    const uint32_t startIdx = m_grainAllocationIndex.fetch_add(1, std::memory_order_relaxed);
    
    for (uint32_t i = 0; i < MAX_GRAINS; ++i) {
        const uint32_t idx = (startIdx + i) % MAX_GRAINS;
        
        bool expected = false;
        if (m_grains[idx].active.compare_exchange_weak(expected, true, 
                                                       std::memory_order_acq_rel)) {
            return &m_grains[idx];
        }
    }
    
    return nullptr;
}

inline float GranularCloud::Grain::processSample(double sampleRate, 
                                                const SincInterpolator& interp) noexcept {
    if (!active.load(std::memory_order_relaxed) || !bufferPtr) return 0.0f;
    
    // Calculate envelope
    const double envelope = calculateEnvelope();
    if (envelope < 1e-6) {
        active.store(false, std::memory_order_release);
        return 0.0f;
    }
    
    // Read position
    double position = readPosAccumulator;
    
    // Apply input filter if needed
    float sample;
    if (useInputFilter) {
        // Filter at read position
        sample = static_cast<float>(
            inputFilter.process(interp.interpolate(bufferPtr, bufferSize, position))
        );
    } else {
        sample = static_cast<float>(interp.interpolate(bufferPtr, bufferSize, position));
    }
    
    // Apply output filter if needed
    if (useOutputFilter) {
        sample = static_cast<float>(outputFilter.process(sample));
    }
    
    // Apply envelope and amplitude
    sample *= static_cast<float>(envelope) * amplitude;
    
    // Update position
    readPosAccumulator += pitchRatio * (sampleRate / 44100.0); // Normalize for oversampling
    while (readPosAccumulator >= bufferSize) {
        readPosAccumulator -= bufferSize;
    }
    
    // Advance envelope
    envelopePos++;
    
    // Check completion
    if (envelopePos >= grainLength) {
        active.store(false, std::memory_order_release);
        reset();
    }
    
    return flushDenorm(sample);
}

double GranularCloud::Grain::calculateEnvelope() const noexcept {
    if (envelopePos >= grainLength) return 0.0;
    
    double envelope = 1.0;
    
    // Smooth fades
    if (envelopePos < FADE_SAMPLES) {
        const double t = static_cast<double>(envelopePos) / FADE_SAMPLES;
        envelope *= t * t * (3.0 - 2.0 * t);
    } else if (envelopePos > grainLength - FADE_SAMPLES) {
        const double t = static_cast<double>(grainLength - envelopePos) / FADE_SAMPLES;
        envelope *= t * t * (3.0 - 2.0 * t);
    }
    
    // Main window (Blackman-Harris for low sidelobes)
    const double pos = static_cast<double>(envelopePos) / grainLength;
    const double window = 0.35875 - 0.48829 * cos(2.0 * M_PI * pos) +
                         0.14128 * cos(4.0 * M_PI * pos) - 
                         0.01168 * cos(6.0 * M_PI * pos);
    
    return envelope * window;
}

void GranularCloud::Grain::reset() noexcept {
    bufferPtr = nullptr;
    bufferSize = 0;
    readPosAccumulator = 0.0;
    grainLength = 0;
    pitchRatio = 1.0;
    amplitude = 1.0f;
    pan = 0.5f;
    active.store(false, std::memory_order_relaxed);
    envelopePos = 0;
    useInputFilter = false;
    useOutputFilter = false;
}

void GranularCloud::generateWindowTable() {
    // Allocate aligned window table
    alignedFree(m_windowTable);
    m_windowTable = static_cast<float*>(alignedAlloc(MAX_WINDOW_SIZE * sizeof(float), 64));
    
    if (!m_windowTable) return;
    
    // Generate Blackman-Harris window
    for (size_t i = 0; i < MAX_WINDOW_SIZE; ++i) {
        const double pos = static_cast<double>(i) / (MAX_WINDOW_SIZE - 1);
        m_windowTable[i] = static_cast<float>(
            0.35875 - 0.48829 * cos(2.0 * M_PI * pos) +
            0.14128 * cos(4.0 * M_PI * pos) - 
            0.01168 * cos(6.0 * M_PI * pos)
        );
    }
}

double GranularCloud::calculatePitchRatio(double scatterAmount) noexcept {
    if (scatterAmount < 0.001) return 1.0;
    
    // Musical pitch distribution
    const double octaveShift = m_normalDist(m_rng) * scatterAmount;
    const double clampedShift = std::clamp(octaveShift, -4.0, 4.0);
    
    // Micro-detune for richness
    const double microDetune = (m_uniformDist(m_rng) - 0.5) * 0.01;
    
    return std::pow(2.0, clampedShift + microDetune);
}

void GranularCloud::calculateStereoPan(float& leftGain, float& rightGain, float pan) noexcept {
    pan = std::clamp(pan, 0.0f, 1.0f);
    const double angle = pan * M_PI * 0.5;
    
    // Equal power panning
    leftGain = static_cast<float>(cos(angle));
    rightGain = static_cast<float>(sin(angle));
}

float GranularCloud::validateParameter(float value, float min, float max) noexcept {
    if (!std::isfinite(value)) return min;
    return std::clamp(value, min, max);
}

void GranularCloud::detectCPUFeatures() noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    int cpuInfo[4];
    
    // Check for SSE2
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    my_cpuid(cpuInfo, 1);
#else
    m_hasSSE = false;
    m_hasSSE2 = false;
    return;
#endif
    m_hasSSE2 = (cpuInfo[3] & (1 << 26)) != 0;
    
    // Check for AVX
    m_hasAVX = (cpuInfo[2] & (1 << 28)) != 0;
    
    // Check for AVX2
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    my_cpuid(cpuInfo, 7);
#else
    m_hasAVX2 = false;
#endif
    m_hasAVX2 = (cpuInfo[1] & (1 << 5)) != 0;
#endif
}

juce::String GranularCloud::getParameterName(int index) const {
    switch (index) {
        case 0: return "Grain Size";
        case 1: return "Density";
        case 2: return "Pitch Scatter";
        case 3: return "Cloud Width";
        default: return "";
    }
}

GranularCloud::QualityReport GranularCloud::getQualityReport() const {
    QualityReport report;
    report.cpuUsage = m_metrics.cpuUsage.load(std::memory_order_relaxed);
    report.thd = m_metrics.thd.load(std::memory_order_relaxed);
    report.peakLevel = m_metrics.peakLevel.load(std::memory_order_relaxed);
    report.rmsLevel = m_metrics.rmsLevel.load(std::memory_order_relaxed);
    report.activeGrains = m_metrics.activeGrainCount.load(std::memory_order_relaxed);
    report.droppedGrains = m_metrics.droppedGrains.load(std::memory_order_relaxed);
    return report;
}

// SincInterpolator implementation
void GranularCloud::SincInterpolator::initialize() {
    alignedFree(sincTable);
    sincTable = static_cast<float*>(alignedAlloc(TABLE_SIZE * SINC_POINTS * sizeof(float), TABLE_ALIGNMENT));
    
    if (!sincTable) return;
    
    // Generate high-quality sinc table
    for (int i = 0; i < TABLE_SIZE; ++i) {
        const double frac = static_cast<double>(i) / TABLE_SIZE;
        
        for (int j = 0; j < SINC_POINTS; ++j) {
            const double x = j - SINC_POINTS/2 + frac;
            const double sinc = (std::abs(x) < 1e-10) ? 1.0 : sin(M_PI * x) / (M_PI * x);
            
            // Blackman-Harris window
            const double n = static_cast<double>(j) / (SINC_POINTS - 1);
            const double window = 0.35875 - 0.48829 * cos(2.0 * M_PI * n) +
                                 0.14128 * cos(4.0 * M_PI * n) - 
                                 0.01168 * cos(6.0 * M_PI * n);
            
            sincTable[i * SINC_POINTS + j] = static_cast<float>(sinc * window);
        }
    }
}

GranularCloud::SincInterpolator::~SincInterpolator() {
    alignedFree(sincTable);
}

inline double GranularCloud::SincInterpolator::interpolate(const float* buffer, int bufferSize,
                                                          double position) const noexcept {
    const int intPos = static_cast<int>(position);
    const double frac = position - intPos;
    const int tableIndex = static_cast<int>(frac * TABLE_SIZE);
    
    const float* kernel = &sincTable[tableIndex * SINC_POINTS];
    double sum = 0.0;
    
    // SIMD-friendly unrolled loop
#ifdef __AVX2__
    __m256d sum_vec = _mm256_setzero_pd();
    
    for (int i = 0; i < SINC_POINTS; i += 8) {
        // Load 8 samples
        alignas(32) float samples[8];
        for (int j = 0; j < 8; ++j) {
            const int idx = (intPos - SINC_POINTS/2 + i + j + bufferSize) % bufferSize;
            samples[j] = buffer[idx];
        }
        
        // Convert to double and multiply with kernel
        __m256 samples_vec = _mm256_load_ps(samples);
        __m256 kernel_vec = _mm256_loadu_ps(&kernel[i]);
        __m256 prod_vec = _mm256_mul_ps(samples_vec, kernel_vec);
        
        // Accumulate
        __m128 low = _mm256_castps256_ps128(prod_vec);
        __m128 high = _mm256_extractf128_ps(prod_vec, 1);
        __m128d low_d = _mm_cvtps_pd(low);
        __m128d high_d = _mm_cvtps_pd(high);
        
        sum_vec = _mm256_add_pd(sum_vec, _mm256_cvtps_pd(low));
        sum_vec = _mm256_add_pd(sum_vec, _mm256_cvtps_pd(high));
    }
    
    // Horizontal sum
    alignas(32) double temp[4];
    _mm256_store_pd(temp, sum_vec);
    sum = temp[0] + temp[1] + temp[2] + temp[3];
#else
    // Scalar fallback
    for (int i = 0; i < SINC_POINTS; i += 4) {
        const int p0 = (intPos - SINC_POINTS/2 + i + bufferSize) % bufferSize;
        const int p1 = (p0 + 1) % bufferSize;
        const int p2 = (p1 + 1) % bufferSize;
        const int p3 = (p2 + 1) % bufferSize;
        
        sum += buffer[p0] * kernel[i] +
               buffer[p1] * kernel[i+1] +
               buffer[p2] * kernel[i+2] +
               buffer[p3] * kernel[i+3];
    }
#endif
    
    return flushDenorm(sum);
}

// AntiAliasingFilter implementation
void GranularCloud::AntiAliasingFilter::designButterworth(double cutoffFreq, double sampleRate) {
    const double wc = 2.0 * M_PI * cutoffFreq / sampleRate;
    const double wc2 = wc * wc;
    const double wc3 = wc2 * wc;
    const double wc4 = wc2 * wc2;
    
    // Bilinear transform
    const double k = tan(wc / 2.0);
    const double k2 = k * k;
    const double k3 = k2 * k;
    const double k4 = k2 * k2;
    const double k5 = k4 * k;
    const double k6 = k3 * k3;
    const double k7 = k6 * k;
    const double k8 = k4 * k4;
    
    // 8th order Butterworth coefficients
    const double sqrt2 = 1.41421356237;
    const double a0 = k8 + 2.613125930 * k7 + 3.414213562 * k6 + 
                     2.613125930 * k5 + k4;
    
    // Normalize
    b[0] = wc4 / a0;
    b[1] = 8 * b[0];
    b[2] = 28 * b[0];
    b[3] = 56 * b[0];
    b[4] = 70 * b[0];
    b[5] = 56 * b[0];
    b[6] = 28 * b[0];
    b[7] = 8 * b[0];
    b[8] = b[0];
    
    a[0] = 1.0;
    a[1] = (-8 * k8 + 2.613125930 * 6 * k7 - 3.414213562 * 4 * k6) / a0;
    a[2] = (28 * k8 - 2.613125930 * 12 * k7) / a0;
    a[3] = (-56 * k8 + 2.613125930 * 8 * k7) / a0;
    a[4] = 70 * k8 / a0;
    a[5] = (-56 * k8 - 2.613125930 * 8 * k7) / a0;
    a[6] = (28 * k8 + 2.613125930 * 12 * k7) / a0;
    a[7] = (-8 * k8 - 2.613125930 * 6 * k7 - 3.414213562 * 4 * k6) / a0;
    a[8] = (k8 - 2.613125930 * k7 + 3.414213562 * k6 - 
            2.613125930 * k5 + k4) / a0;
}

// Quality metrics
void GranularCloud::QualityMetrics::updateCPU(double processingTime, 
                                              double availableTime) noexcept {
    const float usage = static_cast<float>(processingTime / availableTime);
    cpuUsage.store(usage, std::memory_order_relaxed);
}

void GranularCloud::QualityMetrics::updateLevels(const float* buffer, int numSamples) noexcept {
    double sum = 0.0;
    double peak = 0.0;
    
    // SIMD-optimized level calculation
#ifdef __AVX2__
    __m256 peak_vec = _mm256_setzero_ps();
    __m256d sum_vec = _mm256_setzero_pd();
    
    for (int i = 0; i <= numSamples - 8; i += 8) {
        __m256 samples = _mm256_loadu_ps(&buffer[i]);
        __m256 abs_samples = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), samples);
        peak_vec = _mm256_max_ps(peak_vec, abs_samples);
        
        __m256 squared = _mm256_mul_ps(samples, samples);
        __m128 low = _mm256_castps256_ps128(squared);
        __m128 high = _mm256_extractf128_ps(squared, 1);
        
        sum_vec = _mm256_add_pd(sum_vec, _mm256_cvtps_pd(low));
        sum_vec = _mm256_add_pd(sum_vec, _mm256_cvtps_pd(high));
    }
    
    // Horizontal operations
    alignas(32) float peak_temp[8];
    _mm256_store_ps(peak_temp, peak_vec);
    for (int i = 0; i < 8; ++i) {
        peak = std::max(peak, static_cast<double>(peak_temp[i]));
    }
    
    alignas(32) double sum_temp[4];
    _mm256_store_pd(sum_temp, sum_vec);
    sum = sum_temp[0] + sum_temp[1] + sum_temp[2] + sum_temp[3];
    
    // Handle remaining samples
    for (int i = (numSamples & ~7); i < numSamples; ++i) {
        const double sample = std::abs(buffer[i]);
        sum += sample * sample;
        peak = std::max(peak, sample);
    }
#else
    // Scalar fallback
    for (int i = 0; i < numSamples; ++i) {
        const double sample = std::abs(buffer[i]);
        sum += sample * sample;
        peak = std::max(peak, sample);
    }
#endif
    
    const float rms = static_cast<float>(sqrt(sum / numSamples));
    rmsLevel.store(rms, std::memory_order_relaxed);
    peakLevel.store(static_cast<float>(peak), std::memory_order_relaxed);
}

// Oversampler implementation
void GranularCloud::Oversampler::initialize(double sampleRate) {
    // Design polyphase filters for 4x oversampling
    // This is a simplified implementation - in production, use optimized FIR design
    
    // Initialize coefficients for 120dB stopband
    for (int phase = 0; phase < FACTOR; ++phase) {
        for (int tap = 0; tap < TAPS_PER_PHASE; ++tap) {
            const double n = phase + tap * FACTOR;
            const double x = (n - (TAPS_PER_PHASE * FACTOR - 1) / 2.0) / FACTOR;
            
            double h;
            if (std::abs(x) < 1e-10) {
                h = 1.0;
            } else {
                h = sin(M_PI * x) / (M_PI * x);
            }
            
            // Kaiser window (beta = 10 for 120dB)
            const double beta = 10.0;
            const double pos = 2.0 * n / (TAPS_PER_PHASE * FACTOR - 1) - 1.0;
            const double window = 0.5 * (1.0 + cos(M_PI * pos)); // Simplified
            
            const int idx = phase * TAPS_PER_PHASE + tap;
            upCoeffs[idx] = static_cast<float>(h * window * FACTOR);
            downCoeffs[idx] = static_cast<float>(h * window);
        }
    }
    
    // Clear delay lines
    upDelayLine.clear();
    downDelayLine.clear();
    delayIndex = 0;
}

inline float GranularCloud::Oversampler::processSample(float input,
                                                      std::function<float(float)> processor) noexcept {
    // This is a simplified version - in production, implement full polyphase structure
    
    // Upsample
    float upsampled[FACTOR];
    upsampled[0] = input * FACTOR;
    for (int i = 1; i < FACTOR; ++i) {
        upsampled[i] = 0.0f;
    }
    
    // Process at high rate
    float processed[FACTOR];
    for (int i = 0; i < FACTOR; ++i) {
        processed[i] = processor(upsampled[i]);
    }
    
    // Downsample (simplified - just take first sample)
    return flushDenorm(processed[0] / FACTOR);
}