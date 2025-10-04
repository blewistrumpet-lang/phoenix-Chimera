#!/usr/bin/env python3
"""
Maximum Intelligence Calculator with Claude Integration
Uses Claude's deep musical knowledge for parameter optimization
"""

import json
import re
import logging
import asyncio
import hashlib
import os
from typing import Dict, List, Any, Optional, Tuple
from datetime import datetime
import numpy as np
from anthropic import AsyncAnthropic

logger = logging.getLogger(__name__)

class MaxIntelligenceCalculator:
    def __init__(self):
        """Initialize with complete engine knowledge and Claude client"""
        # Load the COMPLETE knowledge base
        try:
            with open("trinity_engine_knowledge_COMPLETE.json", "r") as f:
                self.knowledge = json.load(f)
            logger.info(f"✅ Max Intelligence Calculator loaded knowledge for {len(self.knowledge['engines'])} engines")
        except FileNotFoundError:
            logger.error("❌ Knowledge base not found!")
            raise
        
        self.engines = self.knowledge["engines"]
        
        # Initialize Claude client for maximum intelligence
        api_key = os.getenv("ANTHROPIC_API_KEY")
        if api_key:
            self.claude = AsyncAnthropic(api_key=api_key)
            logger.info("🧠 Claude client initialized for maximum intelligence")
        else:
            self.claude = None
            logger.warning("⚠️ No Anthropic API key - using basic intelligence only")
        
        # Build parameter mappings
        self.param_mappings = self._build_parameter_mappings()
        
        # Initialize caching system
        self.cache_file = "parameter_intelligence_cache.json"
        self.intelligence_cache = self._load_cache()
        
        # Musical time subdivisions
        self.time_subdivisions = {
            "whole": 1.0, "1": 1.0,
            "half": 0.5, "1/2": 0.5,
            "quarter": 0.25, "1/4": 0.25,
            "eighth": 0.125, "1/8": 0.125, "8th": 0.125,
            "sixteenth": 0.0625, "1/16": 0.0625, "16th": 0.0625,
            "dotted half": 0.75, "1/2 dotted": 0.75,
            "dotted quarter": 0.375, "1/4 dotted": 0.375,
            "dotted eighth": 0.1875, "1/8 dotted": 0.1875, "dotted 8th": 0.1875,
            "triplet quarter": 0.1667, "1/4 triplet": 0.1667,
            "triplet eighth": 0.0833, "1/8 triplet": 0.0833
        }
        
        # Parameter patterns for parsing
        self.param_patterns = {
            "percentage": re.compile(r'(\d+(?:\.\d+)?)\s*%\s*(\w+)?'),
            "ratio": re.compile(r'(\d+(?:\.\d+)?):(\d+(?:\.\d+)?)\s*(?:ratio)?'),
            "milliseconds": re.compile(r'(\d+(?:\.\d+)?)\s*ms'),
            "hertz": re.compile(r'(\d+(?:\.\d+)?)\s*[hH]z'),
            "kilohertz": re.compile(r'(\d+(?:\.\d+)?)\s*k[hH]z'),
            "decibels": re.compile(r'([+-]?\d+(?:\.\d+)?)\s*dB'),
        }
        
        logger.info(f"📊 Built mappings for {len(self.param_mappings)} engines")
    
    def _build_parameter_mappings(self) -> Dict[int, Dict]:
        """Build semantic parameter mappings for each engine"""
        mappings = {}
        
        for engine_id, engine_data in self.engines.items():
            engine_id_int = int(engine_id)
            mappings[engine_id_int] = {
                "name": engine_data.get("name", f"Engine {engine_id}"),
                "category": engine_data.get("category", ""),
                "params": {},
                "param_list": [],
                "param_details": []
            }
            
            # Build detailed parameter information
            for idx, param in enumerate(engine_data.get("parameters", [])):
                param_name_lower = param.get("name", "").lower()
                param_info = {
                    "index": idx,
                    "name": param.get("name", f"param{idx+1}"),
                    "default": param.get("default", 0.5),
                    "min": param.get("min", 0.0),
                    "max": param.get("max", 1.0),
                    "description": param.get("description", ""),
                    "units": param.get("units", ""),
                    "skew": param.get("skew", 0.5)
                }
                
                # Store by lowercase name
                mappings[engine_id_int]["params"][param_name_lower] = param_info
                mappings[engine_id_int]["param_list"].append(param_info)
                mappings[engine_id_int]["param_details"].append({
                    "index": idx,
                    "name": param.get("name"),
                    "description": param.get("description"),
                    "default": param.get("default"),
                    "range": f"{param.get('min', 0)}-{param.get('max', 1)}",
                    "units": param.get("units", "0-1")
                })
                
                # Map common aliases
                self._map_parameter_aliases(mappings[engine_id_int]["params"], param_name_lower, param_info)
        
        return mappings
    
    def _map_parameter_aliases(self, params_dict: Dict, param_name: str, param_info: Dict):
        """Map common parameter name aliases"""
        if "feedback" in param_name:
            params_dict["feedback"] = param_info
            params_dict["regen"] = param_info
            params_dict["regeneration"] = param_info
        elif "time" in param_name or "delay" in param_name:
            params_dict["delay"] = param_info
            params_dict["time"] = param_info
        elif "mix" in param_name:
            params_dict["mix"] = param_info
            params_dict["wet"] = param_info
            params_dict["dry/wet"] = param_info
            params_dict["blend"] = param_info
        elif "drive" in param_name or "gain" in param_name:
            params_dict["drive"] = param_info
            params_dict["gain"] = param_info
            params_dict["input"] = param_info
        elif "threshold" in param_name:
            params_dict["threshold"] = param_info
            params_dict["thresh"] = param_info
        elif "ratio" in param_name:
            params_dict["ratio"] = param_info
            params_dict["compression"] = param_info
    
    def _load_cache(self) -> Dict:
        """Load cached intelligence from previous Claude responses"""
        try:
            with open(self.cache_file, 'r') as f:
                cache = json.load(f)
                logger.info(f"📚 Loaded {len(cache.get('style_parameters', {}))} cached parameter sets")
                return cache
        except FileNotFoundError:
            logger.info("🆕 Starting fresh intelligence cache")
            return {
                "style_parameters": {},
                "relationships": {},
                "creative_enhancements": {},
                "statistics": {
                    "cache_hits": 0,
                    "claude_calls": 0,
                    "total_tokens": 0
                }
            }
    
    def _save_cache(self):
        """Save intelligence cache to disk"""
        with open(self.cache_file, 'w') as f:
            json.dump(self.intelligence_cache, f, indent=2)
        logger.info("💾 Saved intelligence cache")
    
    def parse_prompt_values(self, prompt: str) -> Dict[str, Any]:
        """Extract specific parameter values from user prompt"""
        extracted = {}
        prompt_lower = prompt.lower()
        
        # Check for time subdivisions
        for subdivision, value in self.time_subdivisions.items():
            if subdivision in prompt_lower:
                extracted["time_subdivision"] = {
                    "value": value,
                    "original": subdivision,
                    "type": "time"
                }
                logger.info(f"📐 Found time: {subdivision} = {value}")
        
        # Check for percentages
        for match in self.param_patterns["percentage"].finditer(prompt_lower):
            percent_val = float(match.group(1)) / 100.0
            param_hint = match.group(2) if match.group(2) else "mix"
            extracted[f"percentage_{param_hint}"] = {
                "value": percent_val,
                "original": f"{match.group(1)}%",
                "type": "percentage",
                "hint": param_hint
            }
            logger.info(f"📊 Found percentage: {match.group(1)}% {param_hint} = {percent_val}")
        
        # Check for ratios
        for match in self.param_patterns["ratio"].finditer(prompt_lower):
            numerator = float(match.group(1))
            denominator = float(match.group(2))
            ratio_val = numerator / denominator if denominator > 0 else numerator
            # Map to 0-1 range
            normalized = min(1.0, ratio_val / 20.0)  # Assume max ratio of 20:1
            
            extracted["ratio"] = {
                "value": normalized,
                "original": f"{numerator}:{denominator}",
                "actual_ratio": ratio_val,
                "type": "ratio"
            }
            logger.info(f"🔢 Found ratio: {numerator}:{denominator} = {normalized}")
        
        return extracted
    
    async def get_musical_style_parameters(self, prompt: str, engines: List[Dict]) -> Dict:
        """Ask Claude for deep musical understanding of style"""
        
        # Check cache first
        cache_key = hashlib.md5(f"{prompt}_{engines}".encode()).hexdigest()
        if cache_key in self.intelligence_cache.get("style_parameters", {}):
            logger.info("📚 Using cached style parameters")
            self.intelligence_cache["statistics"]["cache_hits"] += 1
            return self.intelligence_cache["style_parameters"][cache_key]
        
        if not self.claude:
            logger.warning("⚠️ No Claude client - using defaults")
            return {}
        
        # Build detailed engine information
        engine_details = ""
        for engine in engines:
            engine_id = engine.get("engine_id", 0)
            if engine_id == 0:
                continue
            
            engine_info = self.param_mappings.get(engine_id, {})
            engine_details += f"\n\n{engine_info.get('name', 'Unknown')} (ID: {engine_id}):\n"
            engine_details += f"Category: {engine_info.get('category', 'Unknown')}\n"
            engine_details += "Parameters:\n"
            
            for param_detail in engine_info.get("param_details", []):
                engine_details += f"  param{param_detail['index']+1}: {param_detail['name']} - {param_detail['description']}\n"
                engine_details += f"    Range: {param_detail['range']}, Default: {param_detail['default']}, Units: {param_detail['units']}\n"
        
        prompt_text = f"""You are a legendary audio engineer with expertise in every musical genre and production technique.

User request: "{prompt}"

Selected engines and their parameters:
{engine_details}

Provide EXACT parameter values (0.0-1.0) based on deep musical knowledge:

1. GENRE ANALYSIS
- What genre/style is this?
- What are the defining characteristics?
- What production techniques are typical?

2. HISTORICAL CONTEXT
- What equipment/techniques were originally used?
- What are the sonic signatures?
- Any specific references (albums, producers, eras)?

3. FREQUENCY PROFILE
- What frequencies should be emphasized/cut?
- Where are the key resonances?
- How should the spectrum be balanced?

4. DYNAMICS BEHAVIOR
- How much compression/limiting?
- What attack/release characteristics?
- Transient handling?

5. SPATIAL CHARACTERISTICS
- Room size and character?
- Depth and width?
- Wet/dry balance?

6. HARMONIC CONTENT
- Type of saturation/distortion?
- Odd vs even harmonics?
- How much color/warmth?

Return a JSON object with this exact structure:
{{
  "style_analysis": "Brief description of the musical style",
  "parameter_values": {{
    "engine_id": {{
      "param1": 0.5,  // Explain why this value
      "param2": 0.3,  // Explain why this value
      // ... for each parameter that should be set
    }}
  }},
  "key_characteristics": ["characteristic1", "characteristic2", ...],
  "reference_points": ["specific album or sound that inspired these settings"]
}}

Be extremely specific and musically accurate. These parameters will define the sound."""

        try:
            response = await self.claude.messages.create(
                model="claude-3-5-sonnet-20241022",
                max_tokens=2000,
                temperature=0.7,
                messages=[{"role": "user", "content": prompt_text}]
            )
            
            # Parse response
            response_text = response.content[0].text
            
            # Try to extract JSON from response
            json_match = re.search(r'\{.*\}', response_text, re.DOTALL)
            if json_match:
                try:
                    result = json.loads(json_match.group())
                    
                    # Cache the result
                    self.intelligence_cache["style_parameters"][cache_key] = result
                    self.intelligence_cache["statistics"]["claude_calls"] += 1
                    self.intelligence_cache["statistics"]["total_tokens"] += response.usage.total_tokens
                    self._save_cache()
                    
                    logger.info(f"🎨 Claude provided style parameters for: {result.get('style_analysis', 'Unknown')}")
                    return result
                except json.JSONDecodeError:
                    logger.warning("⚠️ Could not parse Claude's JSON response")
                    return {}
            else:
                logger.warning("⚠️ No JSON found in Claude's response")
                return {}
                
        except Exception as e:
            logger.error(f"❌ Claude API error: {str(e)}")
            return {}
    
    async def analyze_parameter_relationships(self, preset: Dict, prompt: str) -> Dict:
        """Ask Claude to analyze and fix parameter relationships"""
        
        if not self.claude:
            return {}
        
        # Build current state description
        current_state = "Current parameter settings:\n\n"
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            if engine_id == 0:
                continue
            
            engine_info = self.param_mappings.get(engine_id, {})
            current_state += f"{engine_info.get('name', 'Unknown')}:\n"
            
            for i, param in enumerate(slot.get("parameters", [])[:engine_info.get("param_count", 5)]):
                if i < len(engine_info.get("param_list", [])):
                    param_info = engine_info["param_list"][i]
                    current_state += f"  {param_info['name']}: {param.get('value', 0.5):.3f}\n"
        
        prompt_text = f"""Analyze these audio effect parameters for potential issues and relationships:

User's intent: "{prompt}"

{current_state}

Check for these issues and provide fixes:

1. FREQUENCY CONFLICTS
- Are multiple EQs/filters boosting the same frequencies?
- Will this cause resonance buildup?
- Are cuts and boosts fighting each other?

2. GAIN STAGING PROBLEMS
- Is the signal too hot/cold at any point?
- Will distortion stages clip badly?
- Are compressor thresholds appropriate for input levels?

3. TIMING CONFLICTS
- Are delay/reverb times creating rhythmic chaos?
- Do modulation rates clash?
- Will phase issues occur?

4. MIX BALANCE
- Is the cumulative wetness too high?
- Will the dry signal be lost?
- Are spatial effects balanced?

5. MUSICAL COHERENCE
- Do all settings support the intended style?
- Are any parameters contradictory?
- What subtle adjustments would improve cohesion?

Return specific parameter adjustments as JSON:
{{
  "issues_found": ["issue1", "issue2"],
  "adjustments": {{
    "engine_id": {{
      "param_index": new_value,
      "reason": "why this change helps"
    }}
  }},
  "improvements": ["what will be better after these changes"]
}}"""

        try:
            response = await self.claude.messages.create(
                model="claude-3-5-sonnet-20241022",
                max_tokens=1500,
                temperature=0.5,
                messages=[{"role": "user", "content": prompt_text}]
            )
            
            response_text = response.content[0].text
            json_match = re.search(r'\{.*\}', response_text, re.DOTALL)
            
            if json_match:
                try:
                    result = json.loads(json_match.group())
                    logger.info(f"🔧 Claude found {len(result.get('issues_found', []))} issues to fix")
                    return result
                except json.JSONDecodeError:
                    return {}
            
        except Exception as e:
            logger.error(f"❌ Claude relationship analysis error: {str(e)}")
            return {}
    
    async def add_creative_magic(self, preset: Dict, prompt: str) -> Dict:
        """Ask Claude to add creative enhancements"""
        
        if not self.claude or "basic" in prompt.lower() or "simple" in prompt.lower():
            return {}
        
        prompt_text = f"""You are a creative audio wizard known for adding "secret sauce" to presets.

Current preset for: "{prompt}"

Add 2-3 SUBTLE creative touches that users won't expect but will love:

1. MOVEMENT & LIFE
- What subtle modulation adds organic feel?
- Which parameter could have slight random variation?
- How to prevent static/lifeless sound?

2. HARMONIC MAGIC
- Where to add barely-perceptible saturation?
- What harmonics make it "expensive" sounding?
- How to add warmth without muddiness?

3. SPATIAL DEPTH
- What creates 3D depth without obvious reverb?
- How to add "air" around the sound?
- What makes it sit perfectly in a mix?

4. DYNAMIC INTEREST
- What creates subtle "bounce"?
- How to enhance groove/rhythm?
- What adds energy without loudness?

5. SIGNATURE CHARACTER
- What tiny detail makes this memorable?
- What's the "golden" touch?
- How to exceed expectations?

Return 2-3 specific, subtle adjustments:
{{
  "magic_touches": [
    {{
      "engine_id": X,
      "parameter": "param_name",
      "value": 0.X,
      "why": "what magic this adds"
    }}
  ],
  "overall_effect": "how these touches transform the sound"
}}

Be subtle - these should enhance, not dominate!"""

        try:
            response = await self.claude.messages.create(
                model="claude-3-5-sonnet-20241022",
                max_tokens=1000,
                temperature=0.8,
                messages=[{"role": "user", "content": prompt_text}]
            )
            
            response_text = response.content[0].text
            json_match = re.search(r'\{.*\}', response_text, re.DOTALL)
            
            if json_match:
                try:
                    result = json.loads(json_match.group())
                    logger.info(f"✨ Claude added {len(result.get('magic_touches', []))} creative touches")
                    return result
                except json.JSONDecodeError:
                    return {}
            
        except Exception as e:
            logger.error(f"❌ Claude creative enhancement error: {str(e)}")
            return {}
    
    async def optimize_parameters_max_intelligence(self, preset: Dict[str, Any], user_prompt: str) -> Dict[str, Any]:
        """
        Maximum intelligence parameter optimization using Claude
        """
        logger.info(f"🧠 MAX INTELLIGENCE optimization for: '{user_prompt}'")
        
        # STEP 1: Parse obvious values (instant, no tokens)
        extracted_values = self.parse_prompt_values(user_prompt)
        
        # Apply extracted values immediately
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            if engine_id == 0:
                continue
            
            self._apply_extracted_values_to_slot(slot, engine_id, extracted_values, user_prompt)
        
        # STEP 2: Get musical style parameters from Claude
        engines = [s for s in preset.get("slots", []) if s.get("engine_id", 0) != 0]
        style_params = await self.get_musical_style_parameters(user_prompt, engines)
        
        # Apply style parameters
        if style_params.get("parameter_values"):
            for slot in preset.get("slots", []):
                engine_id = slot.get("engine_id", 0)
                if str(engine_id) in style_params["parameter_values"]:
                    engine_params = style_params["parameter_values"][str(engine_id)]
                    for param_key, param_value in engine_params.items():
                        if param_key.startswith("param") and param_key[5:].isdigit():
                            param_idx = int(param_key[5:]) - 1
                            if param_idx < len(slot["parameters"]):
                                slot["parameters"][param_idx]["value"] = param_value
                                logger.info(f"  Set {param_key} = {param_value} (Claude style)")
        
        # STEP 3: Analyze and fix parameter relationships
        relationships = await self.analyze_parameter_relationships(preset, user_prompt)
        
        # Apply relationship fixes
        if relationships.get("adjustments"):
            for engine_id_str, adjustments in relationships["adjustments"].items():
                engine_id = int(engine_id_str)
                for slot in preset.get("slots", []):
                    if slot.get("engine_id") == engine_id:
                        for param_idx_str, new_value in adjustments.items():
                            if param_idx_str.startswith("param"):
                                param_idx = int(param_idx_str.replace("param", "")) - 1
                            else:
                                param_idx = int(param_idx_str)
                            
                            if param_idx < len(slot["parameters"]):
                                old_value = slot["parameters"][param_idx]["value"]
                                slot["parameters"][param_idx]["value"] = new_value
                                logger.info(f"  Relationship fix: param{param_idx+1} {old_value:.3f} → {new_value:.3f}")
        
        # STEP 4: Add creative magic (if appropriate)
        magic = await self.add_creative_magic(preset, user_prompt)
        
        # Apply creative touches
        if magic.get("magic_touches"):
            for touch in magic["magic_touches"]:
                engine_id = touch.get("engine_id")
                for slot in preset.get("slots", []):
                    if slot.get("engine_id") == engine_id:
                        param_name = touch.get("parameter", "")
                        new_value = touch.get("value", 0.5)
                        
                        # Find parameter index
                        engine_info = self.param_mappings.get(engine_id, {})
                        for i, param_info in enumerate(engine_info.get("param_list", [])):
                            if param_info["name"].lower() == param_name.lower():
                                if i < len(slot["parameters"]):
                                    slot["parameters"][i]["value"] = new_value
                                    logger.info(f"  ✨ Magic: {param_name} = {new_value} ({touch.get('why', '')})")
                                break
        
        # Log intelligence statistics
        stats = self.intelligence_cache.get("statistics", {})
        logger.info(f"📊 Intelligence stats: {stats.get('cache_hits', 0)} cache hits, "
                   f"{stats.get('claude_calls', 0)} Claude calls, "
                   f"{stats.get('total_tokens', 0)} total tokens")
        
        return preset
    
    def _apply_extracted_values_to_slot(self, slot: Dict, engine_id: int, extracted: Dict, prompt: str):
        """Apply extracted values to a specific slot"""
        engine_mapping = self.param_mappings.get(engine_id, {})
        params_map = engine_mapping.get("params", {})
        
        # Special handling for specific engines
        if engine_id == 34:  # Tape Echo
            if "time_subdivision" in extracted:
                slot["parameters"][0]["value"] = extracted["time_subdivision"]["value"]
                logger.info(f"    Set Time = {extracted['time_subdivision']['original']}")
            
            if "percentage_feedback" in extracted:
                slot["parameters"][1]["value"] = extracted["percentage_feedback"]["value"]
                logger.info(f"    Set Feedback = {extracted['percentage_feedback']['original']}")
        
        elif engine_id in [1, 2, 3, 4, 5]:  # Compressors
            if "ratio" in extracted and "ratio" in params_map:
                idx = params_map["ratio"]["index"]
                slot["parameters"][idx]["value"] = extracted["ratio"]["value"]
                logger.info(f"    Set Ratio = {extracted['ratio']['original']}")
        
        # Generic percentage application
        for key, value in extracted.items():
            if key.startswith("percentage_"):
                param_hint = key.replace("percentage_", "")
                if param_hint in params_map:
                    idx = params_map[param_hint]["index"]
                    slot["parameters"][idx]["value"] = value["value"]
                    logger.info(f"    Set {param_hint} = {value['original']}")


