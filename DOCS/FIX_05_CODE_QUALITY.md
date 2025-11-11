# FIX 05: Code Quality Improvements

## Overview
Addresses hardcoded values (TD-006), magic numbers (TD-008), naming consistency (TD-012), and const correctness (TD-013).

### Timeline: 4 Days

## Implementation

### Day 1: Constants & Configuration

```cpp
// ChimeraConstants.h - All magic numbers in one place
namespace ChimeraConstants {

// Audio Processing
namespace Audio {
    constexpr int MAX_BLOCK_SIZE = 8192;
    constexpr int DEFAULT_BLOCK_SIZE = 512;
    constexpr double DEFAULT_SAMPLE_RATE = 44100.0;
    constexpr float DENORMAL_THRESHOLD = 1e-8f;
    constexpr float DEFAULT_MIX = 0.5f;
    constexpr float SOFT_CLIP_THRESHOLD = 0.95f;
    constexpr float HARD_LIMIT = 0.99f;
}

// DSP Constants
namespace DSP {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = PI * 0.5f;
    constexpr float DB_TO_LINEAR = 20.0f;
    constexpr float LINEAR_TO_DB = 0.05f;
    constexpr float NYQUIST_FACTOR = 0.4978f;
}

// Filter Design
namespace Filter {
    constexpr float MIN_FREQUENCY = 20.0f;
    constexpr float MAX_FREQUENCY = 20000.0f;
    constexpr float DEFAULT_Q = 0.707f;  // Butterworth
    constexpr float MAX_RESONANCE = 20.0f;
}

// Compressor/Dynamics
namespace Dynamics {
    constexpr float MIN_THRESHOLD_DB = -60.0f;
    constexpr float MAX_THRESHOLD_DB = 0.0f;
    constexpr float MIN_RATIO = 1.0f;
    constexpr float MAX_RATIO = 100.0f;
    constexpr float MIN_ATTACK_MS = 0.01f;
    constexpr float MAX_ATTACK_MS = 1000.0f;
    constexpr float MIN_RELEASE_MS = 1.0f;
    constexpr float MAX_RELEASE_MS = 5000.0f;
}

// Delay/Reverb
namespace TimeBased {
    constexpr int MAX_DELAY_SAMPLES = 88200;  // 2 sec @ 44.1kHz
    constexpr float MAX_FEEDBACK = 0.98f;
    constexpr float MAX_REVERB_TIME = 30.0f;
    constexpr int REVERB_DIFFUSION_STAGES = 4;
}

// Replace magic numbers in code:
// BEFORE: if (value > 0.95f) { /* clip */ }
// AFTER:  if (value > ChimeraConstants::Audio::SOFT_CLIP_THRESHOLD)
}
```

### Day 2: Dynamic Buffer Management

```cpp
// BufferManager.h - Replace hardcoded buffer sizes
class BufferManager {
private:
    struct BufferPool {
        std::vector<std::unique_ptr<juce::AudioBuffer<float>>> buffers;
        std::mutex mutex;
        size_t maxSize;

        BufferPool(size_t size) : maxSize(size) {
            for (size_t i = 0; i < 10; ++i) {  // Pre-allocate
                buffers.push_back(std::make_unique<juce::AudioBuffer<float>>());
            }
        }
    };

    std::map<size_t, BufferPool> pools;

public:
    class ScopedBuffer {
        BufferManager* manager;
        std::unique_ptr<juce::AudioBuffer<float>> buffer;
        size_t poolSize;

    public:
        ~ScopedBuffer() {
            if (manager && buffer) {
                manager->returnBuffer(std::move(buffer), poolSize);
            }
        }

        juce::AudioBuffer<float>* operator->() { return buffer.get(); }
        juce::AudioBuffer<float>& operator*() { return *buffer; }
    };

    ScopedBuffer getBuffer(int channels, int samples) {
        size_t size = channels * samples * sizeof(float);
        size_t poolSize = roundUpPowerOfTwo(size);

        auto buffer = borrowBuffer(poolSize);
        buffer->setSize(channels, samples, false, false, true);

        return ScopedBuffer{this, std::move(buffer), poolSize};
    }

private:
    std::unique_ptr<juce::AudioBuffer<float>> borrowBuffer(size_t size) {
        std::lock_guard<std::mutex> lock(pools[size].mutex);

        if (!pools[size].buffers.empty()) {
            auto buffer = std::move(pools[size].buffers.back());
            pools[size].buffers.pop_back();
            return buffer;
        }

        return std::make_unique<juce::AudioBuffer<float>>();
    }

    void returnBuffer(std::unique_ptr<juce::AudioBuffer<float>> buffer,
                     size_t size) {
        std::lock_guard<std::mutex> lock(pools[size].mutex);

        if (pools[size].buffers.size() < pools[size].maxSize) {
            pools[size].buffers.push_back(std::move(buffer));
        }
    }
};
```

### Day 3: Naming Consistency

