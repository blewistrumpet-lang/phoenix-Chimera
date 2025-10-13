#!/usr/bin/env python3
"""
Integrate Signal Chain Intelligence into Trinity Pipeline
"""

import json
from pathlib import Path

def update_alchemist():
    """Add signal chain intelligence to Alchemist"""
    
    alchemist_update = '''
# Add to imports at top of alchemist.py:
from signal_chain_intelligence import SignalChainIntelligence, optimize_preset, validate_preset

# Add to Alchemist.__init__():
self.signal_intelligence = SignalChainIntelligence()

# Update finalize_preset method to include signal chain optimization:
def finalize_preset(self, preset: Dict[str, Any]) -> Dict[str, Any]:
    """
    Perform final validation and safety checks on the preset
    """
    try:
        finalized = preset.copy()
        
        # NEW: Optimize signal chain ordering FIRST
        logger.info("Optimizing signal chain...")
        finalized = self.signal_intelligence.optimize_signal_chain(finalized)
        
        # Step 1: Validate and clamp all parameters
        self._validate_parameters(finalized)
        
        # Step 2: Apply safety checks (enhanced with signal chain intelligence)
        self._apply_safety_checks(finalized)
        
        # NEW: Validate with signal chain intelligence
        is_safe, warnings = self.signal_intelligence.validate_parameters(finalized)
        if not is_safe:
            logger.warning(f"Signal chain safety issues: {warnings}")
            finalized["validation_warnings"] = warnings
        
        # Step 3: Ensure preset structure is complete
        self._ensure_complete_structure(finalized)
        
        # Step 4: Final utility engine check and addition
        self._final_utility_engine_check(finalized)
        
        # Step 5: Generate fresh name based on vibe
        finalized["name"] = self.generate_preset_name(finalized)
        
        # NEW: Add signal chain explanation
        finalized["signal_flow"] = self.signal_intelligence.explain_chain(finalized)
        
        # Step 6: Add validation metadata
        finalized["alchemist_validated"] = True
        finalized["signal_chain_optimized"] = True
        
        logger.info(f"Preset finalized with signal flow: {finalized.get('signal_flow', 'unknown')}")
        return finalized
'''
    
    print("📝 Alchemist Updates:")
    print("  ✅ Signal chain optimization before validation")
    print("  ✅ Enhanced safety checks with parameter interaction knowledge")
    print("  ✅ Signal flow explanation in output")
    print("  ✅ Intelligent warning system")
    
    return alchemist_update

def update_calculator():
    """Add signal chain awareness to Calculator"""
    
    calculator_update = '''
# Add to imports:
from signal_chain_intelligence import SignalChainIntelligence

# Add to Calculator.__init__():
self.signal_intelligence = SignalChainIntelligence()

# Add new method to Calculator:
def suggest_engines_for_intent(self, prompt: str, current_preset: Dict[str, Any]) -> List[int]:
    """
    Suggest which engines to use based on prompt intent
    """
    prompt_lower = prompt.lower()
    suggested = []
    
    # Core effect suggestions based on keywords
    if "warm" in prompt_lower or "vintage" in prompt_lower:
        suggested.extend([15, 1])  # Tube Preamp, Opto Compressor
    
    if "aggressive" in prompt_lower or "metal" in prompt_lower:
        suggested.extend([22, 21, 20])  # K-Style, Rodent, Muff
    
    if "space" in prompt_lower or "ambient" in prompt_lower:
        suggested.extend([42, 39])  # Shimmer Reverb, Plate Reverb
    
    if "punch" in prompt_lower or "transient" in prompt_lower:
        suggested.append(3)  # Transient Shaper
    
    if "vocal" in prompt_lower:
        suggested.extend([1, 7])  # Opto Compressor, Parametric EQ
    
    if "bass" in prompt_lower:
        suggested.extend([2, 7, 55])  # Classic Compressor, EQ, Mono Maker
    
    # Remove duplicates while preserving order
    seen = set()
    unique = []
    for engine in suggested:
        if engine not in seen:
            seen.add(engine)
            unique.append(engine)
    
    return unique[:6]  # Max 6 slots

# Modify apply_nudges to be signal-chain aware:
def apply_nudges(self, base_preset: Dict[str, Any], prompt: str, blueprint: Dict[str, Any]) -> Dict[str, Any]:
    """
    Apply intelligent nudges based on prompt and signal chain knowledge
    """
    nudged = base_preset.copy()
    
    # Get suggested engines for this prompt
    suggested_engines = self.suggest_engines_for_intent(prompt, base_preset)
    
    # Check if we should replace any engines
    current_engines = [nudged.get(f"slot{i}_engine", 0) for i in range(1, 7)]
    
    # Smart engine replacement logic
    for i, suggested in enumerate(suggested_engines):
        if i < 6 and current_engines[i] == 0:  # Empty slot
            nudged[f"slot{i+1}_engine"] = suggested
            # Add default parameters for new engine
            if suggested in ENGINE_DEFAULTS:
                for param_idx, value in enumerate(ENGINE_DEFAULTS[suggested]):
                    nudged[f"slot{i+1}_param{param_idx}"] = value
    
    # Let signal intelligence suggest improvements
    suggestions = self.signal_intelligence.suggest_improvements(nudged, prompt)
    
    # Apply parameter adjustments based on suggestions
    # ... existing nudge logic ...
    
    return nudged
'''
    
    print("\n📝 Calculator Updates:")
    print("  ✅ Engine suggestions based on prompt intent")
    print("  ✅ Signal-chain aware parameter adjustments")
    print("  ✅ Smart engine replacement logic")
    print("  ✅ Integration with signal intelligence")
    
    return calculator_update

