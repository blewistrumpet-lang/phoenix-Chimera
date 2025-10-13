#!/bin/bash
################################################################################
# test_all_comprehensive.sh
# Comprehensive Automated Test Runner for ChimeraPhoenix
#
# Features:
#   - Builds all test programs automatically
#   - Runs comprehensive test suite
#   - Captures detailed results
#   - Generates HTML report with pass/fail statistics
#   - Creates trend graphs for regression tracking
#   - Saves results to SQLite database
#   - Tracks historical performance trends
#
# Usage: ./test_all_comprehensive.sh [options]
#   --skip-build         Skip build phase
#   --output-dir DIR     Specify output directory (default: test_reports)
#   --db FILE            Database file (default: test_results.db)
#   --build-only         Only build tests, don't run them
#   --parallel           Run tests in parallel (experimental)
#   --timeout SECONDS    Timeout per test (default: 60)
#   --verbose            Verbose output
################################################################################

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Script paths
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
OUTPUT_DIR="$SCRIPT_DIR/test_reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_DIR="$OUTPUT_DIR/report_$TIMESTAMP"
DB_FILE="$SCRIPT_DIR/test_results.db"

# Options
SKIP_BUILD=false
BUILD_ONLY=false
PARALLEL=false
TIMEOUT=60
VERBOSE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --build-only)
            BUILD_ONLY=true
            shift
            ;;
        --parallel)
            PARALLEL=true
            shift
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            REPORT_DIR="$OUTPUT_DIR/report_$TIMESTAMP"
            shift 2
            ;;
        --db)
            DB_FILE="$2"
            shift 2
            ;;
        --timeout)
            TIMEOUT="$2"
            shift 2
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            grep "^#" "$0" | grep -v "#!/bin/bash" | sed 's/^# //'
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Create directories
mkdir -p "$OUTPUT_DIR"
mkdir -p "$REPORT_DIR"
mkdir -p "$REPORT_DIR/logs"
mkdir -p "$REPORT_DIR/data"
mkdir -p "$REPORT_DIR/graphs"

# Logging function
log() {
    local level=$1
    shift
    local msg="$@"
    local timestamp=$(date "+%Y-%m-%d %H:%M:%S")

    case $level in
        INFO)
            echo -e "${CYAN}[INFO]${NC} $msg"
            ;;
        SUCCESS)
            echo -e "${GREEN}[SUCCESS]${NC} $msg"
            ;;
        WARNING)
            echo -e "${YELLOW}[WARNING]${NC} $msg"
            ;;
        ERROR)
            echo -e "${RED}[ERROR]${NC} $msg"
            ;;
        DEBUG)
            if [ "$VERBOSE" = true ]; then
                echo -e "${MAGENTA}[DEBUG]${NC} $msg"
            fi
            ;;
    esac

    echo "[$timestamp] [$level] $msg" >> "$REPORT_DIR/test_runner.log"
}

################################################################################
# Print Banner
################################################################################
print_banner() {
    echo ""
    echo -e "${BOLD}${CYAN}╔════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${CYAN}║                                                                    ║${NC}"
    echo -e "${BOLD}${CYAN}║         ChimeraPhoenix Comprehensive Test Suite v3.0              ║${NC}"
    echo -e "${BOLD}${CYAN}║                                                                    ║${NC}"
    echo -e "${BOLD}${CYAN}╚════════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${BOLD}Test Run:${NC}     $TIMESTAMP"
    echo -e "${BOLD}Output Dir:${NC}   $REPORT_DIR"
    echo -e "${BOLD}Database:${NC}     $DB_FILE"
    echo -e "${BOLD}Timeout:${NC}      ${TIMEOUT}s per test"
    echo ""
}

