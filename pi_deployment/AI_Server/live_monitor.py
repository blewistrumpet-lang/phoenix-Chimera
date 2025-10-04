#!/usr/bin/env python3
"""Live monitoring of thorough training progress"""

import json
import time
from pathlib import Path
from datetime import datetime

print("\n" + "="*70)
print("TRINITY LEARNING SYSTEM - THOROUGH TRAINING MONITOR")
print("="*70)
print("Training: 30 genomes × 10 prompts × 10 generations")
print("This will be thorough and accurate, taking ~2-3 hours total")
print("-"*70)

start_time = datetime.now()
last_update = None

while True:
    # Check for latest checkpoint
    checkpoints = list(Path("checkpoints").glob("electronic_gen_*.json"))
    
    if checkpoints:
        latest = sorted(checkpoints, key=lambda x: int(x.stem.split('_')[-1]))[-1]
        gen_num = int(latest.stem.split('_')[-1])
        
        with open(latest, 'r') as f:
            data = json.load(f)
        
        fitness = data.get('best_fitness', 0)
        baseline = data.get('baseline', 0.736)
        improvement = (fitness / baseline - 1) * 100 if baseline > 0 else 0
        
        # Get best config details
        config = data.get('best_config', {})
        engine_weight = config.get('oracle', {}).get('engine_weight', 10.0)
        creativity = config.get('visionary', {}).get('creativity_bias', 0.5)
        
        # Calculate time metrics
        elapsed = (datetime.now() - start_time).total_seconds() / 60
        per_gen = elapsed / max(gen_num, 1)
        remaining = per_gen * (10 - gen_num)
        
        # Only update if changed
        update_key = f"{gen_num}-{fitness:.4f}"
        if update_key != last_update:
            last_update = update_key
            
            print(f"\n📊 Generation {gen_num}/10 | {datetime.now().strftime('%H:%M:%S')}")
            print(f"   Performance: {fitness:.1%} ({improvement:+.1f}% vs baseline)")
            print(f"   Progress:    [{'█' * (gen_num)}{'░' * (10-gen_num)}]")
            print(f"   Time:        {elapsed:.1f} min elapsed, ~{remaining:.0f} min remaining")
            print(f"   Config:      Engine weight={engine_weight:.1f}, Creativity={creativity:.2f}")
    
    # Check for completion
    if Path("best_electronic_config.json").exists():
        print("\n" + "="*70)
        print("✅ TRAINING COMPLETE!")
        print("="*70)
        
        with open("best_electronic_config.json", 'r') as f:
            final = json.load(f)
        
        print(f"\n📈 Final Results:")
        print(f"   Training time: {elapsed:.1f} minutes")
        print(f"   Final fitness: {fitness:.1%}")
        print(f"   Total improvement: {improvement:+.1f}%")
        
        print(f"\n🔧 Optimized Configuration Highlights:")
        print(f"   Engine selection weight: {final['oracle']['engine_weight']:.1f} (was 10.0)")
        print(f"   Creativity bias: {final['visionary']['creativity_bias']:.2f} (was 0.5)")
        print(f"   Keyword sensitivity: {final['calculator']['keyword_sensitivity']:.2f} (was 1.0)")
        print(f"   Max effects: {final['alchemist']['max_effects']} (was 6)")
        
        print(f"\n✨ Ready to apply optimized configuration!")
        break
    
    time.sleep(30)  # Check every 30 seconds