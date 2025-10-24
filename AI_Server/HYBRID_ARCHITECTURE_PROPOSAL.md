# Hybrid Visionary Architecture Proposal
**Balancing Precision, Artistry, and Performance**

---

## Executive Summary

The ideal system combines **keyword-based mandatory enforcement** (visionary_enforcer.py) with **full artistic AI generation** (visionary_complete.py) through a **three-tier classification system** that determines the appropriate level of AI creativity vs. technical precision based on prompt analysis.

**Key Performance Target:** Reduce 20-30s generation time to **8-12 seconds** through:
- Parallel API calls where possible
- Smarter prompt engineering (less verbose)
- Strategic use of GPT-4o-mini for certain operations
- Caching and pre-computation

---

## Current System Analysis

### visionary_enforcer.py (Keyword-Based Mandatory)
**Strengths:**
- ✅ Guarantees exact engine matches ("spring reverb" → Engine 40)
- ✅ Predictable, reliable for specific requests
- ✅ Fast keyword detection (no API call needed for matching)

**Weaknesses:**
- ❌ Over-constrains creative requests
- ❌ Forces minimum 4 engines even when 1-2 would be better
- ❌ Can't interpret artistic/poetic language
- ❌ Adds default engines (EQ+Compressor) unnecessarily

### visionary_complete.py (Full Artistic AI)
**Strengths:**
- ✅ Handles artistic prompts ("warm vintage tape delay with subtle chorus")
- ✅ Creative engine selection and naming
- ✅ Understands musical context and poetic language
- ✅ Provides detailed reasoning

**Weaknesses:**
- ❌ May ignore specific engine requests
- ❌ 20-30 second generation time (too slow)
- ❌ Over-engineers simple requests (always 4+ engines)
- ❌ Uses expensive GPT-4 for everything

---

## The Hybrid Solution: Three-Tier Classification

### TIER 1: EXACT MATCH (Direct Execution)
**When:** User specifies exact engine(s) by name
**Examples:**
- "spring reverb"
- "shimmer reverb and plate reverb"
- "noise gate with compression"

**Processing:**
1. Keyword match from `engine_selection_rules`
2. NO AI generation for engine selection
3. GPT-4o-mini ONLY for parameter calculation (fast, cheap)
4. Return 1-2 engines (ONLY what was requested)

**Target Time:** 3-5 seconds

```python
def classify_tier_1(prompt: str) -> Optional[List[int]]:
    """Returns engine IDs if exact match found, None otherwise"""
    matched_engines = []
    prompt_lower = prompt.lower()

    for rule_name, rule_data in selection_rules.items():
        for keyword in rule_data["keywords"]:
            if keyword in prompt_lower:
                matched_engines.append(rule_data["engine_id"])
                break

    # TIER 1 only if we found exact matches AND prompt is simple
    if matched_engines and is_simple_request(prompt):
        return matched_engines
    return None
```

### TIER 2: GUIDED ARTISTIC (Hybrid Approach)
**When:** Complex artistic request with some specifics
**Examples:**
- "warm vintage tape delay with subtle chorus"
- "aggressive metal guitar with tight gate"
- "ambient pad with shimmer and spring reverb"

**Processing:**
1. Extract mandatory engines via keyword matching
2. Use GPT-4o (NOT GPT-4) for creative filling
3. Limit to 2-4 engines total (avoid over-engineering)
4. Faster prompt (less verbose system message)

**Target Time:** 8-12 seconds

