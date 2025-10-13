# ChimeraPhoenix CI/CD Pipeline - Visual Guide

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         TRIGGER EVENTS                              │
├─────────────────────────────────────────────────────────────────────┤
│  • Push to main/develop/feature branches                            │
│  • Pull Request to main/develop                                     │
│  • Scheduled: Nightly at 2 AM UTC                                   │
│  • Manual: Via GitHub Actions UI                                    │
└─────────────────────┬───────────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       JOB 1: BUILD (10 min)                         │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  Setup Environment                                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │Install deps │→ │Cache JUCE  │→ │Cache builds│              │
│  │cmake,harfbuzz│  │framework   │  │artifacts   │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
│                                                                     │
│  Compile                                                            │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐│
│  │ Build Test Suite │→ │Build Benchmarks │→ │Build THD Tests  ││
│  │  8 executables  │  │CPU performance  │  │Audio quality    ││
│  └──────────────────┘  └──────────────────┘  └──────────────────┘│
│                                                                     │
│  Output: chimera-test-binaries/ (7 day retention)                  │
└─────────────────┬───────────────────┬──────────────┬──────────────┘
                  │                   │              │
        ┌─────────┴─────────┐    ┌────┴────┐   ┌────┴────┐
        ▼                   ▼    ▼         ▼   ▼         ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│  JOB 2:       │   │  JOB 3:       │   │  JOB 4:       │
│  FUNCTIONAL   │   │  THD          │   │  PERFORMANCE  │
│  TESTS        │   │  ANALYSIS     │   │  BENCHMARKS   │
│  (30 min)     │   │  (20 min)     │   │  (45 min)     │
└───────────────┘   └───────────────┘   └───────────────┘
        │                   │                   │
        └─────────┬─────────┴─────────┬─────────┘
                  ▼                   ▼
        ┌───────────────────┐  ┌───────────────────┐
        │  JOB 5 (optional):│  │  JOB 6:           │
        │  ENDURANCE        │  │  REPORT & ALERT   │
        │  (60+ min)        │  │  (5 min)          │
        │  Nightly only     │  │  Always runs      │
        └───────────────────┘  └───────────────────┘
```

## Job 2: Functional Tests (Parallel Execution)

```
Download Artifacts (from Job 1)
        │
        ├──────────────────┬──────────────┬──────────────┬──────────────┐
        ▼                  ▼              ▼              ▼              ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│  REVERB      │  │  FILTER      │  │  DISTORTION  │  │  DYNAMICS    │
│              │  │              │  │              │  │              │
│ Engines:     │  │ Engines:     │  │ Engines:     │  │ Engines:     │
│ 39-43        │  │ 7-14         │  │ 15-22        │  │ 1-6          │
│              │  │              │  │              │  │              │
│ Time: 5 min  │  │ Time: 3 min  │  │ Time: 3 min  │  │ Time: 3 min  │
│              │  │              │  │              │  │              │
│ Tests:       │  │ Tests:       │  │ Tests:       │  │ Tests:       │
│ • Init       │  │ • Freq resp  │  │ • Harmonics  │  │ • GR curve   │
│ • Process    │  │ • Phase      │  │ • Saturation │  │ • Threshold  │
│ • Params     │  │ • Resonance  │  │ • Clipping   │  │ • Ratio      │
│ • Stability  │  │ • Stereo     │  │ • Drive      │  │ • Attack/Rel │
└──────────────┘  └──────────────┘  └──────────────┘  └──────────────┘
        │                  │              │              │
        ▼                  ▼              ▼              ▼
        │         (continued below...)   │              │
        │                                 │              │
        ├──────────────┬──────────────────┴──────────────┘
        ▼              ▼                  ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│  MODULATION  │  │  PITCH       │  │  SPATIAL     │
