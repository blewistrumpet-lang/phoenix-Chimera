#!/usr/bin/env python3
"""
Complete Visionary Component with Full Engine Knowledge
This version understands EVERYTHING about the plugin
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

class CompleteVisionary:
    def __init__(self):
        """Initialize with complete engine knowledge"""
        api_key = os.getenv("OPENAI_API_KEY")
        if api_key:
            try:
                # Try new initialization method without proxies
                import httpx
                self.client = AsyncOpenAI(
                    api_key=api_key,
                    http_client=httpx.AsyncClient()
                )
                logger.info(f"✅ OpenAI client initialized with API key (length: {len(api_key)})")
            except Exception as e:
                logger.error(f"Failed to initialize OpenAI client: {e}")
                self.client = None
        else:
            self.client = None
            logger.warning("⚠️ No OpenAI API key found - will use fallback generation only")
        
        # Load the COMPLETE knowledge base - SINGLE SOURCE OF TRUTH
        try:
            # Try to load the COMPLETE version first
            with open("trinity_engine_knowledge_COMPLETE.json", "r") as f:
                self.knowledge = json.load(f)
            logger.info(f"✅ Loaded COMPLETE knowledge base with {len(self.knowledge['engines'])} engines")
        except FileNotFoundError:
            # Fall back to regular version if complete doesn't exist
            try:
                with open("trinity_engine_knowledge.json", "r") as f:
                    self.knowledge = json.load(f)
                logger.warning("⚠️ Using regular knowledge base - complete version preferred")
            except FileNotFoundError:
                logger.error("No knowledge base found!")
                raise
        
        self.engines = self.knowledge["engines"]
        self.musical_contexts = self.knowledge["musical_contexts"]
        self.good_combinations = self.knowledge["good_combinations"]
        self.avoid_combinations = self.knowledge["avoid_combinations"]
        
    def create_system_prompt(self) -> str:
        """Create comprehensive system prompt with full knowledge"""
        
        # Build COMPLETE engine catalog for the AI
        engine_catalog = "COMPLETE ENGINE CATALOG (All 57 Available Engines):\n\n"
        
        # Group engines by category for better organization
        categories = {}
        for eid, engine in self.engines.items():
            category = engine.get('category', 'Other')
            if category not in categories:
                categories[category] = []
            categories[category].append((int(eid), engine))
        
        # List all engines with full details
        for category in sorted(categories.keys()):
            engine_catalog += f"=== {category.upper()} ===\n"
            for engine_id, engine in sorted(categories[category], key=lambda x: x[0]):
                engine_catalog += f"Engine {engine_id}: {engine['name']}\n"
                if 'function' in engine:
                    engine_catalog += f"  Function: {engine['function']}\n"
                if 'character' in engine:
                    engine_catalog += f"  Character: {engine['character']}\n"
                engine_catalog += f"  Parameters: {engine.get('param_count', 15)} "
                if engine.get('mix_param_index', -1) >= 0:
                    engine_catalog += f"(Mix at index {engine['mix_param_index']})\n"
                else:
                    engine_catalog += "\n"
            engine_catalog += "\n"
        
        return f"""You are the Visionary component of the Trinity Pipeline for Chimera Phoenix audio plugin.

{engine_catalog}

CRITICAL RULES:
1. When user mentions an effect by name, find and use the EXACT matching engine from the catalog above
2. ALWAYS use specific Engine IDs (e.g., "Engine 42: Shimmer Reverb"), NEVER generic categories
3. Use at least 4 engines per preset
4. Each engine has a SPECIFIC parameter count - respect it
5. Empty slots use engine_id: 0 (ENGINE_NONE)

KEY MAPPINGS (MUST FOLLOW):
- "shimmer" → Engine 42: Shimmer Reverb
- "spring reverb" → Engine 40: Spring Reverb
- "plate reverb" → Engine 39: Plate Reverb
- "tape" → Engine 34: Tape Echo or Engine 15: Vintage Tube Preamp
- "chorus" → Engine 23: Digital Chorus
- "phaser" → Engine 24: Analog Phaser
- "gate" → Engine 4: Noise Gate

SIGNAL CHAIN ORDER:
1. Noise Gate (if needed)
2. EQ/Filter
3. Compression/Dynamics
4. Distortion/Saturation
5. Modulation
6. Delay
7. Reverb
8. Utility/Output

