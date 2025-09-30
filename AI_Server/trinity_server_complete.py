#!/usr/bin/env python3
"""
Trinity Server with Complete Components
Using single source of truth for engine knowledge
"""

import asyncio
import json
import logging
from datetime import datetime
from typing import Dict, Any, Optional
from fastapi import FastAPI, HTTPException
from fastapi.responses import JSONResponse
from pydantic import BaseModel
import uvicorn

# Import the complete versions that use single source of truth
from visionary_complete import CompleteVisionary
from calculator_complete import CompleteCalculator  
from alchemist_complete import CompleteAlchemist

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("TrinityServerComplete")

# FastAPI app
app = FastAPI(title="Trinity Pipeline Server (Complete)")

class GenerateRequest(BaseModel):
    prompt: str
    
class GenerateResponse(BaseModel):
    preset: Dict[str, Any]
    debug: Dict[str, Any]

# Initialize components
visionary = CompleteVisionary()
calculator = CompleteCalculator()
alchemist = CompleteAlchemist()

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """Generate a preset from a text prompt using the complete Trinity pipeline"""
    
    start_time = datetime.now()
    debug_info = {
        "prompt": request.prompt,
        "timestamp": start_time.isoformat()
    }
    
    try:
        # Stage 1: Visionary - Creative generation
        logger.info(f"Visionary processing: {request.prompt}")
        visionary_preset = await visionary.generate_complete_preset(request.prompt)
        
        if not visionary_preset or visionary_preset.get("error"):
            raise HTTPException(
                status_code=500,
                detail=f"Visionary failed: {visionary_preset.get('error', 'Unknown error')}"
            )
        
        # Wrap the preset for consistency
        visionary_result = {"preset": visionary_preset}
        
        debug_info["visionary"] = {
            "preset_name": visionary_preset["name"],
            "engine_count": len([s for s in visionary_preset["slots"] if s["engine_id"] != 0]),
            "reasoning": visionary_preset.get("reasoning", {})
        }
        
        # Stage 2: Calculator - INTELLIGENT Parameter optimization
        logger.info("Calculator optimizing parameters with user intent...")
        optimized_preset = calculator.optimize_preset(visionary_result["preset"], request.prompt)
        calculator_result = {"preset": optimized_preset}
        
        if not calculator_result or calculator_result.get("error"):
            # Fall back to visionary result if calculator fails
            logger.warning(f"Calculator failed: {calculator_result.get('error', 'Unknown')}, using Visionary result")
            calculator_result = visionary_result
        else:
            debug_info["calculator"] = {
                "optimizations_made": calculator_result.get("optimizations", []),
                "reasoning": calculator_result.get("reasoning", {})
            }
        
        # Stage 3: Alchemist - Final validation
        logger.info("Alchemist validating preset...")
        validated_preset, validation_report = alchemist.validate_and_fix(calculator_result["preset"])
        final_preset = {"preset": validated_preset, "validation": validation_report}
        
        debug_info["alchemist"] = {
            "validation_passed": final_preset.get("valid", True),
            "refinements": final_preset.get("refinements", [])
        }
        
        # Ensure we return the preset structure
        if "preset" in final_preset:
            preset = final_preset["preset"]
        else:
            preset = final_preset
            
        # Add timing
        duration = (datetime.now() - start_time).total_seconds()
        debug_info["processing_time_seconds"] = duration
        
        # Log success
        logger.info(f"Successfully generated preset '{preset['name']}' with {len([s for s in preset['slots'] if s['engine_id'] != 0])} engines")
        
        return GenerateResponse(
            preset=preset,
            debug=debug_info
        )
        
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Unexpected error: {str(e)}", exc_info=True)
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/health")
async def health():
    """Health check endpoint"""
    return {
        "status": "healthy",
        "components": {
            "visionary": "ready",
            "calculator": "ready", 
            "alchemist": "ready"
        },
        "timestamp": datetime.now().isoformat()
    }

@app.get("/")
async def root():
    """Root endpoint with API info"""
    return {
        "service": "Trinity Pipeline Server (Complete)",
        "version": "1.0.0",
        "endpoints": {
            "/generate": "POST - Generate preset from prompt",
            "/health": "GET - Service health check"
        }
    }

if __name__ == "__main__":
    logger.info("Starting Trinity Server (Complete) on port 8000...")
    logger.info("Using single source of truth: trinity_engine_knowledge_COMPLETE.json")
    logger.info("4-engine minimum enforced in Visionary prompts")
    
    uvicorn.run(
        app,
        host="0.0.0.0",
        port=8000,
        log_level="info"
    )