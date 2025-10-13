#!/usr/bin/env python3
"""
Oracle FAISS Index Builder
Builds or rebuilds the FAISS index for the Golden Corpus to enable fast similarity search.
"""

import json
import logging
import numpy as np
import faiss
import pickle
from pathlib import Path
from typing import Dict, Any, List
from collections import defaultdict

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class OracleFAISSBuilder:
    """Builds FAISS index for Oracle preset matching"""
    
    def __init__(self, 
                 corpus_path: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets.json",
                 output_dir: str = "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/"):
        """Initialize builder"""
        self.corpus_path = Path(corpus_path)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # Vector dimensions - match oracle_faiss.py
        self.VECTOR_DIM = 53
        
        logger.info(f"Building FAISS index from {corpus_path}")
        logger.info(f"Output directory: {output_dir}")
    
    def load_corpus(self) -> List[Dict[str, Any]]:
        """Load the corpus file"""
        try:
            with open(self.corpus_path, 'r') as f:
                data = json.load(f)
            
            if isinstance(data, list):
                presets = data
            elif isinstance(data, dict) and 'presets' in data:
                presets = data['presets']
            else:
                presets = [data] if isinstance(data, dict) else []
            
            logger.info(f"Loaded {len(presets)} presets from corpus")
            return presets
            
        except Exception as e:
            logger.error(f"Error loading corpus: {e}")
            return []
    
    def preset_to_vector(self, preset: Dict[str, Any]) -> np.ndarray:
        """Convert a preset to a feature vector for FAISS indexing"""
        vector = np.zeros(self.VECTOR_DIM, dtype=np.float32)
        
        # 1. Sonic characteristics (0-5)
        vector[0] = preset.get('brightness', 0.5)  # brightness
        vector[1] = preset.get('density', 0.5)     # density 
        vector[2] = preset.get('movement', 0.5)    # movement
        vector[3] = preset.get('space', 0.5)       # space
        vector[4] = preset.get('aggression', 0.5)  # aggression
        vector[5] = preset.get('vintage', 0.5)     # vintage
        
        # 2. Engine usage indicators (6-52)
        # Track which engines are used
        engine_features_start = 6
        max_engine_features = 47  # 53 - 6 = 47 features for engines
        
        engines_used = set()
        
        # Extract engines from new format (slot parameters)
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine")
            if engine_id and engine_id > 0:
                engines_used.add(int(engine_id))
        
        # Map engine IDs to feature space
        for engine_id in engines_used:
            if 1 <= engine_id <= 56:  # Valid engine range
                feature_idx = engine_features_start + (engine_id % max_engine_features)
                if feature_idx < self.VECTOR_DIM:
                    vector[feature_idx] = 1.0
        
        # 3. Complexity indicator (last index)
        complexity = preset.get('complexity', 0.5)
        if self.VECTOR_DIM > 52:
            vector[52] = complexity
        
        # Normalize certain ranges
        vector[0:6] = np.clip(vector[0:6], 0.0, 1.0)
        
        return vector
    
    def build_index(self) -> bool:
        """Build the FAISS index from the corpus"""
        try:
            # Load corpus
            presets = self.load_corpus()
            if not presets:
                logger.error("No presets found in corpus!")
                return False
            
            # Convert presets to vectors
            logger.info("Converting presets to vectors...")
            vectors = []
            metadata = []
            
            for i, preset in enumerate(presets):
                try:
                    vector = self.preset_to_vector(preset)
                    vectors.append(vector)
                    
                    # Store metadata
                    meta = {
                        "index": i,
                        "id": preset.get("id", f"preset_{i}"),
                        "creative_name": preset.get("creative_name", f"Preset {i+1}"),
                        "category": preset.get("category", ""),
                        "description": preset.get("description", ""),
                        "tags": preset.get("tags", [])
                    }
                    metadata.append(meta)
                    
                except Exception as e:
                    logger.warning(f"Error processing preset {i}: {e}")
                    continue
            
            if not vectors:
                logger.error("No valid vectors generated!")
                return False
            
            # Convert to numpy array
            vectors_array = np.array(vectors, dtype=np.float32)
            logger.info(f"Generated {len(vectors)} vectors of dimension {vectors_array.shape[1]}")
            
            # Create FAISS index
            logger.info("Building FAISS index...")
            index = faiss.IndexFlatL2(self.VECTOR_DIM)
            
            # Add vectors to index
            index.add(vectors_array)
            logger.info(f"Added {index.ntotal} vectors to FAISS index")
            
            # Save index
            index_path = self.output_dir / "corpus.index"
            faiss.write_index(index, str(index_path))
            logger.info(f"Saved FAISS index to {index_path}")
            
            # Save metadata
            metadata_path = self.output_dir / "metadata.json"
            with open(metadata_path, 'w') as f:
                json.dump(metadata, f, indent=2)
            logger.info(f"Saved metadata to {metadata_path}")
            
            # Verify the index
            self._verify_index(index, vectors_array, metadata)
            
            return True
            
        except Exception as e:
            logger.error(f"Error building index: {e}")
            return False
    
    def _verify_index(self, index: faiss.IndexFlatL2, vectors: np.ndarray, metadata: List[Dict]):
        """Verify the built index works correctly"""
        logger.info("Verifying FAISS index...")
        
        try:
            # Test search with first vector
            if len(vectors) > 0:
                test_vector = vectors[0:1]  # First vector as query
                distances, indices = index.search(test_vector, min(5, index.ntotal))
                
                logger.info(f"Test search results:")
                for i, (dist, idx) in enumerate(zip(distances[0], indices[0])):
                    if idx < len(metadata):
                        preset_name = metadata[idx]["creative_name"]
                        logger.info(f"  {i+1}. {preset_name} (distance: {dist:.4f})")
                
                logger.info("FAISS index verification successful!")
            
        except Exception as e:
            logger.error(f"Index verification failed: {e}")
    
    def rebuild_if_missing(self) -> bool:
        """Rebuild index only if it's missing or corrupted"""
        index_path = self.output_dir / "corpus.index"
        metadata_path = self.output_dir / "metadata.json"
        
        # Check if files exist and are valid
        if index_path.exists() and metadata_path.exists():
            try:
                # Try to load existing index
                index = faiss.read_index(str(index_path))
                with open(metadata_path, 'r') as f:
                    metadata = json.load(f)
                
                logger.info(f"Existing index found: {index.ntotal} vectors, {len(metadata)} metadata entries")
                
                # Basic validation
                if index.ntotal > 0 and len(metadata) > 0:
                    logger.info("Existing index appears valid, skipping rebuild")
                    return True
                    
            except Exception as e:
                logger.warning(f"Existing index corrupted: {e}")
        
        logger.info("Building new FAISS index...")
        return self.build_index()


def main():
    """Main function to build Oracle FAISS index"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Build Oracle FAISS index")
    parser.add_argument("--corpus", default="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets.json",
                        help="Path to corpus JSON file")
    parser.add_argument("--output", default="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/",
                        help="Output directory for index files")
    parser.add_argument("--force", action="store_true",
                        help="Force rebuild even if index exists")
    
    args = parser.parse_args()
    
    builder = OracleFAISSBuilder(args.corpus, args.output)
    
    try:
        if args.force:
            success = builder.build_index()
        else:
            success = builder.rebuild_if_missing()
        
        if success:
            logger.info("Oracle FAISS index build completed successfully!")
            return 0
        else:
            logger.error("Oracle FAISS index build failed!")
            return 1
            
    except Exception as e:
        logger.error(f"Build process failed: {e}")
        return 1


if __name__ == "__main__":
    exit(main())