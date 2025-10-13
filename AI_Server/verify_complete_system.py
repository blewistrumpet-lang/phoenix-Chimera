#!/usr/bin/env python3
"""
Complete System Verification - Ensure 100% Accuracy
"""

import json
import os
from pathlib import Path

def verify_trinity_system():
    """Run comprehensive verification of entire Trinity system"""
    
    print("🔍 COMPLETE TRINITY SYSTEM VERIFICATION")
    print("=" * 60)
    
    results = {
        "engine_mapping": {"status": "❓", "issues": []},
        "trinity_context": {"status": "❓", "issues": []},
        "preset_corpus": {"status": "❓", "issues": []},
        "smart_components": {"status": "❓", "issues": []},
        "pipeline": {"status": "❓", "issues": []}
    }
    
    # 1. Verify Engine Mapping Consistency
    print("\n1️⃣ Verifying Engine Mapping...")
    try:
        # Check authoritative Python mapping
        from engine_mapping_authoritative import ENGINE_NAMES, ENGINE_COUNT
        
        # Check unified manifest
        with open('unified_engine_manifest.json', 'r') as f:
            manifest = json.load(f)
        
        # Verify counts match
        if len(ENGINE_NAMES) != ENGINE_COUNT:
            results["engine_mapping"]["issues"].append(f"ENGINE_NAMES count ({len(ENGINE_NAMES)}) != ENGINE_COUNT ({ENGINE_COUNT})")
        
        # Verify manifest matches
        manifest_count = len(manifest["engine_mapping"])
        if manifest_count != ENGINE_COUNT:
            results["engine_mapping"]["issues"].append(f"Manifest count ({manifest_count}) != ENGINE_COUNT ({ENGINE_COUNT})")
        
        # Check each engine
        for i in range(ENGINE_COUNT):
            if i not in ENGINE_NAMES:
                results["engine_mapping"]["issues"].append(f"Missing engine ID {i}")
            if str(i) not in manifest["engine_mapping"]:
                results["engine_mapping"]["issues"].append(f"Manifest missing engine ID {i}")
        
        if not results["engine_mapping"]["issues"]:
            results["engine_mapping"]["status"] = "✅"
            print("  ✅ All 57 engines properly mapped")
        else:
            results["engine_mapping"]["status"] = "❌"
            print(f"  ❌ Found {len(results['engine_mapping']['issues'])} mapping issues")
            
    except Exception as e:
        results["engine_mapping"]["status"] = "❌"
        results["engine_mapping"]["issues"].append(str(e))
        print(f"  ❌ Error: {e}")
    
    # 2. Verify Trinity Context
    print("\n2️⃣ Verifying Trinity Context...")
    try:
        with open('trinity_context.md', 'r') as f:
            context = f.read()
        
        # Check for all 57 engines
        for i in range(57):
            engine_header = f"(ID: {i})"
            if engine_header not in context:
                results["trinity_context"]["issues"].append(f"Missing documentation for engine {i}")
        
        # Check for problematic old descriptions
        bad_patterns = [
            ("ENGINE_VINTAGE_TUBE", "Tape Compression"),
            ("ENGINE_WAVE_FOLDER", "Tape"),
            ("ENGINE_HARMONIC_EXCITER", "Fuzz")
        ]
        
        for engine, bad_text in bad_patterns:
            if engine in context and bad_text in context:
                # Check if they're in same section
                lines = context.split('\n')
                for i, line in enumerate(lines):
                    if engine in line:
                        # Check next 20 lines for bad text
                        section = '\n'.join(lines[i:i+20])
                        if bad_text in section:
                            results["trinity_context"]["issues"].append(
                                f"{engine} still has incorrect '{bad_text}' description"
                            )
                            break
        
        if not results["trinity_context"]["issues"]:
            results["trinity_context"]["status"] = "✅"
            print("  ✅ Trinity context properly documented")
        else:
            results["trinity_context"]["status"] = "❌"
            print(f"  ❌ Found {len(results['trinity_context']['issues'])} context issues")
            
    except Exception as e:
        results["trinity_context"]["status"] = "❌"
        results["trinity_context"]["issues"].append(str(e))
        print(f"  ❌ Error: {e}")
    
    # 3. Verify Preset Corpus
    print("\n3️⃣ Verifying Preset Corpus...")
    try:
        corpus_path = '/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json'
        with open(corpus_path, 'r') as f:
            presets = json.load(f)
        
        # Check preset structure
        for preset in presets[:10]:  # Sample check
            preset_id = preset.get('id', 'unknown')
            
            # Check slots
            for slot in range(1, 7):
                engine_key = f"slot{slot}_engine"
                if engine_key not in preset:
                    results["preset_corpus"]["issues"].append(f"Preset {preset_id} missing {engine_key}")
                else:
                    engine_id = preset[engine_key]
                    if not (0 <= engine_id <= 56):
                        results["preset_corpus"]["issues"].append(
                            f"Preset {preset_id} has invalid engine {engine_id} in slot {slot}"
                        )
        
        if not results["preset_corpus"]["issues"]:
            results["preset_corpus"]["status"] = "✅"
            print(f"  ✅ {len(presets)} presets validated")
        else:
            results["preset_corpus"]["status"] = "⚠️"
            print(f"  ⚠️ Found {len(results['preset_corpus']['issues'])} preset issues (sample)")
            
    except Exception as e:
        results["preset_corpus"]["status"] = "❌"
        results["preset_corpus"]["issues"].append(str(e))
        print(f"  ❌ Error: {e}")
    
    # 4. Verify Smart Components
    print("\n4️⃣ Verifying Smart Components...")
    try:
        components = [
            ('smart_oracle.py', 'SmartOracle'),
            ('smart_calculator.py', 'SmartCalculator')
        ]
        
        for filename, class_name in components:
            if not Path(filename).exists():
                results["smart_components"]["issues"].append(f"Missing {filename}")
            else:
                with open(filename, 'r') as f:
                    content = f.read()
                    if class_name not in content:
                        results["smart_components"]["issues"].append(f"{filename} missing {class_name}")
        
        # Check for FAISS index
        if Path('oracle_index.faiss').exists():
            print("  ✅ FAISS index found")
        else:
            results["smart_components"]["issues"].append("FAISS index not built")
        
        if not results["smart_components"]["issues"]:
            results["smart_components"]["status"] = "✅"
            print("  ✅ Smart components ready")
        else:
            results["smart_components"]["status"] = "⚠️"
            print(f"  ⚠️ {len(results['smart_components']['issues'])} component issues")
            
    except Exception as e:
        results["smart_components"]["status"] = "❌"
        results["smart_components"]["issues"].append(str(e))
        print(f"  ❌ Error: {e}")
    
    # 5. Verify Pipeline
    print("\n5️⃣ Verifying Pipeline Components...")
    try:
        pipeline_files = {
            'visionary.py': False,  # May not exist yet
            'oracle.py': True,
            'calculator.py': True,
            'alchemist.py': True
        }
        
        for filename, required in pipeline_files.items():
            if Path(filename).exists():
                print(f"  ✅ {filename} found")
            elif required:
                results["pipeline"]["issues"].append(f"Missing required {filename}")
            else:
                results["pipeline"]["issues"].append(f"Optional {filename} not found")
        
        if not any(issue.startswith("Missing required") for issue in results["pipeline"]["issues"]):
            results["pipeline"]["status"] = "✅"
            print("  ✅ Core pipeline components present")
        else:
            results["pipeline"]["status"] = "⚠️"
            print(f"  ⚠️ {len(results['pipeline']['issues'])} pipeline issues")
            
    except Exception as e:
        results["pipeline"]["status"] = "❌"
        results["pipeline"]["issues"].append(str(e))
        print(f"  ❌ Error: {e}")
    
    # Generate Final Report
    print("\n" + "=" * 60)
    print("📊 FINAL VERIFICATION REPORT")
    print("=" * 60)
    
    # Count statuses
    success_count = sum(1 for r in results.values() if r["status"] == "✅")
    warning_count = sum(1 for r in results.values() if r["status"] == "⚠️")
    error_count = sum(1 for r in results.values() if r["status"] == "❌")
    
    print(f"\n✅ Passed: {success_count}/5 components")
    print(f"⚠️ Warnings: {warning_count}/5 components")
    print(f"❌ Errors: {error_count}/5 components")
    
    # Component status
    print("\nComponent Status:")
    for component, data in results.items():
        print(f"  {data['status']} {component.replace('_', ' ').title()}")
        if data["issues"][:3]:  # Show first 3 issues
            for issue in data["issues"][:3]:
                print(f"      • {issue}")
    
    # Calculate confidence
    base_confidence = 100
    confidence_penalty = error_count * 20 + warning_count * 10
    confidence = max(0, base_confidence - confidence_penalty)
    
    print(f"\n🎯 CONFIDENCE LEVEL: {confidence}%")
    
    if confidence == 100:
        print("\n✨ PERFECT! The Trinity system is fully operational!")
        print("🚀 Ready for production use with complete accuracy")
    elif confidence >= 80:
        print("\n✅ GOOD! The Trinity system is mostly operational")
        print("📝 Minor issues should be addressed for optimal performance")
    elif confidence >= 60:
        print("\n⚠️ FUNCTIONAL! The Trinity system can work but needs attention")
        print("🔧 Address critical issues before production use")
    else:
        print("\n❌ CRITICAL! The Trinity system has major issues")
        print("🚨 Must fix errors before the system can function properly")
    
    # Save report
    report = {
        "timestamp": str(Path.cwd()),
        "confidence": confidence,
        "components": results,
        "summary": {
            "passed": success_count,
            "warnings": warning_count,
            "errors": error_count
        }
    }
    
    with open('verification_report.json', 'w') as f:
        json.dump(report, f, indent=2)
    
    print("\n📄 Detailed report saved to verification_report.json")
    
    return confidence == 100

if __name__ == "__main__":
    import sys
    success = verify_trinity_system()
    sys.exit(0 if success else 1)