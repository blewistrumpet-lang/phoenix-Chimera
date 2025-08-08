# Chimera Phoenix Comprehensive Test Harness

A sophisticated automated testing system for all 57 engines (0-56) in the Chimera Phoenix plugin. This harness performs systematic testing to identify issues, measure performance, and provide actionable recommendations for fixes.

## Features

The comprehensive test harness performs the following tests on each engine:

### 1. Parameter Sweep Testing
- Tests all parameters across their full range (0-1)
- Validates parameter effectiveness and responsiveness  
- Detects non-functional parameters
- Measures parameter impact on audio output

### 2. Safety Checks
- **NaN/Inf Testing**: Tests engine response to problematic input values
- **Buffer Overrun Safety**: Tests various buffer sizes for safety
- **Thread Safety**: Concurrent access testing with multiple threads

### 3. Audio Quality Tests
- **Sine Wave Response**: Tests with pure tones at various frequencies
- **Noise Response**: Tests with white noise for stability
- **Transient Response**: Tests with impulses and sharp transients

### 4. Performance Metrics
- **CPU Usage**: Measures processing time and real-time capability
- **Latency**: Measures algorithmic delay introduced by processing

### 5. Stability Tests
- **Mix Parameter Linearity**: Tests blend/mix parameters for smooth behavior
- **Rapid Parameter Changes**: Tests stability under fast parameter automation
- **Bypass Stability**: Tests bypass functionality and transitions

## Building the Test Harness

### Prerequisites
- CMake 3.15 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)
- Make or Ninja build system
- Threading library support

### Quick Build and Test
```bash
# Build and run all tests
./build_and_test.sh

# Build and test specific engine
./build_and_test.sh --engine 15

# Clean build with verbose output
./build_and_test.sh --clean --verbose

# Build without running tests
./build_and_test.sh --no-test
```

### Manual Build
```bash
# Create build directory
mkdir Build && cd Build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --target ComprehensiveTestHarness

# Run
./ComprehensiveTestHarness --help
```

## Usage

### Command Line Options

```bash
ComprehensiveTestHarness [options]

Basic Options:
  --help                Show help message
  --engine ID           Test only specified engine ID (0-56)
  --verbose             Enable verbose output during testing

Audio Configuration:
  --sample-rate RATE    Set sample rate (default: 48000)
  --block-size SIZE     Set block size (default: 512)
  --duration SECONDS    Set test duration per signal (default: 2.0)
  --sweep-steps STEPS   Set parameter sweep steps (default: 20)

Execution Control:
  --parallel            Enable parallel testing (default)
  --sequential          Disable parallel testing
  --max-threads NUM     Set maximum concurrent threads

Report Generation:
  --output-dir DIR      Set output directory (default: .)
  --html-report FILE    HTML report filename
  --json-report FILE    JSON report filename  
  --csv-report FILE     CSV report filename
  --summary-report FILE Summary report filename
  --detailed-report FILE Detailed report filename
  --no-reports          Skip report generation
```

### Common Usage Examples

```bash
# Test all engines with default settings
./ComprehensiveTestHarness

# Test specific engine with verbose output
./ComprehensiveTestHarness --engine 25 --verbose

# Test with custom audio settings
./ComprehensiveTestHarness --sample-rate 96000 --block-size 256

# Sequential testing for debugging
./ComprehensiveTestHarness --sequential --verbose

# Generate reports in custom directory
./ComprehensiveTestHarness --output-dir ./results/

# Test single engine with detailed parameter analysis
./ComprehensiveTestHarness --engine 31 --sweep-steps 50 --verbose
```

## Understanding the Results

### Exit Codes
- `0`: All tests passed (or only warnings found)
- `1`: Errors found requiring attention
- `2`: Critical issues found requiring immediate attention  
- `3`: Some engines failed to create/instantiate
- `4`: Fatal error during testing

### Report Files

The harness generates multiple report formats:

#### 1. Summary Report (`test_summary.txt`)
- High-level overview of all engines
- Pass/fail statistics
- Top problematic engines with priority rankings
- Performance insights

#### 2. Detailed Report (`test_detailed.txt`) 
- Complete test results for each engine
- Detailed failure descriptions
- Specific metrics for each test
- Prioritized recommendations for fixes

#### 3. HTML Report (`test_report.html`)
- Visual dashboard with charts and tables
- Interactive results browsing
- Color-coded status indicators
- Performance visualizations

#### 4. JSON Report (`test_report.json`)
- Machine-readable test data
- Complete test metrics and results
- Suitable for automated analysis tools
- Integration with CI/CD pipelines

#### 5. CSV Report (`test_report.csv`)
- Spreadsheet-compatible format
- Easy data analysis and filtering
- Suitable for tracking progress over time

### Test Categories and Scoring

Each engine receives scores (0-100) for:

- **Parameter Sweep Tests**: Parameter functionality and effectiveness
- **Safety Tests**: Stability and error handling  
- **Audio Quality Tests**: Sound processing correctness
- **Performance Tests**: CPU usage and latency
- **Stability Tests**: Robustness under various conditions

### Issue Severity Levels

- **INFO**: Informational messages, no action required
- **WARNING**: Issues that should be addressed but don't break functionality
- **ERROR**: Problems that affect functionality or quality
- **CRITICAL**: Severe issues that can cause crashes or corrupted audio