################################################################################
# Initialize Database
################################################################################
init_database() {
    log INFO "Initializing database: $DB_FILE"

    sqlite3 "$DB_FILE" <<EOF
-- Test runs table
CREATE TABLE IF NOT EXISTS test_runs (
    run_id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,
    total_tests INTEGER NOT NULL,
    passed INTEGER NOT NULL,
    failed INTEGER NOT NULL,
    timeout INTEGER NOT NULL,
    skipped INTEGER NOT NULL,
    duration_seconds REAL NOT NULL,
    git_commit TEXT,
    git_branch TEXT
);

-- Individual test results
CREATE TABLE IF NOT EXISTS test_results (
    result_id INTEGER PRIMARY KEY AUTOINCREMENT,
    run_id INTEGER NOT NULL,
    test_name TEXT NOT NULL,
    test_category TEXT NOT NULL,
    status TEXT NOT NULL,
    duration_seconds REAL NOT NULL,
    error_message TEXT,
    peak_level REAL,
    rms_level REAL,
    thd_percent REAL,
    cpu_percent REAL,
    memory_mb REAL,
    FOREIGN KEY (run_id) REFERENCES test_runs(run_id)
);

-- Engine-specific results
CREATE TABLE IF NOT EXISTS engine_results (
    engine_result_id INTEGER PRIMARY KEY AUTOINCREMENT,
    run_id INTEGER NOT NULL,
    engine_id INTEGER NOT NULL,
    engine_name TEXT NOT NULL,
    status TEXT NOT NULL,
    duration_seconds REAL NOT NULL,
    peak_level REAL,
    rms_level REAL,
    thd_percent REAL,
    cpu_percent REAL,
    latency_samples INTEGER,
    FOREIGN KEY (run_id) REFERENCES test_runs(run_id)
);

-- Performance metrics
CREATE TABLE IF NOT EXISTS performance_metrics (
    metric_id INTEGER PRIMARY KEY AUTOINCREMENT,
    run_id INTEGER NOT NULL,
    metric_name TEXT NOT NULL,
    metric_value REAL NOT NULL,
    metric_unit TEXT NOT NULL,
    FOREIGN KEY (run_id) REFERENCES test_runs(run_id)
);

-- Create indexes for faster queries
CREATE INDEX IF NOT EXISTS idx_test_results_run_id ON test_results(run_id);
CREATE INDEX IF NOT EXISTS idx_test_results_status ON test_results(status);
CREATE INDEX IF NOT EXISTS idx_engine_results_run_id ON engine_results(run_id);
CREATE INDEX IF NOT EXISTS idx_engine_results_engine_id ON engine_results(engine_id);
CREATE INDEX IF NOT EXISTS idx_performance_metrics_run_id ON performance_metrics(run_id);
EOF

    log SUCCESS "Database initialized"
}

################################################################################
# Build All Test Programs
################################################################################
build_all_tests() {
    log INFO "Building all test programs..."
    echo ""

    local build_scripts=(
        "build_all.sh"
        "build_comprehensive_thd.sh"
        "build_frequency_response_test.sh"
        "build_pitch_test.sh"
        "build_latency_suite.sh"
        "build_cpu_benchmark.sh"
        "build_endurance_test.sh"
        "build_silence_test.sh"
        "build_sample_rate_test.sh"
        "build_dc_offset_test.sh"
    )

    local build_success=0
    local build_failed=0
    local build_start=$(date +%s)

    for script in "${build_scripts[@]}"; do
        if [ -f "$SCRIPT_DIR/$script" ]; then
            log INFO "Running $script..."
            if bash "$SCRIPT_DIR/$script" > "$REPORT_DIR/logs/build_${script%.sh}.log" 2>&1; then
                log SUCCESS "$script completed"
                ((build_success++))
            else
                log ERROR "$script failed (see logs)"
                ((build_failed++))
            fi
        else
            log DEBUG "$script not found, skipping"
        fi
    done

    local build_end=$(date +%s)
    local build_duration=$((build_end - build_start))

    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo -e "${BOLD}Build Summary:${NC}"
    echo "  Successful: $build_success"
    echo "  Failed:     $build_failed"
    echo "  Duration:   ${build_duration}s"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    if [ $build_failed -gt 0 ]; then
        log WARNING "Some builds failed. Continuing with available tests..."
    fi

    # Save build summary
    cat > "$REPORT_DIR/data/build_summary.json" <<EOF
{
    "timestamp": "$TIMESTAMP",
    "successful": $build_success,
    "failed": $build_failed,
    "duration_seconds": $build_duration
}
EOF
}

