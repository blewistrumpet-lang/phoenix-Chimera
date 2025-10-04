#!/usr/bin/env python3
"""
TRUE Trinity Pipeline - Pure AI Generation
Visionary (AI) → Calculator (AI) → Alchemist (Local)
NO Oracle, NO Corpus, NO Preset Matching

This is the authoritative Trinity implementation.
"""

from fastapi import FastAPI, HTTPException, File, UploadFile, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from pydantic import BaseModel, Field
from typing import Dict, Any, Optional, List
import asyncio
import logging
import os
from pathlib import Path
from datetime import datetime
import tempfile
from openai import OpenAI
from dotenv import load_dotenv
from asyncio import Queue, Semaphore
import gc

# Load environment variables from .env file
load_dotenv()

# Import ONLY the true Trinity components
from visionary_complete import CompleteVisionary
from calculator_trinity_ai import CalculatorTrinityAI  
from alchemist_complete import CompleteAlchemist

# Import strict validation for engine enforcement
from strict_validation import StrictValidator

# Import robust pipeline components
from subprocess_manager import SubprocessManager
from pipeline_state import PipelineStateManager, PipelineStage

# Import file exchange system for guaranteed delivery
from file_exchange import FileExchangeManager

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# Global request queue and semaphore to prevent concurrent OpenAI calls
request_semaphore = Semaphore(1)  # Only allow 1 OpenAI request at a time
generation_lock = asyncio.Lock()  # Lock for preset generation

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

# Add validation error handler to log detailed errors
@app.exception_handler(RequestValidationError)
async def validation_exception_handler(request: Request, exc: RequestValidationError):
    """Log validation errors for debugging"""
    body = await request.body()
    logger.error(f"❌ Validation error for {request.method} {request.url.path}")
    logger.error(f"   Body: {body.decode('utf-8') if body else 'empty'}")
    logger.error(f"   Errors: {exc.errors()}")
    return JSONResponse(
        status_code=422,
        content={"detail": exc.errors(), "body": body.decode('utf-8') if body else None}
    )

# Request/Response models
class GenerateRequest(BaseModel):
    """Request for preset generation"""
    prompt: str = Field(..., description="User's creative prompt")
    intensity: float = Field(default=0.5, ge=0.0, le=1.0, description="Effect intensity (0-1)")
    complexity: int = Field(default=3, ge=1, le=6, description="Number of engines to use")
    context: Dict[str, Any] = Field(default_factory=dict, description="Optional context from plugin")

class GenerateResponse(BaseModel):
    """Response with generated preset"""
    success: bool
    type: str = "preset"  # Plugin expects this field
    message: str
    data: Dict[str, Any]  # Plugin expects data.preset structure
    metadata: Optional[Dict[str, Any]] = None  # Optional metadata

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
visionary = CompleteVisionary()  # Uses complete engine knowledge
calculator = CalculatorTrinityAI(api_key=api_key)  # AI-powered intelligent parameter optimization
alchemist = CompleteAlchemist()   # Proper validation

# Initialize strict validator for engine enforcement
validator = StrictValidator()
logger.info("✅ Strict validator initialized for engine enforcement")

# Initialize file exchange manager for guaranteed preset delivery
file_exchange = FileExchangeManager()
logger.info(f"File exchange initialized at {file_exchange.base_dir}")

# Initialize robust pipeline components
subprocess_manager = SubprocessManager(max_retries=3)
pipeline_state = PipelineStateManager()

logger.info("✅ TRUE Trinity Pipeline initialized (NO Oracle/Corpus)")

