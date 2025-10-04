#!/usr/bin/env python3
"""
Intelligent Visionary Component for TRUE Trinity Pipeline
This version UNDERSTANDS each engine and its parameters
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

class IntelligentVisionary:
    def __init__(self):
        """Initialize with deep engine knowledge"""
        api_key = os.getenv("OPENAI_API_KEY")
        if not api_key:
            raise ValueError("OpenAI API key not found")
            
        self.client = AsyncOpenAI(api_key=api_key)
        
        # Load complete engine specifications
        with open("complete_engine_specs.json", "r") as f:
            self.engine_data = json.load(f)
        
        self.engines = self.engine_data["engines"]
        logger.info(f"Loaded {len(self.engines)} engine specifications")
    
    def get_engine_prompt_info(self, engine_id: str) -> str:
        """Get detailed engine information for prompt"""
        engine = self.engines.get(engine_id, {})
        if not engine:
            return ""
        
        info = f"Engine {engine_id}: {engine.get('name', 'Unknown')} ({engine.get('category', 'Unknown')})\n"
        info += f"Parameters ({engine.get('param_count', 0)}):\n"
        
        for i, param in enumerate(engine.get('parameters', [])[:engine.get('param_count', 10)]):
            info += f"  param{i+1}: {param['name']} - {param['description']} "
            info += f"(default: {param['default']}, range: {param['min']}-{param['max']} {param['units']})\n"
        
        return info
    
    def create_system_prompt(self) -> str:
        """Create a system prompt with deep engine understanding"""
        prompt = """You are the Visionary, an expert audio engineer AI that creates presets for the Chimera Phoenix plugin.

You have deep knowledge of 57 audio processing engines. Your job is to create intelligent presets that:
1. Use the RIGHT engines for the desired sound
2. Set parameters based on what they actually DO
3. Create musical and coherent combinations
4. Respect parameter counts (each engine has different numbers of parameters)

CRITICAL FORMAT REQUIREMENTS:
- The plugin has 6 slots (indexed 0-5)
- Each slot can have one engine from IDs 0-56
- Each engine has a SPECIFIC number of parameters (varies from 3-10)
- Parameters must be objects: {"name": "param1", "value": 0.5}
- Use exact parameter counts for each engine

ENGINE CATEGORIES:
- Dynamics (1-6): Compressors, gates, limiters
- EQ/Filters (7-14): EQs, filters, resonators
- Distortion (15-22): Tube, saturation, fuzz, overdrive
- Modulation (23-33): Chorus, phaser, tremolo, pitch shift
- Time-based (34-43): Delays, reverbs
- Spatial (44-46, 53): Stereo imaging, widening
- Special (47-52): Spectral, granular, chaos
- Utility (54-56): Gain, mono, phase

PARAMETER UNDERSTANDING:
- Mix parameters control dry/wet balance
- Threshold/Ratio control dynamics behavior
- Frequency/Q shape filter response
- Time/Feedback control delay characteristics
- Size/Damping affect reverb character

You must create presets that make musical sense and use appropriate engines for the requested sound."""
        
        return prompt
    
    async def generate_intelligent_preset(self, prompt: str) -> Dict[str, Any]:
        """Generate a preset with deep understanding of engines"""
        try:
            # Analyze the prompt to determine appropriate engines
            analysis = await self.analyze_prompt(prompt)
            
            # Build the generation prompt with specific engine info
            engine_info = ""
            for engine_type in analysis.get("suggested_engines", []):
                # Find engines in that category
                for engine_id, engine in self.engines.items():
                    if engine.get("category", "").lower() in engine_type.lower():
                        engine_info += self.get_engine_prompt_info(engine_id) + "\n"
                        break
            
            generation_prompt = f"""Create a preset for: {prompt}

Based on this request, generate a preset using appropriate engines.
Available engine information:
{engine_info}