```python
async def tier_2_generation(prompt: str, mandatory_engines: List[int]) -> Dict:
    """Hybrid: enforce specific engines, let AI fill creatively"""

    # Build CONCISE prompt
    prompt_text = f"""Create preset for: "{prompt}"

MANDATORY ENGINES (user specified):
{format_mandatory_engines(mandatory_engines)}

RULES:
1. MUST include all mandatory engines above
2. Add 1-2 complementary engines (max 4 total)
3. NO unnecessary EQ/compression unless requested
4. Simple requests = simple solutions

Return JSON with slots array."""

    # Use GPT-4o (faster than GPT-4)
    response = await client.chat.completions.create(
        model="gpt-4o",  # 2-3x faster than GPT-4
        messages=[
            {"role": "system", "content": CONCISE_SYSTEM_PROMPT},  # 50% shorter
            {"role": "user", "content": prompt_text}
        ],
        response_format={"type": "json_object"},
        temperature=0.7,
        max_tokens=1500  # Reduced from 2500
    )
```

### TIER 3: FULL ARTISTIC (Maximum Creativity)
**When:** Poetic, complex, or multi-characteristic requests
**Examples:**
- "heavenly mana from an angel"
- "warm ambient pad with subtle tape saturation, gentle chorus, and expansive shimmer reverb"
- "create a soundscape that feels like floating through clouds"

**Processing:**
1. Full AI creative control
2. GPT-4o for engine selection + parameters
3. Allow 4-5 engines for complex soundscapes
4. Use intelligent naming system

**Target Time:** 10-15 seconds

---

## Prompt Engineering Optimization

### Current Problem
The system prompt is **MASSIVE** (includes full engine catalog, all categories, detailed descriptions). This:
- Increases token count (slower, more expensive)
- Overwhelms the AI with information
- Doesn't help if prompt is simple

### Solution: Context-Aware Prompts

```python
def build_smart_prompt(prompt: str, tier: int, mandatory: List[int]) -> str:
    """Build context-appropriate prompt based on tier"""

    if tier == 1:
        # MINIMAL - just parameter guidance
        return f"""Set parameters for these engines: {mandatory}
Prompt: "{prompt}"
Return JSON with parameter values."""

    elif tier == 2:
        # FOCUSED - only relevant engines
        relevant = get_relevant_engines(prompt, mandatory)  # 10-15 engines max
        return f"""Preset: "{prompt}"

MUST USE: {format_engines(mandatory)}
SUGGESTED: {format_engines(relevant[:5])}  # Top 5 only

Rules: Max 4 engines, include all mandatory, creative names.
Return JSON."""

    else:  # tier == 3
        # FULL - current complete system
        return build_complete_generation_prompt(prompt)
```

**Token Reduction:**
- Tier 1: ~200 tokens (vs 2000 current)
- Tier 2: ~600 tokens (vs 2000 current)
- Tier 3: ~2000 tokens (same as current)

---

## Engine Count Philosophy

### Current Problem
- System ALWAYS requires minimum 4 engines
- Adds EQ+Compressor by default
- User requests "spring reverb" → gets 4-6 engines

### New Philosophy: Simplicity First

```python
def determine_engine_count(prompt: str, tier: int) -> tuple[int, int]:
    """Returns (min_engines, max_engines) based on request complexity"""

    if tier == 1:
        # Exact match - give them exactly what they asked for
        return (1, 2)

    elif tier == 2:
        # Guided - moderate complexity
        word_count = len(prompt.split())
        descriptor_count = count_descriptors(prompt)  # "warm", "subtle", etc.

        if word_count <= 5 and descriptor_count <= 1:
            return (2, 3)  # Simple hybrid
        else:
            return (3, 4)  # Complex hybrid

    else:  # tier == 3
        # Full artistic - allow complexity
        return (4, 6)
```

**Examples:**
- "spring reverb" → Tier 1 → 1 engine (just Engine 40)
- "warm spring reverb" → Tier 2 → 2 engines (Engine 40 + vintage saturation)
- "ambient pad with shimmer and spring" → Tier 2 → 3-4 engines
- "heavenly ethereal soundscape" → Tier 3 → 4-5 engines

---

## Performance Optimization Strategy

### 1. Model Selection (2-3x speedup)