Return ONLY valid JSON with specific engine IDs from the catalog."""

    async def generate_complete_preset(self, prompt: str) -> Dict[str, Any]:
        """Generate preset with complete understanding"""
        logger.info(f"🤖 Starting AI generation for: '{prompt}'")
        
        if not self.client:
            logger.warning("⚠️ No OpenAI client available - using fallback generation")
            return self.create_intelligent_fallback(prompt)
        
        logger.info(f"   Using model: gpt-4")
        logger.info(f"   API client configured: {self.client is not None}")

        try:
            # Analyze prompt for context
            context = self.analyze_prompt_context(prompt)
            logger.info(f"   Context analyzed: {context.get('character', 'unknown')}")

            # Build specific prompt with relevant engine info
            generation_prompt = self.build_generation_prompt(prompt, context)
            logger.info(f"   Prompt built, calling OpenAI...")

            # Generate with GPT-4 for superior creative intelligence
            response = await self.client.chat.completions.create(
                model="gpt-4",  # Superior creative intelligence for engine selection
                messages=[
                    {"role": "system", "content": self.create_system_prompt()},
                    {"role": "user", "content": generation_prompt}
                ],
                response_format={"type": "json_object"},
                temperature=0.7,
                max_tokens=2500
            )
            
            preset = json.loads(response.choices[0].message.content)
            logger.info(f"✅ OpenAI responded successfully!")
            logger.info(f"   Preset name: {preset.get('name', 'Unnamed')}")
            logger.info(f"   Engines: {len(preset.get('slots', []))}")
            
            # Log the AI's reasoning
            if "reasoning" in preset:
                logger.info("🧠 VISIONARY AI REASONING:")
                logger.info(f"  Overall approach: {preset['reasoning'].get('overall_approach', 'N/A')}")
                logger.info(f"  Signal flow logic: {preset['reasoning'].get('signal_flow', 'N/A')}")
                for slot_reason in preset['reasoning'].get('slot_reasoning', []):
                    logger.info(f"  Slot {slot_reason.get('slot', '?')}: {slot_reason.get('engine', 'N/A')}")
                    logger.info(f"    Why: {slot_reason.get('why', 'N/A')}")
                    logger.info(f"    Key params: {slot_reason.get('key_params', 'N/A')}")
            
            # Log a sample of parameters to verify they're not all 0.5
            if preset.get('slots'):
                slot0 = preset['slots'][0]
                params = slot0.get('parameters', [])
                if params:
                    param_values = [p.get('value', 0.5) for p in params[:5]]
                    logger.info(f"   Sample param values: {param_values}")
                    all_same = all(v == 0.5 for v in param_values)
                    if all_same:
                        logger.warning("   ⚠️ Parameters all at 0.5 - AI may not be setting thoughtful values!")
            
            # Validate and ensure format compliance
            preset = self.validate_preset_format(preset)
            
            # Preserve reasoning in the preset for downstream components
            if "reasoning" in preset:
                preset["visionary_reasoning"] = preset.pop("reasoning")
            
            logger.info(f"🎯 AI Generated preset: {preset.get('name', 'Unnamed')}")
            return preset
            
        except Exception as e:
            logger.error(f"❌ AI preset generation failed, using fallback: {str(e)}")
            logger.error(f"Error type: {type(e).__name__}")
            import traceback
            logger.error(f"Traceback: {traceback.format_exc()}")
            return self.create_intelligent_fallback(prompt)
    
    def analyze_prompt_context(self, prompt: str) -> Dict[str, Any]:
        """Analyze prompt to determine musical context - handles both technical and poetic language"""
        prompt_lower = prompt.lower()
        
        # POETIC/ARTISTIC INTERPRETATIONS
        poetic_mappings = {
            # Heavenly/ethereal
            "heaven": ["ambient", "reverb", "shimmer", "ethereal"],
            "mana": ["lush", "rich", "warm", "nourishing"],
            "angel": ["bright", "airy", "chorus", "reverb"],
            "divine": ["sacred", "reverb", "hall", "spacious"],
            "celestial": ["space", "ambient", "delay", "vast"],
            "ethereal": ["shimmer", "reverb", "chorus", "soft"],
            
            # Nature/elements
            "thunder": ["powerful", "compression", "distortion", "impact"],
            "lightning": ["bright", "transient", "sharp", "attack"],
            "ocean": ["wave", "phaser", "chorus", "deep"],
            "rain": ["gentle", "reverb", "soft", "ambient"],
            "fire": ["warm", "saturation", "drive", "aggressive"],
            "ice": ["cold", "digital", "precise", "crystal"],
            "earth": ["warm", "analog", "vintage", "grounded"],
            
            # Emotions/feelings
            "dream": ["reverb", "delay", "soft", "ambient"],
            "nightmare": ["distortion", "dark", "heavy", "chaotic"],
            "love": ["warm", "smooth", "gentle", "embrace"],
            "anger": ["aggressive", "distortion", "harsh", "intense"],
            "peace": ["calm", "gentle", "smooth", "balanced"],
            "chaos": ["wild", "modulation", "unpredictable", "extreme"],
            
            # Colors as moods
            "golden": ["warm", "vintage", "rich", "tube"],
            "blue": ["cool", "smooth", "mellow", "jazz"],
            "red": ["hot", "aggressive", "distortion", "passionate"],
            "purple": ["royal", "lush", "deep", "mysterious"],
            "silver": ["bright", "metallic", "modern", "crisp"],
            
            # Abstract concepts
            "test": ["balanced", "moderate", "general", "versatile"],
            "default": ["standard", "balanced", "clean", "neutral"],
            "basic": ["simple", "clean", "minimal", "essential"],
            "epic": ["huge", "powerful", "dramatic", "cinematic"],
            "vintage": ["warm", "analog", "classic", "nostalgic"],
            "modern": ["clean", "digital", "precise", "contemporary"]
        }
        
        # Extract poetic elements
        poetic_elements = []
        for poetic_term, translations in poetic_mappings.items():
            if poetic_term in prompt_lower:
                poetic_elements.extend(translations)
        
        # Combine poetic interpretations with direct analysis
        enhanced_prompt = prompt_lower + " " + " ".join(poetic_elements)
        
        # Check for matching contexts
        matched_contexts = []
        for context_name, context_data in self.musical_contexts.items():
            keywords = context_name.lower().split('_')
            if any(keyword in enhanced_prompt for keyword in keywords):
                matched_contexts.append(context_name)
        
        # If no contexts matched and we have poetic elements, add some defaults
        if not matched_contexts and poetic_elements:
            if any(word in poetic_elements for word in ["reverb", "ambient", "space"]):
                matched_contexts.append("ambient_space")
            if any(word in poetic_elements for word in ["warm", "vintage", "analog"]):
                matched_contexts.append("warm_vintage")
            if any(word in poetic_elements for word in ["aggressive", "distortion", "heavy"]):
                matched_contexts.append("rock_metal")
        
        # Check for instrument mentions (expanded)
        instruments = {
            "vocal": ["vocal", "voice", "sing", "talk", "speak"],
            "guitar": ["guitar", "axe", "string", "strum", "pick"],
            "bass": ["bass", "low", "sub", "bottom", "foundation"],
            "drums": ["drum", "kick", "snare", "percussion", "beat", "rhythm"],
            "synth": ["synth", "pad", "lead", "electronic", "digital"],
            "master": ["master", "mix", "bus", "full", "complete", "everything"]
        }
        
        detected_instrument = None
        for inst, keywords in instruments.items():
            if any(k in enhanced_prompt for k in keywords):
                detected_instrument = inst
                break
        
        # If still no instrument, make intelligent guess based on context
        if not detected_instrument:
            if poetic_elements:
                if any(word in poetic_elements for word in ["ambient", "pad", "space"]):
                    detected_instrument = "synth"
                elif any(word in poetic_elements for word in ["warm", "vintage"]):
                    detected_instrument = "guitar"
                else:
                    detected_instrument = "master"  # General processing
        
        # Determine intensity (expanded)
        if any(word in enhanced_prompt for word in ["subtle", "gentle", "soft", "light", "minimal", "delicate"]):
            intensity = "subtle"
        elif any(word in enhanced_prompt for word in ["extreme", "heavy", "aggressive", "intense", "maximum", "powerful"]):
            intensity = "extreme"
        else:
            intensity = "moderate"
        
        return {
            "contexts": matched_contexts if matched_contexts else ["warm_vintage"],  # Always have at least one
            "instrument": detected_instrument or "master",
            "intensity": intensity,
            "poetic_elements": poetic_elements[:5]  # Keep top 5 interpretations
        }
    
    def build_generation_prompt(self, user_prompt: str, context: Dict) -> str:
        """Build specific generation prompt with relevant engines"""
        
        # Get suggested engines based on context
        suggested_engines = []
        
        # Add engines from matched musical contexts
        for ctx in context.get("contexts", []):
            if ctx in self.musical_contexts:
                suggested_engines.extend(self.musical_contexts[ctx]["engines"])
        
        # Add instrument-specific suggestions
        instrument = context.get("instrument")
        if instrument and instrument in self.good_combinations:
            suggested_engines.extend(self.good_combinations[instrument])
        
        # Remove duplicates and limit
        suggested_engines = list(set(suggested_engines))[:10]
        
        # Build list of MOST RELEVANT engines based on the user's request
        relevant_engines = []
        prompt_lower = user_prompt.lower()
        
        # First, find directly mentioned engines
        for eid, engine in self.engines.items():
            engine_name_lower = engine['name'].lower()
            
            # Check for direct name matches or keywords
            if any(keyword in prompt_lower for keyword in engine_name_lower.split()):
                relevant_engines.append(int(eid))
            elif 'function' in engine and any(word in engine['function'].lower() for word in prompt_lower.split() if len(word) > 4):
                relevant_engines.append(int(eid))
            elif 'character' in engine and any(word in engine['character'].lower() for word in prompt_lower.split() if len(word) > 4):
                relevant_engines.append(int(eid))
        
        # Add context-based suggestions
        for engine_id in suggested_engines:
            if engine_id not in relevant_engines:
                relevant_engines.append(engine_id)
        
        # Build engine information with FULL DETAILS
        engine_info = "MOST RELEVANT ENGINES FOR YOUR REQUEST:\n\n"
        
        if relevant_engines:
            for engine_id in relevant_engines[:15]:  # Show top 15 most relevant
                engine = self.engines.get(str(engine_id), {})
                if engine:
                    engine_info += f"Engine {engine_id}: {engine['name']}\n"
                    if 'function' in engine:
                        engine_info += f"  Function: {engine['function']}\n"
                    if 'character' in engine:
                        engine_info += f"  Character: {engine['character']}\n"
                    engine_info += f"  Category: {engine.get('category', 'Unknown')}\n"
                    engine_info += f"  Parameters: {engine.get('param_count', 15)}"
                    if engine.get('mix_param_index', -1) >= 0:
                        engine_info += f" (Mix at index {engine['mix_param_index']})\n"
                    else:
                        engine_info += "\n"
                    engine_info += f"  Signal position: {engine.get('signal_chain_position', 5)}\n\n"
        else:
            # If no specific matches, suggest common engines
            engine_info += "No specific matches found. Consider these versatile engines:\n\n"
            common = [7, 1, 39, 23, 34, 15, 22]  # Common useful engines
            for engine_id in common:
                engine = self.engines.get(str(engine_id), {})
                if engine:
                    engine_info += f"Engine {engine_id}: {engine['name']} - {engine.get('function', '')}\n"
        
        prompt = f"""Create a preset for: "{user_prompt}"