│              │  │              │  │              │
│ Engines:     │  │ Engines:     │  │ Engines:     │
│ 23-33        │  │ 31-38        │  │ 44-48        │
│              │  │              │  │              │
│ Time: 4 min  │  │ Time: 5 min  │  │ Time: 4 min  │
│              │  │              │  │              │
│ Tests:       │  │ Tests:       │  │ Tests:       │
│ • LFO rate   │  │ • Pitch acc  │  │ • Width      │
│ • Depth      │  │ • Latency    │  │ • Correlation│
│ • Feedback   │  │ • Artifacts  │  │ • Phase      │
│ • Stereo     │  │ • Formants   │  │ • Imaging    │
└──────────────┘  └──────────────┘  └──────────────┘
        │                  │                  │
        └──────────────────┴──────────────────┘
                           │
                           ▼
                    Parse & Report
                    ┌─────────────────┐
                    │ Count P/F       │
                    │ Generate report │
                    │ Upload results  │
                    └─────────────────┘
```

## Job 3: THD Analysis Flow

```
Download Test Binaries
        │
        ▼
┌──────────────────────────────────────┐
│  Run test_comprehensive_thd          │
│                                      │
│  For each clean effect engine:       │
│                                      │
│  1. Generate test signal             │
│     ┌────────────────────────┐      │
│     │ 1kHz sine @ -6dBFS     │      │
│     │ Duration: 2 seconds    │      │
│     └────────────────────────┘      │
│                                      │
│  2. Process through engine           │
│     ┌────────────────────────┐      │
│     │ Neutral parameters     │      │
│     │ Block-based processing │      │
│     └────────────────────────┘      │
│                                      │
│  3. FFT Analysis                     │
│     ┌────────────────────────┐      │
│     │ 16384-point FFT        │      │
│     │ Blackman-Harris window │      │
│     └────────────────────────┘      │
│                                      │
│  4. Extract harmonics                │
│     ┌────────────────────────┐      │
│     │ Fundamental + 2nd-5th  │      │
│     │ Calculate THD %        │      │
│     └────────────────────────┘      │
│                                      │
│  5. Compare to threshold             │
│     ┌────────────────────────┐      │
│     │ Pass: THD < 1.0%       │      │
│     │ Fail: THD >= 1.0%      │      │
│     └────────────────────────┘      │
└──────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────┐
│  Generate Reports                    │
│                                      │
│  • comprehensive_thd_results.csv     │
│    Engine ID | Name | THD% | Status │
│                                      │
│  • comprehensive_thd_report.txt      │
│    Summary statistics               │
│    Failed engines list              │
│                                      │
│  • thd_summary.md                    │
│    Markdown report for PR           │
└──────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────┐
│  Threshold Check                     │
│                                      │
│  If failed_count > 5:                │
│    ❌ Exit with error                │
│  Else:                               │
│    ✓ Pass                            │
└──────────────────────────────────────┘
```

## Job 4: Performance Benchmarks Flow

```
Download Test Binaries
        │
        ▼
┌──────────────────────────────────────────────┐
│  Run cpu_benchmark_all_engines               │
│                                              │
│  For each of 56 engines:                     │
│                                              │
│  ┌─────────────────────────────────────┐   │
│  │ 1. Create engine instance           │   │
│  │ 2. Prepare: 48kHz, 512 block        │   │
│  │ 3. Generate 10 sec test audio       │   │
│  │ 4. Start timer                      │   │
│  │ 5. Process in blocks                │   │
│  │ 6. Stop timer                       │   │
│  │ 7. Calculate: (time/10s) * 100     │   │
│  └─────────────────────────────────────┘   │
│                                              │
│  Example:                                    │
│    Processing time: 4.5 seconds             │
│    Real-time: 10 seconds                    │
│    CPU %: (4.5/10) * 100 = 45%              │
└──────────────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────────────┐
│  Sort & Rank                                 │
│                                              │
│  1. Sort by CPU % (descending)               │
│  2. Calculate category averages              │
│  3. Identify engines > threshold             │
└──────────────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────────────┐
│  Generate Reports                            │
│                                              │
│  • cpu_benchmark_results.csv                 │
│    Rank | ID | Name | Time | CPU%           │
│                                              │
│  • perf_summary.md                           │
│    Top 10, category analysis                │
└──────────────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────────────┐
│  Regression Detection                        │
│                                              │
│  Download previous results → Compare         │
│                                              │
│  For each engine:                            │
│    If CPU increase > 10%:                    │
│      ⚠️ Flag as regression                   │
│                                              │
│  If regressions found:                       │
│    • Create regressions.txt                  │
│    • Set output flag                         │
│    • Will trigger issue creation             │
└──────────────────────────────────────────────┘
```

## Job 6: Report & Alert Flow

```
Wait for all previous jobs
        │
        ▼
