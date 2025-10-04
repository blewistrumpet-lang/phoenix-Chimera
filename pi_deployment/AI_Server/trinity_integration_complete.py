#!/usr/bin/env python3
"""
Complete Trinity AI Integration
Uses real corpus, correct engine mappings, and actual AI
"""

import json
import os
import sys
import faiss
import numpy as np
from pathlib import Path
from typing import Dict, List, Any

# Add parent directory to path
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from engine_mapping_authoritative import *
from smart_oracle import SmartOracle
from smart_calculator import SmartCalculator
from cloud_ai import CloudAI

class TrinityAIIntegration:
    """Complete AI integration for Trinity pipeline"""
    
    def __init__(self):
        print("Initializing Trinity AI Integration...")
        
        # Load unified engine manifest
        with open('unified_engine_manifest.json', 'r') as f:
            self.engine_manifest = json.load(f)
        
        # Load environment
        self.setup_environment()
        
        # Load real corpus
        self.corpus_path = '/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3/faiss_index'
        self.presets = self.load_corpus()
        
        # Initialize FAISS index
        self.index, self.metadata = self.setup_faiss()
        
        # Initialize AI components
        self.setup_ai_components()
        
        print(f"✓ Loaded {len(self.presets)} presets")
        print(f"✓ API Key configured: {bool(os.getenv('OPENAI_API_KEY'))}")
        print("✓ All systems ready!")
    
    def setup_environment(self):
        """Set up environment with API key"""
        # Load from .env file
        env_file = Path('.env')
        if env_file.exists():
            with open(env_file) as f:
                for line in f:
                    if '=' in line and not line.startswith('#'):
                        key, value = line.strip().split('=', 1)
                        os.environ[key] = value
        
        if not os.getenv('OPENAI_API_KEY'):
            print("⚠️ Warning: No OpenAI API key found")
    
    def load_corpus(self) -> List[Dict]:
        """Load the real preset corpus"""
        corpus_file = f"{self.corpus_path}/presets_clean.json"
        
        if not os.path.exists(corpus_file):
            print(f"Error: Corpus file not found at {corpus_file}")
            return []
        
        with open(corpus_file, 'r') as f:
            presets = json.load(f)
        
        # Convert to dict format for Oracle
        preset_dict = {}
        for i, preset in enumerate(presets):
            preset_id = preset.get('id', f"GC_{i+1:04d}")
            
            # The preset already has all fields in flat format
            # Just copy them as parameters
            parameters = {}
            
            # Copy all slot-related fields
            for key, value in preset.items():
                if key.startswith('slot'):
                    parameters[key] = value
            
            # Ensure all slots have at least engine ID
            for slot in range(1, 7):
                engine_key = f"slot{slot}_engine"
                if engine_key not in parameters:
                    parameters[engine_key] = 0  # Empty slot
            
            preset_dict[preset_id] = {
                'id': preset_id,
                'name': preset.get('creative_name', 'Unnamed'),
                'parameters': parameters,
                'category': preset.get('category', 'General'),
                'vibe': preset.get('vibe', ''),
                'genre': preset.get('category', 'General').lower()
            }
        
        return list(preset_dict.values())
    
    def setup_faiss(self):
        """Set up FAISS index from corpus"""
        # Check if pre-built index exists
        index_file = f"{self.corpus_path}/corpus_clean.index"
        metadata_file = f"{self.corpus_path}/metadata_clean.json"
        
        if os.path.exists(index_file) and os.path.exists(metadata_file):
            print("Loading existing FAISS index...")
            index = faiss.read_index(index_file)
            with open(metadata_file, 'r') as f:
                metadata = json.load(f)
        else:
            print("Building FAISS index from corpus...")
            # Create vectors from presets
            vectors = []
            metadata = []
            
            for preset in self.presets:
                vector = self.preset_to_vector(preset)
                vectors.append(vector)
                metadata.append({
                    'id': preset['id'],
                    'name': preset['name']
                })
            
            # Build index
            d = 128  # dimension
            index = faiss.IndexFlatL2(d)
            
            if vectors:
                vectors_array = np.array(vectors).astype('float32')
                index.add(vectors_array)
            
            # Save for future use
            faiss.write_index(index, "oracle_index.faiss")
            with open("oracle_metadata.json", 'w') as f:
                json.dump(metadata, f)
        
        return index, metadata
    
    def preset_to_vector(self, preset: Dict) -> np.ndarray:
        """Convert preset to FAISS vector"""
        vector = np.zeros(128)
        params = preset.get('parameters', {})
        
        # Encode engines used (first 57 dimensions)
        for slot in range(1, 7):
            engine_key = f"slot{slot}_engine"
            if engine_key in params:
                engine_id = params[engine_key]
                if 0 < engine_id < 57:
                    vector[engine_id] = 1.0
        
        # Encode category (dimensions 57-67)
        category = preset.get('category', '').lower()
        category_map = {
            'dynamics': 57, 'eq': 58, 'filter': 59,
            'distortion': 60, 'modulation': 61, 'delay': 62,
            'reverb': 63, 'spatial': 64, 'special': 65, 'utility': 66
        }
        if category in category_map:
            vector[category_map[category]] = 1.0
        
        # Encode vibe characteristics (dimensions 68-80)
        vibe = preset.get('vibe', '').lower()
        if 'warm' in vibe:
            vector[68] = 1.0
        if 'bright' in vibe:
            vector[69] = 1.0
        if 'vintage' in vibe:
            vector[70] = 1.0
        if 'modern' in vibe:
            vector[71] = 1.0
        if 'aggressive' in vibe:
            vector[72] = 1.0
        if 'smooth' in vibe:
            vector[73] = 1.0
        
        # Encode some parameter averages (dimensions 81-100)
        for slot in range(1, 7):
            # Average mix level
            mix_key = f"slot{slot}_mix"
            if mix_key in params:
                vector[80 + slot] = params[mix_key]
        
        return vector.astype('float32')
    
    def setup_ai_components(self):
        """Initialize Smart Oracle and Calculator"""
        # Save corpus files for Oracle
        preset_dict = {p['id']: p for p in self.presets}
        with open("oracle_presets.json", 'w') as f:
            json.dump(preset_dict, f)
        
        # Initialize components
        try:
            self.oracle = SmartOracle(
                "oracle_index.faiss",
                "oracle_metadata.json", 
                "oracle_presets.json"
            )
            print("✓ Smart Oracle initialized")
        except Exception as e:
            print(f"⚠️ Oracle initialization failed: {e}")
            self.oracle = None
        
        try:
            # Create nudge rules from engine manifest
            self.create_nudge_rules()
            self.calculator = SmartCalculator("nudge_rules.json")
            print("✓ Smart Calculator initialized")
        except Exception as e:
            print(f"⚠️ Calculator initialization failed: {e}")
            self.calculator = None
        
        # Test Cloud AI
        try:
            self.cloud_ai = CloudAI()
            if self.cloud_ai.api_key:
                print("✓ Cloud AI connected")
            else:
                print("⚠️ Cloud AI not configured (no API key)")
        except Exception as e:
            print(f"⚠️ Cloud AI initialization failed: {e}")
    
    def create_nudge_rules(self):
        """Create nudge rules based on engine manifest"""
        rules = {
            "keyword_rules": {
                "warm": [
                    {"parameter": "slot1_param0", "type": "set", "value": 0.6},
                    {"parameter": "slot2_param0", "type": "set", "value": 0.5}
                ],
                "bright": [
                    {"parameter": "slot1_param9", "type": "set", "value": 0.7}
                ],
                "punch": [
                    {"parameter": "slot1_param2", "type": "set", "value": 0.2},
                    {"parameter": "slot1_param3", "type": "set", "value": 0.4}
                ],
                "spacious": [
                    {"parameter": "slot3_param0", "type": "set", "value": 0.8},
                    {"parameter": "slot3_mix", "type": "set", "value": 0.4}
                ],
                "aggressive": [
                    {"parameter": "slot2_param0", "type": "set", "value": 0.8}
                ],
                "vintage": [
                    {"parameter": "slot1_param2", "type": "set", "value": 0.3}
                ]
            },
            "engine_rules": {}
        }
        
        # Add engine-specific rules
        for engine_id in [15, 16, 17, 18]:  # Distortion engines
            rules["engine_rules"][str(engine_id)] = {
                "0": {"warm": 0.6, "aggressive": 0.8, "subtle": 0.3}
            }
        
        with open("nudge_rules.json", 'w') as f:
            json.dump(rules, f)
    
    def test_prompt(self, prompt: str) -> Dict:
        """Test a single prompt through the system"""
        print(f"\n{'='*60}")
        print(f"PROMPT: {prompt}")
        print('='*60)
        
        # Create blueprint (simulating Visionary output)
        blueprint = self.create_blueprint(prompt)
        print(f"\n1. Blueprint created: {blueprint['overall_vibe']}")
        
        # Find best preset (Oracle)
        if self.oracle:
            preset = self.oracle.find_best_preset(blueprint)
            source = preset.get('oracle_metadata', {}).get('source', 'unknown')
            print(f"\n2. Oracle found preset: {preset.get('name', 'Unknown')}")
            print(f"   Source: {source}")
            
            # Show engines used
            engines_used = []
            params = preset.get('parameters', {})
            for slot in range(1, 7):
                engine_id = params.get(f"slot{slot}_engine", 0)
                if engine_id > 0:
                    engine_name = self.engine_manifest['engine_mapping'][str(engine_id)]['name']
                    engines_used.append(f"{engine_name} ({engine_id})")
            print(f"   Engines: {' → '.join(engines_used[:5])}")
        else:
            preset = self.fallback_preset()
            print("   Using fallback preset")
        
        # Apply nudges (Calculator)
        if self.calculator:
            adjusted = self.calculator.apply_nudges(preset, prompt, blueprint)
            adj_source = adjusted.get('calculator_metadata', {}).get('source', 'unknown')
            print(f"\n3. Calculator adjusted parameters")
            print(f"   Source: {adj_source}")
        else:
            adjusted = preset
        
        return adjusted
    
    def create_blueprint(self, prompt: str) -> Dict:
        """Create a blueprint from prompt (simulating Visionary)"""
        prompt_lower = prompt.lower()
        
        # Detect genre
        if any(word in prompt_lower for word in ['dubstep', 'trap', 'edm', 'electronic']):
            genre = 'electronic'
        elif any(word in prompt_lower for word in ['rock', 'metal', 'guitar']):
            genre = 'rock'
        elif any(word in prompt_lower for word in ['ambient', 'atmospheric', 'ethereal']):
            genre = 'ambient'
        else:
            genre = 'general'
        
        # Detect vibe
        vibes = []
        if 'warm' in prompt_lower:
            vibes.append('warm')
        if 'aggressive' in prompt_lower or 'hard' in prompt_lower:
            vibes.append('aggressive')
        if 'vintage' in prompt_lower or 'classic' in prompt_lower:
            vibes.append('vintage')
        if 'spacious' in prompt_lower or 'ethereal' in prompt_lower:
            vibes.append('spacious')
        
        overall_vibe = ' '.join(vibes) if vibes else 'neutral'
        
        # Detect desired effects
        slots = []
        if 'reverb' in prompt_lower:
            slots.append({'engine_id': 42, 'category': 'reverb'})
        if 'delay' in prompt_lower:
            slots.append({'engine_id': 35, 'category': 'delay'})
        if 'distortion' in prompt_lower or 'fuzz' in prompt_lower:
            slots.append({'engine_id': 20, 'category': 'distortion'})
        
        return {
            'overall_vibe': overall_vibe,
            'genre': genre,
            'max_engines': 5,
            'slots': slots
        }
    
    def fallback_preset(self) -> Dict:
        """Fallback preset when Oracle unavailable"""
        return {
            'name': 'Default Preset',
            'parameters': {
                'slot1_engine': 7,  # Parametric EQ
                'slot1_mix': 1.0,
                'slot2_engine': 2,  # Compressor
                'slot2_mix': 1.0,
                'slot3_engine': 42,  # Shimmer Reverb
                'slot3_mix': 0.3
            }
        }
    
    def run_test_suite(self):
        """Run complete test suite"""
        print("\n" + "="*60)
        print("🎯 TRINITY AI INTEGRATION TEST")
        print("="*60)
        
        test_prompts = [
            "Create a massive dubstep bass with wobble and grit",
            "Warm vintage guitar with tube saturation and spring reverb",
            "Ethereal ambient pad with infinite space",
            "Punchy trap drums with transient snap",
            "Lo-fi bedroom vocals with tape warmth"
        ]
        
        for prompt in test_prompts:
            result = self.test_prompt(prompt)
        
        # Show statistics
        if self.oracle:
            stats = self.oracle.get_stats()
            print(f"\n📊 Oracle Statistics:")
            print(f"  Cache hits: {stats.get('cache_hits', 0)}")
            print(f"  FAISS hits: {stats.get('faiss_hits', 0)}")
            print(f"  AI escalations: {stats.get('ai_escalations', 0)}")
        
        if self.calculator:
            stats = self.calculator.get_stats()
            print(f"\n📊 Calculator Statistics:")
            print(f"  Pattern hits: {stats.get('pattern_hits', 0)}")
            print(f"  Rule hits: {stats.get('rule_hits', 0)}")
            print(f"  AI escalations: {stats.get('ai_escalations', 0)}")

def main():
    """Run the complete integration"""
    integration = TrinityAIIntegration()
    integration.run_test_suite()

if __name__ == "__main__":
    main()