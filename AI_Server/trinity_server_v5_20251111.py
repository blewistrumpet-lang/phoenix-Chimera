#!/usr/bin/env python3
"""
Trinity Server v5.0 - November 11, 2025
Ultimate Edition with Three-Tier Intelligence

Components:
- Visionary: visionary_hybrid.py (Three-tier classification, GPT-4o)
- Calculator: calculator_gpt4o_v5.py (GPT-4o-mini intelligence)
- Alchemist: alchemist_trinity.py (Local validation)

Features:
- Three-tier prompt classification (Literal → Poetic)
- Real-time progress tracking (/tmp/trinity_progress/)
- GPT-4o-mini for fast parameter optimization
- GPT-4o for creative engine selection
- Local safety validation (no API)
- Whisper transcription
"""

from dotenv import load_dotenv
load_dotenv()

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

# Import Trinity components
from visionary_hybrid import HybridVisionary
from calculator_gpt4o_v5 import GPT4oCalculator
from alchemist_trinity import AlchemistTrinity

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("TrinityV5")

# Progress tracking directory
PROGRESS_DIR = Path("/tmp/trinity_progress")

# API Key validation at startup
def validate_api_key():
    """Validate that OpenAI API key is present at startup"""
    api_key = os.getenv("OPENAI_API_KEY")
    if not api_key:
        logger.error("CRITICAL: OPENAI_API_KEY environment variable not set!")
        raise RuntimeError("OPENAI_API_KEY not found in environment")
    logger.info("✅ OpenAI API key validated")
    return api_key

# Validate immediately
OPENAI_API_KEY = validate_api_key()

# Initialize OpenAI client for Whisper
openai_client = OpenAI(api_key=OPENAI_API_KEY)

# Initialize Trinity components
logger.info("🎭 Initializing Trinity v5.0 Components...")

try:
    visionary = HybridVisionary()
    logger.info("✅ Visionary (Hybrid Three-Tier) initialized")
except Exception as e:
    logger.error(f"❌ Failed to initialize Visionary: {e}")
    sys.exit(1)

try:
    calculator = GPT4oCalculator()
    logger.info("✅ Calculator (GPT-4o) initialized")
except Exception as e:
    logger.error(f"❌ Failed to initialize Calculator: {e}")
    sys.exit(1)

try:
    alchemist = AlchemistTrinity()
    logger.info("✅ Alchemist (Local Validation) initialized")
except Exception as e:
    logger.error(f"❌ Failed to initialize Alchemist: {e}")
    sys.exit(1)

logger.info("🎉 Trinity v5.0 fully initialized!")

# Progress tracking functions
def init_progress_system():
    """Initialize progress tracking system"""
    try:
        PROGRESS_DIR.mkdir(parents=True, exist_ok=True)

        # Clean up old progress files (older than 1 hour)
        cutoff_time = time.time() - 3600
        for progress_file in PROGRESS_DIR.glob("*.json"):
            try:
                if progress_file.stat().st_mtime < cutoff_time:
                    progress_file.unlink()
                    logger.debug(f"Cleaned up old progress file: {progress_file.name}")
            except Exception as e:
                logger.warning(f"Failed to clean up {progress_file.name}: {e}")

        logger.info(f"✅ Progress tracking initialized at {PROGRESS_DIR}")
    except Exception as e:
        logger.error(f"Failed to initialize progress system: {e}")

def write_progress(request_id: str, stage: str, percent: int, message: str = "", preset_name: str = ""):
    """Write progress update to JSON file atomically"""
    try:
        progress_data = {
            "request_id": request_id,
            "stage": stage,
            "percent": percent,
            "message": message,
            "preset_name": preset_name,
            "timestamp": datetime.now().isoformat()
        }

        progress_file = PROGRESS_DIR / f"{request_id}.json"

        # Atomic write via temp file
        temp_file = progress_file.with_suffix('.tmp')
        with temp_file.open('w') as f:
            json.dump(progress_data, f)
        temp_file.replace(progress_file)

        logger.debug(f"Progress: {stage} {percent}% - {message}")
    except Exception as e:
        logger.error(f"Failed to write progress for {request_id}: {e}")