```python
MODEL_STRATEGY = {
    "tier_1_params": "gpt-4o-mini",      # $0.15/1M tokens, 2-3s response
    "tier_2_creative": "gpt-4o",         # $2.50/1M tokens, 5-8s response
    "tier_3_artistic": "gpt-4o",         # Full capability, 8-12s response
    "naming": "gpt-4o-mini"              # Fast, cheap naming
}
```

**Why GPT-4o instead of GPT-4:**
- GPT-4 (current): 20-30 second response times
- GPT-4o: 8-12 second response times (same quality)
- GPT-4o-mini: 2-4 second response times (good for simple tasks)

### 2. Parallel Processing

```python
async def generate_preset_optimized(prompt: str) -> Dict:
    """Parallel AI calls where possible"""

    tier = classify_tier(prompt)

    if tier >= 2:
        # Run these in parallel
        tasks = [
            generate_engines_and_params(prompt, tier),
            generate_preset_name(prompt, tier),  # Don't wait for engine selection
        ]

        results = await asyncio.gather(*tasks)
        preset_data, name = results
        preset_data["name"] = name

        return preset_data
```

### 3. Response Streaming (Future Enhancement)

```python
# Stream preset as it's generated
async def stream_preset_generation(prompt: str):
    """Stream preset components as they're ready"""

    # 1. Send name immediately (1-2s)
    yield {"stage": "name", "data": await quick_name_generation(prompt)}

    # 2. Send engine selection (3-5s)
    yield {"stage": "engines", "data": await select_engines(prompt)}

    # 3. Send parameters (5-8s)
    yield {"stage": "parameters", "data": await calculate_parameters(prompt)}
```

### 4. Smart Caching

```python
class PromptCache:
    """Cache similar prompts to avoid redundant AI calls"""

    def __init__(self):
        self.cache = {}  # prompt_hash → preset
        self.max_age = 3600  # 1 hour

    def get_similar(self, prompt: str) -> Optional[Dict]:
        """Find cached preset for similar prompt"""
        # Use fuzzy matching for variations
        # "spring reverb" and "spring reverb effect" → same cache hit
        normalized = normalize_prompt(prompt)
        if normalized in self.cache:
            cached, timestamp = self.cache[normalized]
            if time.time() - timestamp < self.max_age:
                logger.info(f"Cache hit for: {prompt}")
                return cached
        return None
```

---

## Musical Intent Preservation

### Problem: Over-Engineering
Current system adds EQ+Compressor to everything as "good practice". This:
- Violates musical intent (user wants reverb, not signal processing)
- Clutters the interface
- Wastes computational resources

### Solution: Contextual Defaults

```python
def should_add_utility_processing(prompt: str, engines: List[int]) -> bool:
    """Decide if utility engines (EQ, compression) are appropriate"""

    # NEVER add utilities for simple effect requests
    if is_simple_effect_request(prompt):
        return False

    # Add utilities for instrument-specific processing
    if any(inst in prompt.lower() for inst in ["vocal", "guitar", "bass", "drums"]):
        return True

    # Add for "mix" or "master" contexts
    if any(word in prompt.lower() for word in ["mix", "master", "bus", "channel"]):
        return True

    # Add if user mentions "warm", "bright", "tight" (tonal shaping needed)
    if has_tonal_descriptors(prompt):
        return True

    return False
```

**Examples:**
- "spring reverb" → NO EQ/compression
- "warm vocal with spring reverb" → YES, add EQ for "warm"
- "master bus reverb" → YES, add compression for "master"

---

## Implementation Architecture

