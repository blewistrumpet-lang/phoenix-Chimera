"""
Enhanced Cloud Bridge (Visionary) - Better creative interpretation and naming
"""

import os
import json
import logging
import httpx
import asyncio
from typing import Dict, Any, Optional

logger = logging.getLogger(__name__)

class CloudBridgeEnhanced:
    """
    The Visionary - Translates ALL prompts (technical or poetic) into:
    1. Creative, relevant preset names
    2. Technical blueprint for Oracle
    3. Mood/character analysis
    """
    
    def __init__(self):
        self.api_key = os.getenv("OPENAI_API_KEY")
        if not self.api_key:
            logger.warning("No OpenAI API key found, will fall back to local generation")
        
        self.api_url = "https://api.openai.com/v1/chat/completions"
        self.model = "gpt-3.5-turbo"
        self.timeout = 8.0
        
        logger.info(f"Cloud Bridge Enhanced initialized (API key: {'✓' if self.api_key else '✗'})")
    
    async def get_cloud_generation(self, prompt: str) -> Optional[Dict[str, Any]]:
        """
        Generate a creative interpretation and technical blueprint
        """
        if not self.api_key:
            return self._generate_local_fallback(prompt)
        
        headers = {
            "Authorization": f"Bearer {self.api_key}",
            "Content-Type": "application/json"
        }
        
        # Enhanced system prompt with better creative interpretation
        system_prompt = """You are the VISIONARY component of an audio preset system.
Your job is to:
1. Interpret ANY prompt (technical or poetic) and create a CREATIVE, RELEVANT name
2. Translate the user's intent into technical requirements
3. Understand metaphors and creative descriptions

NAMING RULES:
- Names MUST reflect the prompt content
- Include instrument if mentioned (guitar, bass, vocals, etc.)
- Include genre if apparent (jazz, metal, EDM, etc.)  
- Capture the mood/character (warm, aggressive, ethereal, etc.)
- Be creative but relevant
- NEVER use generic names like "Sonic X" or "Custom Preset"

EXAMPLES OF GOOD NAMES:
- "Warm vintage 1960s bass" → "Motown Bass Groove"
- "Underwater dreamy vocals" → "Submerged Dream Voice"
- "EDM supersaw lead" → "Festival Supersaw Anthem"
- "Make it sound like outer space" → "Cosmic Void Explorer"
- "Aggressive metal guitar" → "Shredder's Inferno"
- "Professional mastering chain" → "Studio Master Polish"

CREATIVE INTERPRETATION:
- "underwater" → chorus, phaser, heavy reverb, filtered
- "outer space" → shimmer, delay, dimension expander, ethereal
- "vintage" → tube, tape, spring reverb, warm compression
- "modern" → digital effects, precision EQ, clean dynamics
- "dreamy" → reverb, chorus, slow attack, soft
- "aggressive" → distortion, gate, compression, tight
- "crystalline" → bright EQ, exciter, clean reverb
- "lo-fi" → bit crusher, tape, limited bandwidth
- "professional" → balanced, controlled, polished

ENGINE CATEGORIES:
DYNAMICS (1-5): Compressors, Gates, Limiters
EQ & FILTERS (6-14): EQs, Filters  
DISTORTION & SATURATION (15-22): Tubes, Overdrives, Fuzz
MODULATION (23-33): Chorus, Phaser, Flanger, etc.
TIME EFFECTS (34-38): Delays, Echoes
REVERB (39-43): Various reverb types
SPATIAL (44-52): Wideners, Special effects

Output JSON:
{
    "slots": [...],
    "creative_name": "<MUST be creative and match the prompt>",
    "technical_translation": "<technical description for Oracle>",
    "character_tags": ["tag1", "tag2", "tag3"],
    "creative_analysis": {
        "mood": "<primary mood>",
        "genre": "<detected genre or 'universal'>",
        "instrument": "<detected instrument or 'any'>",
        "intensity": <0.0-1.0>,
        "space": <0.0-1.0>,
        "warmth": <0.0-1.0>
    }
}"""
        
        payload = {
            "model": self.model,
            "messages": [
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": f"Create preset for: {prompt}"}
            ],
            "temperature": 0.8,
            "max_tokens": 600,
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
                    content = data.get("choices", [{}])[0].get("message", {}).get("content", "")
                    
                    if content:
                        blueprint = json.loads(content)
                        logger.info(f"✨ Visionary created: '{blueprint.get('creative_name', 'Unknown')}'")
                        return blueprint
                    
            except Exception as e:
                logger.warning(f"Cloud generation failed: {e}")
        
        return self._generate_local_fallback(prompt)
    
    def _generate_local_fallback(self, prompt: str) -> Dict[str, Any]:
        """
        Enhanced local fallback with better creative interpretation
        """
        prompt_lower = prompt.lower()
        
        # Extract key information
        instrument = self._detect_instrument(prompt_lower)
        genre = self._detect_genre(prompt_lower) 
        mood = self._detect_mood(prompt_lower)
        character = self._detect_character(prompt_lower)
        
        # Generate creative name
        name_parts = []
        
        # Add mood/character
        if mood:
            mood_names = {
                "warm": ["Warm", "Cozy", "Analog"],
                "aggressive": ["Raging", "Brutal", "Fierce"],
                "ethereal": ["Ethereal", "Floating", "Celestial"],
                "dark": ["Dark", "Shadow", "Midnight"],
                "bright": ["Bright", "Sparkling", "Crystal"],
                "vintage": ["Vintage", "Classic", "Retro"],
                "modern": ["Modern", "Digital", "Contemporary"]
            }
            if mood in mood_names:
                name_parts.append(mood_names[mood][0])
        
        # Add genre
        if genre:
            genre_names = {
                "jazz": "Jazz",
                "metal": "Metal", 
                "edm": "EDM",
                "rock": "Rock",
                "country": "Country",
                "ambient": "Ambient",
                "pop": "Pop",
                "hiphop": "Hip-Hop"
            }
            if genre in genre_names:
                name_parts.append(genre_names[genre])
        
        # Add instrument
        if instrument:
            inst_names = {
                "guitar": "Guitar",
                "bass": "Bass",
                "vocal": "Voice",
                "drums": "Drums",
                "piano": "Piano",
                "synth": "Synth"
            }
            if instrument in inst_names:
                name_parts.append(inst_names[instrument])
        
        # Add character descriptor
        if character:
            name_parts.append(character.title())
        
        # Create name
        if name_parts:
            creative_name = " ".join(name_parts[:3])  # Limit to 3 parts
        else:
            # Fallback to extracting meaningful words
            words = prompt.split()[:3]
            creative_name = " ".join(w.title() for w in words if len(w) > 3)
        
        # Generate technical translation
        technical = []
        if "compression" in prompt_lower or "compress" in prompt_lower:
            technical.append("dynamics control")
        if "reverb" in prompt_lower:
            technical.append("spatial processing")
        if "distortion" in prompt_lower or "overdrive" in prompt_lower:
            technical.append("harmonic saturation")
        if "delay" in prompt_lower or "echo" in prompt_lower:
            technical.append("time-based effects")
        if "eq" in prompt_lower or "equalization" in prompt_lower:
            technical.append("frequency shaping")
        
        technical_translation = ", ".join(technical) if technical else "balanced processing"
        
        # Detect intensity and space
        intensity = 0.7 if any(w in prompt_lower for w in ["aggressive", "heavy", "extreme", "maximum"]) else 0.4
        space = 0.8 if any(w in prompt_lower for w in ["huge", "wide", "spacious", "cathedral"]) else 0.3
        warmth = 0.7 if any(w in prompt_lower for w in ["warm", "vintage", "analog", "tube"]) else 0.4
        
        return {
            "slots": [],  # Will be filled by engine extraction
            "creative_name": creative_name,
            "technical_translation": technical_translation,
            "character_tags": [mood, genre, character] if any([mood, genre, character]) else ["universal"],
            "creative_analysis": {
                "mood": mood or "neutral",
                "genre": genre or "universal", 
                "instrument": instrument or "any",
                "intensity": intensity,
                "space": space,
                "warmth": warmth
            }
        }
    
    def _detect_instrument(self, text: str) -> Optional[str]:
        """Detect instrument from prompt"""
        instruments = {
            "guitar": ["guitar", "strat", "les paul", "acoustic", "electric"],
            "bass": ["bass", "sub", "low end"],
            "vocal": ["vocal", "voice", "singer", "vox"],
            "drums": ["drum", "kick", "snare", "percussion"],
            "piano": ["piano", "keys", "keyboard"],
            "synth": ["synth", "synthesizer", "lead", "pad"]
        }
        
        for inst, keywords in instruments.items():
            if any(kw in text for kw in keywords):
                return inst
        return None
    
    def _detect_genre(self, text: str) -> Optional[str]:
        """Detect genre from prompt"""
        genres = {
            "jazz": ["jazz", "swing", "bebop", "smooth jazz"],
            "metal": ["metal", "djent", "thrash", "death metal"],
            "edm": ["edm", "electronic", "techno", "house", "dubstep", "trance"],
            "rock": ["rock", "indie", "alternative", "punk"],
            "country": ["country", "nashville", "western"],
            "ambient": ["ambient", "atmospheric", "soundscape"],
            "pop": ["pop", "mainstream", "commercial"],
            "hiphop": ["hip hop", "hip-hop", "trap", "rap", "808"]
        }
        
        for genre, keywords in genres.items():
            if any(kw in text for kw in keywords):
                return genre
        return None
    
    def _detect_mood(self, text: str) -> Optional[str]:
        """Detect mood from prompt"""
        moods = {
            "warm": ["warm", "cozy", "intimate", "soft"],
            "aggressive": ["aggressive", "brutal", "heavy", "intense"],
            "ethereal": ["ethereal", "dreamy", "floating", "atmospheric"],
            "dark": ["dark", "evil", "sinister", "horror"],
            "bright": ["bright", "sparkly", "crisp", "clear"],
            "vintage": ["vintage", "retro", "classic", "old"],
            "modern": ["modern", "contemporary", "current", "new"]
        }
        
        for mood, keywords in moods.items():
            if any(kw in text for kw in keywords):
                return mood
        return None
    
    def _detect_character(self, text: str) -> Optional[str]:
        """Detect character descriptors"""
        # Look for descriptive adjectives
        descriptors = [
            "punchy", "smooth", "gritty", "clean", "dirty", "lush",
            "thick", "thin", "wide", "narrow", "deep", "shallow",
            "crispy", "mellow", "harsh", "gentle", "wild", "controlled"
        ]
        
        for desc in descriptors:
            if desc in text:
                return desc
        return None