"""
Smart Calculator with AI Cascade and Pattern Learning
Combines rule-based nudging with Cloud AI for complex adjustments
"""

import json
import time
import hashlib
import numpy as np
from typing import Dict, Any, List, Optional, Tuple
from pathlib import Path
import logging
from cloud_ai import CloudAI

logger = logging.getLogger(__name__)

def _generate_pattern_key(prompt: str, intent: Dict) -> str:
    """Generate pattern cache key"""
    # Create semantic key that similar prompts share
    key_parts = [
        intent['primary'],
        str(int(intent['intensity'] * 10)),
        '|'.join(sorted(intent.get('targets', []))),
        '|'.join(sorted(intent.get('modifiers', [])))
    ]
    
    key_string = '_'.join(key_parts).lower()
    return hashlib.md5(key_string.encode()).hexdigest()[:16]

class SmartCalculator:
    """
    Intelligent Calculator that cascades from fast to smart:
    1. Pattern cache (learned from AI)
    2. Rule-based nudging (immediate)
    3. Cloud AI (for complex/creative)
    """
    
    def __init__(self, rules_path: str = "nudge_rules.json"):
        # Original rule-based system
        with open(rules_path, 'r') as f:
            self.rules = json.load(f)
        
        # New AI components
        self.pattern_cache = PatternCache()
        self.cloud_ai = CloudAI()
        
        # Confidence thresholds
        self.rule_confidence_threshold = 0.8
        self.complexity_threshold = 0.7
        
        # Metrics
        self.stats = {
            "pattern_hits": 0,
            "rule_hits": 0,
            "ai_escalations": 0,
            "total_requests": 0,
            "patterns_learned": 0
        }
    
    def apply_nudges(self, base_preset: Dict, prompt: str, blueprint: Dict) -> Dict:
        """Main entry point - intelligent nudging with cascade"""
        
        self.stats["total_requests"] += 1
        start_time = time.time()
        
        # Analyze prompt complexity
        complexity = self._analyze_complexity(prompt)
        intent = self._extract_intent(prompt)
        
        # Step 1: Check learned patterns (fastest)
        pattern_key = _generate_pattern_key(prompt, intent)
        if pattern := self.pattern_cache.get(pattern_key):
            logger.info(f"Pattern cache hit for intent: {intent['primary']}")
            self.stats["pattern_hits"] += 1
            
            result = self._apply_pattern(base_preset, pattern)
            return self._add_metadata(result, source="pattern", time=time.time()-start_time)
        
        # Step 2: Try rules for simple cases
        if complexity < self.complexity_threshold:
            rule_result = self._apply_rules(base_preset, prompt, blueprint)
            
            if rule_result['confidence'] > self.rule_confidence_threshold:
                logger.info(f"Rules applied with {rule_result['confidence']:.2f} confidence")
                self.stats["rule_hits"] += 1
                
                # Learn this pattern for next time if very confident
                if rule_result['confidence'] > 0.95:
                    self.pattern_cache.learn(pattern_key, rule_result['adjustments'])
                
                return self._add_metadata(rule_result['preset'], 
                                        source="rules", 
                                        confidence=rule_result['confidence'],
                                        time=time.time()-start_time)
        
        # Step 3: Use AI for complex/creative requests
        logger.info(f"Escalating to AI (complexity: {complexity:.2f})")
        self.stats["ai_escalations"] += 1
        
        ai_result = self._cloud_ai_nudge(
            base_preset, prompt, blueprint,
            complexity=complexity,
            intent=intent,
            rule_attempt=rule_result if complexity < self.complexity_threshold else None
        )
        
        # Learn from AI response
        self.pattern_cache.learn(pattern_key, ai_result['adjustments'])
        self.stats["patterns_learned"] += 1
        
        # Optionally generate new rules from AI patterns
        if ai_result.get('confidence', 0) > 0.9:
            self._extract_rules_from_ai(prompt, ai_result)
        
        return self._add_metadata(ai_result['preset'], 
                                source="ai",
                                time=time.time()-start_time)
    
    def _analyze_complexity(self, prompt: str) -> float:
        """
        Analyze prompt complexity to decide routing
        0.0 = very simple, 1.0 = very complex
        """
        complexity = 0.0
        
        # Length factor
        word_count = len(prompt.split())
        complexity += min(word_count / 20, 0.3)  # Max 0.3 from length
        
        # Technical terms
        technical_terms = ['eq', 'compression', 'threshold', 'ratio', 'frequency',
                          'resonance', 'cutoff', 'harmonics', 'phase', 'stereo']
        tech_count = sum(1 for term in technical_terms if term in prompt.lower())
        complexity += min(tech_count * 0.1, 0.3)  # Max 0.3 from technical
        
        # Creative/subjective terms
        creative_terms = ['vibe', 'feel', 'character', 'texture', 'color', 'mood',
                         'atmosphere', 'energy', 'organic', 'lush', 'gritty']
        creative_count = sum(1 for term in creative_terms if term in prompt.lower())
        complexity += min(creative_count * 0.15, 0.4)  # Max 0.4 from creative
        
        # Specific numeric requests
        if any(char.isdigit() for char in prompt):
            complexity += 0.2  # Specific numbers = complex
        
        return min(complexity, 1.0)
    
    def _extract_intent(self, prompt: str) -> Dict[str, Any]:
        """Extract primary intent and parameters from prompt"""
        prompt_lower = prompt.lower()
        
        intent = {
            'primary': 'general',
            'modifiers': [],
            'intensity': 0.5,
            'targets': []
        }
        
        # Detect primary intent
        if any(w in prompt_lower for w in ['warm', 'warmth', 'cozy', 'analog']):
            intent['primary'] = 'warmth'
            intent['intensity'] = 0.7
        elif any(w in prompt_lower for w in ['bright', 'crisp', 'sparkle', 'shiny']):
            intent['primary'] = 'brightness'
            intent['intensity'] = 0.7
        elif any(w in prompt_lower for w in ['punch', 'punchy', 'slam', 'hit']):
            intent['primary'] = 'punch'
            intent['intensity'] = 0.8
        elif any(w in prompt_lower for w in ['space', 'spacious', 'wide', 'huge']):
            intent['primary'] = 'space'
            intent['intensity'] = 0.7
        elif any(w in prompt_lower for w in ['dirty', 'gritty', 'rough', 'raw']):
            intent['primary'] = 'dirt'
            intent['intensity'] = 0.6
        
        # Detect modifiers
        if 'subtle' in prompt_lower:
            intent['intensity'] *= 0.5
            intent['modifiers'].append('subtle')
        if 'extreme' in prompt_lower:
            intent['intensity'] = min(intent['intensity'] * 1.5, 1.0)
            intent['modifiers'].append('extreme')
        
        # Detect targets
        if 'bass' in prompt_lower:
            intent['targets'].append('bass')
        if 'treble' in prompt_lower or 'highs' in prompt_lower:
            intent['targets'].append('highs')
        if 'mids' in prompt_lower:
            intent['targets'].append('mids')
        
        return intent
    
    def _apply_rules(self, preset: Dict, prompt: str, blueprint: Dict) -> Dict:
        """Apply rule-based nudging"""
        
        adjustments = []
        confidence = 0.0
        modified_preset = preset.copy()
        params = modified_preset.get('parameters', {})
        
        # Apply keyword-based rules
        for keyword, rules in self.rules.get('keyword_rules', {}).items():
            if keyword.lower() in prompt.lower():
                for adjustment in rules:
                    param_key = adjustment['parameter']
                    
                    if param_key in params:
                        old_value = params[param_key]
                        
                        # Apply adjustment
                        if adjustment['type'] == 'multiply':
                            new_value = old_value * adjustment['value']
                        elif adjustment['type'] == 'add':
                            new_value = old_value + adjustment['value']
                        elif adjustment['type'] == 'set':
                            new_value = adjustment['value']
                        else:
                            new_value = old_value
                        
                        # Clamp to valid range
                        new_value = max(0.0, min(1.0, new_value))
                        params[param_key] = new_value
                        
                        adjustments.append({
                            'parameter': param_key,
                            'from': old_value,
                            'to': new_value,
                            'reason': f"Keyword '{keyword}' triggered"
                        })
                        
                        confidence += 0.2  # Each successful rule adds confidence
        
        # Apply engine-specific rules
        for slot in range(1, 7):
            engine_key = f"slot{slot}_engine"
            if engine_key in params:
                engine_id = params[engine_key]
                
                if str(engine_id) in self.rules.get('engine_rules', {}):
                    engine_rules = self.rules['engine_rules'][str(engine_id)]
                    
                    for param_idx, param_rules in engine_rules.items():
                        param_key = f"slot{slot}_param{param_idx}"
                        
                        for keyword, adjustment in param_rules.items():
                            if keyword.lower() in prompt.lower():
                                old_value = params.get(param_key, 0.5)
                                new_value = adjustment
                                
                                params[param_key] = new_value
                                adjustments.append({
                                    'parameter': param_key,
                                    'from': old_value,
                                    'to': new_value,
                                    'reason': f"Engine-specific rule for '{keyword}'"
                                })
                                
                                confidence += 0.15
        
        confidence = min(confidence, 1.0)
        
        return {
            'preset': modified_preset,
            'adjustments': adjustments,
            'confidence': confidence
        }
    
    def _cloud_ai_nudge(self, preset: Dict, prompt: str, blueprint: Dict,
                       complexity: float, intent: Dict, 
                       rule_attempt: Optional[Dict] = None) -> Dict:
        """Use Cloud AI for sophisticated nudging"""
        
        context = {
            'preset': preset,
            'prompt': prompt,
            'blueprint': blueprint,
            'complexity': complexity,
            'intent': intent,
            'rule_attempt': rule_attempt,
            'parameter_manifest': self._load_parameter_manifest()
        }
        
        response = self.cloud_ai.calculate_nudges(
            context=context,
            instruction=f"""
            Adjust the preset parameters to match the prompt intent.
            Primary intent: {intent['primary']}
            Intensity: {intent['intensity']}
            Targets: {intent.get('targets', [])}
            
            Return specific parameter adjustments with explanations.
            """
        )
        
        return response
    
    def _apply_pattern(self, preset: Dict, pattern: Dict) -> Dict:
        """Apply learned pattern to preset"""
        modified = preset.copy()
        params = modified.get('parameters', {})
        
        for adjustment in pattern.get('adjustments', []):
            param = adjustment['parameter']
            
            # Handle relative adjustments
            if adjustment['type'] == 'relative':
                if param in params:
                    params[param] = np.clip(
                        params[param] + adjustment['delta'],
                        0.0, 1.0
                    )
            # Handle absolute adjustments
            elif adjustment['type'] == 'absolute':
                params[param] = adjustment['value']
            # Handle proportional adjustments
            elif adjustment['type'] == 'proportional':
                if param in params:
                    params[param] = np.clip(
                        params[param] * adjustment['factor'],
                        0.0, 1.0
                    )
        
        return modified
    
    def _extract_rules_from_ai(self, prompt: str, ai_result: Dict):
        """Extract reusable rules from successful AI adjustments"""
        
        # Analyze what the AI did
        adjustments = ai_result.get('adjustments', [])
        
        if not adjustments:
            return
        
        # Look for patterns
        patterns = {}
        
        for adj in adjustments:
            param = adj['parameter']
            change = adj['to'] - adj['from']
            
            # Group by parameter type
            if 'param0' in param:  # Primary controls
                patterns['primary'] = patterns.get('primary', [])
                patterns['primary'].append(change)
            elif 'param1' in param:  # Secondary controls
                patterns['secondary'] = patterns.get('secondary', [])
                patterns['secondary'].append(change)
            elif 'mix' in param:  # Mix controls
                patterns['mix'] = patterns.get('mix', [])
                patterns['mix'].append(change)
        
        # If we see consistent patterns, create a new rule
        for param_type, changes in patterns.items():
            if len(changes) >= 3:  # Need multiple examples
                avg_change = np.mean(changes)
                
                if abs(avg_change) > 0.1:  # Significant change
                    # Create new rule
                    rule_key = f"learned_{prompt[:20]}_{param_type}"
                    
                    if 'learned_rules' not in self.rules:
                        self.rules['learned_rules'] = {}
                    
                    self.rules['learned_rules'][rule_key] = {
                        'pattern': prompt[:50],
                        'param_type': param_type,
                        'adjustment': avg_change,
                        'confidence': ai_result.get('confidence', 0.5),
                        'learned_at': time.time()
                    }
                    
                    logger.info(f"Learned new rule: {rule_key}")
    
    def _load_parameter_manifest(self) -> Dict:
        """Load parameter descriptions for AI context"""
        manifest_file = Path("parameter_manifest.json")
        if manifest_file.exists():
            with open(manifest_file, 'r') as f:
                return json.load(f)
        return {}
    
    def _add_metadata(self, preset: Dict, **kwargs) -> Dict:
        """Add metadata about nudging process"""
        if 'calculator_metadata' not in preset:
            preset['calculator_metadata'] = {}
        preset['calculator_metadata'].update(kwargs)
        return preset
    
    def get_stats(self) -> Dict:
        """Get performance statistics"""
        total = max(self.stats["total_requests"], 1)
        return {
            **self.stats,
            "pattern_hit_rate": self.stats["pattern_hits"] / total,
            "rule_hit_rate": self.stats["rule_hits"] / total,
            "ai_escalation_rate": self.stats["ai_escalations"] / total,
            "avg_patterns_per_request": self.stats["patterns_learned"] / total
        }