# Initialize progress system on startup
init_progress_system()

# FastAPI app
app = FastAPI(title="Trinity v5.0 - Ultimate Edition", version="5.0.20251111")

# CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Request/Response models
class GenerateRequest(BaseModel):
    prompt: str
    session: Optional[str] = ""
    request_id: Optional[str] = ""  # Plugin sends this for progress tracking

class GenerateResponse(BaseModel):
    success: bool
    preset: Optional[Dict] = None
    message: str = ""
    request_id: str = ""
    tier: Optional[int] = None

class HealthResponse(BaseModel):
    status: str
    service: str
    version: str
    components: Dict[str, str]
    timestamp: str

# Endpoints

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """Generate preset using Trinity v5.0 Pipeline with progress tracking"""
    # Use request_id from plugin if provided, otherwise generate one
    request_id = request.request_id or request.session or f"req_{int(time.time() * 1000)}"

    try:
        logger.info(f"🎯 [{request_id}] Generate request: '{request.prompt}'")

        # Initialize progress
        write_progress(request_id, "initializing", 0, "Starting Trinity v5.0 pipeline...")

        # STAGE 1: VISIONARY (Three-Tier Engine Selection)
        write_progress(request_id, "visionary", 5, "Analyzing prompt and selecting engines...")

        visionary_result = await visionary.generate_preset(request.prompt)

        if not visionary_result or "error" in visionary_result:
            error_msg = visionary_result.get("error", "Unknown error") if visionary_result else "No response"
            write_progress(request_id, "error", 0, f"Visionary failed: {error_msg}")
            return GenerateResponse(
                success=False,
                message=f"Visionary stage failed: {error_msg}",
                request_id=request_id
            )

        preset = visionary_result
        tier_used = visionary_result.get("tier", 2)

        write_progress(request_id, "visionary", 40,
                      f"Engines selected (Tier {tier_used}): {preset.get('name', 'Unnamed')}")

        # STAGE 2: CALCULATOR (GPT-4o Parameter Optimization)
        write_progress(request_id, "calculator", 40,
                      "Optimizing parameters with GPT-4o intelligence...")

        try:
            preset = await calculator.optimize_parameters_max_intelligence(preset, request.prompt)
        except Exception as calc_error:
            logger.warning(f"⚠️ Calculator error (continuing anyway): {calc_error}")
            write_progress(request_id, "calculator", 70, "Using basic parameters")

        write_progress(request_id, "calculator", 80,
                      f"Parameters optimized: {preset.get('name', 'Unnamed')}")

        # STAGE 3: ALCHEMIST (Local Safety Validation)
        write_progress(request_id, "alchemist", 80,
                      "Validating safety and applying professional polish...")

        preset = alchemist.validate_and_optimize(preset)

        write_progress(request_id, "complete", 100,
                      f"✅ Complete!", preset_name=preset.get("name", "Unnamed"))

        logger.info(f"✅ [{request_id}] Generated: {preset.get('name', 'Unnamed')} (Tier {tier_used})")

        return GenerateResponse(
            success=True,
            preset=preset,
            message=f"Generated: {preset.get('name', 'Unnamed')}",
            request_id=request_id,
            tier=tier_used
        )

    except Exception as e:
        logger.error(f"❌ [{request_id}] Generation failed: {str(e)}")
        write_progress(request_id, "error", 0, f"Error: {str(e)}")
        return GenerateResponse(
            success=False,
            message=f"Generation failed: {str(e)}",
            request_id=request_id
        )

