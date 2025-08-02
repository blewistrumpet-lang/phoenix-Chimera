import logging
from typing import Dict, Any, Tuple

logger = logging.getLogger(__name__)

class Alchemist:
    """
    The Alchemist performs final validation, safety checks, and parameter clamping
    to ensure the preset is valid and safe for the audio plugin.
    """
    
    def __init__(self):
        # Parameter ranges for each engine type
        self.parameter_ranges = {
            # K-Style Overdrive
            0: {
                "param1": (0.0, 1.0),  # Drive
                "param2": (0.0, 1.0),  # Tone
                "param3": (0.0, 1.0)   # Level
            },
            # Tape Echo
            1: {
                "param1": (0.0, 1.0),  # Time
                "param2": (0.0, 0.95), # Feedback (limited to prevent runaway)
                "param3": (0.0, 1.0),  # Mix
                "param4": (0.0, 1.0),  # Wow
                "param5": (0.0, 1.0)   # Flutter
            },
            # Plate Reverb
            2: {
                "param1": (0.0, 1.0),  # Size
                "param2": (0.0, 1.0),  # Damping
                "param3": (0.0, 1.0),  # Predelay
                "param4": (0.0, 1.0)   # Mix
            }
        }
        
        # Safety limits
        self.safety_limits = {
            "max_total_gain": 2.0,     # Maximum combined gain across slots
            "max_feedback": 0.95,      # Maximum feedback to prevent runaway
            "min_output_level": 0.1    # Minimum output level to prevent silence
        }
    
    def finalize_preset(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Perform final validation and safety checks on the preset
        """
        try:
            finalized = preset.copy()
            
            # Step 1: Validate and clamp all parameters
            self._validate_parameters(finalized)
            
            # Step 2: Apply safety checks
            self._apply_safety_checks(finalized)
            
            # Step 3: Ensure preset structure is complete
            self._ensure_complete_structure(finalized)
            
            # Step 4: Add validation metadata
            finalized["alchemist_validated"] = True
            finalized["validation_warnings"] = self._check_for_warnings(finalized)
            
            logger.info("Preset finalized successfully")
            return finalized
            
        except Exception as e:
            logger.error(f"Error in finalize_preset: {str(e)}")
            # Return a safe default preset on error
            return self._create_safe_default()
    
    def _validate_parameters(self, preset: Dict[str, Any]):
        """Validate and clamp all parameters to valid ranges"""
        parameters = preset.get("parameters", {})
        
        for param_name, value in parameters.items():
            # Ensure value is a float
            if not isinstance(value, (int, float)):
                parameters[param_name] = 0.5
                continue
            
            # Basic clamping to 0-1 range
            parameters[param_name] = self._clamp(float(value), 0.0, 1.0)
            
            # Engine-specific validation
            if "param" in param_name:
                slot_num = int(param_name[4])  # Extract slot number
                param_num = int(param_name.split("param")[1])
                
                # Get engine ID for this slot
                engine_param = f"slot{slot_num}_engine"
                if engine_param in parameters:
                    engine_id = int(parameters[engine_param]) - 1
                    
                    # Apply engine-specific ranges if available
                    if engine_id in self.parameter_ranges:
                        param_key = f"param{param_num}"
                        if param_key in self.parameter_ranges[engine_id]:
                            min_val, max_val = self.parameter_ranges[engine_id][param_key]
                            parameters[param_name] = self._clamp(parameters[param_name], min_val, max_val)
    
    def _apply_safety_checks(self, preset: Dict[str, Any]):
        """Apply safety checks to prevent audio issues"""
        parameters = preset.get("parameters", {})
        
        # Check 1: Limit total gain to prevent clipping
        total_gain = 0.0
        for slot in [1, 2]:
            if f"slot{slot}_bypass" not in parameters or parameters[f"slot{slot}_bypass"] < 0.5:
                # Slot is active
                level_param = f"slot{slot}_param3"  # Assuming param3 is level/mix
                if level_param in parameters:
                    total_gain += parameters[level_param]
        
        if total_gain > self.safety_limits["max_total_gain"]:
            # Scale down levels proportionally
            scale_factor = self.safety_limits["max_total_gain"] / total_gain
            for slot in [1, 2]:
                level_param = f"slot{slot}_param3"
                if level_param in parameters:
                    parameters[level_param] *= scale_factor
            logger.warning(f"Scaled down levels to prevent clipping (total gain was {total_gain:.2f})")
        
        # Check 2: Ensure at least one slot is active
        slot1_bypass = parameters.get("slot1_bypass", 0.0)
        slot2_bypass = parameters.get("slot2_bypass", 0.0)
        
        if slot1_bypass > 0.5 and slot2_bypass > 0.5:
            # Both slots bypassed - activate slot 1 with safe defaults
            parameters["slot1_bypass"] = 0.0
            parameters["slot1_engine"] = 1  # K-Style Overdrive
            logger.warning("Both slots were bypassed - activated slot 1")
        
        # Check 3: Prevent feedback runaway in delay effects
        for slot in [1, 2]:
            feedback_param = f"slot{slot}_param2"
            if feedback_param in parameters:
                engine_param = f"slot{slot}_engine"
                if engine_param in parameters and parameters[engine_param] == 2:  # Tape Echo
                    parameters[feedback_param] = min(parameters[feedback_param], 
                                                   self.safety_limits["max_feedback"])
    
    def _ensure_complete_structure(self, preset: Dict[str, Any]):
        """Ensure the preset has all required fields"""
        # Ensure required top-level fields
        if "name" not in preset:
            preset["name"] = "Alchemist Preset"
        
        if "parameters" not in preset:
            preset["parameters"] = {}
        
        # Ensure all parameter slots exist
        parameters = preset["parameters"]
        
        for slot in [1, 2]:
            # Ensure all 10 parameters exist for each slot
            for param in range(1, 11):
                param_name = f"slot{slot}_param{param}"
                if param_name not in parameters:
                    parameters[param_name] = 0.5
            
            # Ensure engine selector exists
            engine_param = f"slot{slot}_engine"
            if engine_param not in parameters:
                parameters[engine_param] = 0 if slot == 1 else -1
            
            # Ensure bypass exists
            bypass_param = f"slot{slot}_bypass"
            if bypass_param not in parameters:
                parameters[bypass_param] = 0.0 if slot == 1 else 1.0
    
    def _check_for_warnings(self, preset: Dict[str, Any]) -> list:
        """Check for potential issues and return warnings"""
        warnings = []
        parameters = preset.get("parameters", {})
        
        # Check for extreme parameter values
        for param_name, value in parameters.items():
            if "param" in param_name and isinstance(value, (int, float)):
                if value < 0.1:
                    warnings.append(f"{param_name} is very low ({value:.2f})")
                elif value > 0.9:
                    warnings.append(f"{param_name} is very high ({value:.2f})")
        
        # Check for potentially problematic combinations
        slot1_engine = parameters.get("slot1_engine", 0)
        slot2_engine = parameters.get("slot2_engine", 0)
        
        if slot1_engine == 2 and slot2_engine == 2:  # Both tape echo
            warnings.append("Double delay effects may cause timing issues")
        
        if slot1_engine == 3 and slot2_engine == 3:  # Both reverb
            warnings.append("Double reverb may cause excessive wash")
        
        return warnings
    
    def _clamp(self, value: float, min_val: float, max_val: float) -> float:
        """Clamp a value between min and max"""
        return max(min_val, min(value, max_val))
    
    def _create_safe_default(self) -> Dict[str, Any]:
        """Create a safe default preset"""
        return {
            "name": "Safe Default",
            "vibe": "neutral",
            "source": "alchemist_default",
            "parameters": {
                # Slot 1 - K-Style Overdrive with moderate settings
                "slot1_engine": 1,
                "slot1_bypass": 0.0,
                "slot1_param1": 0.3,  # Drive
                "slot1_param2": 0.5,  # Tone
                "slot1_param3": 0.7,  # Level
                "slot1_param4": 0.5,
                "slot1_param5": 0.5,
                "slot1_param6": 0.5,
                "slot1_param7": 0.5,
                "slot1_param8": 0.5,
                "slot1_param9": 0.5,
                "slot1_param10": 0.5,
                
                # Slot 2 - Bypassed
                "slot2_engine": 0,
                "slot2_bypass": 1.0,
                "slot2_param1": 0.5,
                "slot2_param2": 0.5,
                "slot2_param3": 0.5,
                "slot2_param4": 0.5,
                "slot2_param5": 0.5,
                "slot2_param6": 0.5,
                "slot2_param7": 0.5,
                "slot2_param8": 0.5,
                "slot2_param9": 0.5,
                "slot2_param10": 0.5
            },
            "alchemist_validated": True,
            "validation_warnings": []
        }