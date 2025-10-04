#!/usr/bin/env python3
"""
Trinity Server with INTELLIGENT Calculator
Actually uses the parameter parsing we built!
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

# Import components - USE THE INTELLIGENT CALCULATOR!
from visionary_complete import CompleteVisionary
from calculator_max_intelligence import MaxIntelligenceCalculator  # The intelligent one!
from alchemist_complete import CompleteAlchemist

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("TrinityIntelligent")

# FastAPI app
app = FastAPI(title="Trinity Pipeline Server (Intelligent)")

class GenerateRequest(BaseModel):
    prompt: str
    
class GenerateResponse(BaseModel):
    preset: Dict[str, Any]
    debug: Dict[str, Any]

# Initialize components
visionary = CompleteVisionary()
calculator = MaxIntelligenceCalculator()  # The intelligent calculator!
alchemist = CompleteAlchemist()

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """Generate a preset with INTELLIGENT parameter optimization"""
    
    start_time = datetime.now()
    debug_info = {
        "prompt": request.prompt,
        "timestamp": start_time.isoformat(),
        "calculator_type": "intelligent"
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
        
        # Stage 2: INTELLIGENT Calculator - Parameter optimization WITH PROMPT PARSING!
        logger.info("🧠 INTELLIGENT Calculator parsing prompt and optimizing...")
        
        # First, parse the prompt to extract values
        extracted_values = calculator.parse_prompt_values(request.prompt)
        debug_info["calculator"] = {
            "type": "intelligent",
            "extracted_values": extracted_values
        }
        
        # Log what we extracted
        if extracted_values:
            logger.info(f"📊 Extracted {len(extracted_values)} values from prompt:")
            for key, value in extracted_values.items():
                logger.info(f"  • {value.get('original', 'N/A')}: {value.get('value', 0):.4f}")
        
        # Now optimize the preset WITH THE USER PROMPT!
        # Check if we can use async version
        if hasattr(calculator, 'optimize_parameters_max_intelligence'):
            # Use the max intelligence version with Claude if available
            optimized_preset = await calculator.optimize_parameters_max_intelligence(
                visionary_result["preset"], 
                request.prompt
            )
            debug_info["calculator"]["used_claude"] = True
        else:
            # Use the basic intelligent version (no Claude)
            # First apply extracted values
            for slot in visionary_result["preset"].get("slots", []):
                engine_id = slot.get("engine_id", 0)
                if engine_id == 0:
                    continue
                
                # Apply intelligent parameter mapping
                if engine_id == 34 and "time_subdivision" in extracted_values:  # Tape Echo
                    slot["parameters"][0]["value"] = extracted_values["time_subdivision"]["value"]
                    logger.info(f"  Set Tape Echo Time: {extracted_values['time_subdivision']['value']:.4f}")
                
                # Look for percentage values
                for key, val in extracted_values.items():
                    if "feedback" in key and engine_id == 34:
                        slot["parameters"][1]["value"] = val["value"]
                        logger.info(f"  Set Tape Echo Feedback: {val['value']:.4f}")
                    elif "mix" in key:
                        # Find mix parameter for this engine
                        engine_data = calculator.engines.get(str(engine_id), {})
                        mix_idx = engine_data.get("mix_param_index", -1)
                        if mix_idx >= 0 and mix_idx < len(slot.get("parameters", [])):
                            slot["parameters"][mix_idx]["value"] = val["value"]
                            logger.info(f"  Set {slot.get('engine_name', 'Engine')} Mix: {val['value']:.4f}")
                    elif "ratio" in key and engine_id in [1,2,3,4,5]:  # Compressors
                        slot["parameters"][1]["value"] = val["value"]  # Usually param 2
                        logger.info(f"  Set Compressor Ratio: {val['value']:.4f}")
                    elif "drive" in key and engine_id == 15:  # Tube Preamp
                        slot["parameters"][0]["value"] = val["value"]
                        logger.info(f"  Set Tube Drive: {val['value']:.4f}")
            
            optimized_preset = visionary_result["preset"]
            debug_info["calculator"]["used_claude"] = False
        
        calculator_result = {"preset": optimized_preset}
        
        # Count parameter changes
        param_changes = 0
        for slot in optimized_preset.get("slots", []):
            if slot.get("engine_id", 0) != 0:
                for param in slot.get("parameters", []):
                    if abs(param.get("value", 0.5) - 0.5) > 0.01:
                        param_changes += 1
        
        debug_info["calculator"]["parameter_changes"] = param_changes
        logger.info(f"✨ Made {param_changes} intelligent parameter changes")
        
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
        
        # Log success
        active_engines = [s for s in preset['slots'] if s['engine_id'] != 0]
        logger.info(f"🎉 Generated preset '{preset['name']}' with {len(active_engines)} engines and {param_changes} intelligent parameters")
        
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
            "calculator": "intelligent",
            "alchemist": "ready"
        },
        "features": {
            "parameter_parsing": True,
            "time_subdivisions": True,
            "percentage_extraction": True,
            "ratio_conversion": True
        },
        "timestamp": datetime.now().isoformat()
    }

@app.get("/")
async def root():
    """Root endpoint with API info"""
    return {
        "service": "Trinity Pipeline Server (Intelligent)",
        "version": "1.5.0",
        "description": "Now with intelligent parameter parsing!",
        "endpoints": {
            "/generate": "POST - Generate preset with intelligent parameters",
            "/health": "GET - Service health check"
        },
        "intelligence_features": [
            "Parses '35% feedback' → 0.35",
            "Understands '1/8 dotted' → 0.1875",
            "Converts '4:1 ratio' → normalized value",
            "Maps values to correct parameters",
            "Preserves user intent exactly"
        ]
    }

if __name__ == "__main__":
    logger.info("🚀 Starting Trinity Server with INTELLIGENT Parameter System")
    logger.info("🧠 Calculator can parse: percentages, time subdivisions, ratios, frequencies, dB")
    logger.info("✨ Parameters will be set based on user intent, not generic 0.5!")
    
    uvicorn.run(
        app,
        host="0.0.0.0",
        port=8000,
        log_level="info"
    )