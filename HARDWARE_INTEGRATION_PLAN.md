# ChimeraPhoenix Hardware Prototype - Integration Plan

## Vision
Transform ChimeraPhoenix from software plugin to standalone hardware unit powered by Raspberry Pi 5, featuring voice-controlled Trinity AI preset generation and tactile parameter control.

## Hardware Platform
- **Computer**: Raspberry Pi 5 (8GB) ✅ Confirmed
- **Audio**: USB audio interface (high-quality DAC/ADC)
- **Display**: 2.8" or 3.5" TFT/OLED display (SPI/I2C)
- **Input**: Rotary encoders, buttons, optional microphone
- **Enclosure**: Custom 3D-printed or aluminum case

---

## Phase 1: Software Foundation (Current)

### Status: In Progress

**Objectives:**
- [x] Identify Raspberry Pi model
- [ ] Build ChimeraPhoenix for ARM/Linux
- [ ] Verify audio processing performance
- [ ] Test Trinity AI connectivity

**Deliverables:**
- Native ARM build of ChimeraPhoenix
- JACK audio server configuration
- Performance benchmarks (latency, CPU usage)

---

## Phase 2: Display Integration

### Target Display Options

**Option A: Adafruit 2.8" TFT (Resistive Touch)**
- Resolution: 320x240
- Interface: SPI
- Touch: 4-wire resistive
- Cost: ~$35
- Pros: Bright, built-in touch
- Cons: Lower resolution

**Option B: Waveshare 3.5" OLED**
- Resolution: 480x320
- Interface: SPI
- Cost: ~$25
- Pros: Better contrast, higher res
- Cons: No touch (add separate)

**Option C: SSD1306 OLED (128x64)**
- Interface: I2C
- Cost: ~$10
- Pros: Simple, low power
- Cons: Very small, monochrome only

