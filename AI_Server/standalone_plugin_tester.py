#!/usr/bin/env python3
"""
Standalone Plugin Tester for Chimera Phoenix
Interactive GUI for testing preset generation and modification
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import json
import requests
import threading
from typing import Dict, Any, Optional
from engine_mapping_correct import ENGINE_MAPPING

class ChimeraPluginTester:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Chimera Phoenix - Standalone Plugin Tester")
        self.root.geometry("1200x800")
        
        # Current preset storage
        self.current_preset: Optional[Dict[str, Any]] = None
        self.api_base = "http://localhost:8000"
        
        # Create UI
        self.setup_ui()
        
        # Check server connection
        self.check_server_connection()
        
    def setup_ui(self):
        """Create the user interface"""
        # Main container
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=2)
        main_frame.rowconfigure(2, weight=1)
        
        # === LEFT PANEL: Controls ===
        left_frame = ttk.LabelFrame(main_frame, text="Controls", padding="10")
        left_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(0, 5))
        
        # Server Status
        self.status_label = ttk.Label(left_frame, text="Server: Checking...", foreground="gray")
        self.status_label.grid(row=0, column=0, columnspan=2, pady=(0, 10))
        
        # --- Generation Section ---
        ttk.Label(left_frame, text="Generate Preset", font=("Arial", 12, "bold")).grid(row=1, column=0, columnspan=2, pady=(10, 5))
        
        ttk.Label(left_frame, text="Prompt:").grid(row=2, column=0, sticky=tk.W, pady=(5, 0))
        self.prompt_text = scrolledtext.ScrolledText(left_frame, height=4, width=40)
        self.prompt_text.grid(row=3, column=0, columnspan=2, pady=5, sticky=(tk.W, tk.E))
        self.prompt_text.insert(tk.END, "Create a dark ambient pad with swirling textures")
        
        # Example prompts dropdown
        ttk.Label(left_frame, text="Examples:").grid(row=4, column=0, sticky=tk.W)
        self.example_var = tk.StringVar()
        self.example_combo = ttk.Combobox(left_frame, textvariable=self.example_var, width=35)
        self.example_combo['values'] = (
            "Dark ambient horror atmosphere",
            "Classic 80s synthwave lead",
            "Aggressive dubstep wobble bass",
            "Ethereal space pad with shimmer",
            "Roland TB-303 acid bass",
            "Warm analog Juno-106 pad",
            "Glitchy IDM percussion",
            "Cathedral reverb with choir",
            "The sound of time freezing",
            "Color purple if it could sing"
        )
        self.example_combo.grid(row=5, column=0, columnspan=2, pady=5, sticky=(tk.W, tk.E))
        self.example_combo.bind('<<ComboboxSelected>>', self.load_example)
        
        self.generate_btn = ttk.Button(left_frame, text="Generate Preset", command=self.generate_preset)
        self.generate_btn.grid(row=6, column=0, columnspan=2, pady=10, sticky=(tk.W, tk.E))
        
        # --- Modification Section ---
        ttk.Label(left_frame, text="Modify Preset", font=("Arial", 12, "bold")).grid(row=7, column=0, columnspan=2, pady=(20, 5))
        
        ttk.Label(left_frame, text="Modification:").grid(row=8, column=0, sticky=tk.W, pady=(5, 0))
        self.mod_text = scrolledtext.ScrolledText(left_frame, height=3, width=40)
        self.mod_text.grid(row=9, column=0, columnspan=2, pady=5, sticky=(tk.W, tk.E))
        self.mod_text.insert(tk.END, "Make it darker and more aggressive")
        
        # Quick modification buttons
        mod_frame = ttk.Frame(left_frame)
        mod_frame.grid(row=10, column=0, columnspan=2, pady=5)
        
        ttk.Button(mod_frame, text="Darker", command=lambda: self.quick_mod("Make it darker and more ominous")).pack(side=tk.LEFT, padx=2)
        ttk.Button(mod_frame, text="Brighter", command=lambda: self.quick_mod("Make it brighter and more uplifting")).pack(side=tk.LEFT, padx=2)
        ttk.Button(mod_frame, text="Add Reverb", command=lambda: self.quick_mod("Add spacious reverb")).pack(side=tk.LEFT, padx=2)
        
        mod_frame2 = ttk.Frame(left_frame)
        mod_frame2.grid(row=11, column=0, columnspan=2, pady=5)
        
        ttk.Button(mod_frame2, text="Add Distortion", command=lambda: self.quick_mod("Add heavy distortion and bitcrushing")).pack(side=tk.LEFT, padx=2)
        ttk.Button(mod_frame2, text="Add Chorus", command=lambda: self.quick_mod("Add chorus and phaser")).pack(side=tk.LEFT, padx=2)
        ttk.Button(mod_frame2, text="Add Chaos", command=lambda: self.quick_mod("Add chaos generator")).pack(side=tk.LEFT, padx=2)
        
        self.modify_btn = ttk.Button(left_frame, text="Apply Modification", command=self.modify_preset)
        self.modify_btn.grid(row=12, column=0, columnspan=2, pady=10, sticky=(tk.W, tk.E))
        
        # --- Action Buttons ---
        ttk.Label(left_frame, text="Actions", font=("Arial", 12, "bold")).grid(row=13, column=0, columnspan=2, pady=(20, 5))
        
        action_frame = ttk.Frame(left_frame)
        action_frame.grid(row=14, column=0, columnspan=2, pady=5)
        
        ttk.Button(action_frame, text="Copy JSON", command=self.copy_json).pack(side=tk.LEFT, padx=2)
        ttk.Button(action_frame, text="Clear All", command=self.clear_all).pack(side=tk.LEFT, padx=2)
        ttk.Button(action_frame, text="Random Preset", command=self.random_preset).pack(side=tk.LEFT, padx=2)
        
        # === RIGHT PANEL: Preset Display ===
        right_frame = ttk.LabelFrame(main_frame, text="Current Preset", padding="10")
        right_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(5, 0))
        
        # Preset info
        info_frame = ttk.Frame(right_frame)
        info_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        
        ttk.Label(info_frame, text="Name:", font=("Arial", 10, "bold")).grid(row=0, column=0, sticky=tk.W)
        self.name_label = ttk.Label(info_frame, text="No preset loaded", font=("Arial", 11))
        self.name_label.grid(row=0, column=1, sticky=tk.W, padx=(10, 0))
        
        ttk.Label(info_frame, text="Vibe:", font=("Arial", 10, "bold")).grid(row=1, column=0, sticky=tk.W)
        self.vibe_label = ttk.Label(info_frame, text="", font=("Arial", 10))
        self.vibe_label.grid(row=1, column=1, sticky=tk.W, padx=(10, 0))
        
        # Engines display
        ttk.Label(right_frame, text="Active Engines:", font=("Arial", 11, "bold")).grid(row=1, column=0, sticky=tk.W, pady=(10, 5))
        
        # Create treeview for engines
        self.engine_tree = ttk.Treeview(right_frame, columns=("Mix", "Drive", "Tone", "Depth"), height=6)
        self.engine_tree.grid(row=2, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)
        
        # Configure columns
        self.engine_tree.heading("#0", text="Engine")
        self.engine_tree.heading("Mix", text="Mix %")
        self.engine_tree.heading("Drive", text="Drive %")
        self.engine_tree.heading("Tone", text="Tone %") 
        self.engine_tree.heading("Depth", text="Depth %")
        
        self.engine_tree.column("#0", width=200)
        self.engine_tree.column("Mix", width=80)
        self.engine_tree.column("Drive", width=80)
        self.engine_tree.column("Tone", width=80)
        self.engine_tree.column("Depth", width=80)
        
        # Parameters display
        ttk.Label(right_frame, text="Raw Parameters:", font=("Arial", 11, "bold")).grid(row=3, column=0, sticky=tk.W, pady=(10, 5))
        
        self.param_text = scrolledtext.ScrolledText(right_frame, height=15, width=60)
        self.param_text.grid(row=4, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)
        
        # === BOTTOM PANEL: Log Output ===
        log_frame = ttk.LabelFrame(main_frame, text="Activity Log", padding="10")
        log_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(10, 0))
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=8, width=100)
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure text tags for coloring
        self.log_text.tag_config("info", foreground="black")
        self.log_text.tag_config("success", foreground="green")
        self.log_text.tag_config("error", foreground="red")
        self.log_text.tag_config("warning", foreground="orange")
        
    def log(self, message: str, tag: str = "info"):
        """Add message to log"""
        self.log_text.insert(tk.END, f"{message}\n", tag)
        self.log_text.see(tk.END)
        
    def check_server_connection(self):
        """Check if server is running"""
        def check():
            try:
                response = requests.get(f"{self.api_base}/", timeout=2)
                if response.status_code == 200:
                    self.status_label.config(text="Server: Connected ✓", foreground="green")
                    self.log("Successfully connected to Trinity Pipeline server", "success")
                    return True
            except:
                pass
            
            self.status_label.config(text="Server: Not Connected ✗", foreground="red")
            self.log("Warning: Server not connected. Please ensure the AI server is running.", "warning")
            self.log("Run: python3 main.py", "warning")
            return False
                
        threading.Thread(target=check, daemon=True).start()
        
    def load_example(self, event):
        """Load example prompt"""
        self.prompt_text.delete(1.0, tk.END)
        self.prompt_text.insert(tk.END, self.example_var.get())
        
    def quick_mod(self, text: str):
        """Quick modification button"""
        self.mod_text.delete(1.0, tk.END)
        self.mod_text.insert(tk.END, text)
        
    def generate_preset(self):
        """Generate a new preset"""
        prompt = self.prompt_text.get(1.0, tk.END).strip()
        if not prompt:
            messagebox.showwarning("Warning", "Please enter a prompt")
            return
            
        self.log(f"Generating preset: \"{prompt}\"", "info")
        self.generate_btn.config(state="disabled", text="Generating...")
        
        def generate():
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
                    
                    self.log(f"✓ Generated preset '{self.current_preset.get('name')}' in {metadata.get('generation_time_seconds', 0):.1f}s", "success")
                    self.display_preset()
                else:
                    self.log(f"Failed to generate preset: {response.status_code}", "error")
                    
            except Exception as e:
                self.log(f"Error: {str(e)}", "error")
            finally:
                self.generate_btn.config(state="normal", text="Generate Preset")
                
        threading.Thread(target=generate, daemon=True).start()
        
    def modify_preset(self):
        """Modify current preset"""
        if not self.current_preset:
            messagebox.showwarning("Warning", "Please generate a preset first")
            return
            
        modification = self.mod_text.get(1.0, tk.END).strip()
        if not modification:
            messagebox.showwarning("Warning", "Please enter a modification")
            return
            
        self.log(f"Applying modification: \"{modification}\"", "info")
        self.modify_btn.config(state="disabled", text="Modifying...")
        
        def modify():
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
                        self.log(f"✓ {result.get('message', 'Modified successfully')}", "success")
                        self.display_preset()
                    else:
                        self.log(f"Modification failed: {result.get('message')}", "error")
                else:
                    self.log(f"Failed to modify preset: {response.status_code}", "error")
                    
            except Exception as e:
                self.log(f"Error: {str(e)}", "error")
            finally:
                self.modify_btn.config(state="normal", text="Apply Modification")
                
        threading.Thread(target=modify, daemon=True).start()
        
    def display_preset(self):
        """Display current preset details"""
        if not self.current_preset:
            return
            
        # Update name and vibe
        self.name_label.config(text=self.current_preset.get("name", "Unknown"))
        self.vibe_label.config(text=self.current_preset.get("vibe", ""))
        
        # Clear and update engines
        for item in self.engine_tree.get_children():
            self.engine_tree.delete(item)
            
        parameters = self.current_preset.get("parameters", {})
        
        for slot in range(1, 7):
            engine_id = parameters.get(f'slot{slot}_engine', 0)
            if engine_id > 0 and parameters.get(f'slot{slot}_bypass', 0) < 0.5:
                engine_name = ENGINE_MAPPING.get(engine_id, f"Unknown ({engine_id})")
                mix = int(parameters.get(f'slot{slot}_mix', 0.5) * 100)
                drive = int(parameters.get(f'slot{slot}_param1', 0) * 100)
                tone = int(parameters.get(f'slot{slot}_param2', 0) * 100)
                depth = int(parameters.get(f'slot{slot}_param3', 0) * 100)
                
                self.engine_tree.insert("", tk.END, text=f"Slot {slot}: {engine_name}",
                                       values=(mix, drive, tone, depth))
        
        # Update raw parameters
        self.param_text.delete(1.0, tk.END)
        # Format parameters nicely
        param_str = json.dumps(parameters, indent=2)
        # Truncate long decimals for readability
        import re
        param_str = re.sub(r'(\d+\.\d{3})\d+', r'\1', param_str)
        self.param_text.insert(tk.END, param_str)
        
    def copy_json(self):
        """Copy current preset JSON to clipboard"""
        if not self.current_preset:
            messagebox.showwarning("Warning", "No preset to copy")
            return
            
        json_str = json.dumps(self.current_preset, indent=2)
        self.root.clipboard_clear()
        self.root.clipboard_append(json_str)
        self.log("Preset JSON copied to clipboard", "success")
        
    def clear_all(self):
        """Clear all fields"""
        self.prompt_text.delete(1.0, tk.END)
        self.mod_text.delete(1.0, tk.END)
        self.current_preset = None
        self.name_label.config(text="No preset loaded")
        self.vibe_label.config(text="")
        for item in self.engine_tree.get_children():
            self.engine_tree.delete(item)
        self.param_text.delete(1.0, tk.END)
        self.log_text.delete(1.0, tk.END)
        self.log("All fields cleared", "info")
        
    def random_preset(self):
        """Generate random creative preset"""
        import random
        prompts = [
            "The sound of a black hole eating a star",
            "1980s shopping mall at 3am with vaporwave echoes",
            "Underwater cathedral with mermaids singing",
            "Robot having an existential crisis",
            "The color blue if it could scream",
            "Time traveling through honey",
            "Digital rain in a cyberpunk city",
            "Ancient alien technology powering up",
            "The feeling of nostalgia for the future",
            "Glitchy transmission from parallel universe"
        ]
        
        self.prompt_text.delete(1.0, tk.END)
        self.prompt_text.insert(tk.END, random.choice(prompts))
        self.generate_preset()
        
    def run(self):
        """Start the application"""
        self.log("Chimera Phoenix Standalone Tester Started", "info")
        self.log("Ready to generate and modify presets using Trinity Pipeline", "info")
        self.root.mainloop()

if __name__ == "__main__":
    app = ChimeraPluginTester()
    app.run()