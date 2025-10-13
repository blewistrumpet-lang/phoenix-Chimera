#!/usr/bin/env python3
"""
Pipeline State Manager - Tracks pipeline execution state for resilience
"""

import json
import time
from typing import Dict, Any, Optional, List
from enum import Enum
from dataclasses import dataclass, asdict
from datetime import datetime
import hashlib
import os
import asyncio

class PipelineStage(Enum):
    """Pipeline stages"""
    QUEUED = "queued"
    VISIONARY = "visionary"
    CALCULATOR = "calculator"
    ALCHEMIST = "alchemist"
    COMPLETE = "complete"
    FAILED = "failed"

@dataclass
class PipelineRequest:
    """Pipeline request with state tracking"""
    id: str
    prompt: str
    stage: PipelineStage
    preset: Dict[str, Any]
    created_at: float
    updated_at: float
    attempts: int
    error: Optional[str] = None
    metadata: Dict[str, Any] = None
    
    def to_dict(self):
        data = asdict(self)
        data['stage'] = self.stage.value
        return data
    
    @classmethod
    def from_dict(cls, data: Dict):
        data['stage'] = PipelineStage(data['stage'])
        return cls(**data)

class ResponseCache:
    """Simple in-memory cache for responses"""
    
    def __init__(self, ttl_seconds: int = 300):
        self.cache: Dict[str, tuple[Dict, float]] = {}
        self.ttl = ttl_seconds
    
    def get_key(self, prompt: str) -> str:
        """Generate cache key from prompt"""
        return hashlib.md5(prompt.lower().strip().encode()).hexdigest()
    
    def get(self, prompt: str) -> Optional[Dict]:
        """Get cached response if available and not expired"""
        key = self.get_key(prompt)
        if key in self.cache:
            response, timestamp = self.cache[key]
            if time.time() - timestamp < self.ttl:
                return response
            else:
                del self.cache[key]
        return None
    
    def set(self, prompt: str, response: Dict):
        """Cache a response"""
        key = self.get_key(prompt)
        self.cache[key] = (response, time.time())
    
    def clear_expired(self):
        """Remove expired entries"""
        current_time = time.time()
        expired_keys = [
            key for key, (_, timestamp) in self.cache.items()
            if current_time - timestamp >= self.ttl
        ]
        for key in expired_keys:
            del self.cache[key]

class CircuitBreaker:
    """Circuit breaker pattern for API calls"""
    
    def __init__(self, failure_threshold: int = 3, recovery_timeout: int = 60):
        self.failure_threshold = failure_threshold
        self.recovery_timeout = recovery_timeout
        self.failure_count = 0
        self.last_failure_time = 0
        self.state = "closed"  # closed, open, half-open
    
    def call_succeeded(self):
        """Record successful call"""
        self.failure_count = 0
        self.state = "closed"
    
    def call_failed(self):
        """Record failed call"""
        self.failure_count += 1
        self.last_failure_time = time.time()
        
        if self.failure_count >= self.failure_threshold:
            self.state = "open"
    
    def can_attempt(self) -> bool:
        """Check if we can attempt a call"""
        if self.state == "closed":
            return True
        
        if self.state == "open":
            # Check if recovery timeout has passed
            if time.time() - self.last_failure_time > self.recovery_timeout:
                self.state = "half-open"
                return True
            return False
        
        # Half-open state - allow one attempt
        return True
    
    def get_state(self) -> str:
        """Get current circuit breaker state"""
        return self.state

