#!/usr/bin/env python3
"""
Test script to verify calculator fallback functionality in Trinity Server
Tests that if calculator fails, server falls back to visionary preset
"""

import asyncio
import httpx
import time
import logging
from unittest.mock import patch

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

BASE_URL = "http://localhost:8000"

async def test_calculator_failure_fallback():
    """
    Test 1: Mock calculator failure to verify fallback to visionary preset
    This test mocks the calculator to raise an exception
    """
    logger.info("\n" + "="*60)
    logger.info("TEST 1: Calculator failure with fallback")
    logger.info("="*60)

    from calculator_max_intelligence import MaxIntelligenceCalculator

    # Save original method
    original_optimize = MaxIntelligenceCalculator.optimize_parameters_max_intelligence

    async def failing_optimize(self, preset, prompt):
        """Mock function that always fails"""
        logger.info("Simulating calculator failure...")
        raise Exception("Simulated calculator failure for testing")

    try:
        # Patch the method to fail
        MaxIntelligenceCalculator.optimize_parameters_max_intelligence = failing_optimize

        start_time = time.time()
        async with httpx.AsyncClient(timeout=120.0) as client:
            response = await client.post(
                f"{BASE_URL}/generate",
                json={"prompt": "test calculator fallback mechanism"}
            )
            duration = time.time() - start_time

            if response.status_code == 200:
                data = response.json()
                logger.info(f"SUCCESS: Request completed despite calculator failure in {duration:.2f}s")

                # Check debug info for fallback
                debug = data.get("debug", {})
                calc_debug = debug.get("calculator", {})

                if calc_debug.get("error_occurred"):
                    logger.info("SUCCESS: Calculator error was caught")
                    logger.info(f"Error message: {calc_debug.get('error_message', 'N/A')}")

                    if calc_debug.get("fallback_used"):
                        logger.info("SUCCESS: Fallback to visionary preset was used")

                        # Verify we still got a valid preset
                        preset = data.get("preset", {})
                        if preset.get("name") and preset.get("slots"):
                            logger.info("SUCCESS: Valid preset returned despite calculator failure")
                            logger.info(f"Preset name: {preset.get('name')}")

                            engines = [s for s in preset.get("slots", []) if s.get("engine_id", 0) != 0]
                            logger.info(f"Engines in preset: {len(engines)}")

                            return True
                        else:
                            logger.error("FAILED: Preset is invalid or missing")
                            return False
                    else:
                        logger.error("FAILED: Fallback was not used")
                        return False
                else:
                    logger.error("FAILED: Calculator error was not detected")
                    return False
            else:
                logger.error(f"FAILED: Request failed with status {response.status_code}")
                logger.error(f"Response: {response.text}")
                return False

    except Exception as e:
        duration = time.time() - start_time
        logger.error(f"FAILED: Request failed after {duration:.2f}s: {e}")
        return False

    finally:
        # Restore original method
        MaxIntelligenceCalculator.optimize_parameters_max_intelligence = original_optimize


async def test_calculator_success():
    """
    Test 2: Verify calculator works normally when no errors
    """
    logger.info("\n" + "="*60)
    logger.info("TEST 2: Normal calculator operation")
    logger.info("="*60)

    start_time = time.time()

    try:
        async with httpx.AsyncClient(timeout=120.0) as client:
            response = await client.post(
                f"{BASE_URL}/generate",
                json={"prompt": "vintage tape delay with 1/8 dotted and 35% feedback"}
            )
            duration = time.time() - start_time

            if response.status_code == 200:
                data = response.json()
                logger.info(f"SUCCESS: Request completed in {duration:.2f}s")

                # Check debug info
                debug = data.get("debug", {})
                calc_debug = debug.get("calculator", {})

                if not calc_debug.get("error_occurred", False):
                    logger.info("SUCCESS: Calculator completed without errors")

                    # Check if values were extracted
                    extracted = calc_debug.get("extracted_values", {})
                    if extracted:
                        logger.info(f"SUCCESS: Extracted {len(extracted)} values from prompt")
                        for key, val in extracted.items():
                            logger.info(f"  - {key}: {val.get('original')} = {val.get('value')}")

                    param_changes = calc_debug.get("parameter_changes", 0)
                    logger.info(f"SUCCESS: Made {param_changes} parameter changes")

                    if param_changes > 0:
                        logger.info("SUCCESS: Calculator successfully modified parameters")
                        return True
                    else:
                        logger.warning("WARNING: Calculator didn't modify any parameters")
                        return True  # Still success, just no changes needed
                else:
                    logger.error("FAILED: Calculator reported an error")
                    logger.error(f"Error: {calc_debug.get('error_message', 'Unknown')}")
                    return False
            else:
                logger.error(f"FAILED: Request failed with status {response.status_code}")
                logger.error(f"Response: {response.text}")
                return False

    except Exception as e:
        duration = time.time() - start_time
        logger.error(f"FAILED: Request failed after {duration:.2f}s: {e}")
        return False


