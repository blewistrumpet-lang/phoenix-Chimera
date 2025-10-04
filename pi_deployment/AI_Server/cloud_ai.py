"""
Cloud AI Implementation Helper
Shared by both Smart Oracle and Smart Calculator
"""

import os
import json
import requests
from typing import Dict, Any, List, Optional
from pathlib import Path
import logging

logger = logging.getLogger(__name__)

class CloudAI:
    """
    Unified Cloud AI interface for Trinity components
    Uses the comprehensive context to make intelligent decisions
    """
    
    def __init__(self):
        self.api_key = os.getenv('OPENAI_API_KEY')
        self.model = os.getenv('AI_MODEL', 'gpt-4')
        self.context = self._load_context()
        self.parameter_manifest = self._load_parameter_manifest()
        
    def _load_context(self) -> str:
        """Load comprehensive Trinity context"""
        context_file = Path("trinity_context.md")
        if context_file.exists():
            return context_file.read_text()
        logger.warning("Trinity context not found - AI will have limited knowledge")
        return ""
    
    def _load_parameter_manifest(self) -> Dict:
        """Load parameter manifest"""
        manifest_file = Path("parameter_manifest.json")
        if manifest_file.exists():
            with open(manifest_file, 'r') as f:
                return json.load(f)
        return {}
    
    def find_best_preset(self, context: Dict, instruction: str) -> Dict:
        """
        Find or create the best preset for a blueprint
        Used by Smart Oracle
        """
        
        prompt = f"""
You are an expert audio engineer helping to find the perfect preset.

SYSTEM CONTEXT:
{self.context}

CURRENT REQUEST:
{json.dumps(context, indent=2)}

TASK: {instruction}

Based on the blueprint and requirements, either:
1. Select the best matching preset from the corpus
2. Create a new preset that perfectly matches the intent

Return a complete preset in this exact JSON format:
{{
    "name": "Creative Preset Name",
    "parameters": {{
        "slot1_engine": <engine_id>,
        "slot1_param0": <0.0-1.0>,
        "slot1_param1": <0.0-1.0>,
        "slot1_mix": <0.0-1.0>,
        "slot2_engine": <engine_id>,
        "slot2_param0": <0.0-1.0>,
        ...
    }},
    "explanation": "Why this preset matches the blueprint",
    "ai_confidence": <0.0-1.0>,
    "genre": "target genre",
    "vibe": "overall character"
}}

Remember:
- Max 5 effects (leave slot 6 empty)
- Signal flow order matters
- Mix levels should be balanced
- Parameters should be musical, not extreme
"""

        try:
            response = self._call_api(prompt)
            return self._parse_preset_response(response)
        except Exception as e:
            logger.error(f"Cloud AI error: {e}")
            return self._fallback_preset(context)
    
    def calculate_nudges(self, context: Dict, instruction: str) -> Dict:
        """
        Calculate parameter adjustments based on prompt
        Used by Smart Calculator
        """
        
        # Add parameter manifest to context
        context['parameter_manifest'] = self.parameter_manifest
        
        prompt = f"""
You are an expert audio engineer adjusting preset parameters.

SYSTEM CONTEXT:
{self.context}

PARAMETER SEMANTICS:
{json.dumps(self.parameter_manifest.get('parameter_semantics', {}), indent=2)}

CURRENT SITUATION:
{json.dumps(context, indent=2)}

TASK: {instruction}

Analyze the prompt and adjust the preset parameters to better match the user's intent.

Return specific adjustments in this exact JSON format:
{{
    "preset": {{
        "name": "Adjusted Preset Name",
        "parameters": {{
            "slot1_param0": <new_value>,
            "slot2_param1": <new_value>,
            ...
        }}
    }},
    "adjustments": [
        {{
            "parameter": "slot1_param0",
            "type": "absolute",
            "from": <old_value>,
            "to": <new_value>,
            "reason": "Why this adjustment helps"
        }},
        ...
    ],
    "confidence": <0.0-1.0>,
    "explanation": "Overall strategy for these adjustments"
}}

Remember:
- Only adjust parameters that need changing
- Subtle adjustments often sound better than extreme ones
- Consider the semantic meaning (warmth, brightness, punch, etc.)
- Maintain musicality and usability
"""

        try:
            response = self._call_api(prompt)
            return self._parse_nudge_response(response)
        except Exception as e:
            logger.error(f"Cloud AI error: {e}")
            return self._fallback_nudge(context)
    
    def _call_api(self, prompt: str) -> Dict:
        """Call the cloud AI API"""
        
        if not self.api_key:
            raise ValueError("OPENAI_API_KEY environment variable not set")
        
        response = requests.post(
            "https://api.openai.com/v1/chat/completions",
            json={
                "model": self.model,
                "messages": [
                    {"role": "system", "content": "You are an expert audio engineer with deep knowledge of the Chimera Phoenix plugin and its Trinity preset system."},
                    {"role": "user", "content": prompt}
                ],
                "temperature": 0.7,
                "response_format": {"type": "json_object"}
            },
            headers={
                "Authorization": f"Bearer {self.api_key}",
                "Content-Type": "application/json"
            },
            timeout=30
        )
        
        response.raise_for_status()
        return response.json()
    
    def _parse_preset_response(self, response: Dict) -> Dict:
        """Parse AI response into preset format"""
        try:
            content = response['choices'][0]['message']['content']
            preset = json.loads(content)
            
            # Validate and clean
            if 'parameters' not in preset:
                preset['parameters'] = {}
            
            # Ensure required fields
            preset.setdefault('ai_confidence', 0.7)
            preset.setdefault('explanation', 'AI-generated preset')
            preset.setdefault('name', 'AI Preset')
            
            return preset
            
        except (KeyError, json.JSONDecodeError) as e:
            logger.error(f"Failed to parse AI response: {e}")
            return self._fallback_preset({})
    
    def _parse_nudge_response(self, response: Dict) -> Dict:
        """Parse AI response into nudge format"""
        try:
            content = response['choices'][0]['message']['content']
            result = json.loads(content)
            
            # Ensure required structure
            if 'preset' not in result:
                result['preset'] = {'parameters': {}}
            if 'adjustments' not in result:
                result['adjustments'] = []
            
            result.setdefault('confidence', 0.7)
            
            return result
            
        except (KeyError, json.JSONDecodeError) as e:
            logger.error(f"Failed to parse AI response: {e}")
            return self._fallback_nudge({})
    
    def _fallback_preset(self, context: Dict) -> Dict:
        """Fallback preset when AI fails"""
        return {
            "name": "Fallback Preset",
            "parameters": {
                "slot1_engine": 7,  # Parametric EQ
                "slot1_param0": 0.5,
                "slot1_mix": 1.0,
                "slot2_engine": 1,  # Classic Compressor
                "slot2_param0": 0.5,
                "slot2_param1": 0.5,
                "slot2_mix": 1.0,
                "slot3_engine": 41,  # Hall Reverb
                "slot3_param0": 0.5,
                "slot3_param1": 0.5,
                "slot3_mix": 0.3
            },
            "ai_confidence": 0.0,
            "explanation": "Fallback preset due to AI unavailability"
        }
    
    def _fallback_nudge(self, context: Dict) -> Dict:
        """Fallback nudge when AI fails"""
        preset = context.get('preset', {})
        return {
            "preset": preset,
            "adjustments": [],
            "confidence": 0.0,
            "explanation": "No adjustments made - AI unavailable"
        }


class CloudAIPool:
    """
    Connection pool for Cloud AI to handle multiple requests efficiently
    """
    
    def __init__(self, pool_size: int = 3):
        self.pool = [CloudAI() for _ in range(pool_size)]
        self.current_index = 0
    
    def get_instance(self) -> CloudAI:
        """Get next available CloudAI instance (round-robin)"""
        instance = self.pool[self.current_index]
        self.current_index = (self.current_index + 1) % len(self.pool)
        return instance


# Singleton instance for easy import
cloud_ai = CloudAI()