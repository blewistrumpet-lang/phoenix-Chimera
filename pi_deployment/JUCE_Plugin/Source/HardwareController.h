#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <thread>
#include <functional>

#ifdef __linux__
    #include <gpiod.h>
#endif

/**
 * Hardware Controller for Raspberry Pi GPIO
 *
 * Manages 3 rotary encoders and 3 three-way switches
 * Based on working test_encoder.py implementation
 *
 * Hardware Configuration:
 * - Encoder 1: A=GPIO5,  B=GPIO6,  Button=GPIO26
 * - Encoder 2: A=GPIO23, B=GPIO24, Button=GPIO25
 * - Encoder 3: A=GPIO17, B=GPIO27, Button=GPIO22
 * - Switch 1:  Pin1=GPIO19, Pin2=GPIO21
 * - Switch 2:  Pin1=GPIO16, Pin2=GPIO20
 * - Switch 3:  Pin1=GPIO12, Pin2=GPIO13
 */
class HardwareController : public juce::Thread
{
public:
    // Encoder state
    struct EncoderState {
        std::atomic<int> position{0};
        std::atomic<bool> buttonPressed{false};

        int getPosition() const { return position.load(); }
        bool isButtonPressed() const { return buttonPressed.load(); }
    };

    // Switch position enum
    enum class SwitchPosition {
        UP,
        MIDDLE,
        DOWN,
        UNKNOWN
    };

    // Switch state
    struct SwitchState {
        std::atomic<int> positionValue{1}; // 0=UP, 1=MIDDLE, 2=DOWN

        SwitchPosition getPosition() const {
            switch (positionValue.load()) {
                case 0: return SwitchPosition::UP;
                case 1: return SwitchPosition::MIDDLE;
                case 2: return SwitchPosition::DOWN;
                default: return SwitchPosition::UNKNOWN;
            }
        }

        juce::String getPositionString() const {
            switch (getPosition()) {
                case SwitchPosition::UP: return "UP";
                case SwitchPosition::MIDDLE: return "MIDDLE";
                case SwitchPosition::DOWN: return "DOWN";
                default: return "UNKNOWN";
            }
        }
    };

    // Callback types
    using EncoderCallback = std::function<void(int encoderNum, int position, bool clockwise)>;
    using EncoderButtonCallback = std::function<void(int encoderNum)>;
    using SwitchCallback = std::function<void(int switchNum, SwitchPosition position)>;

    HardwareController();
    ~HardwareController() override;

    // Thread control
    void startHardwareMonitoring();
    void stopHardwareMonitoring();

    // State getters
    EncoderState& getEncoder(int num) { return encoders[num]; }
    SwitchState& getSwitch(int num) { return switches[num]; }

    // Callback setters (for future functionality)
    void setEncoderCallback(EncoderCallback cb) { onEncoderChange = cb; }
    void setEncoderButtonCallback(EncoderButtonCallback cb) { onEncoderButton = cb; }
    void setSwitchCallback(SwitchCallback cb) { onSwitchChange = cb; }

private:
    void run() override;

#ifdef __linux__
    bool initializeGPIO();
    void shutdownGPIO();
    void pollHardware();

    // GPIO chip and lines
    gpiod_chip* chip = nullptr;

    // Encoder lines
    gpiod_line* enc1_a = nullptr;
    gpiod_line* enc1_b = nullptr;
    gpiod_line* enc1_btn = nullptr;

    gpiod_line* enc2_a = nullptr;
    gpiod_line* enc2_b = nullptr;
    gpiod_line* enc2_btn = nullptr;

    gpiod_line* enc3_a = nullptr;
    gpiod_line* enc3_b = nullptr;
    gpiod_line* enc3_btn = nullptr;

    // Switch lines
    gpiod_line* sw1_pin1 = nullptr;
    gpiod_line* sw1_pin2 = nullptr;

    gpiod_line* sw2_pin1 = nullptr;
    gpiod_line* sw2_pin2 = nullptr;

    gpiod_line* sw3_pin1 = nullptr;
    gpiod_line* sw3_pin2 = nullptr;

    // Last states for edge detection
    int enc1_last_a = 1, enc1_last_b = 1;
    int enc2_last_a = 1, enc2_last_b = 1;
    int enc3_last_a = 1, enc3_last_b = 1;

    int sw1_last_pin1 = 1, sw1_last_pin2 = 1;
    int sw2_last_pin1 = 1, sw2_last_pin2 = 1;
    int sw3_last_pin1 = 1, sw3_last_pin2 = 1;

    SwitchPosition decodeSwitchPosition(int pin1, int pin2);
#endif

    // Hardware state (3 encoders, 3 switches)
    EncoderState encoders[3];
    SwitchState switches[3];

    // Callbacks
    EncoderCallback onEncoderChange;
    EncoderButtonCallback onEncoderButton;
    SwitchCallback onSwitchChange;

    std::atomic<bool> hardwareInitialized{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HardwareController)
};