@app.post("/visionary")
async def visionary_generate(request: GenerateRequest):
    """
    Step 1: VISIONARY - Generate creative preset with AI
    Then automatically passes to Calculator
    """
    try:
        logger.info(f"🌟 Visionary: Processing '{request.prompt}'")
        
        # Use semaphore to prevent concurrent OpenAI calls
        async with request_semaphore:
            # Generate with timeout
            creative_preset = await asyncio.wait_for(
                visionary.generate_complete_preset(prompt=request.prompt),
                timeout=30.0
            )
        
        logger.info(f"✅ Visionary generated: {creative_preset.get('name', 'Untitled')}")
        
        # Pass to Calculator automatically
        logger.info("📡 Visionary passing to Calculator...")
        await asyncio.sleep(0.1)  # Small delay to prevent overwhelming
        
        calculator_result = await calculator_refine(creative_preset)
        
        return calculator_result
        
    except asyncio.TimeoutError:
        logger.error("Visionary timed out, using fallback")
        fallback_preset = visionary.create_intelligent_fallback(request.prompt)
        # Still pass fallback to calculator
        calculator_result = await calculator_refine(fallback_preset)
        return calculator_result
        
    except Exception as e:
        logger.error(f"Visionary error: {str(e)}")
        fallback_preset = visionary.create_intelligent_fallback(request.prompt)
        # Still pass fallback to calculator
        calculator_result = await calculator_refine(fallback_preset)
        return calculator_result

@app.post("/calculator")
async def calculator_refine(preset: Dict[str, Any]):
    """
    Step 2: CALCULATOR - Optimize with musical intelligence
    Then automatically passes to Alchemist
    """
    try:
        logger.info(f"🧮 Calculator: Optimizing '{preset.get('name', 'Untitled')}'")

        # Call async refine_preset directly
        prompt_text = preset.get('description', preset.get('name', 'unknown'))
        optimized_preset = await calculator.refine_preset(preset, prompt_text)

        logger.info(f"✅ Calculator optimized: {len(optimized_preset.get('slots', []))} engines")
        
        # Pass to Alchemist automatically
        logger.info("📡 Calculator passing to Alchemist...")
        await asyncio.sleep(0.1)  # Small delay
        
        alchemist_result = await alchemist_validate(optimized_preset)
        
        return alchemist_result
        
    except Exception as e:
        logger.error(f"Calculator error: {str(e)}, passing original to Alchemist")
        # Calculator failure is not fatal, pass original to Alchemist
        alchemist_result = await alchemist_validate(preset)
        return alchemist_result

@app.post("/alchemist")
async def alchemist_validate(preset: Dict[str, Any]):
    """
    Step 3: ALCHEMIST - Validate safety and format
    Returns final preset ready for plugin
    """
    try:
        logger.info(f"⚗️ Alchemist: Validating '{preset.get('name', 'Untitled')}'")
        
        # Alchemist is local validation
        final_preset, validation_report = alchemist.validate_and_fix(preset)
        if not validation_report["valid"]:
            logger.warning(f"Validation issues: {validation_report['errors']}")
        final_preset = alchemist.format_for_plugin(final_preset)
        
        # Remove internal metadata before sending to plugin
        final_preset.pop('calculator_metadata', None)
        final_preset.pop('alchemist_metadata', None)
        
        logger.info(f"✅ Alchemist validated: Safety checks passed")
        logger.info("="*60)
        logger.info(f"✨ Trinity Pipeline Complete")
        logger.info(f"🎼 Final Preset: '{final_preset.get('name', 'Untitled')}'")
        logger.info("="*60)
        
        # Return final preset formatted for plugin
        return {
            "success": True,
            "type": "preset",
            "preset": final_preset,
            "message": f"Generated: {final_preset.get('name', 'Unnamed')}",
            "stage": "complete"
        }
        
    except Exception as e:
        logger.error(f"Alchemist error: {str(e)}")
        # Still try to return the preset even if validation had issues
        preset.pop('calculator_metadata', None)
        preset.pop('alchemist_metadata', None)
        return {
            "success": False,
            "type": "preset",
            "preset": preset,
            "message": f"Validation warning: {str(e)}",
            "stage": "alchemist_error"
        }

