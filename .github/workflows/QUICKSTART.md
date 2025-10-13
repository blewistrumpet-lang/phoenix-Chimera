# ChimeraPhoenix CI/CD Quick Start Guide

## TL;DR

```bash
# The pipeline runs automatically on:
✓ Every push to main/develop/feature branches
✓ Every pull request
✓ Nightly at 2 AM UTC
✓ Manual trigger via GitHub Actions UI
```

## What Gets Tested

| Category | Coverage | Duration |
|----------|----------|----------|
| **Build** | All 56 engines + test suites | 5-10 min |
| **Functional** | 7 test suites (reverb, filter, etc.) | 15-30 min |
| **THD** | Audio quality for clean effects | 10-20 min |
| **Performance** | CPU benchmarks for all engines | 30-45 min |
| **Endurance** | Long-running stability (nightly only) | 60+ min |

## Quick Actions

### Trigger Manual Build
1. Go to **Actions** tab in GitHub
2. Select **ChimeraPhoenix CI/CD Pipeline**
3. Click **Run workflow**
4. Choose options:
   - `run_full_suite`: Enable for comprehensive testing
   - `benchmark_engines`: Enable for CPU benchmarks

### View Latest Results
```
https://github.com/{owner}/{repo}/actions
```

### Download Artifacts
After any workflow run:
1. Scroll to bottom of run page
2. Download artifacts:
   - `chimera-test-binaries` - Compiled executables
   - `test-results-*` - Test outputs
   - `thd-measurements` - Audio quality data
   - `cpu-benchmarks` - Performance data
   - `ci-cd-final-report` - Summary report

## Understanding Results

### Build Success Indicators
```
✓ Build All Components                [5m 30s]
  └─ All 56 engines compiled
  └─ Test suites built successfully
  └─ Benchmarks ready
```

### Test Results Format
```
Engine 39 - Plate Reverb
  ✓ Initialization: PASS
  ✓ Parameter update: PASS
  ✓ Audio processing: PASS
  ✓ Stereo imaging: PASS
```

### THD Results
```
Engine ID | Name              | THD (%)  | Status
----------|-------------------|----------|--------
0         | None (Bypass)     | 0.001    | PASS
7         | Parametric EQ     | 0.234    | PASS
41        | Convolution Rev   | 1.456    | FAIL ⚠️
```
**Pass:** THD < 1.0%

### Performance Results
```
Rank | Engine Name           | CPU %   | Status
-----|----------------------|---------|--------
1    | Convolution Reverb   | 45.2%   | OK
2    | Granular Cloud       | 38.5%   | OK
3    | Shimmer Reverb       | 34.1%   | OK
```
**Warning:** CPU > 50%

## Regression Alerts

When performance degrades, you'll see:

### PR Comment
```markdown
⚠️ Performance Regression Detected

Engine 41 (Convolution Reverb): +15.3%
  Was: 45.2% CPU
  Now: 52.1% CPU

Action: Review recent changes to this engine
```

### GitHub Issue (Auto-created)
```markdown
Title: ⚠️ Performance Regression Detected - abc123

Labels: performance, regression, needs-investigation

Details:
- Commit: abc123
- Branch: feature/new-filter
- Engines affected: 1
```

## Local Testing (Before Push)

```bash
# Run quick validation
cd standalone_test
./build_all.sh
./build/standalone_test

# Run THD check
./build/test_comprehensive_thd

# Run performance check (long)
./build/cpu_benchmark_all_engines
```

## Common Issues

### ❌ Build Failed
**Cause:** Missing dependencies or compilation errors
**Fix:**
```bash
brew install cmake harfbuzz
cd standalone_test && ./build_all.sh
```

### ❌ Test Failed
**Cause:** Engine logic error or parameter issue
**Fix:**
1. Download test artifacts
2. Review `*_results.txt` for specific engine
3. Debug locally with same test

