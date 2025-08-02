# Project Chimera v3.0 "Phoenix"

A revolutionary AI-powered audio effects plugin featuring 50+ boutique DSP engines and intelligent preset generation.

## Architecture Overview

### C++ JUCE Plugin
- **6-Slot Serial Processing**: Chain up to 6 audio engines in series
- **50+ Boutique DSP Engines**: Vintage effects, modulation, filters, distortion, spatial processing
- **Analog Modeling**: Thermal drift, component aging, and circuit-level emulation
- **Professional Quality**: Oversampling, DC blocking, parameter smoothing

### Python AI Server
- **Master FastAPI Server**: Orchestrates the AI pipeline
- **The Trinity Pipeline**:
  - **Visionary**: OpenAI-powered creative blueprint generator
  - **Oracle**: Searches Golden Corpus for best matching preset
  - **Calculator**: Applies intelligent parameter nudges
  - **Alchemist**: Final validation and safety checks

## Project Structure

```
Project_Chimera_v3.0_Phoenix/
├── JUCE_Plugin/
│   ├── Source/
│   │   ├── PluginProcessor.cpp/h
│   │   ├── PluginEditor.cpp/h
│   │   ├── EngineBase.h
│   │   ├── EngineFactory.cpp/h
│   │   └── ParameterDefinitions.h
│   ├── Engines/
│   │   ├── KStyleOverdrive.cpp/h
│   │   ├── TapeEcho.cpp/h
│   │   └── PlateReverb.cpp/h
│   └── ChimeraPhoenix.jucer
├── AI_Server/
│   ├── main.py
│   ├── visionary_client.py
│   ├── oracle.py
│   ├── calculator.py
│   ├── alchemist.py
│   ├── openai_bridge_server.py
│   ├── nudge_rules.json
│   └── requirements.txt
└── Golden_Corpus/
    ├── generate_corpus.py
    └── golden_corpus.json
```

## Building the Plugin

1. Open `ChimeraPhoenix.jucer` in Projucer
2. Configure JUCE module paths
3. Generate and open in your IDE
4. Build the plugin

## Running the AI Server

1. Install Python dependencies:
   ```bash
   cd AI_Server
   pip install -r requirements.txt
   ```

2. (Optional) Start the OpenAI Bridge Server:
   ```bash
   python3 openai_bridge_server.py
   ```

3. Start the main AI server:
   ```bash
   python3 main.py
   ```

The server will run on `http://localhost:8000`

## API Usage

Send a POST request to `/generate` with a prompt:

```json
{
  "prompt": "Create a warm vintage tone with subtle echo"
}
```

Response:
```json
{
  "success": true,
  "preset": {
    "name": "...",
    "parameters": {...}
  }
}
```

## Golden Corpus

The Golden Corpus contains 250 handcrafted presets (30 completed, 220 in progress). Each preset includes:
- Scientific/cultural inspiration  
- Detailed parameter rationale
- Sonic and emotional profiles
- Musical context metadata

To export presets to JSON:
```bash
cd JUCE_Plugin/Tools
python3 export_presets.py
```

To generate FAISS index:
```bash
cd JUCE_Plugin/Python
python3 oracle_faiss_indexer.py
```

## Recent Updates

- ✅ Created unified EngineTypes.h for consistent engine identification
- ✅ Implemented reset() function in all 50+ engines to prevent audio artifacts
- ✅ Exported 30 Golden Corpus presets to JSON format
- ✅ Built FAISS indexing system for Oracle AI component
- 🚧 Working on remaining 220 presets
- 🚧 Developing plugin UI
- 🚧 Integrating Trinity AI pipeline