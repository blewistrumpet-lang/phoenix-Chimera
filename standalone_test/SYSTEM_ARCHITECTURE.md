# Comprehensive Test System - Architecture

## System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                   test_all_comprehensive.sh                         │
│                    Main Test Runner Script                          │
└─────────────────────────────────────────────────────────────────────┘
                                 │
                ┌────────────────┼────────────────┐
                │                │                │
                ▼                ▼                ▼
        ┌───────────┐    ┌───────────┐   ┌───────────┐
        │   Build   │    │   Test    │   │  Report   │
        │   Phase   │    │   Phase   │   │   Phase   │
        └───────────┘    └───────────┘   └───────────┘
```

## Detailed Flow Diagram

```
START
  │
  ├─► Initialize Database (SQLite)
  │     │
  │     └─► Create tables: test_runs, test_results, engine_results, performance_metrics
  │
  ├─► Build Phase (unless --skip-build)
  │     │
  │     ├─► Run build_all.sh
  │     ├─► Run build_comprehensive_thd.sh
  │     ├─► Run build_frequency_response_test.sh
  │     ├─► Run build_pitch_test.sh
  │     ├─► Run build_latency_suite.sh
  │     ├─► Run build_cpu_benchmark.sh
  │     ├─► Run build_endurance_test.sh
  │     ├─► Run build_silence_test.sh
  │     ├─► Run build_sample_rate_test.sh
  │     └─► Run build_dc_offset_test.sh
  │           │
  │           └─► Capture build logs
  │
  ├─► Test Execution Phase
  │     │
  │     ├─► For each test in test suite:
  │     │     │
  │     │     ├─► Check if executable exists
  │     │     │     ├─► Yes: Run test with timeout
  │     │     │     └─► No: Mark as SKIPPED
  │     │     │
  │     │     ├─► Capture stdout/stderr to log file
  │     │     │
  │     │     ├─► Parse test output:
  │     │     │     ├─► Status (PASS/FAIL/TIMEOUT)
  │     │     │     ├─► Peak level
  │     │     │     ├─► RMS level
  │     │     │     ├─► THD percentage
  │     │     │     ├─► CPU percentage
  │     │     │     └─► Duration
  │     │     │
  │     │     ├─► Create JSON result file
  │     │     │
  │     │     └─► Display status (✓/✗/⏱/⊘)
  │     │
  │     └─► Generate test summary JSON
  │
  ├─► Database Storage Phase
  │     │
  │     ├─► Get git commit & branch
  │     │
  │     ├─► Insert test run record
  │     │     └─► Returns run_id
  │     │
  │     └─► For each test result JSON:
  │           └─► Insert test_results record with run_id
  │
  ├─► Visualization Phase
  │     │
  │     └─► Python script generates graphs:
  │           ├─► Query database for historical data
  │           ├─► Generate pass_fail_trend.png
  │           ├─► Generate pass_rate_trend.png
  │           ├─► Generate duration_trend.png
  │           └─► Generate category_breakdown.png
  │
  ├─► Report Generation Phase
  │     │
  │     └─► Generate HTML report:
  │           ├─► Read test summary JSON
  │           ├─► Read individual test JSONs
  │           ├─► Query database for historical stats
  │           ├─► Embed trend graph images
  │           └─► Write test_report.html
  │
  └─► Finalization
        │
        ├─► Print summary to console
        ├─► Open HTML report in browser
        └─► Exit with appropriate code (0=success, 1=failure)
