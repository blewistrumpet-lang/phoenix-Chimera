#!/usr/bin/env python3
"""
PI-OPTIMIZED Calculator - Single GPT call instead of 3
Reduces timing from ~57s to ~15-20s
"""

import json
import logging
import re
import hashlib
from typing import Dict, List, Any
from openai import AsyncOpenAI
import os

logger = logging.getLogger(__name__)

# This is the UNIFIED function that replaces 3 separate GPT calls
async def unified_intelligent_optimization(openai_client, engines_data, preset: Dict, user_prompt: str, engines: List[Dict]) -> Dict:
    """UNIFIED GPT call - combines style analysis, conflict detection, and creative magic into ONE call for Pi speed"""

    if not openai_client:
        logger.warning("⚠️ No OpenAI client")
        return {}

    # Build engine details
    engine_details = ""
    for engine in engines:
        engine_id = engine.get("engine_id", 0)
        engine_spec = engines_data.get(str(engine_id), {})
        engine_details += f"\nEngine {engine_id}: {engine_spec.get('name', 'Unknown')}\n"
        for i, param in enumerate(engine_spec.get("parameters", [])[:10]):
            engine_details += f"  param{i+1}: {param['name']} ({param.get('description', '')})\n"

    # Build current preset state
    current_state = ""
    for slot in preset.get("slots", []):
        if slot.get("engine_id", 0) != 0:
            current_state += f"\nSlot {slot['slot']}: Engine {slot['engine_id']} ({slot.get('engine_name', 'Unknown')})\n"
            for i, p in enumerate(slot.get("parameters", [])):
                current_state += f"  param{i+1}: {p.get('value', 0.5)}\n"

    prompt = f"""You are an expert audio engineer optimizing a multi-effect preset.

User's intent: "{user_prompt}"

Selected engines and parameters:
{engine_details}

Current preset state:
{current_state}

Provide a COMPREHENSIVE optimization in ONE response covering:

1. MUSICAL STYLE PARAMETERS
   - Set exact parameter values (0.0-1.0) based on the musical style
   - Consider genre, production techniques, and sonic characteristics

2. CONFLICT DETECTION & FIXES
   - Check for frequency conflicts, gain staging issues, timing problems
   - Provide parameter adjustments to fix any conflicts

3. CREATIVE ENHANCEMENTS
   - Add subtle creative touches that elevate the preset
   - Suggest 1-2 "magic" parameter tweaks

Return JSON with this EXACT structure:
{{
  "style_parameters": {{
    "engine_id": {{
      "param1": 0.5,
      "param2": 0.7
    }}
  }},
  "conflict_fixes": {{
    "engine_id": {{
      "param1": 0.6
    }}
  }},
  "creative_magic": [
    {{
      "engine_id": 15,
      "parameter": "drive",
      "value": 0.65,
      "why": "adds subtle harmonic richness"
    }}
  ]
}}

Be concise but musically accurate. Focus on the most important parameters."""

    try:
        response = await openai_client.chat.completions.create(
            model="gpt-3.5-turbo",  # Fast model for Pi
            max_tokens=1500,
            temperature=0.7,
            messages=[{"role": "user", "content": prompt}]
        )

        response_text = response.choices[0].message.content
        json_match = re.search(r'\{.*\}', response_text, re.DOTALL)
        if json_match:
            result = json.loads(json_match.group(0))
            logger.info("🎯 Unified optimization complete (1 GPT call)")
            return result

        logger.warning("⚠️ No JSON in unified response")
        return {}

    except Exception as e:
        logger.error(f"❌ Unified optimization failed: {e}")
        return {}


# Instructions for integration:
#
# In calculator_max_intelligence.py, replace the optimize_parameters_max_intelligence function
# to call this unified function ONCE instead of calling:
# 1. get_musical_style_parameters
# 2. analyze_parameter_relationships
# 3. add_creative_magic
#
# This reduces from 3 GPT API calls to 1, cutting time from ~57s to ~15-20s on Pi
