#!/bin/bash
# Local CI Test Runner
# Simulates the CI/CD pipeline locally for development/debugging

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║     ChimeraPhoenix CI/CD Local Test Runner                ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

THD_THRESHOLD=1.0
CPU_THRESHOLD=50.0
RUN_FULL_SUITE=false
RUN_BENCHMARKS=false
QUICK_MODE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --full)
            RUN_FULL_SUITE=true
            shift
            ;;
        --benchmarks)
            RUN_BENCHMARKS=true
            shift
            ;;
        --quick)
            QUICK_MODE=true
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --full         Run full test suite (slower)"
            echo "  --benchmarks   Run CPU benchmarks"
            echo "  --quick        Quick validation (build + one test)"
            echo "  --help         Show this help"
            echo ""
            echo "Examples:"
            echo "  $0                    # Basic tests"
            echo "  $0 --full             # Full test suite"
            echo "  $0 --benchmarks       # With performance tests"
            echo "  $0 --quick            # Fast validation"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Results tracking
TESTS_PASSED=0
TESTS_FAILED=0
BUILD_TIME=0
declare -a FAILED_TESTS

# Helper functions
log_section() {
    echo ""
    echo "════════════════════════════════════════════════════════════"
    echo "$1"
    echo "════════════════════════════════════════════════════════════"
    echo ""
}

log_step() {
    echo "→ $1"
}

log_success() {
    echo "✓ $1"
}

log_error() {
    echo "✗ $1"
}

log_warning() {
    echo "⚠ $1"
}

run_test() {
    local test_name=$1
    local test_binary=$2
    local timeout_seconds=${3:-300}

    if [ ! -f "$test_binary" ]; then
        log_warning "$test_name: Binary not found, skipping"
        return 0
    fi

    log_step "Running $test_name..."

    if timeout ${timeout_seconds}s "$test_binary" > "${test_name}_results.txt" 2>&1; then
        # Check results
        PASSED=$(grep -c "PASS\|✓" "${test_name}_results.txt" || echo "0")
        FAILED=$(grep -c "FAIL\|✗" "${test_name}_results.txt" || echo "0")

        if [ "$FAILED" -gt 0 ]; then
            log_error "$test_name: $FAILED tests failed"
            TESTS_FAILED=$((TESTS_FAILED + 1))
            FAILED_TESTS+=("$test_name")
            return 1
        else
            log_success "$test_name: All tests passed ($PASSED)"
            TESTS_PASSED=$((TESTS_PASSED + 1))
            return 0
        fi
    else
        log_error "$test_name: Test execution failed or timed out"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        FAILED_TESTS+=("$test_name")
        return 1
    fi
}

# =============================================================================
# STEP 1: Build
# =============================================================================

log_section "STEP 1: Building Test Suite"

log_step "Checking dependencies..."
if ! command -v clang++ &> /dev/null; then
    log_error "clang++ not found. Install Xcode Command Line Tools."
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    log_warning "cmake not found. Some features may not work."
fi

log_step "Building all test executables..."
START_TIME=$(date +%s)

if ./build_all.sh > build_local.log 2>&1; then
    END_TIME=$(date +%s)
    BUILD_TIME=$((END_TIME - START_TIME))
    log_success "Build completed in ${BUILD_TIME} seconds"
else
    log_error "Build failed. Check build_local.log for details."
    tail -50 build_local.log
    exit 1
fi

# Verify binaries
log_step "Verifying build artifacts..."
EXPECTED_BINS=(
    "build/standalone_test"
    "build/reverb_test"
    "build/filter_test"
    "build/distortion_test"
    "build/dynamics_test"
    "build/modulation_test"
    "build/pitch_test"
    "build/spatial_test"
)

ALL_FOUND=true
for bin in "${EXPECTED_BINS[@]}"; do
    if [ -f "$bin" ]; then
        log_success "Found: $bin"
    else
        log_error "Missing: $bin"
        ALL_FOUND=false
    fi
done

if [ "$ALL_FOUND" = false ]; then
    log_error "Build verification failed"
    exit 1
fi

# Quick mode: just build and exit
if [ "$QUICK_MODE" = true ]; then
    log_section "QUICK MODE COMPLETE"
    log_success "Build successful in ${BUILD_TIME}s"
    echo ""
    echo "To run tests, use: $0 --full"
    exit 0
fi

# =============================================================================
# STEP 2: Functional Tests
# =============================================================================

log_section "STEP 2: Running Functional Tests"

# Define tests to run
if [ "$RUN_FULL_SUITE" = true ]; then
    TEST_SUITES=(
        "reverb:build/reverb_test:300"
        "filter:build/filter_test:180"
        "distortion:build/distortion_test:180"
        "dynamics:build/dynamics_test:180"
        "modulation:build/modulation_test:240"
        "pitch:build/pitch_test:300"
        "spatial:build/spatial_test:240"
    )
else
    # Quick test: just reverb
    TEST_SUITES=(
        "reverb:build/reverb_test:300"
        "filter:build/filter_test:180"
    )
    log_warning "Running subset of tests. Use --full for complete suite."
fi

for suite_def in "${TEST_SUITES[@]}"; do
    IFS=':' read -r name binary timeout <<< "$suite_def"
    run_test "$name" "$binary" "$timeout"
