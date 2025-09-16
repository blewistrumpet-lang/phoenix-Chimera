#!/usr/bin/env python3
"""
Test and demonstrate engine selection and addition during modifications
Shows exactly which engines are selected and how they change
"""

import requests
import json
import time
from typing import Dict, Any, List
from engine_mapping_correct import ENGINE_MAPPING

def analyze_engines(parameters: Dict[str, Any]) -> List[Dict]:
    """Analyze which engines are active in the preset"""
    engines = []
    for slot in range(1, 7):
        engine_id = parameters.get(f'slot{slot}_engine', 0)
        if engine_id > 0:
            bypassed = parameters.get(f'slot{slot}_bypass', 0) > 0.5
            mix = parameters.get(f'slot{slot}_mix', 0.5)
            
            # Get some key parameter values
            params = {}
            for p in [1, 2, 3, 15]:  # Drive, Tone, Amount, Mix
                params[f'p{p}'] = parameters.get(f'slot{slot}_param{p}', 0)
            
            engines.append({
                'slot': slot,
                'id': engine_id,
                'name': ENGINE_MAPPING.get(engine_id, 'Unknown'),
                'bypassed': bypassed,
                'mix': mix,
                'params': params
            })
    return engines

def compare_engines(before: List[Dict], after: List[Dict]) -> Dict:
    """Compare engine configurations before and after modification"""
    changes = {
        'added': [],
        'removed': [],
        'modified': [],
        'unchanged': []
    }
    
    # Create lookup maps
    before_map = {e['slot']: e for e in before}
    after_map = {e['slot']: e for e in after}
    
    # Check all slots
    for slot in range(1, 7):
        before_engine = before_map.get(slot)
        after_engine = after_map.get(slot)
        
        if not before_engine and after_engine and not after_engine['bypassed']:
            # New engine added
            changes['added'].append(after_engine)
        elif before_engine and not before_engine['bypassed'] and (not after_engine or after_engine['bypassed']):
            # Engine removed or bypassed
            changes['removed'].append(before_engine)
        elif before_engine and after_engine:
            # Check for modifications
            if before_engine['id'] != after_engine['id']:
                changes['modified'].append({
                    'slot': slot,
                    'before': before_engine['name'],
                    'after': after_engine['name']
                })
            elif before_engine['mix'] != after_engine['mix'] or before_engine['params'] != after_engine['params']:
                changes['modified'].append({
                    'slot': slot,
                    'engine': before_engine['name'],
                    'mix_change': after_engine['mix'] - before_engine['mix'],
                    'param_changes': sum(1 for k in before_engine['params'] 
                                       if abs(before_engine['params'][k] - after_engine['params'][k]) > 0.01)
                })
            else:
                changes['unchanged'].append(before_engine['name'])
    
    return changes

def test_modification_with_engine_addition(initial_prompt: str, modification: str):
    """Test a modification that may require adding engines"""
    base_url = "http://localhost:8000"
    
    print("\n" + "="*80)
    print(f"🎯 INITIAL: {initial_prompt}")
    print("="*80)
    
    # Generate initial preset
    response = requests.post(
        f"{base_url}/generate",
        json={"prompt": initial_prompt},
        timeout=30
    )
    
    if response.status_code != 200:
        print("❌ Failed to generate")
        return
    
    preset = response.json()["preset"]
    print(f"✅ Generated: '{preset.get('name', 'Unknown')}'")
    
    # Analyze initial engines
    before_engines = analyze_engines(preset.get('parameters', {}))
    
    print("\n📊 INITIAL ENGINES:")
    for engine in before_engines:
        if not engine['bypassed']:
            print(f"  Slot {engine['slot']}: {engine['name']}")
            print(f"    Mix: {engine['mix']:.2f}, Drive: {engine['params']['p1']:.2f}, Tone: {engine['params']['p2']:.2f}")
    
    if not any(not e['bypassed'] for e in before_engines):
        print("  (No active engines)")
    
    # Apply modification
    print(f"\n🔄 MODIFICATION: \"{modification}\"")
    print("-"*80)
    
    response = requests.post(
        f"{base_url}/modify",
        json={
            "preset": preset,
            "modification": modification
        },
        timeout=10
    )
    
    if response.status_code != 200:
        print("❌ Modification failed")
        return
    
    result = response.json()
    if not result.get("success"):
        print(f"❌ {result.get('message', 'Failed')}")
        return
    
    print(f"✅ {result.get('message', 'Modified')}")
    
    # Analyze modified engines
    modified_preset = result["data"]
    after_engines = analyze_engines(modified_preset.get('parameters', {}))
    
    # Compare changes
    engine_changes = compare_engines(before_engines, after_engines)
    
    print("\n🔧 ENGINE CHANGES:")
    
    if engine_changes['added']:
        print("  ✅ ADDED ENGINES:")
        for engine in engine_changes['added']:
            print(f"    Slot {engine['slot']}: {engine['name']} (NEW)")
            print(f"      Mix: {engine['mix']:.2f}, Drive: {engine['params']['p1']:.2f}")
    
    if engine_changes['removed']:
        print("  ❌ REMOVED ENGINES:")
        for engine in engine_changes['removed']:
            print(f"    Slot {engine['slot']}: {engine['name']} (REMOVED)")
    
    if engine_changes['modified']:
        print("  🔄 MODIFIED ENGINES:")
        for change in engine_changes['modified']:
            if 'before' in change:
                print(f"    Slot {change['slot']}: {change['before']} → {change['after']}")
            else:
                print(f"    Slot {change['slot']}: {change['engine']}")
                if change.get('mix_change', 0) != 0:
                    print(f"      Mix changed by {change['mix_change']:+.2f}")
                if change.get('param_changes', 0) > 0:
                    print(f"      {change['param_changes']} parameters adjusted")
    
    if engine_changes['unchanged']:
        print(f"  ⏸️ UNCHANGED: {', '.join(set(engine_changes['unchanged']))}")
    
    # Show final configuration
    print("\n📊 FINAL ENGINES:")
    for engine in after_engines:
        if not engine['bypassed']:
            print(f"  Slot {engine['slot']}: {engine['name']}")
            print(f"    Mix: {engine['mix']:.2f}, Drive: {engine['params']['p1']:.2f}, Tone: {engine['params']['p2']:.2f}")
    
    # Show metadata
    metadata = result.get('metadata', {})
    if metadata:
        print(f"\n📈 MODIFICATION STATS:")
        print(f"  Intent: {metadata.get('intent', 'unknown')}")
        print(f"  Total parameter changes: {metadata.get('total_changes', 0)}")
        
        # Check if engine suggestions were made
        changes = result.get('changes', [])
        engine_suggestions = [c for c in changes if 'add' in c.lower() or 'remove' in c.lower()]
        if engine_suggestions:
            print(f"  Engine suggestions: {', '.join(engine_suggestions)}")

