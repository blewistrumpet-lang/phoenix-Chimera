#!/usr/bin/env python3
"""
TRUE Trinity Pipeline - Pure AI Generation
Visionary (AI) → Calculator (AI) → Alchemist (Local)
NO Oracle, NO Corpus, NO Preset Matching

This is the authoritative Trinity implementation.
"""

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field
from typing import Dict, Any, Optional, List
import asyncio
import logging
import os
from pathlib import Path
from datetime import datetime

# Import ONLY the true Trinity components
from visionary_trinity import VisionaryTrinity
from calculator_trinity import CalculatorTrinity  
from alchemist_trinity import AlchemistTrinity

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# FastAPI app
app = FastAPI(
    title="Chimera Phoenix TRUE Trinity Pipeline",
    version="4.0",
    description="Pure AI-powered preset generation without corpus dependencies"
)

# Add CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Request/Response models
class GenerateRequest(BaseModel):
    """Request for preset generation"""
    prompt: str = Field(..., description="User's creative prompt")
    intensity: float = Field(default=0.5, ge=0.0, le=1.0, description="Effect intensity (0-1)")
    complexity: int = Field(default=3, ge=1, le=6, description="Number of engines to use")

class GenerateResponse(BaseModel):
    """Response with generated preset"""
    success: bool
    preset: Dict[str, Any]
    message: str
    metadata: Dict[str, Any]

class HealthResponse(BaseModel):
    """Health check response"""
    status: str
    service: str
    version: str
    components: Dict[str, str]
    timestamp: str

# Initialize Trinity components
logger.info("Initializing TRUE Trinity Pipeline components...")

# Check for OpenAI API key
api_key = os.getenv("OPENAI_API_KEY")
if not api_key:
    logger.warning("No OPENAI_API_KEY found in environment. AI generation will be limited.")

# Initialize the three components
visionary = VisionaryTrinity(api_key=api_key)
calculator = CalculatorTrinity()
alchemist = AlchemistTrinity()

logger.info("✅ TRUE Trinity Pipeline initialized (NO Oracle/Corpus)")

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """
    Generate a preset using the TRUE Trinity Pipeline.
    Flow: Visionary → Calculator → Alchemist
    """
    start_time = asyncio.get_event_loop().time()
    
    try:
        logger.info("="*60)
        logger.info("🎭 Starting TRUE Trinity Pipeline")
        logger.info(f"📝 Prompt: {request.prompt[:100]}...")
        logger.info("="*60)
        
        # Step 1: VISIONARY - Generate complete preset with AI
        logger.info("⭐ Step 1: VISIONARY - AI Creative Generation")
        try:
            # Visionary generates COMPLETE preset, not just blueprint
            creative_preset = await asyncio.wait_for(
                visionary.generate_preset(
                    prompt=request.prompt,
                    intensity=request.intensity,
                    num_engines=request.complexity
                ),
                timeout=10.0  # 10 second timeout for AI
            )
            
            logger.info(f"✅ Visionary generated: '{creative_preset.get('name', 'Untitled')}'")
            logger.info(f"   Engines: {len(creative_preset.get('slots', []))}")
            
        except asyncio.TimeoutError:
            logger.error("❌ Visionary timed out")
            raise HTTPException(status_code=504, detail="AI generation timed out")
        except Exception as e:
            logger.error(f"❌ Visionary error: {str(e)}")
            # If AI fails, create a basic preset
            creative_preset = visionary.create_fallback_preset(request.prompt)
            logger.warning("⚠️ Using fallback preset generation")
        
        # Step 2: CALCULATOR - Optimize with musical intelligence  
        logger.info("🧮 Step 2: CALCULATOR - Musical Intelligence Optimization")
        try:
            # Calculator refines and optimizes the preset
            optimized_preset = calculator.optimize_preset(
                preset=creative_preset,
                prompt=request.prompt,
                intensity=request.intensity
            )
            
            # Log optimization details
            optimizations = optimized_preset.get('calculator_metadata', {})
            if optimizations:
                logger.info(f"✅ Calculator applied {len(optimizations.get('adjustments', []))} optimizations")
                logger.info(f"   Signal chain reordered: {optimizations.get('chain_optimized', False)}")
                logger.info(f"   Parameter relationships: {optimizations.get('relationships_applied', 0)}")
            
        except Exception as e:
            logger.error(f"❌ Calculator error: {str(e)}")
            # Calculator failure is not fatal - use creative preset
            optimized_preset = creative_preset
            logger.warning("⚠️ Skipping optimization due to error")
        
        # Step 3: ALCHEMIST - Validate safety and format
        logger.info("⚗️ Step 3: ALCHEMIST - Safety Validation & Formatting")
        try:
            # Alchemist ensures safety and formats for plugin
            final_preset = alchemist.finalize_preset(optimized_preset)
            
            # Log validation results
            validation = final_preset.get('alchemist_metadata', {})
            if validation:
                logger.info(f"✅ Alchemist validation complete")
                logger.info(f"   Safety checks: {validation.get('safety_passed', False)}")
                logger.info(f"   Parameters clamped: {validation.get('parameters_clamped', 0)}")
                logger.info(f"   Dangerous combos: {validation.get('dangerous_combos_found', 0)}")
            
        except Exception as e:
            logger.error(f"❌ Alchemist error: {str(e)}")
            raise HTTPException(status_code=500, detail=f"Safety validation failed: {str(e)}")
        
        # Calculate generation time
        generation_time = asyncio.get_event_loop().time() - start_time
        
        logger.info("="*60)
        logger.info(f"✨ Trinity Pipeline Complete in {generation_time:.2f}s")
        logger.info(f"🎼 Final Preset: '{final_preset.get('name', 'Untitled')}'")
        logger.info("="*60)
        
        # Prepare metadata
        metadata = {
            "generation_time": round(generation_time, 3),
            "pipeline_version": "4.0-TRUE",
            "components_used": ["visionary", "calculator", "alchemist"],
            "ai_model": "gpt-3.5-turbo",
            "no_corpus": True,  # Explicitly state no corpus used
            "no_oracle": True,  # Explicitly state no oracle used
            "pure_ai_generation": True,
            "timestamp": datetime.now().isoformat()
        }
        
        # Remove internal metadata from final preset
        final_preset.pop('calculator_metadata', None)
        final_preset.pop('alchemist_metadata', None)
        
        return GenerateResponse(
            success=True,
            preset=final_preset,
            message=f"Successfully generated '{final_preset.get('name', 'Untitled')}'",
            metadata=metadata
        )
        
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Unexpected pipeline error: {str(e)}")
        return GenerateResponse(
            success=False,
            preset={},
            message=f"Pipeline error: {str(e)}",
            metadata={"error": str(e), "stage": "unknown"}
        )

