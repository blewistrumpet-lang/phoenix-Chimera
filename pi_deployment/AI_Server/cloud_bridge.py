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
from engine_mapping_authoritative import *

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
                else:
                    logger.warning("Cloud generation returned empty result, falling back to local")
            except asyncio.TimeoutError:
                logger.warning("Cloud generation timed out after 8s, falling back to local")
            except Exception as e:
                logger.warning(f"Cloud generation failed: {type(e).__name__}: {str(e)}")
        else:
            logger.info("No OpenAI API key configured, using local generation")
        
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

CRITICAL RULES:
1. NEVER use UTILITY engines (53-56) for primary creative effects
2. UTILITY engines are only for technical correction, NOT musical creativity
3. Prioritize MUSICAL engines (1-52) for all creative requests
4. When specific engines are requested by name, you MUST use those exact engines
5. Generate CREATIVE preset names that reflect the actual sound requested

MUSICAL ENGINES (1-52) - USE THESE FOR CREATIVE EFFECTS:

DYNAMICS & COMPRESSION (1-6):
- 1: Vintage Opto Compressor
- 2: Classic Compressor  
- 3: Transient Shaper
- 4: Noise Gate
- 5: Mastering Limiter
- 6: Dynamic EQ

FILTERS & EQ (7-14):
- 7: Parametric EQ
- 8: Vintage Console EQ
- 9: Ladder Filter
- 10: State Variable Filter
- 11: Formant Filter
- 12: Envelope Filter
- 13: Comb Resonator
- 14: Vocal Formant

DISTORTION & SATURATION (15-22):
- 15: Vintage Tube
- 16: Wave Folder
- 17: Harmonic Exciter
- 18: Bit Crusher
- 19: Multiband Saturator
- 20: Muff Fuzz
- 21: Rodent Distortion
- 22: K-Style

MODULATION EFFECTS (23-33):
- 23: Digital Chorus
- 24: Resonant Chorus
- 25: Analog Phaser
- 26: Ring Modulator
- 27: Frequency Shifter
- 28: Harmonic Tremolo
- 29: Classic Tremolo
- 30: Rotary Speaker
- 31: Pitch Shifter
- 32: Detune Doubler
- 33: Intelligent Harmonizer

REVERB & DELAY (34-43):
- 34: Tape Echo
- 35: Digital Delay
- 36: Magnetic Drum Echo
- 37: Bucket Brigade Delay
- 38: Buffer Repeat
- 39: Plate Reverb
- 40: Spring Reverb
- 41: Convolution Reverb
- 42: Shimmer Reverb
- 43: Gated Reverb

SPATIAL & SPECIAL EFFECTS (44-52):
- 44: Stereo Widener
- 45: Stereo Imager
- 46: Dimension Expander
- 47: Spectral Freeze
- 48: Spectral Gate
- 49: Phased Vocoder
- 50: Granular Cloud
- 51: Chaos Generator
- 52: Feedback Network

UTILITY ENGINES (53-56) - DO NOT USE FOR CREATIVE EFFECTS:
- 53: Mid-Side Processor (technical only)
- 54: Gain Utility (technical only)
- 55: Mono Maker (technical only)
- 56: Phase Align (technical only)

KEY ENGINE MAPPINGS TO REMEMBER:
- "chaos generator" = 51
- "spectral freeze" = 47
- "gated reverb" = 43
- "bitcrusher" = 18
- "granular" = 50
- "shimmer reverb" = 42

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

PRIORITY RULES:
1. NEVER select utility engines (53-56) for musical/creative requests
2. If user requests specific engines by name, YOU MUST USE THEM
3. Prioritize musical engines that match the requested character:
   - Warm → Vintage Tube (15), Tape Echo (34), Opto Compressor (1)
   - Bright → Harmonic Exciter (17), Parametric EQ (7), Shimmer Reverb (42)
   - Dark → Ladder Filter (9), Muff Fuzz (20), Gated Reverb (43)
   - Reverb → Use actual reverbs (39-43), NOT utility engines