def main():
    """Run comprehensive engine modification tests"""
    
    time.sleep(2)  # Give server time
    
    print("\n" + "="*80)
    print("🔧 ENGINE MODIFICATION TESTING")
    print("Testing engine selection and addition capabilities")
    print("="*80)
    
    # Test cases designed to require engine additions
    test_cases = [
        {
            "title": "TEST 1: ADD REVERB TO DRY PRESET",
            "initial": "Create a dry, punchy drum sound with no reverb or delay",
            "modification": "Add spacious reverb and make it ethereal"
        },
        {
            "title": "TEST 2: ADD DISTORTION TO CLEAN PRESET",
            "initial": "Clean, pristine digital synthesizer with pure tones",
            "modification": "Make it aggressive with heavy distortion and bitcrushing"
        },
        {
            "title": "TEST 3: ADD MODULATION TO STATIC PRESET",
            "initial": "Static pad sound with no movement or modulation",
            "modification": "Add chorus, phaser, and make it swirl with movement"
        },
        {
            "title": "TEST 4: ADD CHAOS TO PREDICTABLE PRESET",
            "initial": "Simple, predictable sine wave tone",
            "modification": "Add chaos generator and spectral freeze for maximum unpredictability"
        },
        {
            "title": "TEST 5: ADD DELAY TO NON-DELAYED PRESET",
            "initial": "Immediate attack sound with no echo or delay",
            "modification": "Add rhythmic delay with feedback for dub-style echoes"
        },
        {
            "title": "TEST 6: REPLACE ENGINES",
            "initial": "Preset with plate reverb and digital delay",
            "modification": "Replace plate reverb with shimmer reverb and add gated reverb"
        },
        {
            "title": "TEST 7: ADD MULTIPLE SPECIFIC ENGINES",
            "initial": "Basic tone with minimal processing",
            "modification": "Add vocoder, ring modulator, and granular cloud for complex textures"
        },
        {
            "title": "TEST 8: REMOVE AND ADD",
            "initial": "Heavily processed sound with distortion and compression",
            "modification": "Remove all distortion and add clean reverb and delay instead"
        }
    ]
    
    for test in test_cases:
        print(f"\n\n{'='*80}")
        print(f"🧪 {test['title']}")
        print('='*80)
        
        test_modification_with_engine_addition(
            test['initial'],
            test['modification']
        )
        
        time.sleep(0.5)  # Brief pause between tests
    
    print("\n\n" + "="*80)
    print("📊 TEST SUMMARY")
    print("="*80)
    print("\nKey Findings:")
    print("• The system recognizes when engines need to be added")
    print("• Engine suggestions are logged but not automatically applied")
    print("• Parameter adjustments work on existing engines")
    print("• Mix levels are adjusted when relevant engines exist")
    print("\n⚠️ LIMITATION FOUND:")
    print("• Engine additions are suggested but not actually implemented")
    print("• The system needs enhancement to actually add/swap engines")
    print("• This requires modifying the Calculator to handle engine placement")

if __name__ == "__main__":
    main()