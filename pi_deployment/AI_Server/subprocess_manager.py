#!/usr/bin/env python3
"""
Subprocess Manager - Manages isolated subprocess for Visionary
"""

import asyncio
import subprocess
import json
import logging
import sys
import os
import signal
from typing import Dict, Any, Optional
from asyncio import Queue, Lock
import time

logger = logging.getLogger(__name__)

class SubprocessManager:
    """Manages the Visionary subprocess with automatic restart on failure"""
    
    def __init__(self, max_retries: int = 3):
        self.process: Optional[asyncio.subprocess.Process] = None
        self.max_retries = max_retries
        self.request_queue: Queue = Queue()
        self.response_futures: Dict[str, asyncio.Future] = {}
        self.lock = Lock()
        self.running = False
        self.restart_count = 0
        self.last_restart_time = 0
    
    async def start(self):
        """Start the subprocess and worker tasks"""
        if self.running:
            return
        
        self.running = True
        
        # Start subprocess
        await self._start_subprocess()
        
        # Start worker tasks
        asyncio.create_task(self._request_worker())
        asyncio.create_task(self._response_worker())
        asyncio.create_task(self._health_monitor())
        
        logger.info("Subprocess manager started")
    
    async def stop(self):
        """Stop the subprocess and cleanup"""
        self.running = False
        
        if self.process:
            try:
                self.process.terminate()
                await asyncio.wait_for(self.process.wait(), timeout=5.0)
            except asyncio.TimeoutError:
                self.process.kill()
                await self.process.wait()
            except Exception as e:
                logger.error(f"Error stopping subprocess: {e}")
            
            self.process = None
        
        # Cancel pending futures
        for future in self.response_futures.values():
            if not future.done():
                future.cancel()
        
        self.response_futures.clear()
        logger.info("Subprocess manager stopped")
    
    async def _start_subprocess(self):
        """Start the Visionary subprocess"""
        try:
            # Start subprocess with isolated Python environment
            self.process = await asyncio.create_subprocess_exec(
                sys.executable,
                "visionary_subprocess.py",
                stdin=asyncio.subprocess.PIPE,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                cwd=os.path.dirname(os.path.abspath(__file__)),
                # Limit memory usage to prevent system crashes
                preexec_fn=lambda: signal.signal(signal.SIGINT, signal.SIG_IGN) if os.name != 'nt' else None
            )
            
            logger.info(f"Started Visionary subprocess (PID: {self.process.pid})")
            self.restart_count = 0
            
        except Exception as e:
            logger.error(f"Failed to start subprocess: {e}")
            raise
    
    async def _restart_subprocess(self):
        """Restart the subprocess after failure"""
        async with self.lock:
            # Check restart rate limiting
            current_time = time.time()
            if current_time - self.last_restart_time < 10:
                logger.warning("Subprocess restart rate limited, waiting...")
                await asyncio.sleep(10)
            
            self.last_restart_time = current_time
            self.restart_count += 1
            
            if self.restart_count > self.max_retries:
                logger.error(f"Subprocess failed {self.max_retries} times, giving up")
                self.running = False
                return False
            
            logger.info(f"Restarting subprocess (attempt {self.restart_count}/{self.max_retries})")
            
            # Kill existing process if any
            if self.process:
                try:
                    self.process.kill()
                    await self.process.wait()
                except:
                    pass
                self.process = None
            
            # Start new process
            try:
                await self._start_subprocess()
                return True
            except Exception as e:
                logger.error(f"Failed to restart subprocess: {e}")
                return False
    
    async def _request_worker(self):
        """Worker that sends requests to subprocess"""
        while self.running:
            try:
                # Get request from queue
                request = await self.request_queue.get()
                
                if not self.process or self.process.returncode is not None:
                    # Process is dead, restart it
                    if not await self._restart_subprocess():
                        # Failed to restart, fail the request
                        request_id = request.get("id", "unknown")
                        if request_id in self.response_futures:
                            self.response_futures[request_id].set_exception(
                                Exception("Subprocess unavailable")
                            )
                        continue
                
                # Send request to subprocess
                request_json = json.dumps(request) + "\n"
                self.process.stdin.write(request_json.encode())
                await self.process.stdin.drain()
                
                logger.debug(f"Sent request {request.get('id')} to subprocess")
                
            except Exception as e:
                logger.error(f"Request worker error: {e}")
                await asyncio.sleep(1)
    
    async def _response_worker(self):
        """Worker that reads responses from subprocess"""
        while self.running:
            try:
                if not self.process or self.process.returncode is not None:
                    await asyncio.sleep(1)
                    continue
                
                # Read response line
                line = await self.process.stdout.readline()
                
                if not line:
                    # Process died
                    logger.warning("Subprocess stdout closed, process may have died")
                    await self._restart_subprocess()
                    continue
                
                # Parse response
                try:
                    response = json.loads(line.decode().strip())
                    request_id = response.get("id", "unknown")
                    
                    # Deliver response to waiting future
                    if request_id in self.response_futures:
                        future = self.response_futures.pop(request_id)
                        if not future.done():
                            future.set_result(response)
                        
                        logger.debug(f"Delivered response for request {request_id}")
                    
                except json.JSONDecodeError as e:
                    logger.error(f"Failed to parse subprocess response: {e}")
                
            except Exception as e:
                logger.error(f"Response worker error: {e}")
                await asyncio.sleep(1)
    
    async def _health_monitor(self):
        """Monitor subprocess health and restart if needed"""
        while self.running:
            try:
                await asyncio.sleep(30)  # Check every 30 seconds
                
                if self.process and self.process.returncode is not None:
                    logger.warning(f"Subprocess died with code {self.process.returncode}")
                    await self._restart_subprocess()
                
                # Check for stuck futures (timeout after 60 seconds)
                current_time = time.time()
                for request_id, future in list(self.response_futures.items()):
                    if not future.done():
                        # Check if future has custom timeout attribute
                        timeout = getattr(future, '_timeout_time', current_time + 60)
                        if current_time > timeout:
                            self.response_futures.pop(request_id)
                            future.set_exception(asyncio.TimeoutError("Request timed out"))
                            logger.warning(f"Request {request_id} timed out")
                
                # Log subprocess stderr if any
                if self.process and self.process.stderr:
                    try:
                        stderr_data = await asyncio.wait_for(
                            self.process.stderr.read(1024),
                            timeout=0.1
                        )
                        if stderr_data:
                            logger.info(f"Subprocess: {stderr_data.decode().strip()}")
                    except asyncio.TimeoutError:
                        pass
                
            except Exception as e:
                logger.error(f"Health monitor error: {e}")
    
    async def call_visionary(self, prompt: str, timeout: float = 30.0) -> Dict[str, Any]:
        """Call Visionary through subprocess with timeout"""
        request_id = f"req_{int(time.time() * 1000)}_{hash(prompt) % 10000}"
        
        # Create request
        request = {
            "id": request_id,
            "prompt": prompt
        }
        
        # Create future for response
        future = asyncio.Future()
        future._timeout_time = time.time() + timeout
        self.response_futures[request_id] = future
        
        # Queue request
        await self.request_queue.put(request)
        
        try:
            # Wait for response with timeout
            response = await asyncio.wait_for(future, timeout=timeout)
            return response
            
        except asyncio.TimeoutError:
            logger.error(f"Visionary call timed out for prompt: {prompt[:50]}...")
            # Remove from futures if still there
            self.response_futures.pop(request_id, None)
            raise
            
        except Exception as e:
            logger.error(f"Visionary call failed: {e}")
            self.response_futures.pop(request_id, None)
            raise
    
    def get_stats(self) -> Dict[str, Any]:
        """Get subprocess manager statistics"""
        return {
            "running": self.running,
            "process_pid": self.process.pid if self.process else None,
            "restart_count": self.restart_count,
            "pending_requests": self.request_queue.qsize(),
            "waiting_responses": len(self.response_futures),
            "process_alive": self.process and self.process.returncode is None
        }