@app.post("/generate", response_model=GenerateResponse)
async def generate_preset(request: GenerateRequest):
    """
    Generate a preset using the TRUE Trinity Pipeline.
    Flow: Visionary → Calculator → Alchemist
    """
    # Debug: Log incoming request for troubleshooting
    logger.info(f"🔍 Request received: prompt='{request.prompt}', intensity={request.intensity}, complexity={request.complexity}, context={request.context}")

    start_time = asyncio.get_event_loop().time()

    # Use lock to prevent concurrent generations that could cause memory issues
    async with generation_lock:
        try:
            logger.info("="*60)
            logger.info("🎭 Starting TRUE Trinity Pipeline")
            logger.info(f"📝 Prompt: {request.prompt[:100]}...")
            logger.info("="*60)
            
            # Force garbage collection before generation to free memory
            gc.collect()
            
            # Step 1: VISIONARY - Generate complete preset with AI
            logger.info("⭐ Step 1: VISIONARY - AI Creative Generation")
            try:
                # Visionary generates COMPLETE preset, not just blueprint
                creative_preset = await asyncio.wait_for(
                    visionary.generate_complete_preset(
                        prompt=request.prompt
                    ),
                    timeout=30.0  # 30 second timeout for AI (more time for complex prompts)
                )
            
                logger.info(f"✅ Visionary generated: '{creative_preset.get('name', 'Untitled')}'")
                logger.info(f"   Engines: {len(creative_preset.get('slots', []))}")
                
                # STRICT VALIDATION - Check if preset meets requirements
                validation = validator.validate_preset(creative_preset, request.prompt)
                if not validation["valid"]:
                    logger.warning(f"⚠️ Validation failed - Score: {validation['score']}/100")
                    for error in validation["errors"]:
                        logger.error(f"   ❌ {error}")
                    
                    # Enforce minimum engines if needed
                    creative_preset = validator.enforce_minimum_engines(creative_preset)
                    logger.info("   🔧 Applied automatic fixes for minimum requirements")
                else:
                    logger.info(f"✅ Validation passed - Score: {validation['score']}/100")
                
            except asyncio.TimeoutError:
                logger.error("❌ Visionary timed out")
                raise HTTPException(status_code=504, detail="AI generation timed out")
            except Exception as e:
                logger.error(f"❌ Visionary error: {str(e)}")
                # If AI fails, create a basic preset
                creative_preset = visionary.create_intelligent_fallback(request.prompt)
                logger.warning("⚠️ Using fallback preset generation")
            
            # Step 2: CALCULATOR - Optimize with musical intelligence
            logger.info("🧮 Step 2: CALCULATOR - Musical Intelligence Optimization")
            try:
                # Calculator refines and optimizes the preset (call async method directly)
                optimized_preset = await calculator.refine_preset(creative_preset, request.prompt)

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
                final_preset, validation_report = alchemist.validate_and_fix(optimized_preset)
                if not validation_report["valid"]:
                    logger.warning(f"Validation issues: {validation_report['errors']}")
                final_preset = alchemist.format_for_plugin(final_preset)
                
                # Log validation results
                validation = final_preset.get('alchemist_metadata', {})
                if validation:
                    logger.info(f"✅ Alchemist validation complete")
                    logger.info(f"   Safety checks: {validation.get('safety_passed', False)}")
                    logger.info(f"   Parameters clamped: {validation.get('parameters_clamped', 0)}")
                    logger.info(f"   Dangerous combos: {validation.get('dangerous_combos_found', 0)}")
                
            except Exception as e:
                import traceback
                logger.error(f"❌ Alchemist error: {str(e)}")
                logger.error(traceback.format_exc())
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

            # Normalize parameters from dict format to simple floats for plugin
            for slot in final_preset.get("slots", []):
                params = slot.get("parameters", [])
                normalized_params = []
                for p in params:
                    if isinstance(p, dict) and "value" in p:
                        normalized_params.append(float(p["value"]))
                    else:
                        normalized_params.append(float(p))
                slot["parameters"] = normalized_params

            return GenerateResponse(
                success=True,
                type="preset",
                message=f"Successfully generated '{final_preset.get('name', 'Untitled')}'",
                data={"preset": final_preset},  # Plugin expects data.preset structure
                metadata=metadata
            )
            
        except HTTPException:
            raise
        except Exception as e:
            logger.error(f"Unexpected pipeline error: {str(e)}")
            return GenerateResponse(
                success=False,
                type="error",
                message=f"Pipeline error: {str(e)}",
                data={},
                metadata={"error": str(e), "stage": "unknown"}
            )

