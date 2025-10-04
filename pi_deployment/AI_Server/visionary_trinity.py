#!/usr/bin/env python3
"""
VISIONARY - The Creative Mind of the Trinity Pipeline
Generates complete presets using OpenAI with deep engine understanding
"""

import json
import logging
from typing import Dict, Any, List, Optional
from openai import OpenAI
from engine_knowledge_base import (
    ENGINE_KNOWLEDGE, 
    EngineCategory,
    get_engine_capability,
    find_engines_for_use_case
)
from engine_mapping_authoritative import ENGINE_NAMES

logger = logging.getLogger(__name__)

class VisionaryTrinity:
    """
    The Visionary: Uses OpenAI to generate complete presets with full understanding
    of engines, parameters, and musical relationships.
    """
    
    def __init__(self, api_key: Optional[str] = None):
        """Initialize with OpenAI API key"""
        self.client = OpenAI(api_key=api_key)
        self.engine_knowledge = ENGINE_KNOWLEDGE
        self.engine_names = ENGINE_NAMES
        
        # Build the comprehensive system prompt once
        self.system_prompt = self._build_system_prompt()
        
        logger.info("VisionaryTrinity initialized with complete engine knowledge")
    
    def _build_system_prompt(self) -> str:
        """Build the comprehensive system knowledge prompt"""
        
        # Create engine reference with parameters from UnifiedDefaultParameters
        engine_reference = self._build_engine_reference()
        
        return f"""You are the Visionary component of the Trinity Pipeline for Chimera Phoenix audio plugin.
You have complete understanding of all 56 audio engines and their parameters.

Your role: Generate COMPLETE presets including:
1. Creative preset name
2. 3-5 engines that work together musically  
3. ALL 10 parameters for each engine (0.0-1.0 normalized)
4. Signal chain order consideration

ENGINE REFERENCE:
{engine_reference}

SIGNAL CHAIN ORDER (process in this sequence):
1. Dynamics (Gates, Compressors) - Control dynamics first
2. EQ/Filters - Shape frequency content
3. Distortion - Add harmonics
4. Modulation - Add movement
5. Pitch - Pitch effects
6. Delays - Time-based effects
7. Reverbs - Space and ambience
8. Spatial - Stereo field adjustments

PARAMETER GUIDELINES:
- Mix parameters: Usually 0.2-0.5 for subtlety, up to 1.0 for prominent effects
- Drive/Distortion: 0.1-0.3 subtle, 0.4-0.6 moderate, 0.7+ aggressive
- Feedback: Keep below 0.7 to prevent runaway
- Resonance: Keep below 0.8 to prevent self-oscillation
- Time parameters: Musical subdivisions (0.125=16th, 0.25=8th, 0.5=quarter)

MUSICAL INTELLIGENCE:
- "warm" → Lower treble, add tube harmonics, vintage character
- "aggressive" → Higher drive, faster attack, more presence
- "subtle" → Lower mix values, gentle parameters
- "extreme" → Push parameters toward limits (safely)
- "vintage" → Tape, tube, analog character engines
- "modern" → Digital, precise, clean engines

OUTPUT FORMAT:
Return a JSON object with this exact structure:
{{
    "name": "Creative Preset Name",
    "description": "Brief description of the sound",
    "slots": [
        {{
            "slot": 1,
            "engine_id": 15,
            "engine_name": "Vintage Tube Preamp",
            "reason": "Adds warmth and harmonic richness",
            "parameters": [0.5, 0.3, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.0, 1.0]
        }},
        // ... more slots
    ]
}}

IMPORTANT RULES:
1. Parameters array must have exactly 10 values (0.0 to 1.0)
2. Use 3-5 engines maximum for clarity
3. Ensure musical coherence - engines should complement each other
4. Consider CPU load - limit heavy effects (convolution, shimmer) to 1-2
5. Avoid redundant engines (e.g., multiple reverbs unless intentional)
6. Place engines in optimal signal chain order"""
    
    def _build_engine_reference(self) -> str:
        """Build a concise engine reference for the AI"""
        lines = []
        
        # Group engines by category
        categories = {}
        for engine_id, name in self.engine_names.items():
            if engine_id == 0:
                continue
            
            # Determine category from ID ranges
            if 1 <= engine_id <= 6:
                cat = "DYNAMICS"
            elif 7 <= engine_id <= 14:
                cat = "EQ/FILTER"
            elif 15 <= engine_id <= 22:
                cat = "DISTORTION"
            elif 23 <= engine_id <= 33:
                cat = "MODULATION"
            elif 34 <= engine_id <= 38:
                cat = "DELAY"
            elif 39 <= engine_id <= 43:
                cat = "REVERB"
            elif 44 <= engine_id <= 46:
                cat = "SPATIAL"
            elif 47 <= engine_id <= 52:
                cat = "SPECIAL"
            else:
                cat = "UTILITY"
            
            if cat not in categories:
                categories[cat] = []
            
            # Get engine knowledge
            knowledge = self.engine_knowledge.get(engine_id, {})
            categories[cat].append(
                f"  {engine_id:2d}. {name}: {knowledge.get('function', 'Audio processing')}"
            )
        
        # Format reference
        for cat in ["DYNAMICS", "EQ/FILTER", "DISTORTION", "MODULATION", 
                   "DELAY", "REVERB", "SPATIAL", "SPECIAL", "UTILITY"]:
            if cat in categories:
                lines.append(f"\n{cat}:")
                lines.extend(categories[cat])
        
        return "\n".join(lines)
    
    async def generate_complete_preset(self, prompt: str) -> Dict[str, Any]:
        """
        Generate a complete preset from a user prompt
        
        Args:
            prompt: User's description of desired sound
            
        Returns:
            Complete preset with engines and all parameters
        """
        
        try:
            # Analyze the prompt for context
            context = self._analyze_prompt(prompt)
            
            # Build the user message
            user_message = self._build_user_message(prompt, context)
            
            # Call OpenAI - using gpt-3.5-turbo for stability
            response = self.client.chat.completions.create(
                model="gpt-3.5-turbo",
                messages=[
                    {"role": "system", "content": self.system_prompt},
                    {"role": "user", "content": user_message}
                ],
                temperature=0.7,
                max_tokens=2000,  # Limit response size
                response_format={"type": "json_object"}
            )
            
            # Parse the response
            preset_json = response.choices[0].message.content
            preset = json.loads(preset_json)
            
            # Validate and enhance
            preset = self._validate_preset(preset)
            preset = self._add_metadata(preset, prompt, context)
            
            logger.info(f"Generated preset: {preset['name']} with {len(preset['slots'])} engines")
            
            return preset
            
        except Exception as e:
            logger.error(f"Error generating preset: {str(e)}")
            # Return a safe default preset
            return self._generate_fallback_preset(prompt)
    
    def create_fallback_preset(self, prompt: str) -> Dict[str, Any]:
        """Alias for compatibility"""
        return self._generate_fallback_preset(prompt)
    
    def _analyze_prompt(self, prompt: str) -> Dict[str, Any]:
        """Analyze prompt for musical context"""
        
        prompt_lower = prompt.lower()
        
        context = {
            "genre": self._detect_genre(prompt_lower),
            "character": self._detect_character(prompt_lower),
            "intensity": self._detect_intensity(prompt_lower),
            "instruments": self._detect_instruments(prompt_lower),
            "effects_mentioned": self._extract_effect_mentions(prompt_lower),
            "era": self._detect_era(prompt_lower)
        }
        
        # Find engines that match use cases
        suggested_engines = []
        for word in prompt_lower.split():
            matches = find_engines_for_use_case(word)
            suggested_engines.extend([m[0] for m in matches[:2]])  # Top 2 matches per word
        
        context["suggested_engines"] = list(set(suggested_engines))[:6]  # Unique, max 6
        
        return context
    
    def _detect_genre(self, prompt: str) -> Optional[str]:
        """Detect musical genre from prompt"""
        genres = {
            "metal": ["metal", "heavy", "brutal", "death", "thrash"],
            "rock": ["rock", "grunge", "punk", "indie"],
            "jazz": ["jazz", "smooth", "bebop", "swing"],
            "electronic": ["edm", "techno", "house", "trance", "dubstep"],
            "ambient": ["ambient", "atmospheric", "drone", "soundscape"],
            "pop": ["pop", "radio", "commercial", "mainstream"],
            "classical": ["classical", "orchestra", "symphony", "chamber"],
            "country": ["country", "nashville", "folk", "bluegrass"],
            "blues": ["blues", "soul", "motown", "r&b"],
            "hip-hop": ["hip-hop", "rap", "trap", "boom bap"]
        }
        
        for genre, keywords in genres.items():
            if any(kw in prompt for kw in keywords):
                return genre
        
        return None
    
    def _detect_character(self, prompt: str) -> str:
        """Detect sonic character from prompt"""
        
        if any(w in prompt for w in ["warm", "vintage", "analog", "tube", "smooth"]):
            return "warm"
        elif any(w in prompt for w in ["aggressive", "brutal", "heavy", "harsh", "extreme"]):
            return "aggressive"
        elif any(w in prompt for w in ["clean", "pristine", "transparent", "clear"]):
            return "clean"
        elif any(w in prompt for w in ["dark", "moody", "brooding", "ominous"]):
            return "dark"
        elif any(w in prompt for w in ["bright", "sparkly", "shiny", "brilliant"]):
            return "bright"
        elif any(w in prompt for w in ["space", "ethereal", "ambient", "atmospheric"]):
            return "spacious"
        else:
            return "balanced"
    
    def _detect_intensity(self, prompt: str) -> str:
        """Detect intensity level from prompt"""
        
        if any(w in prompt for w in ["subtle", "gentle", "slight", "touch of"]):
            return "subtle"
        elif any(w in prompt for w in ["extreme", "maximum", "heavy", "intense"]):
            return "extreme"
        elif any(w in prompt for w in ["moderate", "medium", "balanced"]):
            return "moderate"
        else:
            return "moderate"
    
    def _detect_instruments(self, prompt: str) -> List[str]:
        """Detect mentioned instruments"""
        
        instruments = []
        instrument_keywords = {
            "vocals": ["vocal", "voice", "singer", "vox"],
            "guitar": ["guitar", "axe", "strat", "les paul"],
            "bass": ["bass", "low end", "sub"],
            "drums": ["drums", "kit", "percussion", "beat"],
            "keys": ["piano", "keys", "keyboard", "synth"],
            "strings": ["strings", "violin", "cello", "orchestra"]
        }
        
        for instrument, keywords in instrument_keywords.items():
            if any(kw in prompt for kw in keywords):
                instruments.append(instrument)
        
        return instruments
    
    def _extract_effect_mentions(self, prompt: str) -> List[str]:
        """Extract specifically mentioned effects"""
        
        effects = []
        effect_keywords = {
            "reverb": ["reverb", "space", "room", "hall", "cathedral"],
            "delay": ["delay", "echo", "repeat"],
            "chorus": ["chorus", "ensemble", "thicken"],
            "distortion": ["distortion", "overdrive", "fuzz", "crunch"],
            "compression": ["compress", "squeeze", "punch", "glue"],
            "eq": ["eq", "equalize", "tone", "frequency"],
            "filter": ["filter", "sweep", "resonance"],
            "phaser": ["phaser", "phase"],
            "flanger": ["flanger", "jet"],
            "tremolo": ["tremolo", "vibrato"],
            "pitch": ["pitch", "harmonize", "octave", "detune"]
        }
        
        for effect, keywords in effect_keywords.items():
            if any(kw in prompt for kw in keywords):
                effects.append(effect)
        
        return effects
    
    def _detect_era(self, prompt: str) -> str:
        """Detect time period/era from prompt"""
        
        if any(y in prompt for y in ["50s", "60s", "70s", "80s", "90s", "vintage", "classic", "retro"]):
            return "vintage"
        elif any(w in prompt for w in ["modern", "contemporary", "current", "today"]):
            return "modern"
        elif any(w in prompt for w in ["future", "futuristic", "sci-fi", "space age"]):
            return "futuristic"
        else:
            return "timeless"
    
    def _build_user_message(self, prompt: str, context: Dict) -> str:
        """Build the user message with context"""
        
        message_parts = [f"Create a preset for: {prompt}"]
        
        if context["genre"]:
            message_parts.append(f"Genre context: {context['genre']}")
        
        if context["character"]:
            message_parts.append(f"Character: {context['character']}")
        
        if context["intensity"]:
            message_parts.append(f"Intensity: {context['intensity']}")
        
        if context["instruments"]:
            message_parts.append(f"Instruments: {', '.join(context['instruments'])}")
        
        if context["effects_mentioned"]:
            message_parts.append(f"Effects mentioned: {', '.join(context['effects_mentioned'])}")
        
        if context["suggested_engines"]:
            engine_names = [self.engine_names.get(e, f"Unknown({e})") for e in context["suggested_engines"][:5]]
            message_parts.append(f"Consider these engines: {', '.join(engine_names)}")
        
        message_parts.append("\nGenerate a complete preset with 3-5 engines and all parameters.")
        
        return "\n".join(message_parts)
    
    def _validate_preset(self, preset: Dict) -> Dict:
        """Validate and fix any issues in the generated preset"""
        
        # Ensure required fields
        if "name" not in preset:
            preset["name"] = "AI Generated Preset"
        
        if "slots" not in preset:
            preset["slots"] = []
        
        # Validate each slot
        valid_slots = []
        for slot in preset.get("slots", []):
            # Check engine ID is valid
            engine_id = slot.get("engine_id", 0)
            if engine_id not in self.engine_names:
                logger.warning(f"Invalid engine ID {engine_id}, skipping")
                continue
            
            # Ensure parameters array has exactly 10 values
            params = slot.get("parameters", [])
            if not isinstance(params, list):
                params = [0.5] * 10
            
            # Pad or truncate to exactly 10
            if len(params) < 10:
                params.extend([0.0] * (10 - len(params)))
            elif len(params) > 10:
                params = params[:10]
            
            # Clamp all values to 0.0-1.0
            params = [max(0.0, min(1.0, float(p))) for p in params]
            
            # Update slot
            slot["parameters"] = params
            slot["engine_id"] = engine_id
            slot["engine_name"] = self.engine_names[engine_id]
            
            valid_slots.append(slot)
        
        preset["slots"] = valid_slots
        
        # Limit to 6 slots maximum
        if len(preset["slots"]) > 6:
            preset["slots"] = preset["slots"][:6]
        
        return preset
    
    def _add_metadata(self, preset: Dict, prompt: str, context: Dict) -> Dict:
        """Add metadata to the preset"""
        
        preset["metadata"] = {
            "source": "VisionaryTrinity",
            "prompt": prompt,
            "context": context,
            "version": "1.0.0"
        }
        
        return preset
    
    def _generate_fallback_preset(self, prompt: str) -> Dict:
        """Generate a safe fallback preset if AI fails"""
        
        logger.warning("Using fallback preset generation")
        
        # Simple but musical default
        return {
            "name": "Fallback Preset",
            "description": "Safe default preset",
            "slots": [
                {
                    "slot": 1,
                    "engine_id": 7,  # Parametric EQ
                    "engine_name": "Parametric EQ",
                    "reason": "Basic tone shaping",
                    "parameters": [0.2, 0.5, 0.5, 0.5, 0.5, 0.5, 0.8, 0.5, 0.5, 0.0]
                },
                {
                    "slot": 2,
                    "engine_id": 2,  # Classic Compressor
                    "engine_name": "Classic Compressor",
                    "reason": "Dynamics control",
                    "parameters": [0.4, 0.5, 0.2, 0.4, 0.0, 0.5, 1.0, 0.0, 0.0, 0.0]
                },
                {
                    "slot": 3,
                    "engine_id": 39,  # Plate Reverb
                    "engine_name": "Plate Reverb",
                    "reason": "Spatial depth",
                    "parameters": [0.5, 0.5, 0.0, 0.3, 0.5, 0.7, 0.2, 0.8, 0.1, 0.0]
                }
            ],
            "metadata": {
                "source": "VisionaryTrinity_Fallback",
                "prompt": prompt,
                "reason": "Primary generation failed"
            }
        }