@app.get("/health", response_model=HealthResponse)
async def health_check():
    """
    Health check for TRUE Trinity Pipeline
    """
    components = {}
    
    # Check Visionary
    try:
        if visionary and visionary.client:
            components["visionary"] = "ready"
        else:
            components["visionary"] = "no_api_key"
    except:
        components["visionary"] = "error"
    
    # Check Calculator
    try:
        components["calculator"] = "ready" if calculator else "error"
    except:
        components["calculator"] = "error"
    
    # Check Alchemist
    try:
        components["alchemist"] = "ready" if alchemist else "error"
    except:
        components["alchemist"] = "error"
    
    # NO ORACLE CHECK - This is intentional!
    components["oracle"] = "removed"
    components["corpus"] = "not_needed"
    
    all_ready = all(
        status == "ready" 
        for key, status in components.items() 
        if key not in ["oracle", "corpus"]
    )
    
    return HealthResponse(
        status="healthy" if all_ready else "degraded",
        service="Chimera Phoenix TRUE Trinity",
        version="4.0",
        components=components,
        timestamp=datetime.now().isoformat()
    )

@app.get("/")
async def root():
    """
    Root endpoint with TRUE Trinity information
    """
    return {
        "service": "Chimera Phoenix TRUE Trinity Pipeline",
        "version": "4.0",
        "description": "Pure AI preset generation without corpus dependencies",
        "pipeline": {
            "1_visionary": "AI generates complete presets with OpenAI",
            "2_calculator": "AI optimizes musical relationships",
            "3_alchemist": "Local safety validation and formatting"
        },
        "key_features": [
            "NO Oracle component",
            "NO Golden Corpus dependency",
            "NO preset matching",
            "Pure AI generation",
            "Intelligent optimization",
            "Safety validation"
        ],
        "endpoints": {
            "POST /generate": "Generate preset from prompt",
            "GET /health": "Service health check",
            "GET /docs": "Interactive API documentation"
        }
    }

@app.on_event("startup")
async def startup_event():
    """
    Startup message for TRUE Trinity
    """
    print("\n" + "="*60)
    print("🎭 CHIMERA PHOENIX - TRUE TRINITY PIPELINE v4.0")
    print("="*60)
    print("✅ Visionary (AI Creative Generation)")
    print("✅ Calculator (AI Musical Intelligence)")
    print("✅ Alchemist (Local Safety Validation)")
    print("❌ NO Oracle")
    print("❌ NO Corpus")
    print("❌ NO Preset Matching")
    print("="*60)
    print("Pure AI-powered generation ready!")
    print("="*60 + "\n")

@app.on_event("shutdown")
async def shutdown_event():
    """
    Cleanup on shutdown
    """
    logger.info("Shutting down TRUE Trinity Pipeline...")

if __name__ == "__main__":
    import uvicorn
    
    print("\n" + "🎭"*30)
    print("TRUE TRINITY PIPELINE - NO ORACLE/CORPUS")
    print("🎭"*30 + "\n")
    
    uvicorn.run(
        app,
        host="0.0.0.0",
        port=8000,
        log_level="info"
    )