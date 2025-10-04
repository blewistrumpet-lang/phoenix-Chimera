import json
import logging
from typing import Dict, Any, List
from pathlib import Path
from engine_mapping_authoritative import *

logger = logging.getLogger(__name__)

class Oracle:
    """
    The Oracle searches the Golden Corpus for the best matching preset
    based on the Visionary's blueprint.
    """
    
    def __init__(self, corpus_path: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_musical_fixed.json"):
        # Use the musically-fixed corpus with real parameter variance
        self.corpus_path = Path(corpus_path)
        
        # If fixed corpus doesn't exist, try original
        if not self.corpus_path.exists():
            logger.warning(f"Fixed corpus not found at {corpus_path}, trying presets_clean.json")
            self.corpus_path = Path("../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json")
        
        self.corpus = self._load_corpus()
        logger.info(f"Oracle initialized with {len(self.corpus)} presets from {self.corpus_path}")
    
    def _load_corpus(self) -> List[Dict[str, Any]]:
        """Load the Golden Corpus from JSON file"""
        try:
            if self.corpus_path.exists():
                with open(self.corpus_path, 'r') as f:
                    data = json.load(f)
                    # Handle both list format and dict with 'presets' key
                    if isinstance(data, list):
                        return data
                    elif isinstance(data, dict) and 'presets' in data:
                        return data['presets']
                    else:
                        logger.warning(f"Unexpected corpus format in {self.corpus_path}")
                        return [data] if isinstance(data, dict) else []
            else:
                logger.warning(f"Corpus file not found at {self.corpus_path}")
                return []
        except Exception as e:
            logger.error(f"Error loading corpus: {str(e)}")
            return []
    
    def find_best_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Find the best matching preset from the corpus based on the blueprint
        
        For MVP: Simple matching based on requested engine_ids
        """
        try:
            # Extract requested engine IDs from blueprint
            requested_engines = []
            for slot in blueprint.get("slots", []):
                engine_id = slot.get("engine_id", ENGINE_NONE)
                if engine_id != ENGINE_NONE:  # Not bypass
                    requested_engines.append(engine_id)
            
            # If no corpus loaded, return default preset
            if not self.corpus:
                return self._create_default_preset(blueprint)
            
            # Find presets that use at least one of the requested engines
            matching_presets = []
            for preset in self.corpus:
                preset_engines = self._get_preset_engines(preset)
                if any(engine in requested_engines for engine in preset_engines):
                    # Calculate match score
                    score = sum(1 for e in preset_engines if e in requested_engines)
                    matching_presets.append((score, preset))
            
            # Sort by score and return best match
            if matching_presets:
                matching_presets.sort(key=lambda x: x[0], reverse=True)
                best_preset = matching_presets[0][1].copy()
                
                # Update preset to match blueprint's slot configuration
                return self._adapt_preset_to_blueprint(best_preset, blueprint)
            
            # No matches found, create default
            return self._create_default_preset(blueprint)
            
        except Exception as e:
            logger.error(f"Error in find_best_preset: {str(e)}")
            return self._create_default_preset(blueprint)
    
    def _get_preset_engines(self, preset: Dict[str, Any]) -> List[int]:
        """Extract engine IDs used in a preset"""
        engines = []
        
        # Handle new format (direct slot parameters)
        if any(key.startswith('slot') and key.endswith('_engine') for key in preset.keys()):
            for slot in range(1, 7):
                engine_key = f"slot{slot}_engine"
                if engine_key in preset:
                    engine_id = int(preset[engine_key])
                    if engine_id != ENGINE_NONE and validate_engine_id(engine_id):
                        engines.append(engine_id)
        
        # Handle old format (engines array)
        elif "engines" in preset:
            for engine_config in preset["engines"]:
                engine_id = engine_config.get("type")
                if engine_id and engine_id != ENGINE_NONE and validate_engine_id(engine_id):
                    engines.append(int(engine_id))
        
        # Legacy format with parameters dict
        else:
            for param_name, value in preset.get("parameters", {}).items():
                if "engine" in param_name.lower() and isinstance(value, (int, float)):
                    engine_id = int(value)
                    if engine_id != ENGINE_NONE and validate_engine_id(engine_id):
                        engines.append(engine_id)
        
        return engines
    
    def _adapt_preset_to_blueprint(self, preset: Dict[str, Any], blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Adapt a corpus preset to match the blueprint's slot configuration"""
        adapted = preset.copy()
        
        # Update engine selections based on blueprint
        for slot_info in blueprint.get("slots", []):
            slot_num = slot_info.get("slot", 1)
            engine_id = slot_info.get("engine_id", ENGINE_NONE)
            
            # Update engine selector parameter
            engine_param = f"slot{slot_num}_engine"
            adapted["parameters"][engine_param] = engine_id
                
            # Update bypass parameter
            bypass_param = f"slot{slot_num}_bypass"
            adapted["parameters"][bypass_param] = 1.0 if engine_id == ENGINE_NONE else 0.0
        
        # Update metadata
        adapted["vibe"] = blueprint.get("overall_vibe", "custom preset")
        adapted["source"] = "oracle_adapted"
        
        return adapted
    
    def _create_default_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Create a default preset based on the blueprint"""
        preset = {
            "name": "Oracle Default",
            "vibe": blueprint.get("overall_vibe", "default"),
            "source": "oracle_default",
            "parameters": {}
        }
        
        # Initialize all parameters to default values
        for slot in [1, 2]:
            # Engine parameters
            for param in range(1, 11):
                preset["parameters"][f"slot{slot}_param{param}"] = 0.5
            
            # Engine selector and bypass from blueprint
            slot_info = next((s for s in blueprint.get("slots", []) if s.get("slot") == slot), None)
            if slot_info:
                engine_id = slot_info.get("engine_id", ENGINE_NONE)
                preset["parameters"][f"slot{slot}_engine"] = engine_id
                preset["parameters"][f"slot{slot}_bypass"] = 0.0 if engine_id != ENGINE_NONE else 1.0
            else:
                preset["parameters"][f"slot{slot}_engine"] = ENGINE_NONE
                preset["parameters"][f"slot{slot}_bypass"] = 1.0
        
        return preset