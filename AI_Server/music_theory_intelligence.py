"""
Comprehensive Music Theory and Genre Intelligence Module
Deep understanding of musical concepts, genres, and production techniques
"""

from typing import Dict, Any, List, Tuple, Optional
from engine_mapping_authoritative import ENGINE_NAMES

class MusicTheoryIntelligence:
    """
    Deep musical knowledge for intelligent preset generation
    """
    
    def __init__(self):
        # Frequency spectrum understanding
        self.frequency_spectrum = {
            "sub_bass": {"range": (20, 60), "character": "rumble, power", "instruments": ["808", "sub"]},
            "bass": {"range": (60, 250), "character": "foundation, warmth", "instruments": ["bass", "kick"]},
            "low_mids": {"range": (250, 500), "character": "body, muddiness", "instruments": ["guitar_body", "snare_body"]},
            "mids": {"range": (500, 2000), "character": "presence, clarity", "instruments": ["vocals", "guitar", "piano"]},
            "high_mids": {"range": (2000, 4000), "character": "definition, harshness", "instruments": ["vocal_presence", "cymbals"]},
            "presence": {"range": (4000, 6000), "character": "brilliance, sibilance", "instruments": ["vocal_air", "cymbal_sizzle"]},
            "air": {"range": (6000, 20000), "character": "sparkle, air", "instruments": ["overtones", "ambience"]}
        }
        
        # Harmonic relationships
        self.harmonic_theory = {
            "even_harmonics": {
                "character": "warm, musical, pleasing",
                "engines": [15, 1],  # Tube, Opto
                "description": "2nd, 4th, 6th harmonics - vintage warmth"
            },
            "odd_harmonics": {
                "character": "aggressive, edgy, raw",
                "engines": [20, 21, 22],  # Fuzz, distortions
                "description": "3rd, 5th, 7th harmonics - aggressive edge"
            },
            "intermodulation": {
                "character": "complex, rich, dense",
                "engines": [26, 19],  # Ring mod, multiband saturator
                "description": "Sum and difference frequencies"
            }
        }
        
        # Dynamic range principles
        self.dynamics_theory = {
            "transient_control": {
                "attack": {"fast": "punchy, aggressive", "slow": "smooth, natural"},
                "sustain": {"enhanced": "powerful, full", "reduced": "tight, controlled"},
                "engines": [3, 2, 1]  # Transient shaper, compressors
            },
            "dynamic_range": {
                "wide": {"character": "natural, dynamic", "music": ["jazz", "classical"]},
                "moderate": {"character": "controlled, polished", "music": ["pop", "rock"]},
                "narrow": {"character": "loud, consistent", "music": ["metal", "edm"]}
            },
            "compression_types": {
                "optical": {"character": "smooth, musical", "engine": 1},
                "vca": {"character": "precise, transparent", "engine": 2},
                "limiting": {"character": "protective, loud", "engine": 5}
            }
        }
        
        # Spatial principles
        self.spatial_theory = {
            "stereo_field": {
                "mono": {"use": "bass, kick, lead vocal", "engines": [55]},
                "narrow": {"use": "centered elements", "width": 0.3},
                "wide": {"use": "pads, ambient", "engines": [44, 45, 46]},
                "super_wide": {"use": "special effects", "width": 1.0}
            },
            "depth_layers": {
                "close": {"reverb": 0.1, "character": "intimate, dry"},
                "mid": {"reverb": 0.3, "character": "natural, present"},
                "far": {"reverb": 0.6, "character": "distant, spacious"},
                "huge": {"reverb": 0.9, "character": "epic, cathedral"}
            },
            "reverb_types": {
                "plate": {"character": "bright, vintage", "engine": 39},
                "spring": {"character": "boingy, guitar", "engine": 40},
                "convolution": {"character": "realistic, natural", "engine": 41},
                "shimmer": {"character": "ethereal, modern", "engine": 42},
                "gated": {"character": "80s, punchy", "engine": 43}
            }
        }
        
        # Genre-specific knowledge
        self.genre_intelligence = {
            "pop": {
                "characteristics": ["bright", "clean", "compressed", "polished"],
                "typical_chain": [1, 7, 23, 39],  # Opto → EQ → Chorus → Plate
                "frequency_focus": ["high_mids", "presence"],
                "dynamics": "moderate",
                "space": "tight",
                "references": {
                    "billie_eilish": {"intimate": 0.9, "dark": 0.7, "minimal": 0.8},
                    "ariana_grande": {"bright": 0.9, "powerful": 0.8, "polished": 0.9},
                    "dua_lipa": {"punchy": 0.8, "retro": 0.6, "dance": 0.7}
                }
            },
            "rock": {
                "characteristics": ["powerful", "midrange", "roomy", "raw"],
                "typical_chain": [2, 8, 22, 35, 39],  # Comp → Console EQ → Overdrive → Delay → Reverb
                "frequency_focus": ["low_mids", "mids"],
                "dynamics": "moderate_wide",
                "space": "medium",
                "references": {
                    "foo_fighters": {"aggressive": 0.8, "compressed": 0.7, "powerful": 0.9},
                    "led_zeppelin": {"vintage": 0.9, "roomy": 0.8, "dynamic": 0.7},
                    "radiohead": {"experimental": 0.8, "textured": 0.9, "atmospheric": 0.7}
                }
            },
            "metal": {
                "characteristics": ["aggressive", "tight", "scooped", "heavy"],
                "typical_chain": [4, 7, 22, 5],  # Gate → EQ → Overdrive → Limiter
                "frequency_focus": ["bass", "presence"],
                "eq_curve": "scooped_mids",
                "dynamics": "narrow",
                "space": "tight",
                "references": {
                    "metallica": {"tight": 0.9, "aggressive": 0.9, "scooped": 0.8},
                    "meshuggah": {"djent": 0.9, "precise": 0.9, "heavy": 1.0},
                    "gojira": {"organic": 0.7, "powerful": 0.9, "atmospheric": 0.6}
                }
            },
            "jazz": {
                "characteristics": ["warm", "natural", "dynamic", "spacious"],
                "typical_chain": [1, 7, 39],  # Opto → EQ → Plate
                "frequency_focus": ["bass", "mids"],
                "dynamics": "wide",
                "space": "natural",
                "compression": "minimal",
                "references": {
                    "miles_davis": {"cool": 0.9, "spacious": 0.8, "innovative": 0.9},
                    "bill_evans": {"delicate": 0.9, "harmonic": 0.9, "intimate": 0.8},
                    "herbie_hancock": {"funky": 0.7, "fusion": 0.8, "electronic": 0.6}
                }
            },
            "electronic": {
                "characteristics": ["synthetic", "compressed", "filtered", "spacious"],
                "typical_chain": [9, 26, 35, 42],  # Filter → Ring Mod → Delay → Shimmer
                "frequency_focus": ["sub_bass", "presence", "air"],
                "dynamics": "narrow",
                "space": "designed",
                "modulation": "heavy",
                "sub_genres": {
                    "house": {"four_on_floor": 0.9, "warm": 0.7, "groovy": 0.8},
                    "techno": {"minimal": 0.8, "dark": 0.7, "repetitive": 0.9},
                    "dubstep": {"wobble": 0.9, "sub_heavy": 0.9, "spacious": 0.7},
                    "trap": {"808s": 0.9, "hi_hats": 0.9, "auto_tune": 0.8},
                    "ambient": {"textural": 0.9, "evolving": 0.8, "spacious": 0.9}
                }
            },
            "classical": {
                "characteristics": ["natural", "dynamic", "realistic", "spacious"],
                "typical_chain": [7, 41],  # EQ → Convolution
                "frequency_focus": ["full_spectrum"],
                "dynamics": "very_wide",
                "space": "concert_hall",
                "processing": "minimal",
                "instruments": {
                    "orchestra": {"depth": 0.9, "width": 0.9, "natural": 1.0},
                    "piano": {"intimate": 0.7, "resonant": 0.8, "dynamic": 0.9},
                    "strings": {"warm": 0.8, "rich": 0.9, "expressive": 0.9}
                }
            },
            "folk": {
                "characteristics": ["organic", "warm", "intimate", "natural"],
                "typical_chain": [1, 7, 40],  # Opto → EQ → Spring
                "frequency_focus": ["mids", "low_mids"],
                "dynamics": "natural",
                "space": "small_room",
                "character": "authentic"
            },
            "hip_hop": {
                "characteristics": ["punchy", "bass_heavy", "crisp", "wide"],
                "typical_chain": [2, 7, 19, 35],  # Comp → EQ → Saturator → Delay
                "frequency_focus": ["sub_bass", "bass", "presence"],
                "dynamics": "controlled",
                "space": "varied",
                "elements": {
                    "vocals": {"compressed": 0.8, "present": 0.9, "effects": 0.7},
                    "beats": {"punchy": 0.9, "wide": 0.7, "deep": 0.8},
                    "808s": {"sub_heavy": 0.9, "distorted": 0.6, "long": 0.7}
                }
            }
        }
        
        # Production techniques
        self.production_techniques = {
            "parallel_processing": {
                "description": "Blend processed with dry signal",
                "use_cases": ["drums", "vocals", "bass"],
                "typical_mix": 0.3
            },
            "sidechain_compression": {
                "description": "Duck one signal with another",
                "use_cases": ["edm", "bass_duck", "rhythmic_effects"],
                "engines": [2]
            },
            "multiband_processing": {
                "description": "Process frequency bands separately",
                "use_cases": ["mastering", "problem_solving"],
                "engines": [19, 6]
            },
            "serial_compression": {
                "description": "Multiple stages of gentle compression",
                "use_cases": ["vocals", "mastering"],
                "engines": [1, 2]
            },
            "new_york_compression": {
                "description": "Parallel compression on drums",
                "use_cases": ["drums", "punch"],
                "settings": {"ratio": 0.8, "attack": 0.1}
            }
        }
        
        # Instrument-specific processing
        self.instrument_processing = {
            "vocals": {
                "male": {
                    "fundamental": (80, 250),
                    "presence": (3000, 5000),
                    "typical_chain": [1, 7, 39],
                    "character": ["warm", "full", "present"]
                },
                "female": {
                    "fundamental": (160, 400),
                    "presence": (4000, 7000),
                    "typical_chain": [1, 7, 42],
                    "character": ["bright", "airy", "smooth"]
                },
                "processing": {
                    "compression": {"ratio": 0.4, "attack": 0.2},
                    "eq": {"low_cut": 80, "presence_boost": 5000},
                    "reverb": {"size": 0.3, "mix": 0.2}
                }
            },
            "guitar": {
                "electric": {
                    "frequency": (80, 5000),
                    "typical_chain": [4, 22, 25, 35],
                    "character": ["midrange", "cutting", "powerful"]
                },
                "acoustic": {
                    "frequency": (80, 12000),
                    "typical_chain": [1, 7, 23],
                    "character": ["warm", "natural", "full"]
                },
                "processing": {
                    "amp_sim": {"gain": 0.6, "tone": 0.5},
                    "effects": ["overdrive", "delay", "reverb"]
                }
            },
            "bass": {
                "electric": {
                    "frequency": (40, 2000),
                    "typical_chain": [2, 7, 15, 55],
                    "character": ["deep", "punchy", "solid"]
                },
                "synth": {
                    "frequency": (30, 500),
                    "typical_chain": [9, 19, 55],
                    "character": ["sub", "thick", "synthetic"]
                },
                "processing": {
                    "compression": {"ratio": 0.5, "attack": 0.3},
                    "eq": {"sub_boost": 60, "mid_cut": 500},
                    "mono": {"frequency": 120}
                }
            },
            "drums": {
                "kick": {
                    "frequency": (40, 100),
                    "typical_chain": [4, 3, 7],
                    "character": ["punchy", "deep", "tight"]
                },
                "snare": {
                    "frequency": (200, 500),
                    "typical_chain": [4, 3, 43],
                    "character": ["crack", "body", "bright"]
                },
                "overheads": {
                    "frequency": (500, 15000),
                    "typical_chain": [7, 44, 39],
                    "character": ["bright", "wide", "airy"]
                },
                "processing": {
                    "transients": {"attack": 0.7, "sustain": 0.4},
                    "gating": {"threshold": 0.3, "release": 0.2},
                    "reverb": {"size": 0.2, "mix": 0.1}
                }
            },
            "synth": {
                "lead": {
                    "frequency": (200, 8000),
                    "typical_chain": [9, 23, 35],
                    "character": ["cutting", "bright", "expressive"]
                },
                "pad": {
                    "frequency": (100, 10000),
                    "typical_chain": [10, 24, 42],
                    "character": ["wide", "evolving", "atmospheric"]
                },
                "bass": {
                    "frequency": (30, 500),
                    "typical_chain": [9, 19, 55],
                    "character": ["fat", "sub", "powerful"]
                }
            },
            "piano": {
                "grand": {
                    "frequency": (27, 4200),
                    "typical_chain": [1, 7, 41],
                    "character": ["full", "resonant", "dynamic"]
                },
                "upright": {
                    "frequency": (30, 4000),
                    "typical_chain": [1, 7, 40],
                    "character": ["intimate", "vintage", "woody"]
                },
                "processing": {
                    "compression": "minimal",
                    "eq": {"presence": 3000},
                    "reverb": {"hall": 0.4}
                }
            }
        }
    
    def analyze_prompt_musically(self, prompt: str) -> Dict[str, Any]:
        """
        Deep musical analysis of user prompt
        """
        prompt_lower = prompt.lower()
        analysis = {
            "genre": None,
            "instrument": None,
            "character": [],
            "frequency_focus": [],
            "dynamics": "moderate",
            "space": "medium",
            "techniques": [],
            "reference": None
        }
        
        # Detect genre
        for genre, data in self.genre_intelligence.items():
            if genre in prompt_lower:
                analysis["genre"] = genre
                analysis["character"].extend(data["characteristics"])
                analysis["dynamics"] = data.get("dynamics", "moderate")
                analysis["space"] = data.get("space", "medium")
                
                # Check for artist references
                if "references" in data:
                    for artist, qualities in data["references"].items():
                        if artist.replace("_", " ") in prompt_lower:
                            analysis["reference"] = artist
                            analysis["character"].extend(qualities.keys())
        
        # Detect instrument
        for instrument, data in self.instrument_processing.items():
            if instrument in prompt_lower:
                analysis["instrument"] = instrument
                
                # Check for specific types
                for subtype, subdata in data.items():
                    if isinstance(subdata, dict) and subtype in prompt_lower:
                        if "character" in subdata:
                            analysis["character"].extend(subdata["character"])
        
        # Detect frequency descriptors
        freq_keywords = {
            "bright": ["high_mids", "presence", "air"],
            "warm": ["bass", "low_mids"],
            "dark": ["bass", "low_mids"],
            "crisp": ["presence", "air"],
            "muddy": ["low_mids"],
            "thin": ["mids"],
            "full": ["bass", "mids"]
        }
        
        for keyword, frequencies in freq_keywords.items():
            if keyword in prompt_lower:
                analysis["frequency_focus"].extend(frequencies)
        
        # Detect production techniques
        technique_keywords = {
            "parallel": "parallel_processing",
            "sidechain": "sidechain_compression",
            "multiband": "multiband_processing",
            "new york": "new_york_compression"
        }
        
        for keyword, technique in technique_keywords.items():
            if keyword in prompt_lower:
                analysis["techniques"].append(technique)
        
        # Remove duplicates
        analysis["character"] = list(set(analysis["character"]))
        analysis["frequency_focus"] = list(set(analysis["frequency_focus"]))
        
        return analysis
    
    def get_processing_chain(self, analysis: Dict[str, Any]) -> List[int]:
        """
        Get recommended processing chain based on musical analysis
        """
        chain = []
        
        # Start with genre-specific chain if available
        if analysis["genre"] and analysis["genre"] in self.genre_intelligence:
            chain = self.genre_intelligence[analysis["genre"]]["typical_chain"].copy()
        
        # Or instrument-specific chain
        elif analysis["instrument"] and analysis["instrument"] in self.instrument_processing:
            inst_data = self.instrument_processing[analysis["instrument"]]
            if isinstance(inst_data, dict):
                for subtype, subdata in inst_data.items():
                    if isinstance(subdata, dict) and "typical_chain" in subdata:
                        chain = subdata["typical_chain"].copy()
                        break
        
        # Add character-specific engines
        if "warm" in analysis["character"] and 15 not in chain:
            chain.insert(1, 15)  # Add tube warmth
        
        if "aggressive" in analysis["character"] and not any(e in chain for e in [20, 21, 22]):
            chain.insert(2, 22)  # Add overdrive
        
        if "spacious" in analysis["character"] and not any(e in chain for e in [39, 40, 41, 42, 43]):
            chain.append(42)  # Add shimmer reverb
        
        return chain[:6]  # Limit to 6 slots
    
    def get_parameter_suggestions(self, engine_id: int, analysis: Dict[str, Any]) -> Dict[str, float]:
        """
        Get parameter suggestions based on musical analysis
        """
        suggestions = {}
        
        # Genre-specific parameters
        if analysis["genre"] == "metal" and engine_id == 7:  # EQ for metal
            suggestions["param3"] = 0.3  # Scoop mids
            suggestions["param6"] = 0.7  # Boost highs
        
        elif analysis["genre"] == "jazz" and engine_id == 1:  # Opto for jazz
            suggestions["param1"] = 0.2  # Gentle compression
            suggestions["param6"] = 0.1  # Minimal tube harmonics
        
        # Character-specific parameters
        if "warm" in analysis["character"]:
            if engine_id == 15:  # Tube
                suggestions["param1"] = 0.4  # Moderate drive
                suggestions["param2"] = 0.6  # Warm bias
            elif engine_id == 7:  # EQ
                suggestions["param0"] = 0.6  # Boost lows
                suggestions["param6"] = 0.3  # Roll off highs
        
        if "bright" in analysis["character"]:
            if engine_id == 17:  # Harmonic Exciter
                suggestions["param0"] = 0.4  # Good amount of harmonics
                suggestions["param1"] = 0.7  # High frequency focus
            elif engine_id == 7:  # EQ
                suggestions["param6"] = 0.7  # Boost highs
                suggestions["param7"] = 0.6  # Add presence
        
        # Space-specific parameters
        if analysis["space"] == "tight":
            if engine_id in [39, 40, 41, 42, 43]:  # Any reverb
                suggestions["param0"] = 0.2  # Small size
                suggestions["param1"] = 0.2  # Short decay
                suggestions["param5"] = 0.15  # Low mix
        
        elif analysis["space"] == "huge":
            if engine_id in [39, 40, 41, 42, 43]:  # Any reverb
                suggestions["param0"] = 0.9  # Large size
                suggestions["param1"] = 0.8  # Long decay
                suggestions["param5"] = 0.4  # Higher mix
        
        return suggestions


# Singleton instance
music_theory = MusicTheoryIntelligence()

def analyze_musical_intent(prompt: str) -> Dict[str, Any]:
    """Convenience function to analyze musical intent"""
    return music_theory.analyze_prompt_musically(prompt)

def get_genre_chain(genre: str) -> List[int]:
    """Get typical processing chain for a genre"""
    if genre in music_theory.genre_intelligence:
        return music_theory.genre_intelligence[genre]["typical_chain"]
    return []

def get_instrument_processing(instrument: str) -> Dict[str, Any]:
    """Get processing recommendations for an instrument"""
    if instrument in music_theory.instrument_processing:
        return music_theory.instrument_processing[instrument]
    return {}