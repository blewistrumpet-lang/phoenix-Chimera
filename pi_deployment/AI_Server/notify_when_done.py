#!/usr/bin/env python3
"""Notify when training completes"""

import time
import os
from pathlib import Path
import subprocess

print("📡 Monitoring training completion...")
print("   This script will notify you when done")
print("   You can close this terminal - training continues in background\n")

while True:
    if Path("best_electronic_config.json").exists():
        # Training complete!
        print("\n" + "="*60)
        print("🎉 TRAINING COMPLETE!")
        print("="*60)
        
        # macOS notification
        os.system("""
            osascript -e 'display notification "Trinity Learning Complete! Ready to apply optimized configuration." with title "AI Training Finished" sound name "Glass"'
        """)
        
        # Also try terminal-notifier if installed
        try:
            subprocess.run(["terminal-notifier", "-title", "Trinity Training", 
                          "-message", "Learning complete! Configuration optimized.", 
                          "-sound", "default"], capture_output=True)
        except:
            pass
        
        # Audio notification
        os.system("afplay /System/Library/Sounds/Glass.aiff")
        
        print("\n✅ Configuration saved to: best_electronic_config.json")
        print("📈 Run 'python3 apply_config.py' to use optimized settings")
        break
    
    # Check if training died
    result = os.system("ps aux | grep train_10_generations.py | grep -v grep > /dev/null 2>&1")
    if result != 0:
        print("\n⚠️ Training process stopped unexpectedly!")
        os.system("""
            osascript -e 'display notification "Training stopped unexpectedly" with title "AI Training Alert" sound name "Basso"'
        """)
        break
    
    time.sleep(60)  # Check every minute