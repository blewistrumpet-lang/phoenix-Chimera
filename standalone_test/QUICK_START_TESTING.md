# Quick Start Guide - Comprehensive Test System

## 1. Run Your First Test Suite

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
./test_all_comprehensive.sh
```

This single command will:
- Build all test programs
- Run all tests
- Generate an HTML report
- Open the report in your browser
- Save results to a database for future comparison

**Expected Output:**
```
╔════════════════════════════════════════════════════════════════════╗
║                                                                    ║
║         ChimeraPhoenix Comprehensive Test Suite v3.0              ║
║                                                                    ║
╚════════════════════════════════════════════════════════════════════╝

Test Run:     20251011_143025
Output Dir:   /path/to/test_reports/report_20251011_143025
Database:     /path/to/test_results.db
Timeout:      60s per test

[INFO] Initializing database...
[INFO] Building all test programs...
[INFO] Running comprehensive test suite...

filter_quality                 ✓ PASS
distortion_quality             ✓ PASS
dynamics_quality               ✓ PASS
...

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Test Results Summary:
  Total:      16
  Passed:     14
  Failed:     2
  Timeout:    0
  Skipped:    0
  Duration:   145s
  Pass Rate:  87.5%
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## 2. View the HTML Report

The HTML report is automatically opened in your browser. If not, manually open:

```bash
open test_reports/report_TIMESTAMP/test_report.html
```

The report includes:
- **Summary cards** with test statistics
- **Pass rate visualization** with progress bar
- **Detailed test results** in a table
- **Trend graphs** showing historical performance

## 3. Query Test History

After running tests a few times, query the database:

### View all test runs
```bash
./query_test_history.sh summary
```

### View latest run details
```bash
./query_test_history.sh latest
```

### See performance trends
```bash
./query_test_history.sh trends
```

### Find failing tests
```bash
./query_test_history.sh failures
```

## 4. Common Scenarios

### Scenario 1: Quick Test (Skip Build)

If you just built recently and only want to run tests:

```bash
./test_all_comprehensive.sh --skip-build
```

### Scenario 2: Build Only

To just build all tests without running them:

```bash
./test_all_comprehensive.sh --build-only
```

### Scenario 3: Custom Timeout

For slower systems or stress tests:

```bash
./test_all_comprehensive.sh --timeout 120
```

### Scenario 4: Verbose Output

For debugging:

```bash
./test_all_comprehensive.sh --verbose
```

### Scenario 5: Custom Output Location

```bash
./test_all_comprehensive.sh --output-dir /tmp/my_tests --db /tmp/my_tests.db
```

## 5. Compare Two Test Runs

After making code changes, compare performance:

```bash
# Run tests before changes
./test_all_comprehensive.sh
# (note the run_id from output or check with: ./query_test_history.sh summary)

# Make your code changes
# ...

# Run tests again
./test_all_comprehensive.sh

# Compare (e.g., run 1 vs run 2)
./query_test_history.sh compare 1 2
```

## 6. Understanding Results

### Test Status Meanings

- **✓ PASS**: Test completed successfully
- **✗ FAIL**: Test failed (check logs)
- **⏱ TIMEOUT**: Test exceeded timeout limit
- **⊘ SKIPPED**: Test was skipped (executable missing)

### Performance Metrics

- **THD %**: Total Harmonic Distortion (lower is better)
  - < 0.1%: Professional quality
  - < 1%: Excellent
  - < 5%: Good
  - > 5%: Needs attention

- **CPU %**: CPU usage per engine
  - < 1%: Very efficient
  - < 5%: Good
  - < 10%: Acceptable
  - > 10%: May need optimization

### Pass Rate Interpretation

- **100%**: Perfect! All tests passing
- **95-99%**: Excellent, minor issues
- **90-94%**: Good, some issues to address
- **80-89%**: Fair, needs attention
- **< 80%**: Poor, major issues

## 7. Troubleshooting

### Problem: Build Fails

**Check build logs:**
```bash
ls test_reports/report_TIMESTAMP/logs/build_*.log
cat test_reports/report_TIMESTAMP/logs/build_all.log
```

**Try building manually:**
```bash
./build_all.sh
```

### Problem: Tests Timeout

**Increase timeout:**
```bash
./test_all_comprehensive.sh --timeout 180
```

**Check specific test log:**
```bash
cat test_reports/report_TIMESTAMP/logs/filter_quality.log
```

