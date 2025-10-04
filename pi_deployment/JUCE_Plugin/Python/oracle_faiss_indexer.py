#!/usr/bin/env python3
"""
Oracle FAISS Indexer for Chimera Phoenix Golden Corpus
Builds a FAISS index from the 250 Golden Corpus presets for ultra-fast similarity search
"""

import numpy as np
import faiss
import json
import os
import pickle
from typing import List, Dict, Tuple
from dataclasses import dataclass
from sklearn.preprocessing import StandardScaler
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@dataclass
class PresetVector:
    """Converts a Golden Corpus preset into a searchable vector"""
    
    # Vector dimensions breakdown:
    # - Sonic profile: 6 dimensions
    # - Emotional profile: 5 dimensions
    # - Engine presence: 50 dimensions (one-hot for each engine type)
    # - Engine slots used: 6 dimensions (mix levels)
    # - Complexity metrics: 4 dimensions
    # - Source affinity: 5 dimensions
    # Total: 76 dimensions
    
    VECTOR_DIM = 76
    
    @staticmethod
    def encode_preset(preset: Dict) -> np.ndarray:
        """Convert preset JSON to numpy vector"""
        vector = np.zeros(PresetVector.VECTOR_DIM, dtype=np.float32)
        idx = 0
        
        # 1. Sonic Profile (6 dimensions)
        sonic = preset.get('sonicProfile', {})
        vector[idx:idx+6] = [
            sonic.get('brightness', 0.5),
            sonic.get('density', 0.5),
            sonic.get('movement', 0.5),
            sonic.get('space', 0.5),
            sonic.get('aggression', 0.5),
            sonic.get('vintage', 0.5)
        ]
        idx += 6
        
        # 2. Emotional Profile (5 dimensions)
        emotional = preset.get('emotionalProfile', {})
        vector[idx:idx+5] = [
            emotional.get('energy', 0.5),
            emotional.get('mood', 0.5),
            emotional.get('tension', 0.5),
            emotional.get('organic', 0.5),
            emotional.get('nostalgia', 0.5)
        ]
        idx += 5
        
        # 3. Engine Presence (50 dimensions - one-hot encoding)
        engines = preset.get('engines', [])
        for engine in engines:
            engine_type = engine.get('type', -1)
            if 0 <= engine_type < 50:
                vector[idx + engine_type] = 1.0
        idx += 50
        
        # 4. Slot Usage Pattern (6 dimensions - mix levels)
        for i in range(6):
            found = False
            for engine in engines:
                if engine.get('slot') == i:
                    vector[idx + i] = engine.get('mix', 0.0)
                    found = True
                    break
            if not found:
                vector[idx + i] = 0.0
        idx += 6
        
        # 5. Complexity Metrics (4 dimensions)
        vector[idx:idx+4] = [
            preset.get('complexity', 0.5),
            preset.get('experimentalness', 0.5),
            len(engines) / 6.0,  # Normalized engine count
            preset.get('actualCpuPercent', 5.0) / 25.0  # Normalized CPU (0-25% range)
        ]
        idx += 4
        
        # 6. Source Affinity (5 dimensions)
        affinity = preset.get('sourceAffinity', {})
        vector[idx:idx+5] = [
            affinity.get('vocals', 0.5),
            affinity.get('guitar', 0.5),
            affinity.get('drums', 0.5),
            affinity.get('synth', 0.5),
            affinity.get('mix', 0.5)
        ]
        
        return vector


class GoldenCorpusIndex:
    """FAISS-powered preset search engine"""
    
    def __init__(self, vector_dim: int = 76):
        self.vector_dim = vector_dim
        self.index = None
        self.scaler = StandardScaler()
        self.preset_ids = []
        self.preset_data = {}
        
    def build_index(self, corpus_path: str):
        """Build FAISS index from Golden Corpus presets"""
        logger.info(f"Building index from {corpus_path}")
        
        # Load all presets
        presets_dir = os.path.join(corpus_path, "presets")
        if not os.path.exists(presets_dir):
            raise ValueError(f"Presets directory not found: {presets_dir}")
        
        vectors = []
        
        # Read all preset files
        preset_files = sorted([f for f in os.listdir(presets_dir) if f.endswith('.json')])
        logger.info(f"Found {len(preset_files)} preset files")
        
        for filename in preset_files:
            filepath = os.path.join(presets_dir, filename)
            try:
                with open(filepath, 'r') as f:
                    preset = json.load(f)
                
                # Convert to vector
                vector = PresetVector.encode_preset(preset)
                vectors.append(vector)
                
                # Store preset data
                preset_id = preset.get('id', filename.replace('.json', ''))
                self.preset_ids.append(preset_id)
                self.preset_data[preset_id] = preset
                
            except Exception as e:
                logger.error(f"Error processing {filename}: {e}")
                continue
        
        # Convert to numpy array
        vectors = np.array(vectors, dtype=np.float32)
        logger.info(f"Created {len(vectors)} vectors of dimension {vectors.shape[1]}")
        
        # Normalize vectors
        vectors = self.scaler.fit_transform(vectors)
        
        # Build FAISS index
        # Using IndexFlatL2 for exact search (can switch to approximate for larger corpus)
        self.index = faiss.IndexFlatL2(self.vector_dim)
        self.index.add(vectors.astype(np.float32))
        
        logger.info(f"Index built with {self.index.ntotal} presets")
    
    def search(self, query_vector: np.ndarray, k: int = 3) -> List[Tuple[str, float]]:
        """Find k nearest presets to query vector"""
        # Normalize query
        query_vector = query_vector.reshape(1, -1)
        query_vector = self.scaler.transform(query_vector).astype(np.float32)
        
        # Search
        distances, indices = self.index.search(query_vector, k)
        
        # Return preset IDs and distances
        results = []
        for dist, idx in zip(distances[0], indices[0]):
            if idx < len(self.preset_ids):
                preset_id = self.preset_ids[idx]
                results.append((preset_id, float(dist)))
        
        return results
    
    def get_preset(self, preset_id: str) -> Dict:
        """Retrieve full preset data"""
        return self.preset_data.get(preset_id)
    
    def save_index(self, output_path: str):
        """Save index to disk for fast loading"""
        os.makedirs(output_path, exist_ok=True)
        
        # Save FAISS index
        index_file = os.path.join(output_path, "corpus.index")
        faiss.write_index(self.index, index_file)
        logger.info(f"Saved FAISS index to {index_file}")
        
        # Save metadata
        meta_file = os.path.join(output_path, "corpus_meta.pkl")
        with open(meta_file, 'wb') as f:
            pickle.dump({
                'preset_ids': self.preset_ids,
                'preset_data': self.preset_data,
                'scaler': self.scaler,
                'vector_dim': self.vector_dim
            }, f)
        logger.info(f"Saved metadata to {meta_file}")
    
    def load_index(self, index_path: str):
        """Load pre-built index"""
        # Load FAISS index
        index_file = os.path.join(index_path, "corpus.index")
        self.index = faiss.read_index(index_file)
        
        # Load metadata
        meta_file = os.path.join(index_path, "corpus_meta.pkl")
        with open(meta_file, 'rb') as f:
            meta = pickle.load(f)
            self.preset_ids = meta['preset_ids']
            self.preset_data = meta['preset_data']
            self.scaler = meta['scaler']
            self.vector_dim = meta['vector_dim']
        
        logger.info(f"Loaded index with {self.index.ntotal} presets")


