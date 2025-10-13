"""
Fixed Oracle with proper FAISS vector generation
No modulo mapping - direct engine ID to vector position
"""

import json
import logging
import numpy as np
import faiss
from typing import Dict, Any, List, Tuple
from pathlib import Path

logger = logging.getLogger(__name__)

class OracleFAISS:
    """
    Fixed Oracle that uses proper vector mapping for ultra-fast similarity search
    """
    
    def __init__(self, 
                 index_path: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus_clean.index",
                 meta_path: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata_clean.json",
                 presets_path: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json"):
        """Initialize the Oracle with FAISS index and metadata"""
        self.index_path = Path(index_path)
        self.meta_path = Path(meta_path)
        self.presets_path = Path(presets_path)
        
        # FIXED: Proper vector dimensions for all engines
        # 10 vibe features + 56 engine slots = 66 dimensions minimum
        self.VECTOR_DIM = 70  # Extra room for future expansion
        
        # Load resources
        self.presets = self._load_presets()
        self.index = self._rebuild_index()  # Rebuild with correct dimensions
        self.metadata = self._generate_metadata()
        
        logger.info(f"Oracle initialized with {len(self.presets)} presets (fixed vector mapping)")
    
    def _load_presets(self) -> List[Dict[str, Any]]:
        """Load full preset data"""
        try:
            if self.presets_path.exists():
                with open(self.presets_path, 'r') as f:
                    presets = json.load(f)
                logger.info(f"Loaded {len(presets)} full presets")
                return presets
            else:
                logger.warning(f"Presets file not found at {self.presets_path}")
                return []
        except Exception as e:
            logger.error(f"Error loading presets: {str(e)}")
            return []
    
    def _rebuild_index(self) -> faiss.IndexFlatL2:
        """Rebuild FAISS index with proper vector mapping"""
        index = faiss.IndexFlatL2(self.VECTOR_DIM)
        
        # Build vectors for all presets
        vectors = []
        for preset in self.presets:
            vector = self._preset_to_vector(preset)
            vectors.append(vector)
        
        if vectors:
            vectors_array = np.array(vectors, dtype=np.float32)
            index.add(vectors_array)
            logger.info(f"Built FAISS index with {index.ntotal} vectors")
        
        return index
    
    def _generate_metadata(self) -> List[Dict[str, Any]]:
        """Generate metadata for presets"""
        metadata = []
        for preset in self.presets:
            engines = []
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                if engine_id > 0:
                    engines.append(engine_id)
            
            metadata.append({
                "id": preset.get("id"),
                "name": preset.get("creative_name"),
                "category": preset.get("category"),
                "vibe": preset.get("vibe"),
                "engines": engines
            })
        return metadata
    
    def _preset_to_vector(self, preset: Dict[str, Any]) -> np.ndarray:
        """Convert preset to vector with PROPER engine mapping"""
        vector = np.zeros(self.VECTOR_DIM, dtype=np.float32)
        
        # 1. Vibe features (0-9) - lower weight
        vibe = preset.get("vibe", "").lower()
        category = preset.get("category", "").lower()
        
        if "warm" in vibe or "vintage" in vibe:
            vector[0] = 1.0
        if "bright" in vibe or "crisp" in vibe:
            vector[1] = 1.0
        if "aggressive" in vibe or "heavy" in vibe or "metal" in vibe:
            vector[2] = 1.0
        if "spacious" in vibe or "ambient" in vibe:
            vector[3] = 1.0
        if "funky" in vibe or "groove" in vibe:
            vector[4] = 1.0
        if "psychedelic" in vibe or "trippy" in vibe:
            vector[5] = 1.0
        if "clean" in vibe:
            vector[6] = 1.0
        if "distorted" in vibe or "saturated" in vibe:
            vector[7] = 1.0
        if "experimental" in vibe or "creative" in vibe:
            vector[8] = 1.0
        if "vocal" in vibe or "voice" in vibe:
            vector[9] = 1.0
        
        # 2. FIXED: Direct engine mapping (10-66) - NO MODULO!
        # Each engine gets its OWN unique position
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if 0 < engine_id <= 56:
                # Direct mapping: position = 10 + engine_id
                vector[10 + engine_id] = 20.0  # Very high weight for exact matching
        
        return vector
    
    def blueprint_to_vector(self, blueprint: Dict[str, Any]) -> np.ndarray:
        """
        Convert Visionary blueprint to a search vector.
        FIXED: Direct engine ID mapping for accurate matching
        """
        vector = np.zeros(self.VECTOR_DIM, dtype=np.float32)
        
        # Extract vibe features
        vibe = blueprint.get("overall_vibe", "").lower()
        
        # Map vibe keywords (indices 0-9)
        if "warm" in vibe or "vintage" in vibe:
            vector[0] = 1.0
        if "bright" in vibe or "crisp" in vibe:
            vector[1] = 1.0
        if "aggressive" in vibe or "heavy" in vibe or "metal" in vibe:
            vector[2] = 1.0
        if "spacious" in vibe or "ambient" in vibe:
            vector[3] = 1.0
        if "funky" in vibe or "groove" in vibe:
            vector[4] = 1.0
        if "psychedelic" in vibe or "trippy" in vibe:
            vector[5] = 1.0
        if "clean" in vibe:
            vector[6] = 1.0
        if "distorted" in vibe or "saturated" in vibe:
            vector[7] = 1.0
        if "experimental" in vibe or "creative" in vibe:
            vector[8] = 1.0
        if "vocal" in vibe or "voice" in vibe:
            vector[9] = 1.0
        
        # FIXED: Direct engine mapping for requested engines
        requested_engines = set()
        for slot in blueprint.get("slots", []):
            engine_id = slot.get("engine_id", -1)
            if 0 < engine_id <= 56:
                requested_engines.add(engine_id)
                # Direct mapping - each engine has unique position
                vector[10 + engine_id] = 20.0  # Very high weight
        
        # Store for later comparison
        self._last_requested_engines = requested_engines
        
        return vector
    
    def find_best_presets(self, blueprint: Dict[str, Any], k: int = 5) -> List[Dict[str, Any]]:
        """
        Find the k best matching presets using FAISS similarity search.
        FIXED: Proper engine matching with direct ID mapping
        """
        try:
            # Convert blueprint to search vector
            query_vector = self.blueprint_to_vector(blueprint)
            query_vector = query_vector.reshape(1, -1)
            
            # Get requested engines
            requested_engines = getattr(self, '_last_requested_engines', set())
            
            # Search
            distances, indices = self.index.search(query_vector, min(k * 3, self.index.ntotal))
            
            # Score and rank results
            candidates = []
            for dist, idx in zip(distances[0], indices[0]):
                if idx < len(self.presets):
                    preset = self.presets[idx].copy()
                    meta = self.metadata[idx] if idx < len(self.metadata) else {}
                    
                    # Count matching engines
                    preset_engines = set(meta.get('engines', []))
                    engine_matches = len(requested_engines.intersection(preset_engines))
                    
                    # Calculate scores
                    if requested_engines:
                        engine_match_ratio = engine_matches / len(requested_engines)
                    else:
                        engine_match_ratio = 0
                    
                    # Distance-based similarity (inverse)
                    base_similarity = 1.0 / (1.0 + dist)
                    
                    # Heavy weight for engine matching
                    combined_score = base_similarity + (engine_match_ratio * 100.0)
                    
                    preset['similarity_score'] = float(base_similarity)
                    preset['engine_match_score'] = float(engine_match_ratio)
                    preset['combined_score'] = float(combined_score)
                    preset['matching_engines'] = engine_matches
                    preset['requested_engines'] = list(requested_engines)
                    preset['actual_engines'] = list(preset_engines)
                    
                    candidates.append(preset)
            
            # Sort by combined score
            candidates.sort(key=lambda x: x['combined_score'], reverse=True)
            
            # Take top k results
            results = candidates[:k]
            
            # Log results
            for i, preset in enumerate(results):
                logger.info(f"Match {i+1}: {preset.get('creative_name', 'Unknown')} "
                          f"(engines: {preset['matching_engines']}/{len(requested_engines)}, "
                          f"score: {preset['combined_score']:.3f})")
                if requested_engines:
                    logger.info(f"  Requested: {preset['requested_engines']}")
                    logger.info(f"  Found: {preset['actual_engines']}")
            
            return results
            
        except Exception as e:
            logger.error(f"Error in find_best_presets: {str(e)}")
            return []
    
    def find_best_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Find the single best matching preset"""
        results = self.find_best_presets(blueprint, k=1)
        if results:
            best = results[0]
            # Clean up metadata
            for key in ['similarity_score', 'engine_match_score', 'combined_score', 
                       'matching_engines', 'requested_engines', 'actual_engines']:
                best.pop(key, None)
            return self._adapt_preset_to_blueprint(best, blueprint)
        else:
            return self._create_default_preset(blueprint)
    
    def _adapt_preset_to_blueprint(self, preset: Dict[str, Any], blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Adapt preset to plugin format"""
        from engine_mapping_authoritative import UTILITY_ENGINES
        
        adapted = {
            "name": blueprint.get("creative_name", preset.get("creative_name", "Oracle Match")),
            "id": preset.get("id", ""),
            "category": preset.get("category", ""),
            "vibe": blueprint.get("overall_vibe", "custom"),
            "creative_name": blueprint.get("creative_name", preset.get("creative_name", "")),
            "source": "oracle_faiss_fixed",
            "parameters": {}
        }
        
        # Copy all parameters
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            
            # Filter out utility engines
            if engine_id in UTILITY_ENGINES:
                logger.warning(f"Oracle: Filtered utility engine {engine_id} from slot {slot}")
                engine_id = 0
            
            adapted["parameters"][f"slot{slot}_engine"] = engine_id
            adapted["parameters"][f"slot{slot}_bypass"] = preset.get(f"slot{slot}_bypass", 0.0 if engine_id > 0 else 1.0)
            adapted["parameters"][f"slot{slot}_mix"] = preset.get(f"slot{slot}_mix", 0.7 if engine_id > 0 else 0.5)
            adapted["parameters"][f"slot{slot}_solo"] = preset.get(f"slot{slot}_solo", 0.0)
            
            # Copy all 15 parameters
            for param in range(1, 16):
                param_key = f"slot{slot}_param{param}"
                adapted["parameters"][param_key] = preset.get(param_key, 0.5)
        
        return adapted
    
    def _create_default_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Create a default preset when no matches found"""
        preset = {
            "name": blueprint.get("creative_name", "Oracle Default"),
            "vibe": blueprint.get("overall_vibe", "default"),
            "source": "oracle_default",
            "parameters": {}
        }
        
        # Initialize all parameters
        for slot in range(1, 7):
            for param in range(1, 16):
                preset["parameters"][f"slot{slot}_param{param}"] = 0.5
            
            preset["parameters"][f"slot{slot}_engine"] = 0
            preset["parameters"][f"slot{slot}_bypass"] = 1.0
            preset["parameters"][f"slot{slot}_mix"] = 0.5
            preset["parameters"][f"slot{slot}_solo"] = 0.0
        
        # Apply blueprint engine selections
        for slot_info in blueprint.get("slots", []):
            slot_num = slot_info.get("slot", 1)
            engine_id = slot_info.get("engine_id", -1)
            if 1 <= slot_num <= 6 and engine_id > 0:
                preset["parameters"][f"slot{slot_num}_engine"] = engine_id
                preset["parameters"][f"slot{slot_num}_bypass"] = 0.0
        
        return preset