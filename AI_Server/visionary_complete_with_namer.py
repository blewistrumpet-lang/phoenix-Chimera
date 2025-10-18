#!/usr/bin/env python3
"""
Example of how to integrate the IntelligentPresetNamer into Visionary
This shows the key modifications needed
"""

# At the top of visionary_complete.py, add:
from preset_namer import IntelligentPresetNamer

class CompleteVisionary:
    def __init__(self):
        # ... existing init code ...

        # Initialize the intelligent namer
        self.namer = IntelligentPresetNamer()

        # Optional: Control whether to use AI or intelligent namer
        self.use_intelligent_namer = True  # Can be configured

    async def generate_complete_preset(self, prompt: str) -> Dict[str, Any]:
        """Modified version with intelligent naming"""

        if self.use_intelligent_namer:
            # Use the intelligent namer instead of asking GPT for a name
            return await self._generate_with_intelligent_naming(prompt)
        else:
            # Original GPT-based generation
            return await self._generate_with_ai_naming(prompt)

    async def _generate_with_intelligent_naming(self, prompt: str) -> Dict[str, Any]:
        """Generate preset but use intelligent namer for the name"""

        # First, get engines from GPT without requiring a name
        simplified_prompt = self._build_engine_selection_prompt(prompt)

        # Call GPT for engine selection only
        response = await self.client.chat.completions.create(
            model="gpt-4",
            messages=[
                {"role": "system", "content": self._create_engine_selection_system_prompt()},
                {"role": "user", "content": simplified_prompt}
            ],
            response_format={"type": "json_object"},
            temperature=0.7
        )

        preset = json.loads(response.choices[0].message.content)

        # Extract engines info for namer
        engines = []
        for slot in preset.get("slots", []):
            if slot.get("engine_id", 0) != 0:
                engines.append({
                    "engine_id": slot["engine_id"],
                    "engine_name": slot.get("engine_name", "Unknown")
                })

        # Generate name using intelligent system
        context = self.analyze_prompt_context(prompt)
        preset_name = self.namer.generate_name(prompt, engines, context)

        # Add the generated name to preset
        preset["name"] = preset_name

        logger.info(f"✨ Intelligent Namer generated: '{preset_name}'")

        return preset

    def _create_engine_selection_system_prompt(self) -> str:
        """System prompt focused on engine selection, not naming"""
        return f"""You are the Visionary component of the Trinity Pipeline.

{self._build_engine_catalog()}

Your job is to select and arrange engines for the given prompt.

Return JSON with this format:
{{
  "description": "What this preset does",
  "reasoning": {{
    "overall_approach": "Your strategy",
    "signal_flow": "Why this order"
  }},
  "slots": [
    {{
      "slot": 0,
      "engine_id": <id>,
      "engine_name": "<name>",
      "parameters": [...]
    }}
  ]
}}

DO NOT include a "name" field - naming will be handled separately."""

    def _build_engine_selection_prompt(self, prompt: str) -> str:
        """Build prompt focused on engine selection"""
        context = self.analyze_prompt_context(prompt)

        return f"""Create a preset for: "{prompt}"

Context analysis:
- Character: {context.get('character')}
- Intensity: {context.get('intensity')}
- Detected effects: {context.get('effects')}

Select appropriate engines and arrange them in proper signal flow order.
Return the JSON structure as specified."""