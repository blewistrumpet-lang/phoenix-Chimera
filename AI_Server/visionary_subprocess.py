#!/usr/bin/env python3
"""
Isolated subprocess for Visionary OpenAI calls
Runs in separate process to prevent crashes from affecting main server
"""

import sys
import json
import asyncio
import signal
import traceback
from typing import Dict, Any
import os
from pathlib import Path
from dotenv import load_dotenv

# Ensure we load from the correct .env file
env_path = Path(__file__).parent / '.env'
if env_path.exists():
    load_dotenv(dotenv_path=env_path)
else:
    load_dotenv()

# Import Complete Visionary with full knowledge
from visionary_complete import CompleteVisionary

class VisionarySubprocess:
    """Isolated Visionary processor with timeout and error handling"""
    
    def __init__(self):
        self.api_key = os.getenv("OPENAI_API_KEY")
        
        if not self.api_key:
            sys.stderr.write("ERROR: No OPENAI_API_KEY found in environment!\n")
            sys.stderr.write(f"Checked .env at: {env_path}\n")
            sys.stderr.write("Subprocess will use fallback generation only.\n")
        else:
            sys.stderr.write(f"API key loaded successfully (length: {len(self.api_key)})\n")
        
        self.visionary = CompleteVisionary()  # Loads complete engine knowledge
        
        # Set up signal handlers for graceful shutdown
        signal.signal(signal.SIGTERM, self.handle_shutdown)
        signal.signal(signal.SIGINT, self.handle_shutdown)
    
    def handle_shutdown(self, signum, frame):
        """Handle shutdown signals gracefully"""
        sys.stderr.write("Visionary subprocess shutting down gracefully\n")
        sys.exit(0)
    
    async def process_request(self, prompt: str) -> Dict[str, Any]:
        """Process a single request with timeout and error handling"""
        try:
            # Set a hard timeout of 25 seconds
            preset = await asyncio.wait_for(
                self.visionary.generate_complete_preset(prompt),
                timeout=25.0
            )
            
            # Extract reasoning if present and log it
            if "visionary_reasoning" in preset:
                reasoning = preset["visionary_reasoning"]
                sys.stderr.write("🧠 VISIONARY ENGINE SELECTION REASONING:\n")
                sys.stderr.write(f"  Overall approach: {reasoning.get('overall_approach', 'N/A')}\n")
                sys.stderr.write(f"  Signal flow: {reasoning.get('signal_flow', 'N/A')}\n")
                for slot_reason in reasoning.get('slot_reasoning', []):
                    sys.stderr.write(f"  Slot {slot_reason.get('slot', '?')}: {slot_reason.get('engine', 'N/A')}\n")
                    sys.stderr.write(f"    Why selected: {slot_reason.get('why', 'N/A')}\n")
                    sys.stderr.write(f"    Key params: {slot_reason.get('key_params', 'N/A')}\n")
            
            return {
                "success": True,
                "preset": preset,
                "reasoning": preset.get("visionary_reasoning", {}),
                "error": None
            }
            
        except asyncio.TimeoutError:
            sys.stderr.write(f"Visionary timeout for prompt: {prompt[:50]}...\n")
            
            # Return fallback preset
            fallback = self.visionary.create_intelligent_fallback(prompt)
            return {
                "success": False,
                "preset": fallback,
                "error": "timeout"
            }
            
        except Exception as e:
            sys.stderr.write(f"Visionary error: {str(e)}\n")
            sys.stderr.write(traceback.format_exc())
            
            # Return fallback preset
            fallback = self.visionary.create_intelligent_fallback(prompt)
            return {
                "success": False,
                "preset": fallback,
                "error": str(e)
            }
    
    async def run(self):
        """Main loop - read from stdin, process, write to stdout"""
        sys.stderr.write("Visionary subprocess started\n")
        
        while True:
            try:
                # Read line from stdin
                line = await asyncio.get_event_loop().run_in_executor(
                    None, sys.stdin.readline
                )
                
                if not line:
                    break
                
                # Parse request
                try:
                    request = json.loads(line.strip())
                except json.JSONDecodeError:
                    continue
                
                prompt = request.get("prompt", "")
                request_id = request.get("id", "unknown")
                
                sys.stderr.write(f"Processing request {request_id}: {prompt[:50]}...\n")
                
                # Process the request
                result = await self.process_request(prompt)
                result["id"] = request_id
                
                # Write result to stdout
                sys.stdout.write(json.dumps(result) + "\n")
                sys.stdout.flush()
                
            except Exception as e:
                sys.stderr.write(f"Subprocess error: {str(e)}\n")
                sys.stderr.write(traceback.format_exc())
                
                # Send error response
                error_response = {
                    "success": False,
                    "preset": {},
                    "error": str(e),
                    "id": "error"
                }
                sys.stdout.write(json.dumps(error_response) + "\n")
                sys.stdout.flush()

def main():
    """Entry point for subprocess"""
    processor = VisionarySubprocess()
    
    # Run async event loop
    try:
        asyncio.run(processor.run())
    except KeyboardInterrupt:
        sys.stderr.write("Visionary subprocess interrupted\n")
        sys.exit(0)
    except Exception as e:
        sys.stderr.write(f"Fatal error: {str(e)}\n")
        sys.exit(1)

if __name__ == "__main__":
    main()