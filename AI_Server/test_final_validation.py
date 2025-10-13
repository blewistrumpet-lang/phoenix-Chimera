#!/usr/bin/env python3
"""
Final validation test - Quick but comprehensive check
"""

import requests
import json
from engine_mapping_authoritative import ENGINE_NAMES

def final_validation():
    print("✅ FINAL SYSTEM VALIDATION")
    print("=" * 80)
    
    test_results = {
        "naming": [],
        "engines": [],
        "errors": [],
        "performance": []
    }
    
    # Quick diverse test set
    test_prompts = [
        "Warm vintage guitar tone",
        "Modern trap 808 bass",
        "Ethereal ambient pad",
        "Aggressive metal distortion",
        "Clean jazz piano"
    ]
    
    for prompt in test_prompts:
        try:
            response = requests.post(
                "http://localhost:8000/generate",
                json={"prompt": prompt},
                timeout=10
            )
            
            if response.status_code == 200:
                data = response.json()
                preset = data.get("preset", {})
                
                # Check naming
                name = preset.get("name", "")
                if "Sonic" in name or "Safe Default" in name:
                    test_results["naming"].append(f"Bad name: {name}")
                
                # Check engine count
                engines = sum(1 for s in range(1,7) if preset.get(f"slot{s}_engine", 0) > 0)
                if engines < 3 or engines > 6:
                    test_results["engines"].append(f"Engine count: {engines}")
                
            else:
                test_results["errors"].append(f"Status {response.status_code}")
                
        except Exception as e:
            test_results["errors"].append(str(e))
    
    # Report
    print("\n📊 RESULTS:")
    print(f"✅ Naming issues: {len(test_results['naming'])}")
    print(f"✅ Engine issues: {len(test_results['engines'])}")  
    print(f"✅ Errors: {len(test_results['errors'])}")
    
    if any(test_results.values()):
        print("\n⚠️ Issues found:")
        for category, issues in test_results.items():
            for issue in issues:
                print(f"  - {issue}")
    else:
        print("\n🎉 ALL TESTS PASSED!")
    
    return not any(test_results.values())

if __name__ == "__main__":
    success = final_validation()
    exit(0 if success else 1)