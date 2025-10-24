#include "HardwareController.h"

HardwareController::HardwareController()
    : Thread("HardwareController")
{
    DBG("HardwareController created");
}

HardwareController::~HardwareController()
{
    stopHardwareMonitoring();
}

void HardwareController::startHardwareMonitoring()
{
    DBG("Starting hardware monitoring...");
    startThread();
}

void HardwareController::stopHardwareMonitoring()
{
    DBG("Stopping hardware monitoring...");
    signalThreadShouldExit();
    waitForThreadToExit(2000);

#ifdef __linux__
    shutdownGPIO();
#endif
}

void HardwareController::run()
{
#ifdef __linux__
    if (!initializeGPIO()) {
        DBG("ERROR: Failed to initialize GPIO");
        return;
    }

    DBG("Hardware monitoring thread running...");

    while (!threadShouldExit()) {
        pollHardware();
        wait(1); // 1ms polling rate
    }

    shutdownGPIO();
    DBG("Hardware monitoring thread exited");
#else
    DBG("GPIO hardware only available on Linux");
#endif
}

#ifdef __linux__

bool HardwareController::initializeGPIO()
{
    DBG("Initializing GPIO hardware...");

    // Open GPIO chip
    chip = gpiod_chip_open_by_name("gpiochip4");
    if (!chip) {
        DBG("ERROR: Failed to open gpiochip4");
        return false;
    }

    // Get encoder 1 lines
    enc1_a = gpiod_chip_get_line(chip, 5);
    enc1_b = gpiod_chip_get_line(chip, 6);
    enc1_btn = gpiod_chip_get_line(chip, 26);

    // Get encoder 2 lines
    enc2_a = gpiod_chip_get_line(chip, 23);
    enc2_b = gpiod_chip_get_line(chip, 24);
    enc2_btn = gpiod_chip_get_line(chip, 25);

    // Get encoder 3 lines
    enc3_a = gpiod_chip_get_line(chip, 17);
    enc3_b = gpiod_chip_get_line(chip, 27);
    enc3_btn = gpiod_chip_get_line(chip, 22);

    // Get switch lines
    sw1_pin1 = gpiod_chip_get_line(chip, 19);
    sw1_pin2 = gpiod_chip_get_line(chip, 21);

    sw2_pin1 = gpiod_chip_get_line(chip, 16);
    sw2_pin2 = gpiod_chip_get_line(chip, 20);

    sw3_pin1 = gpiod_chip_get_line(chip, 12);
    sw3_pin2 = gpiod_chip_get_line(chip, 13);

    // Request all lines as inputs with pull-up
    gpiod_line_request_config config;
    config.consumer = "ChimeraPhoenix";
    config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
    config.flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;

    // Request encoder 1
    if (gpiod_line_request(enc1_a, &config, 0) < 0 ||
        gpiod_line_request(enc1_b, &config, 0) < 0 ||
        gpiod_line_request(enc1_btn, &config, 0) < 0) {
        DBG("ERROR: Failed to request encoder 1 lines");
        return false;
    }

    // Request encoder 2
    if (gpiod_line_request(enc2_a, &config, 0) < 0 ||
        gpiod_line_request(enc2_b, &config, 0) < 0 ||
        gpiod_line_request(enc2_btn, &config, 0) < 0) {
        DBG("ERROR: Failed to request encoder 2 lines");
        return false;
    }

    // Request encoder 3
    if (gpiod_line_request(enc3_a, &config, 0) < 0 ||
        gpiod_line_request(enc3_b, &config, 0) < 0 ||
        gpiod_line_request(enc3_btn, &config, 0) < 0) {
        DBG("ERROR: Failed to request encoder 3 lines");
        return false;
    }

    // Request switches
    if (gpiod_line_request(sw1_pin1, &config, 0) < 0 ||
        gpiod_line_request(sw1_pin2, &config, 0) < 0 ||
        gpiod_line_request(sw2_pin1, &config, 0) < 0 ||
        gpiod_line_request(sw2_pin2, &config, 0) < 0 ||
        gpiod_line_request(sw3_pin1, &config, 0) < 0 ||
        gpiod_line_request(sw3_pin2, &config, 0) < 0) {
        DBG("ERROR: Failed to request switch lines");
        return false;
    }

    // Initialize last states
    enc1_last_a = gpiod_line_get_value(enc1_a);
    enc1_last_b = gpiod_line_get_value(enc1_b);
    enc2_last_a = gpiod_line_get_value(enc2_a);
    enc2_last_b = gpiod_line_get_value(enc2_b);
    enc3_last_a = gpiod_line_get_value(enc3_a);
    enc3_last_b = gpiod_line_get_value(enc3_b);

    sw1_last_pin1 = gpiod_line_get_value(sw1_pin1);
    sw1_last_pin2 = gpiod_line_get_value(sw1_pin2);
    sw2_last_pin1 = gpiod_line_get_value(sw2_pin1);
    sw2_last_pin2 = gpiod_line_get_value(sw2_pin2);
    sw3_last_pin1 = gpiod_line_get_value(sw3_pin1);
    sw3_last_pin2 = gpiod_line_get_value(sw3_pin2);

    hardwareInitialized = true;
    DBG("✓ GPIO hardware initialized successfully");
    return true;
}

