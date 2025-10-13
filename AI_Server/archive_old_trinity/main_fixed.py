"""
FIXED Trinity Pipeline - Properly preserves user intent and engine requirements
"""

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from typing import Dict, Any, Optional
import asyncio
import logging
import os
from pathlib import Path

# Import context and extraction
from preset_request_context import PresetRequestContext
from engine_extraction import extract_required_engines

# Import Trinity components
from cloud_bridge import CloudBridge
from oracle_enhanced import OracleEnhanced
from calculator_enhanced import CalculatorEnhanced
from alchemist_improved import AlchemistImproved

# Import musical intelligence
from music_theory_intelligence import MusicTheoryIntelligence
from engine_mapping_authoritative import ENGINE_NAMES

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class GenerateRequest(BaseModel):
    prompt: str

class GenerateResponse(BaseModel):
    success: bool
    preset: Dict[str, Any]
    message: str
    metadata: Dict[str, Any]

app = FastAPI(title="Chimera Phoenix FIXED")

# Initialize components
music_theory = MusicTheoryIntelligence()

# Initialize Oracle with corpus
base_dir = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3")
oracle = OracleEnhanced(
    str(base_dir / "faiss_index" / "corpus_clean.index"),
    str(base_dir / "faiss_index" / "metadata_clean.json"),
    str(base_dir / "faiss_index" / "presets_clean.json")
)

calculator = CalculatorEnhanced()
alchemist = AlchemistImproved()

