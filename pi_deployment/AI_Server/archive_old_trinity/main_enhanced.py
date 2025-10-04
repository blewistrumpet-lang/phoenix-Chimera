"""
Enhanced Chimera Phoenix AI Server - Trinity Pipeline with Full Musical Intelligence
Orchestrates: Visionary → Oracle → Calculator → Alchemist
Now with signal chain optimization and deep musical understanding
"""

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
from typing import List, Dict, Any, Optional
import asyncio
import json
import logging
from pathlib import Path

# Import enhanced Trinity components
from cloud_bridge import CloudBridge  # Cloud-first AI with fallback
from oracle_enhanced import OracleEnhanced
from calculator_enhanced import CalculatorEnhanced
from alchemist_enhanced import AlchemistEnhanced

# Import musical intelligence
from music_theory_intelligence import MusicTheoryIntelligence, analyze_musical_intent
from signal_chain_intelligence import SignalChainIntelligence
from engine_knowledge_base import ENGINE_KNOWLEDGE

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

app = FastAPI(
    title="Chimera Phoenix AI Server (Enhanced)",
    version="4.0",
    description="Trinity AI Pipeline with Deep Musical Intelligence"
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

# Initialize Enhanced Trinity Pipeline
logger.info("Initializing Enhanced Trinity Pipeline with Musical Intelligence...")

# Disable automatic utility engine addition (only add when explicitly requested)
try:
    from disable_auto_utilities import disable_auto_utilities
    disable_auto_utilities()
    logger.info("Automatic utility engine addition disabled")
except Exception as e:
    logger.warning(f"Could not disable auto utilities: {e}")

# Initialize Music Theory Intelligence
music_theory = MusicTheoryIntelligence()
signal_intelligence = SignalChainIntelligence()
logger.info("Musical intelligence modules loaded")

# Oracle - Enhanced with musical understanding
oracle = OracleEnhanced(
    index_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus_clean.index",
    meta_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata_clean.json",
    presets_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json"
)
logger.info("Enhanced Oracle initialized with musical understanding")

# Calculator - Enhanced with smart nudging
calculator = CalculatorEnhanced(
    rules_path="nudge_rules.json"
)
logger.info("Enhanced Calculator initialized with intelligent nudging")

# Alchemist - Enhanced with signal chain optimization
alchemist = AlchemistEnhanced()
logger.info("Enhanced Alchemist initialized with safety validation")

logger.info("✨ Enhanced Trinity Pipeline ready with full musical intelligence!")

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """
    Main endpoint orchestrating the Enhanced Trinity Pipeline.
    Flow: Musical Analysis → Visionary → Oracle → Calculator → Alchemist
    """
    start_time = asyncio.get_event_loop().time()
    
    try:
        logger.info(f"═══ Starting Enhanced Trinity Pipeline ═══")
        logger.info(f"Prompt: {request.prompt[:100]}...")
        
        # Step 0: Deep Musical Analysis (NEW)
        logger.info("🎵 Analyzing musical intent...")
        musical_analysis = music_theory.analyze_prompt_musically(request.prompt)
        logger.info(f"   Genre: {musical_analysis.get('genre', 'none')}")
        logger.info(f"   Instrument: {musical_analysis.get('instrument', 'none')}")
        logger.info(f"   Character: {musical_analysis.get('character', [])}")
        
        # Step 1: VISIONARY - Extract Creative Intent
        logger.info("🎨 VISIONARY: Extracting creative intent...")
        try:
            # Try cloud AI first with timeout
            cloud_bridge = CloudBridge()
            blueprint = await asyncio.wait_for(
                cloud_bridge.get_cloud_generation(request.prompt),
                timeout=8.0
            )
            
            # Convert cloud format to Oracle format
            if "slots" in blueprint:
                # Convert slots array to flat format
                for slot in blueprint.get("slots", []):
                    slot_num = slot.get("slot", 0)
                    engine_id = slot.get("engine_id", 0)
                    blueprint[f"slot{slot_num}_engine"] = engine_id
                    blueprint[f"slot{slot_num}_character"] = slot.get("character", "")
            
            # Ensure vibe field exists
            if "overall_vibe" in blueprint and "vibe" not in blueprint:
                blueprint["vibe"] = blueprint["overall_vibe"]
            elif "creative_name" in blueprint and "vibe" not in blueprint:
                blueprint["vibe"] = blueprint["creative_name"]
            elif "vibe" not in blueprint:
                blueprint["vibe"] = request.prompt[:50]
            
            logger.info(f"   Cloud AI generated: {blueprint.get('vibe', 'Unknown')}")
        except Exception as e:
            logger.warning(f"   Cloud AI failed ({str(e)[:50]}...), using enhanced fallback")
            # Use musical analysis for fallback
            blueprint = {
                "vibe": request.prompt,
                "genre": musical_analysis.get("genre"),
                "instrument": musical_analysis.get("instrument"),
                "character": musical_analysis.get("character", []),
                "suggested_chain": music_theory.get_processing_chain(musical_analysis)
            }
        
        # Step 2: ORACLE - Find Best Matching Preset with Musical Understanding
        logger.info("🔮 ORACLE: Finding best preset with musical intelligence...")
        base_preset = oracle.find_best_preset(blueprint)
        
        if "oracle_explanation" in base_preset:
            logger.info(f"   Oracle: {base_preset['oracle_explanation'][:100]}...")
        
        # Step 3: CALCULATOR - Apply Intelligent Nudges
        logger.info("🧮 CALCULATOR: Applying intelligent nudges...")
        nudged_preset = calculator.apply_nudges(base_preset, request.prompt, blueprint)
        
        # Log engine suggestions
        suggested_engines = calculator.suggest_engines_for_intent(request.prompt)
        if suggested_engines:
            from engine_mapping_authoritative import ENGINE_NAMES
            engine_names = [ENGINE_NAMES.get(e, f"Unknown({e})") for e in suggested_engines[:3]]
            logger.info(f"   Suggested engines: {', '.join(engine_names)}")
        
        # Step 4: ALCHEMIST - Finalize with Signal Chain Optimization
        logger.info("⚗️ ALCHEMIST: Finalizing with safety and optimization...")
        final_preset = alchemist.finalize_preset(nudged_preset, request.prompt)
        
        # Log signal chain
        if "signal_flow" in final_preset:
            logger.info(f"   Signal flow: {final_preset['signal_flow'][:100]}...")
        
        # Calculate processing time
        processing_time = asyncio.get_event_loop().time() - start_time
        
        # Prepare response with enhanced metadata
        response = GenerateResponse(
            success=True,
            preset=final_preset,
            message=f"Successfully generated '{final_preset.get('name', 'Unknown')}' with musical intelligence",
            metadata={
                "generation_time_seconds": round(processing_time, 2),
                "pipeline_version": "4.0-enhanced",
                "musical_analysis": musical_analysis,
                "signal_chain_optimized": final_preset.get("signal_chain_optimized", False),
                "safety_validated": final_preset.get("safety_validated", False),
                "warnings": final_preset.get("warnings", []),
                "oracle_explanation": base_preset.get("oracle_explanation", ""),
                "suggested_engines": suggested_engines[:6] if suggested_engines else []
            }
        )
        
        logger.info(f"✅ Generation complete in {processing_time:.2f}s")
        logger.info(f"   Final preset: {final_preset.get('name', 'Unknown')}")
        logger.info(f"═══ Enhanced Pipeline Complete ═══")
        
        return response
        
    except asyncio.TimeoutError:
        logger.error("Generation timed out")
        raise HTTPException(
            status_code=504,
            detail="Preset generation timed out"
        )
    except Exception as e:
        logger.error(f"Generation failed: {str(e)}")
        raise HTTPException(
            status_code=500,
            detail=f"Failed to generate preset: {str(e)}"
        )

@app.get("/health", response_model=HealthResponse)
async def health_check():
    """Health check endpoint with component status"""
    try:
        # Check each component
        components_status = {}
        
        # Check Oracle
        try:
            if hasattr(oracle, 'presets') and len(oracle.presets) > 0:
                components_status["oracle"] = f"✅ Loaded {len(oracle.presets)} presets"
            else:
                components_status["oracle"] = "⚠️ No presets loaded"
        except:
            components_status["oracle"] = "❌ Not initialized"
        
        # Check Calculator
        try:
            test_engines = calculator.suggest_engines_for_intent("test")
            components_status["calculator"] = f"✅ Ready with {len(calculator.engine_suggestions)} instrument profiles"
        except:
            components_status["calculator"] = "❌ Not initialized"
        
        # Check Alchemist
        try:
            if hasattr(alchemist, 'signal_intelligence'):
                components_status["alchemist"] = "✅ Ready with signal chain optimization"
            else:
                components_status["alchemist"] = "⚠️ Missing signal intelligence"
        except:
            components_status["alchemist"] = "❌ Not initialized"
        
        # Check Musical Intelligence
        try:
            test_analysis = music_theory.analyze_prompt_musically("test")
            components_status["music_theory"] = f"✅ {len(music_theory.genre_intelligence)} genres loaded"
        except:
            components_status["music_theory"] = "❌ Not initialized"
        
        # Check Signal Chain
        try:
            test_chain = signal_intelligence.optimize_signal_chain({"slot1_engine": 39, "slot2_engine": 2})
            components_status["signal_chain"] = "✅ Optimization ready"
        except:
            components_status["signal_chain"] = "❌ Not initialized"
        
        return HealthResponse(
            status="healthy",
            service="Chimera Phoenix AI Server (Enhanced)",
            version="4.0",
            components=components_status
        )
    except Exception as e:
        return HealthResponse(
            status="unhealthy",
            service="Chimera Phoenix AI Server (Enhanced)",
            version="4.0",
            components={"error": str(e)}
        )

@app.get("/")
async def root():
    """Root endpoint with system information"""
    return {
        "service": "Chimera Phoenix AI Server (Enhanced)",
        "version": "4.0",
        "status": "ready",
        "endpoints": {
            "/generate": "POST - Generate AI preset with musical intelligence",
            "/health": "GET - System health check",
            "/docs": "GET - Interactive API documentation"
        },
        "enhancements": {
            "musical_intelligence": "Deep understanding of genres, instruments, and production",
            "signal_chain_optimization": "Automatic effect ordering for best sound",
            "parameter_intelligence": "Smart parameter adjustments based on musical intent",
            "safety_validation": "Prevents feedback, clipping, and audio issues"
        }
    }

if __name__ == "__main__":
    import uvicorn
    
    logger.info("🚀 Starting Enhanced Chimera Phoenix AI Server...")
    logger.info("📍 API will be available at: http://localhost:8000")
    logger.info("📚 Documentation at: http://localhost:8000/docs")
    
    uvicorn.run(app, host="0.0.0.0", port=8000)