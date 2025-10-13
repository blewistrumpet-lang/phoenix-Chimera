#!/usr/bin/env python3
"""
Start the Trinity Learning System Training
Improves preset generation from 73.6% baseline to 92% target
"""

import asyncio
from trinity_learning_configured import ElectronicLearningSystem

async def main():
    print("\n" + "="*80)
    print("TRINITY LEARNING SYSTEM - STARTING TRAINING")
    print("="*80)
    print("\nCurrent Performance: 73.6%")
    print("Target Performance: 92%")
    print("Training Generations: 50")
    print("\nThis will:")
    print("  • Evolve 30 pipeline configurations")
    print("  • Test each with 10 prompts per generation")
    print("  • Focus on improving engine selection (70% → 88%)")
    print("  • Enhance creativity scores (52% → 75%)")
    print("  • Save checkpoints every 10 generations")
    print("\n" + "="*80)
    
    # Initialize and run training
    system = ElectronicLearningSystem()
    
    # Run the training
    best_config = await system.run_training(generations=50)
    
    print("\n" + "="*80)
    print("TRAINING COMPLETE")
    print("="*80)
    print("\nBest configuration saved to: best_electronic_config.json")
    print("Apply it to main.py to use the optimized pipeline")
    
    return best_config

if __name__ == "__main__":
    asyncio.run(main())