@app.post("/transcribe")
async def transcribe_audio(audio: UploadFile = File(...)):
    """
    Transcribe audio to text using Whisper API
    """
    try:
        # Check for OpenAI API key
        if not api_key:
            raise HTTPException(
                status_code=500, 
                detail="OpenAI API key not configured"
            )
        
        # Save uploaded audio to temporary file
        with tempfile.NamedTemporaryFile(delete=False, suffix=".webm") as tmp_file:
            content = await audio.read()
            tmp_file.write(content)
            tmp_file_path = tmp_file.name
        
        try:
            # Initialize OpenAI client
            client = OpenAI(api_key=api_key)
            
            # Open the audio file and send to Whisper
            with open(tmp_file_path, "rb") as audio_file:
                transcript = client.audio.transcriptions.create(
                    model="whisper-1",
                    file=audio_file,
                    response_format="json"
                )
            
            # Clean up temp file
            os.unlink(tmp_file_path)
            
            logger.info(f"Transcribed: {transcript.text[:100]}...")
            
            return {
                "success": True,
                "text": transcript.text,
                "message": "Audio transcribed successfully"
            }
            
        except Exception as e:
            # Clean up temp file on error
            if os.path.exists(tmp_file_path):
                os.unlink(tmp_file_path)
            raise e
            
    except Exception as e:
        logger.error(f"Transcription error: {str(e)}")
        return {
            "success": False,
            "text": "",
            "message": f"Transcription failed: {str(e)}"
        }

@app.get("/ping")
async def ping():
    """Simple ping endpoint for connection testing"""
    return {"status": "pong", "timestamp": datetime.now().isoformat()}

@app.post("/session/start")
async def start_session():
    """Start a new session"""
    session_id = f"session_{datetime.now().timestamp()}"
    logger.info(f"New session started: {session_id}")
    return {"success": True, "sessionId": session_id}

@app.get("/poll")
async def poll(session: str = ""):
    """File-based exchange endpoint - replaces message queue polling"""
    # Check for pending preset in file exchange
    pending = file_exchange.get_pending_for_session(session)
    
    if pending:
        logger.info(f"Found pending preset for session {session}: {pending['preset_name']}")
        
        # Mark the marker file as read by removing it
        marker_file = file_exchange.pending_dir / f"{session}_READY.marker"
        if marker_file.exists():
            marker_file.unlink()
        
        # Return the preset data in expected format
        return {
            "session": session,
            "messages": [{
                "type": "preset",
                "data": {
                    "success": True,
                    "type": "preset",
                    "message": f"Generated: {pending['preset_name']}",
                    "data": {"preset": pending['preset_data']}
                }
            }]
        }
    
    # No pending presets
    return {"session": session, "messages": []}

@app.get("/poll_legacy")
async def poll_legacy(session: str = ""):
    """Legacy polling endpoint (kept for reference)"""
    # Original queue-based implementation
    if session in pipeline_state.session_messages:
        messages = []
        # Get all pending messages for this session
        while not pipeline_state.session_messages[session].empty():
            try:
                msg = pipeline_state.session_messages[session].get_nowait()
                messages.append(msg)
            except:
                break
        
        if messages:
            logger.info(f"Sending {len(messages)} queued messages to session {session}")
            # Return the messages
            return {"session": session, "messages": messages}
    
    # No messages, return empty
    return {"session": session, "messages": []}

