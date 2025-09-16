"""
Cloud Bridge Module - Hybrid AI Integration for Trinity Pipeline
Provides cloud-first AI generation with intelligent local fallback
"""

import asyncio
import json
import logging
import os
import httpx
from typing import Dict, Any, Optional
from pathlib import Path
import hashlib
import random

# Load environment variables
try:
    from dotenv import load_dotenv
    env_path = Path(__file__).parent / '.env'
    if env_path.exists():
        load_dotenv(env_path)
except ImportError:
    pass

logger = logging.getLogger(__name__)

class CloudBridge:
    """
    Manages cloud AI connections with fallback to enhanced local generation
    """
    
    def __init__(self):
        self.api_key = os.getenv("OPENAI_API_KEY")
        self.model = os.getenv("OPENAI_MODEL", "gpt-3.5-turbo")
        self.api_url = "https://api.openai.com/v1/chat/completions"
        self.timeout = 8.0  # 8 second timeout for cloud requests
        
        # Load engine definitions for local fallback
        try:
            from engine_defaults import ENGINE_DEFAULTS
            self.engines = ENGINE_DEFAULTS
        except ImportError:
            self.engines = {}
            
        logger.info(f"Cloud Bridge initialized. API Key: {'Present' if self.api_key else 'Missing'}")
    
    async def get_cloud_generation(self, prompt: str) -> Dict[str, Any]:
        """
        Attempts cloud generation first, falls back to enhanced local generation
        
        Returns:
            Blueprint with 6 slots, creative name, and analysis
        """
        # Try cloud generation if API key is available
        if self.api_key:
            try:
                logger.info("Attempting cloud AI generation...")
                result = await self._cloud_generate(prompt)
                if result:
                    logger.info("✓ Cloud generation successful")
                    result["source"] = "cloud"
                    return result
            except Exception as e:
                logger.warning(f"Cloud generation failed: {str(e)}")
        
        # Fall back to enhanced local generation
        logger.info("Using enhanced local generation (fallback)")
        result = await self._enhanced_local_generate(prompt)
        result["source"] = "local"
        return result
    
    async def _cloud_generate(self, prompt: str) -> Optional[Dict[str, Any]]:
        """
        Generate blueprint using OpenAI API
        """
        if not self.api_key:
            return None
            
        headers = {
            "Authorization": f"Bearer {self.api_key}",
            "Content-Type": "application/json"
        }
        
        # Craft the system prompt for blueprint generation
        system_prompt = """You are a creative audio engineer AI that designs effect presets.
Given a user's request, create a JSON blueprint for an audio effect chain with exactly 6 slots.

IMPORTANT: When specific engines are requested by name, you MUST use those exact engines.
Generate CREATIVE preset names that reflect the actual sound requested (e.g., "Spectral Nightmare" for horror, not "Acoustic Limiter").

Available engines (use these EXACT IDs - this is the CORRECT mapping):

DYNAMICS (0-13):
- 0: None (bypass/empty)
- 1: Vintage Opto Compressor
- 2: Classic Compressor
- 3: Mastering Limiter
- 4: Noise Gate
- 5: Transient Shaper
- 6: Multiband Compressor
- 7: Vintage VCA
- 8: Parallel Compressor
- 9: Tube Limiter
- 10: Expander
- 11: Envelope Follower
- 12: Dynamic EQ
- 13: Glue Compressor

DISTORTION (14-22):
- 14: Tube Saturation
- 15: Vintage Tube Preamp
- 16: Analog Tape Emulation
- 17: K-Style Overdrive
- 18: BitCrusher
- 19: Harmonic Exciter Platinum
- 20: WaveFolder
- 21: Fuzz Pedal
- 22: Vintage Distortion

MODULATION (23-27):
- 23: Classic Chorus
- 24: Analog Phaser
- 25: Vintage Flanger
- 26: Classic Tremolo
- 27: Frequency Shifter

FILTER (28-33):
- 28: Parametric EQ
- 29: Graphic EQ
- 30: Vintage EQ
- 31: Ladder Filter
- 32: Vocal Formant Filter
- 33: Envelope Filter

PITCH (34-39):
- 34: Simple Pitch Shift
- 35: Intelligent Harmonizer
- 36: Formant Shifter
- 37: Ring Modulator
- 38: Pitch Correction
- 39: Vocoder

TIME-BASED (40-46):
- 40: Digital Delay
- 41: Tape Delay
- 42: Plate Reverb
- 43: Spring Reverb
- 44: Shimmer Reverb
- 45: Gated Reverb
- 46: Convolution Reverb

UTILITY (47-51):
- 47: Stereo Imager
- 48: Auto Panner
- 49: Gain
- 50: Phase Rotator
- 51: Mid/Side Processor

SPECIAL/EXPERIMENTAL (52-56):
- 52: Dimension Expander
- 53: Bucket Brigade Delay
- 54: Spectral Freeze
- 55: Granular Cloud
- 56: Chaos Generator

KEY ENGINE MAPPINGS TO REMEMBER:
- "chaos generator" = 56
- "spectral freeze" = 54
- "gated reverb" = 45
- "bitcrusher" = 18
- "granular" = 55
- "shimmer reverb" = 44

Output this exact JSON structure:
{
    "slots": [
        {"slot": 1, "engine_id": <id>, "character": "<descriptive>"},
        {"slot": 2, "engine_id": <id>, "character": "<descriptive>"},
        {"slot": 3, "engine_id": <id>, "character": "<descriptive>"},
        {"slot": 4, "engine_id": <id>, "character": "<descriptive>"},
        {"slot": 5, "engine_id": <id>, "character": "<descriptive>"},
        {"slot": 6, "engine_id": <id>, "character": "<descriptive>"}
    ],
    "overall_vibe": "<2-4 word description>",
    "creative_name": "<creative preset name that MATCHES the request>",
    "creative_analysis": {
        "mood": "<mood>",
        "intensity": <0.0-1.0>,
        "space": <0.0-1.0>,
        "character": "<character>"
    }
}

RULES:
1. If user requests specific engines by name, YOU MUST USE THEM
2. Creative names should reflect the actual sound (e.g., "Haunted Spectral Void" not "Acoustic Limiter")
3. Consider signal flow - effects process in order from slot 1 to 6
4. Use 0 for empty slots
5. Be creative but match the user's intent"""
        
        payload = {
            "model": self.model,
            "messages": [
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": f"Create an audio preset for: {prompt}"}
            ],
            "temperature": 0.8,
            "max_tokens": 500,
            "response_format": {"type": "json_object"}
        }
        
        async with httpx.AsyncClient() as client:
            try:
                response = await client.post(
                    self.api_url,
                    headers=headers,
                    json=payload,
                    timeout=self.timeout
                )
                
                if response.status_code == 200:
                    data = response.json()
                    content = data["choices"][0]["message"]["content"]
                    blueprint = json.loads(content)
                    
                    # Validate and ensure structure
                    blueprint = self._validate_blueprint(blueprint)
                    return blueprint
                else:
                    logger.error(f"OpenAI API error: {response.status_code} - {response.text}")
                    return None
                    
            except asyncio.TimeoutError:
                logger.warning("Cloud request timed out")
                return None
            except Exception as e:
                logger.error(f"Cloud request failed: {str(e)}")
                return None
    
    async def _enhanced_local_generate(self, prompt: str) -> Dict[str, Any]:
        """
        Enhanced local generation with sophisticated pattern matching and variation
        """
        prompt_lower = prompt.lower()
        
        # Use prompt hash for deterministic but varied results
        prompt_hash = hashlib.md5(prompt.encode()).hexdigest()
        random.seed(prompt_hash)
        
        # Initialize blueprint
        blueprint = {
            "slots": [],
            "overall_vibe": "",
            "creative_name": "",
            "creative_analysis": {
                "mood": "neutral",
                "intensity": 0.5,
                "space": 0.5,
                "character": "balanced"
            }
        }
        
        # Sophisticated pattern matching for engine selection
        slot_configs = []
        
        # Analyze prompt for multiple characteristics
        characteristics = self._analyze_prompt(prompt_lower)
        
        # Build effect chain based on characteristics
        if characteristics["genre"] == "rock":
            slot_configs.extend([
                {"engine_id": 18, "character": "driven"},  # K-Style Overdrive
                {"engine_id": 2, "character": "punchy"},   # Classic Compressor
                {"engine_id": 8, "character": "sculpted"},  # Parametric EQ
            ])
            blueprint["overall_vibe"] = "rock power"
            
        elif characteristics["genre"] == "electronic":
            slot_configs.extend([
                {"engine_id": 20, "character": "digital"},  # BitCrusher
                {"engine_id": 12, "character": "sweeping"}, # State Variable Filter
                {"engine_id": 40, "character": "rhythmic"}, # Tempo Delay
            ])
            blueprint["overall_vibe"] = "electronic pulse"
            
        elif characteristics["genre"] == "ambient":
            slot_configs.extend([
                {"engine_id": 46, "character": "ethereal"},  # Shimmer Reverb
                {"engine_id": 41, "character": "vintage"},   # Vintage Tape Echo
                {"engine_id": 33, "character": "modulated"}, # Classic Chorus
            ])
            blueprint["overall_vibe"] = "ambient space"
            
        # Add mood-based effects
        if characteristics["mood"] == "dark":
            slot_configs.append({"engine_id": 11, "character": "filtered"})  # Vintage Lowpass
            blueprint["creative_analysis"]["mood"] = "dark"
            
        elif characteristics["mood"] == "bright":
            slot_configs.append({"engine_id": 19, "character": "harmonic"})  # Harmonic Exciter
            blueprint["creative_analysis"]["mood"] = "bright"
            
        # Add texture effects
        if characteristics["texture"] == "warm":
            slot_configs.append({"engine_id": 15, "character": "warm"})  # Vintage Tube Drive
            blueprint["creative_analysis"]["character"] = "warm"
            
        elif characteristics["texture"] == "cold":
            slot_configs.append({"engine_id": 53, "character": "digital"})  # Digital Delay
            blueprint["creative_analysis"]["character"] = "cold"
            
        # Add dynamics based on intensity
        if characteristics["intensity"] == "aggressive":
            slot_configs.append({"engine_id": 4, "character": "limiting"})  # Mastering Limiter
            blueprint["creative_analysis"]["intensity"] = 0.8
            
        elif characteristics["intensity"] == "subtle":
            slot_configs.append({"engine_id": 1, "character": "smooth"})  # Vintage Opto
            blueprint["creative_analysis"]["intensity"] = 0.3
            
        # Add spatial effects
        if characteristics["space"] == "wide":
            slot_configs.append({"engine_id": 47, "character": "wide"})  # Stereo Imager
            blueprint["creative_analysis"]["space"] = 0.8
            
        elif characteristics["space"] == "tight":
            slot_configs.append({"engine_id": 5, "character": "gated"})  # Noise Gate
            blueprint["creative_analysis"]["space"] = 0.2
            
        # Handle special requests
        if "vocal" in prompt_lower or "voice" in prompt_lower:
            slot_configs.extend([
                {"engine_id": 52, "character": "robotic"},  # Vocoder
                {"engine_id": 51, "character": "shifted"},  # Formant Shifter
            ])
            blueprint["overall_vibe"] = "vocal processing"
            
        if "chaos" in prompt_lower or "random" in prompt_lower:
            slot_configs.append({"engine_id": 54, "character": "chaotic"})  # Chaos Generator
            blueprint["overall_vibe"] = "controlled chaos"
            
        if "shimmer" in prompt_lower:
            slot_configs.append({"engine_id": 46, "character": "shimmering"})  # Shimmer Reverb
            blueprint["overall_vibe"] = "shimmering beauty"
            
        # Default if no specific patterns matched
        if not slot_configs:
            slot_configs = [
                {"engine_id": 2, "character": "balanced"},   # Classic Compressor
                {"engine_id": 8, "character": "neutral"},    # Parametric EQ
                {"engine_id": 42, "character": "spacious"},  # Plate Reverb
            ]
            blueprint["overall_vibe"] = "balanced tone"
        
        # Ensure we have variety - add complementary effects
        if len(slot_configs) < 4:
            complementary = self._get_complementary_effects(slot_configs)
            slot_configs.extend(random.sample(complementary, min(2, len(complementary))))
        
        # Limit to 6 slots and randomize order for variety
        if len(slot_configs) > 6:
            slot_configs = random.sample(slot_configs, 6)
        
        # Build final slots
        for i in range(6):
            if i < len(slot_configs):
                slot = {
                    "slot": i + 1,
                    "engine_id": slot_configs[i]["engine_id"],
                    "character": slot_configs[i]["character"]
                }
            else:
                slot = {
                    "slot": i + 1,
                    "engine_id": 0,  # None/bypass
                    "character": "bypass"
                }
            blueprint["slots"].append(slot)
        
        # Generate creative name
        blueprint["creative_name"] = self._generate_creative_name(prompt, characteristics)
        
        # Set overall vibe if not already set
        if not blueprint["overall_vibe"]:
            blueprint["overall_vibe"] = self._generate_vibe(characteristics)
        
        return blueprint
    
    def _analyze_prompt(self, prompt: str) -> Dict[str, str]:
        """
        Analyze prompt for various characteristics
        """
        characteristics = {
            "genre": "general",
            "mood": "neutral",
            "texture": "balanced",
            "intensity": "moderate",
            "space": "medium"
        }
        
        # Genre detection
        if any(word in prompt for word in ["rock", "metal", "guitar", "punk"]):
            characteristics["genre"] = "rock"
        elif any(word in prompt for word in ["electronic", "edm", "techno", "house", "synth"]):
            characteristics["genre"] = "electronic"
        elif any(word in prompt for word in ["ambient", "atmospheric", "ethereal", "space"]):
            characteristics["genre"] = "ambient"
        elif any(word in prompt for word in ["jazz", "blues", "soul"]):
            characteristics["genre"] = "jazz"
        elif any(word in prompt for word in ["classical", "orchestral", "piano"]):
            characteristics["genre"] = "classical"
            
        # Mood detection
        if any(word in prompt for word in ["dark", "gloomy", "ominous", "haunting"]):
            characteristics["mood"] = "dark"
        elif any(word in prompt for word in ["bright", "happy", "uplifting", "cheerful"]):
            characteristics["mood"] = "bright"
        elif any(word in prompt for word in ["melancholic", "sad", "nostalgic"]):
            characteristics["mood"] = "melancholic"
            
        # Texture detection
        if any(word in prompt for word in ["warm", "vintage", "analog", "tube"]):
            characteristics["texture"] = "warm"
        elif any(word in prompt for word in ["cold", "digital", "pristine", "clean"]):
            characteristics["texture"] = "cold"
        elif any(word in prompt for word in ["gritty", "dirty", "lo-fi", "rough"]):
            characteristics["texture"] = "gritty"
            
        # Intensity detection
        if any(word in prompt for word in ["aggressive", "heavy", "extreme", "brutal"]):
            characteristics["intensity"] = "aggressive"
        elif any(word in prompt for word in ["subtle", "gentle", "soft", "delicate"]):
            characteristics["intensity"] = "subtle"
        elif any(word in prompt for word in ["punchy", "tight", "powerful"]):
            characteristics["intensity"] = "punchy"
            
        # Space detection
        if any(word in prompt for word in ["wide", "spacious", "expansive", "panoramic"]):
            characteristics["space"] = "wide"
        elif any(word in prompt for word in ["tight", "focused", "narrow", "mono"]):
            characteristics["space"] = "tight"
        elif any(word in prompt for word in ["reverb", "echo", "delay"]):
            characteristics["space"] = "reverberant"
            
        return characteristics
    
    def _get_complementary_effects(self, existing_configs: list) -> list:
        """
        Get complementary effects that work well with existing selections
        """
        existing_ids = {cfg["engine_id"] for cfg in existing_configs}
        complementary = []
        
        # If we have distortion, add compression
        if any(eid in existing_ids for eid in [15, 16, 17, 18]):
            if 2 not in existing_ids:
                complementary.append({"engine_id": 2, "character": "controlling"})
                
        # If we have reverb, consider delay
        if any(eid in existing_ids for eid in [42, 43, 44, 45, 46]):
            if not any(eid in existing_ids for eid in [40, 41, 53]):
                complementary.append({"engine_id": 41, "character": "echoing"})
                
        # If we have modulation, consider filter
        if any(eid in existing_ids for eid in [33, 34, 35, 36]):
            if not any(eid in existing_ids for eid in [11, 12, 13]):
                complementary.append({"engine_id": 12, "character": "sweeping"})
                
        # Always good to have EQ
        if not any(eid in existing_ids for eid in [8, 9, 10]):
            complementary.append({"engine_id": 8, "character": "sculpting"})
            
        return complementary
    
    def _generate_creative_name(self, prompt: str, characteristics: Dict[str, str]) -> str:
        """
        Generate a creative preset name based on prompt and characteristics
        """
        # Name components based on characteristics
        prefixes = {
            "rock": ["Power", "Crushing", "Heavy", "Electric"],
            "electronic": ["Digital", "Neon", "Cyber", "Quantum"],
            "ambient": ["Ethereal", "Floating", "Cosmic", "Nebula"],
            "jazz": ["Smooth", "Blue", "Velvet", "Golden"],
            "classical": ["Noble", "Grand", "Elegant", "Refined"]
        }
        
        cores = {
            "dark": ["Shadow", "Night", "Abyss", "Void"],
            "bright": ["Sun", "Light", "Crystal", "Diamond"],
            "neutral": ["Wave", "Sound", "Echo", "Tone"],
            "melancholic": ["Rain", "Autumn", "Memory", "Dream"]
        }
        
        suffixes = {
            "warm": ["Glow", "Warmth", "Vintage", "Tube"],
            "cold": ["Ice", "Steel", "Chrome", "Digital"],
            "balanced": ["Balance", "Harmony", "Unity", "Flow"],
            "gritty": ["Dirt", "Grit", "Rust", "Raw"]
        }
        
        # Build name
        prefix_list = prefixes.get(characteristics["genre"], ["Sonic", "Audio", "Sound"])
        core_list = cores.get(characteristics["mood"], ["Wave", "Pulse", "Flow"])
        suffix_list = suffixes.get(characteristics["texture"], ["Tone", "Mix", "Blend"])
        
        # Random selection with prompt-based seed
        name_seed = hashlib.md5(f"{prompt}{str(characteristics)}".encode()).hexdigest()
        random.seed(name_seed)
        
        name_parts = [
            random.choice(prefix_list),
            random.choice(core_list),
            random.choice(suffix_list) if random.random() > 0.3 else ""
        ]
        
        return " ".join(filter(None, name_parts))
    
    def _generate_vibe(self, characteristics: Dict[str, str]) -> str:
        """
        Generate overall vibe description
        """
        genre_vibes = {
            "rock": "rock energy",
            "electronic": "electronic flow",
            "ambient": "ambient space",
            "jazz": "jazz soul",
            "classical": "classical elegance"
        }
        
        mood_modifiers = {
            "dark": "dark",
            "bright": "bright",
            "melancholic": "nostalgic"
        }
        
        vibe = genre_vibes.get(characteristics["genre"], "sonic journey")
        
        if characteristics["mood"] != "neutral":
            modifier = mood_modifiers.get(characteristics["mood"], "")
            if modifier:
                vibe = f"{modifier} {vibe}"
                
        return vibe
    
    def _validate_blueprint(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Validate and ensure blueprint has correct structure
        """
        # Ensure 6 slots
        if "slots" not in blueprint:
            blueprint["slots"] = []
            
        while len(blueprint["slots"]) < 6:
            blueprint["slots"].append({
                "slot": len(blueprint["slots"]) + 1,
                "engine_id": 0,
                "character": "bypass"
            })
            
        # Ensure slot numbers are correct
        for i, slot in enumerate(blueprint["slots"][:6]):
            slot["slot"] = i + 1
            # Validate engine ID
            if "engine_id" not in slot or not isinstance(slot["engine_id"], int):
                slot["engine_id"] = 0
            else:
                slot["engine_id"] = max(0, min(56, slot["engine_id"]))
                
        blueprint["slots"] = blueprint["slots"][:6]
        
        # Ensure other fields
        if "overall_vibe" not in blueprint:
            blueprint["overall_vibe"] = "custom preset"
        if "creative_name" not in blueprint:
            blueprint["creative_name"] = "Custom Preset"
        if "creative_analysis" not in blueprint:
            blueprint["creative_analysis"] = {
                "mood": "neutral",
                "intensity": 0.5,
                "space": 0.5,
                "character": "balanced"
            }
            
        return blueprint
    
    async def get_modification_analysis(self, current_preset: Dict[str, Any], modification_request: str) -> Dict[str, Any]:
        """
        Use Cloud AI to analyze a modification request and generate a modification blueprint.
        """
        if not self.api_key:
            logger.warning("No OpenAI API key, using local modification analysis")
            return self._local_modification_analysis(current_preset, modification_request)
        
        try:
            # Prepare current preset summary
            preset_summary = self._summarize_preset(current_preset)
            
            # Create modification analysis prompt
            system_prompt = """You are an expert audio engineer analyzing preset modification requests.
Given a current preset configuration and a modification request, analyze what needs to change.

Output a JSON modification blueprint with:
1. intent: Brief description of the modification intent
2. mood_shift: How the mood should change (darker/brighter/warmer/etc)
3. intensity_change: Float from -1.0 to 1.0 indicating intensity change
4. parameter_targets: Dict of parameter types and adjustment amounts (-1.0 to 1.0)
5. engine_suggestions: Dict with "add", "remove", "modify" lists for engine changes
6. preserve_character: Boolean whether to maintain the preset's core character

Examples:
- "Make it darker" -> mood_shift: "darker", parameter_targets: {"brightness": -0.3, "highs": -0.2}
- "Add more reverb" -> parameter_targets: {"reverb": 0.3, "space": 0.2}
- "Like falling through water" -> mood_shift: "submerged", parameter_targets: {"filter": -0.4, "reverb": 0.3}
"""
            
            user_prompt = f"""Current Preset:
{preset_summary}

Modification Request: "{modification_request}"

Analyze this modification request and return a JSON blueprint for the changes needed."""
            
            # Make API call
            response = await asyncio.get_event_loop().run_in_executor(
                None,
                lambda: self.client.chat.completions.create(
                    model="gpt-4",
                    messages=[
                        {"role": "system", "content": system_prompt},
                        {"role": "user", "content": user_prompt}
                    ],
                    response_format={"type": "json_object"},
                    temperature=0.7
                )
            )
            
            # Parse response
            content = response.choices[0].message.content
            blueprint = json.loads(content)
            
            # Ensure all required fields
            blueprint.setdefault("intent", modification_request[:50])
            blueprint.setdefault("mood_shift", "adjusted")
            blueprint.setdefault("intensity_change", 0.0)
            blueprint.setdefault("parameter_targets", {})
            blueprint.setdefault("engine_suggestions", {"add": [], "remove": [], "modify": []})
            blueprint.setdefault("preserve_character", True)
            
            logger.info(f"Cloud modification analysis: {blueprint.get('intent', 'unknown')}")
            return blueprint
            
        except Exception as e:
            logger.error(f"Cloud modification analysis failed: {str(e)}")
            return self._local_modification_analysis(current_preset, modification_request)
    
    def _summarize_preset(self, preset: Dict[str, Any]) -> str:
        """Summarize current preset for modification analysis"""
        summary = []
        summary.append(f"Name: {preset.get('name', 'Unknown')}")
        summary.append(f"Vibe: {preset.get('vibe', 'Unknown')}")
        
        # List active engines
        parameters = preset.get('parameters', {})
        active_engines = []
        for slot in range(1, 7):
            engine_id = parameters.get(f'slot{slot}_engine', 0)
            if engine_id > 0 and parameters.get(f'slot{slot}_bypass', 0) < 0.5:
                from engine_mapping_correct import ENGINE_MAPPING
                engine_name = ENGINE_MAPPING.get(engine_id, f"Engine {engine_id}")
                mix = parameters.get(f'slot{slot}_mix', 0.5)
                active_engines.append(f"Slot {slot}: {engine_name} (mix: {mix:.2f})")
        
        if active_engines:
            summary.append("Active Engines:")
            summary.extend(f"  {eng}" for eng in active_engines)
        
        return "\n".join(summary)
    
    def _local_modification_analysis(self, current_preset: Dict[str, Any], modification_request: str) -> Dict[str, Any]:
        """Fallback local analysis of modification request"""
        request_lower = modification_request.lower()
        
        blueprint = {
            "intent": modification_request[:50],
            "mood_shift": "adjusted",
            "intensity_change": 0.0,
            "parameter_targets": {},
            "engine_suggestions": {"add": [], "remove": [], "modify": []},
            "preserve_character": True
        }
        
        # Check what engines are currently active
        parameters = current_preset.get('parameters', {})
        has_reverb = False
        has_delay = False
        has_distortion = False
        has_modulation = False
        has_chaos = False
        
        from engine_mapping_correct import ENGINE_MAPPING
        
        # print(f"DEBUG cloud_bridge: Checking current engines...")
        for slot in range(1, 7):
            engine_id = parameters.get(f'slot{slot}_engine', 0)
            if engine_id > 0 and parameters.get(f'slot{slot}_bypass', 0) < 0.5:
                engine_name = ENGINE_MAPPING.get(engine_id, "").lower()
                # print(f"  Slot {slot}: Engine {engine_id} = {engine_name}")
                if "reverb" in engine_name:
                    has_reverb = True
                    # print(f"    -> Has reverb!")
                elif "delay" in engine_name or "echo" in engine_name:
                    has_delay = True
                elif any(x in engine_name for x in ["distortion", "overdrive", "bitcrush", "saturation"]):
                    has_distortion = True
                elif any(x in engine_name for x in ["chorus", "phaser", "flanger", "tremolo"]):
                    has_modulation = True
                elif "chaos" in engine_name:
                    has_chaos = True
        
        # print(f"DEBUG cloud_bridge: has_reverb={has_reverb}, request='{request_lower}'")
        
        # Analyze mood shifts
        if "darker" in request_lower or "dark" in request_lower:
            blueprint["mood_shift"] = "darker"
            blueprint["parameter_targets"]["brightness"] = -0.3
            blueprint["parameter_targets"]["highs"] = -0.2
        elif "brighter" in request_lower or "bright" in request_lower:
            blueprint["mood_shift"] = "brighter"
            blueprint["parameter_targets"]["brightness"] = 0.3
            blueprint["parameter_targets"]["highs"] = 0.2
        elif "warmer" in request_lower or "warm" in request_lower:
            blueprint["mood_shift"] = "warmer"
            blueprint["parameter_targets"]["warmth"] = 0.3
            blueprint["parameter_targets"]["saturation"] = 0.1
        elif "ethereal" in request_lower:
            blueprint["mood_shift"] = "ethereal"
            blueprint["parameter_targets"]["reverb"] = 0.3
            blueprint["parameter_targets"]["space"] = 0.3
        
        # Analyze intensity
        if "more aggressive" in request_lower or "aggressive" in request_lower:
            blueprint["intensity_change"] = 0.3
            blueprint["parameter_targets"]["drive"] = 0.3
            blueprint["parameter_targets"]["compression"] = 0.2
            if not has_distortion:
                blueprint["engine_suggestions"]["add"].append("BitCrusher")
        elif "gentler" in request_lower or "softer" in request_lower:
            blueprint["intensity_change"] = -0.3
            blueprint["parameter_targets"]["drive"] = -0.2
            blueprint["parameter_targets"]["attack"] = 0.1
        
        # Analyze specific requests and add engines if needed
        if "reverb" in request_lower:
            # print(f"DEBUG: Reverb in request, checking conditions...")
            # print(f"  'more' in request: {'more' in request_lower}")
            # print(f"  'add' in request: {'add' in request_lower}")
            # print(f"  'spacious' in request: {'spacious' in request_lower}")
            if ("more" in request_lower or "add" in request_lower or "spacious" in request_lower):
                blueprint["parameter_targets"]["reverb"] = 0.3
                blueprint["parameter_targets"]["space"] = 0.2
                # print(f"DEBUG: Should add reverb? has_reverb={has_reverb}, will add: {not has_reverb}")
                if not has_reverb:
                    # Add appropriate reverb based on context
                    if "shimmer" in request_lower:
                        blueprint["engine_suggestions"]["add"].append("Shimmer Reverb")
                        # print(f"DEBUG: Adding Shimmer Reverb")
                    elif "gated" in request_lower:
                        blueprint["engine_suggestions"]["add"].append("Gated Reverb")
                        # print(f"DEBUG: Adding Gated Reverb")
                    elif "spring" in request_lower:
                        blueprint["engine_suggestions"]["add"].append("Spring Reverb")
                        # print(f"DEBUG: Adding Spring Reverb")
                    elif "convolution" in request_lower:
                        blueprint["engine_suggestions"]["add"].append("Convolution Reverb")
                        # print(f"DEBUG: Adding Convolution Reverb")
                    else:
                        blueprint["engine_suggestions"]["add"].append("Plate Reverb")  # Default
                        # print(f"DEBUG: Adding Plate Reverb (default)")
            elif "less" in request_lower or "remove" in request_lower:
                blueprint["parameter_targets"]["reverb"] = -0.3
                blueprint["parameter_targets"]["space"] = -0.2
        
        if "delay" in request_lower or "echo" in request_lower:
            if "more" in request_lower or "add" in request_lower or "rhythmic" in request_lower:
                blueprint["parameter_targets"]["delay"] = 0.3
                blueprint["parameter_targets"]["feedback"] = 0.2
                if not has_delay:
                    if "tape" in request_lower:
                        blueprint["engine_suggestions"]["add"].append("Tape Delay")
                    elif "bucket" in request_lower:
                        blueprint["engine_suggestions"]["add"].append("Bucket Brigade Delay")
                    else:
                        blueprint["engine_suggestions"]["add"].append("Digital Delay")
            elif "less" in request_lower or "remove" in request_lower:
                blueprint["parameter_targets"]["delay"] = -0.3
        
        # Add distortion if requested
        if any(x in request_lower for x in ["distortion", "distort", "overdrive", "crush", "bitcrush", "heavy"]):
            if not has_distortion:
                if "bitcrush" in request_lower:
                    blueprint["engine_suggestions"]["add"].append("BitCrusher")
                elif "tube" in request_lower:
                    blueprint["engine_suggestions"]["add"].append("Tube Saturation")
                elif "fuzz" in request_lower:
                    blueprint["engine_suggestions"]["add"].append("Fuzz Pedal")
                else:
                    blueprint["engine_suggestions"]["add"].append("K-Style Overdrive")
        
        # Add modulation if requested
        if any(x in request_lower for x in ["chorus", "phaser", "flanger", "tremolo", "modulation", "movement", "swirl"]):
            if not has_modulation:
                if "chorus" in request_lower:
                    blueprint["engine_suggestions"]["add"].append("Classic Chorus")
                elif "phaser" in request_lower:
                    blueprint["engine_suggestions"]["add"].append("Analog Phaser")
                elif "flanger" in request_lower:
                    blueprint["engine_suggestions"]["add"].append("Vintage Flanger")
                elif "tremolo" in request_lower:
                    blueprint["engine_suggestions"]["add"].append("Classic Tremolo")
                else:
                    blueprint["engine_suggestions"]["add"].append("Classic Chorus")  # Default modulation
        
        # Check for specific engine requests
        from engine_mapping_correct import ENGINE_ALIASES, ENGINE_NAME_TO_ID, ENGINE_MAPPING
        
        for alias, engine_id in ENGINE_ALIASES.items():
            if alias in request_lower:
                engine_name = ENGINE_MAPPING.get(engine_id, "Unknown")
                if "add" in request_lower or ("chaos" in alias and "unpredictable" in request_lower):
                    # Check if we already have this type of engine
                    has_this_engine = False
                    for slot in range(1, 7):
                        if parameters.get(f'slot{slot}_engine', 0) == engine_id:
                            has_this_engine = True
                            break
                    
                    if not has_this_engine:
                        blueprint["engine_suggestions"]["add"].append(engine_name)
                elif "remove" in request_lower:
                    blueprint["engine_suggestions"]["remove"].append(engine_name)
                elif "replace" in request_lower:
                    # Handle replacement logic
                    parts = request_lower.split("with")
                    if len(parts) > 1:
                        # Find what to replace with
                        replacement_part = parts[1]
                        for r_alias, r_engine_id in ENGINE_ALIASES.items():
                            if r_alias in replacement_part:
                                blueprint["engine_suggestions"]["remove"].append(engine_name)
                                blueprint["engine_suggestions"]["add"].append(ENGINE_MAPPING.get(r_engine_id, "Unknown"))
                                break
        
        return blueprint

# Global instance
cloud_bridge = CloudBridge()

async def get_cloud_generation(prompt: str) -> Dict[str, Any]:
    """
    Main entry point for cloud generation with fallback
    """
    return await cloud_bridge.get_cloud_generation(prompt)

async def get_modification_analysis(current_preset: Dict[str, Any], modification_request: str) -> Dict[str, Any]:
    """
    Analyze a modification request and return a blueprint for changes.
    Uses Cloud AI to understand both technical and poetic modification requests.
    
    Args:
        current_preset: The current preset configuration
        modification_request: User's modification request
        
    Returns:
        Modification blueprint with intent, targets, and suggestions
    """
    return await cloud_bridge.get_modification_analysis(current_preset, modification_request)