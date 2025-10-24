#!/usr/bin/env python3
"""
EXAMPLE: How to integrate Intelligent Fallback into Trinity Pipeline

This shows the 3 integration points:
1. Visionary - Replace terrible fallback
2. Calculator - Add fallback for empty presets
3. Main Pipeline - Graceful degradation when AI fails
"""

import logging
from typing import Dict, Any, Optional

logger = logging.getLogger(__name__)


# ============================================================================
# INTEGRATION 1: REPLACE VISIONARY FALLBACK
# ============================================================================

class VisionaryWithSmartFallback:
    """
    Example: Visionary that uses intelligent fallback instead of terrible one
    """

    def __init__(self):
        from intelligent_fallback_system import IntelligentFallbackGenerator
        self.fallback_gen = IntelligentFallbackGenerator()

    async def generate_complete_preset(self, prompt: str) -> Dict[str, Any]:
        """Generate preset with AI, fallback to intelligent system if needed"""

        try:
            # Try GPT/AI first
            logger.info("Attempting AI generation...")
            preset = await self._call_gpt(prompt)
            return preset

        except Exception as e:
            logger.warning(f"AI generation failed: {e}")
            logger.info("Using INTELLIGENT fallback (not terrible EQ+Reverb)")

            # Use intelligent fallback instead of old terrible one
            preset = self.fallback_gen.generate_preset(prompt, num_slots=6)

            logger.info(f"✅ Fallback generated: {preset['name']}")
            logger.info(f"   Engines: {[s['engine_name'] for s in preset['slots'] if s['engine_id'] != 0]}")

            return preset

    async def _call_gpt(self, prompt: str):
        """Placeholder for actual GPT call"""
        # Your actual AI implementation here
        raise NotImplementedError("Replace with real GPT call")


# ============================================================================
# INTEGRATION 2: CALCULATOR FALLBACK FOR EMPTY PRESETS
# ============================================================================

class CalculatorWithSmartFallback:
    """
    Example: Calculator that uses intelligent fallback for empty/failed presets
    """

    def __init__(self):
        from intelligent_fallback_system import IntelligentFallbackGenerator
        self.fallback_gen = IntelligentFallbackGenerator()

    def optimize_preset(self, preset: Dict, prompt: str) -> Dict:
        """Optimize preset, use intelligent fallback if empty/broken"""

        # Check if preset is empty or broken
        if not preset or not preset.get("slots"):
            logger.warning("Empty preset received, using intelligent fallback")
            return self.fallback_gen.generate_preset(prompt, num_slots=6)

        # Check if all slots are empty
        active_slots = [s for s in preset.get("slots", []) if s.get("engine_id", 0) != 0]
        if len(active_slots) == 0:
            logger.warning("All slots empty, using intelligent fallback")
            return self.fallback_gen.generate_preset(prompt, num_slots=6)

        try:
            # Try AI optimization
            optimized = self._optimize_with_ai(preset, prompt)
            return optimized

        except Exception as e:
            logger.warning(f"AI optimization failed: {e}")

            # Check if we have at least some engines
            if len(active_slots) >= 2:
                # Return original if it has content
                logger.info("Returning original preset (has content)")
                return preset
            else:
                # Use intelligent fallback for better quality
                logger.info("Using intelligent fallback for quality")
                return self.fallback_gen.generate_preset(prompt, num_slots=6)

    def _optimize_with_ai(self, preset: Dict, prompt: str):
        """Placeholder for actual AI optimization"""
        # Your actual AI implementation here
        raise NotImplementedError("Replace with real AI optimization")


# ============================================================================
# INTEGRATION 3: MAIN PIPELINE GRACEFUL DEGRADATION
# ============================================================================

class TrinityPipelineWithSmartFallback:
    """
    Example: Complete Trinity Pipeline with intelligent fallback at all levels
    """

    def __init__(self):
        from intelligent_fallback_system import IntelligentFallbackGenerator
        self.fallback_gen = IntelligentFallbackGenerator()

        # Your actual components
        self.visionary = VisionaryWithSmartFallback()
        self.calculator = CalculatorWithSmartFallback()

    async def process_request(self, prompt: str) -> Dict[str, Any]:
        """
        Process user request with graceful degradation:
        Level 1: Full AI (Visionary + Calculator)
        Level 2: Visionary AI + Fallback Calculator
        Level 3: Intelligent Fallback (no AI)
        """

        logger.info(f"Processing request: '{prompt}'")

        # LEVEL 1: Try full AI pipeline
        try:
            # Visionary generates initial preset
            preset = await self.visionary.generate_complete_preset(prompt)

            # Calculator optimizes it
            optimized = self.calculator.optimize_preset(preset, prompt)

            logger.info("✅ Full AI pipeline success")
            return optimized

        except Exception as e:
            logger.warning(f"Full AI pipeline failed: {e}")

        # LEVEL 2: Try Visionary only (skip Calculator)
        try:
            preset = await self.visionary.generate_complete_preset(prompt)

            logger.info("✅ Visionary AI success (no Calculator)")
            return preset

        except Exception as e:
            logger.warning(f"Visionary AI failed: {e}")

        # LEVEL 3: Use intelligent fallback (no AI at all)
        logger.info("Using INTELLIGENT FALLBACK (no AI)")
        preset = self.fallback_gen.generate_preset(prompt, num_slots=6)

        logger.info(f"✅ Fallback success: {preset['name']}")
        return preset


# ============================================================================
# INTEGRATION 4: SIMPLE DROP-IN REPLACEMENT
# ============================================================================

