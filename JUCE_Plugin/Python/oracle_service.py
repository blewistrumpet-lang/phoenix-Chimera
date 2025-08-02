#!/usr/bin/env python3
"""
Oracle Service - REST API for preset search using FAISS
Part of the Trinity AI pipeline for Chimera Phoenix
"""

from flask import Flask, request, jsonify
import numpy as np
import uuid
import os
import logging
from typing import Dict, List
from oracle_faiss_indexer import GoldenCorpusIndex, PresetVector

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Initialize Flask app
app = Flask(__name__)

# Global Oracle instance
oracle = None

class OracleService:
    """Oracle component using FAISS search"""
    
    def __init__(self, index_path: str):
        self.index = GoldenCorpusIndex()
        self.index.load_index(index_path)
        self.search_history = []
        self.feedback_scores = {}
        
    def encode_recipe(self, recipe: Dict) -> np.ndarray:
        """Convert Visionary's recipe to searchable vector"""
        vector = np.zeros(76, dtype=np.float32)
        idx = 0
        
        # Sonic goals from recipe
        sonic = recipe.get('sonic_goals', {})
        vector[idx:idx+6] = [
            sonic.get('brightness', 0.5),
            sonic.get('density', 0.5),
            sonic.get('movement', 0.5),
            sonic.get('space', 0.5),
            sonic.get('aggression', 0.5),
            sonic.get('vintage', 0.5)
        ]
        idx += 6
        
        # Emotional targets
        emotional = recipe.get('emotional_targets', {})
        vector[idx:idx+5] = [
            emotional.get('energy', 0.5),
            emotional.get('mood', 0.5),
            emotional.get('tension', 0.5),
            emotional.get('organic', 0.5),
            emotional.get('nostalgia', 0.5)
        ]
        idx += 5
        
        # Suggested engines (50 dims)
        for i in range(50):
            vector[idx + i] = 0.0
        suggested_engines = recipe.get('suggested_engines', [])
        for engine_id in suggested_engines:
            if 0 <= engine_id < 50:
                vector[idx + engine_id] = 0.3  # Soft preference
        idx += 50
        
        # Skip slot usage (6 dims) - zeros
        idx += 6
        
        # Technical hints (4 dims)
        hints = recipe.get('technical_hints', {})
        vector[idx:idx+4] = [
            hints.get('complexity', 0.5),
            hints.get('experimentalness', 0.5),
            hints.get('engine_count', 3) / 6.0,
            hints.get('cpu_budget', 0.5)
        ]
        idx += 4
        
        # Source context (5 dims)
        context = recipe.get('context', {})
        source_type = context.get('source_type', 'mix')
        source_affinities = {
            'vocals': 1.0 if source_type == 'vocals' else 0.0,
            'guitar': 1.0 if source_type == 'guitar' else 0.0,
            'drums': 1.0 if source_type == 'drums' else 0.0,
            'synth': 1.0 if source_type == 'synth' else 0.0,
            'mix': 1.0 if source_type == 'mix' else 0.5
        }
        vector[idx:idx+5] = [
            source_affinities['vocals'],
            source_affinities['guitar'],
            source_affinities['drums'],
            source_affinities['synth'],
            source_affinities['mix']
        ]
        
        return vector
    
    def calculate_final_score(self, preset: Dict, recipe: Dict, similarity: float) -> float:
        """Enhanced scoring with keyword matching and penalties"""
        score = 1.0 / (1.0 + similarity)  # Convert distance to similarity score
        
        # Keyword bonus
        preset_keywords = set(preset.get('keywords', []))
        recipe_keywords = set(recipe.get('keywords', []))
        keyword_overlap = len(preset_keywords & recipe_keywords)
        score += keyword_overlap * 0.1
        
        # Anti-feature penalty
        anti_features = set(recipe.get('anti_features', []))
        if preset_keywords & anti_features:
            score *= 0.5
        
        # Engine match bonus
        preset_engines = set(e['type'] for e in preset.get('engines', []))
        suggested_engines = set(recipe.get('suggested_engines', []))
        engine_overlap = len(preset_engines & suggested_engines)
        score += engine_overlap * 0.05
        
        # Complexity match
        complexity_diff = abs(preset.get('complexity', 0.5) - 
                            recipe.get('technical_hints', {}).get('complexity', 0.5))
        score -= complexity_diff * 0.2
        
        # Apply feedback if available
        preset_id = preset.get('id')
        if preset_id in self.feedback_scores:
            score *= (1.0 + self.feedback_scores[preset_id])
        
        return max(0.0, min(1.0, score))
    
    def search_presets(self, recipe: Dict, k: int = 3) -> List[Dict]:
        """Find k most relevant presets for given recipe"""
        # Encode recipe to vector
        query_vector = self.encode_recipe(recipe)
        
        # Search using FAISS
        search_results = self.index.search(query_vector, k=k*2)  # Get extra for filtering
        
        # Score and filter results
        scored_results = []
        for preset_id, distance in search_results:
            preset = self.index.get_preset(preset_id)
            if preset:
                score = self.calculate_final_score(preset, recipe, distance)
                scored_results.append({
                    'preset_id': preset_id,
                    'preset': preset,
                    'distance': distance,
                    'score': score,
                    'confidence': score  # For compatibility
                })
        
        # Sort by final score and return top k
        scored_results.sort(key=lambda x: x['score'], reverse=True)
        return scored_results[:k]
    
    def record_selection(self, search_id: str, selected_preset_id: str):
        """Track which presets users actually choose"""
        self.search_history.append({
            'search_id': search_id,
            'selected': selected_preset_id,
            'timestamp': np.datetime64('now')
        })
    
    def update_feedback(self, preset_id: str, rating: float):
        """Update preset scores based on user feedback"""
        # Rating: -1.0 (dislike) to 1.0 (love)
        if preset_id not in self.feedback_scores:
            self.feedback_scores[preset_id] = 0.0
        
        # Exponential moving average
        alpha = 0.3
        self.feedback_scores[preset_id] = (
            alpha * rating + (1 - alpha) * self.feedback_scores[preset_id]
        )


