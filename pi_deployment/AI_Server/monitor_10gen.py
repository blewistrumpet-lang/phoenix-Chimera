#!/usr/bin/env python3
"""Monitor 10-generation training progress"""

import json
import time
from pathlib import Path
from datetime import datetime

def monitor():
    start_time = datetime.now()
    last_gen = -1
    
    print("\n" + "="*60)
    print("TRAINING MONITOR - 10 GENERATIONS")
    print("="*60)
    
    while True:
        # Check for checkpoints
        checkpoints = list(Path("checkpoints").glob("electronic_gen_*.json"))
        
        if checkpoints:
            latest = sorted(checkpoints, key=lambda x: int(x.stem.split('_')[-1]))[-1]
            gen_num = int(latest.stem.split('_')[-1])
            
            with open(latest, 'r') as f:
                data = json.load(f)
            
            if gen_num > last_gen:
                last_gen = gen_num
                fitness = data.get('best_fitness', 0)
                baseline = data.get('baseline', 0.736)
                
                if baseline > 0:
                    improvement = (fitness / baseline - 1) * 100
                else:
                    improvement = 0
                
                elapsed = (datetime.now() - start_time).total_seconds() / 60
                
                print(f"\nGeneration {gen_num}/10:")
                print(f"  Fitness: {fitness:.1%} ({improvement:+.1f}% vs baseline)")
                print(f"  Time: {elapsed:.1f} minutes")
                print(f"  Progress: [{'█' * gen_num}{'░' * (10-gen_num)}]")
                
                if gen_num >= 10:
                    print("\n✅ 10 GENERATIONS COMPLETE!")
                    break
        
        # Check for completion
        if Path("best_electronic_config.json").exists():
            print("\n✅ Training finished! Configuration saved.")
            with open("best_electronic_config.json", 'r') as f:
                config = json.load(f)
            
            print("\n📊 Key Changes from Baseline:")
            print(f"  Oracle engine_weight: {config['oracle']['engine_weight']:.1f} (was 10.0)")
            print(f"  Visionary creativity: {config['visionary']['creativity_bias']:.2f} (was 0.5)")
            print(f"  Calculator sensitivity: {config['calculator']['keyword_sensitivity']:.2f} (was 1.0)")
            print(f"  Max effects: {config['alchemist']['max_effects']} (was 6)")
            break
        
        time.sleep(15)  # Check every 15 seconds

if __name__ == "__main__":
    monitor()