class PatternCache:
    """
    Cache for learned adjustment patterns
    Stores successful nudge patterns for reuse
    """
    
    def __init__(self, cache_dir: str = "calculator_cache"):
        self.cache_dir = Path(cache_dir)
        self.cache_dir.mkdir(exist_ok=True)
        
        self.patterns = {}
        self.load_patterns()
    
    def get(self, key: str) -> Optional[Dict]:
        """Get pattern if exists"""
        return self.patterns.get(key)
    
    def learn(self, key: str, adjustments: List[Dict]):
        """Learn a new pattern"""
        
        # Analyze adjustments to create reusable pattern
        pattern = {
            'key': key,
            'adjustments': adjustments,
            'learned_at': time.time(),
            'usage_count': 0,
            'success_rate': 1.0
        }
        
        self.patterns[key] = pattern
        
        # Persist to disk
        pattern_file = self.cache_dir / f"{key}.json"
        with open(pattern_file, 'w') as f:
            json.dump(pattern, f)
    
    def load_patterns(self):
        """Load patterns from disk"""
        for pattern_file in self.cache_dir.glob("*.json"):
            try:
                with open(pattern_file, 'r') as f:
                    pattern = json.load(f)
                    key = pattern_file.stem
                    self.patterns[key] = pattern
            except:
                continue
    
    def update_success(self, key: str, success: bool):
        """Update pattern success rate based on user feedback"""
        if key in self.patterns:
            pattern = self.patterns[key]
            pattern['usage_count'] += 1
            
            # Update success rate with exponential moving average
            alpha = 0.1  # Learning rate
            pattern['success_rate'] = (
                (1 - alpha) * pattern['success_rate'] + 
                alpha * (1.0 if success else 0.0)
            )


def _generate_pattern_key(prompt: str, intent: Dict) -> str:
    """Generate pattern cache key"""
    # Create semantic key that similar prompts share
    key_parts = [
        intent['primary'],
        str(int(intent['intensity'] * 10)),
        '|'.join(sorted(intent.get('targets', []))),
        '|'.join(sorted(intent.get('modifiers', [])))
    ]
    
    key_string = '_'.join(key_parts).lower()
    return hashlib.md5(key_string.encode()).hexdigest()[:16]