4. Creative names should reflect the actual sound (e.g., "Haunted Spectral Void" not "Acoustic Limiter")
5. Consider signal flow - effects process in order from slot 1 to 6
6. Use 0 for empty/bypass slots
7. Be creative but match the user's intent
8. Utility engines are ONLY for technical problems, not creative effects"""
        
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
                    if "choices" not in data or not data["choices"]:
                        logger.error("OpenAI API returned empty choices")
                        return None
                    
                    content = data["choices"][0]["message"]["content"]
                    if not content:
                        logger.error("OpenAI API returned empty content")
                        return None
                    
                    try:
                        blueprint = json.loads(content)
                    except json.JSONDecodeError as e:
                        logger.error(f"Failed to parse OpenAI response as JSON: {e}")
                        logger.debug(f"Raw content: {content[:500]}...")
                        return None
                    
                    # Validate and ensure structure
                    blueprint = self._validate_blueprint(blueprint)
                    logger.debug(f"Generated blueprint with {len(blueprint.get('slots', []))} slots")
                    return blueprint
                else:
                    logger.error(f"OpenAI API error: {response.status_code} - {response.text[:500]}")
                    return None
                    
            except asyncio.TimeoutError:
                logger.warning(f"Cloud request timed out after {self.timeout}s")
                return None
            except httpx.ConnectError:
                logger.error("Failed to connect to OpenAI API - check network connection")
                return None
            except httpx.ReadTimeout:
                logger.warning(f"OpenAI API read timeout after {self.timeout}s")
                return None
            except Exception as e:
                logger.error(f"Cloud request failed: {type(e).__name__}: {str(e)}")
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
                {"engine_id": ENGINE_K_STYLE, "character": "driven"},  # K-Style Overdrive
                {"engine_id": ENGINE_VCA_COMPRESSOR, "character": "punchy"},   # Classic Compressor
                {"engine_id": ENGINE_PARAMETRIC_EQ, "character": "sculpted"},  # Parametric EQ
            ])
            blueprint["overall_vibe"] = "rock power"
            
        elif characteristics["genre"] == "electronic":
            slot_configs.extend([
                {"engine_id": ENGINE_BIT_CRUSHER, "character": "digital"},  # BitCrusher
                {"engine_id": ENGINE_STATE_VARIABLE_FILTER, "character": "sweeping"}, # State Variable Filter
                {"engine_id": ENGINE_DIGITAL_DELAY, "character": "rhythmic"}, # Digital Delay
            ])
            blueprint["overall_vibe"] = "electronic pulse"
            
        elif characteristics["genre"] == "ambient":
            slot_configs.extend([
                {"engine_id": ENGINE_SHIMMER_REVERB, "character": "ethereal"},  # Shimmer Reverb
                {"engine_id": ENGINE_TAPE_ECHO, "character": "vintage"},   # Tape Echo
                {"engine_id": ENGINE_DIGITAL_CHORUS, "character": "modulated"}, # Digital Chorus
            ])
            blueprint["overall_vibe"] = "ambient space"
            
        # Add mood-based effects
        if characteristics["mood"] == "dark":
            slot_configs.append({"engine_id": ENGINE_LADDER_FILTER, "character": "filtered"})
            blueprint["creative_analysis"]["mood"] = "dark"
            
        elif characteristics["mood"] == "bright":
            slot_configs.append({"engine_id": ENGINE_HARMONIC_EXCITER, "character": "harmonic"})
            blueprint["creative_analysis"]["mood"] = "bright"
            
        # Add texture effects
        if characteristics["texture"] == "warm":
            slot_configs.append({"engine_id": ENGINE_VINTAGE_TUBE, "character": "warm"})
            blueprint["creative_analysis"]["character"] = "warm"
            
        elif characteristics["texture"] == "cold":
            slot_configs.append({"engine_id": ENGINE_DIGITAL_DELAY, "character": "digital"})
            blueprint["creative_analysis"]["character"] = "cold"
            
        # Add dynamics based on intensity
        if characteristics["intensity"] == "aggressive":
            slot_configs.append({"engine_id": ENGINE_MASTERING_LIMITER, "character": "limiting"})
            blueprint["creative_analysis"]["intensity"] = 0.8
            
        elif characteristics["intensity"] == "subtle":
            slot_configs.append({"engine_id": ENGINE_OPTO_COMPRESSOR, "character": "smooth"})
            blueprint["creative_analysis"]["intensity"] = 0.3
            
        # Add spatial effects
        if characteristics["space"] == "wide":
            slot_configs.append({"engine_id": ENGINE_STEREO_IMAGER, "character": "wide"})
            blueprint["creative_analysis"]["space"] = 0.8
            
        elif characteristics["space"] == "tight":
            slot_configs.append({"engine_id": ENGINE_NOISE_GATE, "character": "gated"})
            blueprint["creative_analysis"]["space"] = 0.2
            
        # Handle special requests
        if "vocal" in prompt_lower or "voice" in prompt_lower:
            slot_configs.extend([
                {"engine_id": ENGINE_PHASED_VOCODER, "character": "robotic"},  # Phased Vocoder
                {"engine_id": ENGINE_VOCAL_FORMANT, "character": "shifted"},  # Vocal Formant
            ])
            blueprint["overall_vibe"] = "vocal processing"
            
        if "chaos" in prompt_lower or "random" in prompt_lower:
            slot_configs.append({"engine_id": ENGINE_CHAOS_GENERATOR, "character": "chaotic"})
            blueprint["overall_vibe"] = "controlled chaos"
            
        if "shimmer" in prompt_lower:
            slot_configs.append({"engine_id": ENGINE_SHIMMER_REVERB, "character": "shimmering"})
            blueprint["overall_vibe"] = "shimmering beauty"
            
        # Default if no specific patterns matched
        if not slot_configs:
            slot_configs = [
                {"engine_id": ENGINE_VCA_COMPRESSOR, "character": "balanced"},   # Classic Compressor
                {"engine_id": ENGINE_PARAMETRIC_EQ, "character": "neutral"},    # Parametric EQ
                {"engine_id": ENGINE_PLATE_REVERB, "character": "spacious"},  # Plate Reverb
            ]
            blueprint["overall_vibe"] = "balanced tone"
        
        # CRITICAL: Filter out any utility engines from slot configs
        slot_configs = [cfg for cfg in slot_configs if cfg["engine_id"] not in UTILITY_ENGINES]
        
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
                    "engine_id": ENGINE_NONE,  # None/bypass
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
        if any(eid in DISTORTION_ENGINES for eid in existing_ids):
            if ENGINE_VCA_COMPRESSOR not in existing_ids:
                complementary.append({"engine_id": ENGINE_VCA_COMPRESSOR, "character": "controlling"})
                
        # If we have reverb, consider delay
        reverb_engines = [ENGINE_PLATE_REVERB, ENGINE_SPRING_REVERB, ENGINE_CONVOLUTION_REVERB, ENGINE_SHIMMER_REVERB, ENGINE_GATED_REVERB]
        delay_engines = [ENGINE_TAPE_ECHO, ENGINE_DIGITAL_DELAY, ENGINE_BUCKET_BRIGADE_DELAY]
        if any(eid in reverb_engines for eid in existing_ids):
            if not any(eid in delay_engines for eid in existing_ids):
                complementary.append({"engine_id": ENGINE_TAPE_ECHO, "character": "echoing"})
                
        # If we have modulation, consider filter
        if any(eid in MODULATION_ENGINES for eid in existing_ids):
            if not any(eid in FILTER_ENGINES for eid in existing_ids):
                complementary.append({"engine_id": ENGINE_STATE_VARIABLE_FILTER, "character": "sweeping"})
                
        # Always good to have EQ
        eq_engines = [ENGINE_PARAMETRIC_EQ, ENGINE_VINTAGE_CONSOLE_EQ, ENGINE_DYNAMIC_EQ]
        if not any(eid in eq_engines for eid in existing_ids):
            complementary.append({"engine_id": ENGINE_PARAMETRIC_EQ, "character": "sculpting"})
        
        # CRITICAL: Ensure no utility engines in complementary effects
        complementary = [cfg for cfg in complementary if cfg["engine_id"] not in UTILITY_ENGINES]
            
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
            # Validate engine ID using authoritative validation
            if "engine_id" not in slot or not isinstance(slot["engine_id"], int):
                slot["engine_id"] = ENGINE_NONE
            else:
                if not validate_engine_id(slot["engine_id"]):
                    slot["engine_id"] = ENGINE_NONE
                # CRITICAL: Block utility engines from being used as primary effects
                elif slot["engine_id"] in UTILITY_ENGINES:
                    logger.warning(f"Blocked utility engine {slot['engine_id']} ({get_engine_name(slot['engine_id'])}) from slot {i+1}")
                    slot["engine_id"] = ENGINE_NONE
                    slot["character"] = "bypass"
                
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
            
            # Make API call using httpx for consistency
            headers = {
                "Authorization": f"Bearer {self.api_key}",
                "Content-Type": "application/json"
            }
            
            payload = {
                "model": self.model,  # Use configured model instead of hardcoded gpt-4
                "messages": [
                    {"role": "system", "content": system_prompt},
                    {"role": "user", "content": user_prompt}
                ],
                "response_format": {"type": "json_object"},
                "temperature": 0.7,
                "max_tokens": 500
            }
            
            async with httpx.AsyncClient() as client:
                response = await client.post(
                    self.api_url,
                    headers=headers,
                    json=payload,
                    timeout=self.timeout
                )
                
                if response.status_code != 200:
                    logger.error(f"OpenAI API error: {response.status_code} - {response.text}")
                    return self._local_modification_analysis(current_preset, modification_request)
                
                data = response.json()
                content = data["choices"][0]["message"]["content"]
                blueprint = json.loads(content)
            
            # Validate the blueprint structure
            blueprint = self._validate_modification_blueprint(blueprint)
            
            # Validate the blueprint structure
            blueprint = self._validate_modification_blueprint(blueprint)
            
            logger.info(f"Cloud modification analysis: {blueprint.get('intent', 'unknown')}")
            return blueprint
            
        except Exception as e:
            logger.error(f"Cloud modification analysis failed: {str(e)}")
            return self._local_modification_analysis(current_preset, modification_request)
    
    def _validate_modification_blueprint(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Validate and ensure modification blueprint has correct structure
        """
        # Ensure all required fields
        blueprint.setdefault("intent", "unknown modification")
        blueprint.setdefault("mood_shift", "adjusted")
        blueprint.setdefault("intensity_change", 0.0)
        blueprint.setdefault("parameter_targets", {})
        blueprint.setdefault("engine_suggestions", {"add": [], "remove": [], "modify": []})
        blueprint.setdefault("preserve_character", True)
        
        # Ensure intensity_change is within bounds
        intensity = blueprint.get("intensity_change", 0.0)
        if not isinstance(intensity, (int, float)):
            blueprint["intensity_change"] = 0.0
        else:
            blueprint["intensity_change"] = max(-1.0, min(1.0, intensity))
        
        # Ensure parameter_targets values are within bounds
        targets = blueprint.get("parameter_targets", {})
        for key, value in targets.items():
            if isinstance(value, (int, float)):
                targets[key] = max(-1.0, min(1.0, value))
            else:
                targets[key] = 0.0
        
        # Ensure engine_suggestions has the right structure
        suggestions = blueprint.get("engine_suggestions", {})
        for key in ["add", "remove", "modify"]:
            if key not in suggestions or not isinstance(suggestions[key], list):
                suggestions[key] = []
        
        return blueprint
    
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
                engine_name = get_engine_name(engine_id)
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
        
        # Check current engines using authoritative mapping
        for slot in range(1, 7):
            engine_id = parameters.get(f'slot{slot}_engine', ENGINE_NONE)
            if engine_id != ENGINE_NONE and parameters.get(f'slot{slot}_bypass', 0) < 0.5:
                engine_name = get_engine_name(engine_id).lower()
                if "reverb" in engine_name:
                    has_reverb = True
                elif "delay" in engine_name or "echo" in engine_name:
                    has_delay = True
                elif engine_id in DISTORTION_ENGINES:
                    has_distortion = True
                elif engine_id in MODULATION_ENGINES:
                    has_modulation = True
                elif engine_id == ENGINE_CHAOS_GENERATOR:
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
        
        # This was already updated above in the multi-edit
        
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