```cpp
// NamingConventions.h - Enforce consistent naming
namespace Chimera {

// Type naming: PascalCase
class AudioEngine;
struct ProcessorState;
enum class EngineType;

// Function naming: camelCase
void processAudioBlock();
float calculateGainReduction();
bool isProcessingEnabled();

// Variable naming: camelCase with prefixes
class Example {
private:
    // Member variables: m_ prefix
    float m_sampleRate;
    int m_blockSize;

    // Static members: s_ prefix
    static int s_instanceCount;

    // Constants: k prefix
    static constexpr float kDefaultGain = 1.0f;

public:
    // Parameters: no prefix
    void setGain(float gain);

    // Getters: get prefix
    float getSampleRate() const;

    // Setters: set prefix
    void setSampleRate(float rate);

    // Boolean getters: is/has prefix
    bool isActive() const;
    bool hasChanged() const;
};

// Namespace conventions
namespace dsp {       // Lowercase for nested namespaces
namespace filters {   // Plural for collections
namespace detail {    // For implementation details
}}}

// File naming: SnakeCase for headers, source files
// AudioEngine.h, AudioEngine.cpp
// BufferManager.h, BufferManager.cpp
}
```

### Day 4: Const Correctness

```cpp
// ConstCorrectness.h - Proper const usage
class EngineBase {
public:
    // Const methods that don't modify state
    virtual juce::String getName() const = 0;
    virtual int getNumParameters() const = 0;
    virtual float getParameter(int index) const = 0;
    virtual int getLatencySamples() const noexcept = 0;

    // Non-const methods that modify state
    virtual void setParameter(int index, float value) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;

    // Const reference parameters for large objects
    virtual void loadPreset(const PresetData& preset) = 0;

    // Return const references when appropriate
    const std::vector<ParameterInfo>& getParameterInfos() const {
        return m_parameterInfos;
    }

protected:
    // Mutable for caching
    mutable float m_cachedValue = 0.0f;
    mutable bool m_cacheValid = false;

    float getCachedValue() const {
        if (!m_cacheValid) {
            m_cachedValue = calculateValue();
            m_cacheValid = true;
        }
        return m_cachedValue;
    }

private:
    virtual float calculateValue() const = 0;
};

// Const iterator usage
void processParameters(const std::vector<float>& params) {
    for (const auto& param : params) {  // const auto&
        // Read-only access
    }
}

// Constexpr for compile-time constants
class Constants {
public:
    static constexpr float getMaxGain() { return 2.0f; }
    static constexpr int getMaxVoices() { return 128; }
};
```

## Automated Cleanup Script

```python
#!/usr/bin/env python3
# cleanup_code_quality.py

import re
import os
from pathlib import Path

def fix_magic_numbers(filepath):
    """Replace magic numbers with named constants"""
    replacements = [
        (r'\b0\.707\b', 'ChimeraConstants::Filter::DEFAULT_Q'),
        (r'\b0\.95f?\b', 'ChimeraConstants::Audio::SOFT_CLIP_THRESHOLD'),
        (r'\b44100\b', 'ChimeraConstants::Audio::DEFAULT_SAMPLE_RATE'),
        (r'\b512\b', 'ChimeraConstants::Audio::DEFAULT_BLOCK_SIZE'),
        # Add more patterns
    ]

    with open(filepath, 'r') as f:
        content = f.read()

    for pattern, replacement in replacements:
        content = re.sub(pattern, replacement, content)

    return content

def fix_naming_convention(filepath):
    """Fix inconsistent naming"""
    # Convert member variables to m_ prefix
    content = re.sub(r'\bprivate:\s*\n\s*(\w+)\s+(\w+);',
                     r'private:\n    \1 m_\2;', content)

    # Fix getter/setter names
    content = re.sub(r'\bget_(\w+)\b', r'get\1', content)
    content = re.sub(r'\bset_(\w+)\b', r'set\1', content)

    return content

def add_const_correctness(filepath):
    """Add const where appropriate"""
    # Mark getters as const
    content = re.sub(r'(get\w+\([^)]*\))(?!\s*const)',
                     r'\1 const', content)

    # Mark parameters as const ref
    content = re.sub(r'(const )?(std::\w+<[^>]+>)(&)?(\s+\w+)',
                     r'const \2& \4', content)

    return content

# Process all source files
for filepath in Path('JUCE_Plugin/Source').glob('**/*.cpp'):
    content = fix_magic_numbers(filepath)
    content = fix_naming_convention(content)
    content = add_const_correctness(content)

    with open(filepath, 'w') as f:
        f.write(content)

print("Code quality cleanup complete!")
```

## Validation

```bash
# Check for remaining issues
grep -r "0\.707\|44100\|512\|0\.95" --include="*.cpp" --include="*.h"

# Check naming consistency
grep -r "get_\|set_\|_get\|_set" --include="*.cpp" --include="*.h"

# Check for missing const
grep -r "get.*(" --include="*.h" | grep -v const
```

## Success Metrics
- 0 magic numbers in code
- Consistent naming throughout
- All getters marked const
- Dynamic buffer allocation
- Code passes linting