```python
class HybridVisionary:
    """Intelligent hybrid system combining precision and artistry"""

    def __init__(self):
        # Load knowledge base
        self.knowledge = load_knowledge_base()
        self.selection_rules = self.knowledge["engine_selection_rules"]

        # Initialize OpenAI clients
        self.client_mini = AsyncOpenAI(api_key=api_key)  # For tier 1 & naming
        self.client_standard = AsyncOpenAI(api_key=api_key)  # For tier 2 & 3

        # Initialize components
        self.namer = IntelligentPresetNamer()
        self.cache = PromptCache()

    async def generate_preset(self, prompt: str) -> Dict:
        """Main entry point with intelligent routing"""

        # 1. Check cache
        cached = self.cache.get_similar(prompt)
        if cached:
            return self.vary_cached_preset(cached, prompt)

        # 2. Classify request tier
        tier = self.classify_tier(prompt)
        mandatory_engines = self.extract_mandatory_engines(prompt)

        logger.info(f"Classified as Tier {tier}: {prompt}")
        logger.info(f"Mandatory engines: {mandatory_engines}")

        # 3. Route to appropriate handler
        if tier == 1:
            preset = await self.tier_1_exact_match(prompt, mandatory_engines)
        elif tier == 2:
            preset = await self.tier_2_guided_artistic(prompt, mandatory_engines)
        else:
            preset = await self.tier_3_full_artistic(prompt)

        # 4. Cache result
        self.cache.store(prompt, preset)

        return preset

    def classify_tier(self, prompt: str) -> int:
        """Classify request into tier 1, 2, or 3"""

        prompt_lower = prompt.lower()
        word_count = len(prompt.split())

        # Extract matched engines
        matched = self.extract_mandatory_engines(prompt)

        # Tier 1: Simple, exact match
        if matched and word_count <= 5 and is_simple_request(prompt):
            # "spring reverb", "noise gate", "shimmer"
            return 1

        # Tier 3: Complex artistic
        if (
            has_poetic_language(prompt) or
            word_count > 12 or
            count_descriptors(prompt) > 3
        ):
            # "heavenly mana from an angel"
            # "warm ambient pad with subtle tape saturation..."
            return 3

        # Tier 2: Hybrid (most requests fall here)
        return 2

    async def tier_1_exact_match(self, prompt: str, engines: List[int]) -> Dict:
        """Fast, precise execution for exact engine requests"""

        logger.info(f"⚡ TIER 1: Exact match - {len(engines)} engine(s)")

        # Build minimal prompt for parameter calculation only
        param_prompt = f"""Calculate parameters for: "{prompt}"

Engines: {[self.knowledge['engines'][str(e)]['name'] for e in engines]}

Return JSON with slots array, each engine with 15 parameters."""

        # Use fast, cheap model
        response = await self.client_mini.chat.completions.create(
            model="gpt-4o-mini",
            messages=[
                {"role": "system", "content": MINIMAL_SYSTEM_PROMPT},
                {"role": "user", "content": param_prompt}
            ],
            response_format={"type": "json_object"},
            temperature=0.5,
            max_tokens=800
        )

        preset = json.loads(response.choices[0].message.content)

        # Generate name separately (can be parallel in production)
        preset["name"] = self.namer.generate_name(prompt, engines, {})

        return preset

    async def tier_2_guided_artistic(self, prompt: str, mandatory: List[int]) -> Dict:
        """Hybrid approach with mandatory enforcement + creative filling"""

        logger.info(f"🎨 TIER 2: Guided artistic - {len(mandatory)} mandatory")

        # Get context and relevant engines
        context = self.analyze_prompt_context(prompt)
        relevant = self.get_relevant_engines(prompt, context, exclude=mandatory)[:5]

        # Determine appropriate engine count
        min_engines, max_engines = self.determine_engine_count(prompt, tier=2)

        # Build focused prompt
        generation_prompt = f"""Create preset: "{prompt}"

MANDATORY (must include):
{self.format_engines(mandatory)}

SUGGESTED (choose {max_engines - len(mandatory)} max):
{self.format_engines(relevant)}

RULES:
- Total engines: {min_engines}-{max_engines}
- Include ALL mandatory engines
- Add complementary engines only if beneficial
- NO utility processing unless tonally needed
- Creative but appropriate naming

Context: {context}

Return JSON with slots array."""

        # Use GPT-4o (balanced speed/quality)
        response = await self.client_standard.chat.completions.create(
            model="gpt-4o",
            messages=[
                {"role": "system", "content": FOCUSED_SYSTEM_PROMPT},
                {"role": "user", "content": generation_prompt}
            ],
            response_format={"type": "json_object"},
            temperature=0.7,
            max_tokens=1500
        )

        preset = json.loads(response.choices[0].message.content)

        # Validate mandatory engines were included
        self.validate_mandatory_engines(preset, mandatory)

        return preset

    async def tier_3_full_artistic(self, prompt: str) -> Dict:
        """Full creative AI generation for complex artistic requests"""

        logger.info(f"🌟 TIER 3: Full artistic generation")

        # Use current visionary_complete.py approach
        # but with GPT-4o instead of GPT-4
        context = self.analyze_prompt_context(prompt)
        generation_prompt = self.build_complete_generation_prompt(prompt, context)

        response = await self.client_standard.chat.completions.create(
            model="gpt-4o",  # Faster than GPT-4
            messages=[
                {"role": "system", "content": COMPLETE_SYSTEM_PROMPT},
                {"role": "user", "content": generation_prompt}
            ],
            response_format={"type": "json_object"},
            temperature=0.8,  # Higher creativity for artistic requests
            max_tokens=2000
        )

        preset = json.loads(response.choices[0].message.content)

        return preset
```