class PipelineStateManager:
    """Manages pipeline execution state"""
    
    def __init__(self):
        self.requests: Dict[str, PipelineRequest] = {}
        self.cache = ResponseCache(ttl_seconds=300)  # 5 minute cache
        self.circuit_breaker = CircuitBreaker(
            failure_threshold=3,
            recovery_timeout=60
        )
        
        # Message queues for HTTP polling sessions
        self.session_messages: Dict[str, asyncio.Queue] = {}
        
        # Create state directory if needed
        self.state_dir = "pipeline_state"
        os.makedirs(self.state_dir, exist_ok=True)
    
    def create_request(self, prompt: str, metadata: Dict = None) -> PipelineRequest:
        """Create a new pipeline request"""
        request_id = f"req_{int(time.time() * 1000)}_{hash(prompt) % 10000}"
        
        request = PipelineRequest(
            id=request_id,
            prompt=prompt,
            stage=PipelineStage.QUEUED,
            preset={},
            created_at=time.time(),
            updated_at=time.time(),
            attempts=0,
            metadata=metadata or {}
        )
        
        self.requests[request_id] = request
        self.save_request(request)
        
        return request
    
    def update_stage(self, request_id: str, stage: PipelineStage, 
                     preset: Dict = None, error: str = None):
        """Update request stage"""
        if request_id not in self.requests:
            return
        
        request = self.requests[request_id]
        request.stage = stage
        request.updated_at = time.time()
        
        if preset:
            request.preset = preset
        
        if error:
            request.error = error
            request.attempts += 1
        
        self.save_request(request)
    
    def get_request(self, request_id: str) -> Optional[PipelineRequest]:
        """Get request by ID"""
        return self.requests.get(request_id)
    
    def get_stuck_requests(self, timeout_seconds: int = 120) -> List[PipelineRequest]:
        """Get requests that are stuck (not complete/failed and timed out)"""
        current_time = time.time()
        stuck = []
        
        for request in self.requests.values():
            if request.stage not in [PipelineStage.COMPLETE, PipelineStage.FAILED]:
                if current_time - request.updated_at > timeout_seconds:
                    stuck.append(request)
        
        return stuck
    
    def save_request(self, request: PipelineRequest):
        """Persist request state to disk"""
        filepath = os.path.join(self.state_dir, f"{request.id}.json")
        with open(filepath, 'w') as f:
            json.dump(request.to_dict(), f)
    
    def load_request(self, request_id: str) -> Optional[PipelineRequest]:
        """Load request from disk"""
        filepath = os.path.join(self.state_dir, f"{request_id}.json")
        if os.path.exists(filepath):
            with open(filepath, 'r') as f:
                data = json.load(f)
                return PipelineRequest.from_dict(data)
        return None
    
    def cleanup_old_requests(self, max_age_seconds: int = 3600):
        """Remove old completed/failed requests"""
        current_time = time.time()
        to_remove = []
        
        for request_id, request in self.requests.items():
            if request.stage in [PipelineStage.COMPLETE, PipelineStage.FAILED]:
                if current_time - request.updated_at > max_age_seconds:
                    to_remove.append(request_id)
        
        for request_id in to_remove:
            del self.requests[request_id]
            filepath = os.path.join(self.state_dir, f"{request_id}.json")
            if os.path.exists(filepath):
                os.remove(filepath)
    
    def get_stats(self) -> Dict[str, Any]:
        """Get pipeline statistics"""
        stats = {
            "total_requests": len(self.requests),
            "by_stage": {},
            "circuit_breaker": self.circuit_breaker.get_state(),
            "cache_size": len(self.cache.cache),
            "oldest_request": None,
            "stuck_requests": len(self.get_stuck_requests())
        }
        
        # Count by stage
        for stage in PipelineStage:
            count = sum(1 for r in self.requests.values() if r.stage == stage)
            stats["by_stage"][stage.value] = count
        
        # Find oldest non-complete request
        non_complete = [
            r for r in self.requests.values() 
            if r.stage not in [PipelineStage.COMPLETE, PipelineStage.FAILED]
        ]
        if non_complete:
            oldest = min(non_complete, key=lambda r: r.created_at)
            stats["oldest_request"] = {
                "id": oldest.id,
                "age_seconds": time.time() - oldest.created_at,
                "stage": oldest.stage.value
            }
        
        return stats