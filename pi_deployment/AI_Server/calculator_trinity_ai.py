#!/usr/bin/env python3
"""
CALCULATOR - AI-Powered Musical Intelligence for the Trinity Pipeline
Uses OpenAI to apply deep musical understanding and parameter refinement
"""

import json
import logging
from typing import Dict, Any, List, Optional
from openai import AsyncOpenAI
import numpy as np

from engine_knowledge_base import ENGINE_KNOWLEDGE
from engine_mapping_authoritative import ENGINE_NAMES

logger = logging.getLogger(__name__)

class CalculatorTrinityAI:
    """
    The Calculator: AI-powered musical intelligence that:
    - Optimizes signal chain order with deep understanding
    - Manages complex parameter relationships
    - Ensures musical coherence across all engines
    - Applies nuanced intensity and character adjustments
    """

    def __init__(self, api_key: Optional[str] = None):
        """Initialize with OpenAI API key"""
        # Store API key for lazy client initialization
        self.api_key = api_key
        self._client = None
        self.engine_knowledge = ENGINE_KNOWLEDGE
        self.engine_names = ENGINE_NAMES

        # Build comprehensive system prompt for musical intelligence
        self.system_prompt = self._build_system_prompt()

        logger.info("CalculatorTrinityAI initialized with AI musical intelligence")

    @property
    def client(self):
        """Lazy initialization of AsyncOpenAI client"""
        if self._client is None:
            self._client = AsyncOpenAI(api_key=self.api_key)
        return self._client
    
    def _build_system_prompt(self) -> str:
        """Build the comprehensive musical intelligence prompt"""
        
        return """You are the Calculator component of the Trinity Pipeline - a musical intelligence expert.
Your role is to refine and optimize presets with deep musical understanding.

CORE RESPONSIBILITIES:
1. Signal Chain Optimization - Order engines for optimal signal flow
2. Parameter Relationships - Manage complex interactions between parameters
3. Musical Coherence - Ensure all elements work together harmoniously
4. Character Enhancement - Refine parameters to match the desired character

SIGNAL CHAIN PRINCIPLES:
- Noise gates ALWAYS first (clean input)
- Dynamics before distortion (consistent drive)
- EQ before reverb (prevent muddiness)
- Distortion before modulation (richer harmonics to modulate)
- Delays before reverbs (distinct echoes)
- Stereo effects last (preserve width)
- Special ordering rules:
  * Gate → Compressor → EQ → Distortion → Modulation → Pitch → Delay → Reverb → Spatial

PARAMETER RELATIONSHIPS:
1. Gain Staging:
   - Total gain through chain should not exceed 3.0x
   - Each gain stage multiplies (not adds)
   - Compensate high drive with lower output

2. Feedback Management:
   - Total feedback across all delays/reverbs < 2.0
   - Single delay feedback < 0.8
   - Reverb + delay feedback needs careful balance

3. Frequency Spacing:
   - Multiple EQs should target different frequencies
   - Avoid frequency masking (same freq = cancellation)
   - Space filters at least 0.2 apart (normalized)

4. Modulation Rates:
   - Multiple modulations need different rates (polyrhythmic)
   - Avoid same rate (creates swooshing)
   - Rate ratios: 1:0.66:1.5:2.0

5. Mix Balance (CRITICAL - Effects MUST be audible):
   - Mix = 0% means effect is COMPLETELY INAUDIBLE and useless
   - NEVER set Mix below these ABSOLUTE MINIMUMS:
     * 2 effects: 40-50% each (minimum 35%)
     * 4 effects: 25-35% each (minimum 20%)
     * 6 effects: 15-25% each (MINIMUM 12% - NEVER below this)
   - If an effect isn't needed, DON'T include it in the first place
   - Including an effect with 0% mix defeats the entire purpose

CHARACTER ADJUSTMENTS:
- "warm": Lower highs (-20%), boost lows (+10%), add tube harmonics
- "aggressive": Increase drive (+30%), faster attacks, more presence
- "clean": Reduce all mix levels (-30%), minimal distortion
- "spacious": Larger reverbs (+30%), longer delays, wider stereo
- "tight": Faster gates, shorter decays, controlled low end
- "vintage": More saturation, rolled-off highs, analog character
- "modern": Precise parameters, full frequency, digital clarity

INTENSITY SCALING:
- "subtle/slight": Scale effects to 30-50% of normal
- "moderate": Normal levels (no change)
- "heavy/intense": Scale effects to 130-150%
- "extreme": Push to safe limits (but never unsafe)

GENRE-SPECIFIC OPTIMIZATIONS:
- Metal: Tight gate (threshold +20%), controlled lows, aggressive mids
- Jazz: Warm compression, subtle effects, smooth character
- Electronic: Precise timing, heavy processing, wide stereo
- Ambient: Long decays, high feedback (safely), massive space
- Rock: Punchy dynamics, present mids, controlled effects

SAFETY RULES:
- Never exceed parameter range 0.0-1.0
- Mix parameters: ABSOLUTE MINIMUM 0.12 (12%) - zero mix = useless effect
- Critical drive/level parameters: MINIMUM 0.10 for audibility
- Distortion engines: Drive must be at least 0.15 to be heard
- Limit total feedback to prevent runaway
- Prevent phase cancellation in stereo effects
- Ensure mono compatibility for bass frequencies
- Avoid resonance buildup (max 0.8 per filter)

OUTPUT FORMAT:
Return a JSON object with your refined preset:
{
    "signal_chain_analysis": "Explanation of ordering decisions",
    "parameter_adjustments": "What parameters were adjusted and why",
    "musical_reasoning": "How this achieves the desired sound",
    "slots": [
        {
            "slot": 1,
            "engine_id": 4,
            "engine_name": "Noise Gate",
            "parameters": [
                {"name": "param1", "value": 0.3},
                {"name": "param2", "value": 0.1},
                {"name": "param3", "value": 0.5},
                // ... continue for all parameters used by this engine
            ],
            "parameter_reasoning": {
                "param1": "Threshold at 0.3 for moderate gating",
                "param2": "Fast attack to preserve transients",
                // ... reasoning for key parameters
            }
        }
    ]
}"""
    
    def optimize_preset(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Synchronous wrapper for compatibility with existing code"""
        import asyncio
        # Extract prompt from preset metadata if available
        prompt = preset.get("metadata", {}).get("prompt", preset.get("name", "unknown"))
        try:
            loop = asyncio.get_event_loop()
            if loop.is_running():
                # We're already in an async context, create a task
                import concurrent.futures
                with concurrent.futures.ThreadPoolExecutor() as executor:
                    future = executor.submit(asyncio.run, self.refine_preset(preset, prompt))
                    return future.result(timeout=30)
            else:
                # We can run directly
                return asyncio.run(self.refine_preset(preset, prompt))
        except Exception as e:
            logger.error(f"Error in optimize_preset: {e}")
            return preset  # Return unchanged on error
    
    async def refine_preset(self, preset: Dict[str, Any], prompt: str) -> Dict[str, Any]:
        """
        Use AI to refine the preset with musical intelligence
        
        Args:
            preset: Raw preset from Visionary
            prompt: Original user prompt for context
            
        Returns:
            Refined preset with optimized parameters and ordering
        """
        
        try:
            # Build the refinement request
            user_message = self._build_refinement_request(preset, prompt)
            
            # Call OpenAI for intelligent refinement (await for AsyncOpenAI)
            response = await self.client.chat.completions.create(
                model="gpt-3.5-turbo",  # Using GPT-3.5 Turbo for fast parameter optimization
                messages=[
                    {"role": "system", "content": self.system_prompt},
                    {"role": "user", "content": user_message}
                ],
                temperature=0.5,  # Lower temp for more consistent refinement
                response_format={"type": "json_object"}
            )
            
            # Parse the refined preset
            refined_json = response.choices[0].message.content
            refined_data = json.loads(refined_json)
            
            # Log the AI's reasoning
            logger.info("🧮 CALCULATOR AI REASONING:")
            logger.info(f"  Signal chain analysis: {refined_data.get('signal_chain_analysis', 'N/A')}")
            logger.info(f"  Parameter adjustments: {refined_data.get('parameter_adjustments', 'N/A')}")
            logger.info(f"  Musical reasoning: {refined_data.get('musical_reasoning', 'N/A')}")
            
            # Log per-slot reasoning if available
            for slot_data in refined_data.get("slots", []):
                if "parameter_reasoning" in slot_data:
                    logger.info(f"  Slot {slot_data.get('slot', '?')}: {slot_data.get('engine_name', 'N/A')}")
                    for param, reason in slot_data["parameter_reasoning"].items():
                        logger.info(f"    {param}: {reason}")
            
            # Extract the refined slots
            refined_preset = {
                "name": preset.get("name", "Refined Preset"),
                "description": preset.get("description", ""),
                "slots": refined_data.get("slots", preset.get("slots", [])),
                "metadata": {
                    "original_prompt": prompt,
                    "signal_chain_analysis": refined_data.get("signal_chain_analysis", ""),
                    "parameter_adjustments": refined_data.get("parameter_adjustments", ""),
                    "musical_reasoning": refined_data.get("musical_reasoning", ""),
                    "calculator_version": "AI-1.0"
                },
                "calculator_reasoning": {
                    "signal_chain": refined_data.get("signal_chain_analysis", ""),
                    "adjustments": refined_data.get("parameter_adjustments", ""),
                    "musical": refined_data.get("musical_reasoning", ""),
                    "per_slot": [(s.get("engine_name"), s.get("parameter_reasoning", {})) 
                                 for s in refined_data.get("slots", [])]
                }
            }
            
            # Validate the refinement
            refined_preset = self._validate_refinement(refined_preset)

            # Apply safety overrides to prevent zero-parameter bug
            refined_preset = self._apply_safety_overrides(refined_preset, prompt)

            logger.info(f"AI refined preset: {refined_preset['name']}")

            return refined_preset
            
        except Exception as e:
            logger.error(f"Error in AI refinement: {str(e)}")
            # Fall back to original preset with basic optimization
            return self._basic_optimization(preset)
    
    def _build_refinement_request(self, preset: Dict, prompt: str) -> str:
        """Build the refinement request for the AI"""
        
        # Analyze prompt for context
        context = self._analyze_prompt_context(prompt)
        
        # Format current preset state
        preset_description = self._format_preset_for_ai(preset)
        
        message = f"""Please refine this audio preset for the request: "{prompt}"

DETECTED CONTEXT:
- Character: {context.get('character', 'balanced')}
- Intensity: {context.get('intensity', 'moderate')}
- Genre: {context.get('genre', 'not specified')}
- Key descriptors: {', '.join(context.get('descriptors', []))}

CURRENT PRESET:
{preset_description}

Please:
1. Reorder the signal chain for optimal flow
2. Adjust parameters based on the character and intensity
3. Ensure all parameters work together musically
4. Apply genre-specific optimizations if applicable
5. Manage gain staging and feedback carefully
6. Balance mix levels appropriately

Focus on musical accuracy and professional quality."""
        
        return message
    
    def _analyze_prompt_context(self, prompt: str) -> Dict[str, Any]:
        """Extract musical context from prompt"""
        
        prompt_lower = prompt.lower()
        
        context = {}
        
        # Detect character
        if any(w in prompt_lower for w in ["warm", "vintage", "analog", "tube"]):
            context['character'] = "warm"
        elif any(w in prompt_lower for w in ["aggressive", "brutal", "heavy"]):
            context['character'] = "aggressive"
        elif any(w in prompt_lower for w in ["clean", "pristine", "clear"]):
            context['character'] = "clean"
        elif any(w in prompt_lower for w in ["space", "ethereal", "ambient"]):
            context['character'] = "spacious"
        else:
            context['character'] = "balanced"
        
        # Detect intensity
        if any(w in prompt_lower for w in ["subtle", "slight", "gentle", "touch"]):
            context['intensity'] = "subtle"
        elif any(w in prompt_lower for w in ["extreme", "maximum", "heavy"]):
            context['intensity'] = "extreme"
        else:
            context['intensity'] = "moderate"
        
        # Detect genre
        genres = {
            "metal": ["metal", "djent", "brutal"],
            "jazz": ["jazz", "swing", "smooth"],
            "electronic": ["edm", "techno", "house"],
            "ambient": ["ambient", "atmospheric", "drone"],
            "rock": ["rock", "punk", "grunge"]
        }
        
        for genre, keywords in genres.items():
            if any(kw in prompt_lower for kw in keywords):
                context['genre'] = genre
                break
        
        # Extract key descriptors
        descriptors = []
        descriptor_words = ["punchy", "smooth", "crispy", "dark", "bright", 
                           "thick", "thin", "wide", "narrow", "dry", "wet"]
        
        for word in descriptor_words:
            if word in prompt_lower:
                descriptors.append(word)
        
        context['descriptors'] = descriptors
        
        return context
    
    def _format_preset_for_ai(self, preset: Dict) -> str:
        """Format preset for AI understanding"""
        
        lines = []
        
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            engine_name = slot.get("engine_name", "Unknown")
            params = slot.get("parameters", [])
            
            # Get engine knowledge
            knowledge = self.engine_knowledge.get(engine_id, {})
            
            lines.append(f"\nSlot {slot.get('slot', 0)}: {engine_name} (ID: {engine_id})")
            lines.append(f"  Function: {knowledge.get('function', 'Unknown')}")
            lines.append(f"  Character: {knowledge.get('character', 'Unknown')}")
            lines.append(f"  Parameters: {params}")
            
            # Add parameter meanings if available
            if engine_id in [1, 2, 4, 15, 18, 23, 34, 39]:  # Key engines
                param_meanings = self._get_parameter_meanings(engine_id)
                if param_meanings:
                    lines.append(f"  Parameter meanings: {param_meanings}")
        
        return "\n".join(lines)
    
    def _get_parameter_meanings(self, engine_id: int) -> str:
        """Get parameter meanings for key engines"""
        
        # Simplified parameter descriptions for AI context
        param_maps = {
            1: "Input/Reduction/HF/Output/Mix",  # Opto
            2: "Thresh/Ratio/Att/Rel/Knee/Gain/Mix",  # Compressor
            4: "Thresh/Att/Hold/Rel/Range",  # Gate
            15: "Input/Drive/Bias/Bass/Mid/Treble/Presence/Output/Type/Mix",  # Tube
            18: "Bits/Downsample/Mix",  # Bit Crusher
            23: "Rate/Depth/Mix/Feedback",  # Chorus
            34: "Time/Feedback/Wow/Sat/Mix",  # Tape Echo
            39: "Size/Damp/Pre/Mix/Decay",  # Plate Reverb
        }
        
        return param_maps.get(engine_id, "")
    
    def _validate_refinement(self, preset: Dict) -> Dict:
        """Validate the AI refinement"""
        
        # Ensure all parameters are valid
        for slot in preset.get("slots", []):
            params = slot.get("parameters", [])
            
            # Ensure exactly 10 parameters
            if len(params) < 10:
                params.extend([0.0] * (10 - len(params)))
            elif len(params) > 10:
                params = params[:10]
            
            # Clamp to valid range - keep dict format for Alchemist compatibility
            # Alchemist expects {"name": "param1", "value": 0.5} format
            for p in params:
                if isinstance(p, dict) and "value" in p:
                    # Clamp the value within valid range
                    p["value"] = max(0.0, min(1.0, float(p["value"])))
            slot["parameters"] = params
            
            # Ensure engine_id and name are present
            if "engine_id" not in slot:
                slot["engine_id"] = 0
            if "engine_name" not in slot:
                slot["engine_name"] = self.engine_names.get(slot["engine_id"], "Unknown")
        
        return preset

    def _apply_safety_overrides(self, preset: Dict, prompt: str) -> Dict:
        """
        Apply deterministic safety rules that override AI decisions.
        This ensures professional quality and prevents the zero-parameter bug.
        """

        num_effects = len([s for s in preset.get("slots", []) if s.get("engine_id", 0) > 0])

        # Rule 1: Mix parameter minimums based on effect count
        mix_minimums = {
            1: 0.40,  # Single effect - very audible
            2: 0.35,  # 2 effects
            3: 0.30,  # 3 effects
            4: 0.20,  # 4 effects
            5: 0.15,  # 5 effects
            6: 0.12,  # 6 effects - absolute minimum for audibility
        }
        min_mix = mix_minimums.get(num_effects, 0.15)

        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            if engine_id == 0:
                continue  # Empty slot

            engine_name = slot.get("engine_name", "Unknown")
            params = slot.get("parameters", [])
            knowledge = self.engine_knowledge.get(engine_id, {})

            # CRITICAL: Trim parameters to match engine's actual count
            expected_param_count = knowledge.get("num_params", 0)
            if isinstance(params, dict):
                # Convert dict to only include params up to expected count
                trimmed_params = {}
                for i in range(expected_param_count):
                    key = f"param{i + 1}"
                    if key in params:
                        trimmed_params[key] = params[key]
                slot["parameters"] = trimmed_params
                params = trimmed_params

            # Get mix parameter index
            mix_index = knowledge.get("mix_param_index", -1)

            # RULE 1: Enforce Mix minimum
            if mix_index >= 0 and mix_index < len(params):
                if isinstance(params[mix_index], dict):
                    current_mix = params[mix_index].get("value", 0)
                    if current_mix < min_mix:
                        logger.warning(f"⚙️ SAFETY OVERRIDE: {engine_name} Mix {current_mix:.1%} → {min_mix:.1%} (minimum for {num_effects} effects)")
                        params[mix_index]["value"] = min_mix
                else:
                    if params[mix_index] < min_mix:
                        logger.warning(f"⚙️ SAFETY OVERRIDE: {engine_name} Mix {params[mix_index]:.1%} → {min_mix:.1%}")
                        params[mix_index] = min_mix

            # RULE 2: Distortion engines need audible Drive
            # Engine IDs: 15-22 are distortion/saturation engines
            if engine_id in [15, 16, 17, 18, 19, 20, 21, 22]:
                if len(params) > 0:
                    if isinstance(params[0], dict):
                        drive = params[0].get("value", 0)
                        if drive < 0.15:
                            logger.warning(f"⚙️ SAFETY OVERRIDE: {engine_name} Drive {drive:.1%} → 20% (distortion minimum)")
                            params[0]["value"] = 0.20
                    else:
                        if params[0] < 0.15:
                            logger.warning(f"⚙️ SAFETY OVERRIDE: {engine_name} Drive {params[0]:.1%} → 20%")
                            params[0] = 0.20

            # RULE 3: Dynamics engines need reasonable threshold
            # Engine IDs: 1-6 are dynamics processors
            if engine_id in [1, 2, 3, 4, 5, 6]:
                if len(params) > 0:
                    if isinstance(params[0], dict):
                        threshold = params[0].get("value", 0)
                        if threshold < 0.10 or threshold > 0.90:
                            safe_threshold = 0.35
                            logger.warning(f"⚙️ SAFETY OVERRIDE: {engine_name} Threshold {threshold:.1%} → {safe_threshold:.1%} (safety limit)")
                            params[0]["value"] = safe_threshold
                    else:
                        if params[0] < 0.10 or params[0] > 0.90:
                            params[0] = 0.35

            # RULE 4: Output/Level parameters minimum
            # Check common parameter positions for output/level
            for i in [2, 7, 8]:
                if i < len(params) and isinstance(params[i], dict):
                    param_name = params[i].get("name", "").lower()
                    if "level" in param_name or "output" in param_name or "gain" in param_name:
                        level = params[i].get("value", 0)
                        if level < 0.10:
                            logger.warning(f"⚙️ SAFETY OVERRIDE: {engine_name} {param_name} {level:.1%} → 50%")
                            params[i]["value"] = 0.50

            # RULE 5: Final check - if ALL critical parameters are near-zero, apply safe defaults
            if isinstance(params[0], dict):
                values = [p.get("value", 0) for p in params[:4] if isinstance(p, dict)]
                if all(v < 0.05 for v in values):
                    logger.error(f"❌ CRITICAL: {engine_name} has ALL parameters near zero - applying safe defaults")
                    # Apply conservative but audible defaults
                    safe_defaults = {
                        22: [0.35, 0.50, 0.65, 0.25],  # K-Style: Drive, Tone, Level, Mix
                        15: [0.30, 0.40, 0.50, 0.50, 0.50, 0.50, 0.50, 0.70, 0.0, 0.25],  # Tube
                        18: [0.65, 0.40, 0.30],  # BitCrusher: Bits, Downsample, Mix
                    }

                    if engine_id in safe_defaults:
                        defaults = safe_defaults[engine_id]
                        for i, default in enumerate(defaults):
                            if i < len(params) and isinstance(params[i], dict):
                                params[i]["value"] = default
                    else:
                        # Generic safe defaults
                        for i in range(min(4, len(params))):
                            if isinstance(params[i], dict):
                                params[i]["value"] = 0.35  # Conservative but audible

            slot["parameters"] = params

        return preset

    def _basic_optimization(self, preset: Dict) -> Dict:
        """Basic optimization fallback if AI fails"""
        
        logger.warning("Using basic optimization fallback")
        
        # Simple signal chain ordering by category
        category_order = {
            4: 0,   # Gate first
            1: 1, 2: 1, 3: 1, 5: 1, 6: 1,  # Dynamics
            7: 2, 8: 2, 9: 2, 10: 2,  # EQ/Filter
            15: 3, 16: 3, 17: 3, 18: 3, 19: 3, 20: 3, 21: 3, 22: 3,  # Distortion
            23: 4, 24: 4, 25: 4, 26: 4, 27: 4, 28: 4, 29: 4, 30: 4,  # Modulation
            31: 5, 32: 5, 33: 5,  # Pitch
            34: 6, 35: 6, 36: 6, 37: 6, 38: 6,  # Delay
            39: 7, 40: 7, 41: 7, 42: 7, 43: 7,  # Reverb
            44: 8, 45: 8, 46: 8,  # Spatial
        }
        
        # Sort slots by category
        slots = preset.get("slots", [])
        slots.sort(key=lambda x: category_order.get(x.get("engine_id", 0), 99))
        
        # Renumber slots
        for i, slot in enumerate(slots):
            slot["slot"] = i + 1
        
        preset["slots"] = slots
        
        return preset

# Synchronous wrapper for testing
def refine_preset_sync(preset: Dict, prompt: str, api_key: Optional[str] = None) -> Dict:
    """Synchronous wrapper for preset refinement"""
    import asyncio
    
    calculator = CalculatorTrinityAI(api_key=api_key)
    
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    try:
        refined = loop.run_until_complete(calculator.refine_preset(preset, prompt))
        return refined
    finally:
        loop.close()

if __name__ == "__main__":
    # Test the AI Calculator
    import os
    
    print("🤖 AI CALCULATOR TRINITY TEST")
    print("=" * 60)
    
    # Get API key
    api_key = os.getenv("OPENAI_API_KEY")
    if not api_key:
        print("⚠️ No OPENAI_API_KEY found")
        print("AI refinement unavailable")
    
    # Test preset that needs refinement
    test_preset = {
        "name": "Test Preset",
        "slots": [
            {
                "slot": 1,
                "engine_id": 39,  # Reverb (should be later)
                "engine_name": "Plate Reverb",
                "parameters": [
                    {"name": "param1", "value": 0.5},
                    {"name": "param2", "value": 0.5},
                    {"name": "param3", "value": 0.0},
                    {"name": "param4", "value": 0.8},
                    {"name": "param5", "value": 0.5},
                    {"name": "param6", "value": 0.7},
                    {"name": "param7", "value": 0.2},
                    {"name": "param8", "value": 0.8},
                    {"name": "param9", "value": 0.1},
                    {"name": "param10", "value": 0.0}
                ]
            },
            {
                "slot": 2,
                "engine_id": 15,  # Tube (distortion)
                "engine_name": "Vintage Tube Preamp",
                "parameters": [0.9, 0.9, 0.5, 0.5, 0.5, 0.5, 0.5, 0.9, 0.0, 1.0]
            },
            {
                "slot": 3,
                "engine_id": 4,  # Gate (should be first)
                "engine_name": "Noise Gate",
                "parameters": [0.1, 0.5, 0.3, 0.4, 0.8, 0.2, 0.5, 0.0, 0.0, 0.0]
            },
            {
                "slot": 4,
                "engine_id": 23,  # Chorus
                "engine_name": "Digital Chorus",
                "parameters": [0.5, 0.5, 0.9, 0.3, 0.3, 0.8, 0.8, 0.5, 0.0, 0.0]
            }
        ]
    }
    
    print("Original preset:")
    for slot in test_preset["slots"]:
        print(f"  Slot {slot['slot']}: {slot['engine_name']}")
        print(f"    Parameters: {slot['parameters'][:3]}...")  # Show first 3
    
    # Test with different prompts
    test_prompts = [
        "aggressive metal rhythm tone with tight gate",
        "warm vintage jazz guitar with subtle effects",
        "spacious ambient pad with extreme reverb"
    ]
    
    for prompt in test_prompts[:1]:  # Test first prompt
        print(f"\n📝 Refining for: {prompt}")
        print("-" * 40)
        
        if api_key:
            # Use AI refinement
            refined = refine_preset_sync(test_preset, prompt, api_key)
            
            print("\nRefined preset:")
            for slot in refined["slots"]:
                print(f"  Slot {slot['slot']}: {slot['engine_name']}")
                if 'parameter_reasoning' in slot:
                    print(f"    Reasoning: {list(slot['parameter_reasoning'].values())[0]}...")
            
            if 'metadata' in refined:
                print(f"\nSignal chain analysis: {refined['metadata'].get('signal_chain_analysis', 'N/A')[:100]}...")
                print(f"Musical reasoning: {refined['metadata'].get('musical_reasoning', 'N/A')[:100]}...")
        else:
            # Use basic fallback
            calculator = CalculatorTrinityAI()
            refined = calculator._basic_optimization(test_preset)
            
            print("\nBasic optimization (no AI):")
            for slot in refined["slots"]:
                print(f"  Slot {slot['slot']}: {slot['engine_name']}")
    
    print("\n✅ AI Calculator test complete")