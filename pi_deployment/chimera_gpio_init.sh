#!/bin/bash
#==============================================================================
# Chimera Phoenix GPIO Initialization Script
# Purpose: Enable internal pull-up resistors on all GPIO inputs at boot
# Required for: Raspberry Pi 5 GPIO hardware (encoders and switches)
# Date: November 3, 2025
#==============================================================================

# Enable pull-ups for Encoder 1 (GPIO 5, 6, 26)
pinctrl set 5 ip pu    # Encoder 1 CLK
pinctrl set 6 ip pu    # Encoder 1 DT
pinctrl set 26 ip pu   # Encoder 1 Button

# Enable pull-ups for Encoder 2 (GPIO 23, 24, 25)
pinctrl set 23 ip pu   # Encoder 2 CLK
pinctrl set 24 ip pu   # Encoder 2 DT
pinctrl set 25 ip pu   # Encoder 2 Button

# Enable pull-ups for Encoder 3 (GPIO 4, 14, 16)
pinctrl set 4 ip pu    # Encoder 3 CLK
pinctrl set 14 ip pu   # Encoder 3 DT
pinctrl set 16 ip pu   # Encoder 3 Button

# Enable pull-ups for Switch 1 (GPIO 7, 8)
pinctrl set 7 ip pu    # Switch 1 PIN1
pinctrl set 8 ip pu    # Switch 1 PIN2

# Enable pull-ups for Switch 2 (GPIO 11, 10)
pinctrl set 11 ip pu   # Switch 2 PIN1
pinctrl set 10 ip pu   # Switch 2 PIN2

# Enable pull-ups for Switch 3 (GPIO 12, 13)
pinctrl set 12 ip pu   # Switch 3 PIN1
pinctrl set 13 ip pu   # Switch 3 PIN2

exit 0