# Synchronous wrapper for easier testing
def generate_preset_sync(prompt: str, api_key: Optional[str] = None) -> Dict:
    """Synchronous wrapper for preset generation"""
    import asyncio
    
    visionary = VisionaryTrinity(api_key=api_key)
    
    # Run async function in sync context
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    try:
        preset = loop.run_until_complete(visionary.generate_complete_preset(prompt))
        return preset
    finally:
        loop.close()

if __name__ == "__main__":
    # Test the Visionary
    import os
    
    print("🧠 VISIONARY TRINITY TEST")
    print("=" * 60)
    
    # Get API key from environment
    api_key = os.getenv("OPENAI_API_KEY")
    if not api_key:
        print("⚠️ No OPENAI_API_KEY found in environment")
        print("Using fallback generation only")
    
    # Test prompts
    test_prompts = [
        "Warm vintage guitar tone with tape echo",
        "Aggressive metal rhythm tone",
        "Ethereal ambient pad with shimmer",
        "Punchy drum bus compression",
        "Smooth jazz guitar with chorus"
    ]
    
    visionary = VisionaryTrinity(api_key=api_key)
    
    for prompt in test_prompts[:1]:  # Test first prompt only
        print(f"\n📝 Prompt: {prompt}")
        print("-" * 40)
        
        # Test context analysis
        context = visionary._analyze_prompt(prompt)
        print(f"Context: {json.dumps(context, indent=2)}")
        
        # Generate preset (using fallback if no API key)
        if api_key:
            preset = generate_preset_sync(prompt, api_key)
        else:
            preset = visionary._generate_fallback_preset(prompt)
        
        print(f"\n✅ Generated: {preset['name']}")
        print(f"Engines: {len(preset['slots'])}")
        for slot in preset['slots']:
            print(f"  - {slot['engine_name']}")