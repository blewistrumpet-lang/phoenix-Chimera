#!/usr/bin/env python3
"""Force a status update from the training system"""

import json
from pathlib import Path
import requests

# Try to get current status from server
try:
    response = requests.get("http://localhost:8000/health")
    if response.status_code == 200:
        print("✅ Server is running and healthy")
        
    # Test a sample generation to see current performance
    test_prompt = "deep dubstep bass wobble"
    response = requests.post(
        "http://localhost:8000/generate",
        json={"prompt": test_prompt},
        timeout=5
    )
    
    if response.status_code == 200:
        data = response.json()
        if data["success"]:
            preset = data["preset"]
            params = preset.get("parameters", {})
            
            # Count active engines
            active = 0
            for slot in range(1, 7):
                if params.get(f"slot{slot}_engine", 0) > 0:
                    if params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                        active += 1
            
            print(f"\n📊 Current Generation Test:")
            print(f"   Prompt: '{test_prompt}'")
            print(f"   Preset: {preset.get('name', 'Unknown')}")
            print(f"   Active Engines: {active}/6")
            print(f"   Metadata: {data.get('metadata', {}).get('nudges_applied', 0)} nudges applied")
            
            # Show which engines were selected
            print(f"\n🎛️  Engines Selected:")
            from engine_mapping_authoritative import get_engine_name
            for slot in range(1, 7):
                engine_id = params.get(f"slot{slot}_engine", 0)
                if engine_id > 0 and params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                    print(f"   Slot {slot}: {get_engine_name(engine_id)}")
                    
except Exception as e:
    print(f"Error: {e}")

# Check training files
print("\n📁 Training Files:")
for f in ["checkpoints/electronic_gen_0.json", "checkpoints/electronic_gen_10.json", 
          "best_electronic_config.json"]:
    p = Path(f)
    if p.exists():
        print(f"   ✓ {f}")
    else:
        print(f"   ⏳ {f} (not yet)")