def simple_integration_example(prompt: str) -> Dict[str, Any]:
    """
    SIMPLEST integration: Just replace the old fallback

    OLD CODE:
        def create_intelligent_fallback(self, prompt: str):
            # Returns EQ + Reverb ALWAYS

    NEW CODE:
        from intelligent_fallback_system import IntelligentFallbackGenerator
        fallback_gen = IntelligentFallbackGenerator()
        return fallback_gen.generate_preset(prompt)
    """

    from intelligent_fallback_system import IntelligentFallbackGenerator

    fallback_gen = IntelligentFallbackGenerator()
    preset = fallback_gen.generate_preset(prompt, num_slots=6)

    return preset


# ============================================================================
# INTEGRATION 5: WITH CACHING FOR PERFORMANCE
# ============================================================================

class CachedSmartFallback:
    """
    Example: Cache fallback presets for repeated prompts
    Reduces 20ms → <1ms for cached results
    """

    def __init__(self):
        from intelligent_fallback_system import IntelligentFallbackGenerator
        self.fallback_gen = IntelligentFallbackGenerator()
        self.cache = {}  # prompt → preset

    def generate_preset(self, prompt: str, num_slots: int = 6) -> Dict[str, Any]:
        """Generate with caching"""

        # Normalize prompt for cache key
        cache_key = prompt.lower().strip()

        # Check cache
        if cache_key in self.cache:
            logger.info(f"✅ Cache hit for '{prompt}'")
            return self.cache[cache_key].copy()

        # Generate
        preset = self.fallback_gen.generate_preset(prompt, num_slots)

        # Cache it
        self.cache[cache_key] = preset

        logger.info(f"✅ Generated and cached '{prompt}'")
        return preset


# ============================================================================
# INTEGRATION 6: HYBRID AI + FALLBACK
# ============================================================================

class HybridAIFallback:
    """
    Example: Use fallback for simple requests, AI for complex ones
    Best of both worlds: Speed + Quality
    """

    def __init__(self):
        from intelligent_fallback_system import IntelligentFallbackGenerator
        self.fallback_gen = IntelligentFallbackGenerator()

    async def generate_preset(self, prompt: str) -> Dict[str, Any]:
        """
        Smart routing:
        - Simple requests → Fallback (fast, accurate)
        - Complex requests → AI (creative, nuanced)
        """

        # Detect if request is simple
        if self._is_simple_request(prompt):
            logger.info(f"Simple request detected, using FAST fallback")
            return self.fallback_gen.generate_preset(prompt, num_slots=6)

        # Complex request - use AI
        logger.info(f"Complex request, using AI")
        try:
            return await self._call_ai(prompt)
        except Exception as e:
            logger.warning(f"AI failed: {e}, using fallback")
            return self.fallback_gen.generate_preset(prompt, num_slots=6)

    def _is_simple_request(self, prompt: str) -> bool:
        """Detect if request is simple enough for fallback"""

        simple_patterns = [
            # Exact engine requests
            r'\bbit\s*crush',
            r'\bplate\s*reverb',
            r'\bshimmer',
            r'\bchorus',

            # Simple characters
            r'^\s*(harsh|dark|ethereal|clean)\s+\w+\s*$',

            # Short prompts (likely simple)
            r'^\s*\w{1,20}\s*$',
        ]

        import re
        for pattern in simple_patterns:
            if re.search(pattern, prompt, re.IGNORECASE):
                return True

        # Short prompts are usually simple
        if len(prompt.split()) <= 3:
            return True

        return False

    async def _call_ai(self, prompt: str):
        """Placeholder for AI call"""
        raise NotImplementedError("Replace with real AI")


# ============================================================================
# TESTING EXAMPLES
# ============================================================================

async def test_integrations():
    """Test all integration examples"""

    print("=" * 80)
    print("INTELLIGENT FALLBACK INTEGRATION EXAMPLES")
    print("=" * 80)

    # Test 1: Simple drop-in replacement
    print("\n1. SIMPLE DROP-IN REPLACEMENT")
    print("-" * 40)
    preset = simple_integration_example("bit crusher")
    print(f"Result: {preset['name']}")
    print(f"Engines: {[s['engine_name'] for s in preset['slots'] if s['engine_id'] != 0]}")

    # Test 2: Cached fallback
    print("\n2. CACHED FALLBACK (performance)")
    print("-" * 40)
    cached = CachedSmartFallback()

    import time

    start = time.time()
    preset1 = cached.generate_preset("harsh distortion")
    time1 = (time.time() - start) * 1000
    print(f"First call: {time1:.1f}ms")

    start = time.time()
    preset2 = cached.generate_preset("harsh distortion")
    time2 = (time.time() - start) * 1000
    print(f"Cached call: {time2:.1f}ms (speedup: {time1/time2:.1f}x)")

    # Test 3: Hybrid routing
    print("\n3. HYBRID AI + FALLBACK (smart routing)")
    print("-" * 40)
    hybrid = HybridAIFallback()

    simple_prompts = ["bit crusher", "plate reverb", "harsh"]
    for prompt in simple_prompts:
        is_simple = hybrid._is_simple_request(prompt)
        print(f"'{prompt}' → {'FALLBACK (fast)' if is_simple else 'AI (quality)'}")

    print("\n" + "=" * 80)
    print("✅ Integration examples complete!")
    print("=" * 80)


if __name__ == "__main__":
    import asyncio
    asyncio.run(test_integrations())