---

## Prompt Engineering Examples

### Tier 1 System Prompt (Minimal)
```
You calculate effect parameters for the Trinity audio plugin.

SIGNAL CHAIN ORDER:
Gate → EQ → Dynamics → Distortion → Modulation → Delay → Reverb

PARAMETER RULES:
- All values 0.0 - 1.0
- Mix parameters: minimum 0.15 (effects must be audible)
- 15 parameters per engine (pad with 0.5 if unused)

Return JSON: {"slots": [{"slot": 0, "engine_id": X, "parameters": [...]}]}
```

### Tier 2 System Prompt (Focused)
```
You are the Visionary - creative audio preset designer for Trinity plugin.

Available: 57 audio engines (reverbs, delays, distortions, etc.)

CORE RULES:
1. User-specified engines are MANDATORY
2. Add complementary engines if beneficial (max 4 total for most requests)
3. Simpler is better - don't over-engineer
4. Follow signal chain order
5. Creative naming (not literal prompt)

SIGNAL CHAIN:
Gate → EQ → Dynamics → Distortion → Modulation → Delay → Reverb → Spatial

WHEN TO ADD UTILITIES:
- EQ: Only if tonal shaping requested ("warm", "bright")
- Compression: Only for instrument processing or "tight" requests
- Gate: Only if "tight" or noise control needed

Return JSON with slots array and creative name.
```

### Tier 3 System Prompt (Complete)
```
[Current complete system prompt from visionary_complete.py]
[Including full engine catalog and detailed rules]
[~2000 tokens]
```

---

## Expected Performance Improvements

| Request Type | Current Time | Optimized Time | Speedup |
|-------------|--------------|----------------|---------|
| "spring reverb" | 20-30s | 3-5s | **6x faster** |
| "warm vintage tape delay" | 20-30s | 8-10s | **2.5x faster** |
| "heavenly ethereal soundscape" | 20-30s | 10-15s | **1.8x faster** |

**Cost Savings:**
- Tier 1: ~90% cheaper (GPT-4o-mini vs GPT-4)
- Tier 2: ~60% cheaper (GPT-4o vs GPT-4, shorter prompts)
- Tier 3: ~40% cheaper (GPT-4o vs GPT-4)

---

## Migration Path

### Phase 1: Immediate (Week 1)
1. Switch from GPT-4 to GPT-4o (instant 2x speedup)
2. Implement tier classification system
3. Create minimal/focused system prompts
4. Add prompt caching

