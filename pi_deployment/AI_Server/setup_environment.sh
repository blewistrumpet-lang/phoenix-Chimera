#!/bin/bash

# Setup script for Trinity AI Server environment

echo "Setting up Trinity AI Server environment..."
echo "=========================================="

# Check if we're in a virtual environment
if [ -z "$VIRTUAL_ENV" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
    source venv/bin/activate
else
    echo "Already in virtual environment"
fi

# Install dependencies
echo "Installing Python dependencies..."
pip install -r requirements.txt

# Check for API key
if [ -z "$OPENAI_API_KEY" ]; then
    echo ""
    echo "WARNING: OPENAI_API_KEY not set!"
    echo "The AI will fall back to simulation mode."
    echo "To use OpenAI, set your API key:"
    echo "  export OPENAI_API_KEY='your-key-here'"
fi

# Check FAISS index
if [ ! -f "../JUCE_Plugin/GoldenCorpus/faiss_index/corpus.index" ]; then
    echo ""
    echo "WARNING: FAISS index not found!"
    echo "Run the following to generate it:"
    echo "  cd ../JUCE_Plugin/Source"
    echo "  ./export_golden_corpus.sh"
fi

echo ""
echo "Setup complete! The server can be started with:"
echo "  python main.py"
echo ""
echo "Or for development with auto-reload:"
echo "  uvicorn main:app --reload --host 0.0.0.0 --port 8000"