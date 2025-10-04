"""
Enhanced Oracle with Deep Musical Intelligence
Understands what engines do and finds semantically appropriate presets
"""

import json
import logging
import numpy as np
import faiss
from typing import Dict, Any, List, Optional, Tuple
from pathlib import Path
from engine_mapping_authoritative import ENGINE_NAMES
from engine_knowledge_base import ENGINE_KNOWLEDGE, find_engines_for_use_case, describe_signal_chain

logger = logging.getLogger(__name__)

class OracleEnhanced:
    """
    Enhanced Oracle that truly understands musical intent and engine capabilities
    """
    
    def __init__(self, index_path: str, meta_path: str, presets_path: str):
        """Initialize with FAISS index and enhanced musical intelligence"""
        
        self.index_path = index_path
        self.meta_path = meta_path
        self.presets_path = presets_path
        
        # Load FAISS index
        self.index = self._load_index()
        self.metadata = self._load_metadata()
        self.presets = self._load_presets()
        
        # Musical intelligence
        self.musical_understanding = {
            "vocal_chain": [1, 7, 39],  # Compression → EQ → Reverb
            "guitar_chain": [4, 22, 35],  # Gate → Overdrive → Delay
            "bass_chain": [2, 7, 55],  # Comp → EQ → Mono
            "drums_chain": [4, 3, 39],  # Gate → Transient → Reverb
            "synth_chain": [9, 23, 35],  # Filter → Chorus → Delay
            "master_chain": [2, 7, 5],  # Comp → EQ → Limiter
        }
        
        # Semantic understanding of descriptors
        self.semantic_map = {
            "warm": {
                "engines": [1, 15, 39, 34],
                "avoid": [18, 16, 48],
                "params": {"drive": 0.3, "warmth": 0.6}
            },
            "bright": {
                "engines": [17, 7, 42],
                "avoid": [9, 10],
                "params": {"highs": 0.7, "presence": 0.6}
            },
            "aggressive": {
                "engines": [4, 20, 21, 22],
                "avoid": [1, 39],
                "params": {"drive": 0.8, "gate": 0.3}
            },
            "ambient": {
                "engines": [42, 39, 35, 46],
                "avoid": [4, 3, 18],
                "params": {"size": 0.8, "mix": 0.4}
            },
            "vintage": {
                "engines": [1, 15, 34, 40],
                "avoid": [41, 5, 6],
                "params": {"character": 0.7}
            },
            "modern": {
                "engines": [5, 6, 41, 42],
                "avoid": [34, 40, 1],
                "params": {"precision": 0.8}
            },
            "clean": {
                "engines": [2, 7, 54],
                "avoid": [15, 16, 17, 18, 19, 20, 21, 22],
                "params": {"clarity": 0.9}
            },
            "dirty": {
                "engines": [18, 16, 20, 21],
                "avoid": [54, 7],
                "params": {"grit": 0.7}
            }
        }
        
        logger.info(f"Oracle Enhanced initialized with {len(self.presets)} presets")
        logger.info("Musical intelligence and engine knowledge loaded")
    
    def find_best_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Find the best preset using deep musical understanding
        """
        logger.info(f"Finding best preset for blueprint: {blueprint.get('vibe', 'unknown')}")
        
        # Extract musical intent from blueprint
        intent = self._extract_musical_intent(blueprint)
        
        # Score all presets based on musical understanding
        scored_presets = []
        for preset in self.presets:
            score = self._score_preset_musically(preset, intent)
            scored_presets.append((score, preset))
        
        # Sort by score and take best
        scored_presets.sort(key=lambda x: x[0], reverse=True)
        
        if scored_presets and scored_presets[0][0] > 0:
            best_score, best_preset = scored_presets[0]
            logger.info(f"Found preset with score {best_score:.2f}: {best_preset.get('name', 'Unknown')}")
            
            # Explain why this preset was chosen
            explanation = self._explain_preset_choice(best_preset, intent)
            best_preset["oracle_explanation"] = explanation
            
            return best_preset
        
        # Fallback to FAISS search if musical scoring fails
        return self._faiss_fallback_search(blueprint)
    
    def _extract_musical_intent(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Extract deep musical intent from blueprint
        """
        intent = {
            "required_engines": [],
            "avoid_engines": [],
            "target_params": {},
            "instrument": None,
            "genre": None,
            "character": [],
            "reference": None
        }
        
        vibe = blueprint.get("vibe", "").lower()
        
        # Detect instrument
        instruments = ["vocal", "guitar", "bass", "drums", "synth", "piano", "strings"]
        for inst in instruments:
            if inst in vibe:
                intent["instrument"] = inst
                # Add appropriate chain
                if f"{inst}_chain" in self.musical_understanding:
                    intent["required_engines"].extend(self.musical_understanding[f"{inst}_chain"])
        
        # Detect character descriptors
        for descriptor, mapping in self.semantic_map.items():
            if descriptor in vibe:
                intent["character"].append(descriptor)
                intent["required_engines"].extend(mapping["engines"])
                intent["avoid_engines"].extend(mapping["avoid"])
                intent["target_params"].update(mapping["params"])
        
        # Detect genre
        genres = ["pop", "rock", "metal", "jazz", "electronic", "classical", "folk", "trap"]
        for genre in genres:
            if genre in vibe:
                intent["genre"] = genre
        
        # Extract explicit engine requests from vibe text
        for engine_id, engine_name in ENGINE_NAMES.items():
            if engine_name.lower() in vibe.lower():
                intent["required_engines"].append(engine_id)
        
        # Also extract engines from blueprint slots if present
        for slot in range(1, 7):
            slot_engine = blueprint.get(f"slot{slot}_engine", 0)
            if slot_engine > 0:
                intent["required_engines"].append(slot_engine)
        
        # Remove duplicates
        intent["required_engines"] = list(set(intent["required_engines"]))
        intent["avoid_engines"] = list(set(intent["avoid_engines"]))
        
        logger.info(f"Extracted intent: {intent}")
        return intent
    
    def _score_preset_musically(self, preset: Dict[str, Any], intent: Dict[str, Any]) -> float:
        """
        Score a preset based on musical understanding
        """
        score = 0.0
        
        # Get engines in preset
        preset_engines = []
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                preset_engines.append(engine_id)
        
        # Score based on required engines (most important)
        for required in intent["required_engines"]:
            if required in preset_engines:
                score += 10.0
                # Bonus if in correct position
                expected_pos = self._get_expected_position(required)
                actual_pos = preset_engines.index(required)
                if abs(expected_pos - actual_pos) <= 1:
                    score += 5.0
            else:
                score -= 5.0  # Penalty for missing required engine
        
        # Score based on avoided engines
        for avoid in intent["avoid_engines"]:
            if avoid in preset_engines:
                score -= 8.0
        
        # Score based on engine understanding
        for engine_id in preset_engines:
            if engine_id in ENGINE_KNOWLEDGE:
                engine_info = ENGINE_KNOWLEDGE[engine_id]
                
                # Check if engine matches character
                for char in intent["character"]:
                    if char in engine_info["character"].lower():
                        score += 3.0
                
                # Check use cases
                if intent["instrument"]:
                    for use_case in engine_info["use_cases"]:
                        if intent["instrument"] in use_case.lower():
                            score += 4.0
        
        # Score based on signal chain logic
        chain_score = self._score_signal_chain(preset_engines)
        score += chain_score
        
        # Score based on parameter similarity
        if intent["target_params"]:
            param_score = self._score_parameters(preset, intent["target_params"])
            score += param_score
        
        # Bonus for matching genre/vibe
        preset_name = preset.get("name", "").lower()
        preset_category = preset.get("category", "").lower()
        
        if intent["genre"] and intent["genre"] in preset_name:
            score += 7.0
        
        for char in intent["character"]:
            if char in preset_name or char in preset_category:
                score += 2.0
        
        return score
    
    def _get_expected_position(self, engine_id: int) -> int:
        """
        Get the expected position for an engine type in signal chain
        """
        position_map = {
            # Dynamics early
            1: 0, 2: 0, 3: 0, 4: 0, 5: 5,
            # EQ/Filters next
            6: 1, 7: 1, 8: 1, 9: 1, 10: 1, 11: 1, 12: 1, 13: 1, 14: 1,
            # Distortion mid
            15: 2, 16: 2, 17: 2, 18: 2, 19: 2, 20: 2, 21: 2, 22: 2,
            # Modulation
            23: 3, 24: 3, 25: 3, 26: 3, 27: 3, 28: 3, 29: 3, 30: 3,
            31: 3, 32: 3, 33: 3,
            # Time effects
            34: 4, 35: 4, 36: 4, 37: 4, 38: 4,
            # Reverb last
            39: 5, 40: 5, 41: 5, 42: 5, 43: 5,
            # Spatial/Utility
            44: 4, 45: 4, 46: 4, 47: 3, 48: 3, 49: 3, 50: 3, 51: 3, 52: 3,
            53: 5, 54: 0, 55: 5, 56: 5
        }
        return position_map.get(engine_id, 3)
    
    def _score_signal_chain(self, engines: List[int]) -> float:
        """
        Score how well the signal chain is ordered
        """
        if not engines:
            return 0.0
        
        score = 0.0
        ideal_order = sorted(engines, key=lambda x: self._get_expected_position(x))
        
        # Compare actual order to ideal
        for i, engine in enumerate(engines):
            if i < len(ideal_order) and engine == ideal_order[i]:
                score += 2.0
            else:
                score -= 1.0
        
        # Check for specific good patterns
        good_patterns = [
            [2, 7, 39],  # Comp → EQ → Reverb
            [4, 22, 35],  # Gate → Overdrive → Delay
            [1, 15, 39],  # Opto → Tube → Reverb
        ]
        
        for pattern in good_patterns:
            if all(e in engines for e in pattern):
                score += 5.0
        
        return score
    
    def _score_parameters(self, preset: Dict[str, Any], target_params: Dict[str, float]) -> float:
        """
        Score parameter similarity
        """
        # This is simplified - would need mapping of param names to indices
        score = 0.0
        
        # For now, just check if parameters are in expected ranges
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            
            if engine_id == 15:  # Vintage Tube
                drive = preset.get(f"slot{slot}_param1", 0.5)
                if "drive" in target_params:
                    diff = abs(drive - target_params["drive"])
                    score += (1.0 - diff) * 3.0
            
            elif engine_id == 39:  # Plate Reverb
                size = preset.get(f"slot{slot}_param0", 0.5)
                if "size" in target_params:
                    diff = abs(size - target_params.get("size", 0.5))
                    score += (1.0 - diff) * 2.0
        
        return score
    
    def _explain_preset_choice(self, preset: Dict[str, Any], intent: Dict[str, Any]) -> str:
        """
        Explain why this preset was chosen
        """
        explanations = []
        
        # Get engines
        engines = []
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                engines.append(ENGINE_NAMES.get(engine_id, "Unknown"))
        
        # Explain based on intent matching
        if intent["instrument"]:
            explanations.append(f"Suitable for {intent['instrument']}")
        
        if intent["character"]:
            explanations.append(f"Matches {', '.join(intent['character'])} character")
        
        if intent["genre"]:
            explanations.append(f"Appropriate for {intent['genre']} genre")
        
        # Explain signal chain
        if engines:
            explanations.append(f"Signal chain: {' → '.join(engines[:3])}")
        
        return " | ".join(explanations) if explanations else "Selected based on similarity"
    
    def _faiss_fallback_search(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Fallback to FAISS vector search if musical scoring fails
        """
        logger.info("Using FAISS fallback search")
        
        # Create embedding from blueprint
        embedding = self._create_embedding_from_blueprint(blueprint)
        
        # Search FAISS index
        distances, indices = self.index.search(embedding.reshape(1, -1), k=5)
        
        if len(indices[0]) > 0:
            best_idx = indices[0][0]
            if best_idx < len(self.presets):
                return self.presets[best_idx]
        
        # Ultimate fallback
        return self._create_fallback_preset(blueprint)
    
    def _create_embedding_from_blueprint(self, blueprint: Dict[str, Any]) -> np.ndarray:
        """
        Create a vector embedding from blueprint
        """
        # Simple embedding: one-hot encode engines + parameter values
        embedding = np.zeros(512)
        
        # Encode requested engines
        if "slots" in blueprint:
            for slot_data in blueprint["slots"]:
                engine_id = slot_data.get("engine_id", 0)
                if engine_id > 0 and engine_id < len(embedding):
                    embedding[engine_id] = 1.0
        
        return embedding
    
    def _create_fallback_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Create a safe fallback preset
        """
        return {
            "name": "Oracle Fallback",
            "slot1_engine": 2,  # Compressor
            "slot2_engine": 7,  # EQ
            "slot3_engine": 39,  # Reverb
            "oracle_explanation": "Fallback preset with basic processing"
        }
    
    def _load_index(self) -> faiss.Index:
        """Load FAISS index"""
        try:
            return faiss.read_index(self.index_path)
        except Exception as e:
            logger.error(f"Failed to load FAISS index: {e}")
            # Create dummy index
            return faiss.IndexFlatL2(512)
    
    def _load_metadata(self) -> List[Dict]:
        """Load metadata"""
        try:
            with open(self.meta_path, 'r') as f:
                return json.load(f)
        except Exception as e:
            logger.error(f"Failed to load metadata: {e}")
            return []
    
    def _load_presets(self) -> List[Dict]:
        """Load full presets"""
        try:
            with open(self.presets_path, 'r') as f:
                presets = json.load(f)
                logger.info(f"Loaded {len(presets)} presets")
                return presets
        except Exception as e:
            logger.error(f"Failed to load presets: {e}")
            return []