```

## Component Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         User Interface Layer                        │
├─────────────────────────────────────────────────────────────────────┤
│  • Command Line Interface (bash with color output)                  │
│  • HTML Report (browser-based visualization)                        │
│  • Query Tool (interactive database queries)                        │
└─────────────────────────────────────────────────────────────────────┘
                                 │
┌─────────────────────────────────────────────────────────────────────┐
│                        Execution Layer                              │
├─────────────────────────────────────────────────────────────────────┤
│  • Test Runner (bash orchestration)                                 │
│  • Build System Integration (existing build scripts)                │
│  • Test Executables (C++ test programs)                             │
│  • Timeout Management (timeout command)                             │
└─────────────────────────────────────────────────────────────────────┘
                                 │
┌─────────────────────────────────────────────────────────────────────┐
│                         Data Processing Layer                       │
├─────────────────────────────────────────────────────────────────────┤
│  • Log Parsing (regex-based extraction)                             │
│  • Result Aggregation (JSON generation)                             │
│  • Metric Calculation (averages, rates, trends)                     │
│  • Status Determination (pass/fail logic)                           │
└─────────────────────────────────────────────────────────────────────┘
                                 │
┌─────────────────────────────────────────────────────────────────────┐
│                         Storage Layer                               │
├─────────────────────────────────────────────────────────────────────┤
│  • SQLite Database (structured data)                                │
│  • JSON Files (intermediate results)                                │
│  • Log Files (raw test output)                                      │
│  • Graph Images (PNG visualizations)                                │
└─────────────────────────────────────────────────────────────────────┘
                                 │
┌─────────────────────────────────────────────────────────────────────┐
│                      Visualization Layer                            │
├─────────────────────────────────────────────────────────────────────┤
│  • Python/Matplotlib (graph generation)                             │
│  • HTML/CSS (report styling)                                        │
│  • Chart.js (optional interactive charts)                           │
└─────────────────────────────────────────────────────────────────────┘
```

## Data Flow

```
┌──────────────┐
│ Test Program │
└──────┬───────┘
       │ stdout/stderr
       ▼
┌──────────────┐
│   Log File   │
└──────┬───────┘
       │ parsing
       ▼
┌──────────────┐      ┌──────────────┐
│  JSON Result │ ───► │   Database   │
└──────────────┘      └──────┬───────┘
                             │ query
       ┌─────────────────────┤
       │                     │
       ▼                     ▼
┌──────────────┐      ┌──────────────┐
│ Trend Graphs │      │ HTML Report  │
└──────────────┘      └──────────────┘
       │                     │
       └─────────┬───────────┘
                 ▼
         ┌──────────────┐
         │     User     │
         └──────────────┘
```

## Database Schema

```
┌─────────────────────────────────────────────────────────────┐
│                        test_runs                            │
├─────────────────────────────────────────────────────────────┤
│  PK  run_id             INTEGER                             │
│      timestamp          TEXT                                │
│      total_tests        INTEGER                             │
│      passed             INTEGER                             │
│      failed             INTEGER                             │
│      timeout            INTEGER                             │
│      skipped            INTEGER                             │
│      duration_seconds   REAL                                │
│      git_commit         TEXT                                │
│      git_branch         TEXT                                │
└─────────────────────────────────────────────────────────────┘
                              │ 1:N
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      test_results                           │
├─────────────────────────────────────────────────────────────┤
│  PK  result_id          INTEGER                             │
│  FK  run_id             INTEGER                             │
│      test_name          TEXT                                │
│      test_category      TEXT                                │
│      status             TEXT                                │
│      duration_seconds   REAL                                │
│      error_message      TEXT                                │
│      peak_level         REAL                                │
│      rms_level          REAL                                │
│      thd_percent        REAL                                │
│      cpu_percent        REAL                                │
│      memory_mb          REAL                                │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                     engine_results                          │
├─────────────────────────────────────────────────────────────┤
│  PK  engine_result_id   INTEGER                             │
│  FK  run_id             INTEGER                             │
│      engine_id          INTEGER                             │
│      engine_name        TEXT                                │
│      status             TEXT                                │
│      duration_seconds   REAL                                │
│      peak_level         REAL                                │
│      rms_level          REAL                                │
│      thd_percent        REAL                                │
│      cpu_percent        REAL                                │
│      latency_samples    INTEGER                             │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                  performance_metrics                        │
├─────────────────────────────────────────────────────────────┤
│  PK  metric_id          INTEGER                             │
│  FK  run_id             INTEGER                             │
│      metric_name        TEXT                                │
│      metric_value       REAL                                │
│      metric_unit        TEXT                                │
└─────────────────────────────────────────────────────────────┘
```

