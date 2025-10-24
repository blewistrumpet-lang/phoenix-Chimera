#!/usr/bin/env python3
"""
Hybrid Visionary - Intelligent balance of precision and artistry

This system uses a three-tier classification:
- Tier 1: Exact match (simple, fast, precise)
- Tier 2: Guided artistic (hybrid enforcement + creativity)
- Tier 3: Full artistic (maximum creative freedom)
"""

import json
import logging
import time
import re
from typing import Dict, List, Any, Optional, Tuple
from openai import AsyncOpenAI
import asyncio
import os
from dotenv import load_dotenv

load_dotenv()

logger = logging.getLogger(__name__)

# Import existing components
try:
    from preset_namer import IntelligentPresetNamer
    INTELLIGENT_NAMING_AVAILABLE = True
except ImportError:
    logger.warning("IntelligentPresetNamer not found")
    INTELLIGENT_NAMING_AVAILABLE = False


class PromptCache:
    """Simple caching system for similar prompts"""

    def __init__(self, max_age: int = 3600):
        self.cache = {}
        self.max_age = max_age

    def normalize_prompt(self, prompt: str) -> str:
        """Normalize prompt for matching"""
        # Remove extra spaces, lowercase, remove punctuation
        normalized = prompt.lower().strip()
        normalized = re.sub(r'[^\w\s]', '', normalized)
        normalized = re.sub(r'\s+', ' ', normalized)
        return normalized

    def get(self, prompt: str) -> Optional[Dict]:
        """Get cached preset if available and fresh"""
        key = self.normalize_prompt(prompt)
        if key in self.cache:
            preset, timestamp = self.cache[key]
            if time.time() - timestamp < self.max_age:
                logger.info(f"✨ Cache hit for: {prompt}")
                return preset
        return None

    def store(self, prompt: str, preset: Dict):
        """Store preset in cache"""
        key = self.normalize_prompt(prompt)
        self.cache[key] = (preset, time.time())


