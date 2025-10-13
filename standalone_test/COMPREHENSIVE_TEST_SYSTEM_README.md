# ChimeraPhoenix Comprehensive Test System

## Overview

The Comprehensive Test System provides automated testing, result tracking, regression analysis, and trend visualization for the ChimeraPhoenix audio engine suite.

## Key Features

- **Automated Build System**: Automatically builds all test programs before running tests
- **Comprehensive Test Execution**: Runs all test suites with configurable timeouts
- **Database Persistence**: SQLite database for long-term result tracking
- **Trend Analysis**: Historical performance tracking and regression detection
- **HTML Reports**: Beautiful, interactive HTML reports with graphs
- **Pass/Fail Tracking**: Detailed pass/fail statistics by category
- **Performance Metrics**: THD, CPU usage, latency, and memory tracking
- **Git Integration**: Tracks git commit and branch for each test run

## Quick Start

### Running All Tests

```bash
./test_all_comprehensive.sh
```

This will:
1. Build all test programs
2. Run the complete test suite
3. Save results to `test_results.db`
4. Generate HTML report in `test_reports/report_TIMESTAMP/`
5. Create trend graphs
6. Open the HTML report in your browser

### Command Line Options

```bash
./test_all_comprehensive.sh [options]

Options:
  --skip-build         Skip the build phase
  --build-only         Only build tests, don't run them
  --parallel           Run tests in parallel (experimental)
  --output-dir DIR     Specify output directory (default: test_reports)
  --db FILE            Database file (default: test_results.db)
  --timeout SECONDS    Timeout per test (default: 60)
  --verbose            Verbose output
  --help               Show help message
```

### Examples

**Build and run all tests:**
```bash
./test_all_comprehensive.sh
```

**Run tests only (skip build):**
```bash
./test_all_comprehensive.sh --skip-build
```

