#!/usr/bin/env python3
"""
Trinity Learning System - Quick 10 Generation Training
Gets us actionable results in ~20 minutes instead of 2 hours
"""

import asyncio
from trinity_learning_configured import ElectronicLearningSystem

async def main():
    print("\n" + "="*80)
    print("TRINITY LEARNING - 10 GENERATION QUICK TRAINING")
    print("="*80)
    print("\nCurrent Performance: 73.6%")
    print("Expected after 10 gens: ~82%")
    print("Full target (50 gens): 92%")
    print("\nThis quick training will:")
    print("  • Test 30 configurations per generation")
    print("  • Use 10 test prompts per configuration")
    print("  • Save checkpoint at generation 10")
    print("  • Take approximately 20-30 minutes")
    print("\n" + "="*80)
    
    # Initialize system
    system = ElectronicLearningSystem()
    
    # Run shortened training
    print("\n🚀 Starting accelerated training...")
    best_config = await system.run_training(generations=10)
    
    print("\n" + "="*80)
    print("10-GENERATION TRAINING COMPLETE")
    print("="*80)
    print("\n📊 Results saved to: best_electronic_config.json")
    print("\n🎯 Next steps:")
    print("  1. Review improvement metrics")
    print("  2. Test with sample prompts")
    print("  3. Decide whether to continue training")
    print("  4. Apply configuration if satisfied")
    
    return best_config

if __name__ == "__main__":
    import time
    start_time = time.time()
    
    asyncio.run(main())
    
    elapsed = time.time() - start_time
    print(f"\n⏱️  Training completed in {elapsed/60:.1f} minutes")