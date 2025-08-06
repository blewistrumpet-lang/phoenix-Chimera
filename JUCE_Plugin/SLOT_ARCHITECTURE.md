# Chimera Phoenix v3.0 - Slot Architecture

## Current Configuration: 6-Slot Serial Effects Rack

The system currently uses a **6-slot serial processing chain** where any of the 53 available DSP engines can be loaded into any slot.

### Architecture Overview

```
Input → [Slot 1] → [Slot 2] → [Slot 3] → [Slot 4] → [Slot 5] → [Slot 6] → Output
           ↓          ↓          ↓          ↓          ↓          ↓
      (Any of 53  (Any of 53  (Any of 53  (Any of 53  (Any of 53  (Any of 53
       Engines)    Engines)    Engines)    Engines)    Engines)    Engines)
```

### Slot Configuration

- **Current Slots**: 6 (defined in `SlotConfiguration.h`)
- **Maximum Supported**: 8 (for future expansion)
- **Processing**: Serial (audio flows through each slot sequentially)
- **Bypass**: Each slot can be individually bypassed

### Implementation Details

The slot count is centrally managed in `SlotConfiguration.h`:

```cpp
namespace ChimeraConfig {
    static constexpr int NUM_SLOTS = 6;
    static constexpr int MAX_SLOTS_SUPPORTED = 8;
}
```

### Expanding to 8 Slots

To expand to 8 slots in the future:

1. Change `NUM_SLOTS` in `SlotConfiguration.h` from 6 to 8
2. Update UI layout in `PluginEditor.cpp`
3. Add parameter definitions for slots 7 & 8 in `ParameterDefinitions.cpp`
4. Test CPU performance with 8 active engines

### CPU Considerations

- **6 Slots**: Recommended for most use cases
  - Typical CPU usage: 30-50% with moderate engines
  - Allows headroom for complex engines (reverbs, physical modeling)
  
- **8 Slots**: For advanced users with powerful systems
  - May require CPU usage monitoring
  - Consider implementing dynamic slot limiting based on CPU load

### Why 6 Slots?

1. **Typical Signal Chains**: Most professional mixing chains use 4-6 processors
   - EQ → Compression → Saturation → Modulation → Delay → Reverb
   
2. **Hardware Inspiration**: Matches classic hardware channel strips
   - SSL channels (EQ, Dynamics, Filters)
   - Neve consoles (similar architecture)
   
3. **Performance Balance**: Leaves CPU headroom for:
   - High sample rates (96kHz, 192kHz)
   - Low latency operation
   - Complex engines with oversampling

4. **User Experience**: 
   - Simpler to visualize and manage
   - Reduces decision paralysis
   - Encourages creative constraint

### Multi-Function Engines

Many engines combine multiple effects, reducing the need for more slots:

- **DynamicEQ**: EQ + Compression
- **HarmonicExciter_Platinum**: Saturation + EQ + Enhancement
- **VintageOptoCompressor_Platinum**: Compression + Saturation + Color
- **StateVariableFilter**: Filter + Drive + Envelope

### Future Enhancements

The architecture supports future expansion:

- **Dynamic Slot Count**: Enable/disable slots based on CPU
- **Parallel Processing**: Split signal into parallel chains
- **Sidechain Routing**: Between slots for dynamics processors
- **Mid/Side Processing**: Per-slot M/S encode/decode

## Usage Guidelines

### Recommended Signal Flow

1. **Corrective Processing** (Slots 1-2)
   - EQ (ParametricEQ_Platinum)
   - Noise reduction (NoiseGate_Platinum)

2. **Dynamics** (Slots 3-4)
   - Compression (VintageOptoCompressor_Platinum)
   - Limiting (MasteringLimiter_Platinum)

3. **Character/Color** (Slot 5)
   - Saturation (HarmonicExciter_Platinum)
   - Analog modeling (StateVariableFilter)

4. **Spatial Effects** (Slot 6)
   - Reverb (PlateReverb_Platinum, SpringReverb_Platinum)
   - Delay (AnalogDelay_Platinum, TapeDelay_Platinum)

### CPU Optimization Tips

1. Place CPU-heavy engines (reverbs) at the end
2. Use engine bypass when not needed
3. Choose efficient engines for subtle effects
4. Monitor CPU usage with complex chains
5. Consider freezing/bouncing for dense projects