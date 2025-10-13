#!/usr/bin/env python3
"""
Comprehensive test suite for file-based preset exchange system
Tests the complete flow from prompt to engine loading
"""

import requests
import json
import time
import os
from pathlib import Path
import sys

# Colors for output
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
RESET = '\033[0m'

def test_file_exchange_system():
    """Test the complete file exchange pipeline"""
    print(f"{BLUE}{'='*60}{RESET}")
    print(f"{BLUE}File Exchange System Test Suite{RESET}")
    print(f"{BLUE}{'='*60}{RESET}\n")
    
    base_url = "http://localhost:8000"
    test_session = f"test_session_{int(time.time())}"
    
    # Test 1: Health Check
    print(f"{YELLOW}Test 1: Server Health Check{RESET}")
    try:
        response = requests.get(f"{base_url}/health")
        if response.status_code == 200:
            print(f"{GREEN}✓ Server is healthy{RESET}")
        else:
            print(f"{RED}✗ Server health check failed{RESET}")
            return False
    except Exception as e:
        print(f"{RED}✗ Cannot connect to server: {e}{RESET}")
        return False
    
    # Test 2: Exchange Stats
    print(f"\n{YELLOW}Test 2: File Exchange Stats{RESET}")
    try:
        response = requests.get(f"{base_url}/exchange_stats")
        stats = response.json()
        print(f"Exchange directory: {stats.get('exchange_dir')}")
        print(f"Stats: {json.dumps(stats.get('stats'), indent=2)}")
        print(f"{GREEN}✓ File exchange system initialized{RESET}")
    except Exception as e:
        print(f"{RED}✗ File exchange stats failed: {e}{RESET}")
    
    # Test 3: Send Preset Request
    print(f"\n{YELLOW}Test 3: Sending Preset Request{RESET}")
    test_prompts = [
        "warm vintage compression with plate reverb",
        "aggressive distortion for metal",
        "spacious ambient soundscape"
    ]
    
    results = []
    
    for prompt in test_prompts:
        print(f"\nPrompt: '{prompt}'")
        
        message_data = {
            "type": "preset_request",
            "content": prompt,
            "session_id": test_session
        }
        
        try:
            # Send the request
            response = requests.post(f"{base_url}/message", json=message_data)
            result = response.json()
            
            if result.get("success"):
                exchange_id = result.get("exchange_id")
                preset_name = result.get("preset_name")
                print(f"{GREEN}✓ Preset created: {preset_name}{RESET}")
                print(f"  Exchange ID: {exchange_id}")
                results.append({
                    "prompt": prompt,
                    "exchange_id": exchange_id,
                    "preset_name": preset_name,
                    "success": True
                })
            else:
                print(f"{RED}✗ Failed to create preset{RESET}")
                results.append({
                    "prompt": prompt,
                    "success": False
                })
        except Exception as e:
            print(f"{RED}✗ Request failed: {e}{RESET}")
            results.append({
                "prompt": prompt,
                "success": False,
                "error": str(e)
            })
        
        # Wait for processing
        time.sleep(12)
    
    # Test 4: Poll for Presets
    print(f"\n{YELLOW}Test 4: Polling for Presets{RESET}")
    
    for i in range(len(results)):
        print(f"\nChecking for preset {i+1}...")
        
        try:
            response = requests.get(f"{base_url}/poll?session={test_session}")
            poll_result = response.json()
            
            messages = poll_result.get("messages", [])
            if messages:
                print(f"{GREEN}✓ Found preset in exchange!{RESET}")
                for msg in messages:
                    if msg.get("type") == "preset":
                        preset_data = msg.get("data", {}).get("data", {}).get("preset", {})
                        print(f"  Name: {preset_data.get('name')}")
                        print(f"  Slots: {len(preset_data.get('slots', []))}")
                        
                        # Verify engines are specified
                        for slot_idx, slot in enumerate(preset_data.get('slots', [])):
                            engine_id = slot.get('engine_id')
                            engine_name = slot.get('engine_name')
                            print(f"    Slot {slot_idx}: {engine_name} (ID: {engine_id})")
            else:
                print(f"{YELLOW}⚠ No preset found in this poll{RESET}")
        except Exception as e:
            print(f"{RED}✗ Polling failed: {e}{RESET}")
        
        time.sleep(1)
    
    # Test 5: Verify File Structure
    print(f"\n{YELLOW}Test 5: Verify File Exchange Directory{RESET}")
    exchange_dir = Path.home() / ".chimera_phoenix" / "preset_exchange"
    
    if exchange_dir.exists():
        print(f"{GREEN}✓ Exchange directory exists: {exchange_dir}{RESET}")
        
        # Check subdirectories
        pending = list((exchange_dir / "pending").glob("*.json"))
        processed = list((exchange_dir / "processed").glob("*.json"))
        failed = list((exchange_dir / "failed").glob("*.json"))
        markers = list((exchange_dir / "pending").glob("*.marker"))
        
        print(f"  Pending presets: {len(pending)}")
        print(f"  Processed presets: {len(processed)}")
        print(f"  Failed presets: {len(failed)}")
        print(f"  Active markers: {len(markers)}")
        
        # Display pending preset files
        if pending:
            print(f"\n  Pending preset files:")
            for p in pending[:3]:  # Show max 3
                print(f"    - {p.name}")
    else:
        print(f"{RED}✗ Exchange directory not found{RESET}")
    
    # Test 6: Acknowledge Presets
    print(f"\n{YELLOW}Test 6: Acknowledge Processed Presets{RESET}")
    
    for result in results:
        if result.get("success") and result.get("exchange_id"):
            exchange_id = result["exchange_id"]
            print(f"Acknowledging: {exchange_id}")
            
            try:
                response = requests.post(f"{base_url}/acknowledge", 
                                        json={"exchange_id": exchange_id})
                ack_result = response.json()
                
                if ack_result.get("success"):
                    print(f"{GREEN}✓ Acknowledged successfully{RESET}")
                else:
                    print(f"{YELLOW}⚠ Already processed or not found{RESET}")
            except Exception as e:
                print(f"{RED}✗ Acknowledge failed: {e}{RESET}")
    
    # Final Stats
    print(f"\n{YELLOW}Test 7: Final Exchange Stats{RESET}")
    try:
        response = requests.get(f"{base_url}/exchange_stats")
        stats = response.json()
        print(f"Final stats: {json.dumps(stats.get('stats'), indent=2)}")
    except Exception as e:
        print(f"{RED}✗ Failed to get final stats: {e}{RESET}")
    
    print(f"\n{BLUE}{'='*60}{RESET}")
    print(f"{BLUE}Test Suite Complete{RESET}")
    print(f"{BLUE}{'='*60}{RESET}\n")
    
    # Summary
    successful = sum(1 for r in results if r.get("success"))
    total = len(results)
    
    if successful == total:
        print(f"{GREEN}✓ All {total} presets created successfully!{RESET}")
        return True
    else:
        print(f"{YELLOW}⚠ {successful}/{total} presets created successfully{RESET}")
        return False

if __name__ == "__main__":
    success = test_file_exchange_system()
    sys.exit(0 if success else 1)