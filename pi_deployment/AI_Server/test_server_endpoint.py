#!/usr/bin/env python3
"""
Test the actual server endpoint to verify the complete pipeline works via API
"""

import requests
import json
import time
import subprocess
import sys
from engine_mapping_correct import ENGINE_MAPPING

def start_server():
    """Start the server in background"""
    print("Starting server...")
    process = subprocess.Popen(
        [sys.executable, "main.py"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    time.sleep(3)  # Give server time to start
    return process

def test_server_endpoint():
    """Test the /generate endpoint with specific engine requests"""
    
    base_url = "http://localhost:8000"
    
    # First check if server is running
    try:
        response = requests.get(f"{base_url}/health", timeout=2)
        print("✅ Server is already running")
        server_process = None
    except requests.exceptions.ConnectionError:
        print("Starting server...")
        server_process = start_server()
        time.sleep(2)
    
    try:
        # Test cases with specific engine requests
        test_prompts = [
            {
                "prompt": "Create a terrifying preset with Chaos Generator and Spectral Freeze for horror film scoring",
                "expected_engines": ["Chaos Generator", "Spectral Freeze"]
            },
            {
                "prompt": "Design an ethereal ambient soundscape using Shimmer Reverb and Granular Cloud",
                "expected_engines": ["Shimmer Reverb", "Granular Cloud"]
            },
            {
                "prompt": "Build a crushing metal tone with BitCrusher and Gated Reverb",
                "expected_engines": ["BitCrusher", "Gated Reverb"]
            }
        ]
        
        print("\n" + "="*80)
        print("TESTING SERVER ENDPOINT /generate")
        print("="*80)
        
        for i, test in enumerate(test_prompts, 1):
            print(f"\n📝 Test {i}/3: {test['prompt'][:60]}...")
            print(f"🎯 Expecting: {', '.join(test['expected_engines'])}")
            print("-"*40)
            
            # Make API request
            payload = {
                "prompt": test["prompt"],
                "context": {},
                "max_generation_time": 30
            }
            
            try:
                response = requests.post(
                    f"{base_url}/generate",
                    json=payload,
                    timeout=35
                )
                
                if response.status_code == 200:
                    data = response.json()
                    
                    if data.get("success"):
                        preset = data.get("preset", {})
                        metadata = data.get("metadata", {})
                        
                        print(f"✅ Success: {data.get('message', 'Generated')}")
                        print(f"   Name: '{preset.get('name', 'Unknown')}'")
                        print(f"   Vibe: '{preset.get('vibe', 'Unknown')}'")
                        print(f"   Generation time: {metadata.get('generation_time_seconds', 0)}s")
                        print(f"   Nudges applied: {metadata.get('nudges_applied', 0)}")
                        
                        # Check active engines
                        print("\n   Active Engines:")
                        parameters = preset.get("parameters", {})
                        found_expected = []
                        
                        for slot in range(1, 7):
                            engine_id = parameters.get(f"slot{slot}_engine", 0)
                            if engine_id > 0:
                                bypassed = parameters.get(f"slot{slot}_bypass", 0) > 0.5
                                if not bypassed:
                                    engine_name = ENGINE_MAPPING.get(engine_id, "Unknown")
                                    mix_level = parameters.get(f"slot{slot}_mix", 0.5)
                                    
                                    is_expected = engine_name in test["expected_engines"]
                                    if is_expected:
                                        found_expected.append(engine_name)
                                        print(f"   ✅ Slot {slot}: {engine_name} (mix: {mix_level:.2f})")
                                    else:
                                        print(f"   → Slot {slot}: {engine_name} (mix: {mix_level:.2f})")
                        
                        # Verify results
                        if found_expected:
                            print(f"\n   ✅ Found requested engines: {', '.join(found_expected)}")
                        else:
                            print(f"\n   ⚠️  Requested engines not in active slots")
                        
                        # Check if they were in the blueprint
                        blueprint_vibe = metadata.get("blueprint_vibe", "")
                        if blueprint_vibe:
                            print(f"   Blueprint vibe: '{blueprint_vibe}'")
                    else:
                        print(f"❌ Generation failed: {data.get('message', 'Unknown error')}")
                else:
                    print(f"❌ HTTP {response.status_code}: {response.text[:100]}")
                    
            except requests.exceptions.Timeout:
                print("⚠️  Request timed out")
            except Exception as e:
                print(f"❌ Error: {str(e)}")
        
        # Final health check
        print("\n" + "="*80)
        print("SERVER HEALTH CHECK")
        print("-"*40)
        
        health_response = requests.get(f"{base_url}/health")
        if health_response.status_code == 200:
            health = health_response.json()
            print(f"Status: {health.get('status', 'Unknown')}")
            print(f"Service: {health.get('service', 'Unknown')}")
            print(f"Version: {health.get('version', 'Unknown')}")
            
            components = health.get("components", {})
            print("\nComponents:")
            for component, status in components.items():
                emoji = "✅" if status == "ready" else "⚠️"
                print(f"  {emoji} {component}: {status}")
        
        print("="*80)
        print("✅ SERVER ENDPOINT VERIFICATION COMPLETE")
        print("="*80)
        
    finally:
        # Clean up server process if we started it
        if server_process:
            print("\nStopping server...")
            server_process.terminate()
            server_process.wait()
            print("Server stopped")

if __name__ == "__main__":
    test_server_endpoint()