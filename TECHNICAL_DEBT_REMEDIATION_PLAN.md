# Technical Debt Remediation Plan

## Current Coverage Analysis

### ✅ What We're Addressing (7/14 items = 50%)

| TD ID | Description | Status | Document/Plan |
|-------|------------|--------|---------------|
| TD-001 | No error handling | ✅ COVERED | FIX_CRITICAL_ERROR_HANDLING.md |
| TD-002 | Buffer overflow protection | ✅ COVERED | FIX_CRITICAL_BUFFER_OVERFLOW.md |
| TD-003 | Thread safety issues | ✅ COVERED | FIX_CRITICAL_THREAD_SAFETY.md |
| TD-006 | Hardcoded buffer sizes | ⚠️ PARTIAL | In buffer overflow fixes |
| TD-008 | Magic numbers | ⚠️ PARTIAL | DSP_ENGINE_COMPREHENSIVE_AUDIT.md |
| TD-012 | Inconsistent naming | ⚠️ PARTIAL | DSP audit will identify |
| TD-014 | No performance benchmarks | ⚠️ PARTIAL | DSP audit includes CPU testing |

### ❌ What We're NOT Addressing Yet (7/14 items)

| TD ID | Description | Why It Matters | Risk of Ignoring |
|-------|------------|----------------|------------------|
| TD-004 | No unit tests | Can't verify our fixes work | 🔴 HIGH - Regressions likely |
| TD-005 | 57-case switch | Hard to maintain/extend | 🟠 MEDIUM - Slows development |
| TD-007 | No CPU monitoring | Can't optimize performance | 🟠 MEDIUM - Performance issues |
| TD-009 | Duplicated parameters | Error prone, maintenance burden | 🟡 MEDIUM - Bugs likely |
| TD-010 | No logging framework | Hard to debug production issues | 🔴 HIGH - Can't diagnose crashes |
| TD-011 | Platform ifdefs | Complex builds | 🟡 LOW - Build failures |
| TD-013 | Missing const | Type safety issues | 🟢 LOW - Minor risk |

## Comprehensive Remediation Phases

### Phase 1: Safety Critical (Days 1-7) ✅ ALREADY PLANNED
- FIX_CRITICAL_ERROR_HANDLING.md
- FIX_CRITICAL_BUFFER_OVERFLOW.md
- FIX_CRITICAL_THREAD_SAFETY.md
- DSP_ENGINE_COMPREHENSIVE_AUDIT.md

### Phase 2: Testing Infrastructure (Days 8-10) ❌ MISSING
**TD-004: Unit Test Framework**

#### Implementation Plan:
```cpp
// 1. Create test framework structure
/Tests
  /Unit
    /Engines        # Test each engine
    /Parameters     # Test parameter handling
    /Threading      # Test thread safety
  /Integration
    /AudioChain     # Test full signal flow
    /Presets        # Test preset system
  /Performance
    /CPU            # Benchmark each engine
    /Memory         # Check for leaks
```

#### Quick Start:
```bash
# Install Catch2 or Google Test
brew install catch2

# Create first test
cat > Tests/Unit/test_error_handling.cpp << 'EOF'
#include <catch2/catch.hpp>
#include "PluginProcessor.h"

TEST_CASE("ProcessBlock handles null buffer", "[safety]") {
    ChimeraAudioProcessor processor;
    juce::AudioBuffer<float> nullBuffer;
    juce::MidiBuffer midi;

    // Should not crash
    REQUIRE_NOTHROW(processor.processBlock(nullBuffer, midi));
}
EOF
```

### Phase 3: Logging & Diagnostics (Days 11-12) ❌ MISSING
**TD-010: Logging Framework**

#### Implementation:
```cpp
// Create centralized logging system
class ChimeraLogger {
public:
    enum Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };

    static void log(Level level, const String& message) {
        #ifdef DEBUG_BUILD
            DBG(message);  // Console in debug
        #endif

        // Ring buffer for production
        getInstance().ringBuffer.push(TimestampedMessage{level, message});

        // Critical errors to file
        if (level >= ERROR) {
            getInstance().logToFile(message);
        }
    }

    // Get recent logs without file I/O
    static std::vector<TimestampedMessage> getRecentLogs(int count);
};

// Use throughout code
ChimeraLogger::log(INFO, "Engine swapped to: " + engineName);
ChimeraLogger::log(ERROR, "NaN detected in engine: " + engineName);
```

