#!/usr/bin/env python3
"""
Monitor Trinity Learning System Training Progress
"""

import json
from pathlib import Path
import time

def monitor():
    """Monitor training progress via checkpoint files"""
    
    print("\n" + "="*80)
    print("TRINITY LEARNING SYSTEM - TRAINING MONITOR")
    print("="*80)
    
    checkpoints_dir = Path("checkpoints")
    
    # Check for existing checkpoints
    if checkpoints_dir.exists():
        checkpoints = list(checkpoints_dir.glob("*.json"))
        if checkpoints:
            # Sort by generation number
            checkpoints.sort()
            latest = checkpoints[-1]
            
            with open(latest, 'r') as f:
                data = json.load(f)
            
            print(f"\n📊 Latest Checkpoint: {latest.name}")
            print(f"   Generation: {data.get('generation', 'N/A')}")
            print(f"   Baseline: {data.get('baseline', 0.736):.1%}")
            print(f"   Best Fitness: {data.get('best_fitness', 0):.1%}")
            
            if 'best_config' in data:
                improvement = (data['best_fitness'] / data.get('baseline', 0.736) - 1) * 100
                print(f"   Improvement: {improvement:+.1f}%")
                
                config = data['best_config']
                print(f"\n🔧 Current Best Configuration:")
                print(f"   Oracle engine_weight: {config.get('oracle', {}).get('engine_weight', 10.0):.1f}")
                print(f"   Visionary creativity: {config.get('visionary', {}).get('creativity_bias', 0.5):.2f}")
                print(f"   Calculator sensitivity: {config.get('calculator', {}).get('keyword_sensitivity', 1.0):.2f}")
                print(f"   Max effects limit: {config.get('alchemist', {}).get('max_effects', 6)}")
            
            if 'fitness_history' in data:
                history = data['fitness_history']
                if len(history) > 1:
                    print(f"\n📈 Progress Chart:")
                    for i in range(0, min(len(history), 50), 5):
                        bar_len = int(history[i] * 50)
                        print(f"   Gen {i:2d}: [{'█' * bar_len}{'░' * (50-bar_len)}] {history[i]:.1%}")
        else:
            print("\n⏳ No checkpoints yet - training just started...")
    else:
        print("\n⏳ Training not started yet or checkpoints directory not created...")
    
    # Check for final config
    if Path("best_electronic_config.json").exists():
        print("\n" + "="*80)
        print("✅ TRAINING COMPLETE!")
        print("="*80)
        
        with open("best_electronic_config.json", 'r') as f:
            final_config = json.load(f)
        
        print("\n🎯 Final Optimized Configuration:")
        print(json.dumps(final_config, indent=2))
        
        print("\n📝 Next step: Apply configuration to main.py")
        return True
    
    return False

if __name__ == "__main__":
    # Monitor until training completes
    while True:
        if monitor():
            break
        else:
            print("\n⏳ Training in progress... (checking again in 30 seconds)")
            time.sleep(30)