################################################################################
# Run Individual Test
################################################################################
run_test() {
    local test_name=$1
    local test_executable=$2
    local test_category=$3
    local test_args=${4:-""}

    local log_file="$REPORT_DIR/logs/${test_name}.log"
    local data_file="$REPORT_DIR/data/${test_name}.json"
    local start_time=$(date +%s)

    log DEBUG "Running test: $test_name"

    # Check if executable exists
    if [ ! -f "$test_executable" ]; then
        log WARNING "Test executable not found: $test_executable"
        echo "{\"status\":\"SKIPPED\",\"reason\":\"Executable not found\"}" > "$data_file"
        return 2
    fi

    # Run test with timeout
    local exit_code=0
    if timeout ${TIMEOUT}s "$test_executable" $test_args > "$log_file" 2>&1; then
        exit_code=0
    else
        exit_code=$?
    fi

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    # Parse results
    local status="UNKNOWN"
    local peak_level="null"
    local rms_level="null"
    local thd_percent="null"
    local cpu_percent="null"
    local error_message=""

    if [ $exit_code -eq 124 ]; then
        status="TIMEOUT"
        error_message="Test exceeded ${TIMEOUT}s timeout"
    elif [ $exit_code -eq 0 ]; then
        if grep -q "✓ PASSED\|PASS\|SUCCESS" "$log_file" 2>/dev/null; then
            status="PASS"
        elif grep -q "✗ FAILED\|FAIL\|ERROR" "$log_file" 2>/dev/null; then
            status="FAIL"
            error_message=$(grep -i "error\|fail" "$log_file" | head -1 | tr -d '\n' || echo "Test failed")
        else
            status="PASS"
        fi
    else
        status="FAIL"
        error_message="Test exited with code $exit_code"
    fi

    # Extract metrics
    peak_level=$(grep -oP "Peak.*?:\s*\K[0-9.+-]+" "$log_file" 2>/dev/null | head -1 || echo "null")
    rms_level=$(grep -oP "RMS.*?:\s*\K[0-9.+-]+" "$log_file" 2>/dev/null | head -1 || echo "null")
    thd_percent=$(grep -oP "THD.*?:\s*\K[0-9.]+" "$log_file" 2>/dev/null | head -1 || echo "null")
    cpu_percent=$(grep -oP "CPU.*?:\s*\K[0-9.]+" "$log_file" 2>/dev/null | head -1 || echo "null")

    # Create JSON result
    cat > "$data_file" <<EOF
{
    "test_name": "$test_name",
    "category": "$test_category",
    "status": "$status",
    "duration_seconds": $duration,
    "exit_code": $exit_code,
    "error_message": "$error_message",
    "metrics": {
        "peak_level": $peak_level,
        "rms_level": $rms_level,
        "thd_percent": $thd_percent,
        "cpu_percent": $cpu_percent
    }
}
EOF

    # Return status code
    case $status in
        PASS)
            return 0
            ;;
        TIMEOUT)
            return 124
            ;;
        SKIPPED)
            return 2
            ;;
        *)
            return 1
            ;;
    esac
}