## Interpreting Recommendations

The harness provides specific, actionable recommendations for each issue found:

### Common Parameter Issues
- **"Parameter appears to have no effect"**: Parameter not connected to processing
- **"Parameter value X caused NaN/Inf"**: Add bounds checking and validation
- **"Parameter behavior is not monotonic"**: Review parameter scaling/mapping

### Common Safety Issues  
- **"Engine produces NaN/Inf output"**: Add input sanitization and math safety
- **"Buffer size issues"**: Use dynamic buffer sizing, avoid fixed assumptions
- **"Thread safety issues"**: Add proper synchronization and atomic operations

### Common Audio Quality Issues
- **"Output exceeds full scale"**: Add output limiting or gain control
- **"Engine appears to be muting input"**: Check processing chain connectivity
- **"Excessive noise/artifacts"**: Review algorithm implementation

### Common Performance Issues
- **"Excessive CPU usage"**: Optimize algorithms, use lookup tables
- **"Not real-time capable"**: Reduce computational complexity
- **"High latency"**: Minimize algorithmic delay, optimize processing

## Engine-Specific Testing

### Testing Individual Engines

To focus on a specific problematic engine:

```bash
# Test engine with maximum detail
./ComprehensiveTestHarness --engine 42 --sweep-steps 50 --verbose --sequential

# Test with different audio settings
./ComprehensiveTestHarness --engine 42 --sample-rate 96000 --duration 5.0
```

### Engine ID Reference

Key engine IDs for testing:
- `0`: None/Bypass
- `1-6`: Dynamics & Compression  
- `7-14`: Filters & EQ
- `15-22`: Distortion & Saturation
- `23-33`: Modulation Effects
- `34-43`: Reverb & Delay
- `44-52`: Spatial & Special Effects
- `53-56`: Utility

See `EngineTypes.h` for complete engine mapping.

## Integration with Development Workflow

### Continuous Integration

Add to your CI pipeline:

```bash
# Run tests and fail on critical issues
./ComprehensiveTestHarness --no-reports
EXIT_CODE=$?

if [ $EXIT_CODE -ge 2 ]; then
    echo "Critical issues found - failing build"
    exit $EXIT_CODE  
fi
```

### Pre-Commit Testing

Test modified engines before commits:

```bash
# Test specific engines that were modified
./ComprehensiveTestHarness --engine 25 --engine 31 --verbose
```

### Performance Regression Testing

Track performance over time:

```bash
# Generate CSV for tracking
./ComprehensiveTestHarness --csv-report performance_$(date +%Y%m%d).csv

# Compare with previous results
```

## Advanced Configuration

### Custom Test Parameters

For specialized testing needs:

```bash
# Stress testing with longer duration
./ComprehensiveTestHarness --duration 10.0 --sweep-steps 100

# Low-latency testing  
./ComprehensiveTestHarness --sample-rate 192000 --block-size 64

# Memory-constrained testing
./ComprehensiveTestHarness --sequential --max-threads 1
```

### Debugging Failed Engines

For engines that fail tests:

1. **Run with verbose output**: `--verbose` shows detailed test progress
2. **Test sequentially**: `--sequential` avoids threading issues  
3. **Focus on specific tests**: Use single engine mode
4. **Adjust test parameters**: Reduce duration or steps for faster iteration

## Troubleshooting

### Common Build Issues

**Missing dependencies:**
```bash
# Ubuntu/Debian
sudo apt-get install cmake build-essential

# macOS with Homebrew  
brew install cmake

# Windows
# Install Visual Studio with C++ tools
```

**Threading issues:**
- Ensure threading library is available
- On older systems, may need to link pthread manually

### Common Runtime Issues

**Engine creation failures:**
- Check that all engine source files are included in CMakeLists.txt
- Verify engine factory registration
- Check for missing dependencies

**Memory issues:**
- Reduce parallel thread count: `--max-threads 2`
- Use sequential testing: `--sequential`
- Reduce test duration: `--duration 1.0`

**Performance issues:**
- Testing 57 engines comprehensively takes time (expect 5-15 minutes)
- Use `--engine ID` to test specific engines during development
- Enable parallel testing for faster execution

## Contributing

When adding new engines or modifying existing ones:

1. **Run comprehensive tests**: Ensure new engines pass all test categories
2. **Check recommendations**: Address any issues found by the harness
3. **Update documentation**: Add any engine-specific testing considerations
4. **Performance validation**: Ensure engines meet real-time performance requirements

## Technical Details

### Test Signal Generation
- DC offset, sine waves, white/pink noise
- Impulses, chirp sweeps, multitones  
- Drum transients, extreme levels
- Customizable test signal parameters

### Performance Measurement  
- High-resolution timing using `std::chrono`
- CPU percentage calculation based on real-time requirements
- Memory usage monitoring
- Real-time capability assessment

### Thread Safety Testing
- Concurrent parameter updates
- Multiple processing threads
- Race condition detection
- Deadlock timeout protection

### Statistical Analysis
- RMS and peak level measurements
- Crest factor and correlation analysis
- Parameter linearity and monotonicity
- Frequency response characterization

The comprehensive test harness provides a robust foundation for ensuring the quality, stability, and performance of all Chimera Phoenix engines.