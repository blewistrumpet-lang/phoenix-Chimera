#!/usr/bin/env python3
"""
Diagnose why training fitness is declining
Test our hypotheses about what's wrong
"""

import requests
import json
from engine_mapping_authoritative import *

def test_current_system():
    """Test what the system is actually doing"""
    
    print("\n" + "="*70)
    print("TRAINING DIAGNOSTIC")
    print("="*70)
    
    # Test prompt
    prompt = "deep dubstep bass wobble with sub frequencies"
    
    print(f"\nTest Prompt: '{prompt}'")
    print("-"*70)
    
    # Get current generation
    try:
        response = requests.post(
            "http://localhost:8000/generate",
            json={"prompt": prompt},
            timeout=10
        )
        
        if response.status_code == 200:
            data = response.json()
            preset = data["preset"]
            params = preset.get("parameters", {})
            
            print(f"\n1. PRESET NAME: {preset.get('name', 'Unknown')}")
            
            print(f"\n2. ENGINES SELECTED:")
            engines_selected = []
            for slot in range(1, 7):
                engine_id = params.get(f"slot{slot}_engine", 0)
                if engine_id > 0 and params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                    engine_name = get_engine_name(engine_id)
                    engine_cat = get_engine_category(engine_id)
                    engines_selected.append((engine_id, engine_name, engine_cat))
                    print(f"   Slot {slot}: {engine_name} ({engine_cat})")
            
            print(f"\n3. ANALYSIS:")
            
            # Check for literal bass engine
            bass_engines = [ENGINE_SUB_BASS, ENGINE_VINTAGE_TUBE, ENGINE_RODENT_DISTORTION]
            has_literal_bass = any(e[0] in bass_engines for e in engines_selected)
            print(f"   Has literal bass engine: {has_literal_bass}")
            
            # Check for creative bass processing
            creative_bass = []
            for eid, name, cat in engines_selected:
                if cat == "Filters & EQ":
                    creative_bass.append(name)
                if "Filter" in name or "EQ" in name:
                    creative_bass.append(name)
            
            if creative_bass:
                print(f"   Creative bass processing: {', '.join(creative_bass)}")
            
            # Check wobble implementation
            wobble_engines = []
            for eid, name, cat in engines_selected:
                if "Filter" in name or "LFO" in name or "Formant" in name:
                    wobble_engines.append(name)
            
            if wobble_engines:
                print(f"   Wobble implementation: {', '.join(wobble_engines)}")
            
            # Calculate different scoring methods
            print(f"\n4. SCORING COMPARISON:")
            
            # Method 1: Current strict scoring
            strict_score = 0.3 if has_literal_bass else 0.0
            print(f"   Strict keyword match: {strict_score:.1%}")
            
            # Method 2: Category-based scoring
            category_score = 0.0
            for eid, name, cat in engines_selected:
                if cat == "Filters & EQ" and "bass" in prompt.lower():
                    category_score += 0.2
                if cat == "Distortion" and ("dubstep" in prompt.lower() or "bass" in prompt.lower()):
                    category_score += 0.2
                if "wobble" in prompt.lower() and "filter" in name.lower():
                    category_score += 0.3
            category_score = min(category_score, 1.0)
            print(f"   Category-based: {category_score:.1%}")
            
            # Method 3: Semantic scoring (considering intent)
            semantic_score = 0.0
            # Dubstep bass needs: low frequency emphasis + modulation + possibly distortion
            has_low_freq = any("EQ" in n or "Filter" in n or "Bass" in n for _, n, _ in engines_selected)
            has_modulation = any("Formant" in n or "Filter" in n or "LFO" in n for _, n, _ in engines_selected)
            has_character = any(c in ["Distortion", "Dynamics"] for _, _, c in engines_selected)
            
            if has_low_freq: semantic_score += 0.4
            if has_modulation: semantic_score += 0.4  # Wobble
            if has_character: semantic_score += 0.2
            print(f"   Semantic intent: {semantic_score:.1%}")
            
            print(f"\n5. VERDICT:")
            if strict_score < 0.3 and semantic_score > 0.6:
                print("   ❌ PROBLEM CONFIRMED: Good selection scored as bad!")
                print("   The AI made creative choices that our scorer doesn't recognize")
            elif len(engines_selected) > 5:
                print("   ⚠️ Too many engines selected")
            elif len(engines_selected) == 0:
                print("   ❌ No engines selected - system might be failing")
            else:
                print("   🤔 Unclear - need more test cases")
                
    except Exception as e:
        print(f"Error: {e}")

def test_scoring_function():
    """Test if our scoring function is the problem"""
    
    print("\n" + "="*70)
    print("SCORING FUNCTION TEST")
    print("="*70)
    
    test_cases = [
        {
            "prompt": "bass",
            "engines": [ENGINE_SUB_BASS],
            "expected": "HIGH",
            "reason": "Direct bass engine for bass prompt"
        },
        {
            "prompt": "bass", 
            "engines": [ENGINE_PARAMETRIC_EQ],
            "expected": "MEDIUM",
            "reason": "EQ can shape bass but not specific"
        },
        {
            "prompt": "bass wobble",
            "engines": [ENGINE_FORMANT_FILTER, ENGINE_SUB_BASS],
            "expected": "HIGH", 
            "reason": "Filter for wobble + bass engine"
        },
        {
            "prompt": "bass",
            "engines": [ENGINE_SHIMMER_REVERB],
            "expected": "LOW",
            "reason": "Reverb not relevant for bass"
        }
    ]
    
    from trinity_learning_forced_cloud import ForcedCloudLearning
    system = ForcedCloudLearning()
    
    for test in test_cases:
        # Create fake preset
        preset = {"parameters": {}}
        for i, engine_id in enumerate(test["engines"]):
            preset["parameters"][f"slot{i+1}_engine"] = engine_id
            preset["parameters"][f"slot{i+1}_bypass"] = 0.0
        
        score = system.score_engine_selection(test["prompt"], preset)
        
        print(f"\nPrompt: '{test['prompt']}'")
        print(f"Engines: {[get_engine_name(e) for e in test['engines']]}")
        print(f"Score: {score:.1%}")
        print(f"Expected: {test['expected']}")
        print(f"Reason: {test['reason']}")
        
        if test["expected"] == "HIGH" and score < 0.5:
            print("❌ SCORING BUG!")
        elif test["expected"] == "LOW" and score > 0.3:
            print("❌ SCORING BUG!")

if __name__ == "__main__":
    test_current_system()
    test_scoring_function()