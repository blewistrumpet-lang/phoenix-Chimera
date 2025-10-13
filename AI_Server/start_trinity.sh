#!/bin/bash

echo "=================================================="
echo "🎭 STARTING TRUE TRINITY PIPELINE v4.0"
echo "=================================================="
echo ""
echo "Pipeline Architecture:"
echo "  1. VISIONARY - AI Creative Generation (OpenAI)"
echo "  2. CALCULATOR - AI Musical Intelligence"  
echo "  3. ALCHEMIST - Local Safety Validation"
echo ""
echo "NOT USING:"
echo "  ❌ Oracle (removed)"
echo "  ❌ Golden Corpus (removed)"
echo "  ❌ FAISS indexing (removed)"
echo "  ❌ Preset matching (removed)"
echo ""
echo "=================================================="

# Check for OpenAI API key
if [ -z "$OPENAI_API_KEY" ]; then
    echo "⚠️  WARNING: OPENAI_API_KEY not set!"
    echo "   Export it with: export OPENAI_API_KEY='your-key'"
    echo ""
fi

# Start the server
echo "Starting server on http://localhost:8000"
echo "API docs at http://localhost:8000/docs"
echo ""

cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server
python3 main.py