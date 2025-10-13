#!/bin/bash
################################################################################
# query_test_history.sh
# Query and analyze historical test data from the test results database
#
# Usage: ./query_test_history.sh [command] [options]
#
# Commands:
#   summary              Show summary of all test runs
#   latest               Show latest test run details
#   trends               Show trend analysis
#   failures             Show tests that frequently fail
#   performance          Show performance metrics over time
#   compare RUN1 RUN2    Compare two test runs
#   export CSV           Export data to CSV
################################################################################

DB_FILE="${1:-test_results.db}"

if [ "$1" = "--db" ]; then
    DB_FILE="$2"
    shift 2
fi

COMMAND="${1:-summary}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'
BOLD='\033[1m'

# Check if database exists
if [ ! -f "$DB_FILE" ]; then
    echo -e "${RED}Error: Database file not found: $DB_FILE${NC}"
    echo "Run ./test_all_comprehensive.sh first to create the database"
    exit 1
fi

################################################################################
# Show summary of all test runs
################################################################################
show_summary() {
    echo -e "${BOLD}${CYAN}Test Runs Summary${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    sqlite3 -column -header "$DB_FILE" <<EOF
SELECT
    run_id as "Run",
    timestamp as "Timestamp",
    total_tests as "Total",
    passed as "Pass",
    failed as "Fail",
    timeout as "Timeout",
    ROUND(passed * 100.0 / total_tests, 1) || '%' as "Pass Rate",
    duration_seconds || 's' as "Duration"
FROM test_runs
ORDER BY run_id DESC
LIMIT 20;
EOF

    echo ""
    echo -e "${BOLD}Overall Statistics:${NC}"
    sqlite3 "$DB_FILE" <<EOF
.mode list
.separator ' | '
SELECT
    'Total Runs: ' || COUNT(*) ||
    ' | Avg Pass Rate: ' || ROUND(AVG(passed * 100.0 / total_tests), 1) || '%' ||
    ' | Avg Duration: ' || ROUND(AVG(duration_seconds), 1) || 's'
FROM test_runs;
EOF
    echo ""
}

################################################################################
# Show latest test run details
################################################################################
show_latest() {
    echo -e "${BOLD}${CYAN}Latest Test Run Details${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    local latest_run=$(sqlite3 "$DB_FILE" "SELECT MAX(run_id) FROM test_runs;")

    sqlite3 -box "$DB_FILE" <<EOF
SELECT
    timestamp as "Timestamp",
    total_tests as "Total",
    passed as "Passed",
    failed as "Failed",
    timeout as "Timeout",
    skipped as "Skipped",
    ROUND(passed * 100.0 / total_tests, 1) || '%' as "Pass Rate",
    duration_seconds || 's' as "Duration",
    git_branch as "Branch",
    SUBSTR(git_commit, 1, 8) as "Commit"
FROM test_runs
WHERE run_id = $latest_run;
EOF

    echo ""
    echo -e "${BOLD}Test Results:${NC}"
    echo ""

    sqlite3 -column -header "$DB_FILE" <<EOF
SELECT
    test_name as "Test",
    test_category as "Category",
    status as "Status",
    duration_seconds || 's' as "Duration",
    COALESCE(ROUND(thd_percent, 2), 'N/A') as "THD%",
    COALESCE(ROUND(cpu_percent, 1), 'N/A') as "CPU%"
FROM test_results
WHERE run_id = $latest_run
ORDER BY test_category, test_name;
EOF
    echo ""
}

