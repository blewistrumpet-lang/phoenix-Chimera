#!/usr/bin/env python3
"""
Integration Script - Replace existing components with enhanced versions
This script backs up and replaces the current components with the enhanced versions
"""

import shutil
import os
from pathlib import Path

def integrate_enhanced_system():
    """
    Integrate all enhanced components into the main system
    """
    print("🔧 INTEGRATING ENHANCED TRINITY SYSTEM")
    print("=" * 60)
    
    # Component mappings (enhanced -> original)
    components = {
        "alchemist_enhanced.py": "alchemist.py",
        "calculator_enhanced.py": "calculator.py",
        "oracle_enhanced.py": "oracle_faiss_fixed.py"
    }
    
    # Step 1: Backup original components
    print("\n1️⃣ Backing up original components...")
    backup_dir = Path("backups_original")
    backup_dir.mkdir(exist_ok=True)
    
    for enhanced, original in components.items():
        if Path(original).exists():
            backup_path = backup_dir / f"{original}.backup"
            shutil.copy2(original, backup_path)
            print(f"   ✅ Backed up {original}")
    
    # Step 2: Copy enhanced components
    print("\n2️⃣ Installing enhanced components...")
    for enhanced, original in components.items():
        if Path(enhanced).exists():
            shutil.copy2(enhanced, original)
            print(f"   ✅ Installed {enhanced} → {original}")
        else:
            print(f"   ❌ {enhanced} not found")
    
    # Step 3: Update main.py to use enhanced components
    print("\n3️⃣ Updating main.py imports...")
    main_updates = """
# At the top of main.py, update imports:
from oracle_faiss_fixed import OracleEnhanced as Oracle  # Use enhanced version
from calculator import CalculatorEnhanced as Calculator  # Use enhanced version  
from alchemist import AlchemistEnhanced as Alchemist  # Use enhanced version

# Add music theory and signal chain intelligence
from music_theory_intelligence import MusicTheoryIntelligence
from signal_chain_intelligence import SignalChainIntelligence
from engine_knowledge_base import ENGINE_KNOWLEDGE
"""
    
    print(main_updates)
    
    # Step 4: Create unified test script
    print("\n4️⃣ Creating unified test script...")
    
    test_script = '''#!/usr/bin/env python3
"""
Test the fully integrated enhanced system
"""

import requests
import json
import time
import subprocess

def test_enhanced_system():
    print("🧪 TESTING ENHANCED TRINITY SYSTEM")
    print("=" * 60)
    
    # Start the server
    print("\\nStarting server...")
    server = subprocess.Popen(
        ["python3", "main.py"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    time.sleep(3)  # Wait for server to start
    
    try:
        # Test cases
        test_prompts = [
            "Make my vocals warm and intimate like Billie Eilish",
            "Create aggressive metal guitar tone with tight gating",
            "Design an ethereal ambient pad with lots of space",
            "Jazz piano with vintage warmth and natural dynamics",
            "Modern pop vocals - bright, compressed, and polished"
        ]
        
        print("\\nTesting prompts:")
        for i, prompt in enumerate(test_prompts, 1):
            print(f"\\n{i}. {prompt}")
            
            response = requests.post(
                "http://localhost:8000/generate",
                json={"prompt": prompt},
                timeout=10
            )
            
            if response.status_code == 200:
                result = response.json()
                preset = result.get("preset", {})
                
                # Extract key information
                name = preset.get("name", "Unknown")
                engines = []
                for slot in range(1, 7):
                    engine_id = preset.get("parameters", {}).get(f"slot{slot}_engine", 0)
                    if engine_id > 0:
                        engines.append(engine_id)
                
                signal_flow = preset.get("signal_flow", "No flow")
                
                print(f"   ✅ Generated: {name}")
                print(f"   Engines: {engines[:3]}")
                print(f"   {signal_flow[:80]}...")
            else:
                print(f"   ❌ Failed: {response.status_code}")
                
    finally:
        # Stop server
        server.terminate()
        time.sleep(1)
        print("\\n✅ Test complete!")

if __name__ == "__main__":
    test_enhanced_system()
'''
    
    with open("test_enhanced_system.py", "w") as f:
        f.write(test_script)
    
    print("   ✅ Created test_enhanced_system.py")
    
    # Step 5: Fix import issues in enhanced components
    print("\n5️⃣ Fixing import issues...")
    
    # Fix calculator_enhanced.py
    fix_calculator = """
# Add this import at the top of calculator_enhanced.py:
from engine_defaults import ENGINE_DEFAULTS

# In the _suggest_and_add_engines method, change:
# if engine_id in ENGINE_DEFAULTS:
#     from engine_defaults import ENGINE_DEFAULTS  # Remove this line
# To just:
# if engine_id in ENGINE_DEFAULTS:
"""
    
    # Fix alchemist_enhanced.py  
    fix_alchemist = """
# The AlchemistEnhanced already has the correct imports
# Just ensure engine_defaults.py exists with ENGINE_DEFAULTS dict
"""
    
    print(fix_calculator)
    print(fix_alchemist)
    
    # Step 6: Summary
    print("\n" + "=" * 60)
    print("📊 INTEGRATION SUMMARY")
    print("=" * 60)
    
    print("""
✅ COMPLETED:
- Enhanced components created with deep musical intelligence
- Signal chain optimization integrated
- Music theory knowledge base built
- Genre and instrument processing added
- Safety validation enhanced
- Parameter relationship matrix implemented

⚠️ TO COMPLETE INTEGRATION:
1. Fix the import issues noted above
2. Replace imports in main.py with enhanced versions
3. Run test_enhanced_system.py to verify
4. Monitor for any runtime errors

🎯 EXPECTED RESULT:
- Signal chains automatically optimized
- Engines selected based on musical intent
- Parameters adjusted intelligently
- Safety validation prevents audio issues
- Clear explanations of processing chain
    """)

if __name__ == "__main__":
    integrate_enhanced_system()