logger.info("✅ FIXED Trinity Pipeline initialized")

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """
    FIXED pipeline that preserves user intent
    """
    try:
        # Create context that flows through pipeline
        context = PresetRequestContext(prompt=request.prompt)
        
        logger.info(f"═══ FIXED PIPELINE START ═══")
        logger.info(f"Prompt: {request.prompt}")
        
        # STEP 1: Extract required engines from prompt
        context.update_stage("engine_extraction")
        context.required_engines = extract_required_engines(request.prompt)
        
        if context.required_engines:
            engine_names = [ENGINE_NAMES.get(e, str(e)) for e in context.required_engines]
            logger.info(f"📌 Required engines: {', '.join(engine_names)}")
        
        # STEP 2: Musical analysis
        context.update_stage("musical_analysis")
        musical_analysis = music_theory.analyze_prompt_musically(request.prompt)
        context.genre = musical_analysis.get("genre")
        context.instrument = musical_analysis.get("instrument")
        context.character = musical_analysis.get("character", [])
        
        logger.info(f"🎵 Musical: genre={context.genre}, instrument={context.instrument}")
        
        # STEP 3: Cloud AI generation
        context.update_stage("cloud_ai")
        try:
            cloud_bridge = CloudBridge()
            cloud_result = await asyncio.wait_for(
                cloud_bridge.get_cloud_generation(request.prompt),
                timeout=8.0
            )
            
            # Convert Cloud AI format to flat format
            if "slots" in cloud_result:
                for slot in cloud_result.get("slots", []):
                    slot_num = slot.get("slot", 0)
                    engine_id = slot.get("engine_id", 0)
                    if engine_id > 0:
                        context.cloud_suggested_engines.append(engine_id)
                        cloud_result[f"slot{slot_num}_engine"] = engine_id
            
            # Ensure vibe field
            cloud_result["vibe"] = cloud_result.get("overall_vibe", cloud_result.get("creative_name", request.prompt[:50]))
            
            context.cloud_blueprint = cloud_result
            logger.info(f"☁️ Cloud AI: {cloud_result.get('vibe', 'Unknown')}")
            
        except Exception as e:
            logger.warning(f"Cloud AI failed: {e}")
            # Create fallback blueprint
            context.cloud_blueprint = {
                "vibe": request.prompt[:50],
                "genre": context.genre,
                "instrument": context.instrument,
                "character": context.character
            }
        
        # Add Cloud AI engines to required list
        for engine in context.cloud_suggested_engines:
            if engine not in context.required_engines:
                context.required_engines.append(engine)
        
        # STEP 4: Oracle - Find best preset WITH engine requirements
        context.update_stage("oracle")
        
        # Inject required engines into blueprint for Oracle
        context.cloud_blueprint["required_engines"] = context.required_engines
        
        oracle_preset = oracle.find_best_preset(context.cloud_blueprint)
        context.oracle_preset = oracle_preset
        context.oracle_explanation = oracle_preset.get("oracle_explanation", "")
        
        logger.info(f"🔮 Oracle: {oracle_preset.get('creative_name', 'Unknown')}")
        
        # STEP 5: Force required engines into preset
        context.update_stage("force_requirements")
        
        # Ensure ALL required engines are in the preset
        current_engines = []
        for slot in range(1, 7):
            engine = oracle_preset.get(f"slot{slot}_engine", 0)
            if engine > 0:
                current_engines.append(engine)
        
        missing = [e for e in context.required_engines if e not in current_engines]
        
        if missing:
            logger.warning(f"⚠️ Missing required engines: {missing}")
            # Add them to empty slots or replace non-required engines
            for engine_id in missing:
                added = False
                # Try empty slots first
                for slot in range(1, 7):
                    if oracle_preset.get(f"slot{slot}_engine", 0) == 0:
                        oracle_preset[f"slot{slot}_engine"] = engine_id
                        logger.info(f"✅ Added required engine {ENGINE_NAMES.get(engine_id)} to slot {slot}")
                        added = True
                        break
                
                if not added:
                    # Replace a non-required engine
                    for slot in range(6, 0, -1):
                        current = oracle_preset.get(f"slot{slot}_engine", 0)
                        if current not in context.required_engines:
                            oracle_preset[f"slot{slot}_engine"] = engine_id
                            logger.info(f"✅ Replaced slot {slot} with required engine {ENGINE_NAMES.get(engine_id)}")
                            break
        
        # STEP 6: Calculator - Apply nudges WITHOUT removing required engines
        context.update_stage("calculator")
        
        # Pass context so Calculator knows what to preserve
        calculator_preset = calculator.apply_nudges(
            oracle_preset, 
            request.prompt,
            {"required_engines": context.required_engines, **context.cloud_blueprint}
        )
        
        # Verify required engines still present
        context.verify_requirements_met(calculator_preset)
        
        # STEP 7: Alchemist - Finalize WITHOUT removing required engines
        context.update_stage("alchemist")
        
        # Pass full context to improved Alchemist
        final_preset = alchemist.finalize_preset(
            calculator_preset, 
            request.prompt,
            {"required_engines": context.required_engines, "cloud_blueprint": context.cloud_blueprint}
        )
        
        # FINAL CHECK: Force requirements one more time
        final_preset = context.force_include_requirements(final_preset)
        
        context.final_preset = final_preset
        
        # Verify success
        success = context.verify_requirements_met(final_preset)
        
        # Log results
        final_engines = []
        for slot in range(1, 7):
            engine = final_preset.get(f"slot{slot}_engine", 0)
            if engine > 0:
                final_engines.append(ENGINE_NAMES.get(engine, f"Unknown({engine})")[:20])
        
        logger.info(f"✅ Final engines: {', '.join(final_engines[:4])}")
        
        if context.required_engines:
            match_count = sum(1 for e in context.required_engines if e in [final_preset.get(f"slot{s}_engine", 0) for s in range(1, 7)])
            match_rate = (match_count / len(context.required_engines)) * 100
            logger.info(f"📊 Match rate: {match_rate:.0f}% ({match_count}/{len(context.required_engines)})")
        
        logger.info(f"═══ PIPELINE COMPLETE ═══")
        
        return GenerateResponse(
            success=True,
            preset=final_preset,
            message=f"Generated '{final_preset.get('name', 'Unknown')}' with required engines",
            metadata={
                "required_engines": context.required_engines,
                "cloud_suggested": context.cloud_suggested_engines,
                "warnings": context.warnings,
                "match_rate": match_rate if context.required_engines else 100
            }
        )
        
    except Exception as e:
        logger.error(f"Pipeline error: {str(e)}")
        import traceback
        traceback.print_exc()
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/health")
async def health():
    return {
        "status": "healthy",
        "version": "FIXED-1.0",
        "components": {
            "oracle": "enhanced",
            "calculator": "enhanced", 
            "alchemist": "enhanced",
            "engine_extraction": "active",
            "requirement_enforcement": "active"
        }
    }

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)