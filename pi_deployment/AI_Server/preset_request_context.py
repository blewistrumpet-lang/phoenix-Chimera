"""
Preset Request Context - Single source of truth flowing through the pipeline
"""

from typing import Dict, Any, List, Optional
from dataclasses import dataclass, field
import logging

logger = logging.getLogger(__name__)

@dataclass
class PresetRequestContext:
    """
    Single source of truth that flows through the entire Trinity pipeline.
    Ensures no information is lost between components.
    """
    
    # Original request
    prompt: str
    
    # Extracted requirements
    required_engines: List[int] = field(default_factory=list)
    avoid_engines: List[int] = field(default_factory=list)
    
    # Musical analysis
    genre: Optional[str] = None
    instrument: Optional[str] = None
    character: List[str] = field(default_factory=list)
    reference_artist: Optional[str] = None
    
    # Cloud AI results
    cloud_blueprint: Dict[str, Any] = field(default_factory=dict)
    cloud_suggested_engines: List[int] = field(default_factory=list)
    
    # Oracle results
    oracle_preset: Dict[str, Any] = field(default_factory=dict)
    oracle_explanation: str = ""
    oracle_score: float = 0.0
    
    # Calculator adjustments
    calculator_nudges: Dict[str, Any] = field(default_factory=dict)
    calculator_added_engines: List[int] = field(default_factory=list)
    
    # Final result
    final_preset: Dict[str, Any] = field(default_factory=dict)
    
    # Tracking
    warnings: List[str] = field(default_factory=list)
    pipeline_stage: str = "initialized"
    
    def add_required_engine(self, engine_id: int, reason: str = ""):
        """Add an engine that MUST be included"""
        if engine_id not in self.required_engines:
            self.required_engines.append(engine_id)
            logger.info(f"Required engine {engine_id} added: {reason}")
    
    def add_warning(self, warning: str):
        """Add a warning message"""
        self.warnings.append(warning)
        logger.warning(f"Pipeline warning: {warning}")
    
    def update_stage(self, stage: str):
        """Update current pipeline stage"""
        self.pipeline_stage = stage
        logger.info(f"Pipeline stage: {stage}")
    
    def get_all_required_engines(self) -> List[int]:
        """Get all engines that must be included (from all sources)"""
        all_required = set()
        
        # User explicitly requested
        all_required.update(self.required_engines)
        
        # Cloud AI suggested (high confidence)
        all_required.update(self.cloud_suggested_engines)
        
        return list(all_required)
    
    def verify_requirements_met(self, preset: Dict[str, Any]) -> bool:
        """Check if all requirements are satisfied in the preset"""
        required = self.get_all_required_engines()
        
        # Get engines in preset
        preset_engines = []
        for slot in range(1, 7):
            engine = preset.get(f"slot{slot}_engine", 0)
            if engine > 0:
                preset_engines.append(engine)
        
        # Check all required engines are present
        missing = [e for e in required if e not in preset_engines]
        
        if missing:
            from engine_mapping_authoritative import ENGINE_NAMES
            for engine_id in missing:
                engine_name = ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})")
                self.add_warning(f"Required engine {engine_name} is missing!")
            return False
        
        return True
    
    def force_include_requirements(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Force all required engines into the preset"""
        required = self.get_all_required_engines()
        
        # Get current engines
        current_engines = []
        for slot in range(1, 7):
            engine = preset.get(f"slot{slot}_engine", 0)
            if engine > 0:
                current_engines.append(engine)
        
        # Add missing required engines
        missing = [e for e in required if e not in current_engines]
        
        for engine_id in missing:
            # Find empty slot
            added = False
            for slot in range(1, 7):
                if preset.get(f"slot{slot}_engine", 0) == 0:
                    preset[f"slot{slot}_engine"] = engine_id
                    from engine_mapping_authoritative import ENGINE_NAMES
                    logger.info(f"Forced required engine {ENGINE_NAMES.get(engine_id, f'Unknown({engine_id})')} into slot {slot}")
                    added = True
                    break
            
            if not added:
                # Replace least important engine (last slot that's not required)
                for slot in range(6, 0, -1):
                    current = preset.get(f"slot{slot}_engine", 0)
                    if current not in required:
                        preset[f"slot{slot}_engine"] = engine_id
                        logger.info(f"Replaced slot {slot} with required engine {engine_id}")
                        break
        
        return preset
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for serialization"""
        return {
            "prompt": self.prompt,
            "required_engines": self.required_engines,
            "genre": self.genre,
            "instrument": self.instrument,
            "character": self.character,
            "warnings": self.warnings,
            "pipeline_stage": self.pipeline_stage
        }