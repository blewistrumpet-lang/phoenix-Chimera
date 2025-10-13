#!/usr/bin/env python3
"""
Debug the enhanced system to find the formatting error
"""

from alchemist_enhanced import AlchemistEnhanced

def test_alchemist():
    print("Testing AlchemistEnhanced...")
    
    alchemist = AlchemistEnhanced()
    
    test_preset = {
        "slot1_engine": 15,  # Vintage Tube
        "slot1_param1": 0.5,
        "slot2_engine": 39,  # Plate Reverb
        "slot2_param0": 0.5
    }
    
    try:
        result = alchemist.finalize_preset(test_preset, "warm vintage vocals")
        print("✅ Alchemist works!")
        print(f"Generated: {result.get('name', 'Unknown')}")
    except Exception as e:
        print(f"❌ Alchemist error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    test_alchemist()