# ENGINE ID MISMATCH - ROOT CAUSE ANALYSIS & FIX

## CRITICAL ISSUE DISCOVERED
The GeneratedParameterDatabase.h was using COMPLETELY WRONG engine IDs that didn't match the authoritative EngineTypes.h definitions.

## ROOT CAUSE
1. **Two conflicting ID systems existed:**
   - EngineTypes.h (AUTHORITATIVE) - Used by EngineFactory and actual engine creation
   - GeneratedParameterDatabase.h (INCORRECT) - Used by UI for parameter mapping

2. **The database was generated from parameter_database.json which had wrong "legacy_id" values**

## MAJOR MISMATCHES FOUND

| Engine | EngineTypes.h (CORRECT) | Database Had (WRONG) |
|--------|-------------------------|---------------------|
| Vintage Tube Preamp | 15 | 0 |
| K-Style Overdrive | 22 | 38 |
| Tape Echo | 34 | 1 |
| Shimmer Reverb | 42 | 2 |
| Plate Reverb | 39 | 3 |
| Spring Reverb | 40 | 5 |
| Chaos Generator | 51 | 41 |
| And many more... | | |

## IMPACT
- UI was requesting engines with wrong IDs
- EngineFactory couldn't create the requested engines
- Parameter mappings were completely broken
- Reverbs showed wrong parameter counts

## FIX APPLIED
1. **Updated all engine IDs in GeneratedParameterDatabase.h to match EngineTypes.h**
2. **Fixed reverb parameter counts:**
   - Shimmer Reverb: 4 → 10 parameters
   - Plate Reverb: 4 → 10 parameters  
   - Spring Reverb: 4 → 9 parameters
   - Gated Reverb: 4 → 10 parameters
   - Added Convolution Reverb: 10 parameters

3. **Fixed Noise Gate: 5 → 8 parameters**

## VERIFICATION
- All engine IDs now match EngineTypes.h
- Plugin builds successfully
- Parameter counts match actual implementations

## CRITICAL FINDING
**EngineTypes.h is the SINGLE SOURCE OF TRUTH for engine IDs**
- EngineFactory uses these IDs
- All engine creation depends on these IDs
- The database MUST match these IDs exactly

## RECOMMENDATION
- Regenerate parameter_database.json with correct engine IDs from EngineTypes.h
- Add validation to ensure database IDs always match EngineTypes.h
- Consider removing the "legacy_id" concept and using ENGINE_ constants directly