**Recommendation**: Start with **Option B** (3.5" OLED) for best balance

### Display Content

**Screen 1: Main View**
```
┌─────────────────────────────┐
│ CHIMERA PHOENIX             │
│ Preset: Warm Analog Dream   │
├─────────────────────────────┤
│ Slot 1: Tube Warmth    [35%]│
│ Slot 2: Analog Phaser  [28%]│
│ Slot 3: Plate Reverb   [22%]│
│ Slot 4: Classic Delay  [15%]│
├─────────────────────────────┤
│ CPU: 42%  Latency: 8.2ms    │
└─────────────────────────────┘
```

**Screen 2: Engine Edit**
```
┌─────────────────────────────┐
│ SLOT 2: ANALOG PHASER       │
├─────────────────────────────┤
│ Rate:     2.3 Hz      [===.]│
│ Depth:    68%         [====]│
│ Feedback: 42%         [===.]│
│ Mix:      28%         [==..]│
├─────────────────────────────┤
│ [◀] Prev  [▶] Next  [✓] Save│
└─────────────────────────────┘
```

**Screen 3: Trinity Prompt**
```
┌─────────────────────────────┐
│ TRINITY AI GENERATOR        │
├─────────────────────────────┤
│ Listening...     🎤          │
│                              │
│ "Create a warm vintage      │
│  tape delay effect"         │
│                              │
├─────────────────────────────┤
│ [●] Speak  [✓] Generate     │
└─────────────────────────────┘
```

### Implementation

**Libraries:**
- `fbcp-ili9341` or `fbtft` for SPI display drivers
- `SDL2` or direct framebuffer for JUCE rendering
- JUCE native Linux rendering (already supported)

**JUCE Integration:**
```cpp
// Simplified - JUCE already handles Linux displays
juce::Desktop::getInstance().setDisplays({
    {0, 0, 480, 320, 1.0, 0}  // Configure for hardware display
});
```

**Pi Configuration:**
```bash
# Enable SPI in /boot/config.txt
dtparam=spi=on
dtoverlay=waveshare35a

# Install display drivers
sudo apt install fbtft-dkms
```

---

## Phase 3: Physical Controls

### Control Scheme

**6 Rotary Encoders:**
1. **Main Encoder**: Preset selection / Menu navigation
2. **Slot 1 Mix**: Adjust slot 1 effect mix
3. **Slot 2 Mix**: Adjust slot 2 effect mix
4. **Slot 3 Mix**: Adjust slot 3 effect mix
5. **Slot 4 Mix**: Adjust slot 4 effect mix
6. **Param Edit**: Edit selected parameter (contextual)

**6 Push Buttons:**
1. **Encoder 1-6 Switches**: Each encoder has built-in push button
   - Encoder 1 push: Enter/Exit menu
   - Encoder 2-5 push: Select slot for editing
   - Encoder 6 push: Cycle parameters

**Additional Buttons:**
1. **Trinity Button**: Activate voice input for AI generation
2. **Save Button**: Save current preset
3. **Bypass Button**: Toggle effect bypass

### Hardware Components

**Rotary Encoders:**
- EC11 style rotary encoders with push switches
- 20 pulses per revolution
- Cost: ~$2 each × 6 = $12

**Buttons:**
- Momentary tactile switches
- LED backlit (optional)
- Cost: ~$1 each × 3 = $3

**Total Control Cost**: ~$15

### GPIO Mapping (Raspberry Pi 5)

```
Encoder 1 (Main):
  - A: GPIO 17
  - B: GPIO 27
  - SW: GPIO 22

Encoder 2 (Slot 1):
  - A: GPIO 5
  - B: GPIO 6
  - SW: GPIO 13

Encoder 3 (Slot 2):
  - A: GPIO 19
  - B: GPIO 26
  - SW: GPIO 21

Encoder 4 (Slot 3):
  - A: GPIO 23
  - B: GPIO 24
  - SW: GPIO 25

Encoder 5 (Slot 4):
  - A: GPIO 16
  - B: GPIO 20
  - SW: GPIO 12

Encoder 6 (Param):
  - A: GPIO 7
  - B: GPIO 8
  - SW: GPIO 11

Trinity Button: GPIO 9
Save Button: GPIO 10
Bypass Button: GPIO 14
```

### Software Implementation

**Library**: `pigpio` or `WiringPi` for GPIO access

**Example Encoder Reading:**
```cpp
#include <pigpio.h>

class RotaryEncoder {
public:
    RotaryEncoder(int pinA, int pinB, int pinSW)
        : m_pinA(pinA), m_pinB(pinB), m_pinSW(pinSW) {
        gpioSetMode(m_pinA, PI_INPUT);
        gpioSetMode(m_pinB, PI_INPUT);
        gpioSetMode(m_pinSW, PI_INPUT);

        gpioSetPullUpDown(m_pinA, PI_PUD_UP);
        gpioSetPullUpDown(m_pinB, PI_PUD_UP);
        gpioSetPullUpDown(m_pinSW, PI_PUD_UP);

        gpioSetAlertFunc(m_pinA, encoderCallback);
    }

    int getValue() const { return m_value; }
    bool isPressed() { return gpioRead(m_pinSW) == 0; }

private:
    static void encoderCallback(int gpio, int level, uint32_t tick);
    int m_pinA, m_pinB, m_pinSW;
    volatile int m_value = 0;
};
```

**Integration with JUCE:**
```cpp
// In PluginProcessor.cpp
void ChimeraPhoenixAudioProcessor::timerCallback() {
    // Poll encoders
    int slot1Mix = encoder2.getValue();
    if (slot1Mix != m_lastSlot1Mix) {
        float newValue = juce::jmap(slot1Mix, 0, 100, 0.0f, 1.0f);
        setSlotMix(0, newValue);
        m_lastSlot1Mix = slot1Mix;
    }

    // Check Trinity button
    if (trinityButton.isPressed()) {
        startVoiceRecording();
    }
}
```

---

## Phase 4: Voice Input for Trinity AI

### Microphone Options

**Option A: USB Microphone**
- Plug-and-play USB mic
- Cost: $20-50
- Pros: Simple, good quality
- Cons: External device

**Option B: I2S MEMS Microphone**
- Adafruit I2S MEMS (SPH0645)
- Interface: I2S to Pi GPIO
- Cost: ~$7
- Pros: Compact, integrated
- Cons: Requires I2S configuration

**Recommendation**: Start with **USB mic** for simplicity

### Voice Pipeline

```
User presses Trinity button
       ↓
Record audio (3-10 seconds)
       ↓
Send to Whisper API (OpenAI)
       ↓
Transcribe to text
       ↓
Send text to Trinity server
       ↓
Receive preset JSON
       ↓
Load preset into ChimeraPhoenix
       ↓
Display: "Preset loaded: Warm Analog Dream"
```

### Implementation

**Audio Recording:**
```bash
# Record 5 seconds from USB mic
arecord -D plughw:2,0 -f S16_LE -r 16000 -d 5 prompt.wav
```

**Whisper Transcription (Python on Pi or Mac server):**
```python
import openai

def transcribe_voice(audio_file):
    with open(audio_file, 'rb') as f:
        transcript = openai.Audio.transcribe("whisper-1", f)
    return transcript['text']
```

**JUCE Integration:**
```cpp
void ChimeraPhoenixAudioProcessor::onTrinityButtonPressed() {
    // Record audio
    system("arecord -D plughw:2,0 -f S16_LE -r 16000 -d 5 /tmp/prompt.wav");

    // Send to transcription (via Trinity server)
    juce::URL transcribeUrl("http://localhost:8000/transcribe");
    transcribeUrl.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
        .withPostData(audioData)
        .withProgressCallback([this](int, int) {}),
        true,
        [this](std::unique_ptr<juce::InputStream> stream, bool success) {
            if (success) {
                juce::String prompt = stream->readEntireStreamAsString();
                sendTrinityPrompt(prompt);
            }
        });
}
```

---

## Phase 5: Physical Enclosure

### Design Specifications

**Dimensions** (approximate):
- Width: 200mm
- Depth: 150mm
- Height: 60mm
- Weight: ~500g

**Layout:**
```
Top Panel:
┌─────────────────────────────┐
│                             │
│     3.5" OLED Display       │
│                             │
├─────────────────────────────┤
│  ①  ②  ③  ④  ⑤  ⑥         │
│ [E] [E] [E] [E] [E] [E]     │
│                             │
│ [Trinity] [Save] [Bypass]   │
└─────────────────────────────┘

E = Rotary Encoder with push switch
```

**Rear Panel:**
```
┌─────────────────────────────┐
│ [USB-C]  [Audio In]  [Out]  │
│  Power   (1/4" TRS)  (TRS)  │
└─────────────────────────────┘
```

### Materials

**Option A: 3D Printed**
- PLA or PETG plastic
- Custom designed in Fusion 360 / OpenSCAD
- Cost: $10-20 in filament
- Time: 8-12 hours print

**Option B: Aluminum Enclosure**
- Hammond 1590BB or similar
- Drill/mill holes for controls
- Cost: $30-50
- Pros: Professional, durable

**Option C: Laser-Cut Acrylic**
- Stack layers for 3D form
- Frosted acrylic for modern look
- Cost: $25-40
- Pros: Beautiful, customizable

**Recommendation**: **3D Printed** for rapid prototyping, **Aluminum** for production

---

## Phase 6: Audio Interface

### USB Audio Interface

**Requirements:**
- 2-in / 2-out minimum
- 24-bit / 48kHz
- Low latency (ASIO/JACK compatible)
- USB 2.0 or 3.0

**Options:**

**Budget: Behringer UCA202** ($30)
- 2-in / 2-out RCA
- 16-bit / 48kHz
- Basic but functional

**Mid-Range: Focusrite Scarlett Solo** ($120)
- 1-in (XLR/instrument) / 2-out (TRS)
- 24-bit / 192kHz
- Excellent preamps

**Recommendation**: **Scarlett Solo** for production quality

### JACK Configuration

```bash
# Start JACK with USB interface
jackd -R -dalsa \
    -dhw:Scarlett \
    -r48000 \
    -p256 \
    -n3 \
    -i2 \
    -o2
```

**Latency calculation:**
- Sample rate: 48kHz
- Buffer: 256 samples
- Periods: 3
- **Total latency**: ~16ms round-trip

---

## Phase 7: System Integration

### Boot-to-Audio Configuration

**Auto-start ChimeraPhoenix on boot:**

Create systemd service:
```bash
sudo nano /etc/systemd/system/chimera.service
```

```ini
[Unit]
Description=ChimeraPhoenix Hardware Unit
After=sound.target

[Service]
Type=simple
User=pi
ExecStartPre=/bin/sleep 10
ExecStartPre=/usr/bin/jackd -R -dalsa -dhw:Scarlett -r48000 -p256 -n3 -d &
ExecStart=/home/pi/chimera_run.sh
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

Enable:
```bash
sudo systemctl enable chimera.service
sudo systemctl start chimera.service
```

### Power Management

- Configure Pi to run headless (no HDMI output except display)
- Disable WiFi if not needed (reduce interference)
- CPU governor: `performance` mode

```bash
# Set CPU to performance mode
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

---

## Bill of Materials (BOM)

| Component               | Qty | Unit Price | Total  | Link/Source        |
|-------------------------|-----|------------|--------|--------------------|
| Raspberry Pi 5 (8GB)    | 1   | $80        | $80    | ✅ Already owned   |
| Power Supply (USB-C 5V) | 1   | $10        | $10    | Official RPi       |
| MicroSD Card (64GB)     | 1   | $12        | $12    | SanDisk Extreme    |
| 3.5" OLED Display (SPI) | 1   | $25        | $25    | Waveshare          |
| Rotary Encoders (EC11)  | 6   | $2         | $12    | Amazon             |
| Push Buttons (Tactile)  | 3   | $1         | $3     | Amazon             |
| USB Microphone          | 1   | $25        | $25    | Blue Snowball/Mini |
| Audio Interface         | 1   | $120       | $120   | Focusrite Solo     |
| Enclosure (3D Printed)  | 1   | $15        | $15    | PLA filament       |
| Wiring/Headers          | -   | $10        | $10    | Jumpers, solder    |
| **TOTAL**               |     |            |**$312**| (excl. Pi)         |

**With Pi**: $392 total
**Without Pi** (already owned): $312

---

## Development Timeline

### Week 1: Software Foundation ⬅️ **CURRENT**
- [ ] Build ChimeraPhoenix on Pi
- [ ] Verify audio performance
- [ ] Test Trinity connectivity

### Week 2: Display Integration
- [ ] Order display hardware
- [ ] Configure SPI display
- [ ] Adapt JUCE UI for 480x320 resolution
- [ ] Implement status/preset view

### Week 3: Physical Controls
- [ ] Wire rotary encoders to GPIO
- [ ] Implement encoder reading library
- [ ] Integrate with JUCE parameter system
- [ ] Add button controls

### Week 4: Voice Input
- [ ] Setup USB microphone
- [ ] Implement Whisper transcription
- [ ] Test Trinity voice-to-preset pipeline
- [ ] Polish UI feedback

### Week 5: Enclosure Design
- [ ] Design 3D model (Fusion 360)
- [ ] Print prototype
- [ ] Fit test all components
- [ ] Iterate on design

### Week 6: Final Assembly
- [ ] Assemble all components
- [ ] Configure boot-to-audio
- [ ] Performance optimization
- [ ] User testing and refinement

### Week 7: Polish and Demo
- [ ] Create demo video
- [ ] Document operation
- [ ] Prepare for beta testers
- [ ] Plan manufacturing (if proceeding)

---

## Advanced Features (Future)

### MIDI I/O
- USB MIDI interface or DIN MIDI via HAT
- Control ChimeraPhoenix from external controllers
- Use as MIDI effect processor

### Preset Storage
- SD card preset library
- Quick-load favorites via encoder
- Backup/restore via USB

### Network Features
- Web UI for remote control
- Preset sharing via cloud
- Firmware updates OTA

### Multi-Engine Morphing
- Crossfade between presets
- X/Y pad control (if touchscreen)
- Automation recording

---

## Risk Assessment

| Risk                          | Likelihood | Impact | Mitigation                          |
|-------------------------------|------------|--------|-------------------------------------|
| Insufficient CPU for DSP      | Low        | High   | Benchmark early, optimize code      |
| Display driver issues         | Medium     | Medium | Test multiple display options       |
| GPIO noise/reliability        | Medium     | Low    | Use proper pullups, debouncing      |
| Enclosure fit issues          | Medium     | Low    | Print prototype early, iterate      |
| Audio latency too high        | Low        | High   | Use optimized JACK config, low buffers |
| Trinity server connectivity   | Low        | Medium | Local fallback, cached presets      |

---

## Success Metrics

**Technical:**
- Audio latency: <20ms round-trip
- CPU usage: <70% average
- Stable 48kHz operation
- No audio dropouts

**User Experience:**
- Preset load: <2 seconds
- Trinity generation: <30 seconds
- Intuitive control layout
- Responsive UI (60fps min)

**Commercial Viability:**
- BOM cost: <$350
- Assembly time: <2 hours
- Retail price target: $799-999
- Profit margin: >60%

---

## Next Immediate Actions

1. **Complete Pi build** (run `pi_setup.sh`)
2. **Benchmark audio performance** (latency, CPU)
3. **Order display and encoders** (get shipping started)
4. **Design GPIO control scheme** (detailed pin mapping)
5. **Draft enclosure in CAD** (prepare for printing)

Once software is running on Pi, hardware integration accelerates rapidly.

---

## Questions for Next Planning Session

1. **Display preference**: Touch or buttons-only?
2. **Enclosure aesthetic**: Industrial, boutique, modern?
3. **Price target**: DIY kit ($400) or finished unit ($800)?
4. **Manufacturing**: One-off prototype or small batch (10-50 units)?
5. **Trinity server**: Pi-hosted or Mac-hosted for prototype?

---

**Project Status**: **Phase 1 - Software Foundation** (In Progress)
**Next Milestone**: ChimeraPhoenix running natively on Pi 5
**Target Date**: End of Week 1

Let's build this hardware! 🚀