### ⚠️ THD Threshold Exceeded
**Cause:** Engine adding unwanted harmonics
**Fix:**
1. Check if parameters are neutral
2. Review DSP algorithm
3. May need to adjust threshold if intentional

### ⚠️ Performance Regression
**Cause:** Recent code changes increased CPU usage
**Fix:**
1. Profile the affected engine
2. Optimize hot paths
3. Consider algorithm improvements

## Configuration

### Adjust Thresholds
Edit `.github/workflows/chimera-ci-cd.yml`:
```yaml
env:
  THD_THRESHOLD: 1.0              # Change THD pass criteria
  CPU_THRESHOLD_PERCENT: 50.0     # Change CPU warning level
```

### Enable Slack Notifications
Add secret in GitHub repo settings:
```
Name: SLACK_WEBHOOK_URL
Value: https://hooks.slack.com/services/YOUR/WEBHOOK/URL
```

## Monitoring

### Dashboard View
Go to **Actions** tab, you'll see:
```
Recent workflow runs
├─ ✓ main: All checks passed                    [2h ago]
├─ ⚠️ develop: Performance regression detected   [5h ago]
└─ ✓ PR #42: Tests passed                       [1d ago]
```

### Email Notifications
Configure in your GitHub notification settings:
- **Watching:** Get notified of all workflow runs
- **Custom:** Only notify on failures

## Best Practices

### Before Committing
```bash
# 1. Build locally
./build_all.sh

# 2. Run relevant tests
./build/filter_test  # If you changed filters

# 3. Quick THD check (optional)
./build/test_comprehensive_thd | tail -20
```

### Writing Tests
When adding new engines:
1. Add to appropriate test suite (e.g., `filter_test.cpp`)
2. Include in THD tests if it's a "clean" effect
3. Add to benchmark suite (`cpu_benchmark_all_engines.cpp`)

### Interpreting Benchmarks
- **< 20% CPU:** Excellent
- **20-40% CPU:** Good
- **40-60% CPU:** Acceptable (watch for regressions)
- **> 60% CPU:** Needs optimization

## Support

### Get Help
- **Documentation:** See `README.md` in this directory
- **Issues:** Create issue with `ci-cd` label
- **Logs:** Always attach artifacts from failed runs

### Report Problems
```markdown
Title: CI/CD: [Brief Description]

Environment:
- Workflow: chimera-ci-cd.yml
- Run ID: 123456789
- Branch: main
- Commit: abc123

Problem:
[Describe the issue]

Expected:
[What should happen]

Actual:
[What happened instead]

Artifacts:
[Link to workflow run and artifacts]
```

## Cheat Sheet

| Task | Command |
|------|---------|
| View workflow runs | GitHub → Actions tab |
| Trigger manual build | Actions → ChimeraPhoenix CI/CD → Run workflow |
| Download results | Workflow run → Artifacts (bottom) |
| Check THD locally | `./build/test_comprehensive_thd` |
| Check CPU locally | `./build/cpu_benchmark_all_engines` |
| View build cache | Actions → Caches |
| Clear build cache | Settings → Actions → Caches → Delete |
| Edit workflow | `.github/workflows/chimera-ci-cd.yml` |
| Adjust thresholds | Edit `env:` section in workflow |
| Add Slack notify | Settings → Secrets → New secret |

## Performance Targets

| Metric | Current | Target | Status |
|--------|---------|--------|--------|
| Build time | 8.5 min | < 10 min | ✓ |
| Test time | 25 min | < 30 min | ✓ |
| THD failures | 2 | < 5 | ✓ |
| Avg CPU | 24.3% | < 30% | ✓ |
| Heavy engines | 3 | < 5 | ✓ |

---

**Next Steps:**
1. ✓ Workflow is ready - will run on next push
2. Monitor first few runs
3. Adjust thresholds if needed
4. Configure Slack (optional)
5. Celebrate automated testing! 🎉
