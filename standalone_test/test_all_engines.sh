#!/bin/bash
# Test all engines individually and report results

cd build

echo "Testing all engines individually..."
echo "=================================="
echo ""

passed=0
failed=0
timeout_engines=""

for i in {1..56}; do
    printf "Engine %2d: " $i

    # Run test in background with timeout
    ./standalone_test --engine $i > /tmp/test_$i.log 2>&1 &
    pid=$!

    # Wait up to 10 seconds
    count=0
    while kill -0 $pid 2>/dev/null && [ $count -lt 100 ]; do
        sleep 0.1
        count=$((count + 1))
    done

    # Check if still running
    if kill -0 $pid 2>/dev/null; then
        kill -9 $pid 2>/dev/null
        echo "TIMEOUT/HANG"
        timeout_engines="$timeout_engines $i"
        failed=$((failed + 1))
    else
        # Check result
        if grep -q "✓ PASSED" /tmp/test_$i.log; then
            echo "✓ PASS"
            passed=$((passed + 1))
        else
            echo "✗ FAIL"
            failed=$((failed + 1))
        fi
    fi
done

echo ""
echo "=================================="
echo "Results:"
echo "  Passed: $passed"
echo "  Failed: $failed"
echo "  Total:  56"
if [ -n "$timeout_engines" ]; then
    echo "  Timeout engines:$timeout_engines"
fi
