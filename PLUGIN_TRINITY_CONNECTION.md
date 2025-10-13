# Phoenix Plugin ↔ TRUE Trinity Server Connection

## Connection Configuration

### Server Side (Trinity Pipeline)
- **URL**: `http://localhost:8000`
- **Main file**: `/AI_Server/main.py` (TRUE Trinity without Oracle)
- **Endpoints**:
  - `POST /generate` - Generate preset from prompt
  - `GET /health` - Check server health
  - `GET /` - Server info

### Plugin Side (JUCE)
- **Client**: `AIServerClient.cpp/h`
- **Configuration in Plugin**:
  ```cpp
  // In PluginEditorNexusStatic.cpp:474
  config.httpEndpoint = "http://localhost:8000";
  
  // In PluginEditor.cpp:453
  juce::URL url("http://localhost:8000/generate");
  
  // In PluginProcessor.cpp:878
  juce::URL healthCheck("http://localhost:8000/health");
  ```

## Data Flow

### 1. Plugin Sends Request
```json
{
  "prompt": "warm vintage guitar tone",
  "intensity": 0.5,
  "complexity": 3
}
```

### 2. Trinity Pipeline Processes
```
VISIONARY (AI) → CALCULATOR (AI) → ALCHEMIST (Local)
```

### 3. Server Returns Preset
```json
{
  "success": true,
  "preset": {
    "name": "Vintage Warmth",
    "slot1_engine": 15,
    "slot1_param0": 0.5,
    "slot1_param1": 0.3,
    // ... all parameters
    "slot6_engine": 0
  },
  "metadata": {
    "pipeline_version": "4.0-TRUE",
    "no_oracle": true,
    "no_corpus": true
  }
}
```

### 4. Plugin Applies Preset
- Parses slot configuration
- Sets engine IDs (0-56)
- Applies parameters (0.0-1.0)
- Updates UI

## How to Test Connection

### Step 1: Start Trinity Server
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server

# Set OpenAI key
export OPENAI_API_KEY='your-key-here'

# Start server
./start_trinity.sh
# or
python main.py
```

### Step 2: Verify Server Running
```bash
# Check health
curl http://localhost:8000/health

# Should return:
{
  "status": "healthy",
  "components": {
    "visionary": "ready",
    "calculator": "ready", 
    "alchemist": "ready",
    "oracle": "removed",    # <-- Should say removed!
    "corpus": "not_needed"  # <-- Not needed!
  }
}
```

### Step 3: Test Generation
```bash
python test_plugin_connection.py
```

### Step 4: Run Plugin
1. Build/run the Phoenix plugin in your DAW
2. Click Trinity text box or preset generation button
3. Enter a prompt
4. Should generate preset via TRUE Trinity

## Important Plugin Files

### Primary Connection Points
- `AIServerClient.cpp/h` - HTTP client for server communication
- `TrinityManager.cpp/h` - High-level Trinity integration
- `TrinityNetworkClient.cpp/h` - Network layer (if using WebSocket)

### UI Integration
- `PluginEditor.cpp` - Main editor with Trinity integration
- `PluginEditorNexusStatic.cpp` - Static UI with Trinity
- `TrinityTextBox.cpp/h` - Trinity input UI component

## Troubleshooting

### Server Won't Start
```bash
# Check Python version (needs 3.7+)
python3 --version

# Install dependencies
pip install fastapi uvicorn openai

# Check port 8000 is free
lsof -i :8000
```

### Plugin Can't Connect
1. Verify server is running: `curl http://localhost:8000/health`
2. Check firewall isn't blocking port 8000
3. Ensure plugin is built with latest code
4. Check AIServerClient is using correct URL

### Generation Fails
1. Check OPENAI_API_KEY is set
2. Verify API key is valid
3. Check server logs for errors
4. Test with simple prompt first

## Key Differences: TRUE Trinity vs Old

### OLD Trinity (with Oracle)
```
Prompt → Visionary → Oracle (corpus search) → Calculator → Alchemist
```
- Required Golden Corpus files
- Used FAISS for preset matching
- Depended on preset database

### TRUE Trinity (current)
```
Prompt → Visionary (AI) → Calculator (AI) → Alchemist (Local)
```
- NO corpus files needed
- NO FAISS indexing
- Pure AI generation
- Faster and more creative

## Verification Commands

### Check Server is TRUE Trinity
```bash
# Should NOT have Oracle imports
grep -c "from oracle" /AI_Server/main.py
# Should return 0

# Should show removed Oracle
curl http://localhost:8000/health | grep oracle
# Should show: "oracle": "removed"
```

### Check Plugin Connection
```bash
# Run test script
python /AI_Server/test_plugin_connection.py

# Monitor server logs
tail -f server.log  # If logging to file
```

## Server Startup Sequence

1. `./start_trinity.sh` runs
2. Loads TRUE Trinity components:
   - `visionary_trinity.py` - AI generation
   - `calculator_trinity.py` - Optimization
   - `alchemist_trinity.py` - Safety
3. FastAPI server starts on port 8000
4. Ready to accept plugin connections

## Expected Performance

- **Generation time**: 2-5 seconds typical
- **Timeout**: 30 seconds max
- **Concurrent requests**: Supported
- **Cache**: No corpus cache (pure generation)

---

**This configuration ensures the Phoenix plugin connects to the TRUE Trinity pipeline without any Oracle/corpus dependencies.**