@app.post("/transcribe")
async def transcribe_audio(audio: UploadFile = File(...)):
    """Transcribe audio using OpenAI Whisper"""
    try:
        logger.info(f"🎤 Transcription request: {audio.filename}")

        # Save uploaded file temporarily
        with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as temp_file:
            content = await audio.read()
            temp_file.write(content)
            temp_path = temp_file.name

        try:
            # Send to Whisper API
            with open(temp_path, "rb") as audio_file:
                transcript = openai_client.audio.transcriptions.create(
                    model="whisper-1",
                    file=audio_file
                )

            transcribed_text = transcript.text.strip()
            logger.info(f"✅ Transcribed: '{transcribed_text}'")

            return {
                "success": True,
                "text": transcribed_text,
                "message": "Audio transcribed successfully"
            }

        finally:
            # Clean up temp file
            if os.path.exists(temp_path):
                os.unlink(temp_path)

    except Exception as e:
        logger.error(f"❌ Transcription error: {str(e)}")
        return {
            "success": False,
            "text": "",
            "message": f"Transcription failed: {str(e)}"
        }

@app.get("/progress/{request_id}")
async def get_progress(request_id: str):
    """Get current progress for a generation request"""
    try:
        progress_file = PROGRESS_DIR / f"{request_id}.json"

        if progress_file.exists():
            with progress_file.open('r') as f:
                progress_data = json.load(f)
            return progress_data
        else:
            return {
                "request_id": request_id,
                "stage": "unknown",
                "percent": 0,
                "message": "No progress data found"
            }
    except Exception as e:
        logger.error(f"Error reading progress: {e}")
        return {
            "request_id": request_id,
            "stage": "error",
            "percent": 0,
            "message": f"Error: {str(e)}"
        }

@app.get("/health", response_model=HealthResponse)
async def health_check():
    """Health check endpoint"""
    return HealthResponse(
        status="healthy",
        service="Chimera Phoenix Trinity v5.0",
        version="5.0.20251111",
        components={
            "visionary": "HybridVisionary (Three-Tier, GPT-4o)",
            "calculator": "GPT4oCalculator (GPT-4o-mini)",
            "alchemist": "AlchemistTrinity (Local)",
            "progress": "File-based realtime",
            "whisper": "OpenAI Whisper-1"
        },
        timestamp=datetime.now().isoformat()
    )

@app.get("/")
async def root():
    """Root endpoint with service info"""
    return {
        "service": "Trinity v5.0 Ultimate Edition",
        "version": "5.0.20251111",
        "status": "operational",
        "features": [
            "Three-tier prompt classification",
            "GPT-4o creative engine selection",
            "GPT-4o-mini parameter optimization",
            "Real-time progress tracking",
            "Local safety validation",
            "OpenAI Whisper transcription"
        ],
        "endpoints": {
            "/generate": "POST - Generate preset from text prompt",
            "/transcribe": "POST - Transcribe audio to text",
            "/progress/{id}": "GET - Get generation progress",
            "/health": "GET - Service health check"
        }
    }

# Startup event
@app.on_event("startup")
async def startup_event():
    logger.info("=" * 60)
    logger.info("🎭 TRINITY SERVER v5.0 - ULTIMATE EDITION")
    logger.info("=" * 60)
    logger.info("Date: November 11, 2025")
    logger.info("")
    logger.info("Components:")
    logger.info("  🎨 Visionary: Three-Tier Classification (GPT-4o)")
    logger.info("  🧠 Calculator: Maximum Intelligence (GPT-4o-mini)")
    logger.info("  ⚗️  Alchemist: Local Safety Validation")
    logger.info("  📊 Progress: Real-time file-based tracking")
    logger.info("  🎤 Whisper: OpenAI transcription")
    logger.info("")
    logger.info("Endpoints:")
    logger.info("  POST /generate - Generate preset from prompt")
    logger.info("  POST /transcribe - Transcribe audio file")
    logger.info("  GET  /progress/{id} - Check generation progress")
    logger.info("  GET  /health - Service health check")
    logger.info("")
    logger.info("=" * 60)
    logger.info("🚀 Server ready at http://localhost:8000")
    logger.info("📚 API docs at http://localhost:8000/docs")
    logger.info("=" * 60)

if __name__ == "__main__":
    uvicorn.run(
        app,
        host="0.0.0.0",
        port=8000,
        log_level="info"
    )
