# ChimeraPhoenix CI/CD Pipeline Documentation

## Overview

The ChimeraPhoenix CI/CD pipeline provides comprehensive automated testing, performance monitoring, and quality assurance for all 56 audio processing engines.

## Pipeline Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    TRIGGER (Push/PR/Schedule)                   │
└────────────────────────┬────────────────────────────────────────┘
                         │
         ┌───────────────┴───────────────┐
         │                               │
    ┌────▼─────┐                   ┌────▼──────┐
    │  BUILD   │                   │  CACHE    │
    │ All Test │                   │ JUCE +    │
    │  Suites  │                   │ Objects   │
    └────┬─────┘                   └───────────┘
         │
    ┌────┴─────────────────────────────┐
    │                                  │
┌───▼──────────┐            ┌──────────▼──────┐
│  FUNCTIONAL  │            │   PERFORMANCE   │
│    TESTS     │            │   BENCHMARKS    │
│              │            │                 │
│ • Reverb    │            │ • CPU Usage     │
│ • Filter    │            │ • Memory        │
│ • Distortion│            │ • Latency       │
│ • Dynamics  │            │ • Throughput    │
│ • Modulation│            └──────────┬──────┘
│ • Pitch     │                       │
│ • Spatial   │            ┌──────────▼──────┐
└───┬─────────┘            │  THD ANALYSIS   │
    │                      │                 │
    │                      │ • Harmonic      │
    │                      │   Distortion    │
    │                      │ • Signal/Noise  │
    │                      │ • Quality       │
    │                      └──────────┬──────┘
    │                                 │
    └────────┬────────────────────────┘
             │
      ┌──────▼──────────┐
      │  REGRESSION     │
      │  DETECTION      │
      │                 │
      │ Compare with    │
      │ Previous Build  │
      └──────┬──────────┘
             │
      ┌──────▼──────────┐
      │  REPORTING &    │
      │    ALERTING     │
      │                 │
      │ • PR Comments   │
      │ • GitHub Issues │
      │ • Slack Notify  │
      └─────────────────┘
