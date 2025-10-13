# Phoenix Plugin Auto-Start TRUE Trinity Server

## Implementation Complete ✅

The Phoenix plugin now automatically starts the TRUE Trinity AI server when the plugin loads, ensuring seamless AI integration without manual server management.

## What Was Implemented

### 1. AIServerManager (New)
- **Location**: `JUCE_Plugin/Source/AIServerManager.cpp/h`
- **Purpose**: Singleton that manages server lifecycle
- **Features**:
  - Auto-starts server on plugin load
  - Health monitoring every 5 seconds
  - Auto-restart if server dies
  - Kills zombie processes on port 8000
  - Verifies it's TRUE Trinity (checks oracle="removed")

### 2. PluginProcessor Update
```cpp
// In constructor:
AIServerManager::getInstance().startServerIfNeeded();
```
- Server starts automatically when plugin loads
- No user intervention required

## How It Works

### Startup Sequence
1. Plugin loads in DAW
2. PluginProcessor constructor runs
3. AIServerManager checks if server is running
4. If not running:
   - Kills any processes on port 8000
   - Starts `python3 -m uvicorn main:app`
   - Waits for health check
   - Verifies it's TRUE Trinity
5. Plugin is ready with AI

### Server Location Detection
The manager searches for the server in order:
1. Relative to plugin binary
2. `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server`
3. Other common paths

### Health Monitoring
- Checks `/health` endpoint every 5 seconds
- Verifies response contains:
  - `oracle: "removed"`
  - `corpus: "not_needed"`
- Auto-restarts if unhealthy

## User Experience

### Before (Manual)
1. Open terminal
2. Navigate to AI_Server
3. Run `python main.py`
4. Open DAW
5. Load plugin
6. Hope server is still running

### After (Automatic)
1. Open DAW
2. Load plugin
3. ✨ Everything works

## Configuration

### Environment Variables
```bash
# Optional: Set OpenAI key globally
export OPENAI_API_KEY='your-key'
```

### Server Settings
- **Port**: 8000 (hardcoded)
- **Host**: localhost/0.0.0.0
- **Timeout**: 15 seconds for startup
- **Health check**: Every 5 seconds

## Troubleshooting

### Server Won't Start
```bash
# Check Python is installed
which python3

# Check port 8000 is free
lsof -i :8000

# Check dependencies
pip install fastapi uvicorn openai
```

### Manual Override
If you prefer manual control:
```bash
# Start server manually
cd AI_Server
./start_trinity.sh

# Plugin will detect existing server
```

### Debug Output
Watch Console.app (Mac) or enable debug logging:
```
AIServerManager: Starting TRUE Trinity server...
AIServerManager: Server process started, waiting...
AIServerManager: TRUE Trinity server is ready!
✅ TRUE Trinity server is running and healthy!
```

## Benefits

1. **Zero Configuration** - Works out of the box
2. **Automatic Recovery** - Restarts if crashed
3. **Resource Management** - Cleans up on exit
4. **Process Safety** - Kills zombies
5. **TRUE Trinity Verification** - Ensures correct server

## Technical Details

### Process Management
- Uses JUCE ChildProcess class
- Graceful shutdown on plugin unload
- Kills server when DAW closes

### Port Conflict Resolution
```cpp
// Automatically kills existing process on port 8000
killExistingServers();
```

### Server Discovery
```cpp
// Searches multiple paths for main.py
possiblePaths = {
    pluginBinary/../../../AI_Server,
    /Users/Branden/.../AI_Server
}
```

## Files Modified

1. **New Files**:
   - `AIServerManager.h` - Manager interface
   - `AIServerManager.cpp` - Implementation

2. **Updated Files**:
   - `PluginProcessor.cpp` - Added auto-start

3. **Server Files**:
   - `main.py` - TRUE Trinity server (no changes needed)
   - `start_trinity.sh` - Still works for manual start

## Testing

### Verify Auto-Start
1. Close DAW completely
2. Kill any Python processes
3. Open DAW
4. Load Phoenix plugin
5. Check server started:
```bash
ps aux | grep uvicorn
curl http://localhost:8000/health
```

### Verify TRUE Trinity
```bash
# Should show oracle removed
curl http://localhost:8000/health | python -m json.tool
```

Response should contain:
```json
{
  "components": {
    "oracle": "removed",
    "corpus": "not_needed"
  }
}
```

## Next Steps

The plugin now has seamless AI integration with auto-start. Users can:
1. Load plugin
2. Enter prompts
3. Generate presets
4. No server management needed!

---

**The Phoenix plugin now provides a truly plug-and-play AI experience with the TRUE Trinity pipeline.**