################################################################################
# Show trend analysis
################################################################################
show_trends() {
    echo -e "${BOLD}${CYAN}Test Trend Analysis${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    echo -e "${BOLD}Pass Rate Trend (Last 10 Runs):${NC}"
    echo ""

    sqlite3 "$DB_FILE" <<EOF
.mode column
.headers on
SELECT
    run_id as "Run",
    timestamp as "Date",
    ROUND(passed * 100.0 / total_tests, 1) as "Pass%",
    CASE
        WHEN LAG(passed * 100.0 / total_tests) OVER (ORDER BY run_id) IS NULL THEN 'N/A'
        WHEN passed * 100.0 / total_tests > LAG(passed * 100.0 / total_tests) OVER (ORDER BY run_id) THEN '↑'
        WHEN passed * 100.0 / total_tests < LAG(passed * 100.0 / total_tests) OVER (ORDER BY run_id) THEN '↓'
        ELSE '='
    END as "Trend"
FROM test_runs
ORDER BY run_id DESC
LIMIT 10;
EOF

    echo ""
    echo -e "${BOLD}Duration Trend:${NC}"
    echo ""

    sqlite3 -column -header "$DB_FILE" <<EOF
SELECT
    run_id as "Run",
    duration_seconds as "Duration(s)",
    ROUND(AVG(duration_seconds) OVER (ORDER BY run_id ROWS BETWEEN 2 PRECEDING AND CURRENT ROW), 1) as "Moving Avg"
FROM test_runs
ORDER BY run_id DESC
LIMIT 10;
EOF
    echo ""
}

################################################################################
# Show frequently failing tests
################################################################################
show_failures() {
    echo -e "${BOLD}${CYAN}Frequently Failing Tests${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    sqlite3 -column -header "$DB_FILE" <<EOF
SELECT
    test_name as "Test",
    test_category as "Category",
    COUNT(*) as "Total Runs",
    SUM(CASE WHEN status = 'FAIL' THEN 1 ELSE 0 END) as "Failures",
    SUM(CASE WHEN status = 'TIMEOUT' THEN 1 ELSE 0 END) as "Timeouts",
    ROUND(SUM(CASE WHEN status = 'FAIL' THEN 1 ELSE 0 END) * 100.0 / COUNT(*), 1) || '%' as "Fail Rate"
FROM test_results
GROUP BY test_name, test_category
HAVING SUM(CASE WHEN status = 'FAIL' OR status = 'TIMEOUT' THEN 1 ELSE 0 END) > 0
ORDER BY "Failures" DESC, "Timeouts" DESC;
EOF
    echo ""
}

################################################################################
# Show performance metrics
################################################################################
show_performance() {
    echo -e "${BOLD}${CYAN}Performance Metrics Over Time${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    echo -e "${BOLD}Average THD by Test (Last Run):${NC}"
    echo ""

    local latest_run=$(sqlite3 "$DB_FILE" "SELECT MAX(run_id) FROM test_runs;")

    sqlite3 -column -header "$DB_FILE" <<EOF
SELECT
    test_name as "Test",
    ROUND(AVG(thd_percent), 3) as "Avg THD%",
    ROUND(MIN(thd_percent), 3) as "Min THD%",
    ROUND(MAX(thd_percent), 3) as "Max THD%"
FROM test_results
WHERE run_id = $latest_run AND thd_percent IS NOT NULL
GROUP BY test_name
ORDER BY "Avg THD%" DESC;
EOF

    echo ""
    echo -e "${BOLD}Average CPU Usage by Test (Last Run):${NC}"
    echo ""

    sqlite3 -column -header "$DB_FILE" <<EOF
SELECT
    test_name as "Test",
    ROUND(AVG(cpu_percent), 2) as "Avg CPU%",
    ROUND(MIN(cpu_percent), 2) as "Min CPU%",
    ROUND(MAX(cpu_percent), 2) as "Max CPU%"
FROM test_results
WHERE run_id = $latest_run AND cpu_percent IS NOT NULL
GROUP BY test_name
ORDER BY "Avg CPU%" DESC;
EOF
    echo ""
}

