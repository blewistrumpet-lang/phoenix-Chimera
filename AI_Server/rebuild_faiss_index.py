"""
Rebuild FAISS Index for New Experimental Corpus
Creates FAISS index from the new GoldenCorpus_v3 with proper 57-engine architecture
"""

import json
import numpy as np
import faiss
from pathlib import Path
import os

def extract_features(preset):
    """Extract feature vector from preset for FAISS indexing"""
    features = []
    
    # Engine configuration (6 engines)
    for slot in range(1, 7):
        engine_key = f"slot{slot}_engine"
        features.append(preset.get(engine_key, 0) / 56.0)  # Normalize engine ID
    
    # Key parameters from each slot (first 5 params)
    for slot in range(1, 7):
        for param in range(1, 6):
            param_key = f"slot{slot}_param{param}"
            features.append(preset.get(param_key, 0.0))
    
    # Mix levels
    for slot in range(1, 7):
        mix_key = f"slot{slot}_param15"
        features.append(preset.get(mix_key, 0.0))
    
    # Metadata features
    features.append(preset.get("complexity", 0.5))
    features.append(preset.get("brightness", 0.5))
    features.append(preset.get("warmth", 0.5))
    
    # Category encoding (one-hot)
    categories = ["Experimental", "Ambient", "Glitch", "Lo-Fi", "Spatial", 
                 "Harmonic", "Rhythmic", "Textural"]
    category = preset.get("category", "Experimental")
    for cat in categories:
        features.append(1.0 if cat == category else 0.0)
    
    return np.array(features, dtype=np.float32)

def build_faiss_index(corpus_dir):
    """Build FAISS index from corpus"""
    presets_dir = Path(corpus_dir) / "presets"
    
    # Load all presets
    presets = []
    preset_files = sorted(presets_dir.glob("*.json"))
    
    print(f"Loading {len(preset_files)} presets...")
    
    for preset_file in preset_files:
        with open(preset_file) as f:
            preset = json.load(f)
            presets.append(preset)
    
    # Extract features
    print("Extracting features...")
    feature_matrix = []
    metadata = []
    
    for preset in presets:
        features = extract_features(preset)
        feature_matrix.append(features)
        
        # Store metadata
        metadata.append({
            "id": preset.get("id", preset_file.stem),
            "creative_name": preset.get("creative_name", "Unknown"),
            "category": preset.get("category", "Experimental"),
            "description": preset.get("description", ""),
            "tags": preset.get("tags", [])
        })
    
    feature_matrix = np.array(feature_matrix, dtype=np.float32)
    print(f"Feature matrix shape: {feature_matrix.shape}")
    
    # Create FAISS index
    print("Building FAISS index...")
    dimension = feature_matrix.shape[1]
    
    # Use L2 distance with normalization
    index = faiss.IndexFlatL2(dimension)
    
    # Add vectors to index
    index.add(feature_matrix)
    
    return index, metadata, presets

def save_faiss_index(index, metadata, presets, output_dir):
    """Save FAISS index and metadata"""
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Save FAISS index
    index_file = output_path / "corpus.index"
    faiss.write_index(index, str(index_file))
    print(f"Saved FAISS index to {index_file}")
    
    # Save metadata
    metadata_file = output_path / "metadata.json"
    with open(metadata_file, "w") as f:
        json.dump(metadata, f, indent=2)
    print(f"Saved metadata to {metadata_file}")
    
    # Save full presets for Oracle
    presets_file = output_path / "presets.json"
    with open(presets_file, "w") as f:
        json.dump(presets, f, indent=2)
    print(f"Saved presets to {presets_file}")

def test_similarity_search(index, metadata, query_preset):
    """Test similarity search"""
    query_features = extract_features(query_preset)
    query_features = query_features.reshape(1, -1)
    
    # Search for similar presets
    k = 5  # Find 5 nearest neighbors
    distances, indices = index.search(query_features, k)
    
    print("\nSimilarity search results:")
    for i, (dist, idx) in enumerate(zip(distances[0], indices[0])):
        preset_meta = metadata[idx]
        print(f"{i+1}. {preset_meta['creative_name']} (category: {preset_meta['category']}, distance: {dist:.3f})")

if __name__ == "__main__":
    # Paths
    corpus_dir = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3"
    output_dir = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3/faiss_index"
    
    # Build index
    index, metadata, presets = build_faiss_index(corpus_dir)
    
    # Save index
    save_faiss_index(index, metadata, presets, output_dir)
    
    # Test with a sample query
    print("\n=== Testing Similarity Search ===")
    test_preset = presets[0]  # Use first preset as query
    test_similarity_search(index, metadata, test_preset)
    
    print("\n=== FAISS Index Build Complete ===")
    print(f"Total presets indexed: {len(presets)}")
    print(f"Feature dimension: {index.d}")
    print(f"Index type: {type(index).__name__}")