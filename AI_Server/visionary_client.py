"""
VisionaryClient - TCP client for communicating with the Visionary AI service
Implements the specified TCP bridge architecture for the Trinity Pipeline
"""

import asyncio
import json
import logging
from typing import Dict, Any, Optional
from pathlib import Path

logger = logging.getLogger(__name__)

class VisionaryClient:
    """
    TCP client that connects to a Visionary bridge server.
    Sends prompts and receives complete JSON blueprints.
    Includes robust fallback mechanism for when TCP connection fails.
    """
    
    def __init__(self, host: str = "localhost", port: int = 9999):
        """
        Initialize the Visionary TCP client.
        
        Args:
            host: Bridge server hostname
            port: Bridge server port
        """
        self.host = host
        self.port = port
        self.timeout = 30.0  # 30 second timeout for AI generation
        self.retry_count = 3
        self.retry_delay = 2.0
        
        # Load engine definitions for fallback
        self.engines = self._load_engine_definitions()
        
        # System prompt for consistency
        self.system_prompt = """You are the Visionary, an AI that creates audio effect presets.

Given a user's creative prompt, output a JSON blueprint with this exact structure:

{
    "slots": [
        {
            "slot": 1,
            "engine_id": <engine_id>,
            "character": "<descriptive character>"
        },
        // ... 6 slots total
    ],
    "overall_vibe": "<2-3 word vibe description>",
    "creative_analysis": {
        "mood": "<mood>",
        "intensity": <0.0-1.0>,
        "space": <0.0-1.0>,
        "character": "<character>"
    }
}

Rules:
1. Always provide exactly 6 slots
2. Use -1 for bypass, or valid engine IDs (0-118)
3. Consider signal flow: earlier slots process first
4. Match engines to the user's creative intent
{
    "slots": [
        {"slot": 1, "engine_id": 0, "character": "warm"},
        {"slot": 2, "engine_id": 1, "character": "vintage"}
    ],
    "overall_vibe": "warm vintage tone"
}"""

    async def get_blueprint(self, prompt: str) -> Dict[str, Any]:
        """
        Get a creative blueprint from the Visionary service.
        Attempts TCP connection first, falls back to simulation.
        
        Args:
            prompt: User's creative prompt
            
        Returns:
            Blueprint with exactly 6 slots and creative analysis
        """
        # Try TCP connection with retries
        for attempt in range(self.retry_count):
            try:
                logger.info(f"Attempting TCP connection (attempt {attempt + 1}/{self.retry_count})")
                
                # Create message for bridge
                message = {
                    "system": self.system_prompt,
                    "user": prompt
                }
                
                blueprint = await self._connect_to_bridge(message)
                return self._ensure_six_slots(blueprint)
                
            except (ConnectionRefusedError, OSError, asyncio.TimeoutError) as e:
                logger.warning(f"TCP attempt {attempt + 1} failed: {str(e)}")
                if attempt < self.retry_count - 1:
                    await asyncio.sleep(self.retry_delay)
            except Exception as e:
                logger.error(f"Unexpected error in TCP connection: {str(e)}")
                break
        
        # Fall back to simulation
        logger.info("Using fallback simulation for blueprint generation")
        try:
            blueprint = await self._simulate_openai_response(prompt)
            return self._ensure_six_slots(blueprint)
        except Exception as e:
            logger.error(f"Simulation failed: {str(e)}")
            
            # Return safe default blueprint with 6 slots
            return {
                "slots": [
                    {"slot": 1, "engine_id": 0, "character": "neutral"},
                    {"slot": 2, "engine_id": -1, "character": "bypass"},
                    {"slot": 3, "engine_id": -1, "character": "bypass"},
                    {"slot": 4, "engine_id": -1, "character": "bypass"},
                    {"slot": 5, "engine_id": -1, "character": "bypass"},
                    {"slot": 6, "engine_id": -1, "character": "bypass"}
                ],
                "overall_vibe": "default preset",
                "creative_analysis": {
                    "mood": "neutral",
                    "intensity": 0.5,
                    "space": 0.5,
                    "character": "balanced"
                }
            }
    
    async def _simulate_openai_response(self, prompt: str) -> Dict[str, Any]:
        """
        Enhanced fallback simulation for when TCP connection fails.
        Provides intelligent preset generation based on prompt analysis.
        """
        import random
        import hashlib
        
        prompt_lower = prompt.lower()
        
        # Use hash of prompt to ensure different prompts get different presets
        prompt_hash = hashlib.md5(prompt.encode()).hexdigest()
        random.seed(prompt_hash)
        
        # Initialize blueprint with 6 slots
        blueprint = {
            "slots": [],
            "overall_vibe": "balanced tone",
            "creative_analysis": {
                "mood": "neutral",
                "intensity": 0.5,
                "space": 0.5,
                "character": "balanced"
            },
            "creative_name": self._generate_creative_name(prompt)
        }
        
        # Analyze prompt and build slot configuration with actual engine IDs
        slot_configs = []
        
        # Map keywords to actual engine IDs from our plugin
        if "vintage" in prompt_lower or "retro" in prompt_lower or "warm" in prompt_lower:
            slot_configs.append({"engine_id": 1, "character": "warm"})  # Vintage Opto Compressor
            slot_configs.append({"engine_id": 15, "character": "vintage"})  # Vintage Tube Drive
            slot_configs.append({"engine_id": 41, "character": "analog"})  # Vintage Tape Echo
            blueprint["overall_vibe"] = "vintage warmth"
            blueprint["creative_analysis"]["character"] = "vintage"
            
        elif "space" in prompt_lower or "ambient" in prompt_lower or "atmospheric" in prompt_lower:
            slot_configs.append({"engine_id": 42, "character": "spacious"})  # Plate Reverb Studio
            slot_configs.append({"engine_id": 44, "character": "ethereal"})  # Gated Reverb
            slot_configs.append({"engine_id": 33, "character": "wide"})  # Classic Chorus
            blueprint["overall_vibe"] = "ambient space"
            blueprint["creative_analysis"]["space"] = 0.9
            
        elif "aggressive" in prompt_lower or "heavy" in prompt_lower or "distort" in prompt_lower:
            slot_configs.append({"engine_id": 16, "character": "aggressive"})  # Soft Distortion
            slot_configs.append({"engine_id": 18, "character": "punchy"})  # K-Style Overdrive  
            slot_configs.append({"engine_id": 2, "character": "tight"})  # Classic Compressor
            blueprint["overall_vibe"] = "heavy drive"
            blueprint["creative_analysis"]["intensity"] = 0.8
            
        elif "clean" in prompt_lower or "pristine" in prompt_lower or "crystal" in prompt_lower:
            slot_configs.append({"engine_id": 8, "character": "transparent"})  # Parametric EQ Studio
            slot_configs.append({"engine_id": 5, "character": "clean"})  # Noise Gate
            slot_configs.append({"engine_id": 43, "character": "spatial"})  # Spring Reverb Studio
            blueprint["overall_vibe"] = "clean space"
            blueprint["creative_analysis"]["character"] = "modern"
            
        elif "echo" in prompt_lower or "delay" in prompt_lower:
            slot_configs.append({"engine_id": 40, "character": "rhythmic"})  # Tempo Delay Studio
            slot_configs.append({"engine_id": 41, "character": "vintage"})  # Vintage Tape Echo
            slot_configs.append({"engine_id": 42, "character": "spatial"})  # Plate Reverb Studio
            blueprint["overall_vibe"] = "echoing depth"
            blueprint["creative_analysis"]["space"] = 0.7
            
        elif "shimmer" in prompt_lower or "sparkle" in prompt_lower:
            slot_configs.append({"engine_id": 46, "character": "shimmering"})  # Shimmer Reverb
            slot_configs.append({"engine_id": 49, "character": "bright"})  # Simple Pitch Shift
            slot_configs.append({"engine_id": 33, "character": "modulated"})  # Classic Chorus
            blueprint["overall_vibe"] = "sparkling shimmer"
            blueprint["creative_analysis"]["space"] = 0.8
            
        elif "lo-fi" in prompt_lower or "lofi" in prompt_lower or "bit" in prompt_lower:
            slot_configs.append({"engine_id": 20, "character": "crushed"})  # BitCrusher
            slot_configs.append({"engine_id": 41, "character": "warbled"})  # Vintage Tape Echo
            slot_configs.append({"engine_id": 11, "character": "filtered"})  # Vintage Lowpass Filter
            blueprint["overall_vibe"] = "lo-fi character"
            blueprint["creative_analysis"]["character"] = "lo-fi"
            
        elif "voice" in prompt_lower or "vocal" in prompt_lower or "talk" in prompt_lower:
            slot_configs.append({"engine_id": 52, "character": "robotic"})  # Vocoder Platinum
            slot_configs.append({"engine_id": 51, "character": "shifting"})  # Formant Shifter
            slot_configs.append({"engine_id": 1, "character": "smooth"})  # Vintage Opto Compressor
            blueprint["overall_vibe"] = "vocal processing"
            blueprint["creative_analysis"]["character"] = "vocal"
            
        elif "chaos" in prompt_lower or "random" in prompt_lower or "wild" in prompt_lower:
            slot_configs.append({"engine_id": 54, "character": "chaotic"})  # Chaos Generator
            slot_configs.append({"engine_id": 37, "character": "glitchy"})  # Stutter Edit
            slot_configs.append({"engine_id": 20, "character": "digital"})  # BitCrusher
            blueprint["overall_vibe"] = "controlled chaos"
            blueprint["creative_analysis"]["intensity"] = 0.9
            
        elif "psychedelic" in prompt_lower or "trippy" in prompt_lower:
            slot_configs.append({"engine_id": 35, "character": "phasing"})  # Analog Phaser
            slot_configs.append({"engine_id": 36, "character": "sweeping"})  # Classic Flanger
            slot_configs.append({"engine_id": 46, "character": "atmospheric"})  # Shimmer Reverb
            blueprint["overall_vibe"] = "psychedelic swirl"
            blueprint["creative_analysis"]["space"] = 0.85
            
        else:
            # Default configuration - pick some interesting engines randomly
            available_engines = [
                {"engine_id": 2, "character": "punchy"},    # Classic Compressor
                {"engine_id": 8, "character": "sculpted"},  # Parametric EQ Studio
                {"engine_id": 15, "character": "warm"},     # Vintage Tube Drive
                {"engine_id": 33, "character": "modulated"}, # Classic Chorus
                {"engine_id": 42, "character": "spacious"},  # Plate Reverb Studio
                {"engine_id": 40, "character": "rhythmic"}   # Tempo Delay Studio
            ]
            # Pick 2-3 random engines for variety
            num_engines = random.randint(2, 3)
            slot_configs = random.sample(available_engines, num_engines)
            blueprint["overall_vibe"] = "experimental blend"
        
        # Build 6 slots from configs
        for i in range(6):
            if i < len(slot_configs):
                slot = {
                    "slot": i + 1,
                    "engine_id": slot_configs[i]["engine_id"],
                    "character": slot_configs[i]["character"]
                }
            else:
                # Fill remaining with bypass (0 = None)
                slot = {
                    "slot": i + 1,
                    "engine_id": 0,
                    "character": "bypass"
                }
            blueprint["slots"].append(slot)
        
        return blueprint
    
    def _generate_creative_name(self, prompt: str) -> str:
        """Generate a creative preset name based on the prompt"""
        import random
        
        # Extract key words from prompt
        words = prompt.lower().split()
        
        # Creative name templates
        templates = [
            "Sonic {adjective}",
            "{adjective} {noun}",
            "The {adjective} {noun}",
            "{noun} Dreams",
            "{adjective} Horizons",
            "Crystal {noun}",
            "{noun} Cascade",
            "Quantum {noun}",
            "{adjective} Echo",
            "Neon {noun}"
        ]
        
        # Word banks based on common audio descriptors
        adjectives = [
            "Ethereal", "Cosmic", "Digital", "Analog", "Crystal", "Velvet",
            "Golden", "Silver", "Electric", "Magnetic", "Prismatic", "Lunar",
            "Solar", "Nebula", "Quantum", "Fractal", "Harmonic", "Spectral"
        ]
        
        nouns = [
            "Garden", "Voyage", "Horizon", "Cascade", "Prism", "Portal",
            "Chamber", "Cathedral", "Circuit", "Dream", "Matrix", "Waves",
            "Echoes", "Resonance", "Spectrum", "Dimension", "Paradise", "Odyssey"
        ]
        
        # Try to extract adjectives and nouns from prompt
        prompt_adjectives = []
        prompt_nouns = []
        
        keyword_adjectives = ["warm", "cold", "bright", "dark", "soft", "hard", 
                             "vintage", "modern", "clean", "dirty", "smooth", "rough"]
        keyword_nouns = ["space", "room", "hall", "chamber", "garden", "forest",
                        "ocean", "sky", "mountain", "valley", "city", "temple"]
        
        for word in words:
            if word in keyword_adjectives:
                prompt_adjectives.append(word.capitalize())
            elif word in keyword_nouns:
                prompt_nouns.append(word.capitalize())
        
        # Use prompt words if found, otherwise random
        adj = random.choice(prompt_adjectives) if prompt_adjectives else random.choice(adjectives)
        noun = random.choice(prompt_nouns) if prompt_nouns else random.choice(nouns)
        
        # Pick a random template and fill it
        template = random.choice(templates)
        name = template.format(adjective=adj, noun=noun)
        
        # Special cases for specific keywords
        if "shimmer" in prompt.lower():
            name = f"Shimmer {noun}"
        elif "chaos" in prompt.lower():
            name = f"Chaos {noun}"
        elif "vintage" in prompt.lower():
            name = f"Vintage {noun}"
        elif "space" in prompt.lower():
            name = f"Space {noun}"
            
        return name
    
    def _load_engine_definitions(self) -> Dict[int, Dict[str, Any]]:
        """Load engine definitions for fallback simulation"""
        try:
            from engine_definitions import ENGINES
            # Convert string keys to legacy numeric IDs for compatibility
            engine_map = {}
            id_counter = 0
            for key, engine in ENGINES.items():
                engine_map[id_counter] = {
                    "name": engine["name"],
                    "category": engine.get("category", "Unknown"),
                    "id": id_counter
                }
                id_counter += 1
            return engine_map
        except ImportError:
            # Minimal fallback engines for 6 slots
            return {
                -1: {"name": "Bypass", "category": "None"},
                0: {"name": "Vintage Tube Saturation", "category": "Distortion"},
                1: {"name": "Tape Echo", "category": "Delay"},
                2: {"name": "Plate Reverb", "category": "Reverb"},
                3: {"name": "Classic Compressor", "category": "Dynamics"},
                4: {"name": "Parametric EQ", "category": "EQ"},
                5: {"name": "Analog Chorus", "category": "Modulation"}
            }
    
    async def _connect_to_bridge(self, message: Dict[str, Any]) -> Dict[str, Any]:
        """
        Actual TCP connection to OpenAI bridge with robust error handling
        """
        try:
            reader, writer = await asyncio.wait_for(
                asyncio.open_connection(self.host, self.port),
                timeout=5.0
            )
            
            try:
                # Send message
                message_json = json.dumps(message) + "\n"
                writer.write(message_json.encode())
                await writer.drain()
                
                # Read response with timeout
                data = await asyncio.wait_for(
                    reader.readline(),
                    timeout=self.timeout
                )
                
                if not data:
                    raise ValueError("Empty response from bridge")
                
                response = json.loads(data.decode())
                
                # Ensure response has exactly 6 slots
                blueprint = self._ensure_six_slots(response)
                return blueprint
                
            finally:
                writer.close()
                await writer.wait_closed()
                
        except asyncio.TimeoutError:
            logger.error("TCP connection timed out")
            raise
        except Exception as e:
            logger.error(f"TCP bridge error: {str(e)}")
            raise
    
    def _ensure_six_slots(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Ensure blueprint has exactly 6 slots"""
        if "slots" not in blueprint:
            blueprint["slots"] = []
        
        # Pad with bypass slots if needed
        while len(blueprint["slots"]) < 6:
            blueprint["slots"].append({
                "slot": len(blueprint["slots"]) + 1,
                "engine_id": -1,
                "character": "bypass"
            })
        
        # Truncate if too many
        blueprint["slots"] = blueprint["slots"][:6]
        
        # Ensure slot numbers are correct
        for i, slot in enumerate(blueprint["slots"]):
            slot["slot"] = i + 1
        
        # Add creative_analysis if missing
        if "creative_analysis" not in blueprint:
            blueprint["creative_analysis"] = {
                "mood": "neutral",
                "intensity": 0.5,
                "space": 0.5,
                "character": "balanced"
            }
        
        return blueprint