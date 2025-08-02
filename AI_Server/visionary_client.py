import asyncio
import json
import socket
import logging
from typing import Dict, Any, List

logger = logging.getLogger(__name__)

class VisionaryClient:
    """
    TCP client that connects to the external OpenAI bridge service.
    Sends prompts and receives the simplified VisionaryBlueprint.
    """
    
    def __init__(self, host: str = "localhost", port: int = 9999):
        self.host = host
        self.port = port
        self.system_prompt = """You are the Visionary, an AI that creates audio effect presets.

Given a user's creative prompt, output a JSON blueprint with this exact structure:

{
    "slots": [
        {
            "slot": 1,
            "engine_id": <0, 1, or 2>,
            "character": "<one-word descriptor>"
        },
        {
            "slot": 2,
            "engine_id": <-1, 0, 1, or 2>,
            "character": "<one-word descriptor or 'bypass'>"
        }
    ],
    "overall_vibe": "<2-3 word description>"
}

Engine IDs:
- -1: Bypass (no effect)
- 0: K-Style Overdrive (warm saturation, grit, harmonic richness)
- 1: Tape Echo (vintage delay, analog warmth, modulation)
- 2: Plate Reverb (spacious, metallic, classic studio reverb)

Rules:
1. Slot 1 must have an active engine (0, 1, or 2)
2. Slot 2 can be bypass (-1) or any engine
3. Consider signal flow: Slot 1 processes first, then Slot 2
4. Match engines to the user's intent

Example response:
{
    "slots": [
        {"slot": 1, "engine_id": 0, "character": "warm"},
        {"slot": 2, "engine_id": 1, "character": "vintage"}
    ],
    "overall_vibe": "warm vintage tone"
}"""

    async def get_blueprint(self, prompt: str) -> Dict[str, Any]:
        """
        Send prompt to OpenAI bridge and receive blueprint
        """
        try:
            # Create message for OpenAI bridge
            message = {
                "system": self.system_prompt,
                "user": prompt
            }
            
            # Try actual TCP connection first
            try:
                blueprint = await self._connect_to_bridge(message)
                return blueprint
            except (ConnectionRefusedError, OSError) as e:
                logger.warning(f"OpenAI bridge not available, using simulation: {str(e)}")
                # Fall back to simulation if bridge is not running
                blueprint = await self._simulate_openai_response(prompt)
                return blueprint
            
        except Exception as e:
            logger.error(f"Error in VisionaryClient: {str(e)}")
            # Return default blueprint on error
            return {
                "slots": [
                    {"slot": 1, "engine_id": 0, "character": "neutral"},
                    {"slot": 2, "engine_id": -1, "character": "bypass"}
                ],
                "overall_vibe": "default preset"
            }
    
    async def _simulate_openai_response(self, prompt: str) -> Dict[str, Any]:
        """
        Simulate OpenAI response for MVP testing
        """
        # Simple keyword-based logic for MVP
        prompt_lower = prompt.lower()
        
        # Default blueprint
        blueprint = {
            "slots": [
                {"slot": 1, "engine_id": 0, "character": "neutral"},
                {"slot": 2, "engine_id": -1, "character": "bypass"}
            ],
            "overall_vibe": "balanced tone"
        }
        
        # Analyze prompt for keywords
        if "vintage" in prompt_lower or "retro" in prompt_lower:
            blueprint["slots"][0] = {"slot": 1, "engine_id": 0, "character": "warm"}
            blueprint["slots"][1] = {"slot": 2, "engine_id": 1, "character": "vintage"}
            blueprint["overall_vibe"] = "vintage warmth"
            
        elif "space" in prompt_lower or "ambient" in prompt_lower:
            blueprint["slots"][0] = {"slot": 1, "engine_id": 2, "character": "spacious"}
            blueprint["slots"][1] = {"slot": 2, "engine_id": 1, "character": "ethereal"}
            blueprint["overall_vibe"] = "ambient space"
            
        elif "aggressive" in prompt_lower or "heavy" in prompt_lower:
            blueprint["slots"][0] = {"slot": 1, "engine_id": 0, "character": "aggressive"}
            blueprint["slots"][1] = {"slot": 2, "engine_id": -1, "character": "bypass"}
            blueprint["overall_vibe"] = "heavy drive"
            
        elif "clean" in prompt_lower or "pristine" in prompt_lower:
            blueprint["slots"][0] = {"slot": 1, "engine_id": 2, "character": "clean"}
            blueprint["slots"][1] = {"slot": 2, "engine_id": -1, "character": "bypass"}
            blueprint["overall_vibe"] = "clean space"
            
        elif "echo" in prompt_lower or "delay" in prompt_lower:
            blueprint["slots"][0] = {"slot": 1, "engine_id": 1, "character": "rhythmic"}
            blueprint["slots"][1] = {"slot": 2, "engine_id": 2, "character": "spatial"}
            blueprint["overall_vibe"] = "echoing depth"
        
        return blueprint
    
    async def _connect_to_bridge(self, message: Dict[str, Any]) -> Dict[str, Any]:
        """
        Actual TCP connection to OpenAI bridge (for future implementation)
        """
        reader, writer = await asyncio.open_connection(self.host, self.port)
        
        try:
            # Send message
            message_json = json.dumps(message) + "\n"
            writer.write(message_json.encode())
            await writer.drain()
            
            # Read response
            data = await reader.readline()
            response = json.loads(data.decode())
            
            return response
            
        finally:
            writer.close()
            await writer.wait_closed()