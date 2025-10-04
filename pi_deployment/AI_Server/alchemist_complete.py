#!/usr/bin/env python3
"""
Complete Alchemist Component with Proper Validation
Final validation and safety checks with full knowledge
"""

import json
import logging
from typing import Dict, List, Any, Optional, Tuple

logger = logging.getLogger(__name__)

class CompleteAlchemist:
    def __init__(self):
        """Initialize with complete engine knowledge"""
        # Load the COMPLETE knowledge base - SINGLE SOURCE OF TRUTH
        try:
            with open("trinity_engine_knowledge_COMPLETE.json", "r") as f:
                self.knowledge = json.load(f)
            logger.info(f"✅ Alchemist loaded COMPLETE knowledge for {len(self.knowledge['engines'])} engines")
        except FileNotFoundError:
            try:
                with open("trinity_engine_knowledge.json", "r") as f:
                    self.knowledge = json.load(f)
                logger.warning("⚠️ Using regular knowledge base - complete version preferred")
            except FileNotFoundError:
                logger.error("No knowledge base found!")
                raise
        
        self.engines = self.knowledge["engines"]
        
        # Safety limits
        self.safety_limits = {
            "max_feedback": 0.95,  # Prevent runaway feedback
            "max_resonance": 0.95,  # Prevent self-oscillation
            "max_gain": 0.95,      # Prevent clipping
            "min_threshold": 0.05,  # Ensure gates/compressors work
            "max_mix_sum": 3.0     # Total wetness limit
        }
    
    def validate_and_fix(self, preset: Dict[str, Any]) -> Tuple[Dict[str, Any], Dict[str, Any]]:
        """
        Validate preset and fix any issues
        Returns: (validated_preset, validation_report)
        """
        logger.info(f"Validating preset: {preset.get('name', 'Unnamed')}")
        
        # Deep copy to avoid modifying original
        validated = json.loads(json.dumps(preset))
        
        # Validation report
        report = {
            "valid": True,
            "errors": [],
            "warnings": [],
            "fixes": [],
            "score": 100.0
        }
        
        # 1. Validate basic structure
        validated, report = self.validate_structure(validated, report)
        
        # 2. Validate engine IDs and parameter counts
        validated, report = self.validate_engines(validated, report)
        
        # 3. Validate parameter ranges and safety
        validated, report = self.validate_parameters(validated, report)
        
        # 4. Validate signal flow
        validated, report = self.validate_signal_flow(validated, report)
        
        # 5. Validate mix levels
        validated, report = self.validate_mix_levels(validated, report)
        
        # 6. Final safety checks
        validated, report = self.perform_safety_checks(validated, report)
        
        # Calculate final score
        report["score"] = max(0, report["score"])
        report["valid"] = len(report["errors"]) == 0
        
        logger.info(f"Validation complete. Score: {report['score']:.1f}, Valid: {report['valid']}")
        
        return validated, report
    
    def validate_structure(self, preset: Dict[str, Any], report: Dict) -> Tuple[Dict, Dict]:
        """Validate basic preset structure"""
        
        # Check required fields
        if "name" not in preset:
            preset["name"] = "Unnamed Preset"
            report["fixes"].append("Added missing name")
            report["score"] -= 5
        
        if "description" not in preset:
            preset["description"] = "AI generated preset"
            report["fixes"].append("Added missing description")
            report["score"] -= 2
        
        if "slots" not in preset:
            preset["slots"] = []
            report["errors"].append("No slots defined")
            report["score"] -= 50
        
        # Check slot count
        if len(preset.get("slots", [])) > 6:
            preset["slots"] = preset["slots"][:6]
            report["warnings"].append(f"Truncated to 6 slots")
            report["score"] -= 10
        
        return preset, report
    
    def validate_engines(self, preset: Dict[str, Any], report: Dict) -> Tuple[Dict, Dict]:
        """Validate engine IDs and parameter counts"""
        
        valid_slots = []
        
        for i, slot in enumerate(preset.get("slots", [])):
            # Check engine ID
            engine_id = slot.get("engine_id")
            
            if engine_id is None:
                report["errors"].append(f"Slot {i}: Missing engine_id")
                report["score"] -= 20
                continue
            
            if engine_id < 0 or engine_id >= 57:
                report["errors"].append(f"Slot {i}: Invalid engine_id {engine_id}")
                report["score"] -= 20
                continue
            
            # Get engine data
            engine = self.engines.get(str(engine_id), {})
            
            if not engine and engine_id != 0:
                report["warnings"].append(f"Slot {i}: Unknown engine {engine_id}")
                report["score"] -= 10
            
            # Fix slot index
            slot["slot"] = len(valid_slots)
            
            # Fix engine name
            expected_name = engine.get("name", "None" if engine_id == 0 else f"Engine {engine_id}")
            if slot.get("engine_name") != expected_name:
                slot["engine_name"] = expected_name
                report["fixes"].append(f"Fixed engine name for slot {i}")
            
            # Validate parameter count
            expected_params = engine.get("param_count", 0)
            actual_params = len(slot.get("parameters", []))
            
            if actual_params != expected_params:
                report["errors"].append(
                    f"Slot {i}: Engine {engine_id} has {actual_params} params, expected {expected_params}"
                )
                report["score"] -= 15
                
                # Fix parameter count
                slot["parameters"] = self.fix_parameter_count(slot, expected_params, engine)
                report["fixes"].append(f"Fixed parameter count for slot {i}")
            
            # Validate parameter format
            for j, param in enumerate(slot.get("parameters", [])):
                if not isinstance(param, dict):
                    report["errors"].append(f"Slot {i}, Param {j}: Invalid format")
                    report["score"] -= 5
                elif "name" not in param or "value" not in param:
                    report["errors"].append(f"Slot {i}, Param {j}: Missing name or value")
                    report["score"] -= 5
                elif param.get("name") != f"param{j+1}":
                    param["name"] = f"param{j+1}"
                    report["fixes"].append(f"Fixed param name in slot {i}")
            
            valid_slots.append(slot)
        
        # Ensure exactly 6 slots for plugin compatibility
        while len(valid_slots) < 6:
            valid_slots.append({
                "slot": len(valid_slots),
                "engine_id": 0,
                "engine_name": "None",
                "parameters": []
            })
        
        preset["slots"] = valid_slots
        return preset, report
    
    def validate_parameters(self, preset: Dict[str, Any], report: Dict) -> Tuple[Dict, Dict]:
        """Validate parameter ranges and apply safety limits"""
        
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            engine = self.engines.get(str(engine_id), {})

            for i, param in enumerate(slot.get("parameters", [])):
                # Handle both dict and float parameter formats
                if isinstance(param, dict):
                    value = param.get("value", 0.5)
                else:
                    value = float(param)

                # Clamp to valid range
                if value < 0.0 or value > 1.0:
                    clamped = max(0.0, min(1.0, value))
                    if isinstance(param, dict):
                        param["value"] = clamped
                    else:
                        slot["parameters"][i] = clamped
                    report["fixes"].append(f"Clamped parameter value in slot {slot['slot']}")
                    report["score"] -= 2
                
                # Apply safety limits for specific parameters
                param_info = engine.get("parameters", [{}])[i] if i < len(engine.get("parameters", [])) else {}
                param_name = param_info.get("name", "").lower()

                # Check feedback parameters
                if "feedback" in param_name:
                    if value > self.safety_limits["max_feedback"]:
                        if isinstance(param, dict):
                            param["value"] = self.safety_limits["max_feedback"]
                        else:
                            slot["parameters"][i] = self.safety_limits["max_feedback"]
                        report["warnings"].append(f"Limited feedback in slot {slot['slot']}")
                        report["fixes"].append(f"Reduced feedback to safe level")

                # Check resonance parameters
                elif "resonance" in param_name or "q" in param_name:
                    if value > self.safety_limits["max_resonance"]:
                        if isinstance(param, dict):
                            param["value"] = self.safety_limits["max_resonance"]
                        else:
                            slot["parameters"][i] = self.safety_limits["max_resonance"]
                        report["warnings"].append(f"Limited resonance in slot {slot['slot']}")
                        report["fixes"].append(f"Reduced resonance to safe level")

                # Check threshold parameters
                elif "threshold" in param_name:
                    if value < self.safety_limits["min_threshold"]:
                        if isinstance(param, dict):
                            param["value"] = self.safety_limits["min_threshold"]
                        else:
                            slot["parameters"][i] = self.safety_limits["min_threshold"]
                        report["fixes"].append(f"Raised threshold to minimum")
        
        return preset, report
    
    def validate_signal_flow(self, preset: Dict[str, Any], report: Dict) -> Tuple[Dict, Dict]:
        """Validate signal chain order"""
        
        slots = preset.get("slots", [])
        if len(slots) < 2:
            return preset, report  # No flow to validate
        
        # Check for proper ordering
        positions = []
        for slot in slots:
            engine_id = slot.get("engine_id", 0)
            engine = self.engines.get(str(engine_id), {})
            pos = engine.get("signal_chain_position", 5)
            positions.append(pos)
        
        # Count inversions (out of order elements)
        inversions = 0
        for i in range(len(positions) - 1):
            if positions[i] > positions[i + 1]:
                inversions += 1
        
        if inversions > len(slots) // 2:
            report["warnings"].append(f"Signal chain has {inversions} order issues")
            report["score"] -= inversions * 2
            # Don't auto-reorder in Alchemist, that's Calculator's job
        
        return preset, report
    
    def validate_mix_levels(self, preset: Dict[str, Any], report: Dict) -> Tuple[Dict, Dict]:
        """Validate mix parameter levels"""
        
        total_wetness = 0.0
        
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            engine = self.engines.get(str(engine_id), {})
            mix_index = engine.get("mix_param_index", -1)
            
            if mix_index >= 0 and mix_index < len(slot.get("parameters", [])):
                mix_value = slot["parameters"][mix_index]["value"]
                total_wetness += mix_value
                
                # Check for specific issues
                category = engine.get("category", "")
                
                if category == "Reverb" and mix_value > 0.7:
                    report["warnings"].append(f"High reverb mix in slot {slot['slot']}")
                    if mix_value > 0.9:
                        slot["parameters"][mix_index]["value"] = 0.7
                        report["fixes"].append("Reduced excessive reverb mix")
                        report["score"] -= 5
                
                elif category == "Delay" and mix_value > 0.6:
                    report["warnings"].append(f"High delay mix in slot {slot['slot']}")
        
        # Check total wetness
        if total_wetness > self.safety_limits["max_mix_sum"]:
            report["warnings"].append(f"Total mix level very high: {total_wetness:.2f}")
            report["score"] -= 10
        
        return preset, report
    
    def perform_safety_checks(self, preset: Dict[str, Any], report: Dict) -> Tuple[Dict, Dict]:
        """Perform final safety checks"""
        
        slots = preset.get("slots", [])
        engine_ids = [slot.get("engine_id", 0) for slot in slots]
        
        # Check for multiple pitch shifters (can cause artifacts)
        pitch_engines = [31, 32, 33]  # Pitch Shifter, Detune Doubler, Harmonizer
        pitch_count = sum(1 for eid in engine_ids if eid in pitch_engines)
        if pitch_count > 2:
            report["warnings"].append(f"Multiple pitch shifters ({pitch_count}) may cause artifacts")
            report["score"] -= 10
        
        # Check for multiple heavy reverbs
        reverb_engines = [39, 40, 41, 42, 43]
        reverb_count = sum(1 for eid in engine_ids if eid in reverb_engines)
        if reverb_count > 2:
            report["warnings"].append(f"Multiple reverbs ({reverb_count}) may sound muddy")
            report["score"] -= 10
        
        # Check CPU load
        heavy_count = 0
        for eid in engine_ids:
            engine = self.engines.get(str(eid), {})
            if engine.get("cpu_load") == "heavy":
                heavy_count += 1
        
        if heavy_count > 2:
            report["warnings"].append(f"High CPU load: {heavy_count} heavy effects")
            report["score"] -= 5
        
        # Verify at least one engine is active
        if len(engine_ids) == 0 or all(eid == 0 for eid in engine_ids):
            report["errors"].append("No active engines in preset")
            report["score"] -= 50
        
        return preset, report
    
    def fix_parameter_count(self, slot: Dict, expected: int, engine: Dict) -> List[Dict]:
        """Fix parameter count for a slot"""
        current_params = slot.get("parameters", [])
        fixed_params = []
        
        for i in range(expected):
            param_name = f"param{i+1}"
            
            # Try to find existing value
            value = 0.5  # Default
            for p in current_params:
                if isinstance(p, dict) and p.get("name") == param_name:
                    value = p.get("value", 0.5)
                    break
                elif isinstance(p, (int, float)) and i < len(current_params):
                    value = float(p)  # Use p, not current_params[i]
                    break

            # Use engine default if available
            if i < len(engine.get("parameters", [])):
                param_info = engine["parameters"][i]
                if value == 0.5:  # Still default
                    value = param_info.get("default", 0.5)

            # Ensure value is a number, not a dict
            if isinstance(value, dict):
                value = value.get("value", 0.5) if "value" in value else 0.5

            fixed_params.append({
                "name": param_name,
                "value": max(0.0, min(1.0, float(value)))
            })
        
        return fixed_params
    
    def format_for_plugin(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Format the final preset EXACTLY as the plugin expects
        This is the final step before sending to plugin
        """
        formatted = {
            "name": preset.get("name", "Unnamed"),
            "description": preset.get("description", ""),
            "slots": []
        }
        
        for slot in preset.get("slots", []):
            formatted_slot = {
                "slot": slot.get("slot", 0),
                "engine_id": slot.get("engine_id", 0),
                "engine_name": slot.get("engine_name", "None"),
                "parameters": []
            }
            
            # CRITICAL: Plugin expects EXACTLY 15 parameters per slot
            existing_params = slot.get("parameters", [])

            # Add all existing parameters (handle both dict and float formats)
            for i, param in enumerate(existing_params):
                if isinstance(param, dict):
                    # Dict format: {"name": "param1", "value": 0.5}
                    formatted_slot["parameters"].append({
                        "name": param.get("name", f"param{i+1}"),
                        "value": float(param.get("value", 0.5))
                    })
                else:
                    # Float format: 0.5
                    formatted_slot["parameters"].append({
                        "name": f"param{i+1}",
                        "value": float(param)
                    })
            
            # Pad to 15 parameters if needed
            while len(formatted_slot["parameters"]) < 15:
                param_num = len(formatted_slot["parameters"]) + 1
                formatted_slot["parameters"].append({
                    "name": f"param{param_num}",
                    "value": 0.5
                })
            
            formatted["slots"].append(formatted_slot)
        
        return formatted

def test_alchemist():
    """Test the alchemist component"""
    alchemist = CompleteAlchemist()
    
    # Create a problematic preset to validate
    bad_preset = {
        "name": "Problematic Preset",
        "slots": [
            {
                "slot": 0,
                "engine_id": 39,  # Plate Reverb
                "engine_name": "Wrong Name",  # Wrong name
                "parameters": [  # Wrong count (5 instead of 10)
                    {"name": "p1", "value": 1.5},  # Wrong name and out of range
                    {"name": "param2", "value": 0.5},
                    {"name": "param3", "value": -0.1},  # Out of range
                    {"name": "param4", "value": 0.99},  # Feedback too high
                    {"name": "param5", "value": 0.5}
                ]
            },
            {
                "slot": 1,
                "engine_id": 99,  # Invalid engine
                "parameters": []
            }
        ]
    }
    
    print("Testing problematic preset...")
    validated, report = alchemist.validate_and_fix(bad_preset)
    
    print(f"\nValidation Report:")
    print(f"  Valid: {report['valid']}")
    print(f"  Score: {report['score']:.1f}/100")
    print(f"  Errors: {len(report['errors'])}")
    for error in report["errors"]:
        print(f"    ❌ {error}")
    print(f"  Warnings: {len(report['warnings'])}")
    for warning in report["warnings"]:
        print(f"    ⚠️  {warning}")
    print(f"  Fixes: {len(report['fixes'])}")
    for fix in report["fixes"]:
        print(f"    ✓ {fix}")
    
    # Check the fixed preset
    print(f"\nFixed preset:")
    for slot in validated["slots"]:
        print(f"  Slot {slot['slot']}: {slot['engine_name']} ({len(slot['parameters'])} params)")

if __name__ == "__main__":
    test_alchemist()