#!/usr/bin/env python3
"""
Trinity TCP Bridge Server
Provides TCP interface for the plugin to communicate with the Trinity Pipeline
Handles the complete flow: Plugin → TCP → Trinity Pipeline → Plugin
"""

import asyncio
import json
import logging
import aiohttp
from typing import Dict, Any, Optional, Set
from dataclasses import dataclass, field
import time

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


@dataclass
class ClientSession:
    """Tracks a connected client session"""
    session_id: str
    writer: asyncio.StreamWriter
    reader: asyncio.StreamReader
    address: str
    connected_at: float = field(default_factory=time.time)
    last_heartbeat: float = field(default_factory=time.time)
    pending_messages: asyncio.Queue = field(default_factory=asyncio.Queue)


class TrinityTCPBridge:
    """
    TCP Bridge Server for Trinity Pipeline
    Listens on port 9999 for plugin connections
    Forwards requests to HTTP API server on port 8000
    """
    
    def __init__(self, host: str = "0.0.0.0", port: int = 9999, api_url: str = "http://localhost:8000"):
        self.host = host
        self.port = port
        self.api_url = api_url
        self.server = None
        self.sessions: Dict[str, ClientSession] = {}
        self.running = False
        
    async def start(self):
        """Start the TCP bridge server"""
        self.server = await asyncio.start_server(
            self.handle_client,
            self.host,
            self.port
        )
        
        self.running = True
        addr = self.server.sockets[0].getsockname()
        logger.info(f"🌉 Trinity TCP Bridge listening on {addr[0]}:{addr[1]}")
        logger.info(f"📡 Forwarding to Trinity API at {self.api_url}")
        logger.info("=" * 60)
        
        # Start heartbeat checker
        asyncio.create_task(self.heartbeat_checker())
        
        async with self.server:
            await self.server.serve_forever()
    
    async def handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        """Handle incoming plugin TCP connections"""
        addr = writer.get_extra_info('peername')
        session_id = f"tcp_session_{int(time.time() * 1000)}_{addr[1]}"
        logger.info(f"✅ Plugin connected from {addr} - Session: {session_id}")
        
        # Create session
        session = ClientSession(
            session_id=session_id,
            writer=writer,
            reader=reader,
            address=f"{addr[0]}:{addr[1]}"
        )
        self.sessions[session_id] = session
        
        # Send welcome message
        await self.send_to_client(session, {
            "type": "connection",
            "status": "connected",
            "session_id": session_id,
            "message": "Connected to Trinity TCP Bridge"
        })
        
        try:
            # Start message sender for this client
            sender_task = asyncio.create_task(self.message_sender(session))
            
            # Read messages from plugin
            while self.running:
                try:
                    # Read line-delimited JSON
                    data = await asyncio.wait_for(reader.readline(), timeout=60.0)
                    if not data:
                        logger.info(f"Client {session_id} disconnected")
                        break
                    
                    # Parse message
                    try:
                        message = json.loads(data.decode('utf-8').strip())
                        await self.handle_plugin_message(session, message)
                    except json.JSONDecodeError as e:
                        logger.error(f"Invalid JSON from {session_id}: {e}")
                        await self.send_to_client(session, {
                            "type": "error",
                            "message": "Invalid JSON format"
                        })
                        
                except asyncio.TimeoutError:
                    # Send keepalive
                    await self.send_to_client(session, {
                        "type": "keepalive",
                        "timestamp": int(time.time() * 1000)
                    })
                    
        except Exception as e:
            logger.error(f"Error handling client {session_id}: {e}")
            
        finally:
            # Clean up
            sender_task.cancel()
            del self.sessions[session_id]
            writer.close()
            await writer.wait_closed()
            logger.info(f"Session {session_id} closed")
    
    async def handle_plugin_message(self, session: ClientSession, message: Dict[str, Any]):
        """Process message from plugin and forward to Trinity Pipeline"""
        msg_type = message.get("type", "")
        session.last_heartbeat = time.time()
        
        logger.info(f"📨 Message from {session.session_id}: {msg_type}")
        
        # Handle different message types
        if msg_type == "heartbeat":
            # Respond with heartbeat acknowledgment
            await self.send_to_client(session, {
                "type": "heartbeat_ack",
                "timestamp": message.get("timestamp")
            })
            
        elif msg_type == "query" or msg_type == "preset_request":
            # Forward to Trinity Pipeline
            content = message.get("content", message.get("prompt", ""))
            if content:
                await self.process_trinity_request(session, content, message)
            else:
                await self.send_to_client(session, {
                    "type": "error",
                    "message": "No content in query"
                })
                
        elif msg_type == "plugin_state":
            # Plugin state update - could be used for context
            logger.info(f"Plugin state update from {session.session_id}")
            # Could forward to Trinity for context-aware generation
            
        else:
            logger.warning(f"Unknown message type: {msg_type}")
    
    async def process_trinity_request(self, session: ClientSession, prompt: str, original_msg: Dict):
        """Forward request to Trinity Pipeline and send response back to plugin"""
        logger.info(f"🎭 Processing Trinity request: {prompt[:50]}...")
        
        try:
            # Prepare request for Trinity HTTP API
            payload = {
                "type": "query",
                "content": prompt,
                "session_id": session.session_id,
                "timestamp": int(time.time() * 1000)
            }
            
            # Add any additional metadata from original message
            if "metadata" in original_msg:
                payload["metadata"] = original_msg["metadata"]
            
            # Send to Trinity Pipeline via HTTP
            async with aiohttp.ClientSession() as http_session:
                async with http_session.post(
                    f"{self.api_url}/message",
                    json=payload,
                    timeout=aiohttp.ClientTimeout(total=30)
                ) as response:
                    
                    if response.status == 200:
                        result = await response.json()
                        
                        if result.get("success"):
                            # Extract preset from result
                            preset_data = result.get("data", {}).get("preset", {})
                            
                            # Send preset to plugin in expected format
                            trinity_response = {
                                "type": "preset",
                                "success": True,
                                "data": {
                                    "preset": preset_data
                                },
                                "message": result.get("message", ""),
                                "session_id": session.session_id,
                                "timestamp": int(time.time() * 1000)
                            }
                            
                            logger.info(f"✨ Sending preset '{preset_data.get('name')}' to plugin")
                            await session.pending_messages.put(trinity_response)
                            
                        else:
                            # Error from Trinity
                            error_msg = result.get("message", "Trinity Pipeline error")
                            logger.error(f"Trinity error: {error_msg}")
                            
                            await self.send_to_client(session, {
                                "type": "error",
                                "success": False,
                                "message": error_msg,
                                "session_id": session.session_id
                            })
                    else:
                        logger.error(f"HTTP error {response.status} from Trinity")
                        await self.send_to_client(session, {
                            "type": "error",
                            "success": False,
                            "message": f"Server error: {response.status}"
                        })
                        
        except asyncio.TimeoutError:
            logger.error("Trinity request timed out")
            await self.send_to_client(session, {
                "type": "error",
                "success": False,
                "message": "Request timed out"
            })
            
        except Exception as e:
            logger.error(f"Error processing Trinity request: {e}")
            await self.send_to_client(session, {
                "type": "error",
                "success": False,
                "message": str(e)
            })
    
    async def send_to_client(self, session: ClientSession, message: Dict[str, Any]):
        """Send message to plugin client"""
        try:
            # Add session ID if not present
            if "session_id" not in message:
                message["session_id"] = session.session_id
                
            # Convert to JSON and add newline delimiter
            data = json.dumps(message) + "\n"
            session.writer.write(data.encode('utf-8'))
            await session.writer.drain()
            
            logger.debug(f"Sent to {session.session_id}: {message.get('type')}")
            
        except Exception as e:
            logger.error(f"Error sending to {session.session_id}: {e}")
    
    async def message_sender(self, session: ClientSession):
        """Background task to send queued messages to client"""
        try:
            while session.session_id in self.sessions:
                # Wait for messages in queue
                try:
                    message = await asyncio.wait_for(
                        session.pending_messages.get(),
                        timeout=1.0
                    )
                    await self.send_to_client(session, message)
                except asyncio.TimeoutError:
                    continue
                    
        except asyncio.CancelledError:
            pass
    
    async def heartbeat_checker(self):
        """Check for stale sessions and clean them up"""
        while self.running:
            await asyncio.sleep(30)  # Check every 30 seconds
            
            now = time.time()
            stale_sessions = []
            
            for session_id, session in self.sessions.items():
                if now - session.last_heartbeat > 120:  # 2 minutes timeout
                    stale_sessions.append(session_id)
                    
            for session_id in stale_sessions:
                logger.warning(f"Removing stale session: {session_id}")
                if session_id in self.sessions:
                    session = self.sessions[session_id]
                    session.writer.close()
                    del self.sessions[session_id]


async def main():
    """Run the Trinity TCP Bridge Server"""
    bridge = TrinityTCPBridge()
    
    logger.info("=" * 60)
    logger.info("🚀 Starting Trinity TCP Bridge Server")
    logger.info("=" * 60)
    
    try:
        await bridge.start()
    except KeyboardInterrupt:
        logger.info("Shutting down TCP bridge...")
    except Exception as e:
        logger.error(f"Fatal error: {e}")
        raise


if __name__ == "__main__":
    asyncio.run(main())