################################################################################
# Compare two test runs
################################################################################
compare_runs() {
    local run1=$1
    local run2=$2

    if [ -z "$run1" ] || [ -z "$run2" ]; then
        echo -e "${RED}Error: Please specify two run IDs to compare${NC}"
        echo "Usage: $0 compare RUN1 RUN2"
        exit 1
    fi

    echo -e "${BOLD}${CYAN}Comparing Test Runs: $run1 vs $run2${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    echo -e "${BOLD}Run Summary:${NC}"
    echo ""

    sqlite3 -box "$DB_FILE" <<EOF
SELECT
    run_id as "Run",
    timestamp as "Timestamp",
    passed as "Passed",
    failed as "Failed",
    timeout as "Timeout",
    ROUND(passed * 100.0 / total_tests, 1) || '%' as "Pass Rate",
    duration_seconds || 's' as "Duration"
FROM test_runs
WHERE run_id IN ($run1, $run2)
ORDER BY run_id;
EOF

    echo ""
    echo -e "${BOLD}Status Changes:${NC}"
    echo ""

    sqlite3 -column -header "$DB_FILE" <<EOF
SELECT
    a.test_name as "Test",
    a.status as "Run $run1",
    b.status as "Run $run2",
    CASE
        WHEN a.status = 'PASS' AND b.status != 'PASS' THEN 'REGRESSION'
        WHEN a.status != 'PASS' AND b.status = 'PASS' THEN 'IMPROVEMENT'
        ELSE 'NO CHANGE'
    END as "Change"
FROM test_results a
JOIN test_results b ON a.test_name = b.test_name
WHERE a.run_id = $run1 AND b.run_id = $run2
    AND (a.status != b.status)
ORDER BY "Change" DESC, a.test_name;
EOF
    echo ""
}

################################################################################
# Export to CSV
################################################################################
export_csv() {
    local output_file=${1:-"test_history_export.csv"}

    echo "Exporting test history to $output_file..."

    sqlite3 -header -csv "$DB_FILE" <<EOF > "$output_file"
SELECT
    tr.run_id,
    tr.timestamp,
    tr.total_tests,
    tr.passed,
    tr.failed,
    tr.timeout,
    tr.skipped,
    ROUND(tr.passed * 100.0 / tr.total_tests, 1) as pass_rate,
    tr.duration_seconds,
    tr.git_branch,
    tr.git_commit,
    tres.test_name,
    tres.test_category,
    tres.status,
    tres.duration_seconds as test_duration,
    tres.peak_level,
    tres.rms_level,
    tres.thd_percent,
    tres.cpu_percent
FROM test_runs tr
LEFT JOIN test_results tres ON tr.run_id = tres.run_id
ORDER BY tr.run_id DESC, tres.test_name;
EOF

    echo -e "${GREEN}Exported to: $output_file${NC}"
}

################################################################################
# Show help
################################################################################
show_help() {
    echo "Query Test History Database"
    echo ""
    echo "Usage: $0 [--db DATABASE] [command] [options]"
    echo ""
    echo "Commands:"
    echo "  summary              Show summary of all test runs (default)"
    echo "  latest               Show latest test run details"
    echo "  trends               Show trend analysis"
    echo "  failures             Show tests that frequently fail"
    echo "  performance          Show performance metrics over time"
    echo "  compare RUN1 RUN2    Compare two test runs"
    echo "  export [FILE]        Export data to CSV (default: test_history_export.csv)"
    echo "  help                 Show this help message"
    echo ""
    echo "Options:"
    echo "  --db FILE            Specify database file (default: test_results.db)"
    echo ""
    echo "Examples:"
    echo "  $0 summary"
    echo "  $0 latest"
    echo "  $0 compare 1 2"
    echo "  $0 export my_export.csv"
    echo "  $0 --db custom.db summary"
}

################################################################################
# Main
################################################################################
case "$COMMAND" in
    summary)
        show_summary
        ;;
    latest)
        show_latest
        ;;
    trends)
        show_trends
        ;;
    failures)
        show_failures
        ;;
    performance)
        show_performance
        ;;
    compare)
        compare_runs "$2" "$3"
        ;;
    export)
        export_csv "$2"
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo -e "${RED}Unknown command: $COMMAND${NC}"
        echo ""
        show_help
        exit 1
        ;;
esac
