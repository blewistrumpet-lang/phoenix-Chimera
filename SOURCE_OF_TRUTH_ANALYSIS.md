# DEFINITIVE SOURCE OF TRUTH ANALYSIS

## EXECUTION FLOW TRACE

### 1. ENGINE CREATION
```cpp
// EngineFactory.cpp:189-190
case 39: // ENGINE_PLATE_REVERB
    return std::make_unique<PlateReverb>();
```
**FACT:** When engine ID 39 is requested, a PlateReverb object is created.

### 2. PARAMETER PASSING
```cpp
// PluginProcessor.cpp:669-686
void ChimeraAudioProcessor::updateEngineParameters(int slot) {
    std::map<int, float> params;
    for (int i = 0; i < 15; ++i) {
        params[i] = value;  // Gets value from APVTS param1-param15
    }
    m_activeEngines[slot]->updateParameters(params);
}
```
**FACT:** The processor passes parameters as index->value pairs (0-14).

### 3. ENGINE RECEIVES PARAMETERS
```cpp
// PlateReverb.cpp:413-418
void PlateReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index < 10) {  // PlateReverb only uses first 10
            pImpl->setParameter(index, value);
        }
    }
}
```
**FACT:** PlateReverb uses indices 0-9 for its parameters.

### 4. ENGINE PROCESSING
```cpp
// PlateReverb.cpp internal implementation
// Uses the parameter values at indices 0-9 to control:
// index 0 = Mix
// index 1 = Size  
// index 2 = Damping
// etc.
```

## THE DEFINITIVE SOURCE OF TRUTH

**THE ENGINE IMPLEMENTATION FILES (.cpp) ARE THE SOURCE OF TRUTH**

### PROOF:
1. **The engine's `updateParameters()` method determines what indices it uses**
   - PlateReverb uses indices 0-9
   - NoiseGate uses indices 0-7
   - Each engine decides for itself

2. **The engine's `getParameterName()` method defines the names**
   ```cpp
   // PlateReverb.cpp:421-433
   case 0: return "Mix";
   case 1: return "Size";
   // etc.
   ```

3. **The engine's `getNumParameters()` method defines the count**
   ```cpp
   // PlateReverb.cpp:437-438
   return 10;
   ```

## WHAT THE DATABASE IS FOR

The GeneratedParameterDatabase is ONLY used by:
1. **The UI** - to display parameter names
2. **The UI** - to determine if a parameter should be a toggle vs slider
3. **The UI** - to show the right number of controls

**IT DOES NOT AFFECT:**
- How engines process audio
- What parameters engines actually use
- The actual parameter mapping

## THE PROBLEM

The database MUST match what the engines actually implement, or:
- UI shows wrong parameter names
- UI shows wrong number of controls
- UI might create toggles when sliders are needed

## SYSTEMS THAT NEED TO BE CONSISTENT

1. **EngineTypes.h** - Defines engine IDs (e.g., ENGINE_PLATE_REVERB = 39)
2. **EngineFactory.cpp** - Maps IDs to engine classes (39 -> PlateReverb)
3. **Engine .cpp files** - Define actual parameters used
4. **GeneratedParameterDatabase.h** - Must match engine implementations for UI

## CURRENT STATE

✅ **EngineTypes.h and EngineFactory.cpp agree** (both use 39 for PlateReverb)
✅ **Engine IDs in database now match EngineTypes.h** (we fixed this)
❓ **Parameter counts and names in database match engines?** (need to verify all)

## CONCLUSION

**The actual engine implementation files are the source of truth.**
- They decide what parameters they use
- They decide parameter names
- They decide parameter counts

The database is just metadata for the UI and MUST match the implementations.