async def get_alter_instructions(current_preset: Dict[str, Any], alter_request: str) -> Dict[str, Any]:
    """
    Get alteration instructions for a preset based on a modification request.
    This converts modification analysis to a format compatible with the alter function.
    
    Args:
        current_preset: The current preset configuration
        alter_request: User's alteration request
        
    Returns:
        Alteration instructions with parameter adjustments, engine swaps, etc.
    """
    try:
        # Get modification analysis from Cloud AI
        analysis = await cloud_bridge.get_modification_analysis(current_preset, alter_request)
        
        # Convert to alter instructions format
        instructions = {
            "intent": analysis.get("intent", alter_request[:50]),
            "new_name": f"{current_preset.get('name', 'Preset')} (Altered)",
            "new_vibe": analysis.get("mood_shift", "altered"),
            "parameter_adjustments": [],
            "engine_swaps": []
        }
        
        # Convert parameter targets to parameter adjustments
        parameters = current_preset.get('parameters', {})
        targets = analysis.get('parameter_targets', {})
        
        # Map common parameter targets to actual parameter names
        param_mapping = {
            'reverb': ['slot1_mix', 'slot2_mix', 'slot3_mix', 'slot4_mix', 'slot5_mix', 'slot6_mix'],
            'delay': ['slot1_param1', 'slot2_param1', 'slot3_param1'],
            'drive': ['slot1_param1', 'slot1_param2'],
            'brightness': ['slot1_param3', 'slot2_param3'],
            'space': ['slot1_param4', 'slot2_param4'],
            'filter': ['slot1_param1', 'slot1_param2']
        }
        
        for target, adjustment in targets.items():
            if target in param_mapping:
                for param_name in param_mapping[target][:2]:  # Limit to first 2 to avoid too many changes
                    if param_name in parameters:
                        current_value = parameters[param_name]
                        new_value = max(0.0, min(1.0, current_value + adjustment))
                        instructions["parameter_adjustments"].append({
                            "parameter": param_name,
                            "value": new_value
                        })
        
        # Convert engine suggestions to engine swaps
        suggestions = analysis.get('engine_suggestions', {})
        add_engines = suggestions.get('add', [])
        
        if add_engines:
            # Find first empty slot for new engines
            for slot in range(1, 7):
                engine_param = f'slot{slot}_engine'
                if parameters.get(engine_param, 0) == 0:  # Empty slot
                    # Handle both string and dict engine suggestions
                    engine_name = add_engines[0]
                    if isinstance(engine_name, dict):
                        engine_name = engine_name.get('name', 'plate reverb')
                    engine_name = str(engine_name).lower()
                    # Map engine names to IDs using authoritative mapping
                    from engine_mapping_authoritative import ENGINE_PLATE_REVERB, ENGINE_SHIMMER_REVERB, ENGINE_GATED_REVERB, ENGINE_SPRING_REVERB, ENGINE_CONVOLUTION_REVERB, ENGINE_DIGITAL_DELAY, ENGINE_TAPE_ECHO, ENGINE_BUCKET_BRIGADE_DELAY, ENGINE_BIT_CRUSHER, ENGINE_VINTAGE_TUBE, ENGINE_MUFF_FUZZ, ENGINE_K_STYLE, ENGINE_DIGITAL_CHORUS, ENGINE_ANALOG_PHASER, ENGINE_RESONANT_CHORUS, ENGINE_CLASSIC_TREMOLO, ENGINE_CHAOS_GENERATOR
                    
                    engine_mapping = {
                        'plate reverb': ENGINE_PLATE_REVERB,
                        'shimmer reverb': ENGINE_SHIMMER_REVERB,
                        'gated reverb': ENGINE_GATED_REVERB,
                        'spring reverb': ENGINE_SPRING_REVERB,
                        'convolution reverb': ENGINE_CONVOLUTION_REVERB,
                        'digital delay': ENGINE_DIGITAL_DELAY,
                        'tape delay': ENGINE_TAPE_ECHO,
                        'tape echo': ENGINE_TAPE_ECHO,
                        'bucket brigade delay': ENGINE_BUCKET_BRIGADE_DELAY,
                        'bitcrusher': ENGINE_BIT_CRUSHER,
                        'tube saturation': ENGINE_VINTAGE_TUBE,
                        'vintage tube': ENGINE_VINTAGE_TUBE,
                        'fuzz pedal': ENGINE_MUFF_FUZZ,
                        'k-style overdrive': ENGINE_K_STYLE,
                        'classic chorus': ENGINE_DIGITAL_CHORUS,
                        'analog phaser': ENGINE_ANALOG_PHASER,
                        'vintage flanger': ENGINE_RESONANT_CHORUS,
                        'classic tremolo': ENGINE_CLASSIC_TREMOLO,
                        'chaos generator': ENGINE_CHAOS_GENERATOR
                    }
                    
                    engine_id = engine_mapping.get(engine_name, ENGINE_PLATE_REVERB)  # Default to plate reverb
                    instructions["engine_swaps"].append({
                        "slot": slot,
                        "new_engine_id": engine_id
                    })
                    break
        
        logger.info(f"Generated alter instructions: {len(instructions['parameter_adjustments'])} params, {len(instructions['engine_swaps'])} engines")
        return instructions
        
    except Exception as e:
        logger.error(f"Error generating alter instructions: {str(e)}")
        # Return basic fallback instructions
        return {
            "intent": alter_request[:50],
            "new_name": f"{current_preset.get('name', 'Preset')} (Altered)",
            "new_vibe": "altered",
            "parameter_adjustments": [],
            "engine_swaps": []
        }