#!/usr/bin/env python3
"""
Visionary with ENFORCED engine selection
This version REQUIRES specific engines based on keywords
"""

import json
import logging
from typing import Dict, List, Any, Optional
from visionary_complete import CompleteVisionary

logger = logging.getLogger(__name__)

class EnforcedVisionary(CompleteVisionary):
    """Visionary that ENFORCES engine selection based on keywords"""
    
    def __init__(self):
        super().__init__()
        
        # Load enforcement rules from complete knowledge base
        if "engine_selection_rules" in self.knowledge:
            self.selection_rules = self.knowledge["engine_selection_rules"]
        else:
            # Fallback rules if not in knowledge base
            self.selection_rules = {
                "shimmer_reverb": {
                    "keywords": ["shimmer", "ethereal", "angelic", "celestial"],
                    "engine_id": 42,
                    "engine_name": "Shimmer Reverb"
                },
                "spring_reverb": {
                    "keywords": ["spring", "surf", "vintage reverb", "twangy"],
                    "engine_id": 40,
                    "engine_name": "Spring Reverb"
                },
                "plate_reverb": {
                    "keywords": ["plate", "studio reverb", "classic reverb"],
                    "engine_id": 39,
                    "engine_name": "Plate Reverb"
                },
                "chorus": {
                    "keywords": ["chorus", "ensemble", "thicken"],
                    "engine_id": 23,
                    "engine_name": "Digital Chorus"
                },
                "noise_gate": {
                    "keywords": ["gate", "noise gate", "tight", "clean up"],
                    "engine_id": 4,
                    "engine_name": "Noise Gate"
                }
            }
    
    def get_mandatory_engines(self, prompt: str) -> List[Dict[str, Any]]:
        """Extract MANDATORY engines based on keywords in prompt"""
        prompt_lower = prompt.lower()
        mandatory = []
        
        for rule_name, rule_data in self.selection_rules.items():
            for keyword in rule_data.get("keywords", []):
                if keyword in prompt_lower:
                    mandatory.append({
                        "id": rule_data["engine_id"],
                        "name": rule_data["engine_name"],
                        "reason": f"User requested '{keyword}'"
                    })
                    break  # Found match for this rule, don't check other keywords
        
        return mandatory
    
    def build_generation_prompt(self, user_prompt: str, context: Dict) -> str:
        """Build prompt with STRICT ENFORCEMENT of engine selection"""
        
        # Get MANDATORY engines based on keywords
        mandatory_engines = self.get_mandatory_engines(user_prompt)
        
        # Get suggested engines from context (as before)
        suggested_engines = []
        for ctx in context.get("contexts", []):
            if ctx in self.musical_contexts:
                suggested_engines.extend(self.musical_contexts[ctx]["engines"])
        
        instrument = context.get("instrument")
        if instrument and instrument in self.good_combinations:
            suggested_engines.extend(self.good_combinations[instrument])
        
        # Remove duplicates and exclude mandatory ones from suggestions
        mandatory_ids = [e["id"] for e in mandatory_engines]
        suggested_engines = [e for e in set(suggested_engines) if e not in mandatory_ids][:10]
        
        # Build MANDATORY engine info
        mandatory_info = ""
        if mandatory_engines:
            mandatory_info = "YOU MUST USE THESE ENGINES (user specifically requested them):\n\n"
            for eng in mandatory_engines:
                engine = self.engines.get(str(eng["id"]), {})
                if engine:
                    mandatory_info += f"MANDATORY - Engine {eng['id']}: {eng['name']}\n"
                    mandatory_info += f"  Reason: {eng['reason']}\n"
                    mandatory_info += f"  Category: {engine['category']}\n"
                    mandatory_info += f"  Parameters: {engine['param_count']}\n"
                    mandatory_info += f"  Description: {engine.get('function', '')}\n\n"
        
        # Build suggested engine info
        suggested_info = "Additional suggested engines for this context:\n\n"
        for engine_id in suggested_engines:
            engine = self.engines.get(str(engine_id), {})
            if engine:
                suggested_info += f"Engine {engine_id}: {engine['name']}\n"
                suggested_info += f"  Category: {engine['category']}\n"
                suggested_info += f"  Parameters: {engine['param_count']}\n\n"
        
        # Count mandatory engines to ensure we meet minimum
        min_engines_needed = max(4, len(mandatory_engines))
        
        prompt = f"""Create a preset for: "{user_prompt}"

CRITICAL REQUIREMENTS:
1. YOU MUST include ALL mandatory engines listed below
2. YOU MUST use at least {min_engines_needed} engines total (minimum 4)
3. YOU MUST follow proper signal chain order (EQ/Filter -> Dynamics -> Distortion -> Modulation -> Time/Delay -> Reverb)
4. YOU MUST set parameters appropriate to the request

{mandatory_info}

{suggested_info}

Context detected:
- Musical contexts: {context.get('contexts', [])}
- Instrument: {context.get('instrument', 'general')}
- Intensity: {context.get('intensity', 'moderate')}

IMPORTANT: 
- If user mentioned "shimmer", YOU MUST use Engine 42 (Shimmer Reverb)
- If user mentioned "spring", YOU MUST use Engine 40 (Spring Reverb)
- If user mentioned "plate", YOU MUST use Engine 39 (Plate Reverb)
- If user mentioned "chorus", YOU MUST use Engine 23 (Digital Chorus)
- If user mentioned "gate" or "tight", YOU MUST use Engine 4 (Noise Gate)

Create a unique, creative preset name (not just the prompt in title case).

Return JSON with EXACT format including your reasoning:
{{
  "name": "<creative preset name>",
  "description": "What this preset does",
  "reasoning": {{
    "overall_approach": "Your strategy",
    "signal_flow": "Why this order",
    "mandatory_engines": "List which mandatory engines you included and why",
    "slot_reasoning": [
      {{
        "slot": 0,
        "engine": "Engine name",
        "why": "Why this engine",
        "key_params": "Important parameters"
      }}
    ]
  }},
  "slots": [
    {{
      "slot": 0,
      "engine_id": <id>,
      "engine_name": "<exact name>",
      "parameters": [
        {{"name": "param1", "value": <0.0-1.0>}},
        ... // EXACT number of params for this engine
      ]
    }}
    // AT LEAST {min_engines_needed} slots with engines
  ]
}}

Remember: FAILING TO INCLUDE MANDATORY ENGINES IS AN ERROR."""
        
        return prompt
    
    async def generate_complete_preset(self, prompt: str) -> Dict[str, Any]:
        """Generate preset with ENFORCED engine selection"""
        # First check what engines are mandatory
        mandatory = self.get_mandatory_engines(prompt)
        if mandatory:
            logger.info(f"🔒 ENFORCING {len(mandatory)} mandatory engines:")
            for eng in mandatory:
                logger.info(f"   - {eng['name']} (ID {eng['id']}): {eng['reason']}")
        
        # Generate with enforcement
        preset = await super().generate_complete_preset(prompt)
        
        # Validate that mandatory engines were included
        selected_ids = []
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            if engine_id != 0:
                selected_ids.append(engine_id)
        
        # Check if all mandatory engines were selected
        missing = []
        for eng in mandatory:
            if eng["id"] not in selected_ids:
                missing.append(eng)
                logger.error(f"❌ MANDATORY ENGINE NOT SELECTED: {eng['name']} (ID {eng['id']})")
        
        # If any mandatory engines missing, forcibly add them
        if missing:
            logger.warning(f"⚠️ Forcibly adding {len(missing)} missing mandatory engines")
            
            # Find empty slots or make room
            slots = preset.get("slots", [])
            for eng in missing:
                # Find first empty slot
                added = False
                for slot in slots:
                    if slot.get("engine_id", 0) == 0:
                        # Fill this empty slot
                        engine_data = self.engines.get(str(eng["id"]), {})
                        param_count = engine_data.get("param_count", 15)
                        
                        slot["engine_id"] = eng["id"]
                        slot["engine_name"] = eng["name"]
                        # Set reasonable default parameters
                        slot["parameters"] = []
                        for p in range(15):
                            if p < param_count:
                                # Real parameter - set to moderate value
                                value = 0.5
                                if p == engine_data.get("mix_param_index", -1):
                                    value = 0.4  # Moderate mix
                            else:
                                # Padding parameter
                                value = 0.5
                            
                            slot["parameters"].append({
                                "name": f"param{p+1}",
                                "value": value
                            })
                        
                        logger.info(f"   Added {eng['name']} to slot {slot['slot']}")
                        added = True
                        break
                
                if not added:
                    logger.error(f"   Could not add {eng['name']} - no empty slots!")
        
        # Ensure minimum 4 engines
        active_count = sum(1 for s in preset.get("slots", []) if s.get("engine_id", 0) != 0)
        if active_count < 4:
            logger.warning(f"⚠️ Only {active_count} engines, need at least 4")
            # Add some defaults
            defaults = [
                (7, "Parametric EQ"),
                (1, "Classic Compressor"),
                (39, "Plate Reverb"),
                (54, "Gain Utility")
            ]
            
            for engine_id, engine_name in defaults:
                if active_count >= 4:
                    break
                
                # Check if not already in preset
                if engine_id not in selected_ids:
                    # Find empty slot
                    for slot in preset.get("slots", []):
                        if slot.get("engine_id", 0) == 0:
                            engine_data = self.engines.get(str(engine_id), {})
                            param_count = engine_data.get("param_count", 15)
                            
                            slot["engine_id"] = engine_id
                            slot["engine_name"] = engine_name
                            slot["parameters"] = [
                                {"name": f"param{i+1}", "value": 0.5}
                                for i in range(15)
                            ]
                            
                            active_count += 1
                            logger.info(f"   Added default: {engine_name}")
                            break
        
        return preset

def test_enforcer():
    """Test the enforced visionary"""
    import asyncio
    
    enforcer = EnforcedVisionary()
    
    # Test with specific engine requests
    test_prompts = [
        "ambient pad with shimmer reverb and spring reverb",
        "metal guitar with noise gate and distortion",
        "warm vocal with plate reverb and chorus"
    ]
    
    for prompt in test_prompts:
        print(f"\nTesting: {prompt}")
        mandatory = enforcer.get_mandatory_engines(prompt)
        print(f"Mandatory engines: {len(mandatory)}")
        for eng in mandatory:
            print(f"  - {eng['name']} (ID {eng['id']}): {eng['reason']}")

if __name__ == "__main__":
    test_enforcer()