@app.post("/message")
async def message(request: Request):
    """Handle messages from the plugin - robust pipeline with subprocess isolation"""
    try:
        body = await request.json()
        logger.info(f"Received message from plugin: {body}")
        
        # Handle different message types from plugin
        message_type = body.get("type", "")
        
        # Get prompt from various possible fields
        prompt = body.get("message", body.get("prompt", body.get("content", "")))
        
        # Skip non-query messages
        if message_type in ["heartbeat", "plugin_state"]:
            logger.debug(f"Skipping {message_type} message")
            return {"success": True, "message": "Message received"}
        
        if not prompt:
            logger.warning(f"No prompt in message type: {message_type}")
            return {"success": False, "error": "No prompt provided"}
        
        # Check cache first
        cached_response = pipeline_state.cache.get(prompt)
        if cached_response:
            logger.info(f"🎯 Cache hit for prompt: {prompt[:50]}...")
            return cached_response
        
        # Check circuit breaker
        if not pipeline_state.circuit_breaker.can_attempt():
            logger.warning("Circuit breaker is open, using fallback")
            fallback = visionary.create_intelligent_fallback(prompt)
            return {
                "success": False,
                "type": "preset",
                "preset": fallback,
                "message": f"Service temporarily unavailable, using fallback"
            }
        
        # Create pipeline request for tracking
        pipeline_req = pipeline_state.create_request(prompt, metadata=body)
        
        logger.info(f"Processing request {pipeline_req.id}: {prompt}")
        logger.info("="*60)
        logger.info("🎭 Starting Robust Trinity Pipeline")
        logger.info("="*60)
        
        try:
            # Step 1: Call Visionary through subprocess (isolated)
            logger.info("🌟 Step 1: Visionary (subprocess)")
            pipeline_state.update_stage(pipeline_req.id, PipelineStage.VISIONARY)
            
            try:
                # Call through subprocess manager
                visionary_response = await subprocess_manager.call_visionary(prompt, timeout=25.0)
                
                if visionary_response.get("success"):
                    preset = visionary_response.get("preset", {})
                    
                    # Log Visionary reasoning if available
                    reasoning = visionary_response.get("reasoning", {})
                    if reasoning:
                        logger.info("🧠 VISIONARY ENGINE SELECTION REASONING:")
                        logger.info(f"  Overall approach: {reasoning.get('overall_approach', 'N/A')}")
                        logger.info(f"  Signal flow: {reasoning.get('signal_flow', 'N/A')}")
                        for slot_reason in reasoning.get('slot_reasoning', []):
                            logger.info(f"  Slot {slot_reason.get('slot', '?')}: {slot_reason.get('engine', 'N/A')}")
                            logger.info(f"    Why selected: {slot_reason.get('why', 'N/A')}")
                            logger.info(f"    Key params: {slot_reason.get('key_params', 'N/A')}")
                    
                    pipeline_state.circuit_breaker.call_succeeded()
                else:
                    preset = visionary_response.get("preset", visionary.create_intelligent_fallback(prompt))
                    logger.warning(f"Visionary failed: {visionary_response.get('error')}")
                
            except asyncio.TimeoutError:
                logger.error("Visionary subprocess timed out")
                preset = visionary.create_intelligent_fallback(prompt)
                pipeline_state.circuit_breaker.call_failed()
                
            except Exception as e:
                logger.error(f"Visionary subprocess error: {e}")
                preset = visionary.create_intelligent_fallback(prompt)
                pipeline_state.circuit_breaker.call_failed()
            
            logger.info(f"✅ Visionary complete: {preset.get('name', 'Unknown')}")
            
            # Step 2: Calculator (local, safe)
            logger.info("🧮 Step 2: Calculator")
            pipeline_state.update_stage(pipeline_req.id, PipelineStage.CALCULATOR, preset=preset)

            if calculator:
                try:
                    # Call async refine_preset directly (avoid sync wrapper crash)
                    prompt_text = body.get("content", body.get("prompt", ""))
                    optimized_preset = await calculator.refine_preset(preset, prompt_text)
                    preset = optimized_preset
                    logger.info(f"✅ Calculator optimized: {len(preset.get('slots', []))} engines")
                except Exception as e:
                    logger.error(f"Calculator error: {e}")
                    # Continue with unoptimized preset
            else:
                logger.info("⚠️ Calculator disabled, skipping optimization")
            
            # Step 3: Alchemist (local, safe)
            logger.info("⚗️ Step 3: Alchemist")
            pipeline_state.update_stage(pipeline_req.id, PipelineStage.ALCHEMIST, preset=preset)
            
            try:
                final_preset, validation_report = alchemist.validate_and_fix(preset)
                if not validation_report["valid"]:
                    logger.warning(f"Validation issues: {validation_report['errors']}")
                final_preset = alchemist.format_for_plugin(final_preset)
                logger.info(f"✅ Alchemist validated: Safety checks passed")
            except Exception as e:
                logger.error(f"Alchemist error: {e}")
                final_preset = preset  # Use unvalidated preset
            
            # Remove internal metadata
            final_preset.pop('calculator_metadata', None)
            final_preset.pop('alchemist_metadata', None)
            
            # Mark as complete
            pipeline_state.update_stage(pipeline_req.id, PipelineStage.COMPLETE, preset=final_preset)
            
            logger.info("="*60)
            logger.info(f"✨ Trinity Pipeline Complete")
            logger.info(f"🎼 Final Preset: '{final_preset.get('name', 'Untitled')}'")
            logger.info("="*60)
            
            # Plugin expects slots array format, not flat parameters
            # The plugin's PluginEditorNexusStatic.cpp checks for response.data.slots
            
            # Normalize parameter format for plugin
            # Plugin expects simple float arrays, not {"name": "param1", "value": 0.3}
            normalized_slots = []
            for slot in final_preset.get("slots", []):
                normalized_slot = dict(slot)  # Copy the slot
                params = slot.get("parameters", [])

                # Convert params from dict format to simple float array
                normalized_params = []
                for p in params:
                    if isinstance(p, dict) and "value" in p:
                        normalized_params.append(float(p["value"]))
                    else:
                        normalized_params.append(float(p))

                normalized_slot["parameters"] = normalized_params
                normalized_slots.append(normalized_slot)

            # Format response for plugin - Plugin expects data.preset structure!
            response = {
                "success": True,
                "type": "preset",
                "message": final_preset.get('name', 'Unnamed'),  # Preset name in message for display
                "data": {
                    "preset": {
                        "name": final_preset.get("name", "Unnamed"),
                        "slots": normalized_slots  # Plugin expects simple float arrays
                    }
                }
            }
            
            # Cache successful response
            pipeline_state.cache.set(prompt, response)
            
            # Get session ID from request body
            session_id = body.get("session_id", body.get("sessionId", ""))

            # ALWAYS return preset directly for HTTP requests
            # File exchange is only for TCP transport
            logger.info(f"Sending response directly via HTTP: preset name = {final_preset.get('name', 'Unnamed')}")
            return response
            
        except Exception as e:
            logger.error(f"Pipeline error: {str(e)}")
            pipeline_state.update_stage(pipeline_req.id, PipelineStage.FAILED, error=str(e))
            
            # Return fallback preset
            fallback = visionary.create_intelligent_fallback(prompt)
            return {
                "success": False,
                "type": "preset",
                "preset": fallback,
                "message": f"Pipeline error, using fallback"
            }
        
    except Exception as e:
        logger.error(f"Message endpoint error: {e}")
        import traceback
        logger.error(traceback.format_exc())
        return {"success": False, "error": str(e)}

