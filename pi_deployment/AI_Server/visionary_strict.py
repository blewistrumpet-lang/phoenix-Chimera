#!/usr/bin/env python3
"""
STRICT Visionary Component - Enforces Single Source of Truth
Uses ONLY the complete knowledge base with strict validation
"""

import json
import logging
from typing import Dict, List, Any, Optional
from openai import AsyncOpenAI
import asyncio
import os
from dotenv import load_dotenv

load_dotenv()

logger = logging.getLogger(__name__)

class StrictVisionary:
    def __init__(self):
        """Initialize with COMPLETE engine knowledge"""
        api_key = os.getenv("OPENAI_API_KEY")
        if api_key:
            self.client = AsyncOpenAI(api_key=api_key)
            logger.info(f"✅ OpenAI client initialized")
        else:
            self.client = None
            logger.warning("⚠️ No OpenAI API key found")
        
        # Load the COMPLETE knowledge base - SINGLE SOURCE OF TRUTH
        try:
            with open("trinity_engine_knowledge_COMPLETE.json", "r") as f:
                self.knowledge = json.load(f)
            logger.info(f"Loaded COMPLETE knowledge base with {len(self.knowledge['engines'])} engines")
        except FileNotFoundError:
            # Fall back to regular if complete doesn't exist yet
            with open("trinity_engine_knowledge.json", "r") as f:
                self.knowledge = json.load(f)
            logger.warning("Using regular knowledge base - complete version not found")
        
        self.engines = self.knowledge["engines"]
        self.selection_rules = self.knowledge.get("engine_selection_rules", {})
        self.ai_instructions = self.knowledge.get("ai_instructions", {})
        
        # Build reverse lookup for engine selection
        self.keyword_to_engine = {}
        for rule_name, rule_data in self.selection_rules.items():
            for keyword in rule_data.get("keywords", []):
                self.keyword_to_engine[keyword.lower()] = {
                    "id": rule_data["engine_id"],
                    "name": rule_data["engine_name"]
                }
        
    def create_strict_system_prompt(self) -> str:
        """Create STRICT system prompt that enforces single source of truth"""
        
        # Build complete engine list for reference
        engine_list = []
        for eid, engine in self.engines.items():
            engine_list.append(f"ID {eid}: {engine['name']} - {engine.get('function', 'Audio processing')}")
        
        return f"""You are the STRICT Visionary component of the Trinity Pipeline.

CRITICAL RULES - THESE ARE ABSOLUTE AND CANNOT BE VIOLATED:

1. ONLY use engine IDs from this exact list (0-56):
{chr(10).join(engine_list[:10])}  # Show first 10 as example
[... and {len(engine_list)-10} more engines]

2. NEVER invent or describe engines that don't exist
   ❌ WRONG: "Modulation - Chorus" 
   ✅ RIGHT: "Digital Chorus" (ID 23)

3. MANDATORY engine mappings - when user requests:
   - "shimmer" or "ethereal" → MUST use Engine 42 (Shimmer Reverb)
   - "spring reverb" → MUST use Engine 40 (Spring Reverb)  
   - "plate reverb" → MUST use Engine 39 (Plate Reverb)
   - "noise gate" or "gate" → MUST use Engine 4 (Noise Gate)
   - "chorus" → MUST use Engine 23 (Digital Chorus)
   - "phaser" → MUST use Engine 24 (Analog Phaser)

4. MINIMUM 4 engines per preset (fill remaining slots intelligently)

5. Your reasoning must EXACTLY match your selections:
   - If you say "Spring Reverb", you MUST select engine_id: 40
   - If you say "Noise Gate", you MUST select engine_id: 4

6. Signal chain order (follow strictly):
   Gate → EQ → Compression → Distortion → Modulation → Pitch → Delay → Reverb → Utility

7. Each engine has EXACT parameter counts - respect them:
   - Noise Gate (4): 5 parameters
   - Spring Reverb (40): 9 parameters
   - Shimmer Reverb (42): 12 parameters

VALIDATION: Any preset that violates these rules will be REJECTED."""
    
    def analyze_and_enforce_request(self, prompt: str) -> Dict[str, Any]:
        """Analyze prompt and ENFORCE specific engine selections"""
        prompt_lower = prompt.lower()
        
        # MANDATORY engines based on keywords
        mandatory_engines = []
        
        # Check each selection rule
        for keyword, engine_info in self.keyword_to_engine.items():
            if keyword in prompt_lower:
                mandatory_engines.append({
                    "id": engine_info["id"],
                    "name": engine_info["name"],
                    "reason": f"User requested '{keyword}'"
                })
        
        # Additional mandatory selections based on context
        if "gate" in prompt_lower or "tight" in prompt_lower:
            if not any(e["id"] == 4 for e in mandatory_engines):
                mandatory_engines.append({
                    "id": 4,
                    "name": "Noise Gate",
                    "reason": "Gating requested"
                })
        
        if "parallel compression" in prompt_lower:
            mandatory_engines.append({
                "id": 2,
                "name": "Classic Compressor",
                "reason": "Parallel compression (set mix to 40-50%)"
            })
        
        # Ensure we have at least 4 engines
        if len(mandatory_engines) < 4:
            # Add complementary engines based on what we have
            has_reverb = any(e["id"] in [39, 40, 41, 42, 43] for e in mandatory_engines)
            has_eq = any(e["id"] in [7, 8, 9, 10, 11, 12] for e in mandatory_engines)
            has_dynamics = any(e["id"] in [1, 2, 3, 4, 5] for e in mandatory_engines)
            
            if not has_eq:
                mandatory_engines.append({
                    "id": 7,
                    "name": "Parametric EQ",
                    "reason": "Tonal shaping (always beneficial)"
                })
            
            if not has_dynamics and "aggressive" not in prompt_lower:
                mandatory_engines.append({
                    "id": 1,
                    "name": "Vintage Opto",
                    "reason": "Smooth dynamics control"
                })
            
            if not has_reverb and "dry" not in prompt_lower:
                mandatory_engines.append({
                    "id": 39,
                    "name": "Plate Reverb",
                    "reason": "Spatial depth (subtle mix)"
                })
        
        return {
            "mandatory_engines": mandatory_engines,
            "min_engines": max(4, len(mandatory_engines)),
            "character": self.detect_character(prompt_lower)
        }
    
    def detect_character(self, prompt_lower: str) -> str:
        """Detect the character of the requested sound"""
        if any(word in prompt_lower for word in ["warm", "vintage", "analog", "tube"]):
            return "warm"
        elif any(word in prompt_lower for word in ["aggressive", "heavy", "metal", "brutal"]):
            return "aggressive"
        elif any(word in prompt_lower for word in ["clean", "transparent", "pristine"]):
            return "clean"
        elif any(word in prompt_lower for word in ["ambient", "ethereal", "space", "atmospheric"]):
            return "spacious"
        else:
            return "balanced"
    
    def build_strict_generation_prompt(self, user_prompt: str, requirements: Dict) -> str:
        """Build generation prompt with STRICT requirements"""
        
        # Build mandatory engine list
        mandatory_list = "\n".join([
            f"- MUST include Engine {e['id']} ({e['name']}) because: {e['reason']}"
            for e in requirements["mandatory_engines"]
        ])
        
        # Get complete engine info for mandatory engines
        engine_details = []
        for eng in requirements["mandatory_engines"]:
            engine_data = self.engines.get(str(eng["id"]), {})
            if engine_data:
                engine_details.append(f"""
Engine {eng['id']}: {eng['name']}
  Function: {engine_data.get('function', 'Processing')}
  Character: {engine_data.get('character', 'Standard')}
  Parameters: {engine_data.get('param_count', 0)}
  Mix param at index: {engine_data.get('mix_param_index', -1)}""")
        
        return f"""Create a preset for: "{user_prompt}"

MANDATORY REQUIREMENTS (MUST BE FOLLOWED):
{mandatory_list}

Minimum engines required: {requirements['min_engines']}
Character detected: {requirements['character']}

MANDATORY ENGINE DETAILS:
{''.join(engine_details)}

STRICT RULES:
1. You MUST include ALL mandatory engines listed above
2. You MUST use AT LEAST {requirements['min_engines']} engines total
3. You can ONLY use engine IDs 0-56 from the knowledge base
4. Your reasoning must EXACTLY match your engine selections
5. Fill remaining slots intelligently based on the character

Return JSON with this EXACT structure:
{{
  "name": "<creative preset name>",
  "description": "What this preset does",
  "reasoning": {{
    "overall_approach": "How you're achieving the requested sound",
    "mandatory_fulfillment": "How you included all mandatory engines",
    "signal_flow": "Why this order makes sense",
    "slot_reasoning": [
      {{
        "slot": 0,
        "engine_id": <exact ID from knowledge base>,
        "engine_name": "<exact name from knowledge base>",
        "why": "Why this engine in this position",
        "key_params": "Important parameter settings"
      }}
    ]
  }},
  "slots": [
    {{
      "slot": 0,
      "engine_id": <ID from 0-56 ONLY>,
      "engine_name": "<exact name from knowledge base>",
      "parameters": [
        {{"name": "param1", "value": <0.0-1.0>}},
        ... // EXACT parameter count for this engine
      ]
    }}
    // Continue for at least {requirements['min_engines']} engines
  ]
}}

REMEMBER: Violating these rules = REJECTION. Follow them exactly."""
    
    async def generate_strict_preset(self, prompt: str) -> Dict[str, Any]:
        """Generate preset with STRICT enforcement"""
        logger.info(f"🔒 STRICT generation for: '{prompt}'")
        
        # Analyze and enforce requirements
        requirements = self.analyze_and_enforce_request(prompt)
        logger.info(f"   Mandatory engines: {len(requirements['mandatory_engines'])}")
        logger.info(f"   Minimum required: {requirements['min_engines']}")
        
        if not self.client:
            return self.create_strict_fallback(prompt, requirements)
        
        try:
            # Generate with strict requirements
            generation_prompt = self.build_strict_generation_prompt(prompt, requirements)
            
            response = await self.client.chat.completions.create(
                model="gpt-3.5-turbo",
                messages=[
                    {"role": "system", "content": self.create_strict_system_prompt()},
                    {"role": "user", "content": generation_prompt}
                ],
                response_format={"type": "json_object"},
                temperature=0.5,  # Lower temperature for more consistent rule following
                max_tokens=3000
            )
            
            preset = json.loads(response.choices[0].message.content)
            
            # VALIDATE that mandatory engines are included
            preset = self.validate_and_enforce(preset, requirements)
            
            logger.info(f"✅ STRICT preset generated: {preset.get('name', 'Unnamed')}")
            return preset
            
        except Exception as e:
            logger.error(f"Generation failed: {e}")
            return self.create_strict_fallback(prompt, requirements)
    
    def validate_and_enforce(self, preset: Dict, requirements: Dict) -> Dict:
        """Validate and ENFORCE that all requirements are met"""
        
        # Check if all mandatory engines are present
        preset_engine_ids = [slot.get("engine_id") for slot in preset.get("slots", [])]
        mandatory_ids = [e["id"] for e in requirements["mandatory_engines"]]
        
        missing = [mid for mid in mandatory_ids if mid not in preset_engine_ids]
        
        if missing:
            logger.warning(f"⚠️ Missing mandatory engines: {missing}")
            # Add missing engines
            for engine_id in missing:
                engine_data = self.engines.get(str(engine_id), {})
                if engine_data:
                    # Find appropriate slot position
                    position = len([s for s in preset.get("slots", []) if s.get("engine_id", 0) != 0])
                    if position < 6:
                        preset["slots"].append(self.create_engine_slot(engine_id, engine_data, position))
        
        # Ensure minimum engine count
        active_slots = [s for s in preset.get("slots", []) if s.get("engine_id", 0) != 0]
        if len(active_slots) < requirements["min_engines"]:
            logger.warning(f"⚠️ Only {len(active_slots)} engines, need {requirements['min_engines']}")
            # Add complementary engines
            self.add_complementary_engines(preset, requirements["min_engines"] - len(active_slots))
        
        # Ensure exactly 6 slots with proper formatting
        preset["slots"] = self.format_slots_strictly(preset.get("slots", []))
        
        return preset
    
    def create_engine_slot(self, engine_id: int, engine_data: Dict, slot: int) -> Dict:
        """Create a properly formatted engine slot"""
        param_count = engine_data.get("param_count", 0)
        
        # Create parameters with intelligent defaults
        parameters = []
        for i in range(15):  # Always 15 for plugin compatibility
            if i < param_count:
                # Real parameter - use intelligent default
                default = 0.5
                if i == engine_data.get("mix_param_index", -1):
                    default = 0.3  # Conservative mix
                parameters.append({
                    "name": f"param{i+1}",
                    "value": default
                })
            else:
                # Padding parameter
                parameters.append({
                    "name": f"param{i+1}",
                    "value": 0.5
                })
        
        return {
            "slot": slot,
            "engine_id": engine_id,
            "engine_name": engine_data.get("name", f"Engine {engine_id}"),
            "parameters": parameters
        }
    
    def add_complementary_engines(self, preset: Dict, count: int):
        """Add complementary engines to reach minimum count"""
        # Common useful engines that complement most presets
        complementary = [
            (7, "Parametric EQ"),
            (1, "Vintage Opto"),
            (39, "Plate Reverb"),
            (15, "Vintage Tube Preamp"),
            (23, "Digital Chorus")
        ]
        
        current_ids = [s.get("engine_id") for s in preset.get("slots", [])]
        added = 0
        
        for engine_id, engine_name in complementary:
            if added >= count:
                break
            if engine_id not in current_ids:
                engine_data = self.engines.get(str(engine_id), {})
                if engine_data:
                    slot_num = len([s for s in preset["slots"] if s.get("engine_id", 0) != 0])
                    if slot_num < 6:
                        preset["slots"].append(self.create_engine_slot(engine_id, engine_data, slot_num))
                        added += 1
    
    def format_slots_strictly(self, slots: List[Dict]) -> List[Dict]:
        """Ensure exactly 6 slots with proper formatting"""
        formatted = []
        
        # First, add all active engines
        for i, slot in enumerate(slots):
            if i >= 6:
                break
            if slot.get("engine_id", 0) != 0:
                slot["slot"] = len(formatted)
                # Ensure 15 parameters
                while len(slot.get("parameters", [])) < 15:
                    slot.setdefault("parameters", []).append({
                        "name": f"param{len(slot.get('parameters', [])) + 1}",
                        "value": 0.5
                    })
                formatted.append(slot)
        
        # Fill remaining slots with None
        while len(formatted) < 6:
            formatted.append(self.create_engine_slot(0, {"name": "None", "param_count": 0}, len(formatted)))
        
        return formatted
    
    def create_strict_fallback(self, prompt: str, requirements: Dict) -> Dict:
        """Create fallback that strictly follows requirements"""
        slots = []
        
        # Add all mandatory engines
        for i, engine_req in enumerate(requirements["mandatory_engines"]):
            engine_data = self.engines.get(str(engine_req["id"]), {})
            if engine_data:
                slots.append(self.create_engine_slot(engine_req["id"], engine_data, i))
        
        # Fill to minimum
        while len(slots) < requirements["min_engines"]:
            # Add EQ if not present
            if not any(s["engine_id"] in [7, 8, 9, 10] for s in slots):
                slots.append(self.create_engine_slot(7, self.engines["7"], len(slots)))
            else:
                slots.append(self.create_engine_slot(1, self.engines["1"], len(slots)))
        
        # Format to exactly 6
        slots = self.format_slots_strictly(slots)
        
        return {
            "name": f"Strict {requirements['character'].title()} Preset",
            "description": f"Strictly compliant preset for: {prompt}",
            "slots": slots
        }

async def test_strict_visionary():
    """Test the strict visionary"""
    visionary = StrictVisionary()
    
    test_prompts = [
        "ambient pad with shimmer reverb and chorus",
        "metal guitar with noise gate and spring reverb",
        "drum bus with parallel compression"
    ]
    
    for prompt in test_prompts:
        print(f"\n{'='*60}")
        print(f"Testing: {prompt}")
        print('='*60)
        
        preset = await visionary.generate_strict_preset(prompt)
        
        print(f"Preset: {preset['name']}")
        print(f"Engines used: {len([s for s in preset['slots'] if s['engine_id'] != 0])}")
        
        for slot in preset['slots']:
            if slot['engine_id'] != 0:
                print(f"  Slot {slot['slot']}: {slot['engine_name']} (ID: {slot['engine_id']})")

if __name__ == "__main__":
    asyncio.run(test_strict_visionary())