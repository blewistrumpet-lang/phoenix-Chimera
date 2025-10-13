#!/usr/bin/env python3
"""
Fix to ensure Cloud AI engine selections are preserved throughout the pipeline
"""

def ensure_engines_preserved(blueprint, preset, calculator_result, final_result):
    """
    Ensure that engines specified by Cloud AI are preserved
    """
    
    # Get Cloud AI requested engines
    cloud_engines = {}
    for slot in range(1, 7):
        engine = blueprint.get(f"slot{slot}_engine", 0)
        if engine > 0:
            cloud_engines[slot] = engine
    
    # Check each stage
    print("Cloud AI requested:")
    for slot, engine in cloud_engines.items():
        print(f"  Slot {slot}: Engine {engine}")
    
    print("\nOracle returned:")
    for slot in range(1, 7):
        engine = preset.get(f"slot{slot}_engine", 0)
        if engine > 0:
            print(f"  Slot {slot}: Engine {engine}")
    
    print("\nCalculator returned:")
    for slot in range(1, 7):
        engine = calculator_result.get(f"slot{slot}_engine", 0)
        if engine > 0:
            print(f"  Slot {slot}: Engine {engine}")
    
    print("\nAlchemist returned:")
    for slot in range(1, 7):
        engine = final_result.get(f"slot{slot}_engine", 0)
        if engine > 0:
            print(f"  Slot {slot}: Engine {engine}")
    
    # Fix: Ensure Cloud AI engines are in final result
    for slot, engine in cloud_engines.items():
        if final_result.get(f"slot{slot}_engine", 0) != engine:
            print(f"\n⚠️ WARNING: Cloud AI requested engine {engine} for slot {slot} was lost!")
            # Find an empty slot or override
            placed = False
            for alt_slot in range(1, 7):
                if final_result.get(f"slot{alt_slot}_engine", 0) == 0:
                    final_result[f"slot{alt_slot}_engine"] = engine
                    print(f"   Placed engine {engine} in slot {alt_slot}")
                    placed = True
                    break
            
            if not placed:
                # Override slot 6 as last resort
                final_result["slot6_engine"] = engine
                print(f"   Override slot 6 with engine {engine}")
    
    return final_result

# Strategy to fix the system:
print("""
COMPREHENSIVE FIX STRATEGY:
==========================

1. CALCULATOR ENHANCEMENT:
   - When Cloud AI specifies engines, Calculator should ADD to them, not replace
   - Check if blueprint has slot engines and preserve them

2. ORACLE ENHANCEMENT:
   - Prioritize presets that contain the requested engines
   - If no preset has all requested engines, create a hybrid

3. ALCHEMIST ENHANCEMENT:
   - Never remove engines that were explicitly requested
   - Signal chain optimization should rearrange, not remove

4. MAIN PIPELINE FIX:
   - Pass the Cloud AI blueprint all the way through
   - Each component should respect the blueprint's engine selections
""")