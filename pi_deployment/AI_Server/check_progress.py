#!/usr/bin/env python3
"""Quick check of training progress"""

import json
from pathlib import Path
import time
import os

# Check latest checkpoint
checkpoints = list(Path("checkpoints").glob("*.json"))
if checkpoints:
    latest = sorted(checkpoints)[-1]
    with open(latest, 'r') as f:
        data = json.load(f)
    
    gen = data.get('generation', 0)
    fitness = data.get('best_fitness', 0)
    baseline = data.get('baseline', 0.736)
    
    print(f"\n🏃 Training Status:")
    print(f"   Generation: {gen}/50")
    print(f"   Current Best: {fitness:.1%}")
    print(f"   Progress: {'█' * (gen//2)}{'░' * (25 - gen//2)}")
    
    if baseline > 0:
        improvement = (fitness / baseline - 1) * 100
        print(f"   Improvement: {improvement:+.1f}%")
else:
    print("\n⏳ Waiting for first checkpoint...")

# Check if training completed
if Path("best_electronic_config.json").exists():
    print("\n✅ TRAINING COMPLETE!")
    print("   Run: python3 apply_optimized_config.py")
else:
    print(f"\n⏱️  Estimated time remaining: {(50 - gen) * 2} minutes")