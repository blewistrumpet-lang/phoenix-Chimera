"""
Plugin Compatibility Endpoints for Trinity AI Server
Adds endpoints that the JUCE plugin expects
"""

from fastapi import APIRouter
from typing import Dict, Any, List
import logging

logger = logging.getLogger(__name__)

# Create router for plugin-specific endpoints
plugin_router = APIRouter(prefix="", tags=["plugin"])

# Session storage for async responses
sessions = {}

@plugin_router.get("/ping")
async def ping():
    """Simple ping endpoint for connection testing"""
    return {"status": "ok", "message": "Trinity AI Server is running"}

@plugin_router.post("/suggestions")
async def get_suggestions(request: Dict[str, Any]):
    """Get modification suggestions for current preset"""
    from preset_modifier import PresetModifier
    
    modifier = PresetModifier()
    current_preset = request.get("preset", {})
    
    try:
        suggestions = modifier.suggest_modifications(current_preset)
        return {
            "success": True,
            "type": "suggestions",
            "suggestions": suggestions
        }
    except Exception as e:
        logger.error(f"Error getting suggestions: {e}")
        return {
            "success": False,
            "type": "error",
            "suggestions": []
        }

@plugin_router.post("/modify")
async def modify_preset(request: Dict[str, Any]):
    """
    Handle preset modification requests using Trinity Architecture:
    Visionary → Calculator → Alchemist
    """
    from cloud_bridge import get_modification_analysis
    from calculator import Calculator
    from alchemist import Alchemist
    
    # Extract current preset and modification request
    current_preset = request.get("preset", {})
    modification = request.get("modification", "")
    
    logger.info(f"Received modification request: {modification[:50]}...")
    
    if not current_preset or not modification:
        return {
            "success": False,
            "type": "error",
            "message": "Missing preset or modification request"
        }
    
    try:
        # Step 1: VISIONARY - Analyze modification request
        logger.info("Step 1: Visionary analyzing modification request...")
        modification_blueprint = await get_modification_analysis(current_preset, modification)
        
        logger.info(f"Visionary intent: {modification_blueprint.get('intent', 'unknown')}")
        logger.info(f"Mood shift: {modification_blueprint.get('mood_shift', 'none')}")
        logger.info(f"Parameter targets: {len(modification_blueprint.get('parameter_targets', {}))}")
        
        # Step 2: CALCULATOR - Apply intelligent nudges
        logger.info("Step 2: Calculator applying modifications...")
        calculator = Calculator()
        modified_preset = calculator.apply_modification_nudges(current_preset, modification_blueprint)
        
        # Extract change information
        mod_metadata = modified_preset.get("modification_metadata", {})
        change_log = mod_metadata.get("change_log", {})
        total_changes = change_log.get("total_adjustments", 0)
        
        logger.info(f"Calculator applied {total_changes} parameter changes")
        
        # Step 3: ALCHEMIST - Validate and finalize
        logger.info("Step 3: Alchemist validating modifications...")
        alchemist = Alchemist()
        
        # Preserve the original name unless explicitly changed
        preserve_name = "name" not in modification.lower()
        if preserve_name:
            original_name = current_preset.get("name", "")
            modified_preset["name"] = original_name
        
        final_preset = alchemist.finalize_preset(modified_preset)
        
        # Prepare response
        applied_changes = []
        for change in change_log.get("applied_changes", [])[:5]:  # Show first 5 changes
            if isinstance(change, dict):
                if change.get("type") == "engine_suggestion":
                    applied_changes.append(f"{change['action']} {change['engine']}")
                elif "parameter" in change:
                    param = change["parameter"]
                    adj = change.get("adjustment", 0)
                    reason = change.get("reason", "")
                    applied_changes.append(f"{param}: {adj:+.2f} ({reason})")
        
        # Create summary message
        intent = modification_blueprint.get("intent", modification[:30])
        mood = modification_blueprint.get("mood_shift", "")
        
        if mood and mood != "adjusted":
            message = f"Applied {mood} modification: {intent}"
        else:
            message = f"Applied: {intent}"
        
        if total_changes > 0:
            message += f" ({total_changes} parameters adjusted)"
        
        return {
            "success": True,
            "type": "modification",
            "message": message,
            "data": final_preset,
            "changes": applied_changes,
            "metadata": {
                "intent": intent,
                "mood_shift": mood,
                "total_changes": total_changes,
                "affected_parameters": mod_metadata.get("affected_parameters", [])
            }
        }
        
    except Exception as e:
        logger.error(f"Error in Trinity modification pipeline: {e}")
        import traceback
        traceback.print_exc()
        
        # Fallback to simple modifier if Trinity fails
        try:
            logger.info("Falling back to simple preset modifier...")
            from preset_modifier import PresetModifier
            modifier = PresetModifier()
            modified_preset = modifier.modify_preset(current_preset, modification)
            
            return {
                "success": True,
                "type": "modification",
                "message": "Modification applied (fallback)",
                "data": modified_preset,
                "changes": []
            }
        except Exception as fallback_error:
            logger.error(f"Fallback also failed: {fallback_error}")
            return {
                "success": False,
                "type": "error",
                "message": f"Modification failed: {str(e)}"
            }

