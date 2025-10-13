#!/usr/bin/env python3
"""
Test a specific prompt to find the f-string error
"""

import requests
import json

def test_prompt(prompt):
    """Test a specific prompt"""
    
    print(f"Testing: '{prompt}'")
    print("-" * 60)
    
    try:
        response = requests.post(
            "http://localhost:8000/generate",
            json={"prompt": prompt},
            timeout=10
        )
        
        if response.status_code == 200:
            data = response.json()
            preset = data.get("preset", {})
            name = preset.get("name", "Unknown")
            print(f"✅ SUCCESS: Generated '{name}'")
            return True
        else:
            error = response.json().get("detail", "Unknown error")
            print(f"❌ FAILED: {error}")
            return False
            
    except Exception as e:
        print(f"❌ ERROR: {str(e)}")
        return False

if __name__ == "__main__":
    # Test the failing prompts one by one
    failing_prompts = [
        "Create warm vintage vocals like Billie Eilish - intimate and close",
        "Aggressive metal guitar with heavy distortion and tight gating",
        "Ethereal ambient pad with shimmer reverb and lots of space"
    ]
    
    for prompt in failing_prompts:
        test_prompt(prompt)
        print()