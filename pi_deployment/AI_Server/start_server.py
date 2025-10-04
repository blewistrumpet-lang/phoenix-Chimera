#!/usr/bin/env python3
"""
Startup script for Trinity AI Server
Handles environment setup and dependency checking
"""

import sys
import os
import subprocess
import importlib.util

def check_dependency(module_name):
    """Check if a Python module is installed"""
    spec = importlib.util.find_spec(module_name)
    return spec is not None

def main():
    print("Trinity AI Server Starting...")
    print("=" * 50)
    
    # Check critical dependencies
    required_modules = [
        ("fastapi", "pip install fastapi"),
        ("uvicorn", "pip install uvicorn"),
        ("openai", "pip install openai"),
        ("faiss", "pip install faiss-cpu"),
        ("numpy", "pip install numpy"),
        ("pydantic", "pip install pydantic")
    ]
    
    missing = []
    for module, install_cmd in required_modules:
        if not check_dependency(module):
            missing.append((module, install_cmd))
    
    if missing:
        print("ERROR: Missing required dependencies:")
        for module, cmd in missing:
            print(f"  - {module}: Run '{cmd}'")
        print("\nOr run: pip install -r requirements.txt")
        sys.exit(1)
    
    # Check for FAISS index
    faiss_path = os.path.join(os.path.dirname(__file__), 
                             "../JUCE_Plugin/GoldenCorpus/faiss_index/corpus.index")
    if not os.path.exists(faiss_path):
        print("WARNING: FAISS index not found at", faiss_path)
        print("The Oracle component will not work properly.")
        print("Generate it by running export_golden_corpus.sh")
        print()
    
    # Check for API key
    api_key = os.environ.get("OPENAI_API_KEY")
    if not api_key:
        print("WARNING: OPENAI_API_KEY not set")
        print("The Visionary will use simulation mode.")
        print("To use OpenAI, set OPENAI_API_KEY environment variable")
        print()
    else:
        print("✓ OpenAI API key found")
    
    # Import and start the server
    try:
        import uvicorn
        from main import app
        
        print("Starting Trinity AI Server on http://localhost:8000")
        print("Press Ctrl+C to stop")
        print("-" * 50)
        
        # Start the server
        uvicorn.run(app, host="0.0.0.0", port=8000, log_level="info")
        
    except KeyboardInterrupt:
        print("\nShutting down Trinity AI Server...")
    except Exception as e:
        print(f"Error starting server: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()