# API Routes

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'service': 'Oracle',
        'presets_loaded': oracle.index.index.ntotal if oracle else 0
    })


@app.route('/search', methods=['POST'])
def search_presets():
    """Main search endpoint - receives recipe from Visionary"""
    try:
        data = request.json
        recipe = data.get('recipe', {})
        k = data.get('k', 3)
        
        # Perform search
        results = oracle.search_presets(recipe, k)
        
        # Generate search ID for tracking
        search_id = str(uuid.uuid4())
        
        # Format response
        response = {
            'search_id': search_id,
            'recipe_id': recipe.get('recipe_id'),
            'results': [
                {
                    'preset_id': r['preset_id'],
                    'preset_name': r['preset']['name'],
                    'category': r['preset']['category'],
                    'score': r['score'],
                    'confidence': r['confidence'],
                    'distance': r['distance']
                }
                for r in results
            ],
            'preset_data': {
                r['preset_id']: r['preset'] for r in results
            }
        }
        
        logger.info(f"Search {search_id}: Found {len(results)} presets")
        
        return jsonify(response)
        
    except Exception as e:
        logger.error(f"Search error: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/preset/<preset_id>', methods=['GET'])
def get_preset(preset_id):
    """Get full preset details"""
    try:
        preset = oracle.index.get_preset(preset_id)
        if preset:
            return jsonify(preset)
        else:
            return jsonify({'error': 'Preset not found'}), 404
    except Exception as e:
        logger.error(f"Get preset error: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/feedback', methods=['POST'])
def record_feedback():
    """Record user feedback for learning"""
    try:
        data = request.json
        preset_id = data.get('preset_id')
        rating = data.get('rating', 0.0)  # -1.0 to 1.0
        search_id = data.get('search_id')
        
        # Update feedback score
        oracle.update_feedback(preset_id, rating)
        
        # Track selection if positive
        if rating > 0 and search_id:
            oracle.record_selection(search_id, preset_id)
        
        return jsonify({
            'status': 'recorded',
            'preset_id': preset_id,
            'new_score': oracle.feedback_scores.get(preset_id, 0.0)
        })
        
    except Exception as e:
        logger.error(f"Feedback error: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/stats', methods=['GET'])
def get_statistics():
    """Get Oracle statistics"""
    try:
        stats = {
            'total_presets': oracle.index.index.ntotal,
            'total_searches': len(oracle.search_history),
            'presets_with_feedback': len(oracle.feedback_scores),
            'categories': {}
        }
        
        # Count presets by category
        for preset_id in oracle.index.preset_ids:
            preset = oracle.index.get_preset(preset_id)
            cat = preset.get('category', 'Unknown')
            stats['categories'][cat] = stats['categories'].get(cat, 0) + 1
        
        return jsonify(stats)
        
    except Exception as e:
        logger.error(f"Stats error: {e}")
        return jsonify({'error': str(e)}), 500


# Test endpoint for development
@app.route('/test', methods=['GET'])
def test_search():
    """Test search with a sample recipe"""
    test_recipe = {
        'recipe_id': 'test_001',
        'sonic_goals': {
            'brightness': 0.7,
            'density': 0.4,
            'movement': 0.3,
            'space': 0.8,
            'aggression': 0.1,
            'vintage': 0.6
        },
        'emotional_targets': {
            'energy': 0.4,
            'mood': 0.7,
            'tension': 0.2,
            'organic': 0.7,
            'nostalgia': 0.6
        },
        'keywords': ['warm', 'spacious', 'vintage'],
        'context': {
            'source_type': 'vocals'
        },
        'technical_hints': {
            'complexity': 0.3,
            'experimentalness': 0.2,
            'engine_count': 2,
            'cpu_budget': 0.3
        }
    }
    
    results = oracle.search_presets(test_recipe, k=5)
    
    return jsonify({
        'test_recipe': test_recipe,
        'results': [
            {
                'preset_id': r['preset_id'],
                'name': r['preset']['name'],
                'category': r['preset']['category'],
                'score': r['score']
            }
            for r in results
        ]
    })


def initialize_oracle():
    """Initialize the Oracle service"""
    global oracle
    
    index_path = os.path.join(
        os.path.dirname(__file__),
        "../GoldenCorpus/faiss_index"
    )
    
    if not os.path.exists(index_path):
        raise ValueError(f"FAISS index not found at {index_path}. Run oracle_faiss_indexer.py first.")
    
    oracle = OracleService(index_path)
    logger.info(f"Oracle initialized with {oracle.index.index.ntotal} presets")


if __name__ == '__main__':
    # Initialize Oracle
    initialize_oracle()
    
    # Run Flask app
    logger.info("Starting Oracle service on http://localhost:5001")
    app.run(host='0.0.0.0', port=5001, debug=True)