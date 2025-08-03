"""
Visionary component updated to use string engine identifiers
"""

import asyncio
import json
import logging
import os
from pathlib import Path
from openai import OpenAI
from typing import Dict, Any, List
from engine_definitions import ENGINES, CATEGORIES, get_engine_key

# Try to load .env file if it exists
try:
    from dotenv import load_dotenv
    env_path = Path(__file__).parent / '.env'
    if env_path.exists():
        load_dotenv(env_path)
except ImportError:
    pass

logger = logging.getLogger(__name__)

class VisionaryStringIDs:
    """
    Visionary that uses string engine identifiers instead of numeric IDs
    """
    
    def __init__(self):
        # Get API key from environment
        api_key = os.getenv("OPENAI_API_KEY")
        if api_key:
            self.openai_client = OpenAI(api_key=api_key)
            self.use_openai = True
            logger.info("OpenAI API configured")
        else:
            self.openai_client = None
            self.use_openai = False
            logger.warning("No OpenAI API key, using simulation")
        
        # Build keyword mappings from engine definitions
        self.keyword_to_engines = self._build_keyword_mapping()
        self.system_prompt = self._build_system_prompt()
    
    def _build_keyword_mapping(self) -> Dict[str, List[str]]:
        """Build keyword to engine mapping from engine definitions"""
        keyword_map = {
            # Tone descriptors
            "warm": ["vintage_tube", "tape_echo", "bucket_brigade", "harmonic_tremolo", 
                    "vintage_console_eq", "multiband_saturator", "k_style_overdrive"],
            "bright": ["shimmer_reverb", "harmonic_exciter", "parametric_eq"],
            "dark": ["muff_fuzz", "rodent_distortion", "ladder_filter"],
            "vintage": ["vintage_tube", "tape_echo", "spring_reverb", "vintage_opto",
                        "magnetic_drum_echo", "bucket_brigade", "analog_phaser",
                        "harmonic_tremolo", "classic_tremolo", "vintage_console_eq"],
            "modern": ["digital_chorus", "parametric_eq", "rodent_distortion", 
                      "digital_delay", "dynamic_eq"],
            "clean": ["classic_compressor", "parametric_eq", "noise_gate", "digital_delay"],
            "dirty": ["wave_folder", "bit_crusher", "muff_fuzz", "rodent_distortion",
                     "k_style_overdrive"],
            
            # Effect types
            "reverb": ["shimmer_reverb", "plate_reverb", "convolution_reverb", 
                      "spring_reverb", "gated_reverb"],
            "delay": ["tape_echo", "magnetic_drum_echo", "bucket_brigade", "digital_delay"],
            "echo": ["tape_echo", "magnetic_drum_echo", "bucket_brigade"],
            "distortion": ["wave_folder", "bit_crusher", "muff_fuzz", "rodent_distortion",
                          "k_style_overdrive"],
            "saturation": ["vintage_tube", "harmonic_exciter", "multiband_saturator"],
            "compression": ["vintage_opto", "classic_compressor", "mastering_limiter"],
            "modulation": ["digital_chorus", "analog_phaser", "pitch_shifter", 
                          "ring_modulator", "harmonic_tremolo", "classic_tremolo",
                          "rotary_speaker"],
            "filter": ["vocal_formant", "comb_resonator", "ladder_filter",
                      "state_variable_filter", "formant_filter", "envelope_filter"],
            
            # Character
            "aggressive": ["rodent_distortion", "muff_fuzz", "ladder_filter"],
            "gentle": ["vintage_opto", "vintage_tube", "k_style_overdrive"],
            "spacious": ["shimmer_reverb", "plate_reverb", "convolution_reverb",
                        "dimension_expander", "stereo_widener"],
            "tight": ["classic_compressor", "rodent_distortion", "noise_gate"],
            "lush": ["shimmer_reverb", "digital_chorus", "resonant_chorus"],
            "experimental": ["ring_modulator", "granular_cloud", "frequency_shifter",
                            "spectral_freeze", "chaos_generator", "phased_vocoder",
                            "feedback_network"],
            "psychedelic": ["analog_phaser", "ring_modulator", "chaos_generator"],
            "ambient": ["shimmer_reverb", "granular_cloud", "spectral_freeze"],
            "glitchy": ["bit_crusher", "buffer_repeat", "chaos_generator"],
            
            # Specific uses
            "vocal": ["vintage_tube", "vintage_opto", "vocal_formant", "formant_filter",
                     "phased_vocoder"],
            "drums": ["classic_compressor", "transient_shaper", "gated_reverb", "noise_gate"],
            "guitar": ["vintage_tube", "tape_echo", "spring_reverb", "muff_fuzz",
                      "rodent_distortion", "k_style_overdrive"],
            "synth": ["ladder_filter", "wave_folder", "ring_modulator", "phased_vocoder"],
            "bass": ["vintage_opto", "classic_compressor", "multiband_saturator",
                    "rodent_distortion"],
            "master": ["parametric_eq", "multiband_saturator", "mastering_limiter",
                      "dynamic_eq"]
        }
        return keyword_map
    
    def _build_system_prompt(self) -> str:
        """Build system prompt with string-based engine descriptions"""
        engine_list = []
        for category, engine_keys in CATEGORIES.items():
            if category == "Utility":
                continue  # Skip bypass
            engine_list.append(f"\n{category}:")
            for key in engine_keys:
                engine = ENGINES[key]
                engine_list.append(f'  - "{key}": {engine["name"]} - {engine.get("description", "")}')
        
        engine_text = "\n".join(engine_list)
        
        return f"""You are the Visionary, an AI that creates audio effect presets for Chimera Phoenix.

You have access to these DSP engines organized by category:
{engine_text}

Given a user's creative prompt, output ONLY a valid JSON object.

OUTPUT FORMAT (return ONLY this JSON):
{{
    "slots": [
        {{"slot": 1, "engine": "<string_id>", "character": "<trait>"}},
        {{"slot": 2, "engine": "<string_id>", "character": "<trait>"}},
        {{"slot": 3, "engine": "<string_id>", "character": "<trait>"}},
        {{"slot": 4, "engine": "<string_id>", "character": "<trait>"}},
        {{"slot": 5, "engine": "<string_id>", "character": "<trait>"}},
        {{"slot": 6, "engine": "<string_id>", "character": "<trait>"}}
    ],
    "overall_vibe": "<descriptive string>",
    "reasoning": "<brief explanation of choices>"
}}

RULES:
1. Use string IDs like "vintage_tube", "tape_echo", NOT numbers
2. Use "bypass" for unused slots
3. Consider signal flow: earlier slots process first
4. Match engines to user intent
5. Create synergistic combinations
6. Character should describe how the engine is used

SIGNAL FLOW:
- EQ/Filters early for tone shaping
- Distortion/Saturation before time effects
- Modulation in middle for movement
- Reverb/Delay at end for space
- Dynamics anywhere based on need

Return ONLY valid JSON."""

    async def get_blueprint(self, prompt: str) -> Dict[str, Any]:
        """Generate blueprint using string IDs"""
        if self.use_openai:
            try:
                blueprint = await self._get_openai_blueprint(prompt)
                if blueprint:
                    return blueprint
            except Exception as e:
                logger.warning(f"OpenAI error: {e}, using simulation")
        
        # Fall back to simulation
        return await self._simulate_blueprint(prompt)
    
    async def _get_openai_blueprint(self, prompt: str) -> Dict[str, Any]:
        """Get blueprint from OpenAI API"""
        try:
            response = await asyncio.to_thread(
                lambda: self.openai_client.chat.completions.create(
                    model="gpt-3.5-turbo",
                    messages=[
                        {"role": "system", "content": self.system_prompt},
                        {"role": "user", "content": prompt}
                    ],
                    temperature=0.7,
                    max_tokens=800,
                    response_format={"type": "json_object"}
                )
            )
            
            content = response.choices[0].message.content
            blueprint = json.loads(content)
            
            if self._validate_blueprint(blueprint):
                return blueprint
            else:
                logger.warning("Invalid blueprint from OpenAI")
                return None
                
        except Exception as e:
            logger.error(f"OpenAI API error: {e}")
            return None
    
    async def _simulate_blueprint(self, prompt: str) -> Dict[str, Any]:
        """Simulate blueprint generation using keyword matching"""
        prompt_lower = prompt.lower()
        selected_engines = []
        
        # Find matching engines based on keywords
        for keyword, engines in self.keyword_to_engines.items():
            if keyword in prompt_lower:
                for engine in engines[:2]:  # Take up to 2 per keyword
                    if engine not in selected_engines:
                        selected_engines.append(engine)
        
        # Default selections based on common patterns
        if not selected_engines:
            if "guitar" in prompt_lower:
                selected_engines = ["vintage_tube", "tape_echo", "spring_reverb"]
            elif "vocal" in prompt_lower:
                selected_engines = ["vintage_opto", "parametric_eq", "plate_reverb"]
            elif "drum" in prompt_lower:
                selected_engines = ["transient_shaper", "classic_compressor", "gated_reverb"]
            elif "synth" in prompt_lower:
                selected_engines = ["ladder_filter", "analog_phaser", "digital_delay"]
            else:
                # Generic good preset
                selected_engines = ["vintage_tube", "tape_echo", "plate_reverb"]
        
        # Limit to 4 engines max
        selected_engines = selected_engines[:4]
        
        # Build slots
        slots = []
        for i in range(6):
            if i < len(selected_engines):
                engine_key = selected_engines[i]
                engine_info = ENGINES.get(engine_key, {})
                character = engine_info.get("category", "effect")
                slots.append({
                    "slot": i + 1,
                    "engine": engine_key,
                    "character": character
                })
            else:
                slots.append({
                    "slot": i + 1,
                    "engine": "bypass",
                    "character": "unused"
                })
        
        # Determine vibe
        if "aggressive" in prompt_lower or "metal" in prompt_lower:
            vibe = "aggressive and powerful"
        elif "warm" in prompt_lower or "vintage" in prompt_lower:
            vibe = "warm and vintage"
        elif "clean" in prompt_lower or "pristine" in prompt_lower:
            vibe = "clean and transparent"
        elif "ambient" in prompt_lower or "spacious" in prompt_lower:
            vibe = "spacious and ethereal"
        else:
            vibe = "balanced and musical"
        
        return {
            "slots": slots,
            "overall_vibe": vibe,
            "reasoning": f"Selected engines based on: {prompt[:50]}..."
        }
    
    def _validate_blueprint(self, blueprint: Dict[str, Any]) -> bool:
        """Validate blueprint with string IDs"""
        try:
            if not isinstance(blueprint, dict):
                return False
            
            if "slots" not in blueprint:
                return False
            
            if not isinstance(blueprint["slots"], list) or len(blueprint["slots"]) != 6:
                return False
            
            for i, slot in enumerate(blueprint["slots"]):
                if not isinstance(slot, dict):
                    return False
                
                if slot.get("slot") != i + 1:
                    return False
                
                engine = slot.get("engine", "")
                if engine != "bypass" and engine not in ENGINES:
                    logger.warning(f"Unknown engine: {engine}")
                    return False
            
            return True
            
        except Exception as e:
            logger.error(f"Blueprint validation error: {e}")
            return False


# Test function
async def test_visionary():
    """Test the string-based Visionary"""
    visionary = VisionaryStringIDs()
    
    test_prompts = [
        "warm vintage guitar with tube saturation",
        "aggressive metal sound with tight gate",
        "spacious ambient pad with shimmer"
    ]
    
    print("\n" + "="*80)
    print("TESTING VISIONARY WITH STRING IDS")
    print("="*80)
    
    for prompt in test_prompts:
        print(f"\nPrompt: {prompt}")
        blueprint = await visionary.get_blueprint(prompt)
        print(f"Result: {json.dumps(blueprint, indent=2)}")

if __name__ == "__main__":
    asyncio.run(test_visionary())