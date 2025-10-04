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
from engine_mapping_authoritative import *

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
        
        # Build keyword mappings from authoritative engine mapping
        self.keyword_to_engines = self._build_keyword_mapping()
        self.system_prompt = self._build_system_prompt()
    
    def _build_keyword_mapping(self) -> Dict[str, List[int]]:
        """Build keyword to engine ID mapping using authoritative constants"""
        keyword_map = {
            # Tone descriptors
            "warm": [ENGINE_VINTAGE_TUBE, ENGINE_TAPE_ECHO, ENGINE_BUCKET_BRIGADE_DELAY, ENGINE_HARMONIC_TREMOLO, 
                    ENGINE_VINTAGE_CONSOLE_EQ, ENGINE_MULTIBAND_SATURATOR, ENGINE_K_STYLE],
            "bright": [ENGINE_SHIMMER_REVERB, ENGINE_HARMONIC_EXCITER, ENGINE_PARAMETRIC_EQ],
            "dark": [ENGINE_MUFF_FUZZ, ENGINE_RODENT_DISTORTION, ENGINE_LADDER_FILTER],
            "vintage": [ENGINE_VINTAGE_TUBE, ENGINE_TAPE_ECHO, ENGINE_SPRING_REVERB, ENGINE_OPTO_COMPRESSOR,
                        ENGINE_MAGNETIC_DRUM_ECHO, ENGINE_BUCKET_BRIGADE_DELAY, ENGINE_ANALOG_PHASER,
                        ENGINE_HARMONIC_TREMOLO, ENGINE_CLASSIC_TREMOLO, ENGINE_VINTAGE_CONSOLE_EQ],
            "modern": [ENGINE_DIGITAL_CHORUS, ENGINE_PARAMETRIC_EQ, ENGINE_RODENT_DISTORTION, 
                      ENGINE_DIGITAL_DELAY, ENGINE_DYNAMIC_EQ],
            "clean": [ENGINE_VCA_COMPRESSOR, ENGINE_PARAMETRIC_EQ, ENGINE_NOISE_GATE, ENGINE_DIGITAL_DELAY],
            "dirty": [ENGINE_WAVE_FOLDER, ENGINE_BIT_CRUSHER, ENGINE_MUFF_FUZZ, ENGINE_RODENT_DISTORTION,
                     ENGINE_K_STYLE],
            
            # Effect types
            "reverb": [ENGINE_SHIMMER_REVERB, ENGINE_PLATE_REVERB, ENGINE_CONVOLUTION_REVERB, 
                      ENGINE_SPRING_REVERB, ENGINE_GATED_REVERB],
            "delay": [ENGINE_TAPE_ECHO, ENGINE_MAGNETIC_DRUM_ECHO, ENGINE_BUCKET_BRIGADE_DELAY, ENGINE_DIGITAL_DELAY],
            "echo": [ENGINE_TAPE_ECHO, ENGINE_MAGNETIC_DRUM_ECHO, ENGINE_BUCKET_BRIGADE_DELAY],
            "distortion": [ENGINE_WAVE_FOLDER, ENGINE_BIT_CRUSHER, ENGINE_MUFF_FUZZ, ENGINE_RODENT_DISTORTION,
                          ENGINE_K_STYLE],
            "saturation": [ENGINE_VINTAGE_TUBE, ENGINE_HARMONIC_EXCITER, ENGINE_MULTIBAND_SATURATOR],
            "compression": [ENGINE_OPTO_COMPRESSOR, ENGINE_VCA_COMPRESSOR, ENGINE_MASTERING_LIMITER],
            "modulation": [ENGINE_DIGITAL_CHORUS, ENGINE_ANALOG_PHASER, ENGINE_PITCH_SHIFTER, 
                          ENGINE_RING_MODULATOR, ENGINE_HARMONIC_TREMOLO, ENGINE_CLASSIC_TREMOLO,
                          ENGINE_ROTARY_SPEAKER],
            "filter": [ENGINE_VOCAL_FORMANT, ENGINE_COMB_RESONATOR, ENGINE_LADDER_FILTER,
                      ENGINE_STATE_VARIABLE_FILTER, ENGINE_FORMANT_FILTER, ENGINE_ENVELOPE_FILTER],
            
            # Character
            "aggressive": [ENGINE_RODENT_DISTORTION, ENGINE_MUFF_FUZZ, ENGINE_LADDER_FILTER],
            "gentle": [ENGINE_OPTO_COMPRESSOR, ENGINE_VINTAGE_TUBE, ENGINE_K_STYLE],
            "spacious": [ENGINE_SHIMMER_REVERB, ENGINE_PLATE_REVERB, ENGINE_CONVOLUTION_REVERB,
                        ENGINE_DIMENSION_EXPANDER, ENGINE_STEREO_WIDENER],
            "tight": [ENGINE_VCA_COMPRESSOR, ENGINE_RODENT_DISTORTION, ENGINE_NOISE_GATE],
            "lush": [ENGINE_SHIMMER_REVERB, ENGINE_DIGITAL_CHORUS, ENGINE_RESONANT_CHORUS],
            "experimental": [ENGINE_RING_MODULATOR, ENGINE_GRANULAR_CLOUD, ENGINE_FREQUENCY_SHIFTER,
                            ENGINE_SPECTRAL_FREEZE, ENGINE_CHAOS_GENERATOR, ENGINE_PHASED_VOCODER,
                            ENGINE_FEEDBACK_NETWORK],
            "psychedelic": [ENGINE_ANALOG_PHASER, ENGINE_RING_MODULATOR, ENGINE_CHAOS_GENERATOR],
            "ambient": [ENGINE_SHIMMER_REVERB, ENGINE_GRANULAR_CLOUD, ENGINE_SPECTRAL_FREEZE],
            "glitchy": [ENGINE_BIT_CRUSHER, ENGINE_BUFFER_REPEAT, ENGINE_CHAOS_GENERATOR],
            
            # Specific uses
            "vocal": [ENGINE_VINTAGE_TUBE, ENGINE_OPTO_COMPRESSOR, ENGINE_VOCAL_FORMANT, ENGINE_FORMANT_FILTER,
                     ENGINE_PHASED_VOCODER],
            "drums": [ENGINE_VCA_COMPRESSOR, ENGINE_TRANSIENT_SHAPER, ENGINE_GATED_REVERB, ENGINE_NOISE_GATE],
            "guitar": [ENGINE_VINTAGE_TUBE, ENGINE_TAPE_ECHO, ENGINE_SPRING_REVERB, ENGINE_MUFF_FUZZ,
                      ENGINE_RODENT_DISTORTION, ENGINE_K_STYLE],
            "synth": [ENGINE_LADDER_FILTER, ENGINE_WAVE_FOLDER, ENGINE_RING_MODULATOR, ENGINE_PHASED_VOCODER],
            "bass": [ENGINE_OPTO_COMPRESSOR, ENGINE_VCA_COMPRESSOR, ENGINE_MULTIBAND_SATURATOR,
                    ENGINE_RODENT_DISTORTION],
            "master": [ENGINE_PARAMETRIC_EQ, ENGINE_MULTIBAND_SATURATOR, ENGINE_MASTERING_LIMITER,
                      ENGINE_DYNAMIC_EQ]
        }
        return keyword_map
    
    def _build_system_prompt(self) -> str:
        """Build system prompt with authoritative engine descriptions"""
        engine_list = []
        # Group engines by category using authoritative constants
        categories = {
            "Dynamics & Compression": DYNAMICS_ENGINES,
            "Filters & EQ": FILTER_ENGINES, 
            "Distortion & Saturation": DISTORTION_ENGINES,
            "Modulation Effects": MODULATION_ENGINES,
            "Reverb & Delay": DELAY_REVERB_ENGINES,
            "Spatial & Special Effects": SPATIAL_ENGINES,
            "Utility": UTILITY_ENGINES
        }
        
        for category, engine_ids in categories.items():
            engine_list.append(f"\n{category}:")
            for engine_id in engine_ids:
                engine_name = get_engine_name(engine_id)
                engine_list.append(f'  - {engine_id}: {engine_name}')
        
        engine_text = "\n".join(engine_list)
        
        return f"""You are the Visionary, an AI that creates audio effect presets for Chimera Phoenix.

You have access to these DSP engines organized by category:
{engine_text}

Given a user's creative prompt, output ONLY a valid JSON object.

OUTPUT FORMAT (return ONLY this JSON):
{{
    "slots": [
        {{"slot": 1, "engine_id": <numeric_id>, "character": "<trait>"}},
        {{"slot": 2, "engine_id": <numeric_id>, "character": "<trait>"}},
        {{"slot": 3, "engine_id": <numeric_id>, "character": "<trait>"}},
        {{"slot": 4, "engine_id": <numeric_id>, "character": "<trait>"}},
        {{"slot": 5, "engine_id": <numeric_id>, "character": "<trait>"}},
        {{"slot": 6, "engine_id": <numeric_id>, "character": "<trait>"}}
    ],
    "overall_vibe": "<descriptive string>",
    "reasoning": "<brief explanation of choices>"
}}

RULES:
1. Use numeric IDs from the list above, use 0 for unused/bypass slots
2. Consider signal flow: earlier slots process first
3. Match engines to user intent
4. Create synergistic combinations
5. Character should describe how the engine is used

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
        for keyword, engine_ids in self.keyword_to_engines.items():
            if keyword in prompt_lower:
                for engine_id in engine_ids[:2]:  # Take up to 2 per keyword
                    if engine_id not in selected_engines:
                        selected_engines.append(engine_id)
        
        # Default selections based on common patterns
        if not selected_engines:
            if "guitar" in prompt_lower:
                selected_engines = [ENGINE_VINTAGE_TUBE, ENGINE_TAPE_ECHO, ENGINE_SPRING_REVERB]
            elif "vocal" in prompt_lower:
                selected_engines = [ENGINE_OPTO_COMPRESSOR, ENGINE_PARAMETRIC_EQ, ENGINE_PLATE_REVERB]
            elif "drum" in prompt_lower:
                selected_engines = [ENGINE_TRANSIENT_SHAPER, ENGINE_VCA_COMPRESSOR, ENGINE_GATED_REVERB]
            elif "synth" in prompt_lower:
                selected_engines = [ENGINE_LADDER_FILTER, ENGINE_ANALOG_PHASER, ENGINE_DIGITAL_DELAY]
            else:
                # Generic good preset
                selected_engines = [ENGINE_VINTAGE_TUBE, ENGINE_TAPE_ECHO, ENGINE_PLATE_REVERB]
        
        # Limit to 4 engines max
        selected_engines = selected_engines[:4]
        
        # Build slots
        slots = []
        for i in range(6):
            if i < len(selected_engines):
                engine_id = selected_engines[i]
                character = get_engine_category(engine_id).lower()
                slots.append({
                    "slot": i + 1,
                    "engine_id": engine_id,
                    "character": character
                })
            else:
                slots.append({
                    "slot": i + 1,
                    "engine_id": ENGINE_NONE,
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
                
                engine_id = slot.get("engine_id", ENGINE_NONE)
                if not validate_engine_id(engine_id):
                    logger.warning(f"Invalid engine ID: {engine_id}")
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