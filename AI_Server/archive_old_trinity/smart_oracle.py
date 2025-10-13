"""
Smart Oracle with AI Cascade and Learning
Combines FAISS for speed with Cloud AI for intelligence
"""

import os
import json
import asyncio
import hashlib
import time
from typing import Dict, Any, Optional
from pathlib import Path
import numpy as np
from datetime import datetime
import faiss
import logging

logger = logging.getLogger(__name__)

# Import the helper function
def _generate_cache_key(blueprint: Dict) -> str:
    """Generate semantic cache key from blueprint"""
    # Create semantic hash that similar blueprints share
    key_parts = [
        blueprint.get('overall_vibe', ''),
        str(blueprint.get('max_engines', 5)),
        blueprint.get('genre', ''),
        # Include key engines
        ','.join(str(s['engine_id']) for s in blueprint.get('slots', [])[:3])
    ]
    
    key_string = '|'.join(key_parts).lower()
    return hashlib.md5(key_string.encode()).hexdigest()[:16]

class SmartOracle:
    """
    Intelligent Oracle that cascades from fast to smart:
    1. Semantic cache (learned from AI)
    2. FAISS vector search (150 presets)
    3. Cloud AI (when needed)
    """
    
    def __init__(self, index_path: str, meta_path: str, presets_path: str):
        # Original FAISS components
        self.index = faiss.read_index(index_path)
        with open(meta_path, 'r') as f:
            self.metadata = json.load(f)
        with open(presets_path, 'r') as f:
            self.presets = json.load(f)
        
        # New AI components
        self.semantic_cache = SemanticCache()
        self.cloud_ai = CloudAI()
        
        # Confidence thresholds (tunable)
        self.faiss_confidence_threshold = 0.85
        self.cache_confidence_threshold = 0.9
        
        # Metrics
        self.stats = {
            "cache_hits": 0,
            "faiss_hits": 0,
            "ai_escalations": 0,
            "total_requests": 0
        }
    
    def find_best_preset(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Main entry point - cascading intelligence"""
        
        self.stats["total_requests"] += 1
        start_time = time.time()
        
        # Step 1: Check semantic cache (fastest)
        cache_key = _generate_cache_key(blueprint)
        if cached := self.semantic_cache.get(cache_key):
            logger.info(f"Cache hit for {blueprint.get('overall_vibe', 'unknown')}")
            self.stats["cache_hits"] += 1
            return self._add_metadata(cached, source="cache", time=time.time()-start_time)
        
        # Step 2: Try FAISS (fast)
        faiss_result = self._faiss_search(blueprint)
        confidence = self._calculate_confidence(faiss_result, blueprint)
        
        if confidence > self.faiss_confidence_threshold:
            logger.info(f"FAISS match with {confidence:.2f} confidence")
            self.stats["faiss_hits"] += 1
            
            # Cache this good result for next time
            if confidence > self.cache_confidence_threshold:
                self.semantic_cache.add(cache_key, faiss_result)
            
            return self._add_metadata(faiss_result, source="faiss", 
                                     confidence=confidence, time=time.time()-start_time)
        
        # Step 3: Escalate to AI (intelligent)
        logger.info(f"Escalating to AI (FAISS confidence only {confidence:.2f})")
        self.stats["ai_escalations"] += 1
        
        ai_result = self._cloud_ai_search(blueprint, faiss_result, confidence)
        
        # Learn from AI response
        self.semantic_cache.add(cache_key, ai_result)
        self._update_faiss_if_excellent(ai_result, blueprint)
        
        return self._add_metadata(ai_result, source="ai", time=time.time()-start_time)
    
    def _faiss_search(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Original FAISS search logic"""
        # Convert blueprint to vector
        vector = self._blueprint_to_vector(blueprint)
        
        # Search FAISS
        distances, indices = self.index.search(np.array([vector]), k=5)
        
        best_idx = indices[0][0]
        best_distance = distances[0][0]
        
        # Get preset
        preset_id = self.metadata[best_idx]['id']
        preset = self.presets[preset_id].copy()
        preset['similarity_score'] = 1.0 / (1.0 + best_distance)
        
        return preset
    
    def _calculate_confidence(self, preset: Dict, blueprint: Dict) -> float:
        """
        Calculate confidence that FAISS result is good enough
        Considers multiple factors beyond just vector distance
        """
        confidence = preset.get('similarity_score', 0.5)
        
        # Check engine count match
        blueprint_engines = len([s for s in blueprint['slots'] if s['engine_id'] > 0])
        preset_engines = len([k for k in preset.get('parameters', {}) 
                            if k.startswith('slot') and '_engine' in k 
                            and preset['parameters'][k] > 0])
        
        if abs(blueprint_engines - preset_engines) > 2:
            confidence *= 0.7  # Penalize large engine count mismatch
        
        # Check vibe alignment
        if blueprint.get('overall_vibe') and preset.get('vibe'):
            vibe_match = self._calculate_vibe_similarity(
                blueprint['overall_vibe'], 
                preset['vibe']
            )
            confidence = (confidence + vibe_match) / 2
        
        # Check genre match if available
        if blueprint.get('genre') and preset.get('genre'):
            if blueprint['genre'].lower() != preset['genre'].lower():
                confidence *= 0.8
        
        return confidence
    
    def _cloud_ai_search(self, blueprint: Dict, faiss_result: Dict, 
                        faiss_confidence: float) -> Dict[str, Any]:
        """Use Cloud AI to find/create better preset"""
        
        # Prepare context for AI
        context = {
            "blueprint": blueprint,
            "faiss_attempt": {
                "preset": faiss_result.get('name', 'Unknown'),
                "confidence": faiss_confidence,
                "why_insufficient": self._explain_confidence_issues(faiss_result, blueprint)
            },
            "corpus_summary": self._get_corpus_summary(),
            "requirements": {
                "max_engines": blueprint.get('max_engines', 5),
                "genre": blueprint.get('genre', 'general'),
                "vibe": blueprint.get('overall_vibe', 'neutral')
            }
        }
        
        # Call AI
        response = self.cloud_ai.find_best_preset(
            context=context,
            instruction="Find or create the best preset for this blueprint. "
                       "Explain why it's better than the FAISS match."
        )
        
        return response
    
    def _update_faiss_if_excellent(self, preset: Dict, blueprint: Dict):
        """Add excellent AI-generated presets to our corpus"""
        
        # Only add if it's really good and novel
        if preset.get('ai_confidence', 0) < 0.9:
            return
            
        # Check if similar already exists
        vector = self._preset_to_vector(preset)
        distances, _ = self.index.search(np.array([vector]), k=1)
        
        if distances[0][0] < 0.1:  # Too similar to existing
            return
        
        # Add to FAISS index
        self.index.add(np.array([vector]))
        
        # Add metadata
        new_id = f"ai_learned_{len(self.metadata)}"
        self.metadata.append({
            'id': new_id,
            'learned_from': blueprint.get('overall_vibe', 'unknown'),
            'timestamp': datetime.now().isoformat()
        })
        
        # Add preset
        self.presets[new_id] = preset
        
        logger.info(f"Added AI-learned preset to corpus: {preset.get('name', 'Unknown')}")
    
    def _blueprint_to_vector(self, blueprint: Dict) -> np.ndarray:
        """Convert blueprint to FAISS vector"""
        # Implementation depends on your vector schema
        vector = np.zeros(128)  # Example dimension
        
        # Encode various features
        # ... (your existing logic)
        
        return vector.astype('float32')
    
    def _add_metadata(self, preset: Dict, **kwargs) -> Dict:
        """Add metadata about how preset was found"""
        preset['oracle_metadata'] = kwargs
        return preset
    
    def _calculate_vibe_similarity(self, vibe1: str, vibe2: str) -> float:
        """Calculate similarity between two vibe descriptions"""
        # Simple word overlap for now
        words1 = set(vibe1.lower().split())
        words2 = set(vibe2.lower().split())
        if not words1 or not words2:
            return 0.5
        overlap = len(words1 & words2)
        total = len(words1 | words2)
        return overlap / total if total > 0 else 0.0
    
    def _explain_confidence_issues(self, result: Dict, blueprint: Dict) -> str:
        """Explain why confidence is low"""
        issues = []
        if result.get('similarity_score', 0) < 0.7:
            issues.append("Low vector similarity")
        if blueprint.get('genre') and result.get('genre'):
            if blueprint['genre'] != result['genre']:
                issues.append(f"Genre mismatch: wanted {blueprint['genre']}, got {result['genre']}")
        return "; ".join(issues) if issues else "General mismatch"
    
    def _get_corpus_summary(self) -> Dict:
        """Get summary of corpus for AI context"""
        return {
            "total_presets": len(self.presets),
            "genres": list(set(p.get('genre', 'unknown') for p in self.presets.values())),
            "typical_engines": [1, 7, 15, 31, 41]  # Most common
        }
    
    def _preset_to_vector(self, preset: Dict) -> np.ndarray:
        """Convert preset to FAISS vector"""
        # This is a simplified version - real one would be more sophisticated
        vector = np.zeros(128)
        params = preset.get('parameters', {})
        
        # Encode engines used
        for i in range(6):
            key = f"slot{i+1}_engine"
            if key in params:
                engine_id = params[key]
                if engine_id > 0:
                    vector[engine_id % 128] = 1.0
        
        return vector.astype('float32')
    
    def get_stats(self) -> Dict:
        """Get performance statistics"""
        total = max(self.stats["total_requests"], 1)
        return {
            **self.stats,
            "cache_hit_rate": self.stats["cache_hits"] / total,
            "faiss_hit_rate": self.stats["faiss_hits"] / total,
            "ai_escalation_rate": self.stats["ai_escalations"] / total
        }


class SemanticCache:
    """
    Intelligent cache that learns from AI responses
    Uses semantic hashing for fuzzy matching
    """
    
    def __init__(self, cache_dir: str = "oracle_cache"):
        self.cache_dir = Path(cache_dir)
        self.cache_dir.mkdir(exist_ok=True)
        
        # In-memory cache for speed
        self.memory_cache = {}
        self.max_memory_items = 1000
        
        # Load persistent cache
        self.load_cache()
    
    def get(self, key: str) -> Optional[Dict]:
        """Get from cache if exists and not stale"""
        
        # Check memory first
        if key in self.memory_cache:
            entry = self.memory_cache[key]
            if not self._is_stale(entry):
                return entry['preset']
        
        # Check disk
        cache_file = self.cache_dir / f"{key}.json"
        if cache_file.exists():
            with open(cache_file, 'r') as f:
                entry = json.load(f)
                if not self._is_stale(entry):
                    # Promote to memory
                    self._add_to_memory(key, entry)
                    return entry['preset']
        
        return None
    
    def add(self, key: str, preset: Dict):
        """Add to cache"""
        entry = {
            'preset': preset,
            'timestamp': time.time(),
            'hits': 0
        }
        
        # Add to memory
        self._add_to_memory(key, entry)
        
        # Persist to disk
        cache_file = self.cache_dir / f"{key}.json"
        with open(cache_file, 'w') as f:
            json.dump(entry, f)
    
    def _is_stale(self, entry: Dict) -> bool:
        """Check if cache entry is stale"""
        # Cache valid for 7 days
        age = time.time() - entry['timestamp']
        return age > (7 * 24 * 60 * 60)
    
    def _add_to_memory(self, key: str, entry: Dict):
        """Add to memory cache with LRU eviction"""
        if len(self.memory_cache) >= self.max_memory_items:
            # Evict least recently used
            lru_key = min(self.memory_cache.keys(), 
                         key=lambda k: self.memory_cache[k].get('last_access', 0))
            del self.memory_cache[lru_key]
        
        entry['last_access'] = time.time()
        self.memory_cache[key] = entry
    
    def load_cache(self):
        """Load cache from disk on startup"""
        for cache_file in self.cache_dir.glob("*.json"):
            try:
                with open(cache_file, 'r') as f:
                    entry = json.load(f)
                    if not self._is_stale(entry):
                        key = cache_file.stem
                        self.memory_cache[key] = entry
                        if len(self.memory_cache) >= self.max_memory_items:
                            break
            except:
                continue


class CloudAI:
    """Cloud AI interface for Oracle"""
    
    def __init__(self):
        self.context = self._load_context()
    
    def _load_context(self) -> str:
        """Load comprehensive context for AI"""
        context_file = Path("trinity_context.md")
        if context_file.exists():
            return context_file.read_text()
        return ""
    
    def find_best_preset(self, context: Dict, instruction: str) -> Dict:
        """Call cloud AI to find/create preset"""
        
        import requests
        
        prompt = f"""
{self.context}

Current Situation:
{json.dumps(context, indent=2)}

Task: {instruction}

Return a preset in this exact format:
{{
    "name": "Creative Preset Name",
    "parameters": {{
        "slot1_engine": <engine_id>,
        "slot1_param0": <0.0-1.0>,
        ...
    }},
    "explanation": "Why this preset matches the blueprint",
    "ai_confidence": <0.0-1.0>
}}
"""
        
        # Call your cloud AI service
        response = requests.post(
            "https://api.openai.com/v1/chat/completions",
            json={
                "model": "gpt-4",
                "messages": [{"role": "user", "content": prompt}],
                "temperature": 0.7
            },
            headers={
                "Authorization": f"Bearer {os.getenv('OPENAI_API_KEY')}"
            }
        )
        
        # Parse and return
        return self._parse_response(response.json())
    
    def _parse_response(self, response: Dict) -> Dict:
        """Parse AI response into preset format"""
        # Implementation depends on your AI service
        pass


def _generate_cache_key(blueprint: Dict) -> str:
    """Generate semantic cache key from blueprint"""
    # Create semantic hash that similar blueprints share
    key_parts = [
        blueprint.get('overall_vibe', ''),
        str(blueprint.get('max_engines', 5)),
        blueprint.get('genre', ''),
        # Include key engines
        ','.join(str(s['engine_id']) for s in blueprint.get('slots', [])[:3])
    ]
    
    key_string = '|'.join(key_parts).lower()
    return hashlib.md5(key_string.encode()).hexdigest()[:16]