def analyze_corpus_coverage(index: GoldenCorpusIndex):
    """Analyze the vector space coverage of the corpus"""
    logger.info("\nAnalyzing corpus coverage...")
    
    # Get all vectors
    vectors = []
    for preset_id in index.preset_ids:
        preset = index.get_preset(preset_id)
        vector = PresetVector.encode_preset(preset)
        vectors.append(vector)
    
    vectors = np.array(vectors)
    
    # Analyze variance per dimension
    variances = np.var(vectors, axis=0)
    low_variance_dims = np.where(variances < 0.01)[0]
    
    # Analyze clustering
    from sklearn.cluster import KMeans
    kmeans = KMeans(n_clusters=25, random_state=42)
    labels = kmeans.fit_predict(vectors)
    cluster_sizes = np.bincount(labels)
    
    logger.info(f"Dimensions with low variance: {len(low_variance_dims)}")
    logger.info(f"Cluster size distribution: min={min(cluster_sizes)}, max={max(cluster_sizes)}, mean={np.mean(cluster_sizes):.1f}")
    logger.info(f"Overall coverage score: {np.mean(variances):.4f}")
    
    # Category distribution
    categories = {}
    for preset_id in index.preset_ids:
        preset = index.get_preset(preset_id)
        cat = preset.get('category', 'Unknown')
        categories[cat] = categories.get(cat, 0) + 1
    
    logger.info("\nCategory distribution:")
    for cat, count in sorted(categories.items()):
        logger.info(f"  {cat}: {count} presets")


def test_search(index: GoldenCorpusIndex):
    """Test the search functionality with example queries"""
    logger.info("\nTesting search functionality...")
    
    # Test 1: Find presets similar to a warm vocal preset
    test_vector = np.zeros(76, dtype=np.float32)
    # Sonic profile for warm vocals
    test_vector[0:6] = [0.7, 0.4, 0.2, 0.3, 0.1, 0.8]  # Bright, not dense, little movement, some space, gentle, vintage
    # Emotional profile
    test_vector[6:11] = [0.4, 0.7, 0.2, 0.8, 0.7]  # Calm, positive, relaxed, organic, nostalgic
    # Source affinity
    test_vector[71:76] = [1.0, 0.2, 0.1, 0.3, 0.2]  # Strongly for vocals
    
    results = index.search(test_vector, k=5)
    logger.info("\nSearch for warm vocal preset:")
    for preset_id, distance in results:
        preset = index.get_preset(preset_id)
        logger.info(f"  {preset_id}: {preset.get('name')} (distance: {distance:.3f})")
    
    # Test 2: Find experimental presets
    test_vector2 = np.zeros(76, dtype=np.float32)
    test_vector2[0:6] = [0.5, 0.9, 0.8, 0.7, 0.7, 0.1]  # Dense, moving, spacious, aggressive, modern
    test_vector2[6:11] = [0.8, 0.3, 0.7, 0.1, 0.1]  # Energetic, dark, tense, digital, not nostalgic
    test_vector2[67] = 0.9  # High complexity
    test_vector2[68] = 0.9  # High experimentalness
    
    results2 = index.search(test_vector2, k=5)
    logger.info("\nSearch for experimental preset:")
    for preset_id, distance in results2:
        preset = index.get_preset(preset_id)
        logger.info(f"  {preset_id}: {preset.get('name')} (distance: {distance:.3f})")


def main():
    # Paths
    corpus_path = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus"
    output_path = os.path.join(corpus_path, "faiss_index")
    
    # Create index
    index = GoldenCorpusIndex()
    
    # Build from corpus
    index.build_index(corpus_path)
    
    # Analyze coverage
    analyze_corpus_coverage(index)
    
    # Test search
    test_search(index)
    
    # Save index
    index.save_index(output_path)
    
    logger.info(f"\nFAISS index saved to {output_path}")
    logger.info("Ready for use in Oracle service!")


if __name__ == "__main__":
    main()