void HardwareController::shutdownGPIO()
{
    if (!hardwareInitialized)
        return;

    DBG("Shutting down GPIO...");

    // Release all lines
    if (enc1_a) gpiod_line_release(enc1_a);
    if (enc1_b) gpiod_line_release(enc1_b);
    if (enc1_btn) gpiod_line_release(enc1_btn);

    if (enc2_a) gpiod_line_release(enc2_a);
    if (enc2_b) gpiod_line_release(enc2_b);
    if (enc2_btn) gpiod_line_release(enc2_btn);

    if (enc3_a) gpiod_line_release(enc3_a);
    if (enc3_b) gpiod_line_release(enc3_b);
    if (enc3_btn) gpiod_line_release(enc3_btn);

    if (sw1_pin1) gpiod_line_release(sw1_pin1);
    if (sw1_pin2) gpiod_line_release(sw1_pin2);
    if (sw2_pin1) gpiod_line_release(sw2_pin1);
    if (sw2_pin2) gpiod_line_release(sw2_pin2);
    if (sw3_pin1) gpiod_line_release(sw3_pin1);
    if (sw3_pin2) gpiod_line_release(sw3_pin2);

    // Close chip
    if (chip) {
        gpiod_chip_close(chip);
        chip = nullptr;
    }

    hardwareInitialized = false;
    DBG("✓ GPIO shutdown complete");
}

HardwareController::SwitchPosition HardwareController::decodeSwitchPosition(int pin1, int pin2)
{
    // Inverted from hardware: physical UP = 01 pattern, physical DOWN = 10 pattern
    if (pin1 == 1 && pin2 == 0) return SwitchPosition::UP;
    if (pin1 == 1 && pin2 == 1) return SwitchPosition::MIDDLE;
    if (pin1 == 0 && pin2 == 1) return SwitchPosition::DOWN;
    return SwitchPosition::UNKNOWN;
}

void HardwareController::readImmediateSwitchPositions()
{
#ifdef __linux__
    if (!hardwareInitialized) return;

    // Read all switch positions directly from GPIO pins
    int sw1_p1 = gpiod_line_get_value(sw1_pin1);
    int sw1_p2 = gpiod_line_get_value(sw1_pin2);
    int sw2_p1 = gpiod_line_get_value(sw2_pin1);
    int sw2_p2 = gpiod_line_get_value(sw2_pin2);
    int sw3_p1 = gpiod_line_get_value(sw3_pin1);
    int sw3_p2 = gpiod_line_get_value(sw3_pin2);

    // Decode and update positions
    switches[0].positionValue = static_cast<int>(decodeSwitchPosition(sw1_p1, sw1_p2));
    switches[1].positionValue = static_cast<int>(decodeSwitchPosition(sw2_p1, sw2_p2));
    switches[2].positionValue = static_cast<int>(decodeSwitchPosition(sw3_p1, sw3_p2));

    DBG("Immediate switch read: SW1=" << switches[0].getPositionString()
        << " SW2=" << switches[1].getPositionString()
        << " SW3=" << switches[2].getPositionString());
#endif
}

