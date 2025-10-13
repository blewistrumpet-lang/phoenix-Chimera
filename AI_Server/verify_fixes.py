#!/usr/bin/env python3
"""
Static verification of Trinity Server fixes
Checks that all fixes are present in the code without running the server
"""

import re
import sys

def check_file_contains(filepath, patterns, fix_name):
    """Check if file contains all required patterns"""
    print(f"\n{'='*60}")
    print(f"Checking: {fix_name}")
    print(f"{'='*60}")

    try:
        with open(filepath, 'r') as f:
            content = f.read()

        all_found = True
        for pattern_name, pattern in patterns.items():
            if re.search(pattern, content, re.MULTILINE):
                print(f"✅ {pattern_name}")
            else:
                print(f"❌ {pattern_name} - NOT FOUND")
                all_found = False

        return all_found
    except FileNotFoundError:
        print(f"❌ File not found: {filepath}")
        return False

def verify_all_fixes():
    """Verify all fixes are present"""
    filepath = "trinity_server.py"

    results = {}

    # Fix 1: Request Timeout
    results["timeout"] = check_file_contains(
        filepath,
        {
            "Timeout constant": r"REQUEST_TIMEOUT\s*=\s*60",
            "asyncio.timeout usage": r"async with asyncio\.timeout\(REQUEST_TIMEOUT\)",
            "TimeoutError handler": r"except asyncio\.TimeoutError:",
            "504 status code": r"status_code=504",
        },
        "FIX 1: Request Timeout (CRITICAL)"
    )

    # Fix 2: Calculator Error Recovery
    results["calculator_fallback"] = check_file_contains(
        filepath,
        {
            "Calculator try/except": r"except Exception as calc_error:",
            "Fallback to visionary": r"optimized_preset = visionary_result\[\"preset\"\]",
            "Error logging": r"Calculator optimization failed",
            "Fallback flag": r"fallback_used.*True",
        },
        "FIX 2: Calculator Error Recovery (CRITICAL)"
    )

    # Fix 3: API Key Validation
    results["api_key"] = check_file_contains(
        filepath,
        {
            "validate_api_key function": r"def validate_api_key\(\):",
            "OPENAI_API_KEY check": r"os\.getenv\(\"OPENAI_API_KEY\"\)",
            "RuntimeError raised": r"raise RuntimeError",
            "Validation called": r"validate_api_key\(\)",
        },
        "FIX 3: API Key Validation (HIGH)"
    )

    # Fix 4: Cache Lock
    results["cache_lock"] = check_file_contains(
        filepath,
        {
            "Lock declaration": r"cache_lock\s*=\s*asyncio\.Lock\(\)",
            "Lock usage": r"async with cache_lock:",
            "Lock protects operations": r"# Protect cache operations with lock",
        },
        "FIX 4: Cache Race Condition Protection (HIGH)"
    )

    # Fix 5: CORS Middleware
    results["cors"] = check_file_contains(
        filepath,
        {
            "CORS import": r"from fastapi\.middleware\.cors import CORSMiddleware",
            "CORS middleware": r"app\.add_middleware\(\s*CORSMiddleware",
            "allow_origins": r"allow_origins=",
            "allow_methods": r"allow_methods=",
        },
        "FIX 5: CORS Configuration (MEDIUM)"
    )

    # Fix 6: Enhanced Logging
    results["logging"] = check_file_contains(
        filepath,
        {
            "Stage 1 logging": r"Stage 1: Visionary",
            "Stage 2 logging": r"Stage 2:.*Calculator",
            "Stage 3 logging": r"Stage 3: Alchemist",
            "Success logging": r"SUCCESS:.*Generated preset",
            "Error with context": r"Debug info at",
        },
        "FIX 6: Comprehensive Error Logging"
    )

    # Print summary
    print(f"\n{'='*60}")
    print("VERIFICATION SUMMARY")
    print(f"{'='*60}")

    passed = sum(results.values())
    total = len(results)

    for fix_name, status in results.items():
        status_str = "✅ PASSED" if status else "❌ FAILED"
        print(f"{fix_name:.<30} {status_str}")

    print(f"{'='*60}")
    print(f"Total: {passed}/{total} fixes verified")
    print(f"{'='*60}")

    if passed == total:
        print("\n✅ ALL FIXES VERIFIED!")
        print("Trinity Server has all critical and high-priority fixes.")
        return 0
    else:
        print(f"\n❌ {total - passed} FIX(ES) MISSING")
        print("Please review the missing fixes above.")
        return 1

def verify_test_files():
    """Verify test files exist"""
    print(f"\n{'='*60}")
    print("TEST FILES VERIFICATION")
    print(f"{'='*60}")

    import os

    test_files = [
        "test_timeout.py",
        "test_calculator_fallback.py",
    ]

    all_exist = True
    for test_file in test_files:
        if os.path.exists(test_file):
            print(f"✅ {test_file}")
        else:
            print(f"❌ {test_file} - NOT FOUND")
            all_exist = False

    return all_exist

if __name__ == "__main__":
    print("="*60)
    print("TRINITY SERVER FIXES VERIFICATION")
    print("Static code analysis (no server required)")
    print("="*60)

    # Verify fixes in trinity_server.py
    exit_code = verify_all_fixes()

    # Verify test files exist
    tests_exist = verify_test_files()

    if exit_code == 0 and tests_exist:
        print("\n🎉 VERIFICATION COMPLETE - ALL FIXES PRESENT")
        print("\nNext steps:")
        print("1. Set OPENAI_API_KEY environment variable")
        print("2. Start server: python3 trinity_server.py")
        print("3. Run tests: python3 test_timeout.py")
        print("4. Run tests: python3 test_calculator_fallback.py")
        sys.exit(0)
    else:
        print("\n⚠️  VERIFICATION FAILED")
        print("Some fixes or test files are missing.")
        sys.exit(1)
