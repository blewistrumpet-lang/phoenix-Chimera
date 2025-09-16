"""
Chimera Phoenix AI Server - Trinity Pipeline Implementation
Orchestrates: Visionary → Oracle → Calculator → Alchemist
"""

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
from typing import List, Dict, Any, Optional
import asyncio
import json
import logging
from pathlib import Path

# Import refactored Trinity components
from cloud_bridge import get_cloud_generation  # Cloud-first AI with fallback
from oracle_faiss import OracleFAISS as Oracle
from calculator import Calculator
from alchemist import Alchemist

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

app = FastAPI(
    title="Chimera Phoenix AI Server",
    version="3.0",
    description="Trinity AI Pipeline for intelligent preset generation"
)

# Import and include plugin compatibility endpoints
try:
    from plugin_endpoints import plugin_router
    app.include_router(plugin_router)
    logger.info("Plugin compatibility endpoints loaded")
except ImportError as e:
    logger.warning(f"Could not load plugin endpoints: {e}")

class GenerateRequest(BaseModel):
    """Request model for preset generation"""
    prompt: str = Field(..., description="User's creative prompt")
    context: Dict[str, Any] = Field(default_factory=dict, description="Optional context")
    max_generation_time: int = Field(default=30, description="Max generation time in seconds")

class GenerateResponse(BaseModel):
    """Response model for preset generation"""
    success: bool = Field(..., description="Whether generation succeeded")
    preset: Dict[str, Any] = Field(..., description="Generated preset data")
    message: str = Field(default="", description="Status or error message")
    metadata: Optional[Dict[str, Any]] = Field(default=None, description="Generation metadata")

class HealthResponse(BaseModel):
    """Health check response"""
    status: str
    service: str
    version: str
    components: Dict[str, str]

# Initialize Trinity Pipeline services
logger.info("Initializing Trinity Pipeline components...")

# Oracle - FAISS-powered preset matching
oracle = Oracle(
    index_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus.index",
    meta_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata.json",
    presets_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets.json"
)

# Calculator - Sophisticated nudge system
calculator = Calculator(
    rules_path="nudge_rules.json"
)

# Alchemist - Final validation and safety
alchemist = Alchemist()

logger.info("Trinity Pipeline initialized successfully")

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """
    Main endpoint orchestrating the Trinity Pipeline.
    Flow: Visionary → Oracle → Calculator → Alchemist
    """
    start_time = asyncio.get_event_loop().time()
    
    try:
        logger.info(f"═══ Starting Trinity Pipeline ═══")
        logger.info(f"Prompt: {request.prompt[:100]}...")
        
        # Step 1: CLOUD/VISIONARY - Generate creative blueprint
        logger.info("│ Step 1: CLOUD/VISIONARY - Generating creative blueprint")
        try:
            blueprint = await asyncio.wait_for(
                get_cloud_generation(request.prompt),
                timeout=request.max_generation_time * 0.4  # 40% of time for generation
            )
            source = blueprint.get('source', 'unknown')
            logger.info(f"│ ✓ Blueprint generated via {source.upper()}: {blueprint.get('overall_vibe', 'Unknown vibe')}")
            logger.info(f"│   Creative name: {blueprint.get('creative_name', 'Untitled')}")
            logger.info(f"│   Active engines: {sum(1 for s in blueprint['slots'] if s['engine_id'] > 0)}/6")
        except asyncio.TimeoutError:
            logger.error("│ ✗ Generation timed out")
            raise HTTPException(status_code=504, detail="Blueprint generation timed out")
        except Exception as e:
            logger.error(f"│ ✗ Generation error: {str(e)}")
            raise HTTPException(status_code=500, detail=f"Blueprint generation failed: {str(e)}")
        
        # Step 2: ORACLE - Find best matching preset
        logger.info("│ Step 2: ORACLE - Finding best match from corpus")
        try:
            base_preset = oracle.find_best_preset(blueprint)
            preset_name = base_preset.get('name', 'Unknown')
            logger.info(f"│ ✓ Matched preset: {preset_name}")
            
            # Log similarity score if available
            if 'similarity_score' in base_preset:
                logger.info(f"│   Similarity score: {base_preset['similarity_score']:.3f}")
        except Exception as e:
            logger.error(f"│ ✗ Oracle error: {str(e)}")
            # Oracle failure is not fatal - use default
            base_preset = oracle._create_default_preset(blueprint)
            logger.warning("│ ⚠ Using default preset due to Oracle error")
        
        # Step 3: CALCULATOR - Apply sophisticated nudges
        logger.info("│ Step 3: CALCULATOR - Applying intelligent nudges")
        try:
            nudged_preset = calculator.apply_nudges(base_preset, request.prompt, blueprint)
            
            # Extract nudge metadata
            calc_metadata = nudged_preset.get('calculator_metadata', {})
            num_nudges = calc_metadata.get('total_adjustments', 0)
            affected_params = len(calc_metadata.get('affected_parameters', []))
            
            logger.info(f"│ ✓ Applied {num_nudges} nudges to {affected_params} parameters")
            
            # Log significant nudges
            if 'nudge_log' in calc_metadata:
                for nudge in calc_metadata['nudge_log'].get('applied_nudges', [])[:3]:
                    if isinstance(nudge, dict) and 'parameter' in nudge:
                        logger.info(f"│   {nudge['parameter']}: {nudge.get('adjustment', 0):.2f}")
        except Exception as e:
            logger.error(f"│ ✗ Calculator error: {str(e)}")
            # Calculator failure is not fatal - use base preset
            nudged_preset = base_preset
            logger.warning("│ ⚠ Skipping nudges due to Calculator error")
        
        # Step 4: ALCHEMIST - Final synthesis and validation
        logger.info("│ Step 4: ALCHEMIST - Final validation and safety")
        try:
            final_preset = alchemist.finalize_preset(nudged_preset)
            
            # Extract validation metadata
            warnings = final_preset.get('validation_warnings', [])
            if warnings:
                logger.warning(f"│ ⚠ Validation warnings: {', '.join(warnings[:3])}")
            else:
                logger.info("│ ✓ Preset validated with no warnings")
            
            # Generate creative name
            preset_name = final_preset.get('name', 'Untitled Preset')
            logger.info(f"│ ✓ Final preset: '{preset_name}'")
            
        except Exception as e:
            logger.error(f"│ ✗ Alchemist error: {str(e)}")
            # Alchemist failure IS fatal - can't return unsafe preset
            raise HTTPException(status_code=500, detail=f"Preset validation failed: {str(e)}")
        
        # Calculate generation time
        generation_time = asyncio.get_event_loop().time() - start_time
        logger.info(f"═══ Pipeline Complete ({generation_time:.2f}s) ═══")
        
        # Prepare metadata
        metadata = {
            "generation_time_seconds": round(generation_time, 2),
            "pipeline_version": "3.0",
            "blueprint_vibe": blueprint.get('overall_vibe', 'unknown'),
            "creative_analysis": blueprint.get('creative_analysis', {}),
            "nudges_applied": nudged_preset.get('calculator_metadata', {}).get('total_adjustments', 0),
            "warnings": final_preset.get('validation_warnings', [])
        }
        
        # Remove internal metadata from final preset
        final_preset.pop('calculator_metadata', None)
        final_preset.pop('validation_warnings', None)
        final_preset.pop('alchemist_validated', None)
        
        return GenerateResponse(
            success=True,
            preset=final_preset,
            message=f"Successfully generated '{preset_name}'",
            metadata=metadata
        )
        
    except HTTPException:
        raise  # Re-raise HTTP exceptions
    except Exception as e:
        logger.error(f"Unexpected error in pipeline: {str(e)}")
        return GenerateResponse(
            success=False,
            preset={},
            message=f"Pipeline error: {str(e)}",
            metadata={"error": str(e), "stage": "unknown"}
        )