done

# =============================================================================
# STEP 3: THD Measurements
# =============================================================================

log_section "STEP 3: THD Analysis"

if [ -f "build/test_comprehensive_thd" ]; then
    log_step "Running THD measurements..."

    if timeout 1800s build/test_comprehensive_thd > thd_local_output.txt 2>&1; then
        log_success "THD analysis completed"

        if [ -f "comprehensive_thd_results.csv" ]; then
            # Analyze results
            FAILED_THD=$(awk -F',' -v threshold=$THD_THRESHOLD '
                NR>1 && $3 ~ /^[0-9.]+$/ && $3 > threshold { count++ }
                END { print count+0 }
            ' comprehensive_thd_results.csv)

            WORST_THD=$(awk -F',' 'NR>1 && $3 ~ /^[0-9.]+$/ {
                if ($3 > max) { max=$3; engine=$2 }
            } END { print max " - " engine }' comprehensive_thd_results.csv)

            echo "  Engines exceeding ${THD_THRESHOLD}% THD: $FAILED_THD"
            echo "  Worst THD: $WORST_THD"

            if [ "$FAILED_THD" -gt 5 ]; then
                log_warning "Many engines exceed THD threshold"
            else
                log_success "THD results within acceptable range"
            fi
        fi
    else
        log_error "THD analysis failed or timed out"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
else
    log_warning "THD test binary not found, skipping"
fi

# =============================================================================
# STEP 4: CPU Benchmarks (Optional)
# =============================================================================

if [ "$RUN_BENCHMARKS" = true ]; then
    log_section "STEP 4: CPU Performance Benchmarks"

    if [ -f "build/cpu_benchmark_all_engines" ]; then
        log_step "Running CPU benchmarks (this may take 30-45 minutes)..."

        if timeout 3600s build/cpu_benchmark_all_engines > cpu_benchmark_local_output.txt 2>&1; then
            log_success "CPU benchmarks completed"

            if [ -f "cpu_benchmark_results.csv" ]; then
                # Analyze results
                AVG_CPU=$(awk -F',' 'NR>1 && $6 ~ /^[0-9.]+$/ { sum+=$6; count++ }
                    END { print sum/count }' cpu_benchmark_results.csv)

                HEAVY_COUNT=$(awk -F',' -v threshold=$CPU_THRESHOLD '
                    NR>1 && $6 ~ /^[0-9.]+$/ && $6 > threshold { count++ }
                    END { print count+0 }
                ' cpu_benchmark_results.csv)

                echo "  Average CPU Usage: ${AVG_CPU}%"
                echo "  Engines exceeding ${CPU_THRESHOLD}% CPU: $HEAVY_COUNT"

                # Show top 5
                echo ""
                echo "  Top 5 Most CPU-Intensive Engines:"
                awk -F',' 'NR>1 && NR<=6 { printf "    %d. %-35s %6.2f%%\n", $1, $3, $6 }' cpu_benchmark_results.csv

                if [ "$HEAVY_COUNT" -gt 10 ]; then
                    log_warning "Many engines exceed CPU threshold"
                else
                    log_success "CPU performance within acceptable range"
                fi
            fi
        else
            log_error "CPU benchmarks failed or timed out"
            TESTS_FAILED=$((TESTS_FAILED + 1))
        fi
    else
        log_warning "CPU benchmark binary not found, skipping"
    fi
fi

# =============================================================================
# Final Report
# =============================================================================

log_section "TEST SUMMARY"

TOTAL_TESTS=$((TESTS_PASSED + TESTS_FAILED))

echo "Build Time:        ${BUILD_TIME}s"
echo "Tests Passed:      $TESTS_PASSED"
echo "Tests Failed:      $TESTS_FAILED"
echo "Total Tests:       $TOTAL_TESTS"

if [ $TOTAL_TESTS -gt 0 ]; then
    PASS_RATE=$((TESTS_PASSED * 100 / TOTAL_TESTS))
    echo "Pass Rate:         ${PASS_RATE}%"
fi

echo ""

if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
    echo "Failed Tests:"
    for test in "${FAILED_TESTS[@]}"; do
        echo "  ✗ $test"
        echo "    See: ${test}_results.txt"
    done
    echo ""
fi

echo "Results Files:"
echo "  • Build log:       build_local.log"
echo "  • Test results:    *_results.txt"
if [ -f "comprehensive_thd_results.csv" ]; then
    echo "  • THD results:     comprehensive_thd_results.csv"
fi
if [ -f "cpu_benchmark_results.csv" ]; then
    echo "  • CPU benchmarks:  cpu_benchmark_results.csv"
fi

echo ""
echo "════════════════════════════════════════════════════════════"

if [ $TESTS_FAILED -eq 0 ]; then
    echo "✓ ALL TESTS PASSED"
    echo "════════════════════════════════════════════════════════════"
    echo ""
    echo "Your code is ready for CI/CD pipeline!"
    exit 0
else
    echo "✗ SOME TESTS FAILED"
    echo "════════════════════════════════════════════════════════════"
    echo ""
    echo "Please fix failing tests before pushing to CI."
    exit 1
fi
