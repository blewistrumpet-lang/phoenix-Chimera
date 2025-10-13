#!/usr/bin/env python3
from dotenv import load_dotenv
load_dotenv()
"""
Trinity Server with INTELLIGENT Calculator - Enhanced Progress Reporting
Provides detailed progress updates for each stage of the generation pipeline
"""

import asyncio
import json
import logging
import os
import sys
from datetime import datetime
from typing import Dict, Any, Optional
from fastapi import FastAPI, HTTPException, File, UploadFile
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import uvicorn
import tempfile
import time
from pathlib import Path
from openai import OpenAI

# Import components - USE THE INTELLIGENT CALCULATOR!
from visionary_complete import CompleteVisionary
from calculator_max_intelligence import MaxIntelligenceCalculator  # The intelligent one!
from alchemist_complete import CompleteAlchemist

# Configure logging with more detail for debugging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("TrinityIntelligent")

# Progress tracking directory
PROGRESS_DIR = Path("/tmp/trinity_progress")

# CRITICAL: API Key validation at startup
def validate_api_key():
    """Validate that OpenAI API key is present at startup"""
    api_key = os.getenv("OPENAI_API_KEY")
    if not api_key:
        logger.error("CRITICAL: OPENAI_API_KEY environment variable not set!")
        logger.error("Server will start but AI features will be degraded.")
        logger.error("Set the API key with: export OPENAI_API_KEY=your_key_here")
        raise RuntimeError(
            "OPENAI_API_KEY not found in environment. "
            "Server cannot function properly without it. "
            "Please set the environment variable and restart."
        )
    logger.info("API key validation passed")

# Validate API key before creating app
validate_api_key()

# FastAPI app
app = FastAPI(title="Trinity Pipeline Server (Intelligent)")

# CORS Configuration - Allow plugin to call API
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # In production, restrict to specific origins
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

class GenerateRequest(BaseModel):
    prompt: str
    request_id: Optional[str] = None  # Allow client to pass request_id

class GenerateResponse(BaseModel):
    preset: Dict[str, Any]
    debug: Dict[str, Any]

# Progress tracking functions
def init_progress_system():
    """Initialize progress tracking system - creates directory and cleans old files"""
    try:
        PROGRESS_DIR.mkdir(parents=True, exist_ok=True)
        logger.info(f"Progress tracking initialized at {PROGRESS_DIR}")

        # Clean up old progress files (older than 1 hour)
        cutoff_time = time.time() - 3600
        for progress_file in PROGRESS_DIR.glob("*.json"):
            try:
                if progress_file.stat().st_mtime < cutoff_time:
                    progress_file.unlink()
                    logger.debug(f"Cleaned up old progress file: {progress_file.name}")
            except Exception as e:
                logger.warning(f"Failed to clean up {progress_file.name}: {e}")

    except Exception as e:
        logger.error(f"Failed to initialize progress system: {e}")

def write_progress(request_id: str, stage: str, percent: int, message: str = "", preset_name: str = ""):
    """Write progress update to JSON file atomically with enhanced details"""
    try:
        progress_data = {
            "stage": stage,
            "percent": percent,
            "message": message,
            "preset_name": preset_name,  # Add preset name when available
            "timestamp": time.time()
        }

        progress_file = PROGRESS_DIR / f"{request_id}.json"
        temp_file = PROGRESS_DIR / f"{request_id}.json.tmp"

        # Write to temp file first, then atomic rename
        with open(temp_file, 'w') as f:
            json.dump(progress_data, f)

        temp_file.replace(progress_file)
        logger.debug(f"Progress: {request_id} - {stage} ({percent}%) {message}")

    except Exception as e:
        logger.error(f"Failed to write progress for {request_id}: {e}")

# Initialize progress system on startup
init_progress_system()

# Initialize AI components with INTELLIGENT calculator
visionary = CompleteVisionary()
calculator = MaxIntelligenceCalculator()  # The INTELLIGENT one!
alchemist = CompleteAlchemist()

# TIMEOUT FOR SAFETY (in seconds)
REQUEST_TIMEOUT = 60  # 60 seconds max for full pipeline

