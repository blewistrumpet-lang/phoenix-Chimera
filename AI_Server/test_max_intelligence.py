#!/usr/bin/env python3
"""
Test Maximum Intelligence Parameter System
Demonstrates intelligent parameter setting
"""

import requests
import json
import time
from datetime import datetime

def test_prompt(prompt):
    """Test a single prompt and show parameter intelligence"""
    url = "http://localhost:8000/generate"
    
    print(f"\n{'='*80}")
    print(f"TESTING: {prompt}")
    print(f"{'='*80}")
    
    try:
        response = requests.post(url, json={"prompt": prompt}, timeout=60)
        
        if response.status_code == 200:
            data = response.json()
            preset = data.get("preset", {})
            debug = data.get("debug", {})
            
            print(f"\n✅ SUCCESS: {preset.get('name', 'Unknown')}")
            
            # Show engines selected
            print(f"\n📦 Engines Selected:")
            for slot in preset.get("slots", []):
                if slot.get("engine_id", 0) != 0:
                    print(f"  • {slot.get('engine_name', 'Unknown')} (ID: {slot['engine_id']})")
            
            # Show extracted values
            extracted = debug.get("calculator", {}).get("extracted_values", {})
            if extracted:
                print(f"\n🎯 Extracted Values:")
                for key, value in extracted.items():
                    print(f"  • {key}: {value.get('original', '')} → {value.get('value', ''):.3f}")
            
            # Show intelligent parameter changes
            print(f"\n🧠 Intelligent Parameter Settings:")
            param_changes = []
            for slot in preset.get("slots", []):
                if slot.get("engine_id", 0) == 0:
                    continue
                
                engine_name = slot.get("engine_name", "Unknown")
                for i, param in enumerate(slot.get("parameters", [])):
                    # Only show non-default values
                    if abs(param.get("value", 0.5) - 0.5) > 0.01:
                        param_changes.append(f"  • {engine_name}.param{i+1}: {param.get('value', 0.5):.3f}")
            
            if param_changes:
                for change in param_changes[:10]:  # Show first 10
                    print(change)
                if len(param_changes) > 10:
                    print(f"  ... and {len(param_changes) - 10} more")
            else:
                print("  ⚠️ No parameter changes detected")
            
            # Show cache stats
            cache_stats = debug.get("calculator", {}).get("cache_stats", {})
            if cache_stats:
                print(f"\n📊 Intelligence Stats:")
                print(f"  • Cache hits: {cache_stats.get('cache_hits', 0)}")
                print(f"  • Claude calls: {cache_stats.get('claude_calls', 0)}")
                print(f"  • Total tokens: {cache_stats.get('total_tokens', 0)}")
            
            return {
                "success": True,
                "preset": preset,
                "debug": debug
            }
        else:
            print(f"\n❌ ERROR: HTTP {response.status_code}")
            print(response.text)
            return {"success": False, "error": response.text}
            
    except Exception as e:
        print(f"\n❌ ERROR: {str(e)}")
        return {"success": False, "error": str(e)}

def main():
    print("="*80)
    print("MAXIMUM INTELLIGENCE PARAMETER SYSTEM TEST")
    print(f"Started: {datetime.now().strftime('%H:%M:%S')}")
    print("="*80)
    
    # Give server time to start
    print("\nWaiting for server to start...")
    time.sleep(5)
    
    # Test prompts that demonstrate intelligence
    test_prompts = [
        # Specific value extraction
        "tape delay at 1/8 dotted with 35% feedback and subtle wow",
        
        # Musical style understanding
        "vintage Beatles Abbey Road drums with warm compression",
        
        # Complex technical requirements
        "parallel compression with 4:1 ratio at -10dB threshold",
        
        # Genre-specific intelligence
        "modern EDM sidechain compression with aggressive pumping",
        
        # Creative interpretation
        "ethereal space reverb with shimmer and long decay"
    ]
    
    results = []
    for prompt in test_prompts:
        result = test_prompt(prompt)
        results.append(result)
        time.sleep(2)  # Give Claude time between requests
    
    # Summary
    print(f"\n{'='*80}")
    print("SUMMARY")
    print(f"{'='*80}")
    
    successful = sum(1 for r in results if r.get("success", False))
    print(f"\n✅ Success rate: {successful}/{len(test_prompts)} ({successful/len(test_prompts)*100:.0f}%)")
    
    # Check cache efficiency
    try:
        cache_response = requests.get("http://localhost:8000/cache_stats")
        if cache_response.status_code == 200:
            cache_data = cache_response.json()
            print(f"\n📊 Overall Cache Efficiency:")
            print(f"  • Total cache hits: {cache_data.get('cache_hits', 0)}")
            print(f"  • Total Claude calls: {cache_data.get('claude_calls', 0)}")
            print(f"  • Efficiency: {cache_data.get('efficiency', '0%')}")
            print(f"  • Cached styles: {cache_data.get('cached_styles', 0)}")
    except:
        pass
    
    print(f"\n🏁 Test completed: {datetime.now().strftime('%H:%M:%S')}")

if __name__ == "__main__":
    main()