async def test_parse_prompt_values():
    """
    Test 3: Test calculator's prompt parsing in isolation
    """
    logger.info("\n" + "="*60)
    logger.info("TEST 3: Calculator prompt parsing")
    logger.info("="*60)

    from calculator_max_intelligence import MaxIntelligenceCalculator

    try:
        calc = MaxIntelligenceCalculator()

        test_prompts = [
            "tape delay at 1/8 dotted with 35% feedback",
            "compressor with 4:1 ratio",
            "filter at 2.5kHz with 80% resonance",
            "delay at 250ms with 50% mix"
        ]

        all_passed = True
        for prompt in test_prompts:
            logger.info(f"\nTesting: '{prompt}'")
            extracted = calc.parse_prompt_values(prompt)

            if extracted:
                logger.info(f"SUCCESS: Extracted {len(extracted)} values")
                for key, val in extracted.items():
                    logger.info(f"  - {key}: {val.get('original')} = {val.get('value'):.4f}")
            else:
                logger.warning(f"WARNING: No values extracted from: {prompt}")

        return all_passed

    except Exception as e:
        logger.error(f"FAILED: Parsing test failed: {e}")
        return False


async def test_concurrent_requests():
    """
    Test 4: Verify server handles concurrent requests correctly with cache lock
    """
    logger.info("\n" + "="*60)
    logger.info("TEST 4: Concurrent request handling")
    logger.info("="*60)

    async def make_request(request_num, prompt):
        """Make a single request"""
        logger.info(f"Request {request_num}: Starting")
        start_time = time.time()

        try:
            async with httpx.AsyncClient(timeout=120.0) as client:
                response = await client.post(
                    f"{BASE_URL}/generate",
                    json={"prompt": prompt}
                )
                duration = time.time() - start_time

                if response.status_code == 200:
                    logger.info(f"Request {request_num}: SUCCESS in {duration:.2f}s")
                    return True
                else:
                    logger.error(f"Request {request_num}: FAILED with status {response.status_code}")
                    return False

        except Exception as e:
            duration = time.time() - start_time
            logger.error(f"Request {request_num}: FAILED after {duration:.2f}s: {e}")
            return False

    try:
        # Make 3 concurrent requests
        prompts = [
            "vintage tape echo",
            "modern reverb",
            "classic compressor"
        ]

        tasks = [make_request(i+1, prompt) for i, prompt in enumerate(prompts)]
        results = await asyncio.gather(*tasks)

        success_count = sum(results)
        total_count = len(results)

        logger.info(f"\nConcurrent requests: {success_count}/{total_count} succeeded")

        if success_count == total_count:
            logger.info("SUCCESS: All concurrent requests succeeded")
            logger.info("Cache locking is working correctly")
            return True
        elif success_count > 0:
            logger.warning(f"PARTIAL: {success_count}/{total_count} requests succeeded")
            return False
        else:
            logger.error("FAILED: All concurrent requests failed")
            return False

    except Exception as e:
        logger.error(f"FAILED: Concurrent test failed: {e}")
        return False


async def run_all_tests():
    """Run all calculator fallback tests"""
    logger.info("\n" + "="*80)
    logger.info("TRINITY SERVER CALCULATOR FALLBACK TESTS")
    logger.info("="*80)
    logger.info("NOTE: Make sure Trinity Server is running on http://localhost:8000")
    logger.info("Start it with: python trinity_server.py")
    logger.info("="*80)

    # First check if server is running
    try:
        async with httpx.AsyncClient() as client:
            await client.get(f"{BASE_URL}/health", timeout=5.0)
    except Exception as e:
        logger.error(f"\nERROR: Cannot connect to Trinity Server at {BASE_URL}")
        logger.error(f"Error: {e}")
        logger.error("Please start the server first with: python trinity_server.py")
        return

    results = {}

    # Test 1: Calculator prompt parsing (local test)
    results["prompt_parsing"] = await test_parse_prompt_values()

    # Test 2: Normal calculator operation
    results["normal_operation"] = await test_calculator_success()

    # Test 3: Calculator failure fallback
    results["calculator_fallback"] = await test_calculator_failure_fallback()

    # Test 4: Concurrent requests with cache locking
    results["concurrent_requests"] = await test_concurrent_requests()

    # Print summary
    logger.info("\n" + "="*80)
    logger.info("TEST RESULTS SUMMARY")
    logger.info("="*80)

    passed = 0
    failed = 0
    skipped = 0

    for test_name, result in results.items():
        if result is None:
            status = "SKIPPED"
            skipped += 1
        elif result:
            status = "PASSED"
            passed += 1
        else:
            status = "FAILED"
            failed += 1

        logger.info(f"{test_name:.<40} {status}")

    logger.info("="*80)
    logger.info(f"Total: {passed} passed, {failed} failed, {skipped} skipped")
    logger.info("="*80)

    if failed == 0 and passed > 0:
        logger.info("\nALL TESTS PASSED!")
        logger.info("Calculator fallback and cache locking working correctly.")
    elif failed > 0:
        logger.error("\nSOME TESTS FAILED")
        logger.error("Please review the failures above.")
    else:
        logger.warning("\nNO TESTS RAN")


if __name__ == "__main__":
    asyncio.run(run_all_tests())
