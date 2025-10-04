#!/usr/bin/env python3
"""
Standalone CLI Tester for Chimera Phoenix
Simple command-line interface for testing preset generation and modification
"""

import requests
import json
import sys
from typing import Dict, Any, Optional
from engine_mapping_correct import ENGINE_MAPPING
import argparse

class ChimeraCliTester:
    def __init__(self):
        self.api_base = "http://localhost:8000"
        self.current_preset: Optional[Dict[str, Any]] = None
        
    def check_server(self) -> bool:
        """Check if server is running"""
        try:
            response = requests.get(f"{self.api_base}/", timeout=2)
            return response.status_code == 200
        except:
            return False
            
    def generate_preset(self, prompt: str) -> bool:
        """Generate a preset from prompt"""
        print(f"\n🎯 Generating preset: \"{prompt}\"")
        print("-" * 80)
        
        try:
            response = requests.post(
                f"{self.api_base}/generate",
                json={"prompt": prompt},
                timeout=30
            )
            
            if response.status_code == 200:
                result = response.json()
                self.current_preset = result["preset"]
                metadata = result.get("metadata", {})
                
                print(f"✅ Generated in {metadata.get('generation_time_seconds', 0):.1f}s")
                self.display_preset()
                return True
            else:
                print(f"❌ Failed: {response.status_code}")
                return False
                
        except Exception as e:
            print(f"❌ Error: {str(e)}")
            return False
            
    def modify_preset(self, modification: str) -> bool:
        """Modify current preset"""
        if not self.current_preset:
            print("❌ No preset loaded. Generate one first.")
            return False
            
        print(f"\n🔄 Applying modification: \"{modification}\"")
        print("-" * 80)
        
        try:
            response = requests.post(
                f"{self.api_base}/modify",
                json={
                    "preset": self.current_preset,
                    "modification": modification
                },
                timeout=10
            )
            
            if response.status_code == 200:
                result = response.json()
                if result.get("success"):
                    self.current_preset = result["data"]
                    print(f"✅ {result.get('message', 'Modified successfully')}")
                    self.display_preset()
                    return True
                else:
                    print(f"❌ {result.get('message', 'Failed')}")
                    return False
            else:
                print(f"❌ Failed: {response.status_code}")
                return False
                
        except Exception as e:
            print(f"❌ Error: {str(e)}")
            return False
            
    def display_preset(self):
        """Display current preset"""
        if not self.current_preset:
            return
            
        print(f"\n📝 Preset: '{self.current_preset.get('name', 'Unknown')}'")
        print(f"🎨 Vibe: {self.current_preset.get('vibe', '')}")
        
        print("\n🎛️ Active Engines:")
        parameters = self.current_preset.get("parameters", {})
        
        for slot in range(1, 7):
            engine_id = parameters.get(f'slot{slot}_engine', 0)
            if engine_id > 0 and parameters.get(f'slot{slot}_bypass', 0) < 0.5:
                engine_name = ENGINE_MAPPING.get(engine_id, f"Unknown ({engine_id})")
                mix = parameters.get(f'slot{slot}_mix', 0.5)
                drive = parameters.get(f'slot{slot}_param1', 0)
                tone = parameters.get(f'slot{slot}_param2', 0)
                
                print(f"  Slot {slot}: {engine_name}")
                print(f"    Mix: {mix:.0%} | Drive: {drive:.0%} | Tone: {tone:.0%}")
                
    def save_preset(self, filename: str):
        """Save current preset to file"""
        if not self.current_preset:
            print("❌ No preset to save")
            return
            
        try:
            with open(filename, 'w') as f:
                json.dump(self.current_preset, f, indent=2)
            print(f"✅ Saved to {filename}")
        except Exception as e:
            print(f"❌ Error saving: {str(e)}")
            
    def load_preset(self, filename: str):
        """Load preset from file"""
        try:
            with open(filename, 'r') as f:
                self.current_preset = json.load(f)
            print(f"✅ Loaded from {filename}")
            self.display_preset()
        except Exception as e:
            print(f"❌ Error loading: {str(e)}")
            
    def interactive_mode(self):
        """Run interactive mode"""
        print("\n" + "="*80)
        print("🎭 CHIMERA PHOENIX - Interactive CLI Tester")
        print("="*80)
        
        if not self.check_server():
            print("⚠️  Server not running! Start it with: python3 main.py")
            return
            
        print("✅ Server connected")
        print("\nCommands:")
        print("  g <prompt>     - Generate preset")
        print("  m <mod>        - Modify current preset")
        print("  d              - Display current preset")
        print("  s <filename>   - Save preset to file")
        print("  l <filename>   - Load preset from file")
        print("  e              - Examples")
        print("  q              - Quit")
        print()
        
        while True:
            try:
                command = input("chimera> ").strip()
                
                if not command:
                    continue
                    
                parts = command.split(None, 1)
                cmd = parts[0].lower()
                
                if cmd == 'q':
                    print("Goodbye!")
                    break
                    
                elif cmd == 'g' and len(parts) > 1:
                    self.generate_preset(parts[1])
                    
                elif cmd == 'm' and len(parts) > 1:
                    self.modify_preset(parts[1])
                    
                elif cmd == 'd':
                    self.display_preset()
                    
                elif cmd == 's' and len(parts) > 1:
                    self.save_preset(parts[1])
                    
                elif cmd == 'l' and len(parts) > 1:
                    self.load_preset(parts[1])
                    
                elif cmd == 'e':
                    print("\nExample prompts:")
                    print("  • Dark ambient horror atmosphere")
                    print("  • Classic Roland TB-303 acid bass")
                    print("  • The sound of time freezing")
                    print("  • Vintage Juno-106 pad with chorus")
                    print("\nExample modifications:")
                    print("  • Make it darker and more aggressive")
                    print("  • Add spacious reverb and delay")
                    print("  • Transform into liquid metal")
                    
                else:
                    print("Unknown command. Type 'e' for examples, 'q' to quit.")
                    
            except KeyboardInterrupt:
                print("\nUse 'q' to quit")
            except Exception as e:
                print(f"Error: {str(e)}")

def main():
    parser = argparse.ArgumentParser(description="Chimera Phoenix CLI Tester")
    parser.add_argument("-g", "--generate", help="Generate preset from prompt")
    parser.add_argument("-m", "--modify", help="Modify last/loaded preset")
    parser.add_argument("-l", "--load", help="Load preset from file")
    parser.add_argument("-s", "--save", help="Save preset to file")
    parser.add_argument("-i", "--interactive", action="store_true", help="Interactive mode")
    
    args = parser.parse_args()
    
    tester = ChimeraCliTester()
    
    if not tester.check_server():
        print("⚠️  Server not running! Start it with: python3 main.py")
        sys.exit(1)
    
    # Handle command line arguments
    if args.load:
        tester.load_preset(args.load)
        
    if args.generate:
        tester.generate_preset(args.generate)
        
    if args.modify:
        tester.modify_preset(args.modify)
        
    if args.save:
        tester.save_preset(args.save)
        
    if args.interactive or not any([args.generate, args.modify, args.load, args.save]):
        tester.interactive_mode()

if __name__ == "__main__":
    main()