### Phase 2: Core Features (Week 2)
1. Implement tier 1 (exact match) handler
2. Implement tier 2 (guided artistic) handler
3. Add engine count determination logic
4. Remove unnecessary EQ+Compressor additions

### Phase 3: Optimization (Week 3)
1. Add parallel name generation
2. Implement smart caching system
3. Add contextual utility engine logic
4. Performance tuning and testing

### Phase 4: Advanced (Week 4)
1. Response streaming for real-time feedback
2. Learning system (track which tier classification was correct)
3. User preference tracking
4. A/B testing framework

---

## Testing Strategy

```python
# Test cases covering all tiers
TEST_CASES = {
    "tier_1": [
        ("spring reverb", 1, [40]),
        ("shimmer", 1, [42]),
        ("noise gate", 1, [4]),
        ("plate reverb", 1, [39]),
    ],
    "tier_2": [
        ("warm spring reverb", 2, [40, 15]),  # + vintage preamp
        ("tight metal guitar", 2, [4, 22]),   # gate + overdrive
        ("ambient pad with shimmer", 2, [42, 34]),  # shimmer + delay
    ],
    "tier_3": [
        ("heavenly mana from an angel", 3, None),  # Full creative
        ("warm ambient soundscape with subtle movement", 3, None),
        ("create vintage psychedelic swirl", 3, None),
    ]
}

async def test_hybrid_system():
    """Validate tier classification and performance"""
    hybrid = HybridVisionary()

    for tier_name, cases in TEST_CASES.items():
        print(f"\n=== Testing {tier_name.upper()} ===")

        for prompt, expected_tier, expected_engines in cases:
            start = time.time()

            # Test classification
            actual_tier = hybrid.classify_tier(prompt)
            assert actual_tier == expected_tier, \
                f"Tier mismatch: {prompt} → {actual_tier} (expected {expected_tier})"

            # Test generation
            preset = await hybrid.generate_preset(prompt)
            elapsed = time.time() - start

            # Validate engines
            if expected_engines:
                actual = [s["engine_id"] for s in preset["slots"] if s["engine_id"] != 0]
                assert all(e in actual for e in expected_engines), \
                    f"Missing mandatory engines in {prompt}"

            # Performance check
            max_time = {1: 5, 2: 12, 3: 16}[expected_tier]
            assert elapsed < max_time, \
                f"Too slow: {elapsed:.1f}s (max {max_time}s for tier {expected_tier})"

            print(f"✓ {prompt}")
            print(f"  Tier: {actual_tier}, Engines: {len(actual)}, Time: {elapsed:.1f}s")
```

---

## Key Principles Summary

1. **Respect User Intent**
   - Exact requests get exact results (no extras)
   - Artistic requests get artistic freedom
   - Simple stays simple

2. **Performance First**
   - Use fastest model appropriate for task
   - Minimize token count
   - Cache aggressively
   - Parallel where possible

3. **Musical Intelligence**
   - Don't add processing for sake of processing
   - Understand context (reverb vs master bus)
   - Preserve artistic vision

4. **Fail Gracefully**
   - Tier 1 failure → fallback to Tier 2
   - Cache misses → full generation
   - Always return valid preset

---

## Conclusion

This hybrid architecture achieves the best of both worlds:

- **Precision** when users want specific engines
- **Artistry** when users want creative soundscaping
- **Performance** through intelligent model selection and prompt optimization
- **Simplicity** by avoiding over-engineering

**Expected Results:**
- 2-6x faster generation times
- 40-90% cost reduction
- Better musical results (simpler when appropriate)
- Higher user satisfaction (gets what they asked for)

The system evolves from "always use AI for everything" to "use the right tool for the right job" - sometimes that's simple keyword matching, sometimes it's full artistic AI generation, and often it's a smart hybrid of both.