### Phase 4: Architecture Refactoring (Days 13-15) ❌ MISSING

**TD-005: Replace 57-case Switch**
```cpp
// BEFORE: Giant switch statement
switch(engineId) {
    case 1: return std::make_unique<Compressor>();
    case 2: return std::make_unique<Reverb>();
    // ... 55 more cases
}

// AFTER: Self-registering factory pattern
class EngineRegistry {
    using FactoryFunc = std::function<std::unique_ptr<EngineBase>()>;
    std::map<int, FactoryFunc> factories;

public:
    template<typename T>
    void registerEngine(int id) {
        factories[id] = []{ return std::make_unique<T>(); };
    }

    std::unique_ptr<EngineBase> create(int id) {
        auto it = factories.find(id);
        return (it != factories.end()) ? it->second() : nullptr;
    }
};

// Self-registration
namespace {
    struct CompressorRegistrar {
        CompressorRegistrar() {
            EngineRegistry::instance().registerEngine<Compressor>(1);
        }
    } compressor_reg;
}
```

**TD-009: Extract Parameter Handling**
```cpp
// Create reusable parameter extractor
class ParameterExtractor {
public:
    static std::map<int, float> extractSlotParams(
        const AudioProcessorValueTreeState& state, int slot) {
        std::map<int, float> params;
        String prefix = "slot" + String(slot + 1) + "_param";

        for (int i = 0; i < 15; ++i) {
            auto* param = state.getRawParameterValue(prefix + String(i + 1));
            params[i] = param ? param->load() : 0.0f;
        }
        return params;
    }
};
```

### Phase 5: Performance & Monitoring (Days 16-17) ❌ MISSING

**TD-007: CPU Usage Monitoring**
```cpp
class CPUMonitor {
    std::atomic<float> usage{0.0f};
    std::chrono::high_resolution_clock::time_point lastTime;

public:
    void startMeasurement() {
        lastTime = std::chrono::high_resolution_clock::now();
    }

    void endMeasurement(int samplesProcessed) {
        auto now = std::chrono::high_resolution_clock::now();
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
            now - lastTime).count();

        // Calculate percentage of available time used
        float available = (samplesProcessed / 44100.0f) * 1000000.0f;
        usage.store(100.0f * (micros / available));
    }

    float getCpuUsage() const { return usage.load(); }
};
```

**TD-014: Performance Benchmarks**
```cpp
// Automated benchmark suite
class BenchmarkSuite {
public:
    void benchmarkAllEngines() {
        for (int id = 0; id < 57; ++id) {
            auto result = benchmarkEngine(id);
            report.addResult(id, result);
        }
        report.generateHTML("benchmark_report.html");
    }

private:
    struct BenchmarkResult {
        float avgCPU;
        float peakCPU;
        float latency;
        int maxPolyphony;  // How many instances before 100% CPU
    };
};
```

### Phase 6: Code Quality (Days 18-20) ❌ MISSING

**TD-008: Remove Magic Numbers**
```cpp
// Create constants file
namespace ChimeraConstants {
    // DSP
    constexpr float NYQUIST_COEFFICIENT = 0.4978f;
    constexpr float DB_TO_LINEAR_FACTOR = 20.0f;
    constexpr float SMOOTHING_COEFFICIENT = 0.99f;

    // Limits
    constexpr int MAX_DELAY_SAMPLES = 88200;  // 2 sec @ 44.1kHz
    constexpr float MAX_FEEDBACK = 0.98f;

    // Defaults
    constexpr float DEFAULT_MIX = 0.5f;
    constexpr float DEFAULT_GAIN = 1.0f;
}
```

**TD-011: Platform Abstraction**
```cpp
// Create platform abstraction layer
namespace Platform {
    #ifdef _WIN32
        using ThreadHandle = HANDLE;
    #elif __APPLE__
        using ThreadHandle = pthread_t;
    #endif

    class Thread {
    public:
        static void setRealtime(ThreadHandle t);
        static void setPriority(ThreadHandle t, int priority);
    };
}
```

## Priority Matrix for Remaining Debt