################################################################################
# Run All Tests
################################################################################
run_all_tests() {
    log INFO "Running comprehensive test suite..."
    echo ""

    local test_start=$(date +%s)
    local total_tests=0
    local passed=0
    local failed=0
    local timeout=0
    local skipped=0

    # Define all tests to run
    # Format: "test_name|executable_path|category|args"
    local tests=(
        "filter_quality|$BUILD_DIR/filter_test|Filter|"
        "distortion_quality|$BUILD_DIR/distortion_test|Distortion|"
        "dynamics_quality|$BUILD_DIR/dynamics_test|Dynamics|"
        "modulation_quality|$BUILD_DIR/modulation_test|Modulation|"
        "pitch_quality|$BUILD_DIR/pitch_test|Pitch|"
        "reverb_quality|$BUILD_DIR/reverb_test|Reverb|"
        "spatial_quality|$BUILD_DIR/spatial_test|Spatial|"
        "utility_quality|$BUILD_DIR/utility_test|Utility|"
        "thd_comprehensive|$BUILD_DIR/test_comprehensive_thd|Quality|"
        "frequency_response|$BUILD_DIR/test_frequency_response_8_14|Quality|"
        "latency_suite|$BUILD_DIR/latency_measurement_suite|Performance|"
        "cpu_benchmark|$BUILD_DIR/cpu_benchmark_all_engines|Performance|"
        "endurance_test|$BUILD_DIR/endurance_test|Stability|"
        "silence_handling|$BUILD_DIR/test_silence_handling|Stability|"
        "sample_rate_test|$BUILD_DIR/test_sample_rate_independence|Stability|"
        "dc_offset_test|$BUILD_DIR/test_dc_offset|Stability|"
    )

    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo -e "${BOLD}Running Tests${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    for test_def in "${tests[@]}"; do
        IFS='|' read -r test_name test_executable test_category test_args <<< "$test_def"
        ((total_tests++))

        printf "%-30s " "$test_name"

        if run_test "$test_name" "$test_executable" "$test_category" "$test_args"; then
            echo -e "${GREEN}✓ PASS${NC}"
            ((passed++))
        else
            local result=$?
            if [ $result -eq 124 ]; then
                echo -e "${YELLOW}⏱ TIMEOUT${NC}"
                ((timeout++))
            elif [ $result -eq 2 ]; then
                echo -e "${BLUE}⊘ SKIPPED${NC}"
                ((skipped++))
            else
                echo -e "${RED}✗ FAIL${NC}"
                ((failed++))
            fi
        fi
    done

    local test_end=$(date +%s)
    local test_duration=$((test_end - test_start))

    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo -e "${BOLD}Test Results Summary:${NC}"
    echo "  Total:      $total_tests"
    echo "  Passed:     $passed"
    echo "  Failed:     $failed"
    echo "  Timeout:    $timeout"
    echo "  Skipped:    $skipped"
    echo "  Duration:   ${test_duration}s"
    if [ $total_tests -gt 0 ]; then
        local pass_rate=$(awk "BEGIN {printf \"%.1f\", ($passed/$total_tests)*100}")
        echo "  Pass Rate:  ${pass_rate}%"
    fi
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    # Save test summary
    cat > "$REPORT_DIR/data/test_summary.json" <<EOF
{
    "timestamp": "$TIMESTAMP",
    "total_tests": $total_tests,
    "passed": $passed,
    "failed": $failed,
    "timeout": $timeout,
    "skipped": $skipped,
    "duration_seconds": $test_duration
}
EOF

    # Return values via global variables for database insertion
    TOTAL_TESTS=$total_tests
    PASSED_TESTS=$passed
    FAILED_TESTS=$failed
    TIMEOUT_TESTS=$timeout
    SKIPPED_TESTS=$skipped
    TEST_DURATION=$test_duration
}

