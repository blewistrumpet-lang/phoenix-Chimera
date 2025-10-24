#!/usr/bin/env python3
"""
Live Hardware Monitor - Interactive GPIO Test
Runs plugin and monitors for hardware events in real-time
"""

import subprocess
import sys
import time
import threading
from datetime import datetime

class LiveHardwareMonitor:
    def __init__(self):
        self.plugin_process = None
        self.event_count = 0
        self.ready_for_input = False

    def monitor_output(self):
        """Monitor plugin output in real-time"""
        for line in self.plugin_process.stdout:
            line = line.strip()
            timestamp = datetime.now().strftime("%H:%M:%S")

            # Check for hardware events
            if "ENC" in line and ("CW" in line or "CCW" in line or "pos=" in line):
                print(f"\n🎛️  [{timestamp}] ENCODER EVENT: {line}")
                self.event_count += 1

            elif "SW" in line and ("UP" in line or "MID" in line or "DOWN" in line):
                print(f"\n🔘 [{timestamp}] SWITCH EVENT: {line}")
                self.event_count += 1

            elif "Mode changed to:" in line:
                print(f"\n✨ [{timestamp}] MODE CHANGE: {line}")
                self.event_count += 1

            elif "Parameter" in line and "changed" in line:
                print(f"\n📊 [{timestamp}] PARAMETER: {line}")
                self.event_count += 1

            elif "Hardware monitoring thread running" in line:
                self.ready_for_input = True
                print(f"\n✅ [{timestamp}] GPIO READY - Hardware monitoring active!")

            elif "GPIO hardware initialized" in line:
                print(f"✓ [{timestamp}] {line}")

    def start(self):
        """Start the plugin and monitoring"""
        plugin_path = "/home/branden/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix"

        print("="*70)
        print("🎮 LIVE HARDWARE MONITOR - INTERACTIVE GPIO TEST")
        print("="*70)
        print(f"Starting at {datetime.now().strftime('%H:%M:%S')}")
        print("-"*70)

        # Start plugin
        self.plugin_process = subprocess.Popen(
            [plugin_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )

        # Start monitor thread
        monitor_thread = threading.Thread(target=self.monitor_output)
        monitor_thread.daemon = True
        monitor_thread.start()

        # Wait for initialization
        print("Initializing GPIO hardware...")
        time.sleep(3)

        if self.ready_for_input:
            print("\n" + "="*70)
            print("🟢 READY FOR HARDWARE INPUT!")
            print("="*70)
        else:
            print("\n⏳ Waiting for hardware initialization...")
            time.sleep(2)

        print("\n📋 TEST INSTRUCTIONS:")
        print("-"*70)
        print("Please perform these actions:")
        print()
        print("1️⃣  ENCODER 1: Turn clockwise 5 clicks")
        print("2️⃣  ENCODER 1: Turn counter-clockwise 5 clicks")
        print("3️⃣  ENCODER 2: Turn clockwise")
        print("4️⃣  ENCODER 3: Turn clockwise")
        print("5️⃣  ENCODER 1: Press the button")
        print()
        print("6️⃣  SWITCH 1 (MODE): Flip to MIDDLE position")
        print("7️⃣  SWITCH 1 (MODE): Flip to DOWN position")
        print("8️⃣  SWITCH 2 (VARIANT): Flip to DOWN position (Bank B)")
        print()
        print("9️⃣  In MIX mode: Turn ENCODER 2 (should control Space macro)")
        print()
        print("Press Ctrl+C when done")
        print("="*70)
        print("\n🔴 MONITORING LIVE - READY FOR YOUR INPUT NOW!\n")

        # Keep running
        try:
            while True:
                time.sleep(0.5)
                # Show a heartbeat every 10 seconds if no events
                if self.event_count == 0:
                    if int(time.time()) % 10 == 0:
                        print(".", end="", flush=True)

        except KeyboardInterrupt:
            print(f"\n\n{'='*70}")
            print(f"TEST COMPLETE - Detected {self.event_count} hardware events")
            print("="*70)

            if self.event_count > 0:
                print("✅ GPIO HARDWARE IS WORKING!")
            else:
                print("❌ No hardware events detected")
                print("   Check that encoders/switches are connected")

            self.plugin_process.terminate()

if __name__ == "__main__":
    monitor = LiveHardwareMonitor()
    monitor.start()