| Priority | TD Items | Days | Impact if Fixed | Risk if Ignored |
|----------|----------|------|----------------|-----------------|
| 1️⃣ CRITICAL | TD-004 (Tests) | 5 | Can verify all fixes | Regressions |
| 2️⃣ CRITICAL | TD-010 (Logging) | 2 | Can diagnose issues | Blind debugging |
| 3️⃣ HIGH | TD-007 (CPU Monitor) | 1 | Can optimize | Poor performance |
| 4️⃣ HIGH | TD-005 (Switch refactor) | 1 | Maintainable | Hard to extend |
| 5️⃣ MEDIUM | TD-009 (Parameters) | 1 | Less bugs | Duplicate bugs |
| 6️⃣ MEDIUM | TD-008 (Magic numbers) | 2 | Readable | Confusion |
| 7️⃣ LOW | TD-011 (Platform) | 3 | Clean builds | Build issues |
| 8️⃣ LOW | TD-013 (Const) | 1 | Type safety | Minor bugs |

## Recommended Action Plan

### Week 1: Safety + Tests (Current Plan + TD-004)
- Days 1-2: Error handling ✅
- Days 3-4: Thread safety ✅
- Days 5-7: **ADD: Basic test framework** 🆕

### Week 2: Visibility + Quality
- Days 8-9: **ADD: Logging framework (TD-010)** 🆕
- Days 10: **ADD: CPU monitoring (TD-007)** 🆕
- Days 11-12: Buffer overflow fixes ✅
- Days 13-14: DSP engine audit ✅

### Week 3: Architecture + Polish
- Days 15: **ADD: Refactor switch (TD-005)** 🆕
- Days 16: **ADD: Extract parameters (TD-009)** 🆕
- Days 17-18: **ADD: Remove magic numbers (TD-008)** 🆕
- Days 19-20: Fix broken engines ✅

### Week 4: Polish + Release Prep
- Days 21-22: **ADD: Performance benchmarks (TD-014)** 🆕
- Days 23: **ADD: Platform cleanup (TD-011)** 🆕
- Days 24: **ADD: Const correctness (TD-013)** 🆕
- Days 25-26: Final testing and validation

## Metrics to Track

```markdown
# Technical Debt Scorecard

## Before Remediation
- Debt Items: 14
- Critical Issues: 3
- Code Coverage: 0%
- Crash Rate: Unknown
- CPU Usage: Unknown
- Build Warnings: [Count]

## Target After Remediation
- Debt Items: 0
- Critical Issues: 0
- Code Coverage: >80%
- Crash Rate: <0.01%
- CPU Usage: Monitored
- Build Warnings: 0

## Weekly Progress
| Week | Items Fixed | Tests Added | Coverage | Crashes |
|------|------------|-------------|----------|---------|
| 1 | 0/14 | 0 | 0% | ? |
| 2 | []/14 | [] | []% | [] |
| 3 | []/14 | [] | []% | [] |
| 4 | 14/14 | [] | []% | [] |
```

## Missing Infrastructure Costs

Not addressing these items has hidden costs:

1. **No Tests (TD-004)**: Every fix might break something else
2. **No Logging (TD-010)**: Can't diagnose customer issues
3. **No CPU Monitoring (TD-007)**: Can't prove performance claims
4. **Giant Switch (TD-005)**: Adding new engines is error-prone
5. **Duplicate Code (TD-009)**: Same bugs fixed multiple times

## Return on Investment

| Investment | Time | Return |
|------------|------|--------|
| Test Framework | 5 days | Prevent 20+ days of debugging |
| Logging | 2 days | Save 10+ days diagnosing crashes |
| CPU Monitor | 1 day | Enable performance marketing claims |
| Refactoring | 5 days | Save 2 days per new feature |

**Total Investment**: 13 additional days
**Expected Return**: 40+ days saved in next 6 months

---

## Recommendation

**YES, we should address ALL technical debt, not just critical issues.**

The current plan addresses only 50% of technical debt. Adding the missing 50% requires ~13 more days but will:
- Prevent regressions with tests
- Enable debugging with logging
- Prove performance with monitoring
- Simplify maintenance with refactoring

**Without these, the "fixed" code might still be fragile and unmaintainable.**