Return a JSON preset with this EXACT structure:
{{
  "name": "Preset Name",
  "description": "Description",
  "slots": [
    {{
      "slot": 0,
      "engine_id": <engine_id>,
      "engine_name": "<engine_name>",
      "parameters": [
        {{"name": "param1", "value": <float>}},
        {{"name": "param2", "value": <float>}},
        ... (exact number of params for this engine)
      ]
    }},
    ... (up to 6 slots, use only what's needed)
  ]
}}

IMPORTANT: Each engine has a SPECIFIC parameter count. Use the exact number shown in the engine info above."""

            response = await self.client.chat.completions.create(
                model="gpt-4-turbo-preview",
                messages=[
                    {"role": "system", "content": self.create_system_prompt()},
                    {"role": "user", "content": generation_prompt}
                ],
                response_format={"type": "json_object"},
                temperature=0.7,
                max_tokens=2000
            )
            
            preset = json.loads(response.choices[0].message.content)
            
            # Validate and fix parameter counts
            preset = self.validate_and_fix_preset(preset)
            
            return preset
            
        except Exception as e:
            logger.error(f"Intelligent generation failed: {e}")
            return self.create_fallback_preset(prompt)
    
    async def analyze_prompt(self, prompt: str) -> Dict[str, Any]:
        """Analyze prompt to determine appropriate engines"""
        analysis_prompt = f"""Analyze this audio preset request: "{prompt}"

Determine:
1. What types of effects are needed (dynamics, eq, reverb, etc.)
2. The musical style or genre
3. The intended use case

Return JSON with:
{{
  "suggested_engines": ["category1", "category2", ...],
  "style": "genre/style",
  "intensity": "subtle/moderate/extreme"
}}"""

        try:
            response = await self.client.chat.completions.create(
                model="gpt-3.5-turbo",
                messages=[
                    {"role": "system", "content": "You are an audio effect analyzer."},
                    {"role": "user", "content": analysis_prompt}
                ],
                response_format={"type": "json_object"},
                temperature=0.3,
                max_tokens=200
            )
            
            return json.loads(response.choices[0].message.content)
        except:
            return {
                "suggested_engines": ["EQ", "Dynamics", "Reverb"],
                "style": "general",
                "intensity": "moderate"
            }
    
    def validate_and_fix_preset(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Ensure preset has correct parameter counts for each engine"""
        if "slots" not in preset:
            preset["slots"] = []
        
        fixed_slots = []
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            engine_spec = self.engines.get(str(engine_id), {})
            expected_params = engine_spec.get("param_count", 10)
            
            # Get current parameters
            current_params = slot.get("parameters", [])
            
            # Fix parameter count
            fixed_params = []
            for i in range(expected_params):
                param_name = f"param{i+1}"
                
                # Try to find existing parameter
                existing = None
                for p in current_params:
                    if p.get("name") == param_name:
                        existing = p
                        break
                
                if existing:
                    fixed_params.append(existing)
                else:
                    # Use default from spec or 0.5
                    default = 0.5
                    if i < len(engine_spec.get("parameters", [])):
                        default = engine_spec["parameters"][i].get("default", 0.5)
                    
                    fixed_params.append({
                        "name": param_name,
                        "value": default
                    })
            
            slot["parameters"] = fixed_params
            slot["engine_name"] = engine_spec.get("name", f"Engine {engine_id}")
            fixed_slots.append(slot)
        
        preset["slots"] = fixed_slots
        return preset
    
    def create_fallback_preset(self, prompt: str) -> Dict[str, Any]:
        """Create a safe fallback preset with correct format"""
        return {
            "name": "Balanced Preset",
            "description": f"Safe preset for: {prompt}",
            "slots": [
                {
                    "slot": 0,
                    "engine_id": 7,  # Parametric EQ
                    "engine_name": "Parametric EQ",
                    "parameters": [
                        {"name": f"param{i+1}", "value": 0.5}
                        for i in range(9)  # Parametric EQ has 9 params
                    ]
                },
                {
                    "slot": 1,
                    "engine_id": 2,  # Classic Compressor
                    "engine_name": "Classic Compressor",
                    "parameters": [
                        {"name": f"param{i+1}", "value": 0.5}
                        for i in range(7)  # Classic Compressor has 7 params
                    ]
                },
                {
                    "slot": 2,
                    "engine_id": 39,  # Plate Reverb
                    "engine_name": "Plate Reverb",
                    "parameters": [
                        {"name": f"param{i+1}", "value": 0.3 if i == 0 else 0.5}
                        for i in range(10)  # Plate Reverb has 10 params
                    ]
                }
            ]
        }

async def test_intelligent_visionary():
    """Test the intelligent visionary"""
    visionary = IntelligentVisionary()
    
    test_prompts = [
        "warm vintage tube sound",
        "modern EDM pump and sizzle",
        "spacious ambient pad"
    ]
    
    for prompt in test_prompts:
        print(f"\nGenerating preset for: {prompt}")
        preset = await visionary.generate_intelligent_preset(prompt)
        
        print(f"Generated: {preset.get('name')}")
        for slot in preset.get('slots', []):
            engine_id = slot.get('engine_id')
            param_count = len(slot.get('parameters', []))
            print(f"  Slot {slot.get('slot')}: Engine {engine_id} ({param_count} params)")

if __name__ == "__main__":
    asyncio.run(test_intelligent_visionary())