"""
TCP Bridge Server for Visionary AI Service
Provides a TCP interface to OpenAI or other AI services
"""

import asyncio
import json
import logging
import os
from typing import Dict, Any, Optional
from pathlib import Path

# OpenAI imports - optional
try:
    from openai import OpenAI
    OPENAI_AVAILABLE = True
except ImportError:
    OPENAI_AVAILABLE = False
    print("OpenAI library not installed. Bridge will use simulation only.")

# Load environment variables
try:
    from dotenv import load_dotenv
    env_path = Path(__file__).parent / '.env'
    if env_path.exists():
        load_dotenv(env_path)
except ImportError:
    pass

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class VisionaryBridgeServer:
    """
    TCP server that bridges between the VisionaryClient and AI services.
    Provides a clean separation between networking and AI logic.
    """
    
    def __init__(self, host: str = "localhost", port: int = 9999):
        self.host = host
        self.port = port
        self.server = None
        
        # Initialize OpenAI client if available
        self.openai_client = None
        if OPENAI_AVAILABLE:
            api_key = os.getenv("OPENAI_API_KEY")
            if api_key:
                self.openai_client = OpenAI(api_key=api_key)
                logger.info("OpenAI client initialized")
            else:
                logger.warning("No OpenAI API key found in environment")
    
    async def start(self):
        """Start the TCP server"""
        self.server = await asyncio.start_server(
            self.handle_client,
            self.host,
            self.port
        )
        
        addr = self.server.sockets[0].getsockname()
        logger.info(f"Visionary Bridge Server listening on {addr[0]}:{addr[1]}")
        
        async with self.server:
            await self.server.serve_forever()
    
    async def handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        """Handle incoming client connections"""
        addr = writer.get_extra_info('peername')
        logger.info(f"Client connected from {addr}")
        
        try:
            # Read request
            data = await asyncio.wait_for(reader.readline(), timeout=30.0)
            if not data:
                logger.warning("Empty request received")
                return
            
            request = json.loads(data.decode('utf-8'))
            logger.info(f"Received request: {request.get('user', '')[:100]}...")
            
            # Process request
            response = await self.process_request(request)
            
            # Send response
            response_json = json.dumps(response) + "\n"
            writer.write(response_json.encode('utf-8'))
            await writer.drain()
            
            logger.info("Response sent successfully")
            
        except asyncio.TimeoutError:
            logger.error("Client request timed out")
            error_response = {"error": "Request timed out"}
            writer.write((json.dumps(error_response) + "\n").encode('utf-8'))
            await writer.drain()
            
        except json.JSONDecodeError as e:
            logger.error(f"Invalid JSON received: {e}")
            error_response = {"error": "Invalid JSON format"}
            writer.write((json.dumps(error_response) + "\n").encode('utf-8'))
            await writer.drain()
            
        except Exception as e:
            logger.error(f"Error handling client: {e}")
            error_response = {"error": str(e)}
            writer.write((json.dumps(error_response) + "\n").encode('utf-8'))
            await writer.drain()
            
        finally:
            writer.close()
            await writer.wait_closed()
            logger.info(f"Client {addr} disconnected")
    
    async def process_request(self, request: Dict[str, Any]) -> Dict[str, Any]:
        """
        Process the request and generate a blueprint.
        Uses OpenAI if available, otherwise falls back to simulation.
        """
        system_prompt = request.get("system", "")
        user_prompt = request.get("user", "")
        
        if not user_prompt:
            return {"error": "No user prompt provided"}
        
        # Try OpenAI first if available
        if self.openai_client:
            try:
                blueprint = await self.generate_with_openai(system_prompt, user_prompt)
                if blueprint:
                    return {"blueprint": blueprint}
            except Exception as e:
                logger.error(f"OpenAI generation failed: {e}")
        
        # Fall back to simulation
        logger.info("Using simulation for blueprint generation")
        blueprint = await self.simulate_blueprint(user_prompt)
        return {"blueprint": blueprint}
    
    async def generate_with_openai(self, system_prompt: str, user_prompt: str) -> Optional[Dict[str, Any]]:
        """Generate blueprint using OpenAI API"""
        if not self.openai_client:
            return None
        
        try:
            # Make API call in thread pool to avoid blocking
            response = await asyncio.to_thread(
                lambda: self.openai_client.chat.completions.create(
                    model="gpt-3.5-turbo",
                    messages=[
                        {"role": "system", "content": system_prompt},
                        {"role": "user", "content": user_prompt}
                    ],
                    temperature=0.7,
                    max_tokens=800,
                    response_format={"type": "json_object"}
                )
            )
            
            content = response.choices[0].message.content
            blueprint = json.loads(content)
            
            # Ensure blueprint has required structure
            if self.validate_blueprint(blueprint):
                return self.ensure_six_slots(blueprint)
            else:
                logger.warning("Invalid blueprint from OpenAI")
                return None
                
        except Exception as e:
            logger.error(f"OpenAI API error: {e}")
            return None
    
    async def simulate_blueprint(self, user_prompt: str) -> Dict[str, Any]:
        """Simulate blueprint generation when OpenAI is not available"""
        prompt_lower = user_prompt.lower()
        
        # Build blueprint based on prompt analysis
        slots = []
        overall_vibe = "balanced tone"
        creative_analysis = {
            "mood": "neutral",
            "intensity": 0.5,
            "space": 0.5,
            "character": "balanced"
        }
        
        # Analyze prompt and configure slots
        if "vintage" in prompt_lower or "warm" in prompt_lower:
            slots.extend([
                {"slot": 1, "engine_id": 0, "character": "warm"},
                {"slot": 2, "engine_id": 1, "character": "vintage"},
                {"slot": 3, "engine_id": 2, "character": "spacious"}
            ])
            overall_vibe = "vintage warmth"
            creative_analysis["character"] = "vintage"
            
        elif "aggressive" in prompt_lower or "metal" in prompt_lower:
            slots.extend([
                {"slot": 1, "engine_id": 0, "character": "aggressive"},
                {"slot": 2, "engine_id": 3, "character": "tight"},
                {"slot": 3, "engine_id": 4, "character": "scooped"}
            ])
            overall_vibe = "aggressive metal"
            creative_analysis["intensity"] = 0.9
            
        elif "ambient" in prompt_lower or "spacious" in prompt_lower:
            slots.extend([
                {"slot": 1, "engine_id": 5, "character": "smooth"},
                {"slot": 2, "engine_id": 1, "character": "ethereal"},
                {"slot": 3, "engine_id": 2, "character": "vast"}
            ])
            overall_vibe = "ambient space"
            creative_analysis["space"] = 0.9
            
        elif "clean" in prompt_lower or "pristine" in prompt_lower:
            slots.extend([
                {"slot": 1, "engine_id": 3, "character": "transparent"},
                {"slot": 2, "engine_id": 4, "character": "bright"}
            ])
            overall_vibe = "clean tone"
            creative_analysis["character"] = "modern"
            
        else:
            # Default configuration
            slots.append({"slot": 1, "engine_id": 0, "character": "neutral"})
            overall_vibe = "standard preset"
        
        # Fill remaining slots with bypass
        while len(slots) < 6:
            slots.append({
                "slot": len(slots) + 1,
                "engine_id": -1,
                "character": "bypass"
            })
        
        # Ensure slot numbers are correct
        for i, slot in enumerate(slots):
            slot["slot"] = i + 1
        
        return {
            "slots": slots[:6],  # Ensure exactly 6 slots
            "overall_vibe": overall_vibe,
            "creative_analysis": creative_analysis
        }
    
    def validate_blueprint(self, blueprint: Dict[str, Any]) -> bool:
        """Validate blueprint structure"""
        try:
            if not isinstance(blueprint, dict):
                return False
            
            if "slots" not in blueprint:
                return False
            
            if not isinstance(blueprint["slots"], list):
                return False
            
            # Check each slot
            for slot in blueprint["slots"]:
                if not isinstance(slot, dict):
                    return False
                if "engine_id" not in slot:
                    return False
            
            return True
            
        except Exception:
            return False
    
    def ensure_six_slots(self, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Ensure blueprint has exactly 6 slots"""
        if "slots" not in blueprint:
            blueprint["slots"] = []
        
        # Pad with bypass slots if needed
        while len(blueprint["slots"]) < 6:
            blueprint["slots"].append({
                "slot": len(blueprint["slots"]) + 1,
                "engine_id": -1,
                "character": "bypass"
            })
        
        # Truncate if too many
        blueprint["slots"] = blueprint["slots"][:6]
        
        # Fix slot numbers
        for i, slot in enumerate(blueprint["slots"]):
            slot["slot"] = i + 1
        
        # Add creative_analysis if missing
        if "creative_analysis" not in blueprint:
            blueprint["creative_analysis"] = {
                "mood": "neutral",
                "intensity": 0.5,
                "space": 0.5,
                "character": "balanced"
            }
        
        return blueprint


async def main():
    """Main entry point"""
    server = VisionaryBridgeServer()
    
    print("\n" + "="*60)
    print("VISIONARY TCP BRIDGE SERVER")
    print("="*60)
    print(f"Starting server on {server.host}:{server.port}")
    print("Press Ctrl+C to stop")
    print("="*60 + "\n")
    
    try:
        await server.start()
    except KeyboardInterrupt:
        print("\nShutting down server...")


if __name__ == "__main__":
    asyncio.run(main())