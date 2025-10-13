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
                 index_path: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus.index",
                 meta_path: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata.json",
                 presets_path: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets.json"):
        """Initialize the Oracle with FAISS index and metadata"""
        self.index_path = Path(index_path)
        self.meta_path = Path(meta_path)
        self.presets_path = Path(presets_path)
        
        # Vector dimensions (must match corpus_generator.py)
        self.VECTOR_DIM = 53
        
        # Load FAISS index
        self.index = self._load_index()
        self.metadata = self._load_metadata()
        self.presets = self._load_presets()
        
        # Pre-filter corpus to exclude presets with utility engines
        self._filter_utility_engines()
        
        logger.info(f"Oracle initialized with {len(self.presets)} presets after filtering utility engines")
    
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
                with open(self.meta_path, 'r') as f:
                    metadata = json.load(f)
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
                    presets = json.load(f)
                    # Handle both formats: list or dict with 'presets' key
                    if isinstance(presets, list):
                        logger.info(f"Loaded {len(presets)} full presets")
                        return presets
                    else:
                        presets_list = presets.get('presets', [])
                        logger.info(f"Loaded {len(presets_list)} full presets")
                        return presets_list
            else:
                logger.warning(f"Presets file not found at {self.presets_path}")
                return []
        except Exception as e:
            logger.error(f"Error loading presets: {str(e)}")
            return []
    
    def blueprint_to_vector(self, blueprint: Dict[str, Any]) -> np.ndarray:
        """
        Convert Visionary blueprint to a search vector.
        Prioritizes finding presets with matching engines.
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
        
        # 2. Engine presence - HEAVILY WEIGHTED for matching
        # Increased weight of engine matching to heavily prioritize presets with requested engines
        engine_weight = 10.0  # 10x weight for engine matches (increased from 3.0)
        
        requested_engines = set()
        for slot in slots:
            engine_id = slot.get("engine_id", -1)
            if 0 < engine_id <= 56:  # Valid non-None engine
                requested_engines.add(engine_id)
                # Map engine IDs to feature space with heavy weighting
                feature_idx = 11 + (engine_id % 42)  # Fit within vector bounds
                if feature_idx < self.VECTOR_DIM:
                    vector[feature_idx] = engine_weight
        
        # Store requested engines for later comparison
        self._last_requested_engines = requested_engines
        
        # 3. Normalize certain ranges
        vector[0:6] = np.clip(vector[0:6], 0.0, 1.0)  # Sonic profile
        
        return vector
    
    def find_best_presets(self, blueprint: Dict[str, Any], k: int = 5) -> List[Dict[str, Any]]:
        """
        Find the k best matching presets using FAISS similarity search.
        Prioritizes presets that contain the requested engines.
        
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
            
            # Get requested engines from blueprint
            requested_engines = set()
            for slot in blueprint.get("slots", []):
                engine_id = slot.get("engine_id", -1)
                if 0 < engine_id <= 56:  # Valid non-None engine
                    requested_engines.add(engine_id)
            
            # Search more candidates to filter by engine match
            search_k = min(k * 10, self.index.ntotal)  # Search 10x more candidates
            distances, indices = self.index.search(query_vector, search_k)
            
            # Score and rank results based on distance AND engine matching
            candidates = []
            for dist, idx in zip(distances[0], indices[0]):
                # Check if this FAISS index corresponds to a valid (non-utility) preset
                if hasattr(self, '_original_to_filtered') and idx in self._original_to_filtered:
                    # Map from FAISS index to filtered preset index
                    preset_idx = self._original_to_filtered[idx]
                    if preset_idx < len(self.presets):
                        preset = self.presets[preset_idx].copy()
                        preset_engines = self._get_preset_engines(preset)
                elif idx < len(self.presets):
                    # Fallback for older initialization without filtering
                    preset = self.presets[idx].copy()
                    preset_engines = self._get_preset_engines(preset)
                    from engine_mapping_authoritative import UTILITY_ENGINES
                    if preset_engines.intersection(set(UTILITY_ENGINES)):
                        continue  # Skip presets with utility engines
                else:
                    continue  # Invalid index
                
                # Calculate engine match score
                engine_matches = len(requested_engines.intersection(preset_engines))
                engine_match_ratio = engine_matches / max(len(requested_engines), 1)
                
                # Combined score: heavily weight engine matching
                base_similarity = 1.0 / (1.0 + dist)
                engine_boost = engine_match_ratio * 10.0  # 10x weight for engine matches (increased from 2.0)
                combined_score = base_similarity + engine_boost
                
                preset['similarity_score'] = float(base_similarity)
                preset['engine_match_score'] = float(engine_match_ratio)
                preset['combined_score'] = float(combined_score)
                preset['matching_engines'] = engine_matches
                candidates.append(preset)
            
            # Sort by combined score (higher is better)
            candidates.sort(key=lambda x: x['combined_score'], reverse=True)
            
            # Take top k results
            results = candidates[:k]
            
            # Log which engines were requested vs found
            if requested_engines:
                logger.info(f"Oracle: Requested engines: {sorted(requested_engines)}")
            else:
                logger.info("Oracle: No specific engines requested")
            
            # Log results
            for i, preset in enumerate(results):
                preset_engines = self._get_preset_engines(preset)
                found_engines = requested_engines.intersection(preset_engines) if requested_engines else set()
                logger.info(f"Match {i+1}: {preset.get('creative_name', preset.get('name', 'Unknown'))} "
                          f"(similarity: {preset['similarity_score']:.3f}, "
                          f"engine matches: {preset['matching_engines']}/{len(requested_engines)}, "
                          f"combined: {preset['combined_score']:.3f})")
                if found_engines:
                    logger.info(f"  Found engines: {sorted(found_engines)}")
                logger.info(f"  All preset engines: {sorted(preset_engines)}")
            
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
        # No conversion needed with 1:1 mapping
        
        # Convert from exported format to plugin format
        adapted = {
            "name": blueprint.get("creative_name", preset.get("creative_name", preset.get("name", "Oracle Match"))),  # Prefer blueprint name
            "id": preset.get("id", ""),
            "category": preset.get("category", ""),
            "vibe": blueprint.get("overall_vibe", "custom"),
            "creative_name": blueprint.get("creative_name", preset.get("creative_name", "")),
            "source": "oracle_faiss",
            "parameters": {}
        }
        
        # Check if preset is in new format (direct slot parameters) or old format (engines array)
        if "slot1_engine" in preset:
            # New format - direct copy
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                
                # CRITICAL: Filter out utility engines from Oracle presets
                from engine_mapping_authoritative import UTILITY_ENGINES
                if engine_id in UTILITY_ENGINES:
                    logger.warning(f"Oracle: Filtered utility engine {engine_id} from slot {slot}")
                    engine_id = 0  # Replace with bypass
                
                # Copy engine and control parameters
                adapted["parameters"][f"slot{slot}_engine"] = engine_id
                adapted["parameters"][f"slot{slot}_bypass"] = preset.get(f"slot{slot}_bypass", 0.0)
                adapted["parameters"][f"slot{slot}_mix"] = preset.get(f"slot{slot}_mix", 0.5)
                adapted["parameters"][f"slot{slot}_solo"] = preset.get(f"slot{slot}_solo", 0.0)
                
                # Copy all 15 parameters
                for param in range(1, 16):
                    param_key = f"slot{slot}_param{param}"
                    adapted["parameters"][param_key] = preset.get(param_key, 0.0)
                    
                if engine_id > 0:
                    logger.info(f"Oracle: Slot {slot} - Engine ID {engine_id}")
        else:
            # Old format with engines array
            engines = preset.get("engines", [])
            
            # Initialize all slots to bypass
            for slot in range(1, 7):
                adapted["parameters"][f"slot{slot}_engine"] = 0  # None
                adapted["parameters"][f"slot{slot}_bypass"] = 1.0
                adapted["parameters"][f"slot{slot}_mix"] = 0.5
                adapted["parameters"][f"slot{slot}_solo"] = 0.0
                
                # Initialize all parameters to default (15 params per slot)
                for param in range(1, 16):
                    adapted["parameters"][f"slot{slot}_param{param}"] = 0.5
            
            # Apply engine configuration from the preset
            for engine_config in engines:
                slot = engine_config.get("slot", -1)
                # The exported format uses 0-based slots, but plugin uses 1-based
                plugin_slot = slot + 1
                
                if 1 <= plugin_slot <= 6:
                    engine_type = engine_config.get("type", -1)
                    if engine_type >= 0:
                        # CRITICAL: Filter out utility engines from old format too
                        from engine_mapping_authoritative import UTILITY_ENGINES
                        if engine_type in UTILITY_ENGINES:
                            logger.warning(f"Oracle: Filtered utility engine {engine_type} from slot {plugin_slot} (old format)")
                            continue  # Skip this engine
                        
                        # With 1:1 mapping, engine ID = choice index, no conversion needed
                        adapted["parameters"][f"slot{plugin_slot}_engine"] = engine_type
                        adapted["parameters"][f"slot{plugin_slot}_bypass"] = 0.0  # Active
                        adapted["parameters"][f"slot{plugin_slot}_mix"] = engine_config.get("mix", 1.0)
                        adapted["parameters"][f"slot{plugin_slot}_solo"] = 0.0  # Not soloed
                        
                        logger.info(f"Oracle: Slot {plugin_slot} - Engine ID {engine_type}")
                        
                        # Apply parameters from the params array
                        params_array = engine_config.get("params", [])
                        for param_idx, value in enumerate(params_array):
                            param_num = param_idx + 1  # Convert 0-based to 1-based
                            if 1 <= param_num <= 15:  # Plugin has 15 params per slot
                                adapted["parameters"][f"slot{plugin_slot}_param{param_num}"] = value
        
        # Note: Plugin doesn't have master parameters, removed from Trinity
        
        return adapted
    
    def _create_default_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Create a default preset when no matches found"""
        
        preset = {
            "name": blueprint.get("creative_name", "Oracle Default"),  # Use creative name from blueprint
            "vibe": blueprint.get("overall_vibe", "default"),
            "source": "oracle_default",
            "parameters": {}
        }
        
        # Initialize all parameters to default values
        for slot in range(1, 7):  # 6 slots
            # Engine parameters
            for param in range(1, 16):  # 15 params per engine
                preset["parameters"][f"slot{slot}_param{param}"] = 0.5
            
            # Engine selector and bypass
            preset["parameters"][f"slot{slot}_engine"] = 0  # None
            preset["parameters"][f"slot{slot}_bypass"] = 1.0
            preset["parameters"][f"slot{slot}_mix"] = 0.5
            preset["parameters"][f"slot{slot}_solo"] = 0.0
        
        # Apply blueprint engine selections
        for slot_info in blueprint.get("slots", []):
            slot_num = slot_info.get("slot", 1)
            engine_id = slot_info.get("engine_id", -1)
            if 1 <= slot_num <= 6:
                if engine_id >= 0:
                    # With 1:1 mapping, engine ID = choice index, no conversion needed
                    preset["parameters"][f"slot{slot_num}_engine"] = engine_id
                    preset["parameters"][f"slot{slot_num}_bypass"] = 0.0
                    preset["parameters"][f"slot{slot_num}_solo"] = 0.0
                    logger.info(f"Oracle Default: Slot {slot_num} - Engine ID {engine_id}")
        
        return preset
    
    def _filter_utility_engines(self):
        """Pre-filter corpus to exclude presets with utility engines"""
        from engine_mapping_authoritative import UTILITY_ENGINES
        
        original_count = len(self.presets)
        filtered_presets = []
        filtered_indices = []
        
        for idx, preset in enumerate(self.presets):
            has_utility_engine = False
            
            # Check both new format (slot_engine) and old format (engines array)
            if "slot1_engine" in preset:
                # New format - check all slots
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id in UTILITY_ENGINES:
                        has_utility_engine = True
                        break
            else:
                # Old format - check engines array
                engines = preset.get("engines", [])
                for engine_config in engines:
                    engine_type = engine_config.get("type", -1)
                    if engine_type in UTILITY_ENGINES:
                        has_utility_engine = True
                        break
            
            if not has_utility_engine:
                filtered_presets.append(preset)
                filtered_indices.append(idx)
        
        # Update presets list
        self.presets = filtered_presets
        
        # Update metadata to match filtered presets
        if self.metadata and len(self.metadata) == original_count:
            self.metadata = [self.metadata[i] for i in filtered_indices]
        
        # Store mapping from original indices to filtered preset indices
        self._original_to_filtered = {orig_idx: filt_idx for filt_idx, orig_idx in enumerate(filtered_indices)}
        self._valid_indices = set(filtered_indices)
        
        filtered_count = len(filtered_presets)
        logger.info(f"Oracle: Filtered {original_count - filtered_count} presets with utility engines, {filtered_count} remaining")
    
    def _get_preset_engines(self, preset: Dict[str, Any]) -> set:
        """Extract all engines from a preset"""
        engines = set()
        
        # Check both new format (slot_engine) and old format (engines array)
        if "slot1_engine" in preset:
            # New format - check all slots
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                if 0 < engine_id <= 56:
                    engines.add(engine_id)
        else:
            # Old format - check engines array
            for engine_config in preset.get("engines", []):
                engine_type = engine_config.get("type", -1)
                if 0 < engine_type <= 56:
                    engines.add(engine_type)
        
        return engines