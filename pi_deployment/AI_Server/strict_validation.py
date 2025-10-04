#!/usr/bin/env python3
"""
Strict Validation Module for Trinity Pipeline
Ensures all engine IDs are valid and requirements are met
"""

import json
import logging
from typing import Dict, List, Any, Optional, Set

logger = logging.getLogger(__name__)

class StrictValidator:
    def __init__(self):
        """Initialize with complete knowledge base"""
        # Load the COMPLETE knowledge base
        try:
            with open("trinity_engine_knowledge_COMPLETE.json", "r") as f:
                self.knowledge = json.load(f)
            logger.info(f"✅ Validator loaded COMPLETE knowledge for {len(self.knowledge['engines'])} engines")
        except FileNotFoundError:
            logger.error("Complete knowledge base not found!")
            raise
        
        self.valid_engine_ids = set(int(eid) for eid in self.knowledge["engines"].keys())
        self.engine_names = {
            int(eid): engine["name"] 
            for eid, engine in self.knowledge["engines"].items()
        }
        self.selection_rules = self.knowledge.get("engine_selection_rules", {})
        
    def validate_preset(self, preset: Dict[str, Any], user_prompt: str = "") -> Dict[str, Any]:
        """
        Strictly validate a preset against all requirements
        Returns validation report with pass/fail and issues
        """
        report = {
            "valid": True,
            "errors": [],
            "warnings": [],
            "fixes_applied": [],
            "score": 100
        }
        
        # 1. Check slot count (must have at least 4 non-empty engines)
        slots = preset.get("slots", [])
        active_slots = [s for s in slots if s.get("engine_id", 0) != 0]
        
        if len(active_slots) < 4:
            report["errors"].append(f"CRITICAL: Only {len(active_slots)} engines used. Minimum 4 required!")
            report["valid"] = False
            report["score"] -= 30
        
        # 2. Validate all engine IDs
        for slot in slots:
            engine_id = slot.get("engine_id", 0)
            if engine_id != 0 and engine_id not in self.valid_engine_ids:
                report["errors"].append(f"INVALID ENGINE ID: {engine_id} does not exist!")
                report["valid"] = False
                report["score"] -= 20
            
            # Check name matches ID
            expected_name = self.engine_names.get(engine_id, "None")
            actual_name = slot.get("engine_name", "")
            if engine_id != 0 and actual_name != expected_name:
                report["warnings"].append(
                    f"Name mismatch: Slot has '{actual_name}' but ID {engine_id} is '{expected_name}'"
                )
                slot["engine_name"] = expected_name  # Fix it
                report["fixes_applied"].append(f"Fixed name for engine {engine_id}")
        
        # 3. Check if requested engines were actually selected
        if user_prompt:
            missing = self.check_requested_engines(preset, user_prompt)
            for engine_info in missing:
                report["errors"].append(
                    f"MISSING REQUESTED ENGINE: User asked for '{engine_info['keyword']}' "
                    f"but {engine_info['name']} (ID {engine_info['id']}) not selected!"
                )
                report["valid"] = False
                report["score"] -= 15
        
        # 4. Check parameter counts
        for slot in slots:
            engine_id = slot.get("engine_id", 0)
            if engine_id == 0:
                continue
                
            engine_data = self.knowledge["engines"].get(str(engine_id), {})
            expected_params = engine_data.get("param_count", 15)
            actual_params = len(slot.get("parameters", []))
            
            if actual_params != expected_params:
                report["warnings"].append(
                    f"Engine {engine_id} has {actual_params} params, expected {expected_params}"
                )
                report["score"] -= 5
        
        # 5. Check for exact 6 slots (plugin requirement)
        if len(slots) != 6:
            report["errors"].append(f"Must have exactly 6 slots, found {len(slots)}")
            report["valid"] = False
            report["score"] -= 10
        
        # Final score
        report["score"] = max(0, report["score"])
        
        return report
    
    def check_requested_engines(self, preset: Dict[str, Any], user_prompt: str) -> List[Dict[str, Any]]:
        """
        Check if engines requested in prompt were actually selected
        Returns list of missing engines
        """
        prompt_lower = user_prompt.lower()
        missing = []
        
        # Get engine IDs in preset
        preset_engine_ids = set()
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            if engine_id != 0:
                preset_engine_ids.add(engine_id)
        
        # Check each selection rule
        for rule_name, rule_data in self.selection_rules.items():
            for keyword in rule_data.get("keywords", []):
                if keyword in prompt_lower:
                    required_id = rule_data["engine_id"]
                    if required_id not in preset_engine_ids:
                        missing.append({
                            "id": required_id,
                            "name": rule_data["engine_name"],
                            "keyword": keyword
                        })
                    break  # Found keyword, don't check others for this rule
        
        return missing
    
    def enforce_minimum_engines(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Ensure preset has at least 4 active engines
        Add sensible defaults if needed
        """
        slots = preset.get("slots", [])
        active_count = sum(1 for s in slots if s.get("engine_id", 0) != 0)
        
        if active_count < 4:
            logger.warning(f"Only {active_count} engines, adding defaults to meet minimum")
            
            # Default engines to add for a well-rounded preset
            defaults = [
                (7, "Parametric EQ"),      # Always useful
                (1, "Classic Compressor"),  # Dynamics control
                (39, "Plate Reverb"),       # Space
                (54, "Gain Utility")        # Level control
            ]
            
            # Find empty slots and fill them
            for engine_id, engine_name in defaults:
                if active_count >= 4:
                    break
                    
                # Find first empty slot
                for slot in slots:
                    if slot.get("engine_id", 0) == 0:
                        # Fill with default
                        slot["engine_id"] = engine_id
                        slot["engine_name"] = engine_name
                        
                        # Add default parameters
                        engine_data = self.knowledge["engines"].get(str(engine_id), {})
                        param_count = engine_data.get("param_count", 15)
                        slot["parameters"] = [
                            {"name": f"param{i+1}", "value": 0.5}
                            for i in range(param_count)
                        ]
                        
                        active_count += 1
                        logger.info(f"Added default engine: {engine_name} (ID {engine_id})")
                        break
        
        return preset
    
    def validate_reasoning_vs_selection(self, reasoning: str, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Check if the AI's reasoning matches what it actually selected
        """
        report = {
            "matches": True,
            "mismatches": []
        }
        
        # Extract engine names mentioned in reasoning
        reasoning_lower = reasoning.lower()
        mentioned_engines = []
        
        for eid, engine in self.knowledge["engines"].items():
            engine_name_lower = engine["name"].lower()
            if engine_name_lower in reasoning_lower:
                mentioned_engines.append({
                    "id": int(eid),
                    "name": engine["name"]
                })
        
        # Get actually selected engines
        selected_engines = []
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            if engine_id != 0:
                selected_engines.append({
                    "id": engine_id,
                    "name": slot.get("engine_name", "Unknown")
                })
        
        # Check for mismatches
        mentioned_ids = {e["id"] for e in mentioned_engines}
        selected_ids = {e["id"] for e in selected_engines}
        
        # Engines mentioned but not selected
        for engine in mentioned_engines:
            if engine["id"] not in selected_ids:
                report["mismatches"].append(
                    f"Mentioned {engine['name']} in reasoning but didn't select it"
                )
                report["matches"] = False
        
        # Engines selected but not mentioned
        for engine in selected_engines:
            if engine["id"] not in mentioned_ids:
                # This is less critical - AI might select without mentioning
                report["mismatches"].append(
                    f"Selected {engine['name']} but didn't mention in reasoning"
                )
        
        return report

def test_validator():
    """Test the strict validator"""
    validator = StrictValidator()
    
    # Test preset with issues
    test_preset = {
        "name": "Test Preset",
        "slots": [
            {
                "slot": 0,
                "engine_id": 42,  # Shimmer Reverb
                "engine_name": "Shimmer Reverb",
                "parameters": [{"name": f"p{i}", "value": 0.5} for i in range(15)]
            },
            {
                "slot": 1,
                "engine_id": 999,  # INVALID ID!
                "engine_name": "Fake Engine",
                "parameters": []
            },
            {
                "slot": 2,
                "engine_id": 0,  # Empty
                "engine_name": "None",
                "parameters": []
            }
        ]
    }
    
    # Only 3 slots, only 1 valid engine - should fail validation
    print("Testing validator with problematic preset...")
    report = validator.validate_preset(test_preset, "shimmer reverb with spring reverb")
    
    print(f"\n✅ Valid: {report['valid']}")
    print(f"📊 Score: {report['score']}/100")
    
    if report["errors"]:
        print("\n❌ Errors:")
        for error in report["errors"]:
            print(f"  • {error}")
    
    if report["warnings"]:
        print("\n⚠️ Warnings:")
        for warning in report["warnings"]:
            print(f"  • {warning}")
    
    if report["fixes_applied"]:
        print("\n🔧 Fixes Applied:")
        for fix in report["fixes_applied"]:
            print(f"  • {fix}")
    
    # Test enforcement
    print("\n\nTesting minimum engine enforcement...")
    fixed_preset = validator.enforce_minimum_engines(test_preset)
    active_count = sum(1 for s in fixed_preset["slots"] if s.get("engine_id", 0) != 0)
    print(f"Active engines after enforcement: {active_count}")

if __name__ == "__main__":
    test_validator()