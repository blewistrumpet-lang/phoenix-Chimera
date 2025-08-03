import asyncio
import json
import socket
import logging
from typing import Dict, Any, List

logger = logging.getLogger(__name__)

class VisionaryEnhanced:
    """
    Enhanced Visionary that understands all 53 Chimera Phoenix engines.
    Creates intelligent blueprints based on creative prompts.
    """
    
    def __init__(self, host: str = "localhost", port: int = 9999):
        self.host = host
        self.port = port
        
        # Complete engine mapping with categories and characteristics
        self.engines = {
            # Vintage Effects (0-9)
            0: {"name": "Vintage Tube", "category": "vintage", "traits": ["warm", "saturated", "harmonic", "smooth"]},
            1: {"name": "Tape Echo", "category": "delay", "traits": ["vintage", "warm", "analog", "modulated"]},
            2: {"name": "Shimmer Reverb", "category": "reverb", "traits": ["ethereal", "bright", "angelic", "spacious"]},
            3: {"name": "Plate Reverb", "category": "reverb", "traits": ["metallic", "classic", "studio", "dense"]},
            4: {"name": "Convolution Reverb", "category": "reverb", "traits": ["realistic", "natural", "space", "accurate"]},
            5: {"name": "Spring Reverb", "category": "reverb", "traits": ["vintage", "twangy", "drippy", "surf"]},
            6: {"name": "Opto Compressor", "category": "dynamics", "traits": ["smooth", "vintage", "musical", "gentle"]},
            7: {"name": "VCA Compressor", "category": "dynamics", "traits": ["punchy", "clean", "precise", "fast"]},
            8: {"name": "Magnetic Drum Echo", "category": "delay", "traits": ["vintage", "unique", "rhythmic", "mechanical"]},
            9: {"name": "Bucket Brigade Delay", "category": "delay", "traits": ["analog", "warm", "degraded", "lo-fi"]},
            
            # Modulation (11-12, 14-22)
            11: {"name": "Digital Chorus", "category": "modulation", "traits": ["lush", "wide", "shimmering", "stereo"]},
            12: {"name": "Analog Phaser", "category": "modulation", "traits": ["swirling", "vintage", "psychedelic", "warm"]},
            14: {"name": "Pitch Shifter", "category": "pitch", "traits": ["harmonic", "octave", "detune", "experimental"]},
            15: {"name": "Ring Modulator", "category": "modulation", "traits": ["metallic", "bell", "experimental", "inharmonic"]},
            16: {"name": "Granular Cloud", "category": "experimental", "traits": ["textural", "ambient", "scattered", "evolving"]},
            17: {"name": "Vocal Formant", "category": "filter", "traits": ["vocal", "resonant", "talking", "human"]},
            18: {"name": "Dimension Expander", "category": "spatial", "traits": ["wide", "3D", "immersive", "expansive"]},
            19: {"name": "Frequency Shifter", "category": "experimental", "traits": ["dissonant", "shifting", "alien", "unusual"]},
            20: {"name": "Transient Shaper", "category": "dynamics", "traits": ["punchy", "attack", "sharp", "percussive"]},
            21: {"name": "Harmonic Tremolo", "category": "modulation", "traits": ["vintage", "pulsing", "warm", "musical"]},
            22: {"name": "Classic Tremolo", "category": "modulation", "traits": ["rhythmic", "amplitude", "vintage", "simple"]},
            
            # Filters & EQ (23-30)
            23: {"name": "Comb Resonator", "category": "filter", "traits": ["resonant", "metallic", "pitched", "ringing"]},
            24: {"name": "Rotary Speaker", "category": "modulation", "traits": ["leslie", "spinning", "organ", "doppler"]},
            25: {"name": "Mid-Side Processor", "category": "spatial", "traits": ["stereo", "width", "control", "precise"]},
            26: {"name": "Vintage Console EQ", "category": "eq", "traits": ["warm", "musical", "analog", "colored"]},
            27: {"name": "Parametric EQ", "category": "eq", "traits": ["precise", "clean", "surgical", "flexible"]},
            28: {"name": "Ladder Filter", "category": "filter", "traits": ["resonant", "squelchy", "synth", "aggressive"]},
            29: {"name": "State Variable Filter", "category": "filter", "traits": ["versatile", "clean", "morphing", "multi-mode"]},
            30: {"name": "Formant Filter", "category": "filter", "traits": ["vocal", "vowel", "talking", "expressive"]},
            
            # Distortion & Saturation (31-38, excluding 37)
            31: {"name": "Wave Folder", "category": "distortion", "traits": ["folding", "complex", "harmonic", "synth"]},
            32: {"name": "Harmonic Exciter", "category": "saturation", "traits": ["bright", "presence", "air", "sparkle"]},
            33: {"name": "Bit Crusher", "category": "distortion", "traits": ["digital", "lofi", "crushed", "retro"]},
            34: {"name": "Multiband Saturator", "category": "saturation", "traits": ["controlled", "warm", "full", "balanced"]},
            35: {"name": "Muff Fuzz", "category": "distortion", "traits": ["fuzzy", "thick", "sustained", "wall"]},
            36: {"name": "Rodent Distortion", "category": "distortion", "traits": ["aggressive", "tight", "modern", "brutal"]},
            38: {"name": "K-Style Overdrive", "category": "distortion", "traits": ["warm", "smooth", "tube", "musical"]},
            
            # Spatial & Time (39-50)
            39: {"name": "Spectral Freeze", "category": "experimental", "traits": ["frozen", "spectral", "sustained", "ethereal"]},
            40: {"name": "Buffer Repeat", "category": "glitch", "traits": ["glitchy", "stutter", "rhythmic", "chopped"]},
            41: {"name": "Chaos Generator", "category": "experimental", "traits": ["random", "chaotic", "unpredictable", "wild"]},
            42: {"name": "Intelligent Harmonizer", "category": "pitch", "traits": ["harmonic", "smart", "musical", "polyphonic"]},
            43: {"name": "Gated Reverb", "category": "reverb", "traits": ["80s", "punchy", "drums", "dramatic"]},
            44: {"name": "Detune Doubler", "category": "pitch", "traits": ["thick", "chorus", "wide", "doubled"]},
            45: {"name": "Phased Vocoder", "category": "experimental", "traits": ["robotic", "vocoded", "synthetic", "talking"]},
            46: {"name": "Spectral Gate", "category": "dynamics", "traits": ["frequency", "selective", "clean", "precise"]},
            47: {"name": "Noise Gate", "category": "dynamics", "traits": ["clean", "quiet", "gating", "threshold"]},
            48: {"name": "Envelope Filter", "category": "filter", "traits": ["auto-wah", "dynamic", "responsive", "funky"]},
            49: {"name": "Feedback Network", "category": "experimental", "traits": ["feedback", "complex", "evolving", "unstable"]},
            50: {"name": "Mastering Limiter", "category": "dynamics", "traits": ["loud", "limiting", "transparent", "final"]},
            
            # Additional (51-55)
            51: {"name": "Stereo Widener", "category": "spatial", "traits": ["wide", "stereo", "spread", "immersive"]},
            52: {"name": "Resonant Chorus", "category": "modulation", "traits": ["resonant", "rich", "thick", "evolving"]},
            53: {"name": "Digital Delay", "category": "delay", "traits": ["clean", "precise", "digital", "modern"]},
            54: {"name": "Dynamic EQ", "category": "eq", "traits": ["responsive", "dynamic", "intelligent", "adaptive"]},
            55: {"name": "Stereo Imager", "category": "spatial", "traits": ["imaging", "placement", "stereo", "precise"]}
        }
        
        # Keywords to engine mapping for intelligent selection
        self.keyword_to_engines = {
            # Tone descriptors
            "warm": [0, 1, 9, 21, 26, 34, 38],
            "bright": [2, 32, 27],
            "dark": [35, 36, 28],
            "vintage": [0, 1, 5, 6, 8, 9, 12, 21, 22, 26],
            "modern": [11, 27, 36, 53, 54],
            "clean": [7, 27, 47, 53],
            "dirty": [31, 33, 35, 36, 38],
            
            # Effect types
            "reverb": [2, 3, 4, 5, 43],
            "delay": [1, 8, 9, 53],
            "echo": [1, 8, 9],
            "distortion": [31, 33, 35, 36, 38],
            "saturation": [0, 32, 34],
            "compression": [6, 7, 50],
            "modulation": [11, 12, 14, 15, 21, 22, 24],
            "filter": [17, 23, 28, 29, 30, 48],
            
            # Character
            "aggressive": [36, 35, 28],
            "gentle": [6, 0, 38],
            "spacious": [2, 3, 4, 18, 51],
            "tight": [7, 36, 47],
            "lush": [2, 11, 52],
            "experimental": [15, 16, 19, 39, 41, 45, 49],
            "psychedelic": [12, 15, 41],
            "ambient": [2, 16, 39],
            "glitchy": [33, 40, 41],
            
            # Specific uses
            "vocal": [0, 6, 17, 30, 45],
            "drums": [7, 20, 43, 47],
            "guitar": [0, 1, 5, 35, 36, 38],
            "synth": [28, 31, 15, 45],
            "bass": [6, 7, 34, 36],
            "master": [27, 34, 50, 54]
        }
        
        self.system_prompt = self._build_system_prompt()
    
    def _build_system_prompt(self) -> str:
        """Build comprehensive system prompt with all engine knowledge"""
        engine_list = "\n".join([
            f"- {id}: {info['name']} ({', '.join(info['traits'][:3])})"
            for id, info in self.engines.items()
        ])
        
        return f"""You are the Visionary, an advanced AI that creates audio effect presets for Chimera Phoenix.

You have access to 53 boutique DSP engines. Given a user's creative prompt, output ONLY a valid JSON object with no additional text.

AVAILABLE ENGINES:
{engine_list}

OUTPUT FORMAT (return ONLY this JSON structure):
{{
    "slots": [
        {{"slot": 1, "engine_id": <integer>, "character": "<string>"}},
        {{"slot": 2, "engine_id": <integer>, "character": "<string>"}},
        {{"slot": 3, "engine_id": <integer>, "character": "<string>"}},
        {{"slot": 4, "engine_id": <integer>, "character": "<string>"}},
        {{"slot": 5, "engine_id": <integer>, "character": "<string>"}},
        {{"slot": 6, "engine_id": <integer>, "character": "<string>"}}
    ],
    "overall_vibe": "<string>"
}}

RULES:
1. Use 1-6 slots based on complexity needed
2. Consider signal flow: earlier slots process first
3. Use -1 for engine_id to bypass unused slots
4. Match engines to user intent using their traits
5. Create synergistic combinations
6. Avoid redundant effects unless intentional
7. Character should be a single word from the engine's traits

SIGNAL FLOW TIPS:
- EQ/Filters early for tone shaping
- Distortion/Saturation before time effects
- Modulation in middle for movement
- Reverb/Delay at end for space
- Dynamics can go anywhere based on need

Be creative but practical. Focus on musicality. Return ONLY valid JSON."""

    async def get_blueprint(self, prompt: str) -> Dict[str, Any]:
        """
        Generate an intelligent blueprint based on the prompt
        """
        try:
            # Try actual OpenAI connection first
            try:
                message = {
                    "system": self.system_prompt,
                    "user": prompt
                }
                blueprint = await self._connect_to_bridge(message)
                return blueprint
            except (ConnectionRefusedError, OSError) as e:
                logger.warning(f"OpenAI bridge not available, using enhanced simulation: {str(e)}")
                # Use enhanced simulation
                blueprint = await self._simulate_enhanced_response(prompt)
                return blueprint
                
        except Exception as e:
            logger.error(f"Error in VisionaryEnhanced: {str(e)}")
            # Return safe default
            return self._get_default_blueprint()
    
    async def _simulate_enhanced_response(self, prompt: str) -> Dict[str, Any]:
        """
        Enhanced simulation using keyword analysis and intelligent engine selection
        """
        prompt_lower = prompt.lower()
        selected_engines = []
        
        # Analyze prompt for keywords and collect matching engines
        engine_scores = {}
        for keyword, engine_ids in self.keyword_to_engines.items():
            if keyword in prompt_lower:
                for engine_id in engine_ids:
                    if engine_id in engine_scores:
                        engine_scores[engine_id] += 1
                    else:
                        engine_scores[engine_id] = 1
        
        # Sort engines by score
        sorted_engines = sorted(engine_scores.items(), key=lambda x: x[1], reverse=True)
        
        # Build slot configuration
        slots = []
        used_categories = set()
        
        for engine_id, score in sorted_engines[:6]:  # Max 6 slots
            engine_info = self.engines[engine_id]
            category = engine_info["category"]
            
            # Avoid duplicate categories unless it makes sense
            if category in used_categories and category not in ["eq", "delay", "reverb"]:
                continue
                
            # Determine character based on prompt and engine traits
            character = self._determine_character(prompt_lower, engine_info["traits"])
            
            slots.append({
                "slot": len(slots) + 1,
                "engine_id": engine_id,
                "character": character
            })
            
            used_categories.add(category)
            
            if len(slots) >= 3:  # Usually 3-4 effects is enough
                break
        
        # Fill remaining slots with bypass
        while len(slots) < 6:
            slots.append({
                "slot": len(slots) + 1,
                "engine_id": -1,
                "character": "bypass"
            })
        
        # Determine overall vibe
        vibe = self._determine_vibe(prompt_lower, selected_engines)
        
        return {
            "slots": slots,
            "overall_vibe": vibe
        }
    
    def _determine_character(self, prompt: str, traits: List[str]) -> str:
        """Determine the best character trait based on prompt and available traits"""
        # Check if any trait appears in the prompt
        for trait in traits:
            if trait in prompt:
                return trait
        
        # Default to first trait
        return traits[0] if traits else "neutral"
    
    def _determine_vibe(self, prompt: str, engines: List[int]) -> str:
        """Determine overall vibe from prompt"""
        vibes = []
        
        # Common vibe patterns
        if "warm" in prompt and "vintage" in prompt:
            return "warm vintage"
        elif "aggressive" in prompt:
            return "aggressive power"
        elif "ambient" in prompt or "space" in prompt:
            return "spacious ambient"
        elif "clean" in prompt:
            return "pristine clarity"
        elif "experimental" in prompt:
            return "experimental sonic"
        elif "psychedelic" in prompt:
            return "psychedelic journey"
        elif "lo-fi" in prompt or "lofi" in prompt:
            return "vintage lofi"
        else:
            # Build from keywords
            keywords = ["warm", "bright", "dark", "lush", "tight", "spacious", "vintage", "modern"]
            found = [k for k in keywords if k in prompt]
            if found:
                return " ".join(found[:2])
            else:
                return "balanced tone"
    
    def _get_default_blueprint(self) -> Dict[str, Any]:
        """Return a safe default blueprint with correct engine IDs"""
        # Use actual engine IDs that exist in the plugin:
        # 47 = Noise Gate, 50 = Mastering Limiter, 54 = Dynamic EQ
        # But let's use working engines instead:
        # 2 = Classic Compressor, 19 = Parametric EQ, 21 = Plate Reverb
        return {
            "slots": [
                {"slot": 1, "engine_id": 2, "character": "warm"},  # Classic Compressor
                {"slot": 2, "engine_id": 19, "character": "balanced"},  # Parametric EQ  
                {"slot": 3, "engine_id": 21, "character": "spacious"},  # Plate Reverb
                {"slot": 4, "engine_id": 1, "character": "bypass"},  # Bypass
                {"slot": 5, "engine_id": 1, "character": "bypass"},  # Bypass
                {"slot": 6, "engine_id": 1, "character": "bypass"}  # Bypass
            ],
            "overall_vibe": "balanced warmth"
        }
    
    async def _connect_to_bridge(self, message: Dict[str, Any]) -> Dict[str, Any]:
        """Connect to OpenAI bridge service"""
        reader, writer = await asyncio.open_connection(self.host, self.port)
        
        try:
            # Send message
            message_json = json.dumps(message) + "\n"
            writer.write(message_json.encode())
            await writer.drain()
            
            # Read response
            data = await reader.readline()
            response = json.loads(data.decode())
            
            return response
            
        finally:
            writer.close()
            await writer.wait_closed()