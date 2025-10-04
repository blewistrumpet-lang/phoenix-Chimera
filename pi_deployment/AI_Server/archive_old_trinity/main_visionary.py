"""
Visionary-Led Trinity Pipeline - Creative interpretation first
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
from cloud_bridge_enhanced import CloudBridgeEnhanced
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

app = FastAPI(title="Chimera Phoenix - Visionary Pipeline")

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

logger.info("✅ Visionary-Led Trinity Pipeline initialized")

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """
    Visionary-led pipeline:
    1. Visionary interprets and names
    2. Oracle finds technical match
    3. Calculator adjusts
    4. Alchemist optimizes (no naming)
    """
    try:
        context = PresetRequestContext(prompt=request.prompt)
        
        logger.info(f"═══ VISIONARY PIPELINE START ═══")
        logger.info(f"Prompt: {request.prompt}")
        
        # STEP 1: Visionary - Creative interpretation and naming
        context.update_stage("visionary")
        visionary = CloudBridgeEnhanced()
        
        try:
            visionary_result = await asyncio.wait_for(
                visionary.get_cloud_generation(request.prompt),
                timeout=8.0
            )
            
            # Extract the creative name (this is the FINAL name)
            creative_name = visionary_result.get("creative_name", "Custom Preset")
            technical_translation = visionary_result.get("technical_translation", request.prompt)
            character_tags = visionary_result.get("character_tags", [])
            creative_analysis = visionary_result.get("creative_analysis", {})
            
            logger.info(f"🎨 Visionary: '{creative_name}'")
            logger.info(f"   Translation: {technical_translation}")
            logger.info(f"   Tags: {character_tags}")
            
            # Store visionary's interpretation
            context.cloud_blueprint = visionary_result
            context.preset_name = creative_name  # Lock in the name
            
        except Exception as e:
            logger.warning(f"Visionary failed: {e}, using fallback")
            creative_name = request.prompt[:30]
            technical_translation = request.prompt
            character_tags = []
            creative_analysis = {}
            context.preset_name = creative_name
        
        # STEP 2: Extract required engines from ORIGINAL prompt
        context.update_stage("engine_extraction")
        context.required_engines = extract_required_engines(request.prompt)
        
        if context.required_engines:
            engine_names = [ENGINE_NAMES.get(e, str(e)) for e in context.required_engines]
            logger.info(f"📌 Required engines: {', '.join(engine_names)}")
        
        # STEP 3: Musical analysis
        context.update_stage("musical_analysis")
        musical_analysis = music_theory.analyze_prompt_musically(request.prompt)
        context.genre = musical_analysis.get("genre") or creative_analysis.get("genre")
        context.instrument = musical_analysis.get("instrument") or creative_analysis.get("instrument")
        context.character = musical_analysis.get("character", []) + character_tags
        
        logger.info(f"🎵 Musical: genre={context.genre}, instrument={context.instrument}")
        
        # STEP 4: Oracle - Use technical translation to find best preset
        context.update_stage("oracle")
        
        # Create Oracle query combining technical translation and requirements
        oracle_query = {
            "prompt": technical_translation,  # Use translated version
            "required_engines": context.required_engines,
            "genre": context.genre,
            "instrument": context.instrument,
            "character": context.character,
            "intensity": creative_analysis.get("intensity", 0.5),
            "space": creative_analysis.get("space", 0.5),
            "warmth": creative_analysis.get("warmth", 0.5)
        }
        
        oracle_preset = oracle.find_best_preset(oracle_query)
        context.oracle_preset = oracle_preset
        
        logger.info(f"🔮 Oracle: Found technical match")
        
        # STEP 5: Force required engines
        context.update_stage("force_requirements")
        
        current_engines = []
        for slot in range(1, 7):
            engine = oracle_preset.get(f"slot{slot}_engine", 0)
            if engine > 0:
                current_engines.append(engine)
        
        missing = [e for e in context.required_engines if e not in current_engines]
        
        if missing:
            logger.warning(f"⚠️ Missing required engines: {missing}")
            for engine_id in missing:
                added = False
                # Try empty slots first
                for slot in range(1, 7):
                    if oracle_preset.get(f"slot{slot}_engine", 0) == 0:
                        oracle_preset[f"slot{slot}_engine"] = engine_id
                        logger.info(f"✅ Added {ENGINE_NAMES.get(engine_id)} to slot {slot}")
                        added = True
                        break
                
                if not added:
                    # Replace a non-required engine
                    for slot in range(6, 0, -1):
                        current = oracle_preset.get(f"slot{slot}_engine", 0)
                        if current not in context.required_engines:
                            oracle_preset[f"slot{slot}_engine"] = engine_id
                            logger.info(f"✅ Replaced slot {slot} with {ENGINE_NAMES.get(engine_id)}")
                            break
        
        # STEP 6: Calculator - Apply nudges
        context.update_stage("calculator")
        
        calculator_preset = calculator.apply_nudges(
            oracle_preset, 
            request.prompt,  # Use original prompt for character
            {"required_engines": context.required_engines}
        )
        
        # STEP 7: Alchemist - PURELY TECHNICAL (no naming!)
        context.update_stage("alchemist")
        
        # Temporarily disable name generation in Alchemist
        final_preset = calculator_preset.copy()  # Start with calculator's output
        
        # Apply only technical optimizations
        if hasattr(alchemist, 'signal_intelligence'):
            # Optimize signal chain
            final_preset = alchemist.signal_intelligence.optimize_signal_chain(final_preset)
            
            # Validate parameters
            is_safe, warnings = alchemist.signal_intelligence.validate_parameters(final_preset)
            if not is_safe and warnings:
                logger.warning(f"Safety issues: {warnings}")
            
            # Create signal flow description
            final_preset["signal_flow"] = alchemist.signal_intelligence.explain_chain(final_preset)
        
        # Ensure master controls
        final_preset.setdefault("master_input", 0.7)
        final_preset.setdefault("master_output", 0.7)
        final_preset.setdefault("master_mix", 1.0)
        
        # USE VISIONARY'S NAME (not Alchemist's!)
        final_preset["name"] = context.preset_name
        
        # Force requirements one more time
        final_preset = context.force_include_requirements(final_preset)
        
        context.final_preset = final_preset
        
        # Verify success
        success = context.verify_requirements_met(final_preset)
        
        # Log results
        final_engines = []
        engine_count = 0
        for slot in range(1, 7):
            engine = final_preset.get(f"slot{slot}_engine", 0)
            if engine > 0:
                engine_count += 1
                final_engines.append(ENGINE_NAMES.get(engine, f"Unknown({engine})")[:20])
        
        logger.info(f"✅ Final: '{final_preset['name']}' with {engine_count} engines")
        logger.info(f"   Engines: {', '.join(final_engines[:4])}")
        
        if context.required_engines:
            match_count = sum(1 for e in context.required_engines 
                            if e in [final_preset.get(f"slot{s}_engine", 0) for s in range(1, 7)])
            match_rate = (match_count / len(context.required_engines)) * 100
            logger.info(f"📊 Match rate: {match_rate:.0f}% ({match_count}/{len(context.required_engines)})")
        else:
            match_rate = 100
        
        logger.info(f"═══ PIPELINE COMPLETE ═══")
        
        return GenerateResponse(
            success=True,
            preset=final_preset,
            message=f"Generated '{final_preset.get('name', 'Unknown')}'",
            metadata={
                "creative_name": creative_name,
                "technical_translation": technical_translation,
                "character_tags": character_tags,
                "required_engines": context.required_engines,
                "engine_count": engine_count,
                "warnings": context.warnings,
                "match_rate": match_rate
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
        "version": "VISIONARY-1.0",
        "components": {
            "visionary": "enhanced",
            "oracle": "enhanced",
            "calculator": "enhanced", 
            "alchemist": "technical-only",
            "naming": "visionary-controlled"
        }
    }

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)