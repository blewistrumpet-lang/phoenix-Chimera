#!/usr/bin/env python3
"""
Test what prompt is actually being sent to the AI
"""

from visionary_complete import CompleteVisionary
import asyncio

async def test():
    visionary = CompleteVisionary()
    
    # Test prompt
    test_prompt = "ambient pad with shimmer reverb and spring reverb"
    
    # Get context
    context = visionary.analyze_prompt_context(test_prompt)
    print("Context:", context)
    print("\n" + "="*60)
    
    # Build prompt
    prompt = visionary.build_generation_prompt(test_prompt, context)
    print("ACTUAL PROMPT SENT TO AI:")
    print("="*60)
    print(prompt)
    print("="*60)
    
    # Check if it mentions specific engines
    if "Engine 42" in prompt and "Shimmer Reverb" in prompt:
        print("✅ Shimmer Reverb (42) is mentioned")
    else:
        print("❌ Shimmer Reverb (42) NOT mentioned!")
    
    if "Engine 40" in prompt and "Spring Reverb" in prompt:
        print("✅ Spring Reverb (40) is mentioned")
    else:
        print("❌ Spring Reverb (40) NOT mentioned!")
    
    # Check if selection rules are mentioned
    if "if user asks for spring reverb, use Spring Reverb ID 40" in prompt.lower():
        print("✅ Selection rules are explicit")
    else:
        print("❌ Selection rules NOT explicit!")

asyncio.run(test())