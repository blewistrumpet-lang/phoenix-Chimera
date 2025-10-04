#!/usr/bin/env python3
"""
Fixed Visionary that properly communicates engine mappings to AI
"""

import json
import logging
from typing import Dict, List, Any, Optional
from visionary_complete import CompleteVisionary

logger = logging.getLogger(__name__)

class FixedVisionary(CompleteVisionary):
    """Visionary that properly tells AI which engines to use"""
    
    def build_generation_prompt(self, user_prompt: str, context: Dict) -> str:
        """Build prompt that CLEARLY tells AI which engines to use"""
        
        prompt_lower = user_prompt.lower()
        
        # First, check for SPECIFIC ENGINE REQUESTS based on keywords
        specific_engines = []
        engine_mapping_instructions = []
        
        # Check our selection rules
        if hasattr(self, 'knowledge') and 'engine_selection_rules' in self.knowledge:
            for rule_name, rule_data in self.knowledge['engine_selection_rules'].items():
                for keyword in rule_data.get('keywords', []):
                    if keyword in prompt_lower:
                        engine_id = rule_data['engine_id']
                        engine_name = rule_data['engine_name']
                        specific_engines.append({
                            'id': engine_id,
                            'name': engine_name,
                            'keyword': keyword
                        })
                        engine_mapping_instructions.append(
                            f"- User mentioned '{keyword}' so YOU MUST USE Engine {engine_id} ({engine_name})"
                        )
                        break
        
        # Get additional context-based suggestions
        suggested_engines = []
        for ctx in context.get("contexts", []):
            if ctx in self.musical_contexts:
                suggested_engines.extend(self.musical_contexts[ctx].get("engines", []))
        
        instrument = context.get("instrument")
        if instrument and instrument in self.good_combinations:
            suggested_engines.extend(self.good_combinations[instrument])
        
        # Remove duplicates and already specified engines
        specific_ids = [e['id'] for e in specific_engines]
        suggested_engines = [e for e in set(suggested_engines) if e not in specific_ids][:5]
        
        # Build CLEAR engine instructions
        engine_instructions = "SPECIFIC ENGINES TO USE:\n\n"
        
        if specific_engines:
            engine_instructions += "MANDATORY ENGINES (user explicitly requested these):\n"
            for eng in specific_engines:
                engine_data = self.engines.get(str(eng['id']), {})
                if engine_data:
                    engine_instructions += f"\nEngine {eng['id']}: {eng['name']} (REQUIRED - user said '{eng['keyword']}')\n"
                    engine_instructions += f"  Category: {engine_data.get('category', 'Unknown')}\n"
                    engine_instructions += f"  Function: {engine_data.get('function', 'Audio processing')}\n"
                    engine_instructions += f"  Parameters: {engine_data.get('param_count', 15)}\n"
                    engine_instructions += f"  Mix parameter at index: {engine_data.get('mix_param_index', -1)}\n"
        
        if suggested_engines:
            engine_instructions += "\nADDITIONAL SUGGESTED ENGINES:\n"
            for engine_id in suggested_engines:
                engine_data = self.engines.get(str(engine_id), {})
                if engine_data:
                    engine_instructions += f"\nEngine {engine_id}: {engine_data.get('name', 'Unknown')}\n"
                    engine_instructions += f"  Category: {engine_data.get('category', 'Unknown')}\n"
                    engine_instructions += f"  Parameters: {engine_data.get('param_count', 15)}\n"
        
        # Ensure we have enough engines
        min_engines = max(4, len(specific_engines))
        
        # Build the complete prompt
        prompt = f"""You are creating a preset for: "{user_prompt}"

CRITICAL REQUIREMENTS:
1. YOU MUST USE ALL MANDATORY ENGINES LISTED BELOW
2. YOU MUST USE AT LEAST {min_engines} ENGINES TOTAL (minimum 4)
3. YOU MUST USE SPECIFIC ENGINE IDs - NOT GENERIC CATEGORIES
4. NEVER say "EQ/Filter" - use specific engine like "Engine 7: Parametric EQ"
5. NEVER say "Reverb" - use specific engine like "Engine 42: Shimmer Reverb"

{engine_instructions}

KEYWORD TO ENGINE MAPPING RULES:
{chr(10).join(engine_mapping_instructions) if engine_mapping_instructions else "- No specific keywords detected"}

Context Analysis:
- Musical contexts: {context.get('contexts', [])}
- Detected instrument: {context.get('instrument', 'general')}
- Intensity level: {context.get('intensity', 'moderate')}

Signal Chain Order (follow this):
1. Noise Gate (if needed for tight sound)
2. EQ/Filter (tone shaping)
3. Compression/Dynamics
4. Distortion/Saturation (if needed)
5. Modulation (chorus, phaser, flanger)
6. Delay/Echo
7. Reverb (shimmer, spring, plate, etc.)
8. Utility/Output

IMPORTANT REMINDERS:
- Each slot needs "engine_id" (number) and "engine_name" (exact string)
- DO NOT use generic category names - use SPECIFIC engine IDs
- If user says "shimmer" → Engine 42 (Shimmer Reverb)
- If user says "spring" → Engine 40 (Spring Reverb)
- If user says "plate" → Engine 39 (Plate Reverb)
- If user says "chorus" → Engine 23 (Digital Chorus)
- If user says "gate" → Engine 4 (Noise Gate)

Create a unique preset name (not just the prompt in title case).

Return JSON with this EXACT format:
{{
  "name": "<creative preset name>",
  "description": "What this preset does",
  "reasoning": {{
    "overall_approach": "Your strategy",
    "signal_flow": "Why this order",
    "slot_reasoning": [
      {{
        "slot": 0,
        "engine": "Engine {specific_engines[0]['id'] if specific_engines else 7}: {specific_engines[0]['name'] if specific_engines else 'Parametric EQ'}",
        "why": "Why this engine",
        "key_params": "Important parameters"
      }}
      // Continue for all slots
    ]
  }},
  "slots": [
    {{
      "slot": 0,
      "engine_id": {specific_engines[0]['id'] if specific_engines else 7},
      "engine_name": "{specific_engines[0]['name'] if specific_engines else 'Parametric EQ'}",
      "parameters": [
        {{"name": "param1", "value": 0.5}},
        // ... exact number of params for this engine
      ]
    }}
    // Continue for at least {min_engines} engines
  ]
}}

For intensity '{context.get('intensity', 'moderate')}':
- subtle: mix 0.2-0.4, gentle settings
- moderate: mix 0.4-0.6, balanced settings
- extreme: mix 0.6-1.0, aggressive settings

REMEMBER: Use SPECIFIC ENGINE IDs, not generic categories!"""
        
        return prompt

def test_fixed():
    """Test the fixed visionary"""
    import asyncio
    
    async def run_test():
        visionary = FixedVisionary()
        
        test_prompt = "ambient pad with shimmer reverb and spring reverb"
        context = visionary.analyze_prompt_context(test_prompt)
        prompt = visionary.build_generation_prompt(test_prompt, context)
        
        print("FIXED PROMPT TO AI:")
        print("="*60)
        print(prompt[:1500])  # First part
        print("...")
        print("="*60)
        
        # Check for specific engines
        if "Engine 42" in prompt and "Shimmer Reverb" in prompt:
            print("✅ Shimmer Reverb (42) is CLEARLY specified")
        else:
            print("❌ Shimmer Reverb (42) not found")
        
        if "Engine 40" in prompt and "Spring Reverb" in prompt:
            print("✅ Spring Reverb (40) is CLEARLY specified")
        else:
            print("❌ Spring Reverb (40) not found")
        
        if "MANDATORY ENGINES" in prompt:
            print("✅ Mandatory engines section present")
        else:
            print("❌ No mandatory engines section")
    
    asyncio.run(run_test())

if __name__ == "__main__":
    test_fixed()