@app.get("/health")
async def health_check():
    """Health check endpoint for monitoring"""
    return {
        "status": "healthy",
        "timestamp": datetime.now().isoformat(),
        "components": {
            "visionary": "ready",
            "calculator": "intelligent",
            "alchemist": "ready"
        }
    }

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """Generate a preset with INTELLIGENT parameter optimization and detailed progress"""

    start_time = datetime.now()
    request_id = request.request_id or f"generate_{int(time.time() * 1000)}"
    debug_info = {
        "prompt": request.prompt,
        "timestamp": start_time.isoformat(),
        "calculator_type": "intelligent",
        "timeout_seconds": REQUEST_TIMEOUT
    }

    try:
        # Note: Timeout removed for Python 3.10 compatibility (asyncio.timeout requires 3.11+)
        logger.info(f"Starting preset generation for request {request_id}")

        # Stage 0: Initialization (0-5%)
        write_progress(request_id, "initializing", 0, "Starting Trinity AI pipeline...")
        await asyncio.sleep(0.5)  # Small delay for UI feedback
        write_progress(request_id, "initializing", 5, "Pipeline initialized")

        # Stage 1: Visionary - Creative generation (5-40%)
        write_progress(request_id, "visionary", 5, "Starting creative generation...")
        logger.info(f"Stage 1: Visionary processing: {request.prompt}")

        # Simulate gradual progress during Visionary phase
        progress_task = asyncio.create_task(simulate_visionary_progress(request_id))
        visionary_preset = await visionary.generate_complete_preset(request.prompt)
        progress_task.cancel()  # Stop simulated progress

        if not visionary_preset or visionary_preset.get("error"):
            error_msg = visionary_preset.get('error', 'Unknown error') if visionary_preset else 'No preset generated'
            logger.error(f"Visionary stage failed: {error_msg}")
            write_progress(request_id, "error", 0, f"Visionary failed: {error_msg}")
            raise HTTPException(
                status_code=500,
                detail=f"Visionary failed: {error_msg}"
            )

        # Extract preset name from Visionary output
        preset_name = visionary_preset.get('presetName', 'Generated Preset')

        # Update progress with preset name
        write_progress(request_id, "visionary", 40,
                      f"Creative generation complete: {preset_name}",
                      preset_name=preset_name)

        # Wrap for consistency
        if 'presets' not in visionary_preset:
            visionary_preset = {'presets': [visionary_preset]}

        debug_info['visionary_output'] = {
            'success': True,
            'preset_name': preset_name,
            'preset_count': len(visionary_preset['presets'])
        }

        # Stage 2: Calculator - INTELLIGENT parameter optimization (40-80%)
        write_progress(request_id, "calculator", 40,
                      f"Calculating parameters for {preset_name}...",
                      preset_name=preset_name)
        logger.info(f"Stage 2: Calculator (INTELLIGENT) processing preset: {preset_name}")

        # Simulate gradual progress during Calculator phase
        progress_task = asyncio.create_task(simulate_calculator_progress(request_id, preset_name))
        calculated_preset = await calculator.process_preset(visionary_preset['presets'][0])
        progress_task.cancel()  # Stop simulated progress

        if not calculated_preset or calculated_preset.get("error"):
            error_msg = calculated_preset.get('error', 'Unknown error') if calculated_preset else 'No parameters calculated'
            logger.error(f"Calculator stage failed: {error_msg}")
            write_progress(request_id, "error", 0, f"Calculator failed: {error_msg}")
            raise HTTPException(
                status_code=500,
                detail=f"Calculator failed: {error_msg}"
            )

        write_progress(request_id, "calculator", 80,
                      f"Parameters calculated for {preset_name}",
                      preset_name=preset_name)
        debug_info['calculator_output'] = {
            'success': True,
            'parameter_count': len(calculated_preset.get('engines', [])) * 4  # Approximate
        }

        # Stage 3: Alchemist - Format for plugin (80-100%)
        write_progress(request_id, "alchemist", 80,
                      f"Formatting preset {preset_name} for plugin...",
                      preset_name=preset_name)
        logger.info(f"Stage 3: Alchemist processing preset: {preset_name}")

        final_preset = await alchemist.format_complete_preset(calculated_preset)

        if not final_preset or final_preset.get("error"):
            error_msg = final_preset.get('error', 'Unknown error') if final_preset else 'No preset formatted'
            logger.error(f"Alchemist stage failed: {error_msg}")
            write_progress(request_id, "error", 0, f"Alchemist failed: {error_msg}")
            raise HTTPException(
                status_code=500,
                detail=f"Alchemist failed: {error_msg}"
            )

        write_progress(request_id, "complete", 100,
                      f"Preset '{preset_name}' generated successfully!",
                      preset_name=preset_name)
        debug_info['alchemist_output'] = {
            'success': True,
            'preset_complete': True
        }

        # Calculate total time
        total_time = (datetime.now() - start_time).total_seconds()
        debug_info['total_time_seconds'] = total_time

        logger.info(f"✓ Preset generation complete in {total_time:.2f}s: {preset_name}")
        logger.info(f"  Visionary: Creative generation complete")
        logger.info(f"  Calculator: INTELLIGENT parameter optimization complete")
        logger.info(f"  Alchemist: Plugin formatting complete")

        return GenerateResponse(
            preset=final_preset,
            debug=debug_info
        )

    except asyncio.TimeoutError:
        logger.error("Request timeout after 60 seconds")
        write_progress(request_id, "error", 0, "Request timed out")
        raise HTTPException(
            status_code=504,
            detail="Request timed out - Trinity took too long to generate"
        )
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Unexpected error in generate_preset: {e}", exc_info=True)
        write_progress(request_id, "error", 0, f"Unexpected error: {str(e)}")
        raise HTTPException(
            status_code=500,
            detail=f"Internal server error: {str(e)}"
        )

