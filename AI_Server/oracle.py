import json
import logging
from typing import Dict, Any, List
from pathlib import Path

logger = logging.getLogger(__name__)

class Oracle:
    """
    The Oracle searches the Golden Corpus for the best matching preset
    based on the Visionary's blueprint.
    """
    
    def __init__(self, corpus_path: str = "../Golden_Corpus/golden_corpus.json"):
        self.corpus_path = Path(corpus_path)
        self.corpus = self._load_corpus()
    
    def _load_corpus(self) -> List[Dict[str, Any]]:
        """Load the Golden Corpus from JSON file"""
        try:
            if self.corpus_path.exists():
                with open(self.corpus_path, 'r') as f:
                    return json.load(f)
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
                engine_id = slot.get("engine_id", -1)
                if engine_id >= 0:  # Not bypass
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
        for param_name, value in preset.get("parameters", {}).items():
            if "engine" in param_name.lower() and isinstance(value, (int, float)):
                engine_id = int(value) - 1  # Convert from 1-based to 0-based
                if engine_id >= 0:
                    engines.append(engine_id)
        return engines
    
    def _adapt_preset_to_blueprint(self, preset: Dict[str, Any], blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Adapt a corpus preset to match the blueprint's slot configuration"""
        adapted = preset.copy()
        
        # Update engine selections based on blueprint
        for slot_info in blueprint.get("slots", []):
            slot_num = slot_info.get("slot", 1)
            engine_id = slot_info.get("engine_id", -1)
            
            # Update engine selector parameter
            engine_param = f"slot{slot_num}_engine"
            if engine_id >= 0:
                adapted["parameters"][engine_param] = engine_id + 1  # Convert to 1-based for combo box
            else:
                adapted["parameters"][engine_param] = 0  # Bypass
                
            # Update bypass parameter
            bypass_param = f"slot{slot_num}_bypass"
            adapted["parameters"][bypass_param] = 1.0 if engine_id < 0 else 0.0
        
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
                engine_id = slot_info.get("engine_id", -1)
                preset["parameters"][f"slot{slot}_engine"] = engine_id if engine_id >= 0 else 0
                preset["parameters"][f"slot{slot}_bypass"] = 0.0 if engine_id >= 0 else 1.0
            else:
                preset["parameters"][f"slot{slot}_engine"] = 0
                preset["parameters"][f"slot{slot}_bypass"] = 1.0
        
        return preset