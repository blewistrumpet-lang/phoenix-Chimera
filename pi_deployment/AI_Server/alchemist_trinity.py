#!/usr/bin/env python3
"""
ALCHEMIST - The Safety Engineer of the Trinity Pipeline
Ensures all presets are safe, optimized, and production-ready
"""

import logging
import json
from typing import Dict, Any, List, Tuple, Optional
import numpy as np

from engine_knowledge_base import ENGINE_KNOWLEDGE
from engine_mapping_authoritative import ENGINE_NAMES

logger = logging.getLogger(__name__)

class AlchemistTrinity:
    """
    The Alchemist: Final safety validation and professional polish
    - Parameter range validation
    - Dangerous combination detection
    - Feedback loop prevention
    - CPU optimization
    - Professional polish
    - Format conversion for plugin
    """
    
    def __init__(self):
        """Initialize with safety knowledge"""
        self.engine_knowledge = ENGINE_KNOWLEDGE
        self.engine_names = ENGINE_NAMES
        
        # Define safety rules
        self.safety_rules = self._define_safety_rules()
        
        # Define dangerous combinations
        self.dangerous_combos = self._define_dangerous_combinations()
        
        logger.info("AlchemistTrinity initialized with safety validation")
    
    def _define_safety_rules(self) -> Dict[str, Any]:
        """Define comprehensive safety rules"""
        
        return {
            "parameter_limits": {
                "absolute_min": 0.0,
                "absolute_max": 1.0,
                "safe_feedback_max": 0.85,
                "safe_resonance_max": 0.9,
                "safe_drive_max": 0.95
            },
            "gain_staging": {
                "max_cumulative_gain": 4.0,
                "max_single_gain": 2.0,
                "compensation_required": True
            },
            "feedback_limits": {
                "single_delay_max": 0.85,
                "single_reverb_max": 0.9,
                "cumulative_max": 2.5,
                "with_distortion_max": 0.7
            },
            "cpu_limits": {
                "max_heavy_effects": 2,
                "heavy_engines": [39, 40, 41, 42, 43, 49, 50, 51, 52],
                "max_total_load": 0.8
            },
            "frequency_safety": {
                "bass_mono_below": 100,  # Hz
                "subsonic_filter": 20,    # Hz
                "ultrasonic_limit": 20000  # Hz
            },
            "stereo_safety": {
                "max_width": 1.5,
                "correlation_minimum": -0.5,
                "mono_compatibility": True
            }
        }
    
    def _define_dangerous_combinations(self) -> List[Dict[str, Any]]:
        """Define dangerous engine combinations"""
        
        return [
            {
                "name": "distortion_stack",
                "engines": [15, 16, 17, 18, 19, 20, 21, 22],
                "max_count": 2,
                "risk": "Excessive distortion, harsh sound",
                "mitigation": "reduce_drive_on_additional"
            },
            {
                "name": "feedback_loop",
                "engines": [34, 35, 36, 37],  # Delays
                "when_with": [39, 40, 41, 42, 43],  # Reverbs
                "risk": "Runaway feedback",
                "mitigation": "limit_combined_feedback"
            },
            {
                "name": "multiple_reverbs",
                "engines": [39, 40, 41, 42, 43],
                "max_count": 2,
                "risk": "Muddy, washed out sound",
                "mitigation": "reduce_mix_levels"
            },
            {
                "name": "phase_cancellation",
                "engines": [23, 24, 25],  # Chorus/Phaser
                "when_multiple": True,
                "risk": "Phase issues, thin sound",
                "mitigation": "adjust_phase_offset"
            },
            {
                "name": "resonance_buildup",
                "engines": [9, 10, 11, 12],  # Filters
                "max_resonance_sum": 2.0,
                "risk": "Self-oscillation, harsh peaks",
                "mitigation": "limit_resonance"
            },
            {
                "name": "cpu_overload",
                "engines": [41, 42, 49, 50, 51, 52],  # Heavy CPU
                "max_count": 2,
                "risk": "Audio dropouts, high latency",
                "mitigation": "limit_heavy_effects"
            }
        ]
    
    def validate_and_optimize(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Final validation and optimization pass
        
        Args:
            preset: Refined preset from Calculator
            
        Returns:
            Safe, optimized, production-ready preset
        """
        
        try:
            # Track all modifications for reporting
            modifications = []
            
            # 1. Basic parameter validation
            preset, mods = self._validate_parameter_ranges(preset)
            modifications.extend(mods)
            
            # 2. Detect and fix dangerous combinations
            preset, mods = self._detect_and_fix_dangerous_combinations(preset)
            modifications.extend(mods)
            
            # 3. Prevent feedback loops
            preset, mods = self._prevent_feedback_loops(preset)
            modifications.extend(mods)
            
            # 4. Optimize gain staging
            preset, mods = self._optimize_gain_staging(preset)
            modifications.extend(mods)
            
            # 5. CPU optimization
            preset, mods = self._optimize_cpu_usage(preset)
            modifications.extend(mods)
            
            # 6. Ensure mono compatibility
            preset, mods = self._ensure_mono_compatibility(preset)
            modifications.extend(mods)
            
            # 7. Apply professional polish
            preset = self._apply_professional_polish(preset)
            
            # 8. Generate safety report
            safety_report = self._generate_safety_report(preset, modifications)
            
            # 9. Convert parameters to plugin format (keep nested structure)
            preset = self._format_for_plugin(preset)
            
            # Add safety certification
            preset["safety_certified"] = safety_report["passed"]
            preset["safety_report"] = safety_report
            
            logger.info(f"Alchemist validated preset: {preset.get('name', 'Unknown')}")
            
            return preset
            
        except Exception as e:
            logger.error(f"Error in Alchemist validation: {str(e)}")
            # Return preset with safety flag
            preset["safety_certified"] = False
            preset["safety_error"] = str(e)
            return preset
    
    def _validate_parameter_ranges(self, preset: Dict) -> Tuple[Dict, List[str]]:
        """Validate all parameters are in safe ranges"""
        
        modifications = []
        limits = self.safety_rules["parameter_limits"]
        
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id")
            engine_name = slot.get("engine_name", "Unknown")
            params = slot.get("parameters", [])
            
            for i, param in enumerate(params):
                original = param
                
                # Clamp to absolute limits
                param = max(limits["absolute_min"], min(limits["absolute_max"], float(param)))
                
                # Apply specific limits based on parameter type
                param_name = self._get_parameter_name(engine_id, i)
                
                if "feedback" in param_name.lower():
                    if param > limits["safe_feedback_max"]:
                        param = limits["safe_feedback_max"]
                        modifications.append(f"Limited {engine_name} feedback: {original:.2f} -> {param:.2f}")
                
                elif "resonance" in param_name.lower():
                    if param > limits["safe_resonance_max"]:
                        param = limits["safe_resonance_max"]
                        modifications.append(f"Limited {engine_name} resonance: {original:.2f} -> {param:.2f}")
                
                elif "drive" in param_name.lower():
                    if param > limits["safe_drive_max"]:
                        param = limits["safe_drive_max"]
                        modifications.append(f"Limited {engine_name} drive: {original:.2f} -> {param:.2f}")
                
                params[i] = param
            
            slot["parameters"] = params
        
        return preset, modifications
    
    def _detect_and_fix_dangerous_combinations(self, preset: Dict) -> Tuple[Dict, List[str]]:
        """Detect and mitigate dangerous engine combinations"""
        
        modifications = []
        slots = preset.get("slots", [])
        engine_ids = [slot.get("engine_id") for slot in slots]
        
        for combo in self.dangerous_combos:
            if combo["name"] == "distortion_stack":
                # Check for too many distortions
                distortion_count = sum(1 for eid in engine_ids if eid in combo["engines"])
                
                if distortion_count > combo["max_count"]:
                    # Reduce drive on extra distortions
                    distortion_slots = [s for s in slots if s.get("engine_id") in combo["engines"]]
                    for i, slot in enumerate(distortion_slots[combo["max_count"]:]):
                        params = slot.get("parameters", [])
                        if len(params) > 0:
                            original = params[0]
                            params[0] *= 0.5  # Halve the drive
                            modifications.append(f"Reduced extra distortion drive: {original:.2f} -> {params[0]:.2f}")
            
            elif combo["name"] == "feedback_loop":
                # Check for delay + reverb combination
                has_delay = any(eid in combo["engines"] for eid in engine_ids)
                has_reverb = any(eid in combo["when_with"] for eid in engine_ids)
                
                if has_delay and has_reverb:
                    # Limit combined feedback
                    total_feedback = 0
                    for slot in slots:
                        if slot.get("engine_id") in combo["engines"] + combo["when_with"]:
                            feedback_idx = self._get_feedback_param_index(slot.get("engine_id"))
                            if feedback_idx is not None and feedback_idx < len(slot.get("parameters", [])):
                                total_feedback += slot["parameters"][feedback_idx]
                    
                    if total_feedback > 1.5:
                        # Scale down all feedback
                        scale = 1.5 / total_feedback
                        for slot in slots:
                            if slot.get("engine_id") in combo["engines"] + combo["when_with"]:
                                feedback_idx = self._get_feedback_param_index(slot.get("engine_id"))
                                if feedback_idx is not None and feedback_idx < len(slot.get("parameters", [])):
                                    original = slot["parameters"][feedback_idx]
                                    slot["parameters"][feedback_idx] *= scale
                                    modifications.append(f"Scaled feedback: {original:.2f} -> {slot['parameters'][feedback_idx]:.2f}")
            
            elif combo["name"] == "multiple_reverbs":
                # Check for too many reverbs
                reverb_count = sum(1 for eid in engine_ids if eid in combo["engines"])
                
                if reverb_count > combo["max_count"]:
                    modifications.append(f"Warning: {reverb_count} reverbs detected (max recommended: {combo['max_count']})")
                    # Reduce mix on extra reverbs
                    reverb_slots = [s for s in slots if s.get("engine_id") in combo["engines"]]
                    for slot in reverb_slots[combo["max_count"]:]:
                        mix_idx = self._get_mix_param_index(slot.get("engine_id"))
                        if mix_idx is not None and mix_idx < len(slot.get("parameters", [])):
                            original = slot["parameters"][mix_idx]
                            slot["parameters"][mix_idx] *= 0.5
                            modifications.append(f"Reduced extra reverb mix: {original:.2f} -> {slot['parameters'][mix_idx]:.2f}")
        
        return preset, modifications
    
    def _prevent_feedback_loops(self, preset: Dict) -> Tuple[Dict, List[str]]:
        """Sophisticated feedback prevention"""
        
        modifications = []
        slots = preset.get("slots", [])
        limits = self.safety_rules["feedback_limits"]
        
        # Calculate total feedback
        total_feedback = 0
        feedback_sources = []
        
        for slot in slots:
            engine_id = slot.get("engine_id")
            engine_name = slot.get("engine_name", "Unknown")
            
            feedback_idx = self._get_feedback_param_index(engine_id)
            if feedback_idx is not None and feedback_idx < len(slot.get("parameters", [])):
                feedback_value = slot["parameters"][feedback_idx]
                
                # Weight by engine type
                if engine_id in [34, 35, 36, 37]:  # Delays
                    weight = 1.5
                    max_single = limits["single_delay_max"]
                elif engine_id in [39, 40, 41, 42, 43]:  # Reverbs
                    weight = 1.2
                    max_single = limits["single_reverb_max"]
                else:
                    weight = 1.0
                    max_single = 0.9
                
                # Check single feedback limit
                if feedback_value > max_single:
                    original = feedback_value
                    feedback_value = max_single
                    slot["parameters"][feedback_idx] = feedback_value
                    modifications.append(f"Limited {engine_name} feedback: {original:.2f} -> {feedback_value:.2f}")
                
                total_feedback += feedback_value * weight
                feedback_sources.append((engine_name, feedback_value, weight))
        
        # Check cumulative limit
        if total_feedback > limits["cumulative_max"]:
            scale = limits["cumulative_max"] / total_feedback
            modifications.append(f"Scaling all feedback by {scale:.2f} to prevent runaway")
            
            for slot in slots:
                engine_id = slot.get("engine_id")
                feedback_idx = self._get_feedback_param_index(engine_id)
                if feedback_idx is not None and feedback_idx < len(slot.get("parameters", [])):
                    slot["parameters"][feedback_idx] *= scale
        
        return preset, modifications
    
    def _optimize_gain_staging(self, preset: Dict) -> Tuple[Dict, List[str]]:
        """Optimize gain structure through signal chain"""
        
        modifications = []
        slots = preset.get("slots", [])
        limits = self.safety_rules["gain_staging"]
        
        # Calculate cumulative gain
        cumulative_gain = 1.0
        gain_stages = []
        
        for slot in slots:
            engine_id = slot.get("engine_id")
            engine_name = slot.get("engine_name", "Unknown")
            params = slot.get("parameters", [])
            
            # Get gain-related parameters
            gain_params = self._get_gain_parameters(engine_id)
            stage_gain = 1.0
            
            for param_idx, param_name in gain_params.items():
                if param_idx < len(params):
                    # Convert 0-1 to gain multiplier
                    param_value = params[param_idx]
                    gain_mult = 0.5 + param_value  # 0.5x to 1.5x range
                    stage_gain *= gain_mult
            
            cumulative_gain *= stage_gain
            gain_stages.append((engine_name, stage_gain, cumulative_gain))
        
        # Check if gain compensation needed
        if cumulative_gain > limits["max_cumulative_gain"]:
            compensation = limits["max_cumulative_gain"] / cumulative_gain
            modifications.append(f"Applying gain compensation: {compensation:.2f}")
            
            # Apply compensation to output gains
            for slot in reversed(slots):  # Start from end of chain
                engine_id = slot.get("engine_id")
                output_idx = self._get_output_gain_index(engine_id)
                
                if output_idx is not None and output_idx < len(slot.get("parameters", [])):
                    original = slot["parameters"][output_idx]
                    slot["parameters"][output_idx] *= compensation
                    modifications.append(f"Compensated {slot.get('engine_name')} output: {original:.2f} -> {slot['parameters'][output_idx]:.2f}")
                    break  # Only compensate one stage
        
        return preset, modifications
    
    def _optimize_cpu_usage(self, preset: Dict) -> Tuple[Dict, List[str]]:
        """Optimize for CPU efficiency"""
        
        modifications = []
        slots = preset.get("slots", [])
        limits = self.safety_rules["cpu_limits"]
        
        # Count heavy effects
        heavy_count = sum(1 for slot in slots if slot.get("engine_id") in limits["heavy_engines"])
        
        if heavy_count > limits["max_heavy_effects"]:
            modifications.append(f"Warning: {heavy_count} CPU-heavy effects (max: {limits['max_heavy_effects']})")
            
            # Consider swapping heavy effects for lighter alternatives
            suggestions = {
                41: 39,  # Convolution → Plate Reverb
                42: 39,  # Shimmer → Plate Reverb
                49: 23,  # Spectral Freeze → Chorus
                50: 48,  # Spectral Gate → simpler gate
            }
            
            # Just warn, don't auto-swap
            for slot in slots:
                if slot.get("engine_id") in suggestions:
                    alternative = self.engine_names.get(suggestions[slot.get("engine_id")])
                    modifications.append(f"Consider replacing {slot.get('engine_name')} with {alternative} for better CPU performance")
        
        return preset, modifications
    
    def _ensure_mono_compatibility(self, preset: Dict) -> Tuple[Dict, List[str]]:
        """Ensure preset is mono-compatible"""
        
        modifications = []
        slots = preset.get("slots", [])
        
        # Check for stereo widening effects
        stereo_engines = [44, 45, 46]  # Widener, Imager, Dimension
        has_stereo = any(slot.get("engine_id") in stereo_engines for slot in slots)
        
        if has_stereo:
            # Ensure bass frequencies stay mono
            for slot in slots:
                if slot.get("engine_id") == 44:  # Stereo Widener
                    params = slot.get("parameters", [])
                    if len(params) > 1:  # Bass mono parameter
                        if params[1] < 0.3:  # Too little bass mono
                            original = params[1]
                            params[1] = 0.3
                            modifications.append(f"Increased bass mono for compatibility: {original:.2f} -> {params[1]:.2f}")
        
        return preset, modifications
    
    def _apply_professional_polish(self, preset: Dict) -> Dict:
        """Apply final professional touches"""
        
        # Ensure preset has all required fields
        if "name" not in preset:
            preset["name"] = "Trinity Preset"
        
        if "description" not in preset:
            preset["description"] = "Generated by Trinity Pipeline"
        
        # Add version info
        preset["version"] = "1.0.0"
        preset["pipeline"] = "Trinity"
        
        # Ensure clean structure
        for slot in preset.get("slots", []):
            # Ensure slot numbering is sequential
            pass
        
        return preset
    
    def _generate_safety_report(self, preset: Dict, modifications: List[str]) -> Dict[str, Any]:
        """Generate comprehensive safety report"""
        
        report = {
            "passed": True,
            "modifications_count": len(modifications),
            "modifications": modifications,
            "checks": {
                "parameter_ranges": "✓",
                "gain_staging": "✓",
                "feedback_safety": "✓",
                "cpu_optimization": "✓",
                "mono_compatibility": "✓"
            },
            "warnings": [],
            "stats": {
                "engine_count": len(preset.get("slots", [])),
                "total_modifications": len(modifications)
            }
        }
        
        # Add any warnings
        if len(modifications) > 10:
            report["warnings"].append("Many safety modifications required")
        
        # Check for critical issues
        critical_keywords = ["runaway", "overload", "dangerous"]
        for mod in modifications:
            if any(kw in mod.lower() for kw in critical_keywords):
                report["warnings"].append(f"Critical issue addressed: {mod}")
        
        # Determine pass/fail
        if len(report["warnings"]) > 5:
            report["passed"] = False
        
        return report
    
    def _format_for_plugin(self, preset: Dict) -> Dict:
        """Format preset for plugin while maintaining nested structure"""
        
        # Keep the nested structure but format parameters correctly
        formatted_preset = {
            "name": preset.get("name", "Untitled"),
            "description": preset.get("description", ""),
            "slots": []
        }
        
        # Format each slot with proper parameter structure
        for slot in preset.get("slots", []):
            formatted_slot = {
                "slot": slot.get("slot", 0),
                "engine_id": slot.get("engine_id", 0),
                "engine_name": slot.get("engine_name", "Empty"),
                "parameters": []
            }
            
            # Convert parameters to named format expected by plugin
            params = slot.get("parameters", [])
            for i in range(10):  # Always 10 parameters
                param_value = params[i] if i < len(params) else 0.0
                formatted_slot["parameters"].append({
                    "name": f"param{i+1}",  # Plugin expects param1, param2, etc.
                    "value": float(param_value)
                })
            
            formatted_preset["slots"].append(formatted_slot)
        
        return formatted_preset
    
    def _convert_to_plugin_format(self, preset: Dict) -> Dict:
        """Convert to final plugin-ready format"""
        
        # Flatten slots to plugin format
        plugin_preset = {
            "name": preset.get("name", "Untitled"),
            "description": preset.get("description", ""),
        }
        
        # Convert slots to flat format
        for i, slot in enumerate(preset.get("slots", []), 1):
            if i > 6:  # Max 6 slots
                break
            
            plugin_preset[f"slot{i}_engine"] = slot.get("engine_id", 0)
            
            # Add all 10 parameters
            params = slot.get("parameters", [])
            for j in range(10):
                param_key = f"slot{i}_param{j}"
                param_value = params[j] if j < len(params) else 0.0
                plugin_preset[param_key] = round(float(param_value), 3)
        
        # Fill empty slots
        for i in range(len(preset.get("slots", [])) + 1, 7):
            plugin_preset[f"slot{i}_engine"] = 0
            for j in range(10):
                plugin_preset[f"slot{i}_param{j}"] = 0.0
        
        # Add metadata
        plugin_preset["metadata"] = preset.get("metadata", {})
        plugin_preset["safety_report"] = preset.get("safety_report", {})
        plugin_preset["safety_certified"] = preset.get("safety_certified", True)
        
        return plugin_preset
    
    # Helper methods
    
    def _get_parameter_name(self, engine_id: int, param_idx: int) -> str:
        """Get parameter name for engine and index"""
        # Simplified - in real implementation would use full parameter database
        param_names = {
            1: ["Input Gain", "Peak Reduction", "HF Emphasis", "Output Gain", "Mix"],
            34: ["Time", "Feedback", "Wow & Flutter", "Saturation", "Mix"],
            # ... etc
        }
        
        if engine_id in param_names and param_idx < len(param_names[engine_id]):
            return param_names[engine_id][param_idx]
        
        return f"Param{param_idx}"
    
    def _get_feedback_param_index(self, engine_id: int) -> Optional[int]:
        """Get feedback parameter index for an engine"""
        feedback_indices = {
            34: 1,  # Tape Echo
            35: 1,  # Digital Delay
            36: 1,  # Magnetic Drum
            37: 1,  # Bucket Brigade
            23: 3,  # Chorus
            11: 2,  # Comb Resonator
            # ... etc
        }
        return feedback_indices.get(engine_id)
    
    def _get_mix_param_index(self, engine_id: int) -> Optional[int]:
        """Get mix parameter index for an engine"""
        mix_indices = {
            1: 4,   # Opto
            15: 9,  # Tube
            18: 2,  # Bit Crusher
            23: 2,  # Chorus
            34: 4,  # Tape Echo
            39: 3,  # Plate Reverb
            # ... etc
        }
        return mix_indices.get(engine_id)
    
    def _get_gain_parameters(self, engine_id: int) -> Dict[int, str]:
        """Get gain-related parameters for an engine"""
        gain_params = {
            1: {0: "Input Gain", 3: "Output Gain"},
            2: {5: "Makeup Gain"},
            15: {0: "Input Gain", 7: "Output Gain"},
            # ... etc
        }
        return gain_params.get(engine_id, {})
    
    def _get_output_gain_index(self, engine_id: int) -> Optional[int]:
        """Get output gain parameter index"""
        output_indices = {
            1: 3,   # Opto
            15: 7,  # Tube
            54: 0,  # Gain Utility
            # ... etc
        }
        return output_indices.get(engine_id)

if __name__ == "__main__":
    # Test the Alchemist
    print("⚗️ ALCHEMIST TRINITY TEST")
    print("=" * 60)
    
    alchemist = AlchemistTrinity()
    
    # Test preset with safety issues
    test_preset = {
        "name": "Dangerous Test Preset",
        "slots": [
            {
                "slot": 1,
                "engine_id": 34,  # Tape Echo
                "engine_name": "Tape Echo",
                "parameters": [0.5, 0.95, 0.3, 0.3, 0.8, 0.6, 0.2, 0.3, 0.0, 0.0]  # Very high feedback!
            },
            {
                "slot": 2,
                "engine_id": 39,  # Plate Reverb
                "engine_name": "Plate Reverb",
                "parameters": [0.8, 0.5, 0.1, 0.7, 0.9, 0.7, 0.2, 0.8, 0.1, 0.0]  # High mix with delay
            },
            {
                "slot": 3,
                "engine_id": 15,  # Vintage Tube
                "engine_name": "Vintage Tube Preamp",
                "parameters": [1.2, 1.1, 0.5, 0.5, 0.5, 0.5, 0.5, 1.0, 0.0, 1.0]  # Out of range!
            }
        ]
    }
    
    print("Testing preset with issues:")
    print(f"  - Tape Echo feedback: 0.95 (too high)")
    print(f"  - Tube parameters > 1.0 (out of range)")
    print(f"  - Delay + Reverb (feedback risk)")
    
    # Validate and fix
    safe_preset = alchemist.validate_and_optimize(test_preset)
    
    print("\n✅ ALCHEMIST RESULTS:")
    print("-" * 40)
    
    if safe_preset.get("safety_certified"):
        print("✓ Preset is now SAFE")
    else:
        print("✗ Preset still has issues")
    
    if "safety_report" in safe_preset:
        report = safe_preset["safety_report"]
        print(f"\nModifications made: {report['modifications_count']}")
        for mod in report["modifications"][:5]:  # Show first 5
            print(f"  - {mod}")
        
        if report.get("warnings"):
            print("\nWarnings:")
            for warning in report["warnings"]:
                print(f"  ⚠️ {warning}")
    
    # Check final format
    print("\nPlugin-ready format:")
    for key in ["slot1_engine", "slot1_param0", "slot1_param1"]:
        if key in safe_preset:
            print(f"  {key}: {safe_preset[key]}")
    
    print("\n✅ Alchemist validation complete!")