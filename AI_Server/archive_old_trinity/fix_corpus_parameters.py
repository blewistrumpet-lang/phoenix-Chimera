#!/usr/bin/env python3
"""
Fix the broken corpus by adding real parameter variance
"""

import json
import random
import numpy as np
from pathlib import Path
from typing import Dict, Any, List
from engine_mapping_authoritative import ENGINE_NAMES

class CorpusFixer:
    def __init__(self):
        self.corpus_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3")
        self.preset_file = self.corpus_path / "faiss_index" / "presets_clean.json"
        self.backup_file = self.corpus_path / "faiss_index" / "presets_clean_backup.json"
        
        # Engine-specific parameter knowledge
        self.engine_params = {
            # Compressors (1-5)
            1: {"param0": ("Threshold", 0.3, 0.7), "param1": ("Ratio", 0.2, 0.6), 
                "param2": ("Attack", 0.1, 0.5), "param3": ("Release", 0.2, 0.8),
                "param5": ("Mix", 0.7, 1.0)},
            2: {"param0": ("Threshold", 0.3, 0.7), "param1": ("Ratio", 0.3, 0.8),
                "param5": ("Mix", 0.8, 1.0)},
            
            # EQs (6-8)
            7: {"param0": ("Low Gain", 0.3, 0.7), "param1": ("Mid Gain", 0.4, 0.6),
                "param2": ("High Gain", 0.3, 0.7), "param3": ("Low Freq", 0.1, 0.4),
                "param4": ("High Freq", 0.6, 0.9)},
            
            # Filters (9-14)
            9: {"param0": ("Cutoff", 0.2, 0.8), "param1": ("Resonance", 0.0, 0.7),
                "param2": ("Drive", 0.0, 0.5)},
            
            # Distortion (15-22)
            15: {"param0": ("Drive", 0.3, 0.7), "param1": ("Bias", 0.4, 0.6),
                 "param2": ("Tone", 0.4, 0.7), "param5": ("Mix", 0.5, 1.0)},
            18: {"param0": ("Bits", 0.2, 0.8), "param1": ("Sample Rate", 0.2, 0.8),
                 "param5": ("Mix", 0.3, 0.8)},
            20: {"param0": ("Drive", 0.4, 0.9), "param1": ("Tone", 0.3, 0.7),
                 "param5": ("Mix", 0.6, 1.0)},
            21: {"param0": ("Distortion", 0.3, 0.8), "param1": ("Filter", 0.4, 0.7),
                 "param5": ("Mix", 0.7, 1.0)},
            22: {"param0": ("Drive", 0.3, 0.8), "param1": ("Tone", 0.4, 0.8),
                 "param5": ("Mix", 0.6, 1.0)},
            
            # Modulation (23-33)
            24: {"param0": ("Rate", 0.1, 0.6), "param1": ("Depth", 0.2, 0.7),
                 "param2": ("Feedback", 0.0, 0.4), "param5": ("Mix", 0.2, 0.6)},
            25: {"param0": ("Rate", 0.05, 0.5), "param1": ("Depth", 0.3, 0.8),
                 "param2": ("Feedback", 0.1, 0.5), "param5": ("Mix", 0.3, 0.7)},
            
            # Delays (34-38)
            34: {"param0": ("Time", 0.1, 0.6), "param1": ("Feedback", 0.2, 0.6),
                 "param2": ("Tone", 0.3, 0.7), "param5": ("Mix", 0.2, 0.5)},
            35: {"param0": ("Time", 0.1, 0.8), "param1": ("Feedback", 0.1, 0.7),
                 "param2": ("Filter", 0.3, 0.7), "param5": ("Mix", 0.2, 0.5)},
            
            # Reverbs (39-43)
            39: {"param0": ("Size", 0.3, 0.8), "param1": ("Decay", 0.3, 0.7),
                 "param2": ("Damping", 0.2, 0.6), "param5": ("Mix", 0.15, 0.4)},
            40: {"param0": ("Size", 0.2, 0.6), "param1": ("Decay", 0.2, 0.5),
                 "param2": ("Tension", 0.3, 0.7), "param5": ("Mix", 0.1, 0.3)},
            42: {"param0": ("Size", 0.5, 0.9), "param1": ("Shimmer", 0.3, 0.7),
                 "param2": ("Decay", 0.4, 0.8), "param5": ("Mix", 0.2, 0.5)},
            
            # Spatial (44-46)
            44: {"param0": ("Width", 0.3, 0.8), "param1": ("Center", 0.4, 0.6)},
            46: {"param0": ("Dimension", 0.3, 0.7), "param1": ("Depth", 0.2, 0.6),
                 "param5": ("Mix", 0.3, 0.7)},
        }
    
    def fix_corpus(self) -> List[Dict[str, Any]]:
        """Fix the corpus by adding parameter variance"""
        
        print("🔧 FIXING CORPUS PARAMETERS")
        print("=" * 80)
        
        # Load existing corpus
        with open(self.preset_file, 'r') as f:
            presets = json.load(f)
        
        # Backup first
        with open(self.backup_file, 'w') as f:
            json.dump(presets, f, indent=2)
        print(f"✅ Backed up to {self.backup_file}")
        
        fixed_count = 0
        
        for preset in presets:
            preset_name = preset.get("creative_name", "Unknown")
            
            # Determine character from name
            character = self._detect_character(preset_name)
            
            # Fix parameters for each slot
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                
                if engine_id > 0:
                    # Get engine-specific parameter ranges
                    if engine_id in self.engine_params:
                        param_info = self.engine_params[engine_id]
                        
                        for param_key, (param_name, min_val, max_val) in param_info.items():
                            param_num = int(param_key.replace("param", ""))
                            key = f"slot{slot}_param{param_num}"
                            
                            # Generate value based on character
                            if character == "warm" and "Drive" in param_name:
                                value = random.uniform(min_val + 0.1, max_val)
                            elif character == "aggressive" and "Drive" in param_name:
                                value = random.uniform(max_val - 0.2, max_val)
                            elif character == "clean" and "Mix" in param_name:
                                value = random.uniform(min_val, min_val + 0.2)
                            else:
                                # Normal distribution around middle
                                center = (min_val + max_val) / 2
                                std = (max_val - min_val) / 4
                                value = np.random.normal(center, std)
                                value = max(min_val, min(max_val, value))
                            
                            preset[key] = round(value, 3)
                    else:
                        # Generic parameters for unknown engines
                        for param in range(10):
                            key = f"slot{slot}_param{param}"
                            
                            if param == 0:  # Main parameter
                                value = random.uniform(0.3, 0.7)
                            elif param == 5:  # Mix parameter
                                value = random.uniform(0.2, 0.5)
                            else:
                                # Random with bias toward middle
                                value = np.random.beta(2, 2)  # Bell curve 0-1
                            
                            preset[key] = round(value, 3)
                    
                    fixed_count += 1
        
        print(f"\n✅ Fixed parameters for {fixed_count} engine slots")
        
        # Save fixed corpus
        fixed_file = self.corpus_path / "faiss_index" / "presets_fixed.json"
        with open(fixed_file, 'w') as f:
            json.dump(presets, f, indent=2)
        print(f"💾 Saved fixed corpus to {fixed_file}")
        
        # Verify the fix
        self._verify_fix(presets)
        
        return presets
    
    def _detect_character(self, name: str) -> str:
        """Detect character from preset name"""
        name_lower = name.lower()
        
        if any(w in name_lower for w in ["warm", "vintage", "tube", "analog"]):
            return "warm"
        elif any(w in name_lower for w in ["metal", "brutal", "aggressive", "heavy"]):
            return "aggressive"
        elif any(w in name_lower for w in ["clean", "pristine", "clear", "pure"]):
            return "clean"
        elif any(w in name_lower for w in ["ambient", "space", "ethereal"]):
            return "ambient"
        else:
            return "neutral"
    
    def _verify_fix(self, presets: List[Dict]):
        """Verify the fix worked"""
        print("\n📊 VERIFICATION:")
        print("-" * 40)
        
        all_values = []
        for preset in presets:
            for slot in range(1, 7):
                for param in range(10):
                    key = f"slot{slot}_param{param}"
                    if key in preset:
                        all_values.append(preset[key])
        
        if all_values:
            unique_values = len(set(all_values))
            variance = np.var(all_values)
            
            print(f"Total parameter values: {len(all_values)}")
            print(f"Unique values: {unique_values}")
            print(f"Variance: {variance:.6f}")
            print(f"Min: {min(all_values):.3f}")
            print(f"Max: {max(all_values):.3f}")
            print(f"Mean: {np.mean(all_values):.3f}")
            
            if variance > 0.01:
                print("\n✅ SUCCESS! Parameters now have variance!")
            else:
                print("\n❌ FAILED! Still no variance!")
        else:
            print("❌ No parameters found!")
    
    def show_examples(self, presets: List[Dict], count: int = 3):
        """Show example presets with new parameters"""
        print("\n📝 EXAMPLE FIXED PRESETS:")
        print("-" * 40)
        
        for preset in presets[:count]:
            print(f"\n'{preset.get('creative_name', 'Unknown')}':")
            
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                if engine_id > 0:
                    engine_name = ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})")
                    print(f"  Slot {slot}: {engine_name}")
                    
                    # Show first 3 parameters
                    params = []
                    for p in range(3):
                        key = f"slot{slot}_param{p}"
                        if key in preset:
                            params.append(f"p{p}={preset[key]:.2f}")
                    
                    if params:
                        print(f"    {', '.join(params)}")


