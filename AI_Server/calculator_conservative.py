"""
Conservative Calculator patch - only adds utility engines when truly needed
"""

def patch_calculator():
    """Monkey patch the Calculator to be more conservative with utilities"""
    
    # Import the Calculator
    import calculator
    from engine_mapping_authoritative import *
    
    # Save original method
    original_analyze = calculator.Calculator._analyze_preset_for_utility_needs
    
    # Create conservative version
    def conservative_analyze_preset_for_utility_needs(self, preset, prompt_lower, blueprint):
        """
        Conservative analysis - only add utilities when EXPLICITLY needed.
        """
        parameters = preset.get("parameters", {})
        
        # Start with everything FALSE - be conservative
        analysis = {
            "needs_stereo_processing": False,
            "needs_level_management": False,
            "needs_mono_compatibility": False,
            "needs_phase_correction": False
        }
        
        # Only add utilities for EXPLICIT requests:
        
        # 1. Stereo processing - only if explicitly requested
        if any(x in prompt_lower for x in ["mid-side", "m/s processor", "stereo processor"]):
            analysis["needs_stereo_processing"] = True
        
        # 2. Level management - only for mastering chains
        if "mastering" in prompt_lower and "chain" in prompt_lower:
            # Check if we already have a limiter
            has_limiter = False
            for slot in range(1, 7):
                if parameters.get(f"slot{slot}_engine", 0) == ENGINE_MASTERING_LIMITER:
                    has_limiter = True
                    break
            if not has_limiter:
                analysis["needs_level_management"] = True
        
        # 3. Mono compatibility - only if explicitly requested
        if "mono compatible" in prompt_lower or "check mono" in prompt_lower:
            analysis["needs_mono_compatibility"] = True
        
        # 4. Phase correction - only if phase problems mentioned
        if "phase issues" in prompt_lower or "fix phase" in prompt_lower:
            analysis["needs_phase_correction"] = True
        
        return analysis
    
    # Replace the method
    calculator.Calculator._analyze_preset_for_utility_needs = conservative_analyze_preset_for_utility_needs
    
    print("Calculator patched to be conservative with utility engines")

if __name__ == "__main__":
    patch_calculator()