#!/usr/bin/env python3
"""
Trinity Server with Maximum Intelligence Calculator
Uses Claude for deep parameter optimization
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

# Import components
from visionary_complete import CompleteVisionary
from calculator_max_intelligence import MaxIntelligenceCalculator
from alchemist_complete import CompleteAlchemist

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("TrinityMaxServer")

# FastAPI app
app = FastAPI(title="Trinity Pipeline Server (Maximum Intelligence)")

class GenerateRequest(BaseModel):
    prompt: str
    
class GenerateResponse(BaseModel):
    preset: Dict[str, Any]
    debug: Dict[str, Any]

# Initialize components
visionary = CompleteVisionary()
calculator = MaxIntelligenceCalculator()
alchemist = CompleteAlchemist()

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """Generate a preset with maximum intelligence parameter optimization"""
    
    start_time = datetime.now()
    debug_info = {
        "prompt": request.prompt,
        "timestamp": start_time.isoformat(),
        "mode": "maximum_intelligence"
    }
    
    try:
        # Stage 1: Visionary - Creative generation
        logger.info(f"🎨 Visionary processing: {request.prompt}")
        visionary_preset = await visionary.generate_complete_preset(request.prompt)
        
        if not visionary_preset or visionary_preset.get("error"):
            raise HTTPException(
                status_code=500,
                detail=f"Visionary failed: {visionary_preset.get('error', 'Unknown error')}"
            )
        
        # Wrap for consistency
        visionary_result = {"preset": visionary_preset}
        
        debug_info["visionary"] = {
            "preset_name": visionary_preset["name"],
            "engine_count": len([s for s in visionary_preset["slots"] if s["engine_id"] != 0]),
            "engines_selected": [
                {"id": s["engine_id"], "name": s.get("engine_name", "Unknown")} 
                for s in visionary_preset["slots"] if s["engine_id"] != 0
            ]
        }
        
        # Stage 2: Calculator - Maximum intelligence parameter optimization
        logger.info("🧠 Calculator applying maximum intelligence...")
        
        # Use async maximum intelligence optimization
        optimized_preset = await calculator.optimize_parameters_max_intelligence(
            visionary_result["preset"], 
            request.prompt
        )
        calculator_result = {"preset": optimized_preset}
        
        # Track what optimizations were made
        debug_info["calculator"] = {
            "mode": "maximum_intelligence",
            "used_claude": True,
            "cache_stats": calculator.intelligence_cache.get("statistics", {}),
            "extracted_values": calculator.parse_prompt_values(request.prompt)
        }
        
        # Stage 3: Alchemist - Final validation
        logger.info("✅ Alchemist validating preset...")
        validated_preset, validation_report = alchemist.validate_and_fix(calculator_result["preset"])
        final_preset = {"preset": validated_preset, "validation": validation_report}
        
        debug_info["alchemist"] = {
            "validation_passed": validation_report.get("valid", True),
            "issues_fixed": len(validation_report.get("issues", [])),
            "warnings": validation_report.get("warnings", [])
        }
        
        # Ensure we return the preset structure
        if "preset" in final_preset:
            preset = final_preset["preset"]
        else:
            preset = final_preset
        
        # Add timing
        duration = (datetime.now() - start_time).total_seconds()
        debug_info["processing_time_seconds"] = duration
        
        # Log success with parameter intelligence
        active_engines = [s for s in preset['slots'] if s['engine_id'] != 0]
        param_changes = 0
        for slot in active_engines:
            for param in slot.get('parameters', []):
                if abs(param.get('value', 0.5) - 0.5) > 0.01:
                    param_changes += 1
        
        logger.info(f"✨ Generated preset '{preset['name']}' with {len(active_engines)} engines")
        logger.info(f"🎯 Intelligently set {param_changes} parameter values")
        
        return GenerateResponse(
            preset=preset,
            debug=debug_info
        )
        
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"❌ Unexpected error: {str(e)}", exc_info=True)
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/health")
async def health():
    """Health check endpoint"""
    return {
        "status": "healthy",
        "components": {
            "visionary": "ready",
            "calculator": "maximum_intelligence",
            "alchemist": "ready"
        },
        "features": {
            "claude_integration": True,
            "parameter_parsing": True,
            "style_analysis": True,
            "relationship_optimization": True,
            "creative_enhancement": True,
            "caching": True
        },
        "timestamp": datetime.now().isoformat()
    }

@app.get("/cache_stats")
async def cache_stats():
    """Get intelligence cache statistics"""
    stats = calculator.intelligence_cache.get("statistics", {})
    return {
        "cache_hits": stats.get("cache_hits", 0),
        "claude_calls": stats.get("claude_calls", 0),
        "total_tokens_used": stats.get("total_tokens", 0),
        "cached_styles": len(calculator.intelligence_cache.get("style_parameters", {})),
        "efficiency": f"{stats.get('cache_hits', 0) / max(1, stats.get('cache_hits', 0) + stats.get('claude_calls', 0)) * 100:.1f}%"
    }

@app.get("/")
async def root():
    """Root endpoint with API info"""
    return {
        "service": "Trinity Pipeline Server (Maximum Intelligence)",
        "version": "2.0.0",
        "description": "Enhanced with Claude-powered parameter intelligence",
        "endpoints": {
            "/generate": "POST - Generate preset with max intelligence",
            "/health": "GET - Service health check",
            "/cache_stats": "GET - Intelligence cache statistics"
        },
        "features": [
            "Parse specific values from prompts (35% → 0.35)",
            "Claude musical style analysis",
            "Parameter relationship optimization",
            "Creative enhancement suggestions",
            "Intelligent caching system"
        ]
    }

if __name__ == "__main__":
    logger.info("🚀 Starting Trinity Server with Maximum Intelligence")
    logger.info("🧠 Claude integration enabled for parameter optimization")
    logger.info("📊 Intelligent caching enabled to reduce API calls")
    logger.info("✨ This server provides the highest quality parameter settings")
    
    uvicorn.run(
        app,
        host="0.0.0.0",
        port=8000,
        log_level="info"
    )