################################################################################
# Save Results to Database
################################################################################
save_to_database() {
    log INFO "Saving results to database..."

    # Get git information
    local git_commit=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
    local git_branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")

    # Insert test run
    sqlite3 "$DB_FILE" <<EOF
INSERT INTO test_runs (timestamp, total_tests, passed, failed, timeout, skipped, duration_seconds, git_commit, git_branch)
VALUES ('$TIMESTAMP', $TOTAL_TESTS, $PASSED_TESTS, $FAILED_TESTS, $TIMEOUT_TESTS, $SKIPPED_TESTS, $TEST_DURATION, '$git_commit', '$git_branch');
EOF

    # Get the run_id
    local run_id=$(sqlite3 "$DB_FILE" "SELECT last_insert_rowid();")

    log DEBUG "Run ID: $run_id"

    # Insert individual test results
    for json_file in "$REPORT_DIR/data"/*.json; do
        if [ -f "$json_file" ] && [ "$(basename "$json_file")" != "test_summary.json" ] && [ "$(basename "$json_file")" != "build_summary.json" ]; then
            # Parse JSON and insert
            python3 - "$json_file" "$run_id" "$DB_FILE" <<'PYTHON_SCRIPT'
import json
import sys
import sqlite3

json_file = sys.argv[1]
run_id = sys.argv[2]
db_file = sys.argv[3]

with open(json_file, 'r') as f:
    data = json.load(f)

conn = sqlite3.connect(db_file)
cursor = conn.cursor()

test_name = data.get('test_name', 'unknown')
test_category = data.get('category', 'unknown')
status = data.get('status', 'UNKNOWN')
duration = data.get('duration_seconds', 0)
error_message = data.get('error_message', '')
metrics = data.get('metrics', {})

peak_level = metrics.get('peak_level')
rms_level = metrics.get('rms_level')
thd_percent = metrics.get('thd_percent')
cpu_percent = metrics.get('cpu_percent')

cursor.execute('''
    INSERT INTO test_results (run_id, test_name, test_category, status, duration_seconds, error_message, peak_level, rms_level, thd_percent, cpu_percent)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
''', (run_id, test_name, test_category, status, duration, error_message, peak_level, rms_level, thd_percent, cpu_percent))

conn.commit()
conn.close()
PYTHON_SCRIPT
        fi
    done

    log SUCCESS "Results saved to database (run_id: $run_id)"
    RUN_ID=$run_id
}

################################################################################
# Generate Trend Graphs
################################################################################
generate_trend_graphs() {
    log INFO "Generating trend graphs..."

    python3 - "$DB_FILE" "$REPORT_DIR/graphs" <<'PYTHON_SCRIPT'
import sqlite3
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from datetime import datetime

db_file = sys.argv[1]
output_dir = sys.argv[2]

conn = sqlite3.connect(db_file)
cursor = conn.cursor()

# Get historical test runs
cursor.execute('''
    SELECT timestamp, passed, failed, timeout, skipped, duration_seconds
    FROM test_runs
    ORDER BY timestamp
''')
runs = cursor.fetchall()

if not runs or len(runs) < 2:
    print("Not enough historical data for trends")
    conn.close()
    sys.exit(0)

timestamps = [datetime.strptime(r[0], '%Y%m%d_%H%M%S') for r in runs]
passed = [r[1] for r in runs]
failed = [r[2] for r in runs]
timeout = [r[3] for r in runs]
skipped = [r[4] for r in runs]
durations = [r[5] for r in runs]

# Graph 1: Pass/Fail Trend
fig, ax = plt.subplots(figsize=(12, 6))
ax.plot(timestamps, passed, marker='o', label='Passed', color='green', linewidth=2)
ax.plot(timestamps, failed, marker='s', label='Failed', color='red', linewidth=2)
ax.plot(timestamps, timeout, marker='^', label='Timeout', color='orange', linewidth=2)
ax.set_xlabel('Test Run Date', fontsize=12)
ax.set_ylabel('Number of Tests', fontsize=12)
ax.set_title('Test Results Trend Over Time', fontsize=14, fontweight='bold')
ax.legend()
ax.grid(True, alpha=0.3)
plt.xticks(rotation=45)
plt.tight_layout()
plt.savefig(f'{output_dir}/pass_fail_trend.png', dpi=150)
plt.close()

# Graph 2: Pass Rate Trend
fig, ax = plt.subplots(figsize=(12, 6))
total = [p + f + t for p, f, t in zip(passed, failed, timeout)]
pass_rate = [(p / t * 100) if t > 0 else 0 for p, t in zip(passed, total)]
ax.plot(timestamps, pass_rate, marker='o', color='blue', linewidth=2)
ax.fill_between(timestamps, pass_rate, alpha=0.3, color='blue')
ax.set_xlabel('Test Run Date', fontsize=12)
ax.set_ylabel('Pass Rate (%)', fontsize=12)
ax.set_title('Pass Rate Trend Over Time', fontsize=14, fontweight='bold')
ax.set_ylim(0, 105)
ax.axhline(y=100, color='green', linestyle='--', alpha=0.5, label='100% Target')
ax.axhline(y=90, color='orange', linestyle='--', alpha=0.5, label='90% Threshold')
ax.legend()
ax.grid(True, alpha=0.3)
plt.xticks(rotation=45)
plt.tight_layout()
plt.savefig(f'{output_dir}/pass_rate_trend.png', dpi=150)
plt.close()

# Graph 3: Test Duration Trend
fig, ax = plt.subplots(figsize=(12, 6))
ax.plot(timestamps, durations, marker='o', color='purple', linewidth=2)
ax.fill_between(timestamps, durations, alpha=0.3, color='purple')
ax.set_xlabel('Test Run Date', fontsize=12)
ax.set_ylabel('Duration (seconds)', fontsize=12)
ax.set_title('Test Execution Time Trend', fontsize=14, fontweight='bold')
ax.grid(True, alpha=0.3)
plt.xticks(rotation=45)
plt.tight_layout()
plt.savefig(f'{output_dir}/duration_trend.png', dpi=150)
plt.close()

# Graph 4: Test Category Breakdown (Latest Run)
cursor.execute('''
    SELECT test_category, status, COUNT(*) as count
    FROM test_results
    WHERE run_id = (SELECT MAX(run_id) FROM test_runs)
    GROUP BY test_category, status
''')
category_data = cursor.fetchall()

if category_data:
    from collections import defaultdict
    categories = defaultdict(lambda: {'PASS': 0, 'FAIL': 0, 'TIMEOUT': 0})

    for cat, status, count in category_data:
        categories[cat][status] = count

    cat_names = list(categories.keys())
    pass_counts = [categories[c]['PASS'] for c in cat_names]
    fail_counts = [categories[c]['FAIL'] for c in cat_names]
    timeout_counts = [categories[c]['TIMEOUT'] for c in cat_names]

    fig, ax = plt.subplots(figsize=(12, 6))
    x = range(len(cat_names))
    width = 0.25

    ax.bar([i - width for i in x], pass_counts, width, label='Passed', color='green')
    ax.bar(x, fail_counts, width, label='Failed', color='red')
    ax.bar([i + width for i in x], timeout_counts, width, label='Timeout', color='orange')

    ax.set_xlabel('Test Category', fontsize=12)
    ax.set_ylabel('Number of Tests', fontsize=12)
    ax.set_title('Test Results by Category (Latest Run)', fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(cat_names, rotation=45, ha='right')
    ax.legend()
    ax.grid(True, alpha=0.3, axis='y')
    plt.tight_layout()
    plt.savefig(f'{output_dir}/category_breakdown.png', dpi=150)
    plt.close()

print(f"Generated {4} trend graphs")

conn.close()
PYTHON_SCRIPT

    log SUCCESS "Trend graphs generated"
}

################################################################################
# Generate HTML Report
################################################################################
generate_html_report() {
    log INFO "Generating HTML report..."

    local html_file="$REPORT_DIR/test_report.html"

    # Read test summary
    local test_summary=$(cat "$REPORT_DIR/data/test_summary.json" 2>/dev/null || echo '{}')

    # Get historical stats from database
    local total_runs=$(sqlite3 "$DB_FILE" "SELECT COUNT(*) FROM test_runs;" 2>/dev/null || echo "0")
    local avg_pass_rate=$(sqlite3 "$DB_FILE" "SELECT ROUND(AVG(passed * 100.0 / total_tests), 1) FROM test_runs WHERE total_tests > 0;" 2>/dev/null || echo "0.0")

    cat > "$html_file" <<'HTML_HEADER'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ChimeraPhoenix Test Report</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            line-height: 1.6;
            color: #333;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
        }
        .container {
            max-width: 1400px;
            margin: 0 auto;
            background: white;
            border-radius: 12px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 40px;
            text-align: center;
        }
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.2);
        }
        .header .subtitle {
            font-size: 1.2em;
            opacity: 0.9;
        }
        .summary {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            padding: 40px;
            background: #f8f9fa;
        }
        .stat-card {
            background: white;
            padding: 25px;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
            text-align: center;
            transition: transform 0.3s ease;
        }
        .stat-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 4px 12px rgba(0,0,0,0.15);
        }
        .stat-card .number {
            font-size: 3em;
            font-weight: bold;
            margin: 10px 0;
        }
        .stat-card .label {
            color: #666;
            font-size: 0.9em;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .stat-card.passed .number { color: #28a745; }
        .stat-card.failed .number { color: #dc3545; }
        .stat-card.timeout .number { color: #ffc107; }
        .stat-card.skipped .number { color: #6c757d; }
        .stat-card.total .number { color: #007bff; }

        .section {
            padding: 40px;
        }
        .section-title {
            font-size: 1.8em;
            margin-bottom: 20px;
            padding-bottom: 10px;
            border-bottom: 3px solid #667eea;
            color: #333;
        }
        .test-results {
            margin-top: 20px;
        }
        .test-table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
        }
        .test-table th {
            background: #667eea;
            color: white;
            padding: 15px;
            text-align: left;
            font-weight: 600;
        }
        .test-table td {
            padding: 12px 15px;
            border-bottom: 1px solid #ddd;
        }
        .test-table tr:hover {
            background: #f8f9fa;
        }
        .status {
            display: inline-block;
            padding: 5px 15px;
            border-radius: 20px;
            font-weight: bold;
            font-size: 0.85em;
            text-transform: uppercase;
        }
        .status.pass { background: #d4edda; color: #155724; }
        .status.fail { background: #f8d7da; color: #721c24; }
        .status.timeout { background: #fff3cd; color: #856404; }
        .status.skipped { background: #e2e3e5; color: #383d41; }

        .graphs {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(500px, 1fr));
            gap: 30px;
            margin-top: 20px;
        }
        .graph {
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
        }
        .graph img {
            width: 100%;
            height: auto;
            border-radius: 4px;
        }
        .graph-title {
            font-size: 1.2em;
            font-weight: bold;
            margin-bottom: 15px;
            color: #333;
        }

        .footer {
            background: #f8f9fa;
            padding: 30px 40px;
            text-align: center;
            color: #666;
            border-top: 1px solid #dee2e6;
        }
        .progress-bar {
            width: 100%;
            height: 30px;
            background: #e9ecef;
            border-radius: 15px;
            overflow: hidden;
            margin: 20px 0;
        }
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #28a745 0%, #20c997 100%);
            transition: width 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: bold;
        }
        .alert {
            padding: 15px;
            border-radius: 8px;
            margin: 20px 0;
        }
        .alert.success {
            background: #d4edda;
            color: #155724;
            border-left: 4px solid #28a745;
        }
        .alert.warning {
            background: #fff3cd;
            color: #856404;
            border-left: 4px solid #ffc107;
        }
        .alert.danger {
            background: #f8d7da;
            color: #721c24;
            border-left: 4px solid #dc3545;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>ChimeraPhoenix Test Report</h1>
HTML_HEADER

    echo "            <div class=\"subtitle\">Test Run: $TIMESTAMP</div>" >> "$html_file"
    echo "        </div>" >> "$html_file"

    # Add summary cards
    cat >> "$html_file" <<EOF
        <div class="summary">
            <div class="stat-card total">
                <div class="label">Total Tests</div>
                <div class="number">$TOTAL_TESTS</div>
            </div>
            <div class="stat-card passed">
                <div class="label">Passed</div>
                <div class="number">$PASSED_TESTS</div>
            </div>
            <div class="stat-card failed">
                <div class="label">Failed</div>
                <div class="number">$FAILED_TESTS</div>
            </div>
            <div class="stat-card timeout">
                <div class="label">Timeout</div>
                <div class="number">$TIMEOUT_TESTS</div>
            </div>
            <div class="stat-card skipped">
                <div class="label">Skipped</div>
                <div class="number">$SKIPPED_TESTS</div>
            </div>
        </div>
EOF

    # Add pass rate progress bar
    if [ $TOTAL_TESTS -gt 0 ]; then
        local pass_rate=$(awk "BEGIN {printf \"%.1f\", ($PASSED_TESTS/$TOTAL_TESTS)*100}")
        local pass_rate_int=$(awk "BEGIN {printf \"%.0f\", ($PASSED_TESTS/$TOTAL_TESTS)*100}")

        cat >> "$html_file" <<EOF
        <div class="section">
            <h2 class="section-title">Pass Rate</h2>
            <div class="progress-bar">
                <div class="progress-fill" style="width: ${pass_rate_int}%">
                    ${pass_rate}%
                </div>
            </div>
EOF

        if [ "$pass_rate_int" -ge 95 ]; then
            echo "            <div class=\"alert success\"><strong>Excellent!</strong> Pass rate is ${pass_rate}%</div>" >> "$html_file"
        elif [ "$pass_rate_int" -ge 80 ]; then
            echo "            <div class=\"alert warning\"><strong>Good</strong> Pass rate is ${pass_rate}% - some tests need attention</div>" >> "$html_file"
        else
            echo "            <div class=\"alert danger\"><strong>Action Required!</strong> Pass rate is ${pass_rate}% - multiple tests failing</div>" >> "$html_file"
        fi

        echo "        </div>" >> "$html_file"
    fi

    # Add test results table
    cat >> "$html_file" <<EOF
        <div class="section">
            <h2 class="section-title">Test Results</h2>
            <table class="test-table">
                <thead>
                    <tr>
                        <th>Test Name</th>
                        <th>Category</th>
                        <th>Status</th>
                        <th>Duration (s)</th>
                        <th>THD %</th>
                        <th>CPU %</th>
                    </tr>
                </thead>
                <tbody>
EOF

    # Add each test result
    for json_file in "$REPORT_DIR/data"/*.json; do
        if [ -f "$json_file" ] && [ "$(basename "$json_file")" != "test_summary.json" ] && [ "$(basename "$json_file")" != "build_summary.json" ]; then
            python3 - "$json_file" >> "$html_file" <<'PYTHON_TABLE'
import json
import sys

json_file = sys.argv[1]
with open(json_file, 'r') as f:
    data = json.load(f)

test_name = data.get('test_name', 'unknown')
category = data.get('category', 'unknown')
status = data.get('status', 'UNKNOWN')
duration = data.get('duration_seconds', 0)
metrics = data.get('metrics', {})
thd = metrics.get('thd_percent', 'N/A')
cpu = metrics.get('cpu_percent', 'N/A')

status_class = status.lower()
thd_str = f"{thd:.2f}" if isinstance(thd, (int, float)) else "N/A"
cpu_str = f"{cpu:.2f}" if isinstance(cpu, (int, float)) else "N/A"

print(f'''                    <tr>
                        <td>{test_name}</td>
                        <td>{category}</td>
                        <td><span class="status {status_class}">{status}</span></td>
                        <td>{duration}</td>
                        <td>{thd_str}</td>
                        <td>{cpu_str}</td>
                    </tr>''')
PYTHON_TABLE
        fi
    done

    cat >> "$html_file" <<EOF
                </tbody>
            </table>
        </div>
EOF

    # Add trend graphs
    cat >> "$html_file" <<EOF
        <div class="section">
            <h2 class="section-title">Trend Analysis</h2>
            <div class="graphs">
EOF

    for graph in "$REPORT_DIR/graphs"/*.png; do
        if [ -f "$graph" ]; then
            local graph_name=$(basename "$graph" .png)
            local graph_title=$(echo "$graph_name" | sed 's/_/ /g' | sed 's/\b\(.\)/\u\1/g')
            cat >> "$html_file" <<EOF
                <div class="graph">
                    <div class="graph-title">$graph_title</div>
                    <img src="graphs/$(basename "$graph")" alt="$graph_title">
                </div>
EOF
        fi
    done

    cat >> "$html_file" <<EOF
            </div>
        </div>
EOF

    # Add footer
    cat >> "$html_file" <<EOF
        <div class="footer">
            <p><strong>ChimeraPhoenix Test Suite v3.0</strong></p>
            <p>Generated on $(date "+%Y-%m-%d %H:%M:%S")</p>
            <p>Database: $DB_FILE | Total Runs: $total_runs | Average Pass Rate: ${avg_pass_rate}%</p>
        </div>
    </div>
</body>
</html>
EOF

    log SUCCESS "HTML report generated: $html_file"
}

################################################################################
# Main Execution
################################################################################
main() {
    print_banner

    # Initialize database
    init_database

    # Build phase
    if [ "$SKIP_BUILD" = false ]; then
        build_all_tests
    else
        log INFO "Skipping build phase (--skip-build)"
    fi

    # Exit if build-only
    if [ "$BUILD_ONLY" = true ]; then
        log SUCCESS "Build-only mode complete"
        exit 0
    fi

    # Run tests
    run_all_tests

    # Save to database
    save_to_database

    # Generate visualizations
    generate_trend_graphs

    # Generate HTML report
    generate_html_report

    # Final summary
    echo ""
    echo -e "${BOLD}${GREEN}╔════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${GREEN}║                      TEST RUN COMPLETE                             ║${NC}"
    echo -e "${BOLD}${GREEN}╚════════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${BOLD}Report Location:${NC}"
    echo "  HTML Report:  $REPORT_DIR/test_report.html"
    echo "  Database:     $DB_FILE"
    echo "  Logs:         $REPORT_DIR/logs/"
    echo "  Graphs:       $REPORT_DIR/graphs/"
    echo ""
    echo -e "${BOLD}Quick Stats:${NC}"
    echo "  Tests Run:    $TOTAL_TESTS"
    echo "  Passed:       $PASSED_TESTS"
    echo "  Failed:       $FAILED_TESTS"
    echo "  Pass Rate:    $(awk "BEGIN {printf \"%.1f%%\", ($PASSED_TESTS/$TOTAL_TESTS)*100}")"
    echo ""

    # Open HTML report
    if command -v open &> /dev/null; then
        log INFO "Opening HTML report..."
        open "$REPORT_DIR/test_report.html"
    fi

    # Exit with appropriate code
    if [ $FAILED_TESTS -eq 0 ] && [ $TIMEOUT_TESTS -eq 0 ]; then
        exit 0
    else
        exit 1
    fi
}

# Run main function
main
