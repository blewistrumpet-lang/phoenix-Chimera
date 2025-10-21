# GPIO Hardware Quick Reference
## ChimeraPhoenix Pi

### Pin Connections (Physical Board)

```
┌─────────────────────────────────┐
│         RASPBERRY PI GPIO        │
├─────────────────────────────────┤
│                                  │
│  ENCODER 1:                      │
│    A (CLK) ───── GPIO 5  (Pin 29)│
│    B (DT)  ───── GPIO 6  (Pin 31)│
│    BTN     ───── GPIO 26 (Pin 37)│
│    GND     ───── GND              │
│    VCC     ───── 3.3V             │
│                                  │
│  ENCODER 2:                      │
│    A (CLK) ───── GPIO 23 (Pin 16)│
│    B (DT)  ───── GPIO 24 (Pin 18)│
│    BTN     ───── GPIO 25 (Pin 22)│
│    GND     ───── GND              │
│    VCC     ───── 3.3V             │
│                                  │
│  ENCODER 3:                      │
│    A (CLK) ───── GPIO 17 (Pin 11)│
│    B (DT)  ───── GPIO 27 (Pin 13)│
│    BTN     ───── GPIO 22 (Pin 15)│
│    GND     ───── GND              │
│    VCC     ───── 3.3V             │
│                                  │
│  SWITCH 1:                       │
│    PIN1    ───── GPIO 19 (Pin 35)│
│    PIN2    ───── GPIO 21 (Pin 40)│
│    COM     ───── GND              │
│                                  │
│  SWITCH 2:                       │
│    PIN1    ───── GPIO 16 (Pin 36)│
│    PIN2    ───── GPIO 20 (Pin 38)│
│    COM     ───── GND              │
│                                  │
│  SWITCH 3:                       │
│    PIN1    ───── GPIO 12 (Pin 32)│
│    PIN2    ───── GPIO 13 (Pin 33)│
│    COM     ───── GND              │
│                                  │
└─────────────────────────────────┘
```

### Switch Position Logic

| PIN1 | PIN2 | Position |
|------|------|----------|
| LOW  | LOW  | UP       |
| LOW  | HIGH | MIDDLE   |
| HIGH | LOW  | DOWN     |

### Quick Commands

```bash
# Test hardware
cd /home/branden/phoenix-Chimera/pi_deployment
python3 test_encoder.py

# Build plugin
cd JUCE_Plugin/Builds/LinuxMakefile
make -j4

# Run plugin
./build/ChimeraPhoenix

# Watch hardware events
tail -f /tmp/chimera_dev.log | grep -E "ENC|SW"

# Kill plugin
pkill -f ChimeraPhoenix
```

### UI Positions (800x480)

```
TOP (Encoders):
  [150,10]  [350,10]  [550,10]

BOTTOM (Switches):
  [160,410] [360,410] [560,410]
```

### Color Coding
- **Encoders**: Cyan border, white text, red button when pressed
- **Switches**:
  - UP = Green
  - MID = Yellow
  - DOWN = Red

### Files to Edit

**Add hardware feature:**
- `/Source/HardwareController.cpp` - GPIO logic
- `/Source/HardwareDisplayComponents.h` - UI appearance
- `/Source/PluginEditor_Pi.cpp` - Integration & callbacks

**Change pin mapping:**
- `/Source/HardwareController.cpp` - Update pin numbers

**Adjust UI position:**
- `/Source/PluginEditor_Pi.cpp` - resized() method

### Build Flags
- `-DCHIMERA_PI=1` - Enable Pi features
- `-DENABLE_GPIO_HARDWARE=1` - Enable GPIO

### Dependencies
- libgpiod-dev
- libjack-jackd2-dev
- JUCE 8.0.4