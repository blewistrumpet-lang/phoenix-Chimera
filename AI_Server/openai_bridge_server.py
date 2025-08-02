"""
OpenAI Bridge Server - TCP server that handles OpenAI API calls
Run this separately to handle OpenAI requests from the VisionaryClient
"""

import asyncio
import json
import logging
import os
from openai import OpenAI
from typing import Dict, Any

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# OpenAI API configuration
# First try environment variable, then use the provided key
OPENAI_API_KEY = os.getenv("OPENAI_API_KEY", "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")

class OpenAIBridgeServer:
    def __init__(self, host: str = "0.0.0.0", port: int = 9999):
        self.host = host
        self.port = port
        self.client = OpenAI(api_key=OPENAI_API_KEY)
        
    async def handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        """Handle individual client connections"""
        client_address = writer.get_extra_info('peername')
        logger.info(f"Client connected from {client_address}")
        
        try:
            while True:
                # Read message from client
                data = await reader.readline()
                if not data:
                    break
                
                message = json.loads(data.decode().strip())
                logger.info(f"Received message: {message}")
                
                # Process with OpenAI
                response = await self.process_with_openai(message)
                
                # Send response back
                response_json = json.dumps(response) + "\n"
                writer.write(response_json.encode())
                await writer.drain()
                
        except Exception as e:
            logger.error(f"Error handling client: {str(e)}")
            error_response = {"error": str(e)}
            writer.write((json.dumps(error_response) + "\n").encode())
            await writer.drain()
            
        finally:
            logger.info(f"Client {client_address} disconnected")
            writer.close()
            await writer.wait_closed()
    
    async def process_with_openai(self, message: Dict[str, Any]) -> Dict[str, Any]:
        """Process message with OpenAI API"""
        try:
            system_prompt = message.get("system", "")
            user_prompt = message.get("user", "")
            
            # Call OpenAI API with new client
            response = await asyncio.to_thread(
                lambda: self.client.chat.completions.create(
                    model="gpt-4",
                    messages=[
                        {"role": "system", "content": system_prompt},
                        {"role": "user", "content": user_prompt}
                    ],
                    temperature=0.7,
                    max_tokens=500,
                    response_format={"type": "json_object"}  # Request JSON response
                )
            )
            
            # Extract content from response
            content = response.choices[0].message.content
            
            # Try to parse as JSON
            try:
                # The response should already be valid JSON due to response_format
                blueprint = json.loads(content)
                
                # Validate the blueprint structure
                if self._validate_blueprint(blueprint):
                    return blueprint
                else:
                    logger.warning(f"Invalid blueprint structure: {blueprint}")
                    return self._get_default_blueprint()
                    
            except json.JSONDecodeError:
                logger.warning(f"Failed to parse OpenAI response as JSON: {content}")
                return self._get_default_blueprint()
                
        except Exception as e:
            logger.error(f"OpenAI API error: {str(e)}")
            # Return a safe default on error
            return self._get_default_blueprint()
    
    def _validate_blueprint(self, blueprint: Dict[str, Any]) -> bool:
        """Validate that the blueprint has the correct structure"""
        if not isinstance(blueprint, dict):
            return False
        
        if "slots" not in blueprint or "overall_vibe" not in blueprint:
            return False
        
        if not isinstance(blueprint["slots"], list):
            return False
        
        # Check each slot
        for slot in blueprint["slots"]:
            if not isinstance(slot, dict):
                return False
            if "slot" not in slot or "engine_id" not in slot or "character" not in slot:
                return False
            if not isinstance(slot["slot"], int) or not isinstance(slot["engine_id"], int):
                return False
        
        return True
    
    def _get_default_blueprint(self) -> Dict[str, Any]:
        """Return a safe default blueprint"""
        return {
            "slots": [
                {"slot": 1, "engine_id": 0, "character": "warm"},
                {"slot": 2, "engine_id": 27, "character": "balanced"},
                {"slot": 3, "engine_id": 3, "character": "spacious"},
                {"slot": 4, "engine_id": -1, "character": "bypass"},
                {"slot": 5, "engine_id": -1, "character": "bypass"},
                {"slot": 6, "engine_id": -1, "character": "bypass"}
            ],
            "overall_vibe": "balanced warmth"
        }
    
    async def start_server(self):
        """Start the TCP server"""
        server = await asyncio.start_server(
            self.handle_client, 
            self.host, 
            self.port
        )
        
        addr = server.sockets[0].getsockname()
        logger.info(f"OpenAI Bridge Server running on {addr[0]}:{addr[1]}")
        logger.info(f"Using OpenAI API key: {OPENAI_API_KEY[:20]}...")
        
        async with server:
            await server.serve_forever()

async def main():
    """Main entry point"""
    server = OpenAIBridgeServer()
    await server.start_server()

if __name__ == "__main__":
    asyncio.run(main())