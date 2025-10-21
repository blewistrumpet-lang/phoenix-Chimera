# ChimeraPhoenix Pi Hardware Integration
**Date**: October 20, 2025
**Status**: Successfully Integrated and Tested

## Overview
Successfully integrated GPIO-based hardware controls (3 rotary encoders and 3 three-way switches) into the ChimeraPhoenix Pi audio plugin UI. The hardware controls are now visible in the UI and respond to physical input.

## Hardware Components

### Rotary Encoders (3x)
- **Type**: Incremental rotary encoders with push buttons
- **Features**:
  - Quadrature encoding for rotation detection
  - Integrated push button
  - Position tracking with CW/CCW detection

### Three-Way Switches (3x)
- **Type**: SPDT ON-OFF-ON toggle switches
- **Positions**: UP / MIDDLE / DOWN
- **Logic**: Two GPIO pins per switch to decode 3 positions

## GPIO Pin Mappings

### Final Pin Assignments (After Testing and Corrections)
```
ENCODER 1: A=5,  B=6,  BTN=26
ENCODER 2: A=23, B=24, BTN=25
ENCODER 3: A=17, B=27, BTN=22

SWITCH 1: PIN1=19, PIN2=21
SWITCH 2: PIN1=16, PIN2=20
SWITCH 3: PIN1=12, PIN2=13
```

### Important Notes
- Encoders 1 and 3 were physically swapped from initial wiring
- All encoders required direction inversion (CW ↔ CCW)
- All switches required position inversion (UP ↔ DOWN)

## Software Architecture

### Core Components

1. **HardwareController.h/cpp**
   - Thread-safe GPIO controller using libgpiod
   - Separate polling thread (1ms intervals)
   - Atomic state variables for lock-free operation
   - Callback system for event handling
   - Quadrature decoding with debouncing

2. **HardwareDisplayComponents.h**
   - `EncoderDisplay`: Visual component showing position + button state
   - `SwitchDisplay`: Visual component showing UP/MID/DOWN positions
   - Dark theme with colored accents
   - Real-time updates via timer callback

3. **PluginEditor_Pi Integration**
   - Conditional compilation with `ENABLE_GPIO_HARDWARE` flag
   - Hardware initialization in constructor
   - Positioning in `resized()` method
   - State updates in `timerCallback()` at 30Hz

### Build Configuration
- Added to Makefile:
  - `-DCHIMERA_PI=1` compiler flag
  - HardwareController compilation rule
  - `-lgpiod` library linking
  - `-ljack` library linking

## UI Layout

### Display Positioning (800x480 window)
```
ENCODERS (Top):
- Encoder 1: X=150, Y=10, W=100, H=80
- Encoder 2: X=350, Y=10, W=100, H=80
- Encoder 3: X=550, Y=10, W=100, H=80

SWITCHES (Bottom):
- Switch 1: X=160, Y=410, W=80, H=60
- Switch 2: X=360, Y=410, W=80, H=60
- Switch 3: X=560, Y=410, W=80, H=60
```

## Development Environment

### System Requirements
- Raspberry Pi with GPIO access (tested on Pi 5)
- libgpiod-dev package installed
- JACK audio server (dummy driver OK for development)
- JUCE framework v8.0.4

### Building
```bash
cd /home/branden/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile
make clean
make -j4
```

### Running
```bash
./build/ChimeraPhoenix
# or use the launch script:
/home/branden/launch_chimera_dev.sh
```

## Testing Results

### Working Features
✅ GPIO hardware detection and initialization
✅ Encoder rotation tracking (CW/CCW)
✅ Encoder button press detection
✅ Switch position detection (UP/MID/DOWN)
✅ UI displays updating in real-time
✅ Thread-safe operation with audio processing
✅ Correct positioning in UI (top/bottom)

### Console Output Examples
```
Initializing GPIO hardware...
✓ GPIO hardware initialized successfully
ENC1: pos=0 CW
ENC2: pos=-1 CCW
SW3: DOWN
SW1: UP
ENC3 BUTTON PRESSED
```

## Known Issues and Solutions

### Issue 1: UI Components at Wrong Position
**Problem**: All hardware displays appeared at (0,0)
**Solution**:
- Use full window bounds instead of reduced bounds
- Call `resized()` after hardware initialization
- Add `toFront()` to ensure visibility

### Issue 2: Compilation Errors
**Problem**: Missing gpiod.h, undefined JACK references
**Solution**:
- Install libgpiod-dev
- Add -ljack to linker flags
- Sync source files from HiFiBerry Pi

## Future Development

### Planned Functionality Mapping
1. **Encoder 1**: Slot selection (1-6)
2. **Encoder 2**: Parameter selection within slot
3. **Encoder 3**: Parameter value adjustment
4. **Switch 1**: Slot bypass toggle
5. **Switch 2**: Preset navigation
6. **Switch 3**: Special functions/modes

### Next Steps
- [ ] Implement parameter mapping to hardware controls
- [ ] Add visual feedback for active slot/parameter
- [ ] Create preset navigation via hardware
- [ ] Add hold-button functions for encoders
- [ ] Implement switch combination shortcuts

## Files Created/Modified

### New Files
- `/Source/HardwareController.h`
- `/Source/HardwareController.cpp`
- `/Source/HardwareDisplayComponents.h`

### Modified Files
- `/Source/PluginEditor_Pi.h` - Added hardware members
- `/Source/PluginEditor_Pi.cpp` - Added initialization and positioning
- `/Builds/LinuxMakefile/Makefile` - Added compilation and linking

## Repository Structure
```
/home/branden/phoenix-Chimera/pi_deployment/
├── JUCE_Plugin/
│   ├── Source/
│   │   ├── HardwareController.h       [NEW]
│   │   ├── HardwareController.cpp     [NEW]
│   │   ├── HardwareDisplayComponents.h [NEW]
│   │   ├── PluginEditor_Pi.h          [MODIFIED]
│   │   └── PluginEditor_Pi.cpp        [MODIFIED]
│   └── Builds/LinuxMakefile/
│       ├── Makefile                    [MODIFIED]
│       └── build/
│           └── ChimeraPhoenix          [114MB binary]
└── test_encoder.py                     [Test script]
```

## Debug Commands

### Check Hardware Events
```bash
# Watch for hardware events in real-time
tail -f /tmp/chimera_dev.log | grep -E "ENC|SW"
```

### Test Individual Hardware
```bash
# Run standalone test script
cd /home/branden/phoenix-Chimera/pi_deployment
python3 test_encoder.py
```

## Credits
Hardware integration developed on October 20, 2025 for the ChimeraPhoenix Pi project.
Integration includes GPIO control via libgpiod, JUCE UI components, and real-time audio-safe threading.