### Problem: Database Locked

```bash
# Check what's using the database
lsof test_results.db

# Kill any stuck processes
# Then try again
```

### Problem: Graphs Not Generating

**Install Python dependencies:**
```bash
pip3 install matplotlib numpy
```

### Problem: HTML Report Not Opening

```bash
# Open manually
open test_reports/report_TIMESTAMP/test_report.html

# Or find the most recent report
open $(ls -t test_reports/*/test_report.html | head -1)
```

## 8. Daily Workflow

### Morning Routine
```bash
# Run full test suite
./test_all_comprehensive.sh

# Check pass rate and review failures
./query_test_history.sh latest
```

### After Code Changes
```bash
# Run tests
./test_all_comprehensive.sh --skip-build

# If any failures, check the logs
cat test_reports/report_TIMESTAMP/logs/FAILED_TEST.log
```

### Weekly Review
```bash
# Check trends
./query_test_history.sh trends

# Find problematic tests
./query_test_history.sh failures

# Export data for deeper analysis
./query_test_history.sh export weekly_report.csv
```

## 9. Advanced Tips

### Tip 1: Alias for Quick Access

Add to your `~/.bashrc` or `~/.zshrc`:

```bash
alias test-all='cd /path/to/standalone_test && ./test_all_comprehensive.sh'
alias test-query='cd /path/to/standalone_test && ./query_test_history.sh'
alias test-view='open $(ls -t /path/to/standalone_test/test_reports/*/test_report.html | head -1)'
```

Then use:
```bash
test-all              # Run all tests
test-query latest     # View latest results
test-view             # Open latest report
```

### Tip 2: Scheduled Testing

Run tests automatically with cron:

```bash
# Edit crontab
crontab -e

# Run tests every night at 2 AM
0 2 * * * cd /path/to/standalone_test && ./test_all_comprehensive.sh --skip-build > /dev/null 2>&1
```

### Tip 3: Custom Test Selection

Edit `test_all_comprehensive.sh` to customize which tests run. Find the `tests` array and modify:

```bash
local tests=(
    "filter_quality|$BUILD_DIR/filter_test|Filter|"
    "distortion_quality|$BUILD_DIR/distortion_test|Distortion|"
    # Comment out tests you don't want to run
    # "dynamics_quality|$BUILD_DIR/dynamics_test|Dynamics|"
)
```

### Tip 4: Export for Analysis

Export results and analyze in external tools:

```bash
# Export to CSV
./query_test_history.sh export test_data.csv

# Open in Excel/Numbers
open test_data.csv

# Or analyze with Python
python3 -c "
import pandas as pd
df = pd.read_csv('test_data.csv')
print(df.groupby('test_category')['pass_rate'].mean())
"
```

## 10. Next Steps

1. **Run your first test** - Start with the basic command
2. **Review the HTML report** - Understand what each section shows
3. **Check the database** - Use query tool to explore historical data
4. **Set up automation** - Create aliases or cron jobs
5. **Customize for your needs** - Adjust timeouts, test selection, etc.

## Quick Reference Card

```bash
# RUN TESTS
./test_all_comprehensive.sh                    # Full run (build + test)
./test_all_comprehensive.sh --skip-build       # Tests only
./test_all_comprehensive.sh --build-only       # Build only
./test_all_comprehensive.sh --timeout 120      # Custom timeout

# QUERY DATABASE
./query_test_history.sh summary                # All runs
./query_test_history.sh latest                 # Latest run
./query_test_history.sh trends                 # Trends
./query_test_history.sh failures               # Failing tests
./query_test_history.sh performance            # Performance metrics
./query_test_history.sh compare 1 2            # Compare runs
./query_test_history.sh export data.csv        # Export to CSV

# VIEW REPORTS
open test_reports/report_TIMESTAMP/test_report.html
ls -lt test_reports/                           # List all reports
cat test_reports/report_TIMESTAMP/logs/*.log   # View logs
```

## Getting Help

```bash
./test_all_comprehensive.sh --help
./query_test_history.sh --help
```

Or check the full documentation:
- `COMPREHENSIVE_TEST_SYSTEM_README.md` - Complete documentation
- `test_reports/report_TIMESTAMP/logs/` - Detailed logs

---

**Ready to start?**

```bash
./test_all_comprehensive.sh
```

Happy Testing!