async def simulate_visionary_progress(request_id: str):
    """Simulate gradual progress during Visionary phase"""
    try:
        for percent in range(10, 40, 5):
            await asyncio.sleep(1.5)
            write_progress(request_id, "visionary", percent, "Creative generation in progress...")
    except asyncio.CancelledError:
        pass

async def simulate_calculator_progress(request_id: str, preset_name: str):
    """Simulate gradual progress during Calculator phase"""
    try:
        for percent in range(45, 80, 5):
            await asyncio.sleep(1.2)
            write_progress(request_id, "calculator", percent,
                          f"Calculating parameters for {preset_name}...",
                          preset_name=preset_name)
    except asyncio.CancelledError:
        pass

@app.post("/transcribe")
async def transcribe_audio(audio: UploadFile = File(...)):
    """
    Transcribe audio to text using Whisper API
    This endpoint comes FIRST in the voice→preset pipeline
    """
    request_id = f"transcribe_{int(time.time() * 1000)}"

    try:
        # Save uploaded audio to temp file
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tmp_file:
            content = await audio.read()
            tmp_file.write(content)
            tmp_path = tmp_file.name

        # Create OpenAI client
        client = OpenAI()

        # Transcribe using Whisper
        with open(tmp_path, "rb") as audio_file:
            transcript = client.audio.transcriptions.create(
                model="whisper-1",
                file=audio_file
            )

        # Clean up temp file
        os.unlink(tmp_path)

        return {
            "text": transcript.text,
            "request_id": request_id
        }

    except Exception as e:
        logger.error(f"Transcription failed: {e}")
        raise HTTPException(status_code=500, detail=f"Transcription failed: {str(e)}")

@app.get("/progress/{request_id}")
async def get_progress(request_id: str):
    """Get current progress for a request"""
    progress_file = PROGRESS_DIR / f"{request_id}.json"

    if not progress_file.exists():
        return {"stage": "unknown", "percent": 0, "message": "No progress data"}

    try:
        with open(progress_file, 'r') as f:
            return json.load(f)
    except Exception as e:
        logger.error(f"Failed to read progress for {request_id}: {e}")
        return {"stage": "error", "percent": 0, "message": "Failed to read progress"}

if __name__ == "__main__":
    # Important: Don't reload in production, it causes issues with async
    uvicorn.run(app, host="0.0.0.0", port=8000, reload=False)