class HybridVisionary:
    """Intelligent hybrid system combining precision and artistry"""

    def __init__(self):
        """Initialize with knowledge base and AI clients"""

        # Load API key
        api_key = os.getenv("OPENAI_API_KEY")
        if not api_key:
            raise ValueError("OPENAI_API_KEY not found in environment")

        # Initialize OpenAI clients
        import httpx
        self.client = AsyncOpenAI(
            api_key=api_key,
            http_client=httpx.AsyncClient()
        )

        # Load knowledge base
        try:
            with open("trinity_engine_knowledge_COMPLETE.json", "r") as f:
                self.knowledge = json.load(f)
            logger.info(f"✅ Loaded knowledge base with {len(self.knowledge['engines'])} engines")
        except FileNotFoundError:
            with open("trinity_engine_knowledge.json", "r") as f:
                self.knowledge = json.load(f)
            logger.info("⚠️ Using regular knowledge base")

        self.engines = self.knowledge["engines"]
        self.selection_rules = self.knowledge.get("engine_selection_rules", {})
        self.musical_contexts = self.knowledge["musical_contexts"]

        # Initialize components
        self.cache = PromptCache()

        if INTELLIGENT_NAMING_AVAILABLE:
            self.namer = IntelligentPresetNamer()
        else:
            self.namer = None

        logger.info("✅ HybridVisionary initialized")

    # ============================================================================
    # TIER CLASSIFICATION
    # ============================================================================

    def classify_tier(self, prompt: str) -> int:
        """
        Classify request into tier 1, 2, or 3

        Returns:
            1 = Exact match (simple, fast)
            2 = Guided artistic (hybrid)
            3 = Full artistic (complex)
        """
        prompt_lower = prompt.lower()
        words = prompt.split()
        word_count = len(words)

        # Get matched engines
        matched_engines = self.extract_mandatory_engines(prompt)

        # Count descriptors
        descriptors = ['warm', 'cold', 'bright', 'dark', 'aggressive', 'gentle',
                      'subtle', 'heavy', 'vintage', 'modern', 'clean', 'dirty',
                      'tight', 'loose', 'wide', 'narrow', 'deep', 'shallow']
        descriptor_count = sum(1 for d in descriptors if d in prompt_lower)

        # Tier 1: Simple exact match
        if matched_engines and word_count <= 5 and descriptor_count <= 1:
            # "spring reverb", "shimmer", "noise gate"
            return 1

        # Tier 3: Complex artistic
        poetic_words = ['heaven', 'angel', 'dream', 'mana', 'celestial', 'ethereal',
                       'cosmic', 'divine', 'mystic', 'magic']
        has_poetic = any(word in prompt_lower for word in poetic_words)

        if has_poetic or word_count > 12 or descriptor_count > 3:
            # "heavenly mana from an angel"
            # "warm vintage tape delay with subtle chorus and gentle shimmer"
            return 3

        # Tier 2: Everything else (most common)
        return 2

    def extract_mandatory_engines(self, prompt: str) -> List[int]:
        """Extract mandatory engines based on keyword matching"""
        prompt_lower = prompt.lower()
        mandatory = []

        for rule_name, rule_data in self.selection_rules.items():
            for keyword in rule_data.get("keywords", []):
                if keyword in prompt_lower:
                    engine_id = rule_data["engine_id"]
                    if engine_id not in mandatory:
                        mandatory.append(engine_id)
                    break

        return mandatory

    def determine_engine_count(self, prompt: str, tier: int) -> Tuple[int, int]:
        """
        Determine appropriate min/max engine count

        Returns:
            (min_engines, max_engines)
        """
        if tier == 1:
            # Exact match - just what they asked for
            return (1, 2)

        elif tier == 2:
            # Guided - analyze complexity
            word_count = len(prompt.split())
            descriptors = sum(1 for d in ['warm', 'bright', 'tight', 'vintage',
                                          'subtle', 'heavy', 'gentle', 'aggressive']
                            if d in prompt.lower())

            if word_count <= 5 and descriptors <= 1:
                return (2, 3)  # Simple hybrid
            else:
                return (3, 4)  # Complex hybrid

        else:  # tier == 3
            # Full artistic - allow complexity
            return (4, 6)

    def should_add_utilities(self, prompt: str, engines: List[int]) -> bool:
        """Decide if utility engines (EQ, compression) should be added"""
        prompt_lower = prompt.lower()

        # NEVER add for simple effect requests
        simple_effects = ['reverb', 'delay', 'echo', 'shimmer', 'chorus', 'phaser']
        if any(eff in prompt_lower for eff in simple_effects) and len(prompt.split()) <= 3:
            return False

        # Add for instrument processing
        if any(inst in prompt_lower for inst in ['vocal', 'guitar', 'bass', 'drums', 'piano']):
            return True

        # Add for mix/master contexts
        if any(word in prompt_lower for word in ['mix', 'master', 'bus', 'channel']):
            return True

        # Add if tonal shaping requested
        if any(word in prompt_lower for word in ['warm', 'bright', 'tight', 'punchy', 'clean']):
            return True

        return False

    # ============================================================================
    # TIER 1: EXACT MATCH
    # ============================================================================

    async def tier_1_exact_match(self, prompt: str, engines: List[int]) -> Dict:
        """
        Fast, precise execution for exact engine requests

        Target time: 3-5 seconds
        """
        logger.info(f"⚡ TIER 1: Exact match - {len(engines)} engine(s)")
        start_time = time.time()

        # Build minimal prompt
        engine_names = [self.engines[str(e)]["name"] for e in engines]

        param_prompt = f"""Calculate parameters for: "{prompt}"

Engines to use:
{chr(10).join(f"- Engine {eid}: {name}" for eid, name in zip(engines, engine_names))}

Rules:
- Each engine needs exactly 15 parameters (param1-param15)
- Values must be 0.0 - 1.0
- Mix parameters minimum 0.15 (effects must be audible)
- Set parameters appropriate to the request

Return JSON:
{{
    "slots": [
        {{
            "slot": 0,
            "engine_id": {engines[0]},
            "engine_name": "{engine_names[0]}",
            "parameters": [
                {{"name": "param1", "value": 0.5}},
                ...15 total parameters...
            ]
        }}
    ]
}}"""

        # Use fast model
        response = await self.client.chat.completions.create(
            model="gpt-4o-mini",  # Fast and cheap
            messages=[
                {"role": "system", "content": self._get_minimal_system_prompt()},
                {"role": "user", "content": param_prompt}
            ],
            response_format={"type": "json_object"},
            temperature=0.5,
            max_tokens=800
        )

        preset = json.loads(response.choices[0].message.content)

        # Generate name
        if self.namer:
            context = {"intensity": "moderate"}
            engine_info = [{"engine_id": e, "engine_name": self.engines[str(e)]["name"]}
                          for e in engines]
            preset["name"] = self.namer.generate_name(prompt, engine_info, context)
        else:
            preset["name"] = f"{engine_names[0]} Preset"

        preset["description"] = f"Preset for: {prompt}"

        # Validate and pad
        preset = self._validate_preset_format(preset)

        elapsed = time.time() - start_time
        logger.info(f"✅ Tier 1 completed in {elapsed:.2f}s")

        return preset

    # ============================================================================
    # TIER 2: GUIDED ARTISTIC
    # ============================================================================

    async def tier_2_guided_artistic(self, prompt: str, mandatory: List[int]) -> Dict:
        """
        Hybrid approach with mandatory enforcement + creative filling

        Target time: 8-12 seconds
        """
        logger.info(f"🎨 TIER 2: Guided artistic - {len(mandatory)} mandatory engine(s)")
        start_time = time.time()

        # Get context
        context = self._analyze_prompt_context(prompt)

        # Get relevant engines
        relevant = self._get_relevant_engines(prompt, context, exclude=mandatory)[:8]

        # Determine engine count
        min_engines, max_engines = self.determine_engine_count(prompt, tier=2)

        # Build focused prompt
        mandatory_info = "\n".join(
            f"- Engine {eid}: {self.engines[str(eid)]['name']} (MANDATORY)"
            for eid in mandatory
        )

        relevant_info = "\n".join(
            f"- Engine {eid}: {self.engines[str(eid)]['name']}"
            for eid in relevant[:5]
        )

        generation_prompt = f"""Create preset for: "{prompt}"

MANDATORY ENGINES (must include ALL):
{mandatory_info}

SUGGESTED ENGINES (choose {max_engines - len(mandatory)} max):
{relevant_info}

RULES:
1. MUST include all mandatory engines above
2. Total engines: {min_engines} to {max_engines}
3. Add complementary engines ONLY if they enhance the sound
4. NO utility processing (EQ/compression) unless needed for tonal shaping
5. Follow signal chain order: Gate → EQ → Dynamics → Distortion → Modulation → Delay → Reverb
6. Creative but appropriate preset name

Context:
- Musical style: {context.get('contexts', [])}
- Instrument: {context.get('instrument', 'general')}
- Intensity: {context.get('intensity', 'moderate')}

Return JSON with slots array (each engine needs 15 parameters)."""

        # Use GPT-4o (balanced speed/quality)
        response = await self.client.chat.completions.create(
            model="gpt-4o",
            messages=[
                {"role": "system", "content": self._get_focused_system_prompt()},
                {"role": "user", "content": generation_prompt}
            ],
            response_format={"type": "json_object"},
            temperature=0.7,
            max_tokens=1500
        )

        preset = json.loads(response.choices[0].message.content)

        # Validate mandatory engines were included
        actual_engines = [s["engine_id"] for s in preset.get("slots", [])
                         if s.get("engine_id", 0) != 0]

        missing = [e for e in mandatory if e not in actual_engines]
        if missing:
            logger.warning(f"⚠️ Missing mandatory engines: {missing}")
            # Add them (simplified - full implementation would be more sophisticated)
            for engine_id in missing:
                preset["slots"].append({
                    "slot": len(preset["slots"]),
                    "engine_id": engine_id,
                    "engine_name": self.engines[str(engine_id)]["name"],
                    "parameters": [{"name": f"param{i+1}", "value": 0.5} for i in range(15)]
                })

        # Validate format
        preset = self._validate_preset_format(preset)

        elapsed = time.time() - start_time
        logger.info(f"✅ Tier 2 completed in {elapsed:.2f}s")

        return preset

    # ============================================================================
    # TIER 3: FULL ARTISTIC
    # ============================================================================

    async def tier_3_full_artistic(self, prompt: str) -> Dict:
        """
        Full creative AI generation for complex artistic requests

        Target time: 10-15 seconds
        """
        logger.info(f"🌟 TIER 3: Full artistic generation")
        start_time = time.time()

        # Get context
        context = self._analyze_prompt_context(prompt)

        # Get relevant engines
        relevant = self._get_relevant_engines(prompt, context, exclude=[])[:15]

        # Build complete prompt
        engine_info = "\n".join(
            f"Engine {eid}: {self.engines[str(eid)]['name']}\n"
            f"  Category: {self.engines[str(eid)].get('category', 'Unknown')}\n"
            f"  Function: {self.engines[str(eid)].get('function', 'N/A')}"
            for eid in relevant
        )

        generation_prompt = f"""Create a unique, artistic preset for: "{prompt}"

MOST RELEVANT ENGINES:
{engine_info}

Context:
- Musical style: {context.get('contexts', [])}
- Poetic elements: {context.get('poetic_elements', [])}
- Instrument: {context.get('instrument', 'general')}
- Intensity: {context.get('intensity', 'moderate')}

RULES:
1. Use 4-6 engines total (can use engines not listed above if better)
2. Creative engine selection that captures the artistic vision
3. Follow signal chain order
4. Each engine needs exactly 15 parameters
5. Create an evocative, creative preset name (not literal prompt)
6. Mix parameters minimum 0.15 (effects must be audible)

Return JSON with slots array and creative name."""

        # Use GPT-4o with higher creativity
        response = await self.client.chat.completions.create(
            model="gpt-4o",
            messages=[
                {"role": "system", "content": self._get_complete_system_prompt()},
                {"role": "user", "content": generation_prompt}
            ],
            response_format={"type": "json_object"},
            temperature=0.8,  # Higher for creativity
            max_tokens=2000
        )

        preset = json.loads(response.choices[0].message.content)

        # Validate format
        preset = self._validate_preset_format(preset)

        elapsed = time.time() - start_time
        logger.info(f"✅ Tier 3 completed in {elapsed:.2f}s")

        return preset

    # ============================================================================
    # MAIN ENTRY POINT
    # ============================================================================

    async def generate_preset(self, prompt: str) -> Dict:
        """
        Main entry point with intelligent routing

        Returns:
            Complete preset dictionary
        """
        logger.info(f"🎯 Generating preset for: '{prompt}'")
        overall_start = time.time()

        # Check cache
        cached = self.cache.get(prompt)
        if cached:
            return cached

        # Classify tier
        tier = self.classify_tier(prompt)
        mandatory_engines = self.extract_mandatory_engines(prompt)

        logger.info(f"📊 Classification: Tier {tier}")
        if mandatory_engines:
            logger.info(f"🔒 Mandatory engines: {mandatory_engines}")

        # Route to appropriate handler
        try:
            if tier == 1:
                preset = await self.tier_1_exact_match(prompt, mandatory_engines)
            elif tier == 2:
                preset = await self.tier_2_guided_artistic(prompt, mandatory_engines)
            else:
                preset = await self.tier_3_full_artistic(prompt)

            # Cache result
            self.cache.store(prompt, preset)

            overall_elapsed = time.time() - overall_start
            logger.info(f"✅ Total generation time: {overall_elapsed:.2f}s")

            return preset

        except Exception as e:
            logger.error(f"❌ Generation failed: {e}")
            # Fallback to simpler tier
            if tier == 3:
                logger.info("⚠️ Falling back to Tier 2")
                return await self.tier_2_guided_artistic(prompt, mandatory_engines)
            elif tier == 2 and mandatory_engines:
                logger.info("⚠️ Falling back to Tier 1")
                return await self.tier_1_exact_match(prompt, mandatory_engines)
            else:
                raise

    # ============================================================================
    # HELPER METHODS
    # ============================================================================

    def _analyze_prompt_context(self, prompt: str) -> Dict[str, Any]:
        """Analyze prompt for musical context"""
        prompt_lower = prompt.lower()

        # Check contexts
        matched_contexts = []
        for context_name in self.musical_contexts.keys():
            keywords = context_name.lower().split('_')
            if any(keyword in prompt_lower for keyword in keywords):
                matched_contexts.append(context_name)

        # Detect instrument
        instruments = {
            "vocal": ["vocal", "voice"],
            "guitar": ["guitar"],
            "bass": ["bass"],
            "drums": ["drum", "percussion"],
            "synth": ["synth", "pad"],
            "master": ["master", "mix", "bus"]
        }

        instrument = None
        for inst, keywords in instruments.items():
            if any(k in prompt_lower for k in keywords):
                instrument = inst
                break

        # Determine intensity
        if any(word in prompt_lower for word in ['subtle', 'gentle', 'soft', 'light']):
            intensity = "subtle"
        elif any(word in prompt_lower for word in ['heavy', 'aggressive', 'intense', 'extreme']):
            intensity = "extreme"
        else:
            intensity = "moderate"

        # Poetic elements
        poetic = []
        poetic_words = ['heaven', 'angel', 'dream', 'mana', 'celestial', 'ethereal']
        for word in poetic_words:
            if word in prompt_lower:
                poetic.append(word)

        return {
            "contexts": matched_contexts or ["warm"],
            "instrument": instrument or "general",
            "intensity": intensity,
            "poetic_elements": poetic
        }

    def _get_relevant_engines(self, prompt: str, context: Dict, exclude: List[int]) -> List[int]:
        """Get relevant engines based on context"""
        relevant = []

        # Add from musical contexts
        for ctx in context.get("contexts", []):
            if ctx in self.musical_contexts:
                relevant.extend(self.musical_contexts[ctx].get("engines", []))

        # Remove duplicates and excluded
        relevant = [e for e in list(set(relevant)) if e not in exclude]

        return relevant

    def _validate_preset_format(self, preset: Dict) -> Dict:
        """Ensure preset has correct format"""
        if "slots" not in preset:
            preset["slots"] = []

        # Ensure exactly 6 slots
        while len(preset["slots"]) < 6:
            preset["slots"].append({
                "slot": len(preset["slots"]),
                "engine_id": 0,
                "engine_name": "None",
                "parameters": [{"name": f"param{i+1}", "value": 0.5} for i in range(15)]
            })

        # Ensure each slot has exactly 15 parameters
        for slot in preset["slots"]:
            params = slot.get("parameters", [])
            if len(params) < 15:
                params.extend([
                    {"name": f"param{i+1}", "value": 0.5}
                    for i in range(len(params), 15)
                ])
                slot["parameters"] = params[:15]

        if "name" not in preset:
            preset["name"] = "Generated Preset"
        if "description" not in preset:
            preset["description"] = "AI generated preset"

        return preset

    def _get_minimal_system_prompt(self) -> str:
        """System prompt for Tier 1"""
        return """You calculate effect parameters for the Trinity audio plugin.

SIGNAL CHAIN ORDER:
Gate → EQ → Dynamics → Distortion → Modulation → Delay → Reverb

PARAMETER RULES:
- All values 0.0 - 1.0
- Mix parameters: minimum 0.15 (effects must be audible)
- Each engine needs exactly 15 parameters
- Set values appropriate to the request

Return JSON with slots array."""

    def _get_focused_system_prompt(self) -> str:
        """System prompt for Tier 2"""
        return """You are the Visionary - creative audio preset designer for Trinity plugin.

Available: 57 audio engines (reverbs, delays, distortions, modulation, etc.)

CORE RULES:
1. User-specified engines are MANDATORY - must include ALL of them
2. Add complementary engines ONLY if they enhance the sound
3. Simpler is better - don't over-engineer
4. Follow signal chain order
5. Creative naming (not literal prompt)

SIGNAL CHAIN:
Gate → EQ → Dynamics → Distortion → Modulation → Delay → Reverb → Spatial

WHEN TO ADD UTILITIES:
- EQ: Only if tonal shaping requested ("warm", "bright")
- Compression: Only for instrument processing or "tight" requests
- Gate: Only if "tight" or noise control needed

PARAMETER RULES:
- Each engine needs exactly 15 parameters
- Mix parameters minimum 0.15 (must be audible)
- Values 0.0 - 1.0

Return JSON with slots array and creative name."""

    def _get_complete_system_prompt(self) -> str:
        """System prompt for Tier 3"""
        # For Tier 3, use more complete system prompt
        # This is simplified - full version would include more details
        return """You are the Visionary - master audio preset designer for Trinity plugin.

You have access to 57 professional audio engines across all categories:
reverbs, delays, distortions, modulation, filters, dynamics, spatial effects, etc.

CORE PRINCIPLES:
1. Create artistic, evocative presets that match the user's vision
2. Use 4-6 engines for complex, layered sounds
3. Follow proper signal chain order
4. Creative, inspiring preset names
5. Each engine needs exactly 15 parameters
6. Mix parameters minimum 0.15 (effects must be audible)

SIGNAL CHAIN ORDER:
1. Noise Gate
2. EQ/Filters
3. Dynamics (compression, limiting)
4. Distortion/Saturation
5. Modulation (chorus, phaser, flanger)
6. Pitch effects
7. Delays
8. Reverbs
9. Spatial/Stereo effects

Return JSON with slots array, creative name, and description."""


