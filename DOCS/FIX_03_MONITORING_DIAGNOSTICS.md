# FIX 03: Monitoring & Diagnostics

## Overview
Implements logging framework (TD-010) and CPU monitoring (TD-007) for production visibility.

### Timeline: 3 Days

## Implementation

### Logging Framework

```cpp
// ChimeraLogger.h
class ChimeraLogger {
private:
    struct LogEntry {
        TimePoint timestamp;
        Level level;
        std::string message;
        std::thread::id threadId;
    };

    RingBuffer<LogEntry, 10000> buffer;  // Lock-free
    std::ofstream criticalLog{"chimera_critical.log"};

public:
    enum Level { DEBUG, INFO, WARN, ERROR, CRITICAL };

    static void log(Level level, const std::string& msg) {
        auto& logger = instance();
        LogEntry entry{
            std::chrono::steady_clock::now(),
            level,
            msg,
            std::this_thread::get_id()
        };

        logger.buffer.push(entry);

        if (level >= ERROR) {
            std::lock_guard<std::mutex> lock(logger.fileMutex);
            logger.criticalLog << entry << std::endl;
        }
    }

    static std::vector<LogEntry> getRecent(int count = 100) {
        return instance().buffer.getLast(count);
    }
};

#define LOG_DEBUG(msg) ChimeraLogger::log(ChimeraLogger::DEBUG, msg)
#define LOG_ERROR(msg) ChimeraLogger::log(ChimeraLogger::ERROR, msg)
```

### CPU Monitoring

```cpp
// CPUMonitor.h
class CPUMonitor {
private:
    struct Measurement {
        TimePoint start;
        std::atomic<double> cpuPercent{0.0};
    };

    std::array<Measurement, 57> engineMeasurements;
    std::atomic<double> totalCPU{0.0};

public:
    class ScopedTimer {
        CPUMonitor* monitor;
        int engineId;
        TimePoint start;

    public:
        ScopedTimer(CPUMonitor* m, int id)
            : monitor(m), engineId(id), start(Clock::now()) {}

        ~ScopedTimer() {
            auto duration = Clock::now() - start;
            monitor->recordMeasurement(engineId, duration);
        }
    };

    ScopedTimer measureEngine(int id) {
        return ScopedTimer(this, id);
    }

    double getEngineCPU(int id) const {
        return engineMeasurements[id].cpuPercent.load();
    }

    double getTotalCPU() const {
        return totalCPU.load();
    }
};
```

### Integration

```cpp
void ChimeraAudioProcessor::processBlock(AudioBuffer& buffer, MidiBuffer&) {
    auto timer = cpuMonitor.measureEngine(currentEngineId);

    LOG_DEBUG("Processing block: " + std::to_string(buffer.getNumSamples()));

    try {
        // Process...
    } catch (const std::exception& e) {
        LOG_ERROR("Process failed: " + std::string(e.what()));
    }
}
```

## Diagnostics Dashboard

```cpp
class DiagnosticsReport {
public:
    struct Report {
        // Performance
        double avgCPU;
        double peakCPU;
        int dropouts;

        // Errors
        int errorCount;
        std::vector<std::string> recentErrors;

        // Memory
        size_t memoryUsed;
        int activeEngines;

        // System
        std::string osVersion;
        std::string cpuInfo;
        size_t ramTotal;
    };

    static Report generate() {
        Report r;
        r.avgCPU = CPUMonitor::instance().getTotalCPU();
        r.recentErrors = ChimeraLogger::getRecent(10);
        // ... populate
        return r;
    }

    static void dumpToFile(const std::string& path) {
        auto report = generate();
        // Write JSON report
    }
};
```

## Success Metrics
- All critical errors logged
- CPU usage tracked per engine
- <1ms logging overhead
- Diagnostic reports generated