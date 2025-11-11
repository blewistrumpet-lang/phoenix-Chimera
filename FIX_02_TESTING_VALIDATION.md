# FIX 02: Testing & Validation Infrastructure

## Overview

Implements comprehensive testing framework (TD-004) and performance benchmarks (TD-014) to ensure all fixes work and prevent regressions.

### Timeline: 6 Days
- Day 1-2: Test framework setup
- Day 3-4: Unit test creation
- Day 5: Performance benchmarks
- Day 6: Integration and CI/CD

## Implementation Plan

### Day 1-2: Framework Setup

```bash
# Install testing dependencies
brew install catch2 google-benchmark

# Create test structure
mkdir -p Tests/{Unit,Integration,Performance,Fixtures}
```

```cmake
# CMakeLists.txt for tests
add_executable(ChimeraTests
    Tests/main.cpp
    Tests/Unit/test_engines.cpp
    Tests/Unit/test_safety.cpp
    Tests/Unit/test_parameters.cpp
    Tests/Integration/test_signal_flow.cpp
    Tests/Performance/benchmark_engines.cpp
)

target_link_libraries(ChimeraTests
    Catch2::Catch2
    benchmark::benchmark
    ChimeraPlugin
)
```

### Day 3-4: Core Test Suite

```cpp
// Tests/Unit/test_engines.cpp
#include <catch2/catch.hpp>
#include "EngineFactory.h"

TEST_CASE("All engines can be created", "[engines]") {
    for (int id = 0; id < 57; ++id) {
        auto engine = EngineFactory::createEngine(id);
        REQUIRE(engine != nullptr);
        REQUIRE(!engine->getName().isEmpty());
    }
}

TEST_CASE("Engines handle standard test signals", "[engines][audio]") {
    struct TestSignal {
        std::string name;
        std::function<void(juce::AudioBuffer<float>&)> generator;
        std::function<bool(const juce::AudioBuffer<float>&)> validator;
    };

    std::vector<TestSignal> signals = {
        {"Silence",
         [](auto& buf) { buf.clear(); },
         [](const auto& buf) { return buf.getMagnitude(0, buf.getNumSamples()) < 0.001f; }},

        {"Impulse",
         [](auto& buf) { buf.clear(); buf.setSample(0, 0, 1.0f); },
         [](const auto& buf) { return std::abs(buf.getSample(0, 0)) <= 1.0f; }},

        {"White Noise",
         [](auto& buf) {
             std::random_device rd;
             std::mt19937 gen(rd());
             std::uniform_real_distribution<> dis(-1.0, 1.0);
             for (int ch = 0; ch < buf.getNumChannels(); ++ch) {
                 for (int s = 0; s < buf.getNumSamples(); ++s) {
                     buf.setSample(ch, s, dis(gen));
                 }
             }
         },
         [](const auto& buf) {
             for (int ch = 0; ch < buf.getNumChannels(); ++ch) {
                 for (int s = 0; s < buf.getNumSamples(); ++s) {
                     if (!std::isfinite(buf.getSample(ch, s))) return false;
                     if (std::abs(buf.getSample(ch, s)) > 10.0f) return false;
                 }
             }
             return true;
         }}
    };

    for (int id = 0; id < 57; ++id) {
        auto engine = EngineFactory::createEngine(id);
        if (!engine) continue;

        engine->prepareToPlay(44100, 512);

        DYNAMIC_SECTION("Engine " << id << ": " << engine->getName().toStdString()) {
            for (const auto& signal : signals) {
                SECTION(signal.name) {
                    juce::AudioBuffer<float> buffer(2, 512);
                    signal.generator(buffer);

                    REQUIRE_NOTHROW(engine->process(buffer));
                    REQUIRE(signal.validator(buffer));
                }
            }
        }
    }
}
```

### Day 5: Performance Benchmarks

```cpp
// Tests/Performance/benchmark_engines.cpp
#include <benchmark/benchmark.h>
#include "EngineFactory.h"

class EngineBenchmark : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) {
        engineId = state.range(0);
        engine = EngineFactory::createEngine(engineId);
        if (engine) {
            engine->prepareToPlay(44100, 512);
            buffer.setSize(2, 512);
            buffer.clear();
        }
    }

    int engineId;
    std::unique_ptr<EngineBase> engine;
    juce::AudioBuffer<float> buffer;
};

BENCHMARK_DEFINE_F(EngineBenchmark, ProcessBlock)(benchmark::State& state) {
    if (!engine) {
        state.SkipWithError("Engine creation failed");
        return;
    }

    for (auto _ : state) {
        engine->process(buffer);
        benchmark::DoNotOptimize(buffer.getReadPointer(0));
    }

    state.SetItemsProcessed(state.iterations() * 512);
}

// Register benchmark for all engines
BENCHMARK_REGISTER_F(EngineBenchmark, ProcessBlock)
    ->DenseRange(0, 56, 1)
    ->Unit(benchmark::kMicrosecond);
```

### Day 6: CI/CD Integration

```yaml
# .github/workflows/test.yml
name: Test Suite

on: [push, pull_request]

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]

    steps:
    - uses: actions/checkout@v2

    - name: Install dependencies
      run: |
        if [ "$RUNNER_OS" == "macOS" ]; then
          brew install catch2 google-benchmark
        fi

    - name: Build tests
      run: |
        cmake -B build -DBUILD_TESTS=ON
        cmake --build build

    - name: Run unit tests
      run: ./build/ChimeraTests

    - name: Run benchmarks
      run: ./build/ChimeraBenchmarks --benchmark_format=json > benchmark_results.json

    - name: Upload results
      uses: actions/upload-artifact@v2
      with:
        name: test-results-${{ matrix.os }}
        path: |
          benchmark_results.json
          test_results.xml
```

## Test Coverage Requirements

### Unit Tests (500+ tests)
- [ ] Each engine creation
- [ ] Each engine with standard signals
- [ ] Parameter validation
- [ ] Thread safety
- [ ] Buffer handling
- [ ] Error conditions

### Integration Tests (50+ tests)
- [ ] Full signal chain
- [ ] Preset loading/saving
- [ ] A/B switching
- [ ] Engine swapping
- [ ] GPIO integration

### Performance Tests
- [ ] CPU usage per engine
- [ ] Memory usage
- [ ] Latency measurement
- [ ] Cache efficiency
- [ ] SIMD utilization

## Success Metrics
- >80% code coverage
- All tests pass on all platforms
- <5% CPU per engine
- <1ms latency
- 0 memory leaks