# ============================================================================
# TESTING
# ============================================================================

async def test_hybrid_visionary():
    """Test the hybrid system"""
    visionary = HybridVisionary()

    test_cases = [
        ("spring reverb", 1),  # Tier 1: exact match
        ("warm spring reverb", 2),  # Tier 2: guided
        ("warm vintage tape delay with subtle chorus", 2),  # Tier 2: complex hybrid
        ("heavenly mana from an angel", 3),  # Tier 3: full artistic
    ]

    print("\n" + "="*80)
    print("TESTING HYBRID VISIONARY SYSTEM")
    print("="*80)

    for prompt, expected_tier in test_cases:
        print(f"\n{'='*80}")
        print(f"Prompt: {prompt}")
        print(f"Expected Tier: {expected_tier}")
        print('-'*80)

        start = time.time()

        # Test classification
        actual_tier = visionary.classify_tier(prompt)
        print(f"Classified as: Tier {actual_tier} {'✓' if actual_tier == expected_tier else '✗'}")

        # Test generation
        try:
            preset = await visionary.generate_preset(prompt)
            elapsed = time.time() - start

            # Count active engines
            active_engines = sum(1 for s in preset["slots"] if s.get("engine_id", 0) != 0)

            print(f"\n✅ SUCCESS")
            print(f"Name: {preset.get('name', 'N/A')}")
            print(f"Engines: {active_engines}")
            print(f"Time: {elapsed:.2f}s")

            # Show engines
            for slot in preset["slots"]:
                if slot.get("engine_id", 0) != 0:
                    print(f"  - Slot {slot['slot']}: Engine {slot['engine_id']} ({slot.get('engine_name', 'Unknown')})")

        except Exception as e:
            print(f"\n❌ FAILED: {e}")
            import traceback
            traceback.print_exc()

    print("\n" + "="*80)


if __name__ == "__main__":
    import asyncio
    asyncio.run(test_hybrid_visionary())
