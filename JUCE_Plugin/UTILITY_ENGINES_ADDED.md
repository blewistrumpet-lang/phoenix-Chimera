# Utility Engines Successfully Added to Chimera Phoenix v3.0

## Summary
Successfully integrated 3 utility engines into the main engine factory, bringing the total active engine count to 59.

## Engines Added

### 1. GainUtility_Platinum (ID: 56)
- **Purpose**: Professional gain control with precision metering
- **Features**:
  - Peak, RMS, and LUFS metering
  - True peak detection with 4x oversampling
  - Stereo/Mid-Side gain adjustment
  - A/B state management
  - Phase inversion and channel swap

### 2. MonoMaker_Platinum (ID: 57)
- **Purpose**: Frequency-selective mono conversion for bass management
- **Features**:
  - Three processing modes (Standard, Elliptical, Mid/Side)
  - Butterworth filters with 6-48 dB/oct slopes
  - Phase correlation monitoring
  - DC blocking
  - Stereo width control above cutoff

### 3. PhaseAlign_Platinum (ID: 58)
- **Purpose**: Multi-band phase alignment tool
- **Features**:
  - 4-band processing with Linkwitz-Riley crossovers
  - All-pass filter based phase rotation
  - Real-time correlation analysis
  - Auto-alignment capability
  - Band solo/mute for setup

## Integration Details

1. **EngineTypes.h**:
   - Added ENGINE_GAIN_UTILITY (56)
   - Added ENGINE_MONO_MAKER (57)
   - Added ENGINE_PHASE_ALIGN (58)
   - Updated ENGINE_COUNT to 59
   - Added to UTILITY category

2. **EngineFactory.cpp**:
   - Added includes for all 3 engines
   - Added case statements in createEngine()
   - Fully integrated into factory system

3. **Total Engine Count**:
   - **59 engines** total (IDs 0-58)
   - **56 active factory engines** (3 commented out: 10, 13, 37)
   - **4 utility-focused engines** (including BypassEngine at -1)

## About BypassEngine
- BypassEngine (ENGINE_BYPASS = -1) remains in the system
- Serves as a null processor for bypassed slots
- Different from parameter-based bypass (provides actual passthrough processing)

## Usage
All three utility engines are now available for selection in any of the 6 slots in the effects chain, providing essential mixing and mastering tools alongside the creative effects processors.