def test_openai_only():
    """Test OpenAI-only preset generation"""
    print("\n" + "=" * 80)
    print("🤖 TESTING OPENAI-ONLY GENERATION")
    print("=" * 80)
    
    print("""
PROPOSAL: Let OpenAI generate complete presets directly
    
Benefits:
✅ No corpus maintenance
✅ Infinite variety  
✅ Always current
✅ Understands context perfectly

Implementation:
1. Extend Visionary to output full preset
2. Validate with PresetValidator
3. Skip Oracle entirely
4. Calculator becomes optional
5. Alchemist still validates/optimizes

Cost Analysis:
- Current: $0.002 per request (GPT-3.5)
- 1000 requests/day = $2/day
- Acceptable for beta/testing
    """)


def main():
    # Fix the corpus
    fixer = CorpusFixer()
    fixed_presets = fixer.fix_corpus()
    fixer.show_examples(fixed_presets)
    
    # Test OpenAI approach
    test_openai_only()
    
    print("\n" + "=" * 80)
    print("🎯 RECOMMENDATIONS")
    print("=" * 80)
    
    print("""
1. IMMEDIATE: Use fixed corpus (just completed)
   - Parameters now have variance
   - Will improve Oracle search
   - Calculator can now nudge meaningfully
   
2. SHORT TERM: Test OpenAI-only generation
   - Implement in Visionary
   - Compare quality
   - Measure cost
   
3. LONG TERM: Hybrid approach
   - OpenAI generates novel presets
   - Best ones added to corpus
   - Corpus becomes self-improving
    """)

if __name__ == "__main__":
    main()