## File System Layout

```
standalone_test/
│
├── Scripts (Executable)
│   ├── test_all_comprehensive.sh       # Main runner
│   └── query_test_history.sh            # Query tool
│
├── Documentation
│   ├── COMPREHENSIVE_TEST_SYSTEM_README.md
│   ├── QUICK_START_TESTING.md
│   ├── TEST_SYSTEM_SUMMARY.md
│   └── SYSTEM_ARCHITECTURE.md          # This file
│
├── Database
│   └── test_results.db                 # SQLite database
│
├── Build Output (existing)
│   └── build/
│       ├── filter_test
│       ├── distortion_test
│       └── ... (test executables)
│
└── Report Output (generated)
    └── test_reports/
        └── report_YYYYMMDD_HHMMSS/
            ├── test_report.html         # Main report
            ├── test_runner.log          # Runner log
            │
            ├── logs/                    # Test logs
            │   ├── build_all.log
            │   ├── filter_quality.log
            │   └── ...
            │
            ├── data/                    # JSON data
            │   ├── test_summary.json
            │   ├── build_summary.json
            │   └── ...
            │
            └── graphs/                  # Visualizations
                ├── pass_fail_trend.png
                ├── pass_rate_trend.png
                ├── duration_trend.png
                └── category_breakdown.png
```

## Process Communication

```
┌──────────────────────────────────────────────────────────────┐
│                   Main Process (Bash)                        │
└──────────────────────────────────────────────────────────────┘
         │
         ├─► Fork ─► Build Script Process (Bash)
         │            │
         │            └─► Compiler (clang++)
         │                 │
         │                 └─► Object Files → Executables
         │
         ├─► Fork ─► Test Process (C++ executable)
         │            │
         │            └─► stdout/stderr → Captured by parent
         │
         ├─► Fork ─► Python Graph Generator
         │            │
         │            ├─► Read from SQLite
         │            └─► Write PNG files
         │
         └─► Direct Call ─► SQLite CLI
                            │
                            └─► Execute SQL commands
```

## Test Execution State Machine

```
    START
      │
      ▼
┌────────────┐
│  PENDING   │  (Test queued)
└─────┬──────┘
      │
      ▼
┌────────────┐
│  BUILDING  │  (Compiling)
└─────┬──────┘
      │
      ├─► Build Failed ──► SKIPPED
      │
      ▼
┌────────────┐
│  RUNNING   │  (Executing)
└─────┬──────┘
      │
      ├─► Timeout ──────► TIMEOUT
      ├─► Error ────────► FAIL
      └─► Success ──────► PASS
            │
            ▼
      ┌────────────┐
      │  COMPLETE  │
      └────────────┘
```

## Query Tool Architecture

```
┌─────────────────────────────────────────────────────────────┐
│               query_test_history.sh                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Command Router                                              │
│    │                                                         │
│    ├─► summary      ──► SQL: SELECT * FROM test_runs        │
│    ├─► latest       ──► SQL: SELECT * WHERE run_id=MAX(...)  │
│    ├─► trends       ──► SQL: Window functions for trends    │
│    ├─► failures     ──► SQL: GROUP BY test_name, status     │
│    ├─► performance  ──► SQL: AVG metrics by test            │
│    ├─► compare      ──► SQL: JOIN two runs                  │
│    └─► export       ──► SQL: Full export to CSV             │
│                                                              │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌─────────────────┐
                    │  SQLite Engine  │
                    └─────────────────┘
                              │
                              ▼
                    ┌─────────────────┐
                    │ Formatted Output│
                    │  (colored text) │
                    └─────────────────┘
```

## Concurrent Operations