{engine_info}

CRITICAL REQUIREMENTS:
1. YOU MUST USE AT LEAST 4 ENGINES (MINIMUM 4, NO EXCEPTIONS!)
2. YOU MUST use the SPECIFIC Engine IDs from the catalog (not generic categories)
3. If user mentions "shimmer" → YOU MUST include Engine 42: Shimmer Reverb
4. If user mentions "spring" → YOU MUST include Engine 40: Spring Reverb  
5. If user mentions "plate" → YOU MUST include Engine 39: Plate Reverb

MINIMUM 4 ENGINES REQUIRED - Even for simple requests, add complementary engines:
- If only 1 specific effect requested, add EQ + Compression + Reverb
- If only 2 effects requested, add EQ + another appropriate effect
- NEVER use less than 4 engines total

Context Analysis:
- Musical style: {context.get('contexts', [])}
- Detected instrument: {context.get('instrument', 'general')}
- Intensity: {context.get('intensity', 'moderate')}

Signal Chain Order (follow this):
1. EQ/Filter → 2. Dynamics → 3. Distortion → 4. Modulation → 5. Delay → 6. Reverb

IMPORTANT: Create a unique, creative preset name inspired by the prompt.
DO NOT just use the prompt words in title case. Be imaginative and evocative.
Examples: "Obsidian Echo Chamber", "Velvet Thunder", "Crystal Cascade", "Midnight Resonance"