# Test the maximum intelligence calculator
if __name__ == "__main__":
    import asyncio
    
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s'
    )
    
    async def test():
        calc = MaxIntelligenceCalculator()
        
        # Test prompts
        test_prompts = [
            "vintage Beatles Abbey Road drums with compression",
            "tape delay at 1/8 dotted with 35% feedback",
            "modern EDM bass with heavy distortion and sidechain compression",
        ]
        
        for prompt in test_prompts:
            print(f"\n{'='*60}")
            print(f"Testing: {prompt}")
            print(f"{'='*60}")
            
            # Create test preset
            test_preset = {
                "name": "Test",
                "slots": [
                    {
                        "slot": 0,
                        "engine_id": 34,  # Tape Echo
                        "engine_name": "Tape Echo",
                        "parameters": [{"name": f"param{i+1}", "value": 0.5} for i in range(15)]
                    },
                    {
                        "slot": 1,
                        "engine_id": 1,  # Vintage Opto Compressor
                        "engine_name": "Vintage Opto Compressor",
                        "parameters": [{"name": f"param{i+1}", "value": 0.5} for i in range(15)]
                    }
                ]
            }
            
            # Test with max intelligence
            optimized = await calc.optimize_parameters_max_intelligence(test_preset, prompt)
            
            print("\nOptimized parameters:")
            for slot in optimized["slots"]:
                if slot["engine_id"] != 0:
                    print(f"\n  {slot['engine_name']}:")
                    for i, param in enumerate(slot["parameters"][:5]):
                        if param["value"] != 0.5:
                            print(f"    param{i+1}: {param['value']:.3f}")
    
    asyncio.run(test())