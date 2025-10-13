"""
File-based exchange system for guaranteed preset delivery
"""
import os
import json
import time
import logging
from pathlib import Path
from typing import Dict, Optional, Any
from dataclasses import dataclass, asdict
import uuid

logger = logging.getLogger(__name__)

@dataclass
class PresetExchange:
    """Represents a preset exchange file"""
    id: str
    session_id: str
    preset_name: str
    preset_data: Dict[str, Any]
    timestamp: float
    status: str = "pending"
    
class FileExchangeManager:
    """Manages file-based preset exchange between server and plugin"""
    
    def __init__(self, base_dir: Optional[Path] = None):
        """Initialize the file exchange manager"""
        if base_dir is None:
            self.base_dir = Path.home() / ".chimera_phoenix" / "preset_exchange"
        else:
            self.base_dir = Path(base_dir)
        
        # Create directory structure
        self.base_dir.mkdir(parents=True, exist_ok=True)
        self.pending_dir = self.base_dir / "pending"
        self.pending_dir.mkdir(exist_ok=True)
        self.processed_dir = self.base_dir / "processed"
        self.processed_dir.mkdir(exist_ok=True)
        self.failed_dir = self.base_dir / "failed"
        self.failed_dir.mkdir(exist_ok=True)
        
        logger.info(f"FileExchangeManager initialized at {self.base_dir}")
    
    def write_preset(self, session_id: str, preset_data: Dict[str, Any]) -> str:
        """
        Write a preset to the exchange directory
        Returns the exchange ID
        """
        exchange_id = f"{session_id}_{int(time.time() * 1000)}_{uuid.uuid4().hex[:8]}"
        
        exchange = PresetExchange(
            id=exchange_id,
            session_id=session_id,
            preset_name=preset_data.get("name", "Unknown"),
            preset_data=preset_data,
            timestamp=time.time(),
            status="pending"
        )
        
        # Write the preset file
        preset_file = self.pending_dir / f"{exchange_id}.json"
        with open(preset_file, 'w') as f:
            json.dump(asdict(exchange), f, indent=2)
        
        # Write a marker file for the plugin to detect
        marker_file = self.pending_dir / f"{session_id}_READY.marker"
        with open(marker_file, 'w') as f:
            json.dump({
                "exchange_id": exchange_id,
                "preset_file": str(preset_file),
                "timestamp": time.time()
            }, f)
        
        logger.info(f"Preset written to exchange: {exchange_id}")
        return exchange_id
    
    def mark_processed(self, exchange_id: str) -> bool:
        """Mark a preset as successfully processed"""
        preset_file = self.pending_dir / f"{exchange_id}.json"
        if preset_file.exists():
            # Move to processed directory
            processed_file = self.processed_dir / f"{exchange_id}.json"
            preset_file.rename(processed_file)
            
            # Update status in file
            with open(processed_file, 'r') as f:
                data = json.load(f)
            data['status'] = 'processed'
            data['processed_time'] = time.time()
            with open(processed_file, 'w') as f:
                json.dump(data, f, indent=2)
            
            logger.info(f"Preset marked as processed: {exchange_id}")
            return True
        return False
    
    def mark_failed(self, exchange_id: str, error: str) -> bool:
        """Mark a preset as failed"""
        preset_file = self.pending_dir / f"{exchange_id}.json"
        if preset_file.exists():
            # Move to failed directory
            failed_file = self.failed_dir / f"{exchange_id}.json"
            preset_file.rename(failed_file)
            
            # Update status in file
            with open(failed_file, 'r') as f:
                data = json.load(f)
            data['status'] = 'failed'
            data['error'] = error
            data['failed_time'] = time.time()
            with open(failed_file, 'w') as f:
                json.dump(data, f, indent=2)
            
            logger.warning(f"Preset marked as failed: {exchange_id} - {error}")
            return True
        return False
    
    def get_pending_for_session(self, session_id: str) -> Optional[Dict[str, Any]]:
        """Get the next pending preset for a session"""
        marker_file = self.pending_dir / f"{session_id}_READY.marker"
        
        if marker_file.exists():
            try:
                with open(marker_file, 'r') as f:
                    marker_data = json.load(f)
                
                preset_file = Path(marker_data['preset_file'])
                if preset_file.exists():
                    with open(preset_file, 'r') as f:
                        return json.load(f)
            except Exception as e:
                logger.error(f"Error reading pending preset for {session_id}: {e}")
        
        return None
    
    def cleanup_old_files(self, max_age_seconds: int = 3600):
        """Clean up old files (default: older than 1 hour)"""
        current_time = time.time()
        cleaned_count = 0
        
        # Clean processed directory
        for file in self.processed_dir.glob("*.json"):
            if (current_time - file.stat().st_mtime) > max_age_seconds:
                file.unlink()
                cleaned_count += 1
        
        # Clean failed directory (keep longer for debugging)
        for file in self.failed_dir.glob("*.json"):
            if (current_time - file.stat().st_mtime) > (max_age_seconds * 24):  # 24 hours
                file.unlink()
                cleaned_count += 1
        
        # Clean orphaned marker files
        for marker in self.pending_dir.glob("*.marker"):
            if (current_time - marker.stat().st_mtime) > 300:  # 5 minutes
                # Check if corresponding preset file exists
                with open(marker, 'r') as f:
                    data = json.load(f)
                preset_file = Path(data.get('preset_file', ''))
                if not preset_file.exists():
                    marker.unlink()
                    cleaned_count += 1
        
        if cleaned_count > 0:
            logger.info(f"Cleaned up {cleaned_count} old files")
        
        return cleaned_count
    
    def get_exchange_stats(self) -> Dict[str, int]:
        """Get statistics about the exchange directory"""
        return {
            "pending": len(list(self.pending_dir.glob("*.json"))),
            "processed": len(list(self.processed_dir.glob("*.json"))),
            "failed": len(list(self.failed_dir.glob("*.json"))),
            "markers": len(list(self.pending_dir.glob("*.marker")))
        }