"""
Oracle component updated to use string engine identifiers
"""

import json
import logging
import numpy as np
import faiss
import pickle
from typing import Dict, Any, List
from pathlib import Path
from engine_mapping_authoritative import *

logger = logging.getLogger(__name__)

class OracleStringIDs:
    """
    Oracle that uses string engine identifiers for preset matching
    """
    
    def __init__(self,
                 index_path: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus.index",
                 meta_path: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata.json",
                 presets_path: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets.json"):
        """Initialize Oracle with string-based corpus"""
        self.index_path = Path(index_path)
        self.meta_path = Path(meta_path)
        self.presets_path = Path(presets_path)
        
        # Vector dimensions (updated for v3 corpus)
        self.VECTOR_DIM = 53
        
        # Load resources
        self.index = self._load_index()
        self.metadata = self._load_metadata()
        self.presets = self._load_presets()
        
        logger.info(f"Oracle initialized with {len(self.presets)} string-ID presets")
    
    def _load_index(self) -> faiss.IndexFlatL2:
        """Load FAISS index"""
        try:
            if self.index_path.exists():
                index = faiss.read_index(str(self.index_path))
                logger.info(f"Loaded FAISS index with {index.ntotal} vectors")
                return index
            else:
                logger.warning("No FAISS index found, creating empty index")
                return faiss.IndexFlatL2(self.VECTOR_DIM)
        except Exception as e:
            logger.error(f"Error loading FAISS index: {e}")
            return faiss.IndexFlatL2(self.VECTOR_DIM)
    
    def _load_metadata(self) -> List[Dict[str, Any]]:
        """Load preset metadata"""
        try:
            if self.meta_path.exists():
                if self.meta_path.suffix == '.pkl':
                    with open(self.meta_path, 'rb') as f:
                        metadata = pickle.load(f)
                else:  # JSON format
                    with open(self.meta_path, 'r') as f:
                        metadata = json.load(f)
                return metadata if isinstance(metadata, list) else [metadata]
            return []
        except Exception as e:
            logger.error(f"Error loading metadata: {e}")
            return []
    
    def _load_presets(self) -> List[Dict[str, Any]]:
        """Load string-ID presets"""
        try:
            if self.presets_path.exists():
                with open(self.presets_path, 'r') as f:
                    data = json.load(f)
                    presets = data.get('presets', [])
                logger.info(f"Loaded {len(presets)} presets with string IDs")
                return presets
            else:
                # Try using the main v3 presets file (it's already in the correct format)
                logger.info(f"Using presets directly from v3 corpus")
                return presets
        except Exception as e:
            logger.error(f"Error loading presets: {e}")
            return []
    
    def _convert_numeric_presets(self, presets: List[Dict]) -> List[Dict]:
        """Convert legacy numeric presets to authoritative engine IDs"""
        converted = []
        for preset in presets:
            preset_copy = preset.copy()
            if "engines" in preset_copy:
                for engine in preset_copy["engines"]:
                    if "type" in engine and isinstance(engine["type"], int):
                        # Keep numeric ID as-is since we're using authoritative numeric mapping
                        legacy_id = engine["type"]
                        if validate_engine_id(legacy_id):
                            engine["type"] = legacy_id  # Already numeric, just validate
                        else:
                            engine["type"] = ENGINE_NONE  # Invalid, set to bypass
            converted.append(preset_copy)
        return converted
    
    def blueprint_to_vector(self, blueprint: Dict[str, Any]) -> np.ndarray:
        """Convert string-based blueprint to search vector"""
        vector = np.zeros(self.VECTOR_DIM, dtype=np.float32)
        
        vibe = blueprint.get("overall_vibe", "").lower()
        slots = blueprint.get("slots", [])
        
        # 1. Sonic Profile based on vibe
        if "warm" in vibe or "vintage" in vibe:
            vector[0] = 0.3  # brightness (lower)
            vector[5] = 0.8  # vintage (higher)
        if "bright" in vibe or "crisp" in vibe:
            vector[0] = 0.8
        if "aggressive" in vibe or "metal" in vibe:
            vector[4] = 0.8  # aggression
        if "spacious" in vibe or "ambient" in vibe:
            vector[3] = 0.8  # space
        
        # 2. Engine presence using authoritative IDs
        for slot in slots:
            engine_id = slot.get("engine_id", ENGINE_NONE)
            if engine_id != ENGINE_NONE and validate_engine_id(engine_id):
                # Map engine ID to vector position
                if 1 <= engine_id <= ENGINE_COUNT - 1:
                    vector[11 + engine_id - 1] = 1.0  # Offset by 1 since ENGINE_NONE=0
        
        # Normalize
        vector[0:6] = np.clip(vector[0:6], 0.0, 1.0)
        
        return vector
    
    def find_best_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Find best matching preset using string IDs"""
        results = self.find_best_presets(blueprint, k=1)
        if results:
            return self._adapt_preset_to_plugin_format(results[0])
        return self._create_default_preset(blueprint)
    
    def find_best_presets(self, blueprint: Dict[str, Any], k: int = 5) -> List[Dict[str, Any]]:
        """Find k best matching presets"""
        try:
            # Convert blueprint to search vector
            query_vector = self.blueprint_to_vector(blueprint)
            query_vector = query_vector.reshape(1, -1)
            
            # Search in FAISS
            if self.index.ntotal > 0:
                k_actual = min(k, self.index.ntotal)
                distances, indices = self.index.search(query_vector, k_actual)
                
                results = []
                for i, (dist, idx) in enumerate(zip(distances[0], indices[0])):
                    if idx < len(self.presets):
                        preset = self.presets[idx].copy()
                        preset['similarity_score'] = float(1.0 / (1.0 + dist))
                        preset['match_rank'] = i + 1
                        results.append(preset)
                        logger.info(f"Match {i+1}: {preset.get('name')} (score: {preset['similarity_score']:.3f})")
                
                return results
            else:
                logger.warning("FAISS index is empty, using first preset")
                if self.presets:
                    return [self.presets[0]]
                return []
            
        except Exception as e:
            logger.error(f"Error in find_best_presets: {e}")
            return []
    
    def _adapt_preset_to_plugin_format(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Adapt corpus preset to plugin format with string IDs"""
        adapted = {
            "name": preset.get("name", "Oracle Match"),
            "id": preset.get("id", ""),
            "category": preset.get("category", ""),
            "vibe": preset.get("overall_vibe", "custom"),
            "source": "oracle",
            "parameters": {}
        }
        
        # Initialize all slots
        for slot in range(1, 7):
            adapted["parameters"][f"slot{slot}_engine"] = "bypass"
            adapted["parameters"][f"slot{slot}_bypass"] = 1.0
            adapted["parameters"][f"slot{slot}_mix"] = 0.5
            for param in range(1, 11):
                adapted["parameters"][f"slot{slot}_param{param}"] = 0.5
        
        # Apply engines from preset
        engines = preset.get("engines", [])
        for engine_config in engines:
            slot = engine_config.get("slot", -1)
            plugin_slot = slot + 1  # Convert 0-based to 1-based
            
            if 1 <= plugin_slot <= 6:
                engine_id = engine_config.get("type")
                
                # Validate engine ID
                if engine_id and validate_engine_id(engine_id):
                    adapted["parameters"][f"slot{plugin_slot}_engine"] = engine_id
                    adapted["parameters"][f"slot{plugin_slot}_bypass"] = 0.0
                    adapted["parameters"][f"slot{plugin_slot}_mix"] = engine_config.get("mix", 1.0)
                    
                    # Apply parameters
                    params_array = engine_config.get("params", [])
                    for param_idx, value in enumerate(params_array):
                        param_num = param_idx + 1
                        if 1 <= param_num <= 10:
                            adapted["parameters"][f"slot{plugin_slot}_param{param_num}"] = value
                else:
                    logger.warning(f"Invalid engine ID: {engine_id}")
        
        # Master parameters
        adapted["parameters"]["master_input"] = 0.7
        adapted["parameters"]["master_output"] = 0.7
        adapted["parameters"]["master_mix"] = 1.0
        
        return adapted
    
    def _create_default_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Create default preset using string IDs from blueprint"""
        preset = {
            "name": "Oracle Default",
            "vibe": blueprint.get("overall_vibe", "default"),
            "source": "oracle_default",
            "parameters": {}
        }
        
        # Initialize all parameters
        for slot in range(1, 7):
            for param in range(1, 11):
                preset["parameters"][f"slot{slot}_param{param}"] = 0.5
            preset["parameters"][f"slot{slot}_engine"] = "bypass"
            preset["parameters"][f"slot{slot}_bypass"] = 1.0
            preset["parameters"][f"slot{slot}_mix"] = 0.5
        
        # Apply blueprint engines using authoritative IDs
        for slot_info in blueprint.get("slots", []):
            slot_num = slot_info.get("slot", 1)
            engine_id = slot_info.get("engine_id", ENGINE_NONE)
            
            if 1 <= slot_num <= 6 and engine_id != ENGINE_NONE:
                if validate_engine_id(engine_id):
                    preset["parameters"][f"slot{slot_num}_engine"] = engine_id
                    preset["parameters"][f"slot{slot_num}_bypass"] = 0.0
                    logger.info(f"Default preset: Slot {slot_num} = {get_engine_name(engine_id)}")
        
        # Master parameters
        preset["parameters"]["master_input"] = 0.7
        preset["parameters"]["master_output"] = 0.7
        preset["parameters"]["master_mix"] = 1.0
        
        return preset


# Test function
async def test_oracle():
    """Test Oracle with string IDs"""
    oracle = OracleStringIDs()
    
    # Test blueprint with engine IDs
    test_blueprint = {
        "slots": [
            {"slot": 1, "engine_id": ENGINE_VINTAGE_TUBE, "character": "warm"},
            {"slot": 2, "engine_id": ENGINE_TAPE_ECHO, "character": "vintage"},
            {"slot": 3, "engine_id": ENGINE_PLATE_REVERB, "character": "spacious"},
            {"slot": 4, "engine_id": ENGINE_NONE, "character": "unused"},
            {"slot": 5, "engine_id": ENGINE_NONE, "character": "unused"},
            {"slot": 6, "engine_id": ENGINE_NONE, "character": "unused"}
        ],
        "overall_vibe": "warm vintage with space"
    }
    
    print("\n" + "="*80)
    print("TESTING ORACLE WITH STRING IDS")
    print("="*80)
    
    print("\nTest Blueprint:")
    print(json.dumps(test_blueprint, indent=2))
    
    print("\nFinding best match...")
    preset = oracle.find_best_preset(test_blueprint)
    
    print("\nMatched Preset:")
    print(f"Name: {preset.get('name')}")
    print(f"Engines:")
    for slot in range(1, 7):
        engine_id = preset['parameters'].get(f'slot{slot}_engine')
        if engine_id != ENGINE_NONE:
            print(f"  Slot {slot}: {get_engine_name(engine_id)}")

if __name__ == "__main__":
    import asyncio
    asyncio.run(test_oracle())