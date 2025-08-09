# Chimera Phoenix Changelog

## [3.1.0] - December 9, 2024

### Major Architecture Upgrade - Studio-Grade DSP Infrastructure

#### Added
- **DspEngineUtilities Framework**: Centralized DSP safety and utility system
  - Hardware-accelerated denormal protection (FTZ/DAZ)
  - Universal NaN/Inf scrubbing
  - Thread-safe parameter smoothing
  - Lock-free atomic parameters
  - Circular buffers for delays
  - DC blocking utilities
  - Level metering systems

- **Transport Synchronization**: Professional DAW integration
  - All delay effects now support tempo sync
  - 9 beat divisions (1/64 to 4 bars)
  - BPM range: 20-999
  - Backward compatible (sync off by default)
  - Engines updated: TapeEcho, DigitalDelay, MagneticDrumEcho, BucketBrigadeDelay

- **Latency Reporting**: Accurate plugin delay compensation
  - MasteringLimiter_Platinum reports lookahead latency
  - SpectralGate_Platinum reports FFT processing latency
  - All FFT engines report accurate latency
  - Framework ready for all engines

- **Reference Implementation**: StereoChorus_Reference
  - Demonstrates all best practices
  - PIMPL pattern for ABI stability
  - Complete parameter smoothing
  - Transport sync integration
  - Bypass ramping

- **Comprehensive Documentation**
  - DSP_ARCHITECTURE_GUIDELINES.md for team standards
  - PHASE_3_MIGRATION_LOG.md for migration details
  - Complete API documentation

#### Changed
- **All 57 Engines Updated**: Complete DSP safety overhaul
  - Every engine now uses DenormalGuard RAII protection
  - Every engine has scrubBuffer() for NaN/Inf cleanup
  - Standardized parameter smoothing
  - Feedback parameters clamped to safe ranges (-0.95 to 0.95)

- **Performance Improvements**
  - Eliminated CPU spikes from denormal calculations
  - Hardware-accelerated denormal handling
  - Reduced overhead from redundant safety code
  - Optimized buffer operations

- **Code Quality Improvements**
  - Removed ~500 lines of redundant safety code
  - Consolidated duplicate denormal implementations
  - Standardized architecture patterns
  - Single source of truth for DSP utilities

#### Fixed
- **Critical Stability Issues**
  - LadderFilter: Recursive filter now properly protected
  - FeedbackNetwork: Multiple feedback loops stabilized
  - CombResonator: Resonant feedback bounded
  - All FFT engines: Protected against denormal propagation
  - ConvolutionReverb: IR processing secured

- **NaN/Inf Propagation**: Eliminated across entire signal chain
- **Denormal Performance**: No more CPU spikes on quiet signals
- **Feedback Instabilities**: All recursive structures bounded
- **Thread Safety**: All parameter updates now lock-free

#### Technical Details

**Engines by Category Updated:**
- 6 FFT/Convolution engines
- 8 Filter engines
- 8 Modulation engines
- 15 Distortion/Saturation engines
- 8 Dynamics processors
- 5 Reverb engines
- 4 Delay engines (with sync)
- 8 Utility engines
- Special/Advanced engines

**Testing Results:**
- 100% engine pass rate (57/57)
- No NaN/Inf detected
- No timeouts or hangs
- All existing presets compatible
- DAW transport sync verified

### Migration Notes

#### For Developers
- Include `DspEngineUtilities.h` in new engines
- Use `DenormalGuard` at start of `process()`
- Use `scrubBuffer()` at end of `process()`
- Use `DSPUtils::flushDenorm()` for scalars
- Follow DSP_ARCHITECTURE_GUIDELINES.md

#### For Users
- All existing presets remain compatible
- Transport sync is optional (off by default)
- Performance improvements on all systems
- More stable at extreme settings

## [3.0.0] - Previous Release

### Initial Phoenix Architecture
- 57 DSP engines
- JUCE 8.0.8 framework
- Basic parameter system
- Initial stability fixes

---

*Chimera Phoenix is a professional audio plugin featuring 57 studio-grade DSP engines for comprehensive audio processing.*