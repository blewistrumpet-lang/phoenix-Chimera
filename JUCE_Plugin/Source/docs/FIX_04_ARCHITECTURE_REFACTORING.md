# FIX 04: Architecture Refactoring

## Overview
Eliminates giant switch (TD-005), parameter duplication (TD-009), and platform ifdefs (TD-011).

### Timeline: 4 Days

## Implementation

### Day 1-2: Self-Registering Factory Pattern

```cpp
// EngineRegistry.h - Replace 57-case switch
class EngineRegistry {
private:
    using FactoryFunc = std::function<std::unique_ptr<EngineBase>()>;
    using MetadataFunc = std::function<EngineMetadata()>;

    struct Registration {
        FactoryFunc factory;
        MetadataFunc metadata;
        int category;
    };

    std::map<int, Registration> engines;

    EngineRegistry() = default;

public:
    static EngineRegistry& instance() {
        static EngineRegistry reg;
        return reg;
    }

    template<typename T>
    class Registrar {
    public:
        Registrar(int id, int category) {
            EngineRegistry::instance().engines[id] = {
                []() { return std::make_unique<T>(); },
                []() { return T::getMetadata(); },
                category
            };
        }
    };

    std::unique_ptr<EngineBase> create(int id) {
        auto it = engines.find(id);
        return (it != engines.end()) ? it->second.factory() : nullptr;
    }

    std::vector<int> getEnginesInCategory(int category) {
        std::vector<int> result;
        for (const auto& [id, reg] : engines) {
            if (reg.category == category) {
                result.push_back(id);
            }
        }
        return result;
    }
};

// Self-registration in each engine file
namespace {
    EngineRegistry::Registrar<VintageCompressor> reg(1, CATEGORY_DYNAMICS);
}
```

### Day 3: Parameter System Refactoring

```cpp
// ParameterSystem.h - Eliminate duplication
class ParameterSystem {
private:
    struct SlotParameters {
        std::atomic<float> values[15];
        std::atomic<float> mix{0.5f};
        std::atomic<bool> bypass{false};
        std::atomic<bool> solo{false};
    };

    std::array<SlotParameters, 4> slots;

public:
    // Single place to extract parameters
    std::map<int, float> getSlotParameters(int slot) {
        if (slot < 0 || slot >= 4) return {};

        std::map<int, float> params;
        for (int i = 0; i < 15; ++i) {
            params[i] = slots[slot].values[i].load();
        }
        return params;
    }

    // Type-safe parameter access
    template<typename T>
    T getParameter(int slot, int param, T defaultValue) {
        if (slot < 0 || slot >= 4 || param < 0 || param >= 15) {
            return defaultValue;
        }
        return static_cast<T>(slots[slot].values[param].load());
    }

    // Batch update from UI
    void updateFromValueTree(const AudioProcessorValueTreeState& state) {
        for (int slot = 0; slot < 4; ++slot) {
            String prefix = "slot" + String(slot + 1);

            // Extract once, use everywhere
            for (int i = 0; i < 15; ++i) {
                auto* param = state.getRawParameterValue(
                    prefix + "_param" + String(i + 1));
                if (param) {
                    slots[slot].values[i].store(param->load());
                }
            }

            // Special parameters
            if (auto* mix = state.getRawParameterValue(prefix + "_mix")) {
                slots[slot].mix.store(mix->load());
            }
            if (auto* bypass = state.getRawParameterValue(prefix + "_bypass")) {
                slots[slot].bypass.store(bypass->load() > 0.5f);
            }
        }
    }
};
```

### Day 4: Platform Abstraction

```cpp
// Platform.h - Clean platform abstraction
namespace Platform {

#ifdef _WIN32
    using ThreadHandle = HANDLE;
    using LibraryHandle = HMODULE;
    constexpr const char* DLL_EXTENSION = ".dll";
#elif __APPLE__
    using ThreadHandle = pthread_t;
    using LibraryHandle = void*;
    constexpr const char* DLL_EXTENSION = ".dylib";
#else
    using ThreadHandle = pthread_t;
    using LibraryHandle = void*;
    constexpr const char* DLL_EXTENSION = ".so";
#endif

class System {
public:
    static int getCoreCount() {
        return std::thread::hardware_concurrency();
    }

    static size_t getMemoryUsage() {
        #ifdef _WIN32
            PROCESS_MEMORY_COUNTERS_EX pmc;
            GetProcessMemoryInfo(GetCurrentProcess(),
                                (PROCESS_MEMORY_COUNTERS*)&pmc,
                                sizeof(pmc));
            return pmc.WorkingSetSize;
        #elif __APPLE__
            struct task_basic_info info;
            mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
            task_info(mach_task_self(), TASK_BASIC_INFO,
                     (task_info_t)&info, &size);
            return info.resident_size;
        #else
            // Linux
            long rss = 0L;
            FILE* fp = fopen("/proc/self/statm", "r");
            if (fp) {
                fscanf(fp, "%*s%ld", &rss);
                fclose(fp);
            }
            return rss * sysconf(_SC_PAGESIZE);
        #endif
    }

    static void setThreadPriority(ThreadPriority priority) {
        #ifdef _WIN32
            SetThreadPriority(GetCurrentThread(),
                priority == ThreadPriority::Realtime ?
                THREAD_PRIORITY_TIME_CRITICAL :
                THREAD_PRIORITY_NORMAL);
        #else
            struct sched_param param;
            param.sched_priority = (priority == ThreadPriority::Realtime) ?
                sched_get_priority_max(SCHED_FIFO) : 0;
            pthread_setschedparam(pthread_self(),
                priority == ThreadPriority::Realtime ? SCHED_FIFO : SCHED_OTHER,
                &param);
        #endif
    }
};

// Clean SIMD abstraction
class SIMD {
public:
    static bool hasSSE2() {
        #if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
            return true;
        #else
            return false;
        #endif
    }

    static void processSIMD(float* data, int numSamples,
                           std::function<void(float*, int)> fallback) {
        #if defined(__SSE2__)
            // SSE2 implementation
            processSSE2(data, numSamples);
        #elif defined(__ARM_NEON)
            // NEON implementation
            processNEON(data, numSamples);
        #else
            // Scalar fallback
            fallback(data, numSamples);
        #endif
    }
};

} // namespace Platform
```

## Refactoring Checklist

- [ ] Replace EngineFactory switch with Registry
- [ ] Convert all engines to self-register
- [ ] Create centralized ParameterSystem
- [ ] Remove all parameter extraction duplication
- [ ] Create Platform abstraction layer
- [ ] Remove all platform #ifdefs from engines
- [ ] Update build system
- [ ] Test on all platforms

## Success Metrics
- 0 switch statements > 10 cases
- 0 duplicated code blocks
- All platform code in Platform namespace
- 50% reduction in EngineFactory size