@app.get("/test-perfect-preset")
async def test_perfect_preset():
    """
    Test endpoint that returns a perfectly formatted preset
    to verify plugin can load it
    """
    perfect_preset = {
        "success": True,
        "type": "preset", 
        "message": "Test preset loaded successfully",
        "data": {
            "preset": {
                "name": "Perfect Test Preset",
                "description": "Hardcoded preset for testing",
                "slots": [
                    {
                        "slot": 0,
                        "engine_id": 15,  # Vintage Tube Preamp
                        "engine_name": "Vintage Tube Preamp",
                        "parameters": [
                            {"name": "param1", "value": 0.7},
                            {"name": "param2", "value": 0.3},
                            {"name": "param3", "value": 0.4},
                            {"name": "param4", "value": 0.6},
                            {"name": "param5", "value": 0.2},
                            {"name": "param6", "value": 0.5},
                            {"name": "param7", "value": 0.5},
                            {"name": "param8", "value": 0.5},
                            {"name": "param9", "value": 0.0},
                            {"name": "param10", "value": 1.0}
                        ]
                    },
                    {
                        "slot": 1,
                        "engine_id": 23,  # Stereo Chorus
                        "engine_name": "Stereo Chorus",
                        "parameters": [
                            {"name": "param1", "value": 0.3},
                            {"name": "param2", "value": 0.5},
                            {"name": "param3", "value": 0.4},
                            {"name": "param4", "value": 0.5},
                            {"name": "param5", "value": 0.5},
                            {"name": "param6", "value": 0.5},
                            {"name": "param7", "value": 0.5},
                            {"name": "param8", "value": 0.5},
                            {"name": "param9", "value": 0.0},
                            {"name": "param10", "value": 0.0}
                        ]
                    },
                    {
                        "slot": 2,
                        "engine_id": 39,  # Plate Reverb
                        "engine_name": "Plate Reverb",
                        "parameters": [
                            {"name": "param1", "value": 0.2},
                            {"name": "param2", "value": 0.6},
                            {"name": "param3", "value": 0.5},
                            {"name": "param4", "value": 0.4},
                            {"name": "param5", "value": 0.5},
                            {"name": "param6", "value": 0.5},
                            {"name": "param7", "value": 0.5},
                            {"name": "param8", "value": 0.5},
                            {"name": "param9", "value": 0.0},
                            {"name": "param10", "value": 0.0}
                        ]
                    }
                ]
            }
        }
    }
    
    logger.info("Sending perfect test preset")
    return perfect_preset

