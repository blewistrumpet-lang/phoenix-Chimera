#!/usr/bin/env python3
"""
Test OpenAI connection through the bridge server
"""

import asyncio
import json
import logging
import subprocess
import time
from visionary_enhanced import VisionaryEnhanced

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

async def test_openai_connection():
    """Test the OpenAI connection through the bridge server"""
    
    # Start the OpenAI bridge server in the background
    logger.info("Starting OpenAI bridge server...")
    server_process = subprocess.Popen(
        ["python3", "openai_bridge_server.py"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    # Wait for server to start
    await asyncio.sleep(2)
    
    try:
        # Create Visionary client
        visionary = VisionaryEnhanced()
        
        # Test prompts
        test_prompts = [
            "Create a warm vintage guitar tone with tape saturation and plate reverb",
            "Design an aggressive modern metal sound with tight compression",
            "Build a spacious ambient pad with shimmer reverb and granular textures"
        ]
        
        logger.info("\n" + "="*80)
        logger.info("TESTING OPENAI CONNECTION")
        logger.info("="*80)
        
        for i, prompt in enumerate(test_prompts):
            logger.info(f"\nTest {i+1}: {prompt}")
            logger.info("-"*80)
            
            try:
                # Get blueprint from Visionary (will try OpenAI first)
                blueprint = await visionary.get_blueprint(prompt)
                
                # Check if we got a real OpenAI response or simulation
                if "error" in blueprint:
                    logger.error(f"Error: {blueprint['error']}")
                else:
                    logger.info(f"Success! Blueprint received:")
                    logger.info(f"Overall vibe: {blueprint.get('overall_vibe', 'Unknown')}")
                    
                    # Show active engines
                    active_slots = []
                    for slot in blueprint.get('slots', []):
                        if slot.get('engine_id', -1) >= 0:
                            engine_name = visionary.engines.get(slot['engine_id'], {}).get('name', 'Unknown')
                            active_slots.append(f"Slot {slot['slot']}: {engine_name} ({slot['character']})")
                    
                    if active_slots:
                        logger.info("Active engines:")
                        for slot_info in active_slots:
                            logger.info(f"  - {slot_info}")
                    
                    # Pretty print the full blueprint
                    logger.info("\nFull blueprint:")
                    logger.info(json.dumps(blueprint, indent=2))
                    
            except Exception as e:
                logger.error(f"Test failed: {str(e)}")
            
            # Small delay between tests
            await asyncio.sleep(1)
            
    finally:
        # Terminate the server
        logger.info("\nStopping OpenAI bridge server...")
        server_process.terminate()
        server_process.wait()

async def quick_api_test():
    """Quick test of OpenAI API directly"""
    logger.info("\n" + "="*80)
    logger.info("QUICK OPENAI API TEST")
    logger.info("="*80)
    
    try:
        from openai import OpenAI
        import os
        
        # Get API key
        api_key = os.getenv("OPENAI_API_KEY", "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")
        
        # Create client
        client = OpenAI(api_key=api_key)
        
        # Simple test
        response = client.chat.completions.create(
            model="gpt-3.5-turbo",  # Using 3.5 for faster response
            messages=[
                {"role": "system", "content": "You are a helpful assistant. Respond with a simple JSON object."},
                {"role": "user", "content": "Return a JSON object with a 'status' field set to 'connected' and a 'message' field."}
            ],
            temperature=0.7,
            max_tokens=100
        )
        
        content = response.choices[0].message.content
        logger.info(f"OpenAI API Response: {content}")
        
        # Try to parse as JSON
        try:
            data = json.loads(content)
            logger.info("✓ Successfully connected to OpenAI API")
            logger.info(f"  Status: {data.get('status', 'Unknown')}")
            logger.info(f"  Message: {data.get('message', 'No message')}")
        except:
            logger.info("✓ Connected to OpenAI API (response was not JSON)")
            
    except Exception as e:
        logger.error(f"✗ Failed to connect to OpenAI API: {str(e)}")
        logger.error("  Please check your API key and internet connection")

async def main():
    """Run all tests"""
    # First do a quick API test
    await quick_api_test()
    
    # Then test through the bridge server
    await test_openai_connection()

if __name__ == "__main__":
    asyncio.run(main())