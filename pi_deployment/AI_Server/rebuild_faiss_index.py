"""
Rebuild FAISS index from clean corpus
"""

import json
import numpy as np
import faiss
from pathlib import Path
from engine_mapping_authoritative import *

# Load clean corpus
corpus_path = Path("../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json")
with open(corpus_path) as f:
    presets = json.load(f)

print(f"Loaded {len(presets)} clean presets")

# Vector dimensions
VECTOR_DIM = 53

# Create FAISS index
index = faiss.IndexFlatL2(VECTOR_DIM)

# Convert presets to vectors
vectors = []
metadata = []

for preset in presets:
    vector = np.zeros(VECTOR_DIM, dtype=np.float32)
    
    # Engine presence (heavily weighted)
    for slot in range(1, 7):
        engine_id = preset.get(f"slot{slot}_engine", 0)
        if 0 < engine_id <= 52:  # Only musical engines
            feature_idx = 11 + (engine_id % 42)
            if feature_idx < VECTOR_DIM:
                vector[feature_idx] = 10.0
    
    vectors.append(vector)
    metadata.append({
        "id": preset.get("id"),
        "name": preset.get("creative_name"),
        "category": preset.get("category"),
        "engines": [preset.get(f"slot{s}_engine", 0) for s in range(1, 7) if preset.get(f"slot{s}_engine", 0) > 0]
    })

# Add to index
vectors_array = np.array(vectors, dtype=np.float32)
index.add(vectors_array)

# Save
output_dir = Path("../JUCE_Plugin/GoldenCorpus_v3/faiss_index")
faiss.write_index(index, str(output_dir / "corpus_clean.index"))

with open(output_dir / "metadata_clean.json", "w") as f:
    json.dump(metadata, f, indent=2)

print(f"Created clean index with {index.ntotal} vectors")
print("✅ No utility engines in clean corpus")
