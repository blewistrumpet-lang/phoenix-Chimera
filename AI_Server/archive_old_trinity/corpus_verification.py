#!/usr/bin/env python3
"""
Golden Corpus Verification Script
Validates the FAISS corpus integration and reports corpus statistics.
"""

import json
import logging
import numpy as np
import faiss
import pickle
from pathlib import Path
from typing import Dict, Any, List
from collections import defaultdict, Counter

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class CorpusVerifier:
    """Verifies the Golden Corpus and FAISS index integrity"""
    
    def __init__(self):
        """Initialize verifier with all possible corpus paths"""
        self.base_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix")
        self.corpus_paths = {
            "legacy": self.base_path / "Golden_Corpus" / "golden_corpus.json",
            "juce_v1": self.base_path / "JUCE_Plugin" / "GoldenCorpus" / "all_presets.json",
            "juce_v3": self.base_path / "JUCE_Plugin" / "GoldenCorpus_v3" / "faiss_index" / "presets.json"
        }
        
        self.faiss_paths = {
            "juce_v1": {
                "index": self.base_path / "JUCE_Plugin" / "GoldenCorpus" / "faiss_index" / "corpus.index",
                "meta": self.base_path / "JUCE_Plugin" / "GoldenCorpus" / "faiss_index" / "corpus_meta.pkl"
            },
            "juce_v3": {
                "index": self.base_path / "JUCE_Plugin" / "GoldenCorpus_v3" / "faiss_index" / "corpus.index",
                "meta": self.base_path / "JUCE_Plugin" / "GoldenCorpus_v3" / "faiss_index" / "metadata.json"
            }
        }
        
        self.report = {
            "corpus_files": {},
            "faiss_indices": {},
            "validation_errors": [],
            "statistics": {},
            "recommendations": []
        }
    
    def verify_all(self) -> Dict[str, Any]:
        """Run complete corpus verification"""
        logger.info("Starting comprehensive corpus verification...")
        
        # Check all corpus files
        self._verify_corpus_files()
        
        # Check FAISS indices
        self._verify_faiss_indices()
        
        # Generate statistics
        self._generate_statistics()
        
        # Generate recommendations
        self._generate_recommendations()
        
        return self.report
    
    def _verify_corpus_files(self):
        """Verify all corpus JSON files"""
        logger.info("Verifying corpus files...")
        
        for name, path in self.corpus_paths.items():
            logger.info(f"Checking {name} corpus at {path}")
            
            if not path.exists():
                self.report["corpus_files"][name] = {
                    "status": "MISSING",
                    "path": str(path),
                    "error": "File does not exist"
                }
                continue
            
            try:
                with open(path, 'r') as f:
                    data = json.load(f)
                
                # Determine structure
                if isinstance(data, list):
                    presets = data
                elif isinstance(data, dict) and 'presets' in data:
                    presets = data['presets']
                else:
                    presets = [data] if isinstance(data, dict) else []
                
                # Validate each preset
                valid_presets = 0
                invalid_presets = 0
                engine_ids_used = set()
                param_ranges = {"min": {}, "max": {}}
                
                for i, preset in enumerate(presets):
                    try:
                        self._validate_preset(preset, name, i)
                        valid_presets += 1
                        
                        # Collect engine IDs
                        if name == "juce_v3":
                            # New format with slot parameters
                            for slot in range(1, 7):
                                engine_id = preset.get(f"slot{slot}_engine")
                                if engine_id and engine_id != 0:
                                    engine_ids_used.add(engine_id)
                        else:
                            # Old format with engines array
                            for engine in preset.get("engines", []):
                                engine_id = engine.get("type")
                                if engine_id and engine_id != 0:
                                    engine_ids_used.add(engine_id)
                        
                        # Collect parameter ranges
                        self._collect_param_ranges(preset, param_ranges)
                        
                    except Exception as e:
                        invalid_presets += 1
                        self.report["validation_errors"].append(f"{name}[{i}]: {str(e)}")
                
                self.report["corpus_files"][name] = {
                    "status": "OK" if invalid_presets == 0 else "WARNING",
                    "path": str(path),
                    "total_presets": len(presets),
                    "valid_presets": valid_presets,
                    "invalid_presets": invalid_presets,
                    "unique_engines": len(engine_ids_used),
                    "engine_ids_used": sorted(list(engine_ids_used)),
                    "param_ranges": param_ranges
                }
                
            except Exception as e:
                self.report["corpus_files"][name] = {
                    "status": "ERROR",
                    "path": str(path),
                    "error": str(e)
                }
    
    def _validate_preset(self, preset: Dict[str, Any], corpus_name: str, index: int):
        """Validate individual preset"""
        if not isinstance(preset, dict):
            raise ValueError("Preset must be a dictionary")
        
        # Check required fields
        required_fields = ["name"] if corpus_name == "legacy" else []
        for field in required_fields:
            if field not in preset:
                raise ValueError(f"Missing required field: {field}")
        
        # Validate parameters based on format
        if corpus_name == "juce_v3":
            # New format validation
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine")
                if engine_id is not None and not (0 <= engine_id <= 56):
                    raise ValueError(f"Invalid engine ID {engine_id} in slot {slot}")
                
                bypass = preset.get(f"slot{slot}_bypass")
                if bypass is not None and not (0.0 <= bypass <= 1.0):
                    raise ValueError(f"Bypass parameter {bypass} out of range in slot {slot}")
                
                # Check all 15 parameters per slot
                for param in range(1, 16):
                    value = preset.get(f"slot{slot}_param{param}")
                    if value is not None and not (0.0 <= value <= 1.0):
                        raise ValueError(f"Parameter slot{slot}_param{param} = {value} out of range [0.0, 1.0]")
        
        elif "engines" in preset:
            # Old format validation
            for engine_idx, engine in enumerate(preset["engines"]):
                engine_id = engine.get("type")
                if engine_id is not None and not (0 <= engine_id <= 56):
                    raise ValueError(f"Invalid engine ID {engine_id} in engine {engine_idx}")
                
                mix = engine.get("mix")
                if mix is not None and not (0.0 <= mix <= 1.0):
                    raise ValueError(f"Mix parameter {mix} out of range in engine {engine_idx}")
                
                # Check params array
                for param_idx, value in enumerate(engine.get("params", [])):
                    if not (0.0 <= value <= 1.0):
                        raise ValueError(f"Parameter {param_idx} = {value} out of range [0.0, 1.0] in engine {engine_idx}")
    
    def _collect_param_ranges(self, preset: Dict[str, Any], ranges: Dict):
        """Collect parameter min/max ranges for analysis"""
        for key, value in preset.items():
            if isinstance(value, (int, float)) and "param" in key:
                if key not in ranges["min"] or value < ranges["min"][key]:
                    ranges["min"][key] = value
                if key not in ranges["max"] or value > ranges["max"][key]:
                    ranges["max"][key] = value
    
    def _verify_faiss_indices(self):
        """Verify FAISS indices"""
        logger.info("Verifying FAISS indices...")
        
        for name, paths in self.faiss_paths.items():
            logger.info(f"Checking FAISS index for {name}")
            
            index_path = paths["index"]
            meta_path = paths["meta"]
            
            result = {
                "index_path": str(index_path),
                "meta_path": str(meta_path),
                "status": "ERROR"
            }
            
            # Check index file
            if not index_path.exists():
                result["error"] = "Index file missing"
                self.report["faiss_indices"][name] = result
                continue
            
            try:
                # Load FAISS index
                index = faiss.read_index(str(index_path))
                result["vector_count"] = index.ntotal
                result["vector_dimension"] = index.d
                
                # Load metadata
                if meta_path.exists():
                    if meta_path.suffix == '.pkl':
                        with open(meta_path, 'rb') as f:
                            metadata = pickle.load(f)
                    else:  # JSON
                        with open(meta_path, 'r') as f:
                            metadata = json.load(f)
                    
                    result["metadata_count"] = len(metadata) if isinstance(metadata, list) else 1
                    
                    # Verify vector count matches metadata
                    if index.ntotal != len(metadata) if isinstance(metadata, list) else 1:
                        result["warning"] = f"Vector count ({index.ntotal}) != metadata count ({len(metadata)})"
                    
                    result["status"] = "OK"
                else:
                    result["warning"] = "Metadata file missing"
                    result["status"] = "WARNING"
                
            except Exception as e:
                result["error"] = str(e)
            
            self.report["faiss_indices"][name] = result
    
    def _generate_statistics(self):
        """Generate overall statistics"""
        stats = {
            "total_corpus_files": len(self.corpus_paths),
            "working_corpus_files": 0,
            "total_presets": 0,
            "total_faiss_indices": len(self.faiss_paths),
            "working_faiss_indices": 0,
            "unique_engines_across_all": set()
        }
        
        for name, info in self.report["corpus_files"].items():
            if info["status"] in ["OK", "WARNING"]:
                stats["working_corpus_files"] += 1
                stats["total_presets"] += info.get("total_presets", 0)
                stats["unique_engines_across_all"].update(info.get("engine_ids_used", []))
        
        for name, info in self.report["faiss_indices"].items():
            if info["status"] in ["OK", "WARNING"]:
                stats["working_faiss_indices"] += 1
        
        stats["unique_engines_across_all"] = len(stats["unique_engines_across_all"])
        self.report["statistics"] = stats
    
    def _generate_recommendations(self):
        """Generate recommendations based on verification results"""
        recommendations = []
        
        # Check for best corpus version
        best_corpus = None
        max_presets = 0
        
        for name, info in self.report["corpus_files"].items():
            if info["status"] in ["OK", "WARNING"] and info.get("total_presets", 0) > max_presets:
                max_presets = info["total_presets"]
                best_corpus = name
        
        if best_corpus:
            recommendations.append(f"Use '{best_corpus}' corpus - it has the most presets ({max_presets})")
            
            # Recommend correct Oracle paths
            if best_corpus == "juce_v3":
                recommendations.append("Update Oracle paths to use GoldenCorpus_v3:")
                recommendations.append("  - presets: ../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets.json")
                recommendations.append("  - index: ../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus.index")
                recommendations.append("  - metadata: ../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata.json")
        
        # Check for missing FAISS indices
        for name, info in self.report["faiss_indices"].items():
            if info["status"] == "ERROR":
                recommendations.append(f"Rebuild FAISS index for {name}")
        
        # Check for validation errors
        if self.report["validation_errors"]:
            recommendations.append(f"Fix {len(self.report['validation_errors'])} validation errors")
        
        # Check engine ID coverage
        total_engines = self.report["statistics"]["unique_engines_across_all"]
        if total_engines < 50:  # Chimera has ~57 engines
            recommendations.append(f"Corpus only covers {total_engines} engines - consider expanding")
        
        self.report["recommendations"] = recommendations
    
    def print_report(self):
        """Print verification report to console"""
        print("\n" + "="*80)
        print("GOLDEN CORPUS VERIFICATION REPORT")
        print("="*80)
        
        # Corpus Files
        print("\n📂 CORPUS FILES:")
        for name, info in self.report["corpus_files"].items():
            status_icon = "✅" if info["status"] == "OK" else "⚠️" if info["status"] == "WARNING" else "❌"
            print(f"  {status_icon} {name.upper()}: {info['status']}")
            if info["status"] in ["OK", "WARNING"]:
                print(f"     • Path: {info['path']}")
                print(f"     • Presets: {info['total_presets']} ({info['valid_presets']} valid)")
                print(f"     • Engines: {info['unique_engines']} different engine IDs")
            elif "error" in info:
                print(f"     • Error: {info['error']}")
        
        # FAISS Indices
        print("\n🔍 FAISS INDICES:")
        for name, info in self.report["faiss_indices"].items():
            status_icon = "✅" if info["status"] == "OK" else "⚠️" if info["status"] == "WARNING" else "❌"
            print(f"  {status_icon} {name.upper()}: {info['status']}")
            if "vector_count" in info:
                print(f"     • Vectors: {info['vector_count']} x {info['vector_dimension']}D")
            if "warning" in info:
                print(f"     • Warning: {info['warning']}")
            elif "error" in info:
                print(f"     • Error: {info['error']}")
        
        # Statistics
        print(f"\n📊 STATISTICS:")
        stats = self.report["statistics"]
        print(f"  • Working corpus files: {stats['working_corpus_files']}/{stats['total_corpus_files']}")
        print(f"  • Total presets: {stats['total_presets']}")
        print(f"  • Working FAISS indices: {stats['working_faiss_indices']}/{stats['total_faiss_indices']}")
        print(f"  • Unique engines covered: {stats['unique_engines_across_all']}")
        
        # Validation Errors
        if self.report["validation_errors"]:
            print(f"\n❌ VALIDATION ERRORS ({len(self.report['validation_errors'])}):")
            for error in self.report["validation_errors"][:10]:  # Show first 10
                print(f"  • {error}")
            if len(self.report["validation_errors"]) > 10:
                print(f"  ... and {len(self.report['validation_errors']) - 10} more")
        
        # Recommendations
        if self.report["recommendations"]:
            print(f"\n💡 RECOMMENDATIONS:")
            for rec in self.report["recommendations"]:
                print(f"  • {rec}")
        
        print("\n" + "="*80)
    
    def save_report(self, output_path: str = None):
        """Save report as JSON"""
        if output_path is None:
            output_path = "corpus_verification_report.json"
        
        # Convert sets to lists for JSON serialization
        report_copy = json.loads(json.dumps(self.report, default=str))
        
        with open(output_path, 'w') as f:
            json.dump(report_copy, f, indent=2)
        
        logger.info(f"Report saved to {output_path}")


def main():
    """Run corpus verification"""
    verifier = CorpusVerifier()
    
    try:
        report = verifier.verify_all()
        verifier.print_report()
        verifier.save_report()
        
        # Return exit code based on results
        has_working_corpus = any(
            info["status"] in ["OK", "WARNING"] 
            for info in report["corpus_files"].values()
        )
        
        if not has_working_corpus:
            logger.error("No working corpus found!")
            return 1
        
        logger.info("Corpus verification completed successfully")
        return 0
        
    except Exception as e:
        logger.error(f"Verification failed: {e}")
        return 1


if __name__ == "__main__":
    exit(main())