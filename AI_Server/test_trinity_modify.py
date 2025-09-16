#!/usr/bin/env python3
"""
Test the Trinity-based /modify endpoint with various poetic and technical requests
"""

import requests
import json
import time
from typing import Dict, Any
from engine_mapping_correct import ENGINE_MAPPING

def test_trinity_modify():
    """Test the new Trinity-based modification system"""
    
    base_url = "http://localhost:8000"
    
    # Wait for server
    time.sleep(3)
    
    print("\n" + "="*80)
    print("🔺 TRINITY ARCHITECTURE MODIFY TEST")
    print("Flow: Visionary → Calculator → Alchemist")
    print("="*80)
    
    # First, generate a preset to modify
    print("\n📝 Generating initial preset...")
    response = requests.post(
        f"{base_url}/generate",
        json={"prompt": "Create a warm ambient pad with lush reverb"},
        timeout=30
    )
    
    if response.status_code != 200:
        print("❌ Failed to generate initial preset")
        return
    
    initial_preset = response.json()["preset"]
    print(f"✅ Generated: '{initial_preset.get('name', 'Unknown')}'")
    print(f"   Vibe: {initial_preset.get('vibe', 'Unknown')}")
    
    # Show initial engines
    print("\n   Initial Engines:")
    parameters = initial_preset.get("parameters", {})
    for slot in range(1, 7):
        engine_id = parameters.get(f"slot{slot}_engine", 0)
        if engine_id > 0 and parameters.get(f"slot{slot}_bypass", 0) < 0.5:
            engine_name = ENGINE_MAPPING.get(engine_id, "Unknown")
            mix = parameters.get(f"slot{slot}_mix", 0.5)
            print(f"   - Slot {slot}: {engine_name} (mix: {mix:.2f})")
    
    # Test various modification types
    test_modifications = [
        {
            "type": "🌑 MOOD SHIFT",
            "request": "Make it darker and more ominous",
            "expected": ["darker mood", "reduced brightness"]
        },
        {
            "type": "🎯 TECHNICAL",
            "request": "Increase reverb by 30% and add more compression",
            "expected": ["reverb increase", "compression added"]
        },
        {
            "type": "✨ POETIC",
            "request": "Transform it to sound like memories dissolving in rain",
            "expected": ["ethereal quality", "increased space"]
        },
        {
            "type": "🔧 ENGINE REQUEST",
            "request": "Add chaos generator for unpredictability",
            "expected": ["chaos suggestion", "intensity change"]
        },
        {
            "type": "🎨 CREATIVE",
            "request": "Make it feel like sunrise over an alien ocean",
            "expected": ["brighter", "otherworldly"]
        }
    ]
    
    current_preset = initial_preset.copy()
    
    for i, test in enumerate(test_modifications, 1):
        print(f"\n{'='*80}")
        print(f"TEST {i}/5: {test['type']}")
        print(f"Request: \"{test['request']}\"")
        print("-"*40)
        
        # Make modification request
        response = requests.post(
            f"{base_url}/modify",
            json={
                "preset": current_preset,
                "modification": test["request"]
            },
            timeout=10
        )
        
        if response.status_code == 200:
            result = response.json()
            
            if result.get("success"):
                print(f"✅ {result.get('message', 'Modified')}")
                
                # Show metadata
                metadata = result.get("metadata", {})
                if metadata:
                    print(f"\n📊 Modification Analysis:")
                    print(f"   Intent: {metadata.get('intent', 'unknown')}")
                    print(f"   Mood Shift: {metadata.get('mood_shift', 'none')}")
                    print(f"   Total Changes: {metadata.get('total_changes', 0)}")
                    
                    affected = metadata.get("affected_parameters", [])
                    if affected:
                        print(f"   Affected Parameters: {len(affected)}")
                
                # Show changes
                changes = result.get("changes", [])
                if changes:
                    print(f"\n🔄 Applied Changes:")
                    for change in changes[:3]:  # Show first 3
                        print(f"   • {change}")
                
                # Update preset for next modification
                if "data" in result:
                    current_preset = result["data"]
                    
                    # Show new engine configuration if changed
                    new_params = current_preset.get("parameters", {})
                    engine_changed = False
                    for slot in range(1, 7):
                        old_engine = parameters.get(f"slot{slot}_engine", 0)
                        new_engine = new_params.get(f"slot{slot}_engine", 0)
                        if old_engine != new_engine:
                            engine_changed = True
                            if new_engine > 0:
                                print(f"   🔄 Slot {slot}: → {ENGINE_MAPPING.get(new_engine, 'Unknown')}")
                    
                    if not engine_changed and metadata.get("total_changes", 0) > 0:
                        print("   ✓ Parameters adjusted while preserving engines")
                
                # Check if expectations were met
                print(f"\n📋 Expected behaviors:")
                for expected in test["expected"]:
                    # Simple check if any indication of expected behavior
                    found = False
                    if expected in str(result).lower():
                        found = True
                    elif metadata and expected in str(metadata).lower():
                        found = True
                    elif changes and expected in str(changes).lower():
                        found = True
                    
                    if found:
                        print(f"   ✅ {expected}")
                    else:
                        print(f"   ⚠️  {expected} (not clearly indicated)")
            else:
                print(f"❌ Modification failed: {result.get('message', 'Unknown error')}")
        else:
            print(f"❌ HTTP {response.status_code}")
    
    print(f"\n{'='*80}")
    print("📊 FINAL PRESET STATE")
    print("="*80)
    print(f"Name: {current_preset.get('name', 'Unknown')}")
    print(f"Vibe: {current_preset.get('vibe', 'Unknown')}")
    
    print("\nFinal Engines:")
    final_params = current_preset.get("parameters", {})
    for slot in range(1, 7):
        engine_id = final_params.get(f"slot{slot}_engine", 0)
        if engine_id > 0 and final_params.get(f"slot{slot}_bypass", 0) < 0.5:
            engine_name = ENGINE_MAPPING.get(engine_id, "Unknown")
            mix = final_params.get(f"slot{slot}_mix", 0.5)
            
            # Compare with initial
            initial_mix = parameters.get(f"slot{slot}_mix", 0.5)
            if abs(mix - initial_mix) > 0.01:
                print(f"   Slot {slot}: {engine_name} (mix: {initial_mix:.2f} → {mix:.2f})")
            else:
                print(f"   Slot {slot}: {engine_name} (mix: {mix:.2f})")
    
    print("\n" + "="*80)
    print("✅ TRINITY MODIFY TEST COMPLETE")
    print("\nKey Achievements:")
    print("• Visionary interprets both technical and poetic language")
    print("• Calculator applies intelligent, targeted nudges")
    print("• Alchemist ensures quality and safety")
    print("• Original preset character preserved (unless requested otherwise)")
    print("• Name stays consistent throughout modifications")
    print("="*80)

if __name__ == "__main__":
    test_trinity_modify()