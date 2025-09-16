#!/usr/bin/env python3
"""
Comprehensive Production Readiness Test for Trinity Pipeline
Tests all components and identifies issues
"""

import asyncio
import json
import os
import sys
from pathlib import Path
import requests
import time
from typing import Dict, Any, List

# ANSI color codes for output
RED = '\033[91m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
RESET = '\033[0m'
BOLD = '\033[1m'

class TrinityPipelineTest:
    def __init__(self):
        self.server_url = "http://localhost:8001"  # Server on port 8001
        self.tcp_bridge_port = 9999
        self.issues = []
        self.warnings = []
        self.successes = []
        
    def print_header(self):
        print("\n" + "="*80)
        print(f"{BOLD}TRINITY PIPELINE PRODUCTION READINESS TEST{RESET}")
        print("="*80)
        print(f"Testing server at: {self.server_url}")
        print(f"Testing TCP bridge port: {self.tcp_bridge_port}")
        print("="*80 + "\n")
    
    def test_security(self):
        """Test for security issues"""
        print(f"{BLUE}[SECURITY CHECK]{RESET}")
        
        # Check for hardcoded API keys
        dangerous_files = [
            "test_openai_simple.py",
            "test_openai_connection.py", 
            "openai_bridge_server.py"
        ]
        
        for file in dangerous_files:
            filepath = Path(file)
            if filepath.exists():
                with open(filepath, 'r') as f:
                    content = f.read()
                    if 'sk-proj-' in content or 'sk-' in content:
                        self.issues.append(f"CRITICAL: Hardcoded API key found in {file}")
                        print(f"  {RED}✗ Hardcoded API key in {file}{RESET}")
        
        # Check .env file
        env_file = Path('.env')
        if env_file.exists():
            print(f"  {GREEN}✓ .env file exists{RESET}")
            self.successes.append(".env file configured")
            
            # Check if API key is present
            with open(env_file, 'r') as f:
                content = f.read()
                if 'OPENAI_API_KEY=' in content and 'sk-' in content:
                    print(f"  {GREEN}✓ OpenAI API key configured in .env{RESET}")
                    self.successes.append("OpenAI API key in .env")
                else:
                    self.warnings.append("OpenAI API key may not be properly configured")
        else:
            self.warnings.append("No .env file found")
            print(f"  {YELLOW}⚠ No .env file found{RESET}")
    
    def test_data_files(self):
        """Test if all required data files exist"""
        print(f"\n{BLUE}[DATA FILES CHECK]{RESET}")
        
        required_files = {
            "nudge_rules.json": "Sophisticated nudge rules",
            "parameter_manifest.json": "Parameter validation rules",
            "engine_defaults.py": "Engine parameter definitions",
            "engine_mapping.py": "Engine ID conversion"
        }
        
        for file, description in required_files.items():
            filepath = Path(file)
            if filepath.exists():
                print(f"  {GREEN}✓ {file}: {description}{RESET}")
                self.successes.append(f"{file} present")
                
                # Validate JSON files
                if file.endswith('.json'):
                    try:
                        with open(filepath, 'r') as f:
                            json.load(f)
                        print(f"    {GREEN}Valid JSON structure{RESET}")
                    except json.JSONDecodeError as e:
                        self.issues.append(f"Invalid JSON in {file}: {e}")
                        print(f"    {RED}Invalid JSON: {e}{RESET}")
            else:
                self.issues.append(f"Missing required file: {file}")
                print(f"  {RED}✗ Missing: {file}{RESET}")
    
    def test_server_health(self):
        """Test server health endpoint"""
        print(f"\n{BLUE}[SERVER HEALTH CHECK]{RESET}")
        
        try:
            response = requests.get(f"{self.server_url}/health", timeout=5)
            if response.status_code == 200:
                health = response.json()
                print(f"  {GREEN}✓ Server responding{RESET}")
                print(f"    Status: {health.get('status', 'unknown')}")
                print(f"    Version: {health.get('version', 'unknown')}")
                
                # Check components
                components = health.get('components', {})
                for component, status in components.items():
                    if status == "ready":
                        print(f"    {GREEN}✓ {component}: {status}{RESET}")
                    else:
                        print(f"    {YELLOW}⚠ {component}: {status}{RESET}")
                        self.warnings.append(f"{component} not ready: {status}")
                
                if health.get('status') == 'healthy':
                    self.successes.append("Server fully healthy")
                else:
                    self.warnings.append(f"Server status: {health.get('status')}")
            else:
                self.issues.append(f"Health check failed: HTTP {response.status_code}")
        except requests.exceptions.RequestException as e:
            self.issues.append(f"Cannot connect to server: {e}")
            print(f"  {RED}✗ Cannot connect to server: {e}{RESET}")
    
    def test_generate_endpoint(self):
        """Test preset generation endpoint"""
        print(f"\n{BLUE}[GENERATE ENDPOINT TEST]{RESET}")
        
        test_prompts = [
            "Create a warm vintage guitar tone",
            "Design an aggressive metal sound",
            "Build a spacious ambient pad"
        ]
        
        for prompt in test_prompts:
            print(f"\n  Testing: '{prompt[:40]}...'")
            
            try:
                response = requests.post(
                    f"{self.server_url}/generate",
                    json={"prompt": prompt, "max_generation_time": 10},
                    timeout=15
                )
                
                if response.status_code == 200:
                    result = response.json()
                    if result.get('success'):
                        preset = result.get('preset', {})
                        preset_name = preset.get('name', 'Unknown')
                        print(f"    {GREEN}✓ Generated: '{preset_name}'{RESET}")
                        
                        # Check preset structure
                        if 'parameters' in preset:
                            param_count = len(preset['parameters'])
                            print(f"      Parameters: {param_count}")
                            
                            # Check for 6 slots
                            slot_count = sum(1 for k in preset['parameters'] if 'slot' in k and '_engine' in k)
                            if slot_count == 6:
                                print(f"      {GREEN}✓ All 6 slots configured{RESET}")
                            else:
                                self.warnings.append(f"Only {slot_count}/6 slots configured")
                        
                        # Check metadata
                        metadata = result.get('metadata', {})
                        if metadata:
                            gen_time = metadata.get('generation_time_seconds', 0)
                            print(f"      Generation time: {gen_time:.2f}s")
                            
                            if gen_time > 5:
                                self.warnings.append(f"Slow generation: {gen_time:.2f}s")
                        
                        self.successes.append(f"Generated preset for: {prompt[:30]}")
                    else:
                        self.issues.append(f"Generation failed: {result.get('message')}")
                        print(f"    {RED}✗ Failed: {result.get('message')}{RESET}")
                else:
                    self.issues.append(f"HTTP {response.status_code} for prompt: {prompt[:30]}")
                    print(f"    {RED}✗ HTTP {response.status_code}{RESET}")
                    
            except requests.exceptions.Timeout:
                self.issues.append(f"Timeout for prompt: {prompt[:30]}")
                print(f"    {RED}✗ Request timed out{RESET}")
            except requests.exceptions.RequestException as e:
                self.issues.append(f"Request failed: {e}")
                print(f"    {RED}✗ Request failed: {e}{RESET}")
    
    def test_tcp_bridge(self):
        """Test TCP bridge connectivity"""
        print(f"\n{BLUE}[TCP BRIDGE CHECK]{RESET}")
        
        import socket
        
        try:
            # Try to connect to TCP bridge
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(2)
            result = sock.connect_ex(('localhost', self.tcp_bridge_port))
            sock.close()
            
            if result == 0:
                print(f"  {GREEN}✓ TCP bridge port {self.tcp_bridge_port} is open{RESET}")
                self.successes.append("TCP bridge accessible")
            else:
                print(f"  {YELLOW}⚠ TCP bridge not responding on port {self.tcp_bridge_port}{RESET}")
                self.warnings.append("TCP bridge not available (fallback will be used)")
        except Exception as e:
            print(f"  {YELLOW}⚠ Could not test TCP bridge: {e}{RESET}")
            self.warnings.append(f"TCP bridge test failed: {e}")
    
    def test_corpus_index(self):
        """Test FAISS index and corpus"""
        print(f"\n{BLUE}[CORPUS INDEX CHECK]{RESET}")
        
        index_path = Path("../JUCE_Plugin/GoldenCorpus/faiss_index/corpus.index")
        meta_path = Path("../JUCE_Plugin/GoldenCorpus/faiss_index/corpus_meta.pkl")
        presets_path = Path("../JUCE_Plugin/GoldenCorpus/all_presets.json")
        
        if index_path.exists():
            print(f"  {GREEN}✓ FAISS index exists{RESET}")
            self.successes.append("FAISS index present")
        else:
            self.issues.append("FAISS index missing")
            print(f"  {RED}✗ FAISS index missing{RESET}")
        
        if meta_path.exists():
            print(f"  {GREEN}✓ Corpus metadata exists{RESET}")
        else:
            self.warnings.append("Corpus metadata missing")
            print(f"  {YELLOW}⚠ Corpus metadata missing{RESET}")
        
        if presets_path.exists():
            try:
                with open(presets_path, 'r') as f:
                    data = json.load(f)
                    preset_count = len(data.get('presets', []))
                    print(f"  {GREEN}✓ Corpus has {preset_count} presets{RESET}")
                    
                    if preset_count < 250:
                        self.warnings.append(f"Only {preset_count}/250 presets in corpus")
            except Exception as e:
                self.issues.append(f"Cannot read corpus: {e}")
                print(f"  {RED}✗ Cannot read corpus: {e}{RESET}")
        else:
            self.issues.append("Preset corpus missing")
            print(f"  {RED}✗ Preset corpus missing{RESET}")
    
    def print_summary(self):
        """Print test summary and production readiness assessment"""
        print("\n" + "="*80)
        print(f"{BOLD}TEST SUMMARY{RESET}")
        print("="*80)
        
        # Successes
        if self.successes:
            print(f"\n{GREEN}{BOLD}SUCCESSES ({len(self.successes)}):{RESET}")
            for success in self.successes:
                print(f"  {GREEN}✓{RESET} {success}")
        
        # Warnings
        if self.warnings:
            print(f"\n{YELLOW}{BOLD}WARNINGS ({len(self.warnings)}):{RESET}")
            for warning in self.warnings:
                print(f"  {YELLOW}⚠{RESET} {warning}")
        
        # Issues
        if self.issues:
            print(f"\n{RED}{BOLD}CRITICAL ISSUES ({len(self.issues)}):{RESET}")
            for issue in self.issues:
                print(f"  {RED}✗{RESET} {issue}")
        
        # Production readiness assessment
        print("\n" + "="*80)
        print(f"{BOLD}PRODUCTION READINESS ASSESSMENT{RESET}")
        print("="*80)
        
        if not self.issues:
            if not self.warnings:
                print(f"{GREEN}{BOLD}✓ READY FOR PRODUCTION{RESET}")
                print("All tests passed successfully!")
            else:
                print(f"{YELLOW}{BOLD}⚠ READY WITH WARNINGS{RESET}")
                print("System is functional but has minor issues.")
        else:
            print(f"{RED}{BOLD}✗ NOT READY FOR PRODUCTION{RESET}")
            print("Critical issues must be resolved before deployment.")
            
            # Provide recommendations
            print(f"\n{BOLD}REQUIRED FIXES:{RESET}")
            
            if any("Hardcoded API key" in issue for issue in self.issues):
                print(f"  1. {RED}REMOVE ALL HARDCODED API KEYS{RESET}")
                print("     - Delete keys from test files")
                print("     - Use environment variables only")
                print("     - Regenerate compromised keys")
            
            if any("FAISS index" in issue for issue in self.issues):
                print(f"  2. {RED}BUILD FAISS INDEX{RESET}")
                print("     - Run corpus indexing script")
                print("     - Verify corpus data exists")
            
            if any("Missing required file" in issue for issue in self.issues):
                print(f"  3. {RED}ENSURE ALL DATA FILES PRESENT{RESET}")
                print("     - Check nudge_rules.json")
                print("     - Check parameter_manifest.json")
        
        print("\n" + "="*80)

def main():
    tester = TrinityPipelineTest()
    tester.print_header()
    
    # Run all tests
    tester.test_security()
    tester.test_data_files()
    tester.test_server_health()
    tester.test_corpus_index()
    tester.test_tcp_bridge()
    tester.test_generate_endpoint()
    
    # Print summary
    tester.print_summary()

if __name__ == "__main__":
    main()