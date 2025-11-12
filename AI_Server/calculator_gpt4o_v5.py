#!/usr/bin/env python3
"""
Maximum Intelligence Calculator with GPT-4o Integration
Version 5.0 - November 11, 2025

Uses GPT-4o-mini for fast, intelligent parameter optimization
Converted from Claude 3.5 Sonnet to OpenAI GPT-4o
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
from openai import AsyncOpenAI

logger = logging.getLogger(__name__)

class GPT4oCalculator:
    def __init__(self):
        """Initialize with complete engine knowledge and GPT-4o client"""
        # Load the COMPLETE knowledge base
        try:
            with open("trinity_engine_knowledge_COMPLETE.json", "r") as f:
                self.knowledge = json.load(f)
            logger.info(f"✅ GPT-4o Calculator loaded knowledge for {len(self.knowledge['engines'])} engines")
        except FileNotFoundError:
            logger.error("❌ Knowledge base not found!")
            raise

        self.engines = self.knowledge["engines"]

        # Initialize OpenAI client for maximum intelligence
        api_key = os.getenv("OPENAI_API_KEY")
        if api_key:
            self.client = AsyncOpenAI(api_key=api_key)
            logger.info("🧠 GPT-4o client initialized for maximum intelligence")
        else:
            self.client = None
            logger.warning("⚠️ No OpenAI API key - using basic intelligence only")

        # Build parameter mappings
        self.param_mappings = self._build_parameter_mappings()

        # Initialize caching system
        self.cache_file = "parameter_intelligence_cache_gpt4o.json"
        self.intelligence_cache = self._load_cache()

        # Musical time subdivisions (instant parsing - "1/8 dotted" → 0.1875)
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
                "param_details": [],
                "param_count": len(engine_data.get("parameters", []))
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
        """Load cached intelligence from previous GPT responses"""
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
                    "gpt_calls": 0,
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

        # Check for time subdivisions ("1/8 dotted" → 0.1875)
        for subdivision, value in self.time_subdivisions.items():
            if subdivision in prompt_lower:
                extracted["time_subdivision"] = {
                    "value": value,
                    "original": subdivision,
                    "type": "time"
                }
                logger.info(f"📐 Found time: {subdivision} = {value}")

        # Check for percentages ("35%" → 0.35)
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

        # Check for ratios ("4:1" → compression ratio)
        for match in self.param_patterns["ratio"].finditer(prompt_lower):
            numerator = float(match.group(1))
            denominator = float(match.group(2))
            ratio_val = numerator / denominator if denominator > 0 else numerator
            normalized = min(1.0, ratio_val / 20.0)

            extracted["ratio"] = {
                "value": normalized,
                "original": f"{numerator}:{denominator}",
                "actual_ratio": ratio_val,
                "type": "ratio"
            }
            logger.info(f"🔢 Found ratio: {numerator}:{denominator} = {normalized}")

        return extracted

    async def get_musical_style_parameters(self, prompt: str, engines: List[Dict]) -> Dict:
        """Ask GPT-4o for deep musical understanding of style"""

        # Check cache first
        cache_key = hashlib.md5(f"{prompt}_{engines}".encode()).hexdigest()
        if cache_key in self.intelligence_cache.get("style_parameters", {}):
            logger.info("📚 Using cached style parameters")
            self.intelligence_cache["statistics"]["cache_hits"] += 1
            return self.intelligence_cache["style_parameters"][cache_key]

        if not self.client:
            logger.warning("⚠️ No GPT-4o client - using defaults")
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

        system_prompt = "You are a legendary audio engineer with expertise in every musical genre and production technique."

        user_prompt = f"""User request: "{prompt}"

Selected engines:{engine_details}

Provide EXACT parameter values (0.0-1.0) based on musical knowledge.

Return JSON:
{{
  "style_analysis": "Brief musical style description",
  "parameter_values": {{
    "engine_id": {{
      "param1": 0.5,
      "param2": 0.3
    }}
  }},
  "key_characteristics": ["char1", "char2"],
  "reference_points": ["reference"]
}}"""

        try:
            response = await self.client.chat.completions.create(
                model="gpt-4o-mini",
                messages=[
                    {"role": "system", "content": system_prompt},
                    {"role": "user", "content": user_prompt}
                ],
                response_format={"type": "json_object"},
                max_tokens=2000,
                temperature=0.7
            )

            response_text = response.choices[0].message.content
            result = json.loads(response_text)

            # Cache the result
            self.intelligence_cache["style_parameters"][cache_key] = result
            self.intelligence_cache["statistics"]["gpt_calls"] = self.intelligence_cache["statistics"].get("gpt_calls", 0) + 1
            self.intelligence_cache["statistics"]["total_tokens"] = self.intelligence_cache["statistics"].get("total_tokens", 0) + response.usage.total_tokens
            self._save_cache()

            logger.info(f"🎨 GPT-4o provided style parameters for: {result.get('style_analysis', 'Unknown')}")
            return result

        except Exception as e:
            logger.error(f"❌ GPT-4o API error: {str(e)}")
            return {}

    async def optimize_parameters_max_intelligence(self, preset: Dict[str, Any], user_prompt: str) -> Dict[str, Any]:
        """Maximum intelligence parameter optimization using GPT-4o"""
        logger.info(f"🧠 MAX INTELLIGENCE optimization for: '{user_prompt}'")

        # STEP 1: Parse obvious values (instant, no API calls)
        extracted_values = self.parse_prompt_values(user_prompt)

        # Apply extracted values immediately
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            if engine_id == 0:
                continue
            self._apply_extracted_values_to_slot(slot, engine_id, extracted_values, user_prompt)

        # STEP 2: Get musical style parameters from GPT-4o
        engines = [s for s in preset.get("slots", []) if s.get("engine_id", 0) != 0]
        if self.client:
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
                                    logger.info(f"  Set {param_key} = {param_value} (GPT style)")

        # Log intelligence statistics
        stats = self.intelligence_cache.get("statistics", {})
        logger.info(f"📊 Intelligence stats: {stats.get('cache_hits', 0)} cache hits, "
                   f"{stats.get('gpt_calls', 0)} GPT calls, "
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