```
Sequential Operations (Must Complete in Order):
  Build Phase → Test Phase → Report Phase

Parallel Opportunities (Future Enhancement):
  ┌─────────────┐
  │  Test 1     │
  ├─────────────┤
  │  Test 2     │  ← Can run simultaneously
  ├─────────────┤
  │  Test 3     │
  └─────────────┘
       │
       └─► Results Collection (synchronized)
            │
            └─► Database Write (sequential)
```

## Error Handling Flow

```
Error Detected
      │
      ├─► Build Error
      │     └─► Log to build_*.log
      │         └─► Continue with next build
      │             └─► Mark tests as SKIPPED
      │
      ├─► Test Timeout
      │     └─► Kill test process
      │         └─► Mark as TIMEOUT
      │             └─► Continue with next test
      │
      ├─► Test Crash
      │     └─► Capture exit code
      │         └─► Mark as FAIL
      │             └─► Log error message
      │
      ├─► Database Error
      │     └─► Retry operation
      │         └─► If fails: Log and continue
      │
      └─► Graph Generation Error
            └─► Log warning
                └─► Continue (report still generates)
```

## Security Considerations

```
┌──────────────────────────────────────────────────────────────┐
│                     Security Measures                        │
├──────────────────────────────────────────────────────────────┤
│  1. Input Validation                                         │
│     • Validate command line arguments                        │
│     • Sanitize file paths                                    │
│                                                              │
│  2. File System Security                                     │
│     • Create files with safe permissions                     │
│     • Check executable permissions                           │
│     • Avoid arbitrary code execution                         │
│                                                              │
│  3. Database Security                                        │
│     • Use parameterized queries (via Python)                 │
│     • Local file access only                                 │
│     • No network exposure                                    │
│                                                              │
│  4. Process Isolation                                        │
│     • Timeout limits prevent runaway processes               │
│     • Each test runs in separate process                     │
│     • Clean up on exit                                       │
└──────────────────────────────────────────────────────────────┘
```

## Performance Optimization

```
Optimization Strategy:
    │
    ├─► Caching
    │     └─► Compiled object files cached
    │         └─► Only rebuild changed sources
    │
    ├─► Parallel Execution (Future)
    │     └─► Run independent tests in parallel
    │         └─► Use process pool
    │
    ├─► Database Indexing
    │     └─► Indexes on frequently queried columns
    │         └─► Fast historical queries
    │
    ├─► Lazy Graph Generation
    │     └─► Only generate if historical data exists
    │         └─► Skip if < 2 runs
    │
    └─► Efficient Parsing
          └─► Stream-based log parsing
              └─► Don't load entire log into memory
```

## Extensibility Points

```
┌──────────────────────────────────────────────────────────────┐
│                    Extension Points                          │
├──────────────────────────────────────────────────────────────┤
│  1. Test Categories                                          │
│     • Add new categories in test array                       │
│     • Automatic categorization in reports                    │
│                                                              │
│  2. Metrics                                                  │
│     • Add parsing for new metrics                            │
│     • Store in performance_metrics table                     │
│                                                              │
│  3. Graph Types                                              │
│     • Add new Python visualization functions                 │
│     • Embed in HTML report                                   │
│                                                              │
│  4. Output Formats                                           │
│     • Add JSON report generator                              │
│     • Add XML report generator                               │
│     • Add PDF report generator                               │
│                                                              │
│  5. Notification Systems                                     │
│     • Email integration                                      │
│     • Slack/Discord webhooks                                 │
│     • SMS alerts                                             │
└──────────────────────────────────────────────────────────────┘
```

## Summary

This architecture provides:

- **Modularity**: Clear separation of concerns
- **Scalability**: Database supports unlimited history
- **Extensibility**: Easy to add new tests and metrics
- **Reliability**: Error handling at every stage
- **Usability**: Clear interfaces and documentation
- **Performance**: Optimized for fast execution
- **Maintainability**: Well-structured code with logging

The system is production-ready and designed for long-term use.
