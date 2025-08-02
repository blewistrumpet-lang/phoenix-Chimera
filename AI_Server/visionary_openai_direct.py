import asyncio
import json
import logging
import os
from openai import OpenAI
from typing import Dict, Any, List
from visionary_enhanced import VisionaryEnhanced

logger = logging.getLogger(__name__)

class VisionaryOpenAIDirect(VisionaryEnhanced):
    """
    Visionary that connects directly to OpenAI API without bridge server
    """
    
    def __init__(self):
        super().__init__()
        # Get API key from environment or use provided key
        api_key = os.getenv("OPENAI_API_KEY", "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")
        self.openai_client = OpenAI(api_key=api_key)
        self.use_openai = True  # Flag to enable/disable OpenAI
        
    async def get_blueprint(self, prompt: str) -> Dict[str, Any]:
        """
        Generate blueprint using OpenAI directly or fall back to simulation
        """
        if self.use_openai:
            try:
                # Try OpenAI first
                logger.info("Consulting OpenAI GPT-4...")
                blueprint = await self._get_openai_blueprint(prompt)
                if blueprint:
                    logger.info("Successfully received blueprint from OpenAI")
                    return blueprint
            except Exception as e:
                logger.warning(f"OpenAI API error: {str(e)}, falling back to simulation")
        
        # Fall back to enhanced simulation
        logger.info("Using enhanced simulation")
        return await self._simulate_enhanced_response(prompt)
    
    async def _get_openai_blueprint(self, prompt: str) -> Dict[str, Any]:
        """Get blueprint from OpenAI API"""
        try:
            # Use asyncio.to_thread for async compatibility
            response = await asyncio.to_thread(
                lambda: self.openai_client.chat.completions.create(
                    model="gpt-3.5-turbo",  # Using 3.5-turbo which supports JSON mode
                    messages=[
                        {"role": "system", "content": self.system_prompt},
                        {"role": "user", "content": prompt}
                    ],
                    temperature=0.7,
                    max_tokens=800,
                    response_format={"type": "json_object"}
                )
            )
            
            # Extract and parse response
            content = response.choices[0].message.content
            blueprint = json.loads(content)
            
            # Validate structure
            if self._validate_blueprint(blueprint):
                return blueprint
            else:
                logger.warning("Invalid blueprint structure from OpenAI")
                return None
                
        except Exception as e:
            logger.error(f"OpenAI API error: {str(e)}")
            return None
    
    def _validate_blueprint(self, blueprint: Dict[str, Any]) -> bool:
        """Validate blueprint structure"""
        try:
            # Check required fields
            if not isinstance(blueprint, dict):
                return False
            
            if "slots" not in blueprint or "overall_vibe" not in blueprint:
                return False
            
            if not isinstance(blueprint["slots"], list) or len(blueprint["slots"]) != 6:
                return False
            
            # Validate each slot
            for i, slot in enumerate(blueprint["slots"]):
                if not isinstance(slot, dict):
                    return False
                
                if "slot" not in slot or "engine_id" not in slot or "character" not in slot:
                    return False
                
                # Check slot number
                if slot["slot"] != i + 1:
                    return False
                
                # Check engine_id is valid
                engine_id = slot["engine_id"]
                if not isinstance(engine_id, int):
                    return False
                
                if engine_id != -1 and engine_id not in self.engines:
                    logger.warning(f"Unknown engine_id: {engine_id}")
                    return False
            
            return True
            
        except Exception as e:
            logger.error(f"Blueprint validation error: {str(e)}")
            return False


# Quick test function
async def test_visionary_openai():
    """Test the Visionary with direct OpenAI connection"""
    visionary = VisionaryOpenAIDirect()
    
    test_prompts = [
        "Create a warm vintage guitar tone with tube saturation and spring reverb",
        "Design an aggressive metal sound with tight gate and distortion",
        "Build a spacious ambient pad with shimmer and long reverb tails"
    ]
    
    print("\n" + "="*80)
    print("TESTING VISIONARY WITH DIRECT OPENAI CONNECTION")
    print("="*80)
    
    for i, prompt in enumerate(test_prompts):
        print(f"\nTest {i+1}: {prompt}")
        print("-"*60)
        
        blueprint = await visionary.get_blueprint(prompt)
        
        print(f"Vibe: {blueprint.get('overall_vibe', 'Unknown')}")
        print("Active Engines:")
        
        for slot in blueprint['slots']:
            if slot['engine_id'] >= 0:
                engine = visionary.engines.get(slot['engine_id'], {})
                print(f"  Slot {slot['slot']}: {engine.get('name', 'Unknown')} ({slot['character']})")
        
        # Add delay between requests to avoid rate limiting
        if i < len(test_prompts) - 1:
            await asyncio.sleep(1)


if __name__ == "__main__":
    # Run test
    asyncio.run(test_visionary_openai())