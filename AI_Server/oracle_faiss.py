import json
import logging
import numpy as np
import faiss
import pickle
from typing import Dict, Any, List, Tuple
from pathlib import Path

logger = logging.getLogger(__name__)

class OracleFAISS:
    """
    Enhanced Oracle that uses FAISS for ultra-fast similarity search
    across the Golden Corpus of 250 presets.
    """
    
    def __init__(self, 
                 index_path: str = "../JUCE_Plugin/GoldenCorpus/faiss_index/corpus.index",
                 meta_path: str = "../JUCE_Plugin/GoldenCorpus/faiss_index/corpus_meta.pkl",
                 presets_path: str = "../JUCE_Plugin/GoldenCorpus/all_presets.json"):
        """Initialize the Oracle with FAISS index and metadata"""
        self.index_path = Path(index_path)
        self.meta_path = Path(meta_path)
        self.presets_path = Path(presets_path)
        
        # Vector dimensions (must match oracle_faiss_indexer.py)
        self.VECTOR_DIM = 76
        
        # Load FAISS index
        self.index = self._load_index()
        self.metadata = self._load_metadata()
        self.presets = self._load_presets()
        
        logger.info(f"Oracle initialized with {self.index.ntotal} presets in FAISS index")
    
    def _load_index(self) -> faiss.IndexFlatL2:
        """Load the FAISS index"""
        try:
            if self.index_path.exists():
                index = faiss.read_index(str(self.index_path))
                logger.info(f"Loaded FAISS index from {self.index_path}")
                return index
            else:
                logger.warning(f"FAISS index not found at {self.index_path}, creating empty index")
                return faiss.IndexFlatL2(self.VECTOR_DIM)
        except Exception as e:
            logger.error(f"Error loading FAISS index: {str(e)}")
            return faiss.IndexFlatL2(self.VECTOR_DIM)
    
    def _load_metadata(self) -> List[Dict[str, Any]]:
        """Load preset metadata"""
        try:
            if self.meta_path.exists():
                with open(self.meta_path, 'rb') as f:
                    metadata = pickle.load(f)
                logger.info(f"Loaded metadata for {len(metadata)} presets")
                return metadata
            else:
                logger.warning(f"Metadata not found at {self.meta_path}")
                return []
        except Exception as e:
            logger.error(f"Error loading metadata: {str(e)}")
            return []
    
    def _load_presets(self) -> List[Dict[str, Any]]:
        """Load full preset data"""
        try:
            if self.presets_path.exists():
                with open(self.presets_path, 'r') as f:
                    data = json.load(f)
                    presets = data.get('presets', [])
                logger.info(f"Loaded {len(presets)} full presets")
                return presets
            else:
                logger.warning(f"Presets file not found at {self.presets_path}")
                return []
        except Exception as e:
            logger.error(f"Error loading presets: {str(e)}")
            return []
    
    def blueprint_to_vector(self, blueprint: Dict[str, Any]) -> np.ndarray:
        """
        Convert Visionary blueprint to a search vector.
        This is a simplified version - enhance based on your needs.
        """
        vector = np.zeros(self.VECTOR_DIM, dtype=np.float32)
        
        # Extract features from blueprint
        vibe = blueprint.get("overall_vibe", "").lower()
        slots = blueprint.get("slots", [])
        
        # 1. Sonic Profile (indices 0-5)
        # Map vibe keywords to sonic characteristics
        if "warm" in vibe or "vintage" in vibe:
            vector[0] = 0.3  # brightness (lower)
            vector[5] = 0.8  # vintage (higher)
        if "bright" in vibe or "crisp" in vibe:
            vector[0] = 0.8  # brightness
        if "dense" in vibe or "thick" in vibe:
            vector[1] = 0.8  # density
        if "spacious" in vibe or "ambient" in vibe:
            vector[3] = 0.8  # space
        if "aggressive" in vibe or "heavy" in vibe:
            vector[4] = 0.8  # aggression
        
        # 2. Engine presence (indices 11-63, one-hot for each engine type)
        for slot in slots:
            engine_id = slot.get("engine_id", -1)
            if 0 <= engine_id < 53:  # Valid engine
                vector[11 + engine_id] = 1.0
        
        # 3. Normalize certain ranges
        vector[0:6] = np.clip(vector[0:6], 0.0, 1.0)  # Sonic profile
        
        return vector
    
    def find_best_presets(self, blueprint: Dict[str, Any], k: int = 5) -> List[Dict[str, Any]]:
        """
        Find the k best matching presets using FAISS similarity search.
        
        Args:
            blueprint: Visionary blueprint
            k: Number of presets to return
            
        Returns:
            List of best matching presets with similarity scores
        """
        try:
            # Convert blueprint to search vector
            query_vector = self.blueprint_to_vector(blueprint)
            query_vector = query_vector.reshape(1, -1)  # FAISS expects 2D array
            
            # Search in FAISS index
            distances, indices = self.index.search(query_vector, k)
            
            # Collect results
            results = []
            for i, (dist, idx) in enumerate(zip(distances[0], indices[0])):
                if idx < len(self.presets):
                    preset = self.presets[idx].copy()
                    preset['similarity_score'] = float(1.0 / (1.0 + dist))  # Convert distance to similarity
                    preset['match_rank'] = i + 1
                    results.append(preset)
                    
                    logger.info(f"Match {i+1}: {preset.get('name', 'Unknown')} "
                              f"(distance: {dist:.3f}, similarity: {preset['similarity_score']:.3f})")
            
            return results
            
        except Exception as e:
            logger.error(f"Error in find_best_presets: {str(e)}")
            return []
    
    def find_best_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Find the single best matching preset (for compatibility with old interface).
        """
        results = self.find_best_presets(blueprint, k=1)
        if results:
            best = results[0]
            # Adapt to blueprint's slot configuration
            return self._adapt_preset_to_blueprint(best, blueprint)
        else:
            # Fallback to default preset
            return self._create_default_preset(blueprint)
    
    def blend_presets(self, presets: List[Dict[str, Any]], weights: List[float] = None) -> Dict[str, Any]:
        """
        Blend multiple presets together based on weights.
        This enables morphing between different sonic characteristics.
        """
        if not presets:
            return {}
        
        if weights is None:
            weights = [1.0 / len(presets)] * len(presets)
        
        # Normalize weights
        total_weight = sum(weights)
        weights = [w / total_weight for w in weights]
        
        # Convert all presets to plugin format first
        adapted_presets = []
        for preset in presets:
            # Create a dummy blueprint for adaptation
            dummy_blueprint = {"overall_vibe": "blended"}
            adapted = self._adapt_preset_to_blueprint(preset, dummy_blueprint)
            adapted_presets.append(adapted)
        
        # Start with first adapted preset as base
        blended = adapted_presets[0].copy()
        
        # Blend numeric parameters
        for param_key in blended.get("parameters", {}):
            if isinstance(blended["parameters"][param_key], (int, float)):
                weighted_sum = 0.0
                for preset, weight in zip(adapted_presets, weights):
                    if param_key in preset.get("parameters", {}):
                        weighted_sum += preset["parameters"][param_key] * weight
                blended["parameters"][param_key] = weighted_sum
        
        # Blend metadata
        blended["name"] = "Oracle Blend"
        blended["source"] = "oracle_blend"
        
        return blended
    
    def _adapt_preset_to_blueprint(self, preset: Dict[str, Any], blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Adapt a corpus preset to match the blueprint's slot configuration"""
        # Convert from exported format to plugin format
        adapted = {
            "name": preset.get("name", "Oracle Match"),
            "id": preset.get("id", ""),
            "category": preset.get("category", ""),
            "vibe": blueprint.get("overall_vibe", "custom"),
            "source": "oracle_faiss",
            "parameters": {}
        }
        
        # Extract engine configuration from preset
        engines = preset.get("engines", [])
        
        # Initialize all slots to bypass
        for slot in range(1, 7):
            adapted["parameters"][f"slot{slot}_engine"] = 0  # Bypass
            adapted["parameters"][f"slot{slot}_bypass"] = 1.0
            adapted["parameters"][f"slot{slot}_mix"] = 0.5
            
            # Initialize all parameters to default
            for param in range(1, 11):
                adapted["parameters"][f"slot{slot}_param{param}"] = 0.5
        
        # Apply engine configuration from the preset
        for engine_config in engines:
            slot = engine_config.get("slot", -1)
            # The exported format uses 0-based slots, but plugin uses 1-based
            plugin_slot = slot + 1
            
            if 1 <= plugin_slot <= 6:
                engine_type = engine_config.get("type", -1)
                if engine_type >= 0:
                    adapted["parameters"][f"slot{plugin_slot}_engine"] = engine_type
                    adapted["parameters"][f"slot{plugin_slot}_bypass"] = 0.0  # Active
                    adapted["parameters"][f"slot{plugin_slot}_mix"] = engine_config.get("mix", 1.0)
                    
                    # Apply parameters from the params array
                    params_array = engine_config.get("params", [])
                    for param_idx, value in enumerate(params_array):
                        param_num = param_idx + 1  # Convert 0-based to 1-based
                        if 1 <= param_num <= 10:
                            adapted["parameters"][f"slot{plugin_slot}_param{param_num}"] = value
        
        # Add master parameters
        adapted["parameters"]["master_input"] = 0.7
        adapted["parameters"]["master_output"] = 0.7
        adapted["parameters"]["master_mix"] = 1.0
        
        return adapted
    
    def _create_default_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Create a default preset when no matches found"""
        preset = {
            "name": "Oracle Default",
            "vibe": blueprint.get("overall_vibe", "default"),
            "source": "oracle_default",
            "parameters": {}
        }
        
        # Initialize all parameters to default values
        for slot in range(1, 7):  # 6 slots
            # Engine parameters
            for param in range(1, 11):  # 10 params per engine
                preset["parameters"][f"slot{slot}_param{param}"] = 0.5
            
            # Engine selector and bypass
            preset["parameters"][f"slot{slot}_engine"] = 0  # Bypass
            preset["parameters"][f"slot{slot}_bypass"] = 1.0
            preset["parameters"][f"slot{slot}_mix"] = 0.5
        
        # Apply blueprint engine selections
        for slot_info in blueprint.get("slots", []):
            slot_num = slot_info.get("slot", 1)
            engine_id = slot_info.get("engine_id", -1)
            if 1 <= slot_num <= 6:
                if engine_id >= 0:
                    preset["parameters"][f"slot{slot_num}_engine"] = engine_id
                    preset["parameters"][f"slot{slot_num}_bypass"] = 0.0
        
        return preset