@plugin_router.post("/message")
async def handle_message(request: Dict[str, Any]):
    """Handle messages from the plugin"""
    from main import generate_preset, GenerateRequest
    
    # Extract prompt from plugin message
    prompt = request.get("content", request.get("prompt", ""))
    message_type = request.get("type", "query")
    
    logger.info(f"Received plugin message: type={message_type}, prompt={prompt[:50]}...")
    
    if not prompt:
        return {
            "success": False, 
            "type": "error",
            "message": "No prompt provided"
        }
    
    # Handle heartbeat ping - don't generate presets
    if message_type == "heartbeat" or prompt.strip().lower() == "ping":
        return {
            "success": True,
            "type": "heartbeat",
            "message": "pong",
            "data": None
        }
    
    try:
        # Generate preset using the Trinity pipeline
        generate_request = GenerateRequest(prompt=prompt)
        response = await generate_preset(generate_request)
        
        # Convert response to plugin format
        # Extract just the preset name from the message
        preset_name = response.preset.get('name', 'Untitled') if response.preset else 'Untitled'
        
        plugin_response = {
            "success": response.success,
            "type": "response",
            "message": preset_name,  # Send just the preset name
            "data": response.preset if response.preset else None,
            "metadata": {
                "generation_time": response.metadata.get("generation_time_seconds", 0),
                "pipeline_version": response.metadata.get("pipeline_version", "3.0"),
                "full_message": response.message  # Keep original message in metadata
            }
        }
        
        logger.info(f"Successfully generated response for prompt")
        return plugin_response
        
    except Exception as e:
        logger.error(f"Error processing plugin message: {e}")
        return {
            "success": False,
            "type": "error", 
            "message": f"Error processing request: {str(e)}"
        }

@plugin_router.get("/poll")
async def poll(session: str = None):
    """Polling endpoint for async responses"""
    # Check if there are any pending messages for this session
    if session and session in sessions:
        messages = sessions.get(session, [])
        # Clear messages after sending
        sessions[session] = []
        return {"messages": messages, "session": session}
    
    # Return empty response if no messages
    return {"messages": [], "session": session}

@plugin_router.post("/session/start")
async def start_session(request: Dict[str, Any]):
    """Start a new session"""
    session_id = request.get("session_id", f"session_{id(request)}")
    sessions[session_id] = []
    return {
        "success": True,
        "session_id": session_id,
        "message": "Session started"
    }

@plugin_router.post("/session/end")
async def end_session(request: Dict[str, Any]):
    """End a session"""
    session_id = request.get("session_id")
    if session_id in sessions:
        del sessions[session_id]
    return {
        "success": True,
        "message": "Session ended"
    }