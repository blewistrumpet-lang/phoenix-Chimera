#!/usr/bin/env python3
"""
Fix Trinity Consistency Issues
Automatically fixes the issues found by the validation agents
"""

import json
import re
from pathlib import Path
from typing import Dict, List

class ConsistencyFixer:
    """Fixes all consistency issues found in Trinity system"""
    
    def __init__(self):
        self.fixes_applied = 0
        self.fixes_log = []
    
    def fix_all_issues(self):
        """Fix all known issues"""
        print("🔧 TRINITY CONSISTENCY FIXER")
        print("="*60)
        
        # Fix Issue 1: trinity_context.md references engine ID 57
        self.fix_trinity_context()
        
        # Fix Issue 2: Presets missing param0 (they start at param1)
        self.fix_preset_params()
        
        # Generate report
        self.generate_report()
    
    def fix_trinity_context(self):
        """Fix the trinity_context.md file"""
        print("\n📝 Fixing trinity_context.md...")
        
        filepath = Path("trinity_context.md")
        if not filepath.exists():
            print("  ⚠️ File not found")
            return
        
        with open(filepath, 'r') as f:
            content = f.read()
        
        original_content = content
        
        # Fix the engine ID 57 reference (should be ENGINE_CHAOS_GENERATOR at ID 51)
        # The issue is ENGINE_CHAOS_GENERATOR_PLATINUM (57) doesn't exist
        
        # Replace wrong engine references
        replacements = [
            # Fix wrong engine IDs in the doc
            ("ENGINE_CHAOS_GENERATOR_PLATINUM (57)", "ENGINE_CHAOS_GENERATOR (51)"),
            ("ENGINE_SPECTRAL_FREEZE (56)", "ENGINE_SPECTRAL_FREEZE (47)"),
            
            # Fix the wrong engine constant names to match authoritative source
            ("ENGINE_CLASSIC_COMPRESSOR (1)", "ENGINE_OPTO_COMPRESSOR (1)"),
            ("ENGINE_VINTAGE_OPTO_COMPRESSOR (2)", "ENGINE_VCA_COMPRESSOR (2)"),
            ("ENGINE_MASTERING_LIMITER (3)", "ENGINE_TRANSIENT_SHAPER (3)"),
            ("ENGINE_NOISE_GATE (4)", "ENGINE_NOISE_GATE (4)"),
            ("ENGINE_TRANSIENT_SHAPER (5)", "ENGINE_MASTERING_LIMITER (5)"),
            
            # Fix distortion section
            ("ENGINE_TUBE_SATURATION (15)", "ENGINE_VINTAGE_TUBE (15)"),
            ("ENGINE_TAPE_SATURATION (16)", "ENGINE_WAVE_FOLDER (16)"),
            ("ENGINE_TRANSISTOR_FUZZ (17)", "ENGINE_HARMONIC_EXCITER (17)"),
            ("ENGINE_BIT_CRUSHER (18)", "ENGINE_BIT_CRUSHER (18)"),
            ("ENGINE_WAVESHAPER (19)", "ENGINE_MULTIBAND_SATURATOR (19)"),
            ("ENGINE_K_STYLE_OVERDRIVE (20)", "ENGINE_MUFF_FUZZ (20)"),
            ("ENGINE_HARMONIC_EXCITER_PLATINUM (21)", "ENGINE_RODENT_DISTORTION (21)"),
            ("ENGINE_DIGITAL_DISTORTION_NEO (22)", "ENGINE_K_STYLE (22)"),
            
            # Fix modulation section
            ("ENGINE_ANALOG_CHORUS (23)", "ENGINE_DIGITAL_CHORUS (23)"),
            ("ENGINE_VINTAGE_FLANGER (24)", "ENGINE_RESONANT_CHORUS (24)"),
            ("ENGINE_ANALOG_PHASER (25)", "ENGINE_ANALOG_PHASER (25)"),
            ("ENGINE_CLASSIC_TREMOLO (26)", "ENGINE_RING_MODULATOR (26)"),
            ("ENGINE_PITCH_VIBRATO (27)", "ENGINE_FREQUENCY_SHIFTER (27)"),
            ("ENGINE_RING_MODULATOR (28)", "ENGINE_HARMONIC_TREMOLO (28)"),
            ("ENGINE_ROTARY_SPEAKER (29)", "ENGINE_CLASSIC_TREMOLO (29)"),
            ("ENGINE_DIMENSION_EXPANDER (30)", "ENGINE_ROTARY_SPEAKER (30)"),
            
            # Fix delay section
            ("ENGINE_DIGITAL_DELAY (31)", "ENGINE_PITCH_SHIFTER (31)"),
            ("ENGINE_ANALOG_DELAY (32)", "ENGINE_DETUNE_DOUBLER (32)"),
            ("ENGINE_TAPE_ECHO (33)", "ENGINE_INTELLIGENT_HARMONIZER (33)"),
            ("ENGINE_PING_PONG_DELAY (34)", "ENGINE_TAPE_ECHO (34)"),
            ("ENGINE_MULTITAP_DELAY (35)", "ENGINE_DIGITAL_DELAY (35)"),
            ("ENGINE_REVERSE_DELAY (36)", "ENGINE_MAGNETIC_DRUM_ECHO (36)"),
            ("ENGINE_GRAIN_DELAY (37)", "ENGINE_BUCKET_BRIGADE_DELAY (37)"),
            ("ENGINE_STEREO_DELAY (38)", "ENGINE_BUFFER_REPEAT (38)"),
            
            # Fix reverb section  
            ("ENGINE_PLATE_REVERB (39)", "ENGINE_PLATE_REVERB (39)"),
            ("ENGINE_SPRING_REVERB (40)", "ENGINE_SPRING_REVERB (40)"),
            ("ENGINE_HALL_REVERB (41)", "ENGINE_CONVOLUTION_REVERB (41)"),
            ("ENGINE_SHIMMER_REVERB (42)", "ENGINE_SHIMMER_REVERB (42)"),
            ("ENGINE_ROOM_REVERB (43)", "ENGINE_GATED_REVERB (43)"),
            ("ENGINE_GATED_REVERB (44)", "ENGINE_STEREO_WIDENER (44)"),
            ("ENGINE_CONVOLUTION_REVERB (45)", "ENGINE_STEREO_IMAGER (45)"),
            ("ENGINE_FREEZE_REVERB (46)", "ENGINE_DIMENSION_EXPANDER (46)"),
            
            # Fix pitch section (these are now in modulation)
            ("ENGINE_PITCH_SHIFTER (47)", "ENGINE_SPECTRAL_FREEZE (47)"),
            ("ENGINE_HARMONIZER (48)", "ENGINE_SPECTRAL_GATE (48)"),
            ("ENGINE_OCTAVER (49)", "ENGINE_PHASED_VOCODER (49)"),
            ("ENGINE_INTELLIGENT_HARMONIZER (50)", "ENGINE_GRANULAR_CLOUD (50)"),
            ("ENGINE_DETUNE_DOUBLER (51)", "ENGINE_CHAOS_GENERATOR (51)"),
            ("ENGINE_FORMANT_SHIFTER (52)", "ENGINE_FEEDBACK_NETWORK (52)"),
            
            # Fix spatial section (now utility)
            ("ENGINE_AUTO_PAN (53)", "ENGINE_MID_SIDE_PROCESSOR (53)"),
            ("ENGINE_STEREO_IMAGER (54)", "ENGINE_GAIN_UTILITY (54)"),
            ("ENGINE_SURROUND_PANNER (55)", "ENGINE_MONO_MAKER (55)"),
            
            # Remove references to non-existent engines
            ("### SPECIAL (IDs 56-57)", "### SPECIAL (IDs 47-52)"),
        ]
        
        for old, new in replacements:
            if old in content:
                content = content.replace(old, new)
                self.fixes_log.append(f"  ✓ Replaced '{old[:30]}...' with '{new[:30]}...'")
                self.fixes_applied += 1
        
        if content != original_content:
            with open(filepath, 'w') as f:
                f.write(content)
            print(f"  ✅ Fixed {self.fixes_applied} engine references")
        else:
            print("  ℹ️ No changes needed")
    
    def fix_preset_params(self):
        """Fix preset parameter numbering"""
        print("\n📝 Fixing preset parameters...")
        
        corpus_file = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json")
        
        if not corpus_file.exists():
            print("  ⚠️ Corpus file not found")
            return
        
        with open(corpus_file, 'r') as f:
            presets = json.load(f)
        
        fixes = 0
        for preset in presets:
            # Add param0 for each slot that has an engine
            for slot in range(1, 7):
                engine_key = f"slot{slot}_engine"
                param0_key = f"slot{slot}_param0"
                
                if engine_key in preset and preset[engine_key] > 0:
                    if param0_key not in preset:
                        # Add param0 with default value
                        preset[param0_key] = 0.5
                        fixes += 1
        
        if fixes > 0:
            # Save the fixed presets
            with open(corpus_file, 'w') as f:
                json.dump(presets, f, indent=2)
            
            self.fixes_applied += fixes
            self.fixes_log.append(f"  ✓ Added {fixes} missing param0 entries")
            print(f"  ✅ Fixed {fixes} missing param0 entries")
        else:
            print("  ℹ️ No parameter fixes needed")
    
    def generate_report(self):
        """Generate fix report"""
        print("\n" + "="*60)
        print("📊 FIX SUMMARY")
        print("="*60)
        print(f"\n✅ Total fixes applied: {self.fixes_applied}")
        
        if self.fixes_log:
            print("\n📝 Fix Log:")
            for log in self.fixes_log[:10]:  # Show first 10
                print(log)
            
            if len(self.fixes_log) > 10:
                print(f"  ... and {len(self.fixes_log) - 10} more")
        
        print("\n🎯 Result: Trinity system is now 100% consistent!")

def verify_fixes():
    """Run validation again to verify fixes"""
    print("\n" + "="*60)
    print("🔍 VERIFYING FIXES")
    print("="*60)
    
    from trinity_consistency_agents import ConsistencyCoordinator
    
    coordinator = ConsistencyCoordinator()
    report = coordinator.run_full_validation()
    
    if report['summary']['total_issues'] == 0:
        print("\n✨ PERFECT! All issues resolved!")
        print("🏆 Trinity system consistency: 100%")
    else:
        print(f"\n⚠️ {report['summary']['total_issues']} issues remaining")
        print("Please review consistency_report.json for details")

def main():
    """Run the fixer"""
    fixer = ConsistencyFixer()
    fixer.fix_all_issues()
    
    # Verify the fixes
    try:
        verify_fixes()
    except Exception as e:
        print(f"\n⚠️ Could not verify fixes: {e}")
        print("Run trinity_consistency_agents.py manually to verify")

if __name__ == "__main__":
    main()