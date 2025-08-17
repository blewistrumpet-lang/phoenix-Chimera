#\!/usr/bin/env python3
import re

test_output = """
[0] Testing MuffFuzz...
  Bypass/Mix          : ✅ PASS (Bypass error: 0.000000)
  NaN/Inf/Denormal    : ❌ FAIL (NaN: 0, Inf: 0, Denorm: 1)
  Reset               : ✅ PASS (Residual: 0.000000)
  Block Invariance    : ✅ PASS (Max diff: 0.000000)
  CPU Usage           : ✅ PASS (0.000000%)

[1] Testing RodentDistortion...
  Bypass/Mix          : ❌ FAIL (Bypass error: 0.110196)
  NaN/Inf/Denormal    : ❌ FAIL (NaN: 0, Inf: 0, Denorm: 1)
  Reset               : ✅ PASS (Residual: 0.000000)
  Block Invariance    : ❌ FAIL (Max diff: 0.175382)
  CPU Usage           : ✅ PASS (1.025362%)

[2] Testing GritCrusher...
  Bypass/Mix          : ❌ FAIL (Bypass error: 0.118352)
  NaN/Inf/Denormal    : ❌ FAIL (NaN: 0, Inf: 0, Denorm: 1)
  Reset               : ✅ PASS (Residual: 0.000000)
  Block Invariance    : ❌ FAIL (Max diff: 0.320920)
  CPU Usage           : ✅ PASS (0.676142%)
"""

print("Test Results Analysis")
print("=" * 40)

engines_tested = re.findall(r'\[(\d+)\] Testing (\w+)\.\.\.', test_output)
print(f"Engines tested: {len(engines_tested)}")
for id, name in engines_tested:
    print(f"  [{id}] {name}")

print("\nTest Results Summary:")
pass_count = test_output.count("✅ PASS")
fail_count = test_output.count("❌ FAIL")
print(f"  Total PASS: {pass_count}")
print(f"  Total FAIL: {fail_count}")
print(f"  Pass rate: {pass_count/(pass_count+fail_count)*100:.1f}%")

print("\nCommon Issues:")
if "Denorm: 1" in test_output:
    print("  - Denormal numbers detected (need DenormalGuard)")
if "Bypass error" in test_output:
    errors = re.findall(r'Bypass error: ([\d.]+)', test_output)
    non_zero = [float(e) for e in errors if float(e) > 0.001]
    if non_zero:
        print(f"  - Bypass/Mix law violations in {len(non_zero)} engines")
if "Block Invariance" in test_output and "FAIL" in test_output:
    print("  - Block size invariance issues (non-deterministic processing)")