Return JSON with EXACT format including your reasoning:
{{
  "name": "<creative preset name here>",
  "description": "What this preset does",
  "reasoning": {{
    "overall_approach": "Explain your overall strategy for this sound",
    "signal_flow": "Explain why you ordered engines this way",
    "slot_reasoning": [
      {{
        "slot": 0,
        "engine": "Engine name",
        "why": "Why this engine for this slot",
        "key_params": "Which parameters are most important and why"
      }}
      // For each slot you use
    ]
  }},
  "slots": [
    {{
      "slot": 0,
      "engine_id": <id>,
      "engine_name": "<exact name from knowledge>",
      "parameters": [
        {{"name": "param1", "value": <0.0-1.0>}},
        ... // EXACT number of params for this engine
      ]
    }},
    // AT LEAST 4 ENGINES REQUIRED (MINIMUM 4!)
    // Continue with slot 1, 2, 3... up to 6 maximum
    // NEVER return less than 4 engines
  ]
}}

For intensity '{context.get('intensity')}':
- subtle: lower mix values (0.2-0.4), gentle settings
- moderate: balanced mix (0.4-0.6), typical settings  
- extreme: higher mix (0.6-1.0), aggressive settings"""
        
        return prompt
    
    def validate_preset_format(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Ensure preset has exactly correct format"""
        if "slots" not in preset:
            preset["slots"] = []
        
        validated_slots = []
        
        for i, slot in enumerate(preset.get("slots", [])):
            if i >= 6:  # Max 6 slots
                break
                
            engine_id = slot.get("engine_id", 0)
            engine_data = self.engines.get(str(engine_id), {})
            
            if not engine_data and engine_id != 0:
                logger.warning(f"Unknown engine {engine_id}, skipping slot")
                continue
            
            # Fix slot index if needed
            slot["slot"] = i
            
            # Fix engine name
            slot["engine_name"] = engine_data.get("name", "None" if engine_id == 0 else f"Engine {engine_id}")
            
            # Fix parameter count and format
            # CRITICAL: Plugin ALWAYS expects exactly 15 parameters per engine
            actual_param_count = engine_data.get("param_count", 0)
            current_params = slot.get("parameters", [])
            
            fixed_params = []
            # Always create 15 parameters (plugin requirement)
            for p in range(15):
                param_name = f"param{p+1}"
                
                # Find existing param value - PRESERVE AI'S THOUGHTFUL VALUES
                value = 0.5  # Default for unused parameters
                
                # Only use values for actual engine parameters
                if p < actual_param_count:
                    # This is a real parameter for this engine
                    if p < len(current_params):
                        existing_param = current_params[p]
                        if isinstance(existing_param, dict):
                            # It's a dict with 'value' key
                            value = existing_param.get("value", 0.5)
                            logger.debug(f"  Using AI value for {param_name}: {value}")
                        elif isinstance(existing_param, (int, float)):
                            # It's a direct numeric value
                            value = float(existing_param)
                            logger.debug(f"  Using numeric value for {param_name}: {value}")
                    else:
                        # No value provided by AI - use knowledge default if available
                        if p < len(engine_data.get("parameters", [])):
                            param_info = engine_data["parameters"][p]
                            if "default" in param_info:
                                value = param_info["default"]
                                logger.debug(f"  Using knowledge default for {param_name}: {value}")
                else:
                    # This is a padding parameter (beyond engine's actual params)
                    # Always use 0.5 for unused parameters
                    value = 0.5
                
                # Clamp to valid range
                value = max(0.0, min(1.0, float(value)))
                
                fixed_params.append({
                    "name": param_name,
                    "value": value
                })
            
            slot["parameters"] = fixed_params
            validated_slots.append(slot)
        
        # CRITICAL: Plugin expects EXACTLY 6 slots (0-5)
        # Fill remaining slots with engine_id 0 (None)
        while len(validated_slots) < 6:
            # Even empty slots need 15 parameters
            empty_params = []
            for p in range(15):
                empty_params.append({
                    "name": f"param{p+1}",
                    "value": 0.5
                })
            
            validated_slots.append({
                "slot": len(validated_slots),
                "engine_id": 0,
                "engine_name": "None",
                "parameters": empty_params
            })
        
        preset["slots"] = validated_slots
        
        # Ensure required fields
        if "name" not in preset:
            preset["name"] = "Generated Preset"
        if "description" not in preset:
            preset["description"] = "AI generated preset"
            
        return preset
    
    def create_intelligent_fallback(self, prompt: str) -> Dict[str, Any]:
        """Create an intelligent fallback based on prompt analysis"""
        logger.warning("⚠️ ⚠️ ⚠️ USING FALLBACK - NOT USING AI ⚠️ ⚠️ ⚠️")
        logger.warning(f"   This preset will have default 0.5 parameters!")
        logger.warning(f"   Prompt was: '{prompt}'")
        context = self.analyze_prompt_context(prompt)
        
        # Choose appropriate engines based on context
        slots = []
        
        # Always start with EQ for tone shaping
        slots.append({
            "slot": 0,
            "engine_id": 7,
            "engine_name": "Parametric EQ",
            "parameters": [{"name": f"param{i+1}", "value": 0.5} for i in range(9)]
        })
        
        # Add compression for dynamics
        if context.get("instrument") in ["vocal", "drums", "bass"]:
            slots.append({
                "slot": len(slots),
                "engine_id": 2,
                "engine_name": "Classic Compressor", 
                "parameters": [{"name": f"param{i+1}", "value": 0.5} for i in range(7)]
            })
        
        # Add character based on intensity
        if context.get("intensity") == "extreme":
            slots.append({
                "slot": len(slots),
                "engine_id": 22,
                "engine_name": "K-Style Overdrive",
                "parameters": [
                    {"name": "param1", "value": 0.6},
                    {"name": "param2", "value": 0.5},
                    {"name": "param3", "value": 0.5},
                    {"name": "param4", "value": 0.5}  # Mix
                ]
            })
        
        # Add space with reverb
        slots.append({
            "slot": len(slots),
            "engine_id": 39,
            "engine_name": "Plate Reverb",
            "parameters": [
                {"name": "param1", "value": 0.3 if context.get("intensity") == "subtle" else 0.5},
                *[{"name": f"param{i+1}", "value": 0.5} for i in range(1, 10)]
            ]
        })
        
        # Ensure exactly 6 slots
        while len(slots) < 6:
            slots.append({
                "slot": len(slots),
                "engine_id": 0,
                "engine_name": "None",
                "parameters": []
            })
        
        return {
            "name": f"{context.get('intensity', 'Moderate').title()} {context.get('instrument', 'Audio').title()} Preset",
            "description": f"Fallback preset for: {prompt}",
            "slots": slots
        }

