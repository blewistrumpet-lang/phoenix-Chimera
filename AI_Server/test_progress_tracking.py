#!/usr/bin/env python3
"""
Test script for Phase 1 Progress Tracking

This script simulates the plugin's behavior by:
1. Sending a generation request
2. Polling the progress endpoint
3. Displaying real-time progress updates
"""

import requests
import time
import sys
import json
from pathlib import Path

# Server configuration
SERVER_URL = "http://localhost:8000"
PROGRESS_DIR = Path("/tmp/trinity_progress")


def test_progress_via_http():
    """Test progress tracking via HTTP endpoint"""
    print("\n" + "="*60)
    print("PHASE 1 PROGRESS TRACKING TEST - HTTP Endpoint")
    print("="*60)

    # Start generation request
    prompt = "warm vintage sound with subtle chorus and reverb"
    print(f"\n📝 Sending prompt: '{prompt}'")

    # Make async request (non-blocking)
    import threading

    result = {"preset": None, "error": None}

    def make_request():
        try:
            response = requests.post(
                f"{SERVER_URL}/generate",
                json={
                    "prompt": prompt,
                    "intensity": 0.5,
                    "complexity": 3
                },
                timeout=60
            )
            result["preset"] = response.json()
        except Exception as e:
            result["error"] = str(e)

    # Start request in background thread
    thread = threading.Thread(target=make_request)
    thread.start()

    # Give it a moment to start
    time.sleep(0.5)

    # Poll for progress - first need to find the request_id
    # Check the progress directory for new files
    print("\n🔍 Looking for active request...")

    request_id = None
    for _ in range(10):  # Try for 5 seconds
        time.sleep(0.5)
        progress_files = list(PROGRESS_DIR.glob("*.json"))
        if progress_files:
            request_id = progress_files[0].stem
            print(f"✅ Found request: {request_id}")
            break

    if not request_id:
        print("❌ No active request found in progress directory")
        thread.join()
        return

    # Poll progress endpoint
    print("\n📊 Polling progress updates...\n")
    last_stage = None
    last_progress = -1

    while thread.is_alive():
        try:
            # Read from HTTP endpoint
            response = requests.get(f"{SERVER_URL}/progress/{request_id}")
            if response.status_code == 200:
                data = response.json()

                if data.get("success"):
                    stage = data.get("stage", "unknown")
                    overall = data.get("overall_progress", 0)
                    message = data.get("message", "")
                    preset_name = data.get("preset_name", "")

                    # Only print if progress changed
                    if stage != last_stage or overall != last_progress:
                        progress_bar = "█" * int(overall * 40)
                        print(f"[{progress_bar:<40}] {overall*100:5.1f}% | {stage:12} | {message}")

                        if preset_name and last_stage != stage:
                            print(f"   🎼 Preset: {preset_name}")

                        last_stage = stage
                        last_progress = overall
                else:
                    # Progress file doesn't exist - likely completed
                    break

        except Exception as e:
            print(f"⚠️ Error polling progress: {e}")
            break

        time.sleep(0.2)  # Poll every 200ms

    # Wait for request to complete
    thread.join()

    # Display final result
    print("\n" + "="*60)
    if result["preset"]:
        preset_data = result["preset"]
        if preset_data.get("success"):
            final_preset = preset_data.get("data", {}).get("preset", {})
            print(f"✅ GENERATION COMPLETE")
            print(f"🎼 Preset: {final_preset.get('name', 'Unknown')}")
            print(f"🎛️  Engines: {len(final_preset.get('slots', []))}")
            print(f"⏱️  Time: {preset_data.get('metadata', {}).get('generation_time', 'N/A')}s")
        else:
            print(f"❌ Generation failed: {preset_data.get('message')}")
    elif result["error"]:
        print(f"❌ Request error: {result['error']}")
    print("="*60 + "\n")


def test_progress_via_file():
    """Test progress tracking via direct file reading"""
    print("\n" + "="*60)
    print("PHASE 1 PROGRESS TRACKING TEST - File System")
    print("="*60)

    # Start generation request
    prompt = "dark atmospheric pad with deep reverb"
    print(f"\n📝 Sending prompt: '{prompt}'")

    # Make async request
    import threading

    result = {"preset": None, "error": None}

    def make_request():
        try:
            response = requests.post(
                f"{SERVER_URL}/generate",
                json={
                    "prompt": prompt,
                    "intensity": 0.7,
                    "complexity": 4
                },
                timeout=60
            )
            result["preset"] = response.json()
        except Exception as e:
            result["error"] = str(e)

    thread = threading.Thread(target=make_request)
    thread.start()

    time.sleep(0.5)

    # Find request ID from directory
    request_id = None
    for _ in range(10):
        time.sleep(0.5)
        progress_files = list(PROGRESS_DIR.glob("*.json"))
        if progress_files:
            request_id = progress_files[0].stem
            print(f"✅ Found request: {request_id}")
            break

    if not request_id:
        print("❌ No active request found")
        thread.join()
        return

    # Read progress file directly
    print("\n📊 Reading progress file directly...\n")
    progress_file = PROGRESS_DIR / f"{request_id}.json"
    last_stage = None

    while thread.is_alive() or progress_file.exists():
        try:
            if progress_file.exists():
                data = json.loads(progress_file.read_text())

                stage = data.get("stage", "unknown")
                overall = data.get("overall_progress", 0)
                message = data.get("message", "")
                preset_name = data.get("preset_name", "")

                if stage != last_stage:
                    progress_bar = "█" * int(overall * 40)
                    print(f"[{progress_bar:<40}] {overall*100:5.1f}% | {stage:12} | {message}")

                    if preset_name:
                        print(f"   🎼 Preset: {preset_name}")

                    last_stage = stage
            else:
                # File deleted - generation complete
                break

        except Exception as e:
            # File might be mid-write, just continue
            pass

        time.sleep(0.3)

    thread.join()

    print("\n" + "="*60)
    if result["preset"]:
        preset_data = result["preset"]
        if preset_data.get("success"):
            final_preset = preset_data.get("data", {}).get("preset", {})
            print(f"✅ GENERATION COMPLETE")
            print(f"🎼 Preset: {final_preset.get('name', 'Unknown')}")
        else:
            print(f"❌ Generation failed")
    print("="*60 + "\n")


def main():
    # Check server is running
    try:
        response = requests.get(f"{SERVER_URL}/ping")
        if response.status_code != 200:
            print(f"❌ Server not responding at {SERVER_URL}")
            return
        print(f"✅ Server is running at {SERVER_URL}")
    except Exception as e:
        print(f"❌ Cannot connect to server: {e}")
        return

    # Check progress directory exists
    if not PROGRESS_DIR.exists():
        print(f"⚠️ Progress directory doesn't exist: {PROGRESS_DIR}")
        print("   Server needs to be started to initialize it")
        return

    print(f"✅ Progress directory exists: {PROGRESS_DIR}")

    # Run tests
    choice = input("\nTest method? (1=HTTP endpoint, 2=File system, 3=Both): ")

    if choice == "1":
        test_progress_via_http()
    elif choice == "2":
        test_progress_via_file()
    elif choice == "3":
        test_progress_via_http()
        time.sleep(2)
        test_progress_via_file()
    else:
        print("Invalid choice")


if __name__ == "__main__":
    main()