**Build only (don't run tests):**
```bash
./test_all_comprehensive.sh --build-only
```

**Custom timeout and verbose output:**
```bash
./test_all_comprehensive.sh --timeout 120 --verbose
```

**Custom database and output location:**
```bash
./test_all_comprehensive.sh --db custom.db --output-dir /tmp/test_output
```

## Querying Test History

The `query_test_history.sh` script provides powerful database querying capabilities.

### Available Commands

```bash
./query_test_history.sh [command] [options]

Commands:
  summary              Show summary of all test runs
  latest               Show latest test run details
  trends               Show trend analysis
  failures             Show tests that frequently fail
  performance          Show performance metrics over time
  compare RUN1 RUN2    Compare two test runs
  export [FILE]        Export data to CSV
```

### Query Examples

**View all test runs:**
```bash
./query_test_history.sh summary
```

**View latest test details:**
```bash
./query_test_history.sh latest
```

**Analyze trends:**
```bash
./query_test_history.sh trends
```

**Find failing tests:**
```bash
./query_test_history.sh failures
```

**View performance metrics:**
```bash
./query_test_history.sh performance
```

**Compare two runs:**
```bash
./query_test_history.sh compare 1 2
```

**Export to CSV:**
```bash
./query_test_history.sh export my_results.csv
```

**Use custom database:**
```bash
./query_test_history.sh --db custom.db summary
```

## Database Schema

### Tables

**test_runs** - Overall test run information
- `run_id`: Unique identifier
- `timestamp`: When the test was run
- `total_tests`, `passed`, `failed`, `timeout`, `skipped`: Test counts
- `duration_seconds`: How long the test run took
- `git_commit`, `git_branch`: Git context

**test_results** - Individual test results
- `result_id`: Unique identifier
- `run_id`: Foreign key to test_runs
- `test_name`: Name of the test
- `test_category`: Category (Filter, Distortion, etc.)
- `status`: PASS, FAIL, TIMEOUT, or SKIPPED
- `duration_seconds`: Test execution time
- `error_message`: Error details if failed
- `peak_level`, `rms_level`, `thd_percent`, `cpu_percent`: Performance metrics

**engine_results** - Engine-specific results
- `engine_result_id`: Unique identifier
- `run_id`: Foreign key to test_runs
- `engine_id`: Engine number (0-56)
- `engine_name`: Engine name
- `status`, `duration_seconds`: Test status and timing
- `peak_level`, `rms_level`, `thd_percent`, `cpu_percent`, `latency_samples`: Metrics

**performance_metrics** - Additional performance data
- `metric_id`: Unique identifier
- `run_id`: Foreign key to test_runs
- `metric_name`, `metric_value`, `metric_unit`: Custom metrics

## HTML Report

The HTML report includes:

### Summary Cards
- Total tests, passed, failed, timeout, skipped
- Visual stat cards with color coding

### Pass Rate Visualization
- Progress bar showing pass rate percentage
- Color-coded alerts based on pass rate

### Detailed Test Results Table
- All test results in a sortable table
- Status badges (pass/fail/timeout/skipped)
- Performance metrics (THD, CPU usage)
- Test duration

### Trend Graphs
1. **Pass/Fail Trend**: Historical pass/fail counts over time
2. **Pass Rate Trend**: Pass rate percentage over time
3. **Duration Trend**: Test execution time over time
4. **Category Breakdown**: Results by test category

## Test Categories

The system organizes tests into the following categories:

- **Filter**: EQ and filter quality tests
- **Distortion**: Distortion and saturation tests
- **Dynamics**: Compressor, limiter, gate tests
- **Modulation**: Chorus, flanger, phaser tests
- **Pitch**: Pitch shifter tests
- **Reverb**: Reverb algorithm tests
- **Spatial**: Spatial effects tests
- **Utility**: Utility effects tests
- **Quality**: THD and frequency response tests
- **Performance**: CPU and latency benchmarks
- **Stability**: Endurance and edge case tests

## Regression Tracking

The system automatically tracks regressions by:

1. **Storing Historical Results**: All test runs are stored in the database
2. **Comparing Pass/Fail Status**: Detects when passing tests start failing
3. **Monitoring Metrics**: Tracks increases in THD, CPU usage, etc.
4. **Trend Visualization**: Graphs show performance over time
5. **Git Integration**: Associates results with specific commits

### Detecting Regressions

Use the query tool to compare runs:

```bash
# Compare run 5 with run 6
./query_test_history.sh compare 5 6
```

This shows:
- Tests that changed status (pass → fail or fail → pass)
- Performance metric changes
- Duration differences

## Integration with CI/CD

The test system is designed for CI/CD integration:

### Exit Codes
- `0`: All tests passed
- `1`: Some tests failed or timed out

### Example CI Configuration

**GitHub Actions:**
```yaml
- name: Run Comprehensive Tests
  run: |
    cd standalone_test
    ./test_all_comprehensive.sh --timeout 120

- name: Upload Test Report
  uses: actions/upload-artifact@v3
  with:
    name: test-report
    path: standalone_test/test_reports/
```

**Jenkins:**
```groovy
stage('Test') {
    steps {
        sh './test_all_comprehensive.sh --timeout 120'
    }
    post {
        always {
            publishHTML([
                reportDir: 'test_reports',
                reportFiles: '*/test_report.html',
                reportName: 'Test Report'
            ])
        }
    }
}
```

## Performance Benchmarking

The system tracks multiple performance metrics:

### THD (Total Harmonic Distortion)
- Measures audio quality
- Lower is better
- Tracked per test

### CPU Usage
- Percentage of CPU used
- Indicates efficiency
- Critical for real-time audio

### Latency
- Processing delay in samples
- Important for live performance
- Tracked for each engine

### Memory Usage
- Memory consumption
- Helps detect leaks
- Tracked over long runs

## Troubleshooting

### Database Locked Error

If you see "database is locked", another process is accessing it:

```bash
# Check for locks
lsof test_results.db

# Force unlock (be careful!)
sqlite3 test_results.db "PRAGMA locking_mode=EXCLUSIVE; BEGIN EXCLUSIVE; COMMIT;"
```

### Build Failures

If tests fail to build:

1. Check build logs in `test_reports/report_TIMESTAMP/logs/`
2. Ensure all dependencies are installed
3. Try building manually: `./build_all.sh`

### Timeout Issues

If tests frequently timeout:

1. Increase timeout: `--timeout 120`
2. Check system load
3. Review test logs for infinite loops

### Missing Python Dependencies

If graph generation fails:

```bash
pip3 install matplotlib numpy
```

### HTML Report Not Opening

Manually open the report:

```bash
open test_reports/report_TIMESTAMP/test_report.html
```

## Advanced Usage

### Custom Test Selection

Edit the `tests` array in `test_all_comprehensive.sh` to customize which tests run:

```bash
local tests=(
    "filter_quality|$BUILD_DIR/filter_test|Filter|"
    "distortion_quality|$BUILD_DIR/distortion_test|Distortion|"
    # Add or remove tests here
)
```

### Adding Custom Metrics

Insert custom metrics into the database:

```sql
INSERT INTO performance_metrics (run_id, metric_name, metric_value, metric_unit)
VALUES (1, 'custom_metric', 42.5, 'dB');
```

### Exporting Data for Analysis

Export all data to CSV for external analysis:

```bash
./query_test_history.sh export full_export.csv
```

Then analyze in Excel, R, Python, etc.

### Database Maintenance

**Backup the database:**
```bash
cp test_results.db test_results_backup_$(date +%Y%m%d).db
```

**Compact the database:**
```bash
sqlite3 test_results.db "VACUUM;"
```

**Reset the database:**
```bash
rm test_results.db
./test_all_comprehensive.sh  # Will recreate
```

## File Structure

```
standalone_test/
├── test_all_comprehensive.sh       # Main test runner
├── query_test_history.sh            # Database query tool
├── test_results.db                  # SQLite database
├── test_reports/                    # Report output directory
│   └── report_TIMESTAMP/
│       ├── test_report.html         # HTML report
│       ├── logs/                    # Test logs
│       │   ├── build_*.log          # Build logs
│       │   └── *.log                # Test logs
│       ├── data/                    # JSON data files
│       │   ├── test_summary.json
│       │   ├── build_summary.json
│       │   └── *.json               # Individual test results
│       └── graphs/                  # Trend graphs
│           ├── pass_fail_trend.png
│           ├── pass_rate_trend.png
│           ├── duration_trend.png
│           └── category_breakdown.png
└── build/                           # Compiled test executables
```

## Best Practices

1. **Run Tests Regularly**: Run the full suite after every significant change
2. **Review Trends**: Check trend graphs weekly to catch gradual degradation
3. **Investigate Failures**: Don't ignore failing tests, investigate immediately
4. **Track Metrics**: Monitor THD and CPU usage trends over time
5. **Backup Database**: Regularly backup test_results.db
6. **Document Changes**: Use git commits to track what changed between runs
7. **Set Baselines**: After major improvements, create new baselines
8. **Monitor Timeouts**: Frequent timeouts indicate performance issues

## Maintenance

### Daily
- Run full test suite
- Review HTML report
- Check for new failures

### Weekly
- Review trend graphs
- Compare with previous week
- Investigate any degradation

### Monthly
- Backup database
- Export historical data
- Clean old reports

### As Needed
- Add new tests as features are added
- Update timeout values if needed
- Expand database schema for new metrics

## Support

For issues or questions:
1. Check logs in `test_reports/report_TIMESTAMP/logs/`
2. Run with `--verbose` for detailed output
3. Review database with query tool
4. Check individual test executables manually

## Version History

- **v3.0** (2025-10-11): Initial comprehensive test system
  - Automated build and test execution
  - SQLite database integration
  - HTML report generation
  - Trend graph visualization
  - Historical regression tracking

## License

Part of the ChimeraPhoenix Project