void HardwareController::pollHardware()
{
    // Read encoder 1
    int enc1_a_val = gpiod_line_get_value(enc1_a);
    int enc1_b_val = gpiod_line_get_value(enc1_b);
    int enc1_btn_val = gpiod_line_get_value(enc1_btn);

    // Check encoder 1 rotation (inverted: CW when A!=B after A changes)
    if (enc1_a_val != enc1_last_a || enc1_b_val != enc1_last_b) {
        if (enc1_a_val != enc1_last_a) {
            bool clockwise = (enc1_a_val == enc1_b_val); // Inverted logic
            if (clockwise) {
                encoders[0].position++;
            } else {
                encoders[0].position--;
            }
            if (onEncoderChange) {
                onEncoderChange(0, encoders[0].position.load(), clockwise);
            }
        }
        enc1_last_a = enc1_a_val;
        enc1_last_b = enc1_b_val;
    }

    // Check encoder 1 button (active low)
    if (enc1_btn_val == 0 && !encoders[0].buttonPressed) {
        encoders[0].buttonPressed = true;
        if (onEncoderButton) {
            onEncoderButton(0);
        }
    } else if (enc1_btn_val == 1) {
        encoders[0].buttonPressed = false;
    }

    // Read encoder 2
    int enc2_a_val = gpiod_line_get_value(enc2_a);
    int enc2_b_val = gpiod_line_get_value(enc2_b);
    int enc2_btn_val = gpiod_line_get_value(enc2_btn);

    // Check encoder 2 rotation
    if (enc2_a_val != enc2_last_a || enc2_b_val != enc2_last_b) {
        if (enc2_a_val != enc2_last_a) {
            bool clockwise = (enc2_a_val == enc2_b_val);
            if (clockwise) {
                encoders[1].position++;
            } else {
                encoders[1].position--;
            }
            if (onEncoderChange) {
                onEncoderChange(1, encoders[1].position.load(), clockwise);
            }
        }
        enc2_last_a = enc2_a_val;
        enc2_last_b = enc2_b_val;
    }

    // Check encoder 2 button
    if (enc2_btn_val == 0 && !encoders[1].buttonPressed) {
        encoders[1].buttonPressed = true;
        if (onEncoderButton) {
            onEncoderButton(1);
        }
    } else if (enc2_btn_val == 1) {
        encoders[1].buttonPressed = false;
    }

    // Read encoder 3
    int enc3_a_val = gpiod_line_get_value(enc3_a);
    int enc3_b_val = gpiod_line_get_value(enc3_b);
    int enc3_btn_val = gpiod_line_get_value(enc3_btn);

    // Check encoder 3 rotation
    if (enc3_a_val != enc3_last_a || enc3_b_val != enc3_last_b) {
        if (enc3_a_val != enc3_last_a) {
            bool clockwise = (enc3_a_val == enc3_b_val);
            if (clockwise) {
                encoders[2].position++;
            } else {
                encoders[2].position--;
            }
            if (onEncoderChange) {
                onEncoderChange(2, encoders[2].position.load(), clockwise);
            }
        }
        enc3_last_a = enc3_a_val;
        enc3_last_b = enc3_b_val;
    }

    // Check encoder 3 button
    if (enc3_btn_val == 0 && !encoders[2].buttonPressed) {
        encoders[2].buttonPressed = true;
        if (onEncoderButton) {
            onEncoderButton(2);
        }
    } else if (enc3_btn_val == 1) {
        encoders[2].buttonPressed = false;
    }

    // Read switches
    int sw1_p1 = gpiod_line_get_value(sw1_pin1);
    int sw1_p2 = gpiod_line_get_value(sw1_pin2);
    int sw2_p1 = gpiod_line_get_value(sw2_pin1);
    int sw2_p2 = gpiod_line_get_value(sw2_pin2);
    int sw3_p1 = gpiod_line_get_value(sw3_pin1);
    int sw3_p2 = gpiod_line_get_value(sw3_pin2);

    // Check switch 1
    if (sw1_p1 != sw1_last_pin1 || sw1_p2 != sw1_last_pin2) {
        SwitchPosition pos = decodeSwitchPosition(sw1_p1, sw1_p2);
        switches[0].positionValue = static_cast<int>(pos);
        if (onSwitchChange) {
            onSwitchChange(0, pos);
        }
        sw1_last_pin1 = sw1_p1;
        sw1_last_pin2 = sw1_p2;
    }

    // Check switch 2
    if (sw2_p1 != sw2_last_pin1 || sw2_p2 != sw2_last_pin2) {
        SwitchPosition pos = decodeSwitchPosition(sw2_p1, sw2_p2);
        switches[1].positionValue = static_cast<int>(pos);
        if (onSwitchChange) {
            onSwitchChange(1, pos);
        }
        sw2_last_pin1 = sw2_p1;
        sw2_last_pin2 = sw2_p2;
    }

    // Check switch 3
    if (sw3_p1 != sw3_last_pin1 || sw3_p2 != sw3_last_pin2) {
        SwitchPosition pos = decodeSwitchPosition(sw3_p1, sw3_p2);
        switches[2].positionValue = static_cast<int>(pos);
        if (onSwitchChange) {
            onSwitchChange(2, pos);
        }
        sw3_last_pin1 = sw3_p1;
        sw3_last_pin2 = sw3_p2;
    }
}

#endif // __linux__
