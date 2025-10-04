#!/usr/bin/env python3
"""
Bug Detection Test Suite - Find edge cases and potential failures
"""

import requests
import json
import time
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed
from engine_mapping_authoritative import ENGINE_NAMES

def test_edge_cases():
    """Test various edge cases that might break the system"""
    
    print("🐛 BUG DETECTION TEST SUITE")
    print("=" * 80)
    
    base_url = "http://localhost:8000"
    bugs_found = []
    
    # Test 1: Empty prompt
    print("\n1. Testing EMPTY prompt...")
    try:
        response = requests.post(f"{base_url}/generate", json={"prompt": ""}, timeout=10)
        if response.status_code != 200:
            bugs_found.append("Empty prompt causes error")
            print(f"   ❌ Failed: {response.status_code}")
        else:
            print(f"   ✅ Handled gracefully")
    except Exception as e:
        bugs_found.append(f"Empty prompt exception: {str(e)}")
        print(f"   ❌ Exception: {str(e)}")
    
    # Test 2: Very long prompt
    print("\n2. Testing VERY LONG prompt (10000 chars)...")
    long_prompt = "Create a preset that " + "has amazing sound " * 500
    try:
        response = requests.post(f"{base_url}/generate", json={"prompt": long_prompt}, timeout=10)
        if response.status_code != 200:
            bugs_found.append("Long prompt causes error")
            print(f"   ❌ Failed: {response.status_code}")
        else:
            print(f"   ✅ Handled gracefully")
    except Exception as e:
        bugs_found.append(f"Long prompt exception: {str(e)}")
        print(f"   ❌ Exception: {str(e)}")
    
    # Test 3: Special characters
    print("\n3. Testing SPECIAL CHARACTERS...")
    special_prompts = [
        "Create preset with 日本語 characters",
        "Preset with emoji 🎸🎹🎤🎧",
        "SQL injection'; DROP TABLE presets; --",
        "Path traversal ../../etc/passwd",
        "HTML <script>alert('xss')</script>",
        "Null byte injection\x00test",
        "Unicode snowman ☃️ preset"
    ]
    
    for prompt in special_prompts:
        print(f"   Testing: {prompt[:30]}...")
        try:
            response = requests.post(f"{base_url}/generate", json={"prompt": prompt}, timeout=10)
            if response.status_code != 200:
                bugs_found.append(f"Special char prompt failed: {prompt[:30]}")
                print(f"      ❌ Failed")
            else:
                print(f"      ✅ OK")
        except Exception as e:
            bugs_found.append(f"Special char exception: {prompt[:30]}")
            print(f"      ❌ Exception: {str(e)[:50]}")
    
    # Test 4: Numeric edge cases
    print("\n4. Testing NUMERIC EDGE CASES...")
    numeric_prompts = [
        "Engine 999999",
        "Engine -1",
        "Engine 0",
        "Engine 3.14159",
        "Engine NaN",
        "Engine Infinity"
    ]
    
    for prompt in numeric_prompts:
        print(f"   Testing: {prompt}...")
        try:
            response = requests.post(f"{base_url}/generate", json={"prompt": prompt}, timeout=10)
            if response.status_code != 200:
                bugs_found.append(f"Numeric prompt failed: {prompt}")
                print(f"      ❌ Failed")
            else:
                data = response.json()
                preset = data.get("preset", {})
                # Check for invalid engine IDs
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id < 0 or engine_id > 100:
                        bugs_found.append(f"Invalid engine ID {engine_id} from prompt: {prompt}")
                        print(f"      ❌ Invalid engine ID: {engine_id}")
                        break
                else:
                    print(f"      ✅ OK")
        except Exception as e:
            bugs_found.append(f"Numeric exception: {prompt}")
            print(f"      ❌ Exception: {str(e)[:50]}")
    
    # Test 5: Concurrent requests
    print("\n5. Testing CONCURRENT REQUESTS...")
    def make_request(i):
        try:
            prompt = f"Test concurrent request {i}"
            response = requests.post(f"{base_url}/generate", json={"prompt": prompt}, timeout=10)
            return response.status_code == 200
        except:
            return False
    
    with ThreadPoolExecutor(max_workers=10) as executor:
        futures = [executor.submit(make_request, i) for i in range(20)]
        results = [f.result() for f in as_completed(futures)]
        success_rate = sum(results) / len(results) * 100
        
        if success_rate < 100:
            bugs_found.append(f"Concurrent requests failed: {100-success_rate:.0f}% failure rate")
            print(f"   ❌ {success_rate:.0f}% success rate")
        else:
            print(f"   ✅ All concurrent requests successful")
    
    # Test 6: Missing required fields
    print("\n6. Testing MISSING FIELDS...")
    try:
        # Send request without prompt field
        response = requests.post(f"{base_url}/generate", json={}, timeout=10)
        if response.status_code == 422:  # Expected validation error
            print(f"   ✅ Properly validates missing prompt")
        else:
            bugs_found.append("Missing prompt field not validated")
            print(f"   ❌ Should have failed validation")
    except Exception as e:
        bugs_found.append(f"Missing field exception: {str(e)}")
        print(f"   ❌ Exception: {str(e)[:50]}")
    
    # Test 7: Wrong data types
    print("\n7. Testing WRONG DATA TYPES...")
    wrong_types = [
        {"prompt": 12345},  # Number instead of string
        {"prompt": ["array", "of", "strings"]},  # Array
        {"prompt": {"nested": "object"}},  # Object
        {"prompt": None},  # Null
        {"prompt": True},  # Boolean
    ]
    
    for payload in wrong_types:
        print(f"   Testing: {str(payload)[:40]}...")
        try:
            response = requests.post(f"{base_url}/generate", json=payload, timeout=10)
            if response.status_code == 422:
                print(f"      ✅ Properly rejected")
            else:
                bugs_found.append(f"Wrong type accepted: {payload}")
                print(f"      ❌ Should have been rejected")
        except Exception as e:
            bugs_found.append(f"Wrong type exception: {payload}")
            print(f"      ❌ Exception: {str(e)[:50]}")
    
    # Test 8: Parameter validation
    print("\n8. Testing PARAMETER VALIDATION...")
    response = requests.post(f"{base_url}/generate", 
                            json={"prompt": "Test parameter ranges"}, 
                            timeout=10)
    if response.status_code == 200:
        data = response.json()
        preset = data.get("preset", {})
        param_issues = []
        
        for slot in range(1, 7):
            for param in range(10):
                key = f"slot{slot}_param{param}"
                if key in preset:
                    value = preset[key]
                    try:
                        float_val = float(value)
                        if float_val < 0 or float_val > 1:
                            param_issues.append(f"{key}={value}")
                    except:
                        param_issues.append(f"{key}={value} (not numeric)")
        
        if param_issues:
            bugs_found.extend([f"Param out of range: {p}" for p in param_issues])
            print(f"   ❌ Invalid parameters: {param_issues[:3]}")
        else:
            print(f"   ✅ All parameters in valid range")
    
    # Test 9: Response structure validation
    print("\n9. Testing RESPONSE STRUCTURE...")
    response = requests.post(f"{base_url}/generate", 
                            json={"prompt": "Test response structure"}, 
                            timeout=10)
    if response.status_code == 200:
        data = response.json()
        required_fields = ["success", "preset", "message", "metadata"]
        missing_fields = [f for f in required_fields if f not in data]
        
        if missing_fields:
            bugs_found.append(f"Missing response fields: {missing_fields}")
            print(f"   ❌ Missing fields: {missing_fields}")
        else:
            # Check preset structure
            preset = data["preset"]
            preset_required = ["name", "master_input", "master_output", "master_mix"]
            missing_preset = [f for f in preset_required if f not in preset]
            
            if missing_preset:
                bugs_found.append(f"Missing preset fields: {missing_preset}")
                print(f"   ❌ Missing preset fields: {missing_preset}")
            else:
                print(f"   ✅ Response structure valid")
    
    # Test 10: Engine conflict detection
    print("\n10. Testing ENGINE CONFLICTS...")
    conflict_prompts = [
        "Use all 5 reverbs at once",
        "Add 10 distortion engines",
        "I want delay in every slot",
        "Give me compression in slot 1, 2, 3, 4, 5, and 6"
    ]
    
    for prompt in conflict_prompts:
        print(f"   Testing: {prompt}...")
        try:
            response = requests.post(f"{base_url}/generate", json={"prompt": prompt}, timeout=10)
            if response.status_code == 200:
                data = response.json()
                preset = data.get("preset", {})
                
                # Count engine types
                engine_counts = {}
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id > 0:
                        engine_name = ENGINE_NAMES.get(engine_id, "Unknown")
                        category = engine_name.split()[0] if engine_name else "Unknown"
                        engine_counts[category] = engine_counts.get(category, 0) + 1
                
                # Check for too many of same type
                overcrowded = [k for k, v in engine_counts.items() if v > 3]
                if overcrowded:
                    bugs_found.append(f"Engine overcrowding: {overcrowded}")
                    print(f"      ⚠️ Possible overcrowding: {overcrowded}")
                else:
                    print(f"      ✅ Handled appropriately")
        except Exception as e:
            bugs_found.append(f"Conflict test exception: {prompt}")
            print(f"      ❌ Exception: {str(e)[:50]}")
    
    # Test 11: Memory/performance test
    print("\n11. Testing PERFORMANCE...")
    start_times = []
    for i in range(10):
        start = time.time()
        response = requests.post(f"{base_url}/generate", 
                                json={"prompt": f"Performance test {i}"}, 
                                timeout=10)
        elapsed = time.time() - start
        start_times.append(elapsed)
    
    avg_time = sum(start_times) / len(start_times)
    max_time = max(start_times)
    
    if avg_time > 2.0:
        bugs_found.append(f"Slow average response: {avg_time:.2f}s")
        print(f"   ⚠️ Slow average: {avg_time:.2f}s")
    else:
        print(f"   ✅ Average response: {avg_time:.2f}s")
    
    if max_time > 5.0:
        bugs_found.append(f"Very slow max response: {max_time:.2f}s")
        print(f"   ⚠️ Max response: {max_time:.2f}s")
    
    # Test 12: Check for server crashes
    print("\n12. Testing SERVER STABILITY...")
    try:
        response = requests.get(f"{base_url}/health", timeout=5)
        if response.status_code == 200:
            print(f"   ✅ Server still healthy after all tests")
        else:
            bugs_found.append("Server unhealthy after tests")
            print(f"   ❌ Server unhealthy")
    except:
        bugs_found.append("Server not responding after tests")
        print(f"   ❌ Server not responding!")
    
    # FINAL REPORT
    print("\n" + "=" * 80)
    print("🐛 BUG REPORT")
    print("=" * 80)
    
    if not bugs_found:
        print("✅ NO BUGS FOUND! System is robust!")
    else:
        print(f"❌ Found {len(bugs_found)} potential issues:\n")
        for i, bug in enumerate(bugs_found, 1):
            print(f"{i:2}. {bug}")
        
        # Categorize bugs
        critical = [b for b in bugs_found if "exception" in b.lower() or "crash" in b.lower()]
        major = [b for b in bugs_found if "failed" in b.lower() or "invalid" in b.lower()]
        minor = [b for b in bugs_found if b not in critical and b not in major]
        
        print(f"\nSeverity breakdown:")
        print(f"  🔴 Critical: {len(critical)}")
        print(f"  🟡 Major: {len(major)}")
        print(f"  🟢 Minor: {len(minor)}")
    
    return bugs_found

if __name__ == "__main__":
    bugs = test_edge_cases()