async def test_complete_visionary():
    """Test the complete visionary"""
    visionary = CompleteVisionary()
    
    test_prompts = [
        "warm vintage vocal with tape saturation",
        "aggressive metal guitar with tight gate",
        "spacious ambient pad with shimmer",
        "punchy drum bus with parallel compression"
    ]
    
    for prompt in test_prompts:
        print(f"\n{'='*60}")
        print(f"Generating: {prompt}")
        print('='*60)
        
        preset = await visionary.generate_complete_preset(prompt)
        
        print(f"Name: {preset['name']}")
        print(f"Description: {preset['description']}")
        print(f"Slots used: {len(preset['slots'])}")
        
        for slot in preset['slots']:
            engine_id = slot['engine_id']
            param_count = len(slot['parameters'])
            engine_data = visionary.engines.get(str(engine_id), {})
            expected = engine_data.get('param_count', 0)
            
            status = "✓" if param_count == expected else f"✗ (expected {expected})"
            print(f"  Slot {slot['slot']}: Engine {engine_id} ({slot['engine_name']}) - {param_count} params {status}")
            
            # Check mix parameter
            mix_index = engine_data.get('mix_param_index', -1)
            if mix_index >= 0 and mix_index < len(slot['parameters']):
                mix_value = slot['parameters'][mix_index]['value']
                print(f"    Mix (param{mix_index+1}): {mix_value:.2f}")

if __name__ == "__main__":
    asyncio.run(test_complete_visionary())