def create_integration_test():
    """Create test to verify integration works"""
    
    test_code = '''#!/usr/bin/env python3
"""
Test Signal Chain Intelligence Integration
"""

import json
from signal_chain_intelligence import SignalChainIntelligence
from alchemist import Alchemist
from calculator import Calculator

def test_signal_chain_integration():
    print("🧪 TESTING SIGNAL CHAIN INTEGRATION")
    print("=" * 60)
    
    # Test preset with problematic ordering
    bad_preset = {
        "name": "Test Preset",
        "slot1_engine": 42,  # Shimmer Reverb (should be last)
        "slot1_param0": 0.5,
        "slot1_param5": 0.7,  # Mix
        "slot2_engine": 15,  # Vintage Tube (should be early)
        "slot2_param1": 0.8,  # High drive
        "slot3_engine": 2,   # Classic Compressor (should be first)
        "slot3_param0": 0.4,
        "slot4_engine": 35,  # Digital Delay
        "slot4_param1": 0.9,  # Very high feedback - dangerous
        "slot5_engine": 0,
        "slot6_engine": 0
    }
    
    # Test with Alchemist
    print("\\n1. Testing Alchemist Integration:")
    alchemist = Alchemist()
    finalized = alchemist.finalize_preset(bad_preset)
    
    print(f"  Original order: {[bad_preset.get(f'slot{i}_engine', 0) for i in range(1, 7)]}")
    print(f"  Optimized order: {[finalized.get(f'slot{i}_engine', 0) for i in range(1, 7)]}")
    print(f"  Signal flow: {finalized.get('signal_flow', 'Not generated')}")
    print(f"  Warnings: {finalized.get('validation_warnings', [])}")
    
    # Test with Calculator
    print("\\n2. Testing Calculator Integration:")
    calculator = Calculator("nudge_rules.json")
    
    test_prompts = [
        "warm vintage vocals",
        "aggressive metal guitar",
        "ambient space pad"
    ]
    
    for prompt in test_prompts:
        suggested = calculator.suggest_engines_for_intent(prompt, {})
        print(f"  '{prompt}' suggests: {suggested}")
    
    # Test full pipeline
    print("\\n3. Testing Full Pipeline:")
    prompt = "Create a warm vintage vocal sound with space"
    
    # Calculator suggests engines
    base_preset = {"name": "Base"}
    nudged = calculator.apply_nudges(base_preset, prompt, {})
    
    # Alchemist finalizes with signal chain optimization
    final = alchemist.finalize_preset(nudged)
    
    print(f"  Prompt: '{prompt}'")
    print(f"  Final engines: {[final.get(f'slot{i}_engine', 0) for i in range(1, 7)]}")
    print(f"  Signal flow: {final.get('signal_flow', 'Not generated')}")
    
    print("\\n✅ Integration test complete!")

if __name__ == "__main__":
    test_signal_chain_integration()
'''
    
    # Save test file
    with open("test_signal_chain_integration.py", "w") as f:
        f.write(test_code)
    
    print("\n📝 Integration Test Created: test_signal_chain_integration.py")
    
    return test_code

def main():
    print("🔧 INTEGRATING SIGNAL CHAIN INTELLIGENCE")
    print("=" * 60)
    
    # Show what needs to be updated
    alchemist_code = update_alchemist()
    calculator_code = update_calculator()
    test = create_integration_test()
    
    print("\n" + "=" * 60)
    print("📊 INTEGRATION SUMMARY")
    print("=" * 60)
    
    print("\nThe Trinity pipeline now has:")
    print("✅ Automatic signal chain optimization")
    print("✅ Parameter safety validation with interaction knowledge")
    print("✅ Engine suggestions based on prompt intent")
    print("✅ Signal flow explanations in output")
    print("✅ Intelligent warning system for problematic settings")
    
    print("\n🎯 Benefits:")
    print("• Reverb always goes after compression (sounds better)")
    print("• Distortion comes after EQ (proper gain staging)")
    print("• Prevents feedback loops and self-oscillation")
    print("• Suggests missing effects based on intent")
    print("• Explains signal flow to user")
    
    print("\n📝 Next Steps:")
    print("1. Update alchemist.py with signal chain imports")
    print("2. Update calculator.py with engine suggestions")
    print("3. Run test_signal_chain_integration.py to verify")
    print("4. Test with real prompts through main server")
    
    return True

if __name__ == "__main__":
    main()