@app.get("/health", response_model=HealthResponse)
async def health_check():
    """
    Health check endpoint with component status
    """
    components = {}
    
    # Check Cloud Bridge
    try:
        from cloud_bridge import cloud_bridge
        if cloud_bridge.api_key:
            components["cloud_ai"] = "ready"
        else:
            components["cloud_ai"] = "no_api_key"
        components["local_fallback"] = "ready"
    except:
        components["cloud_ai"] = "error"
        components["local_fallback"] = "error"
    
    # Check Oracle (FAISS index)
    try:
        components["oracle"] = "ready" if oracle.index and oracle.index.ntotal > 0 else "no_index"
    except:
        components["oracle"] = "error"
    
    # Check Calculator (rules loaded)
    try:
        components["calculator"] = "ready" if calculator.rules else "no_rules"
    except:
        components["calculator"] = "error"
    
    # Check Alchemist
    try:
        components["alchemist"] = "ready"
    except:
        components["alchemist"] = "error"
    
    # Overall status
    all_ready = all(status == "ready" for status in components.values())
    
    return HealthResponse(
        status="healthy" if all_ready else "degraded",
        service="Chimera Phoenix AI Server",
        version="3.0",
        components=components
    )

@app.get("/")
async def root():
    """
    Root endpoint with API documentation
    """
    return {
        "service": "Chimera Phoenix AI Server",
        "version": "3.0",
        "description": "Trinity AI Pipeline for intelligent audio preset generation",
        "endpoints": {
            "POST /generate": "Generate preset from creative prompt",
            "GET /health": "Service health check with component status",
            "GET /docs": "Interactive API documentation (Swagger UI)",
            "GET /redoc": "Alternative API documentation (ReDoc)"
        },
        "pipeline": {
            "1_visionary": "AI-powered creative blueprint generation",
            "2_oracle": "FAISS vector search for best preset match",
            "3_calculator": "Sophisticated multi-layered parameter nudging",
            "4_alchemist": "Final validation and creative naming"
        }
    }

@app.on_event("startup")
async def startup_event():
    """
    Startup tasks
    """
    logger.info("="*60)
    logger.info("CHIMERA PHOENIX AI SERVER - TRINITY PIPELINE v3.0")
    logger.info("="*60)
    logger.info("Starting server components...")
    
    # Verify data files exist
    data_files = [
        Path("nudge_rules.json"),
        Path("parameter_manifest.json"),
        Path("engine_defaults.py"),
        Path("engine_mapping.py")
    ]
    
    for file_path in data_files:
        if file_path.exists():
            logger.info(f"✓ Found: {file_path.name}")
        else:
            logger.warning(f"✗ Missing: {file_path.name}")
    
    logger.info("Server ready to accept requests")
    logger.info("="*60)

@app.on_event("shutdown")
async def shutdown_event():
    """
    Cleanup tasks
    """
    logger.info("Shutting down Trinity Pipeline...")
    # Any cleanup needed
    logger.info("Server shutdown complete")

if __name__ == "__main__":
    import uvicorn
    
    # Server configuration
    config = {
        "app": "main:app",
        "host": "0.0.0.0",
        "port": 8000,
        "reload": False,  # Set to True for development
        "log_level": "info",
        "access_log": True
    }
    
    print("\n" + "="*60)
    print("STARTING CHIMERA PHOENIX AI SERVER")
    print("Trinity Pipeline: Visionary → Oracle → Calculator → Alchemist")
    print("="*60)
    print(f"Server: http://{config['host']}:{config['port']}")
    print(f"Docs: http://{config['host']}:{config['port']}/docs")
    print("="*60 + "\n")
    
    uvicorn.run(**config)