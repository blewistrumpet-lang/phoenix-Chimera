# Trinity System Version Log

## Current Configuration (Oct 7, 2025 - 1:15 PM)

### RASPBERRY PI DEPLOYMENT

#### Server Components:
- **Main Server**: `trinity_server_pi.py` (v1.5 with Whisper + polling)
- **Visionary**: `visionary_complete.py` (OpenAI GPT-4 based)
- **Calculator**: `calculator_max_intelligence.py` (INTELLIGENT with full parameter understanding)
- **Alchemist**: `alchemist_complete.py` (Local preset assembly)
- **Knowledge Base**: `trinity_engine_knowledge_COMPLETE.json` (57 engines)

#### Plugin Build:
- **Executable**: `ChimeraPhoenix` (Debug build)
- **Build Time**: Oct 7, 13:01
- **Build Location**: `~/phoenix-Chimera/JUCE_Plugin/Builds/LinuxMakefile/build/`
- **Key Features**:
  - USB Microphone support (hw:2,0)
  - Voice recording via Hold to Speak button
  - Trinity polling support
  - Debug logging enabled

#### Required Endpoints:
- `/whisper` - Voice transcription (POST)
- `/generate` - Preset generation (POST)
- `/poll` - Status polling (GET)
- `/health` - Health check (GET)

### MAC DEVELOPMENT

#### Server Components:
- **Main Server**: `main.py` or `main_trinity.py`
- **Calculator**: `calculator_trinity_ai.py` (for text prompts)
- **Same Visionary/Alchemist as Pi**

### CRITICAL NOTES:
1. **NEVER use `main.py` on Pi** - it lacks whisper and polling endpoints
2. **Always use `trinity_server_pi.py` for Pi deployment**
3. **Calculator MUST be `calculator_max_intelligence.py` for proper parameter handling**
4. **OpenAI API key required in environment**

### Common Mistakes to Avoid:
- ❌ Using wrong server file
- ❌ Missing whisper endpoint for voice
- ❌ Using non-intelligent calculator
- ❌ Not having polling endpoint
- ❌ Wrong knowledge base file

### Verification Commands:
```bash
# Check running server
ps aux | grep python3 | grep trinity

# Verify endpoints
curl http://localhost:8000/health

# Check API key
echo $OPENAI_API_KEY
```

Last Updated: Oct 7, 2025 1:15 PM