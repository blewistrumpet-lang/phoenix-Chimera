"""
OpenAI Bridge Server - TCP server that handles OpenAI API calls
Run this separately to handle OpenAI requests from the VisionaryClient
"""

import asyncio
import json
import logging
import openai
from typing import Dict, Any

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# OpenAI API configuration
OPENAI_API_KEY = "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"
openai.api_key = OPENAI_API_KEY

class OpenAIBridgeServer:
    def __init__(self, host: str = "0.0.0.0", port: int = 9999):
        self.host = host
        self.port = port
        
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
            
            # Call OpenAI API
            response = await asyncio.to_thread(
                openai.ChatCompletion.create,
                model="gpt-4",
                messages=[
                    {"role": "system", "content": system_prompt},
                    {"role": "user", "content": user_prompt}
                ],
                temperature=0.7,
                max_tokens=500
            )
            
            # Extract JSON from response
            content = response.choices[0].message.content
            
            # Try to parse as JSON
            try:
                # Find JSON in the response (it might be wrapped in markdown or text)
                import re
                json_match = re.search(r'\{.*\}', content, re.DOTALL)
                if json_match:
                    blueprint = json.loads(json_match.group())
                else:
                    # Fallback to parsing the whole content
                    blueprint = json.loads(content)
                    
                return blueprint
                
            except json.JSONDecodeError:
                logger.warning(f"Failed to parse OpenAI response as JSON: {content}")
                # Return a default blueprint
                return {
                    "slots": [
                        {"slot": 1, "engine_id": 0, "character": "neutral"},
                        {"slot": 2, "engine_id": -1, "character": "bypass"}
                    ],
                    "overall_vibe": "default preset"
                }
                
        except Exception as e:
            logger.error(f"OpenAI API error: {str(e)}")
            raise
    
    async def start_server(self):
        """Start the TCP server"""
        server = await asyncio.start_server(
            self.handle_client, 
            self.host, 
            self.port
        )
        
        addr = server.sockets[0].getsockname()
        logger.info(f"OpenAI Bridge Server running on {addr[0]}:{addr[1]}")
        
        async with server:
            await server.serve_forever()

async def main():
    """Main entry point"""
    server = OpenAIBridgeServer()
    await server.start_server()

if __name__ == "__main__":
    asyncio.run(main())