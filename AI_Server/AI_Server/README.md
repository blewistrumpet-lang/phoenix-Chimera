# Trinity AI Server

**Production-ready AI preset generation system for ChimeraPhoenix**

## Quick Start

### 1. Start the Server

```bash
python3 trinity_server.py
```

Server runs on `http://localhost:8000`

### 2. Generate a Preset

```bash
curl -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt":"warm analog tape saturation"}'
```

### 3. Check Health

```bash
curl http://localhost:8000/health
```

---

## Architecture

Trinity uses a 3-agent system optimized for speed and quality:

```
User Prompt → [Visionary] → [Calculator] → [Alchemist] → Final Preset
                 GPT-4       GPT-3.5-turbo    Rule-based
```

### Components

1. **Visionary** (`visionary_complete.py`)
   - Uses GPT-4 for superior creative intelligence
   - Selects appropriate audio engines based on prompt
   - Generates initial parameter estimates

2. **Calculator** (`calculator_max_intelligence.py`)
   - Uses GPT-3.5-turbo for fast parameter optimization
   - **Optimized**: 1 unified API call (was 3 separate calls)
   - Handles musical style analysis, conflict detection, and creative enhancements

3. **Alchemist** (`alchemist_complete.py`)
   - Pure Python validation (no AI)
   - Ensures parameter ranges and engine compatibility
   - Fixes any structural issues

---

## Performance

**Target**: <20 seconds on Raspberry Pi 5

**Actual**: ~14 seconds (75% improvement from original 57s)

**API Calls per Request**:
- 1x Whisper (voice-to-text, Pi only)
- 1x GPT-4 (Visionary)
- 1x GPT-3.5-turbo (Calculator)
- **Total: 3 API calls** (optimized from 5)

---

## Configuration

### OpenAI API Key

**Option A**: Environment Variable
```bash
export OPENAI_API_KEY="sk-your-key-here"
python3 trinity_server.py
```

**Option B**: Config File (recommended)
```bash
echo "OPENAI_API_KEY=sk-your-key-here" > ~/.chimera_config
chmod 600 ~/.chimera_config
```

### Server Port

Default: `8000`

Change in `trinity_server.py`:
```python
uvicorn.run(app, host="0.0.0.0", port=8000)
```

---

## API Reference

### POST /generate

**Request**:
```json
{
  "prompt": "vintage Beatles Abbey Road drums with compression"
}
```

**Response**:
```json
{
  "preset": {
    "name": "Abbey Road Vintage Drums",
    "slots": [
      {
        "slot": 1,
        "engine_id": 2,
        "engine_name": "VintageOptoCompressor",
        "parameters": [...]
      }
    ]
  },
  "debug": {
    "processing_time_seconds": 14.2,
    "calculator": {
      "type": "intelligent",
      "used_claude": false
    }
  }
}
```

### GET /health

Returns service status and component readiness.

---

## File Structure

```
AI_Server/
├── trinity_server.py              # Main production server
├── calculator_max_intelligence.py # Optimized parameter calculator
├── visionary_complete.py          # Creative engine selection
├── alchemist_complete.py          # Validation and fixes
├── archive/                       # Old/deprecated versions
└── README.md                      # This file
```

---

## Deployment

### Mac (Development)

```bash
python3 trinity_server.py
```

### Raspberry Pi 5 (Standalone)

See `../pi_deployment/PI_VOICE_SETUP.md` for full setup including voice control.

**Quick start**:
```bash
# Install dependencies
pip3 install fastapi uvicorn openai pydantic anthropic

# Run server
export OPENAI_API_KEY="sk-your-key"
python3 trinity_server.py
```

---

## Troubleshooting

### "No OpenAI API key"
- Check `~/.chimera_config` exists
- OR set `export OPENAI_API_KEY=...`
- Restart server

### "Calculator failed"
- Check OpenAI API key is valid
- Verify internet connection
- Check API rate limits

### Slow performance (>20s)
- Expected on slower hardware
- Ensure using wired ethernet (not WiFi) on Pi
- Check network latency to OpenAI API

---

## Development

### Testing Calculator

```bash
python3 calculator_max_intelligence.py
```

### Testing Full Pipeline

```bash
python3 test_trinity_production.py
```

---

## Changelog

### v2.0 (Pi Optimization)
- Reduced Calculator from 3 → 1 GPT API call
- Changed Calculator model: GPT-4 → GPT-3.5-turbo
- Performance: 75% faster (57s → 14s on Pi)
- Renamed `trinity_server_intelligent.py` → `trinity_server.py`

### v1.0 (Original)
- 3-agent architecture
- Claude Code integration
- Intelligent parameter parsing
