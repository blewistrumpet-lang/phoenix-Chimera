"""
Main API server for ChimeraPhoenix using string engine identifiers
This eliminates ALL numeric ID to choice index conversion complexity
"""

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from typing import List, Dict, Any
import asyncio
import json
import logging

from visionary_string_ids import VisionaryStringIDs
from oracle_string_ids import OracleStringIDs
from calculator_string_ids import CalculatorStringIDs
from alchemist_string_ids import AlchemistStringIDs

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI(title="Chimera Phoenix AI Server (String IDs)")

class GenerateRequest(BaseModel):
    prompt: str
    context: Dict[str, Any] = {}

class GenerateResponse(BaseModel):
    success: bool
    preset: Dict[str, Any]
    message: str = ""

# Initialize Trinity pipeline components
visionary = VisionaryStringIDs()
oracle = OracleStringIDs()
calculator = CalculatorStringIDs()
alchemist = AlchemistStringIDs()

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """
    Main endpoint - orchestrates Trinity pipeline with string IDs throughout
    NO CONVERSION NEEDED ANYWHERE!
    """
    try:
        logger.info(f"Received prompt: {request.prompt}")
        
        # Step 1: Visionary - Returns blueprint with string engine IDs
        logger.info("Step 1: Consulting Visionary...")
        blueprint = await visionary.get_blueprint(request.prompt)
        logger.info(f"Visionary blueprint (string IDs): {blueprint}")
        
        # Step 2: Oracle - Works with string IDs natively
        logger.info("Step 2: Consulting Oracle...")
        base_preset = oracle.find_best_preset(blueprint)
        logger.info(f"Oracle selected preset with string IDs")
        
        # Step 3: Calculator - Applies nudges using string IDs
        logger.info("Step 3: Consulting Calculator...")
        nudged_preset = calculator.apply_nudges(base_preset, request.prompt, blueprint)
        logger.info(f"Calculator applied nudges")
        
        # Step 4: Alchemist - Finalizes with string IDs
        logger.info("Step 4: Consulting Alchemist...")
        final_preset = alchemist.finalize_preset(nudged_preset, request.prompt)
        
        # NO CONVERSION NEEDED! 
        # The preset already has string IDs that the plugin will understand
        
        # Log the final engines for debugging
        logger.info("Final preset engines (string IDs):")
        for slot in range(1, 7):
            engine = final_preset['parameters'].get(f'slot{slot}_engine', 'bypass')
            if engine != 'bypass':
                logger.info(f"  Slot {slot}: {engine}")
        
        return GenerateResponse(
            success=True,
            preset=final_preset,
            message="Preset generated successfully with string IDs"
        )
        
    except Exception as e:
        logger.error(f"Error in generate_preset: {str(e)}")
        import traceback
        traceback.print_exc()
        return GenerateResponse(
            success=False,
            preset={},
            message=f"Error: {str(e)}"
        )

@app.get("/health")
async def health_check():
    """Health check endpoint"""
    return {
        "status": "healthy",
        "service": "Chimera Phoenix AI Server",
        "engine_format": "string_ids",
        "version": "2.0"
    }

@app.get("/")
async def root():
    """Root endpoint with version info"""
    return {
        "service": "Chimera Phoenix AI Server",
        "version": "2.0",
        "engine_format": "string_ids",
        "endpoints": ["/generate", "/health", "/engines"],
        "description": "Uses string engine identifiers throughout - no conversion needed"
    }

@app.get("/engines")
async def list_engines():
    """List all available engines with their string IDs"""
    from engine_definitions import ENGINES, CATEGORIES
    
    return {
        "total_engines": len(ENGINES),
        "categories": CATEGORIES,
        "engines": {
            key: {
                "name": info["name"],
                "category": info.get("category", "unknown"),
                "description": info.get("description", "")
            }
            for key, info in ENGINES.items()
        }
    }

@app.post("/test")
async def test_pipeline():
    """Test endpoint to verify the string-based pipeline"""
    test_prompts = [
        "warm vintage guitar tone",
        "aggressive metal sound",
        "spacious ambient pad"
    ]
    
    results = []
    for prompt in test_prompts:
        try:
            # Run through pipeline
            blueprint = await visionary.get_blueprint(prompt)
            preset = oracle.find_best_preset(blueprint)
            preset = calculator.apply_nudges(preset, prompt, blueprint)
            preset = alchemist.finalize_preset(preset, prompt)
            
            # Extract engines
            engines = []
            for slot in range(1, 7):
                engine = preset['parameters'].get(f'slot{slot}_engine', 'bypass')
                if engine != 'bypass':
                    engines.append(f"Slot {slot}: {engine}")
            
            results.append({
                "prompt": prompt,
                "preset_name": preset.get("name", "Unknown"),
                "engines": engines,
                "success": True
            })
        except Exception as e:
            results.append({
                "prompt": prompt,
                "error": str(e),
                "success": False
            })
    
    return {"test_results": results}

if __name__ == "__main__":
    import uvicorn
    print("\n" + "="*80)
    print("CHIMERA PHOENIX AI SERVER - STRING ID VERSION")
    print("="*80)
    print("This server uses string engine identifiers throughout.")
    print("No numeric ID conversion needed!")
    print("Starting server on http://localhost:8000")
    print("="*80 + "\n")
    
    uvicorn.run(app, host="0.0.0.0", port=8000)