```

## Pipeline Jobs

### 1. Build Job
**Purpose:** Compile all test executables and dependencies
**Duration:** ~5-10 minutes
**Triggers:** All events

**Steps:**
- Setup macOS environment
- Install dependencies (cmake, harfbuzz)
- Cache JUCE framework
- Compile standalone test suite
- Build CPU benchmark suite
- Build THD measurement suite
- Verify all binaries created successfully

**Artifacts:**
- `chimera-test-binaries/` - All compiled executables
- `build-logs/` - Compilation logs

### 2. Functional Tests Job
**Purpose:** Run category-specific test suites
**Duration:** ~15-30 minutes per suite
**Triggers:** Push, PR, Schedule

**Test Matrix:**
| Suite | Engines Tested | Duration |
|-------|----------------|----------|
| Reverb | 39-43 | 5 min |
| Filter | 7-14 | 3 min |
| Distortion | 15-22 | 3 min |
| Dynamics | 1-6 | 3 min |
| Modulation | 23-33 | 4 min |
| Pitch | 31-38 | 5 min |
| Spatial | 44-48 | 4 min |

**Pass Criteria:**
- All engines initialize successfully
- Audio processing produces valid output
- No crashes or hangs
- Parameters update correctly

**Artifacts:**
- `test-results-{suite}/` - Test output logs, CSV data

### 3. THD Measurements Job
**Purpose:** Measure Total Harmonic Distortion for clean effects
**Duration:** ~10-20 minutes
**Triggers:** Push, PR, Schedule

**Test Coverage:**
- Engines 0-14: Dynamics, Filters/EQ
- Engines 24-31: Modulation effects
- Engines 34-38: Delays
- Engines 42-43: Shimmer & Gated Reverb
- Engines 46-48: Spectral effects
- Engines 50-52: Special effects

**Methodology:**
1. Generate 1kHz sine wave at -6dBFS
2. Process through each engine with neutral parameters
3. FFT analysis to extract harmonics
4. Calculate THD from 2nd-5th harmonics
5. Flag engines with THD > 1.0%

**Threshold:** 1.0% THD
**Pass Criteria:**
- Less than 5 engines exceed threshold
- No engine has THD > 5%

**Artifacts:**
- `thd-measurements/comprehensive_thd_results.csv` - Full THD data
- `thd-measurements/thd_summary.md` - Summary report

### 4. CPU Performance Benchmarks Job
**Purpose:** Measure CPU usage for all 56 engines
**Duration:** ~30-45 minutes
**Triggers:** Push, Schedule, Manual (benchmark_engines input)

**Methodology:**
1. Process 10 seconds of audio per engine
2. Measure wall-clock processing time
3. Calculate CPU percentage: (time_taken / real_time) * 100
4. Compare with previous results
5. Flag regressions > 10% increase

**Threshold:** 50% CPU usage
**Pass Criteria:**
- Average CPU usage < 30%
- No engine uses > 100% CPU
- No regressions > 10%

**Artifacts:**
- `cpu-benchmarks/cpu_benchmark_results.csv` - Full results
- `cpu-benchmarks/perf_summary.md` - Top 10 + analysis
- `cpu-benchmarks/regressions.txt` - Performance regressions (if any)

### 5. Endurance & Stress Tests Job
**Purpose:** Long-running stability tests
**Duration:** ~60-90 minutes
**Triggers:** Nightly schedule, Manual

**Tests:**
- **Endurance Test:** Run all engines continuously for extended periods
- **Stress Test:** Test with extreme parameter values
- **Memory Test:** Monitor for leaks and excessive allocation

**Artifacts:**
- `endurance-test-results/` - Extended test logs

### 6. Report & Alert Job
**Purpose:** Aggregate results and send notifications
**Duration:** ~2-5 minutes
**Triggers:** After all other jobs (always runs)

**Actions:**
- Generate comprehensive CI/CD report
- Comment on PRs with test results
- Create GitHub issues for regressions
- Send Slack notifications for failures (if configured)

**Artifacts:**
- `ci-cd-final-report/CI_CD_REPORT.md` - Master report

## Triggers

### Push Events
```yaml
on:
  push:
    branches: [ main, develop, feature/* ]
```
**Runs:** Build, Functional Tests, THD, Benchmarks, Reporting

### Pull Requests
```yaml
on:
  pull_request:
    branches: [ main, develop ]
```
**Runs:** All tests + PR comment with results

### Scheduled (Nightly)
```yaml
on:
  schedule:
    - cron: '0 2 * * *'  # 2 AM UTC daily
```
**Runs:** Full suite including endurance tests

### Manual Dispatch
```yaml
on:
  workflow_dispatch:
    inputs:
      run_full_suite: boolean
      benchmark_engines: boolean
```
**Runs:** Custom test configuration

## Configuration

### Environment Variables
```yaml
JUCE_DIR: /usr/local/JUCE          # JUCE framework path
BUILD_TYPE: Release                 # Build configuration
THD_THRESHOLD: 1.0                  # THD pass threshold (%)
CPU_THRESHOLD_PERCENT: 50.0         # CPU usage threshold (%)
```

### Secrets Required
```yaml
SLACK_WEBHOOK_URL                   # For Slack notifications (optional)
```

## Regression Detection

The pipeline automatically detects performance regressions by:

1. **Downloading previous benchmark results**
2. **Comparing current vs. previous CPU usage**
3. **Flagging increases > 10%**
4. **Creating GitHub issue with details**

Example regression alert:
```
⚠️ PERFORMANCE REGRESSIONS DETECTED:
  Engine 41 (Convolution Reverb): +15.3% (was 45.2%, now 52.1%)
  Engine 50 (Granular Cloud): +12.7% (was 38.5%, now 43.4%)
```

## Artifact Retention

| Artifact Type | Retention |
|---------------|-----------|
| Build logs | 14 days |
| Test results | 30 days |
| THD measurements | 90 days |
| CPU benchmarks | 90 days |
| Final reports | 90 days |

## Notifications

### PR Comments
Automatically added to pull requests:
```markdown
## ChimeraPhoenix CI/CD Report

**Build Status:** ✓ Success

**Test Results:**
- ✓ Reverb tests passed
- ✓ Filter tests passed
...

**THD Results:**
- Engines exceeding 1.0% THD: 2
- Worst case: 1.8% (Engine 27)

**Performance:**
- Average CPU usage: 24.3%
- Engines exceeding 50% CPU: 3
```

### GitHub Issues
Automatically created for:
- Performance regressions
- Repeated test failures
- THD threshold violations

### Slack Notifications
Sent on main branch failures (if webhook configured):
```json
{
  "text": "🚨 ChimeraPhoenix CI/CD Pipeline Failed",
  "branch": "main",
  "commit": "abc123",
  "link": "https://github.com/.../runs/123"
}
```

## Local Testing

### Run individual test suites locally:
```bash
cd standalone_test

# Build all tests
./build_all.sh

# Run specific category
./build/reverb_test

# Run THD analysis
./build/test_comprehensive_thd

# Run CPU benchmarks
./build/cpu_benchmark_all_engines
```

### View results:
```bash
# Test outputs
cat *_results.txt

# THD report
cat comprehensive_thd_results.csv

# CPU benchmarks
cat cpu_benchmark_results.csv
```

## Performance Baselines

Current baselines (as of last successful run):

| Metric | Target | Current |
|--------|--------|---------|
| Average CPU Usage | < 30% | 24.3% |
| Engines > 50% CPU | 0 | 3 |
| THD Failures (>1%) | < 5 | 2 |
| Build Time | < 10 min | 8.5 min |

## Troubleshooting

### Build Failures
1. Check `build-logs/build.log`
2. Verify JUCE installation
3. Check for missing dependencies

### Test Failures
1. Review test output in artifacts
2. Check for engine-specific issues
3. Verify audio generation correctness

### THD Issues
1. Review `comprehensive_thd_results.csv`
2. Check parameter settings
3. Investigate specific engine implementations

### Performance Regressions
1. Review `cpu_benchmarks/regressions.txt`
2. Compare with previous benchmarks
3. Profile affected engines locally

## Maintenance

### Adding New Tests
1. Create test executable in `standalone_test/`
2. Add to `build_all.sh`
3. Add to workflow test matrix

### Modifying Thresholds
Edit workflow environment variables:
```yaml
env:
  THD_THRESHOLD: 1.0              # Adjust as needed
  CPU_THRESHOLD_PERCENT: 50.0     # Adjust as needed
```

### Changing Schedule
Edit cron expression:
```yaml
schedule:
  - cron: '0 2 * * *'  # Modify time/frequency
```

## Future Enhancements

- [ ] Linux build support
- [ ] Windows build support
- [ ] GPU performance testing
- [ ] Real-time latency measurements
- [ ] Plugin validation (VST3/AU)
- [ ] Code coverage reporting
- [ ] Memory profiling
- [ ] Threading safety tests

## Contact

For questions or issues with the CI/CD pipeline:
- Create an issue with label `ci-cd`
- Contact the development team

---

*Last Updated: 2025-10-11*
*Pipeline Version: 1.0*
