#!/usr/bin/env python3
"""
Trinity Validation Suite - Comprehensive Testing for Production Deployment
This script validates that all Trinity components are working correctly.
Can be run as part of CI/CD pipeline.
"""

import requests
import json
import time
import sys
from typing import Dict, List, Any, Tuple
from datetime import datetime


class TrinityValidator:
    """Comprehensive validation suite for Trinity pipeline"""

    def __init__(self, base_url: str = "http://localhost:8000"):
        self.base_url = base_url
        self.results = []
        self.start_time = datetime.now()

    def log_test(self, test_name: str, passed: bool, details: str = "", data: Dict = None):
        """Log a test result"""
        result = {
            'test': test_name,
            'passed': passed,
            'details': details,
            'timestamp': datetime.now().isoformat()
        }
        if data:
            result['data'] = data
        self.results.append(result)

        status = "✅ PASS" if passed else "❌ FAIL"
        print(f"{status}: {test_name}")
        if details:
            print(f"       {details}")

        return passed

    def test_health_endpoint(self) -> bool:
        """Test 0: Health check endpoint"""
        try:
            response = requests.get(f"{self.base_url}/health", timeout=5)
            if response.status_code == 200:
                data = response.json()
                status = data.get('status')
                components = data.get('components', {})

                all_ready = all(
                    v in ['ready', 'intelligent']
                    for v in components.values()
                )

                if status == 'healthy' and all_ready:
                    return self.log_test(
                        "Health Endpoint",
                        True,
                        f"All components ready: {', '.join(components.keys())}",
                        data
                    )
                else:
                    return self.log_test(
                        "Health Endpoint",
                        False,
                        f"Status: {status}, Components: {components}"
                    )
            else:
                return self.log_test(
                    "Health Endpoint",
                    False,
                    f"HTTP {response.status_code}"
                )
        except Exception as e:
            return self.log_test("Health Endpoint", False, str(e))

    def test_preset_generation(self, prompt: str, test_name: str) -> Tuple[bool, Dict]:
        """Test preset generation with validation"""
        try:
            response = requests.post(
                f"{self.base_url}/generate",
                json={'prompt': prompt},
                timeout=60
            )

            if response.status_code != 200:
                self.log_test(
                    test_name,
                    False,
                    f"HTTP {response.status_code}: {response.text[:100]}"
                )
                return False, {}

            data = response.json()
            preset = data.get('preset', {})

            # Validate structure
            validations = {
                'has_slots': 'slots' in preset,
                'slot_count': len(preset.get('slots', [])) == 6,
                'has_name': 'name' in preset,
                'all_params': True,
                'param_ranges': True,
                'active_engines': 0
            }

            # Check each slot
            for i, slot in enumerate(preset.get('slots', [])):
                if slot.get('engine_id', 0) != 0:
                    validations['active_engines'] += 1

                # Check parameters
                params = slot.get('parameters', [])
                if len(params) != 15:
                    validations['all_params'] = False

                for param in params:
                    value = param.get('value', 0.5)
                    if value < 0.0 or value > 1.0:
                        validations['param_ranges'] = False

            # All validations must pass
            all_valid = all([
                validations['has_slots'],
                validations['slot_count'],
                validations['has_name'],
                validations['all_params'],
                validations['param_ranges'],
                validations['active_engines'] >= 1
            ])

            details = f"Preset: {preset.get('name', 'Unknown')}, "
            details += f"{validations['active_engines']} engines, "
            details += f"Structure valid: {all_valid}"

            return self.log_test(test_name, all_valid, details, validations), data

        except Exception as e:
            self.log_test(test_name, False, str(e))
            return False, {}

    def test_parameter_clamping(self) -> bool:
        """Test that extreme values are clamped correctly"""
        passed, data = self.test_preset_generation(
            "extreme 200% feedback 1000ms delay",
            "Parameter Clamping"
        )

        if not passed:
            return False

        # Check all values are in range
        preset = data.get('preset', {})
        violations = []

        for slot in preset.get('slots', []):
            for param in slot.get('parameters', []):
                value = param.get('value', 0.5)
                if value < 0.0 or value > 1.0:
                    violations.append(f"Slot {slot['slot']} {param['name']}: {value}")

        if violations:
            return self.log_test(
                "Parameter Value Ranges",
                False,
                f"{len(violations)} violations: {violations[:3]}"
            )
        else:
            return self.log_test(
                "Parameter Value Ranges",
                True,
                "All 90 parameters within [0.0, 1.0]"
            )

    def test_concurrent_requests(self) -> bool:
        """Test concurrent request handling"""
        import concurrent.futures

        prompts = [
            "warm reverb",
            "bright chorus",
            "deep delay"
        ]

        start = time.time()
        results = []

        with concurrent.futures.ThreadPoolExecutor(max_workers=3) as executor:
            futures = [
                executor.submit(
                    requests.post,
                    f"{self.base_url}/generate",
                    json={'prompt': p},
                    timeout=60
                )
                for p in prompts
            ]

            for future in concurrent.futures.as_completed(futures):
                try:
                    response = future.result()
                    results.append(response.status_code == 200)
                except Exception as e:
                    results.append(False)

        duration = time.time() - start
        success_rate = sum(results) / len(results)

        # Pass if at least 66% succeed (account for rate limits)
        passed = success_rate >= 0.66
        details = f"{sum(results)}/{len(results)} succeeded in {duration:.1f}s"

        return self.log_test("Concurrent Requests", passed, details)

    def test_performance_benchmarks(self) -> bool:
        """Test performance is acceptable"""
        test_cases = [
            ("Simple prompt", "warm delay"),
            ("Complex prompt", "vintage tape delay shimmer reverb chorus")
        ]

        timings = []
        all_passed = True

        for name, prompt in test_cases:
            start = time.time()
            try:
                response = requests.post(
                    f"{self.base_url}/generate",
                    json={'prompt': prompt},
                    timeout=60
                )
                duration = time.time() - start

                if response.status_code == 200:
                    timings.append(duration)
                    # Individual request should be under 60s
                    if duration > 60:
                        all_passed = False
                else:
                    all_passed = False
            except Exception:
                all_passed = False

        if timings:
            avg_time = sum(timings) / len(timings)
            details = f"Avg: {avg_time:.1f}s, Max: {max(timings):.1f}s"
            # Pass if average under 45s
            passed = all_passed and avg_time < 45
        else:
            passed = False
            details = "No successful requests"

        return self.log_test("Performance Benchmarks", passed, details)

    def test_engine_knowledge(self) -> bool:
        """Test that engine knowledge base is complete"""
        try:
            with open('/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server/trinity_engine_knowledge_COMPLETE.json', 'r') as f:
                knowledge = json.load(f)

            engines = knowledge.get('engines', {})
            engine_count = len(engines)

            # Check essential engines
            essential = [1, 7, 15, 34, 39, 40, 42]
            all_present = all(str(eid) in engines for eid in essential)

            passed = engine_count == 57 and all_present
            details = f"{engine_count} engines, Essential: {'All present' if all_present else 'Missing'}"

            return self.log_test("Engine Knowledge Base", passed, details)
        except Exception as e:
            return self.log_test("Engine Knowledge Base", False, str(e))

    def test_debug_output(self) -> bool:
        """Test that debug output is comprehensive"""
        passed, data = self.test_preset_generation(
            "test debug output",
            "Debug Output Validation"
        )

        if not passed:
            return False

        debug = data.get('debug', {})
        required_fields = ['prompt', 'timestamp', 'visionary', 'calculator', 'alchemist', 'processing_time_seconds']

        has_all_fields = all(field in debug for field in required_fields)

        if has_all_fields:
            return self.log_test(
                "Debug Information",
                True,
                f"All required fields present: {', '.join(required_fields)}"
            )
        else:
            missing = [f for f in required_fields if f not in debug]
            return self.log_test(
                "Debug Information",
                False,
                f"Missing fields: {missing}"
            )

    def run_all_tests(self) -> bool:
        """Run all validation tests"""
        print("=" * 70)
        print("TRINITY VALIDATION SUITE")
        print(f"Started: {self.start_time.isoformat()}")
        print(f"Target: {self.base_url}")
        print("=" * 70)
        print()

        # Run all tests
        tests = [
            self.test_health_endpoint,
            lambda: self.test_preset_generation("warm vintage delay", "Basic Preset Generation"),
            self.test_parameter_clamping,
            self.test_concurrent_requests,
            self.test_performance_benchmarks,
            self.test_engine_knowledge,
            self.test_debug_output,
        ]

        for test in tests:
            test()
            print()

        # Summary
        print("=" * 70)
        print("VALIDATION SUMMARY")
        print("=" * 70)

        total = len(self.results)
        passed = sum(1 for r in self.results if r['passed'])
        failed = total - passed

        print(f"\nTotal Tests: {total}")
        print(f"Passed:      {passed} ✅")
        print(f"Failed:      {failed} ❌")
        print(f"Success Rate: {passed/total*100:.1f}%")

        if failed > 0:
            print("\nFailed Tests:")
            for r in self.results:
                if not r['passed']:
                    print(f"  ❌ {r['test']}: {r['details']}")

        duration = (datetime.now() - self.start_time).total_seconds()
        print(f"\nTotal Duration: {duration:.1f}s")

        # Final verdict
        print("\n" + "=" * 70)
        if failed == 0:
            print("✅ ALL TESTS PASSED - SYSTEM READY FOR PRODUCTION")
            print("=" * 70)
            return True
        else:
            print("❌ TESTS FAILED - DO NOT DEPLOY TO PRODUCTION")
            print("=" * 70)
            return False

    def save_report(self, filename: str = "trinity_validation_report.json"):
        """Save detailed test report"""
        report = {
            'timestamp': self.start_time.isoformat(),
            'base_url': self.base_url,
            'total_tests': len(self.results),
            'passed': sum(1 for r in self.results if r['passed']),
            'failed': sum(1 for r in self.results if not r['passed']),
            'results': self.results
        }

        with open(filename, 'w') as f:
            json.dump(report, f, indent=2)

        print(f"\nDetailed report saved to: {filename}")


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(description='Trinity Validation Suite')
    parser.add_argument('--url', default='http://localhost:8000', help='Base URL of Trinity server')
    parser.add_argument('--report', default='trinity_validation_report.json', help='Output report file')

    args = parser.parse_args()

    validator = TrinityValidator(base_url=args.url)
    all_passed = validator.run_all_tests()
    validator.save_report(args.report)

    # Exit with appropriate code
    sys.exit(0 if all_passed else 1)


if __name__ == "__main__":
    main()