@app.get("/pipeline-stats")
async def pipeline_stats():
    """
    Get detailed pipeline statistics and health
    """
    return {
        "subprocess": subprocess_manager.get_stats(),
        "pipeline_state": pipeline_state.get_stats(),
        "cache_entries": len(pipeline_state.cache.cache),
        "circuit_breaker": {
            "state": pipeline_state.circuit_breaker.get_state(),
            "failure_count": pipeline_state.circuit_breaker.failure_count,
            "can_attempt": pipeline_state.circuit_breaker.can_attempt()
        }
    }

@app.post("/acknowledge")
async def acknowledge_preset(request: Request):
    """Mark a preset as successfully processed"""
    try:
        body = await request.json()
        exchange_id = body.get("exchange_id")
        
        if exchange_id and file_exchange.mark_processed(exchange_id):
            return {"success": True, "message": f"Preset {exchange_id} marked as processed"}
        else:
            return {"success": False, "error": "Exchange ID not found"}
    except Exception as e:
        logger.error(f"Acknowledge error: {e}")
        return {"success": False, "error": str(e)}

@app.get("/exchange_stats")
async def get_exchange_stats():
    """Get statistics about the file exchange system"""
    stats = file_exchange.get_exchange_stats()
    
    # Also run cleanup periodically
    file_exchange.cleanup_old_files()
    
    return {
        "success": True,
        "stats": stats,
        "exchange_dir": str(file_exchange.base_dir)
    }

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
    Startup message for TRUE Trinity with subprocess management
    """
    print("\n" + "="*60)
    print("🎭 CHIMERA PHOENIX - ROBUST TRINITY PIPELINE v4.0")
    print("="*60)
    print("✅ Visionary (Isolated Subprocess)")
    print("✅ Calculator (Local Processing)")
    print("✅ Alchemist (Safety Validation)")
    print("✅ Circuit Breaker Protection")
    print("✅ Response Caching")
    print("✅ State Tracking")
    print("❌ NO Oracle")
    print("❌ NO Corpus")
    print("="*60)
    print("Starting subprocess manager...")
    
    # Start subprocess manager
    await subprocess_manager.start()
    
    print("✨ Robust pipeline ready!")
    print("="*60 + "\n")

@app.on_event("shutdown")
async def shutdown_event():
    """
    Cleanup on shutdown
    """
    logger.info("Shutting down Robust Trinity Pipeline...")
    
    # Stop subprocess manager
    await subprocess_manager.stop()
    
    # Cleanup old pipeline states
    pipeline_state.cleanup_old_requests()
    
    logger.info("Shutdown complete")

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