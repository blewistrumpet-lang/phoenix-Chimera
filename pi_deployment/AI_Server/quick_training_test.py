#!/usr/bin/env python3
"""
Quick test of Trinity Learning System - 5 generations only
"""

import asyncio
from trinity_learning_configured import ElectronicLearningSystem

async def main():
    print("\n" + "="*80)
    print("TRINITY LEARNING - QUICK TEST (5 GENERATIONS)")
    print("="*80)
    
    system = ElectronicLearningSystem()
    
    # Quick test with 5 generations
    best_config = await system.run_training(generations=5)
    
    print("\n" + "="*80)
    print("QUICK TEST COMPLETE")
    print("="*80)
    
    return best_config

if __name__ == "__main__":
    asyncio.run(main())