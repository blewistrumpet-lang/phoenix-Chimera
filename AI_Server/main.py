from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from typing import List, Dict, Any
import asyncio
import json
import logging

from visionary_openai_direct import VisionaryOpenAIDirect as VisionaryClient
from oracle_faiss import OracleFAISS as Oracle
from calculator import Calculator
from alchemist import Alchemist

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI(title="Chimera Phoenix AI Server")

class GenerateRequest(BaseModel):
    prompt: str
    context: Dict[str, Any] = {}

class GenerateResponse(BaseModel):
    success: bool
    preset: Dict[str, Any]
    message: str = ""

# Initialize services
visionary = VisionaryClient()
oracle = Oracle()
calculator = Calculator()
alchemist = Alchemist()

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """
    Main endpoint that orchestrates the Visionary -> Oracle -> Calculator -> Alchemist pipeline
    """
    try:
        logger.info(f"Received prompt: {request.prompt}")
        
        # Step 1: Visionary - Get creative blueprint from OpenAI
        logger.info("Step 1: Consulting Visionary...")
        blueprint = await visionary.get_blueprint(request.prompt)
        logger.info(f"Visionary blueprint: {blueprint}")
        
        # Step 2: Oracle - Find best matching preset from corpus
        logger.info("Step 2: Consulting Oracle...")
        base_preset = oracle.find_best_preset(blueprint)
        logger.info(f"Oracle selected preset: {base_preset}")
        
        # Step 3: Calculator - Apply intelligent nudges based on prompt
        logger.info("Step 3: Consulting Calculator...")
        nudged_preset = calculator.apply_nudges(base_preset, request.prompt, blueprint)
        logger.info(f"Calculator nudged preset: {nudged_preset}")
        
        # Step 4: Alchemist - Final validation and safety checks
        logger.info("Step 4: Consulting Alchemist...")
        final_preset = alchemist.finalize_preset(nudged_preset)
        logger.info(f"Alchemist final preset: {final_preset}")
        
        return GenerateResponse(
            success=True,
            preset=final_preset,
            message="Preset generated successfully"
        )
        
    except Exception as e:
        logger.error(f"Error in generate_preset: {str(e)}")
        return GenerateResponse(
            success=False,
            preset={},
            message=f"Error: {str(e)}"
        )

@app.get("/health")
async def health_check():
    """Health check endpoint"""
    return {"status": "healthy", "service": "Chimera Phoenix AI Server"}

@app.get("/")
async def root():
    """Root endpoint"""
    return {
        "service": "Chimera Phoenix AI Server",
        "version": "3.0",
        "endpoints": ["/generate", "/health"]
    }

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)