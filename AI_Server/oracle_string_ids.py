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
from engine_definitions import ENGINES, get_engine_key

logger = logging.getLogger(__name__)

class OracleStringIDs:
    """
    Oracle that uses string engine identifiers for preset matching
    """
    
    def __init__(self,
                 index_path: str = "../JUCE_Plugin/GoldenCorpus/faiss_index/corpus.index",
                 meta_path: str = "../JUCE_Plugin/GoldenCorpus/faiss_index/corpus_meta.pkl",
                 presets_path: str = "../JUCE_Plugin/GoldenCorpus/all_presets_string_ids.json"):
        """Initialize Oracle with string-based corpus"""
        self.index_path = Path(index_path)
        self.meta_path = Path(meta_path)
        self.presets_path = Path(presets_path)
        
        # Vector dimensions
        self.VECTOR_DIM = 76
        
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
                with open(self.meta_path, 'rb') as f:
                    metadata = pickle.load(f)
                return metadata
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
                # Try numeric backup if string version doesn't exist
                numeric_path = self.presets_path.parent / "all_presets.json"
                if numeric_path.exists():
                    logger.warning("String ID corpus not found, loading numeric and converting")
                    with open(numeric_path, 'r') as f:
                        data = json.load(f)
                        return self._convert_numeric_presets(data.get('presets', []))
                return []
        except Exception as e:
            logger.error(f"Error loading presets: {e}")
            return []
    
    def _convert_numeric_presets(self, presets: List[Dict]) -> List[Dict]:
        """Convert numeric presets to string IDs on the fly"""
        from engine_definitions import get_engine_by_legacy_id
        
        converted = []
        for preset in presets:
            preset_copy = preset.copy()
            if "engines" in preset_copy:
                for engine in preset_copy["engines"]:
                    if "type" in engine and isinstance(engine["type"], int):
                        engine_info = get_engine_by_legacy_id(engine["type"])
                        if engine_info:
                            engine["type"] = engine_info["key"]
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
        
        # 2. Engine presence using string IDs
        for slot in slots:
            engine_key = slot.get("engine", "bypass")
            if engine_key != "bypass" and engine_key in ENGINES:
                # Map string ID to vector position
                engine_info = ENGINES[engine_key]
                legacy_id = engine_info.get("legacy_id", -1)
                if 0 <= legacy_id < 53:
                    vector[11 + legacy_id] = 1.0
        
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
                engine_key = engine_config.get("type")
                
                # Validate engine key
                if engine_key and engine_key in ENGINES:
                    adapted["parameters"][f"slot{plugin_slot}_engine"] = engine_key
                    adapted["parameters"][f"slot{plugin_slot}_bypass"] = 0.0
                    adapted["parameters"][f"slot{plugin_slot}_mix"] = engine_config.get("mix", 1.0)
                    
                    # Apply parameters
                    params_array = engine_config.get("params", [])
                    for param_idx, value in enumerate(params_array):
                        param_num = param_idx + 1
                        if 1 <= param_num <= 10:
                            adapted["parameters"][f"slot{plugin_slot}_param{param_num}"] = value
                else:
                    logger.warning(f"Invalid engine key: {engine_key}")
        
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
        
        # Apply blueprint engines (already using string IDs)
        for slot_info in blueprint.get("slots", []):
            slot_num = slot_info.get("slot", 1)
            engine_key = slot_info.get("engine", "bypass")
            
            if 1 <= slot_num <= 6 and engine_key != "bypass":
                if engine_key in ENGINES:
                    preset["parameters"][f"slot{slot_num}_engine"] = engine_key
                    preset["parameters"][f"slot{slot_num}_bypass"] = 0.0
                    logger.info(f"Default preset: Slot {slot_num} = {engine_key}")
        
        # Master parameters
        preset["parameters"]["master_input"] = 0.7
        preset["parameters"]["master_output"] = 0.7
        preset["parameters"]["master_mix"] = 1.0
        
        return preset


# Test function
async def test_oracle():
    """Test Oracle with string IDs"""
    oracle = OracleStringIDs()
    
    # Test blueprint with string IDs
    test_blueprint = {
        "slots": [
            {"slot": 1, "engine": "vintage_tube", "character": "warm"},
            {"slot": 2, "engine": "tape_echo", "character": "vintage"},
            {"slot": 3, "engine": "plate_reverb", "character": "spacious"},
            {"slot": 4, "engine": "bypass", "character": "unused"},
            {"slot": 5, "engine": "bypass", "character": "unused"},
            {"slot": 6, "engine": "bypass", "character": "unused"}
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
        engine = preset['parameters'].get(f'slot{slot}_engine')
        if engine != 'bypass':
            print(f"  Slot {slot}: {engine}")

if __name__ == "__main__":
    import asyncio
    asyncio.run(test_oracle())