┌──────────────────────────────────────────────┐
│  Download All Artifacts                      │
│                                              │
│  artifacts/                                  │
│  ├── build-logs/                             │
│  ├── test-results-reverb/                    │
│  ├── test-results-filter/                    │
│  ├── ... (all test suites)                   │
│  ├── thd-measurements/                       │
│  └── cpu-benchmarks/                         │
└──────────────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────────────┐
│  Generate Comprehensive Report               │
│                                              │
│  CI_CD_REPORT.md:                            │
│  ┌────────────────────────────────────┐     │
│  │ # ChimeraPhoenix CI/CD Report      │     │
│  │                                    │     │
│  │ ## Build Status                    │     │
│  │ ✓ Completed in 8.5 min             │     │
│  │                                    │     │
│  │ ## Test Results                    │     │
│  │ - ✓ Reverb: 5/5 passed             │     │
│  │ - ✓ Filter: 8/8 passed             │     │
│  │ - ... (all suites)                 │     │
│  │                                    │     │
│  │ ## THD Analysis                    │     │
│  │ • Failed engines: 2                │     │
│  │ • Worst: 1.8% (Engine 27)          │     │
│  │                                    │     │
│  │ ## Performance                     │     │
│  │ • Avg CPU: 24.3%                   │     │
│  │ • Heavy engines: 3                 │     │
│  └────────────────────────────────────┘     │
└──────────────────────────────────────────────┘
        │
        ├─────────────────┬─────────────────┬─────────────────┐
        ▼                 ▼                 ▼                 ▼
┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│  PR Comment │  │GitHub Issue │  │Slack Notify │  │Upload Report│
│             │  │             │  │             │  │             │
│If PR:       │  │If regressions│ │If failures: │  │Always:      │
│• Post report│  │detected:    │  │• Main branch│  │• Save report│
│• Show THD   │  │• Auto-create│  │• Send alert │  │• 90 days    │
│• Show perf  │  │• Add labels │  │• Include URL│  │             │
└─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘
```

## Data Flow: Test Results

```
┌─────────────────────────────────────────────────────────────┐
│                   Test Execution                            │
│                                                             │
│  ┌─────────────────────────────────────────────┐          │
│  │ Test Suite Binary                           │          │
│  │                                             │          │
│  │ • Initialize engines                        │          │
│  │ • Run test cases                            │          │
│  │ • Capture stdout/stderr                     │          │
│  │ • Generate CSV data                         │          │
│  └─────────────────┬───────────────────────────┘          │
│                    │                                        │
│                    ▼                                        │
│  ┌─────────────────────────────────────────────┐          │
│  │ Output Files                                │          │
│  │                                             │          │
│  │ • *_results.txt  (logs)                     │          │
│  │ • *.csv          (data)                     │          │
│  │ • *.json         (structured)               │          │
│  └─────────────────┬───────────────────────────┘          │
└────────────────────┼──────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                   Parsing & Analysis                        │
│                                                             │
│  ┌─────────────────────────────────────────────┐          │
│  │ Shell Scripts (awk, grep, sed)              │          │
│  │                                             │          │
│  │ • Count PASS/FAIL                           │          │
│  │ • Extract metrics                           │          │
│  │ • Compare thresholds                        │          │
│  │ • Calculate statistics                      │          │
│  └─────────────────┬───────────────────────────┘          │
│                    │                                        │
│                    ▼                                        │
│  ┌─────────────────────────────────────────────┐          │
│  │ Structured Data                             │          │
│  │                                             │          │
│  │ • passed_count                              │          │
│  │ • failed_count                              │          │
│  │ • threshold_violations                      │          │
│  │ • worst_case_values                         │          │
│  └─────────────────┬───────────────────────────┘          │
└────────────────────┼──────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                   Reporting                                 │
│                                                             │
│  ┌─────────────────────────────────────────────┐          │
│  │ Markdown Reports                            │          │
│  │                                             │          │
│  │ • Summary tables                            │          │
│  │ • Pass/Fail indicators                      │          │
│  │ • Detailed breakdowns                       │          │
│  │ • Trend analysis                            │          │
│  └─────────────────┬───────────────────────────┘          │
│                    │                                        │
│                    ▼                                        │
│  ┌─────────────────────────────────────────────┐          │
│  │ Artifacts                                   │          │
│  │                                             │          │
│  │ • Uploaded to GitHub                        │          │
│  │ • Available for download                    │          │
│  │ • Retention: 7-90 days                      │          │
│  └─────────────────┬───────────────────────────┘          │
└────────────────────┼──────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                   Notifications                             │
│                                                             │
│  • PR comments                                              │
│  • GitHub issues                                            │
│  • Slack messages                                           │
│  • Email (via GitHub)                                       │
└─────────────────────────────────────────────────────────────┘
```

## Caching Strategy

```
┌─────────────────────────────────────────────┐
│               First Build                   │
│                                             │
│  No cache available                         │
│                                             │
│  1. Clone JUCE                              │
│     ├─ 500 MB download                      │
│     └─ 3-5 minutes                          │
│                                             │
│  2. Compile JUCE modules                    │
│     ├─ 10 modules                           │
│     └─ 4-6 minutes                          │
│                                             │
│  3. Compile engines                         │
│     ├─ 56 engines                           │
│     └─ 2-3 minutes                          │
│                                             │
│  Total: ~10-15 minutes                      │
│                                             │
│  ✓ Cache saved:                             │
│    - JUCE framework                         │
│    - Compiled objects                       │
└─────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────┐
│            Subsequent Builds                │
│                                             │
│  Cache restored                             │
│                                             │
│  1. JUCE modules (cached)                   │
│     ├─ 500 MB restored                      │
│     └─ 30 seconds                           │
│                                             │
│  2. Unchanged engines (cached)              │
│     ├─ Reuse compiled .o files              │
│     └─ 10 seconds                           │
│                                             │
│  3. Changed engines (recompile)             │
│     ├─ Only modified files                  │
│     └─ 1-2 minutes                          │
│                                             │
│  Total: ~3-5 minutes (50-70% faster!)       │
└─────────────────────────────────────────────┘

Cache Keys:
┌──────────────────────────────────────────────────┐
│ juce-7.0.12-macos-{arch}                         │
│   ├─ Version locked                              │
│   └─ Platform specific                           │
│                                                  │
│ build-cache-{os}-{hash-of-sources}               │
│   ├─ Hash includes all .cpp/.h files             │
│   └─ Invalidated on source changes               │
└──────────────────────────────────────────────────┘
```

## Failure Handling

```
Any step can fail
        │
        ▼
┌──────────────────────────────────────┐
│  Set job output                      │
│  exit_status = "failed"              │
└──────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────┐
│  Upload artifacts (always)           │
│  • Logs                              │
│  • Partial results                   │
│  • Error messages                    │
└──────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────┐
│  Dependent jobs                      │
│                                      │
│  Some jobs: Continue anyway          │
│    allow_dependency_failure: true    │
│                                      │
│  Other jobs: Stop                    │
│    (default behavior)                │
└──────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────┐
│  Report job (always runs)            │
│                                      │
│  • Collect what succeeded            │
│  • Report what failed                │
│  • Send notifications                │
└──────────────────────────────────────┘
```

---

## Legend

```
│  Sequence/Flow
▼  Next step
→  Leads to
┌─┐ Box/Container
├─┤ Branch point
└─┘ End of section
✓  Success
✗  Failure
⚠  Warning
```

---

*For more details, see:*
- **Full Documentation:** `README.md`
- **Quick Start:** `QUICKSTART.md`
- **Pipeline Config:** `chimera-ci-cd.yml`
