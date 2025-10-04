# Chimera Phoenix AI Server - Trinity Pipeline v3.0
## Phoenix Reboot - Production-Ready Implementation

### Overview
This is the refactored and production-ready implementation of the Chimera Phoenix AI Server, featuring the complete Trinity AI Pipeline for intelligent audio preset generation.

### Architecture
The Trinity Pipeline orchestrates four specialized components in sequence:

```
User Prompt → [VISIONARY] → [ORACLE] → [CALCULATOR] → [ALCHEMIST] → Final Preset
```

#### 1. VISIONARY (Creative Blueprint Generation)
- **TCP Client Architecture**: Connects to bridge server for AI services
- **Fallback Simulation**: Intelligent fallback when TCP unavailable
- **Blueprint Format**: 6 slots with engine assignments and creative analysis

#### 2. ORACLE (Corpus Search & Matching)
- **FAISS Vector Search**: Ultra-fast similarity matching
- **250 Preset Corpus**: Rich golden corpus with metadata
- **Re-ranking Algorithm**: Score boosting based on engine selections
- **Engine Mapping**: Compatible with JUCE plugin parameter system

#### 3. CALCULATOR (Sophisticated Nudging)
- **Multi-layered System**: 
  - Creative analysis nudges
  - Contextual keyword nudges
  - Engine-specific intelligent nudges
  - Harmonic balancing
- **Parameter Roles**: Understanding of parameter functions
- **Weighted Rules**: Context modifiers with weights

#### 4. ALCHEMIST (Final Synthesis & Safety)
- **Parameter Validation**: Engine-specific limits and safety checks
- **Creative Naming**: Intelligent preset name generation
- **Safety Limits**: Prevents feedback runaway and clipping
- **Warnings System**: Identifies potential issues

### Installation

#### Prerequisites
- Python 3.8 or higher
- pip package manager
- Optional: CUDA-capable GPU for faiss-gpu

#### Setup
```bash
# Clone or extract the repository
cd AI_Server

# Create virtual environment
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Optional: Install GPU support
pip install faiss-gpu --extra-index-url https://pypi.nvidia.com
```

### Configuration

#### Environment Variables
Create a `.env` file for optional OpenAI integration:
```
OPENAI_API_KEY=your_api_key_here
```

#### Data Files
Ensure these files are present:
- `nudge_rules.json` - Sophisticated nudging rules
- `parameter_manifest.json` - Validation limits
- `engine_defaults.py` - Engine parameter definitions
- `engine_mapping.py` - ID conversion utilities

### Running the Server

#### Option 1: Main FastAPI Server
```bash
python main.py
```
Server will start on http://localhost:8000

#### Option 2: With TCP Bridge (for OpenAI)
```bash
# Terminal 1: Start TCP Bridge
python tcp_bridge_server.py

# Terminal 2: Start Main Server
python main.py
```

#### Option 3: Production Deployment
```bash
uvicorn main:app --host 0.0.0.0 --port 8000 --workers 4
```

### API Usage

#### Generate Preset
```bash
curl -X POST "http://localhost:8000/generate" \
  -H "Content-Type: application/json" \
  -d '{
    "prompt": "Create a warm vintage guitar tone with tube saturation and spring reverb",
    "max_generation_time": 30
  }'
```

#### Health Check
```bash
curl "http://localhost:8000/health"
```

#### API Documentation
- Swagger UI: http://localhost:8000/docs
- ReDoc: http://localhost:8000/redoc

### File Structure
```
AI_Server/
├── main.py                    # Main FastAPI server
├── visionary_client.py        # TCP client for Visionary
├── tcp_bridge_server.py       # TCP bridge to AI services
├── oracle_faiss.py           # FAISS-powered preset matching
├── calculator.py             # Sophisticated nudge system
├── alchemist.py              # Final validation & safety
├── engine_mapping.py         # Engine ID conversion
├── engine_defaults.py        # Engine parameter definitions
├── nudge_rules.json          # Sophisticated nudge rules
├── parameter_manifest.json   # Validation limits
├── requirements.txt          # Python dependencies
├── GAP_ANALYSIS_REPORT.md   # Implementation audit
└── README_PHOENIX.md        # This file
```

### Testing

#### Unit Tests
```bash
pytest tests/
```

#### Integration Test
```python
# test_pipeline.py
import asyncio
import httpx

async def test_pipeline():
    async with httpx.AsyncClient() as client:
        response = await client.post(
            "http://localhost:8000/generate",
            json={"prompt": "aggressive metal tone"}
        )
        assert response.status_code == 200
        preset = response.json()["preset"]
        assert "parameters" in preset
        print(f"Generated: {preset['name']}")

asyncio.run(test_pipeline())
```

### Monitoring

The server provides detailed logging for each pipeline stage:
```
═══ Starting Trinity Pipeline ═══
│ Step 1: VISIONARY - Generating creative blueprint
│ ✓ Blueprint generated: vintage warmth
│ Step 2: ORACLE - Finding best match from corpus
│ ✓ Matched preset: Warm Vintage Drive
│ Step 3: CALCULATOR - Applying intelligent nudges
│ ✓ Applied 12 nudges to 8 parameters
│ Step 4: ALCHEMIST - Final validation and safety
│ ✓ Final preset: 'Golden Tube Dreams'
═══ Pipeline Complete (2.34s) ═══
```

### Performance

- **Visionary**: 0.5-2s (with fallback)
- **Oracle**: <100ms (FAISS vector search)
- **Calculator**: <50ms (rule application)
- **Alchemist**: <50ms (validation)
- **Total Pipeline**: 1-3s typical

### Troubleshooting

#### FAISS Index Missing
```bash
# Regenerate FAISS index
python scripts/build_faiss_index.py
```

#### TCP Connection Refused
- Ensure TCP bridge is running on port 9999
- Check firewall settings
- Fallback simulation will activate automatically

#### High Memory Usage
- Use `faiss-cpu` instead of `faiss-gpu`
- Reduce corpus size in oracle_faiss.py
- Adjust worker count in production

### Development

#### Adding New Engines
1. Update `engine_defaults.py` with parameter definitions
2. Add to `engine_mapping.py` for ID conversion
3. Create character responses in `nudge_rules.json`
4. Update parameter manifest for validation

#### Extending Calculator
1. Add new context modifiers in `nudge_rules.json`
2. Implement new nudge methods in `calculator.py`
3. Update parameter roles for intelligent mapping

### Security Notes

- Never commit API keys to repository
- Use environment variables for secrets
- Validate all user inputs
- Apply rate limiting in production
- Use HTTPS in production deployments

### License
Proprietary - Chimera Phoenix Project

### Support
For issues or questions, consult the GAP_ANALYSIS_REPORT.md for implementation details.

---
**Phoenix Reboot v3.0** - Built with the Trinity Pipeline Architecture