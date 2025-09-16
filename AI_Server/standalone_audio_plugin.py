#!/usr/bin/env python3
"""
Chimera Phoenix - Standalone Audio Plugin
Full audio processing plugin with Trinity Pipeline integration
"""

import numpy as np
import soundfile as sf
import pyaudio
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import threading
import queue
import json
import requests
from typing import Dict, Any, Optional, List
import time
from engine_mapping_correct import ENGINE_MAPPING

class AudioEngine:
    """Simple audio processing engine for demonstration"""
    
    def __init__(self, sample_rate=44100, buffer_size=512):
        self.sample_rate = sample_rate
        self.buffer_size = buffer_size
        self.is_processing = False
        self.audio_queue = queue.Queue()
        
        # Audio parameters from preset
        self.parameters = {}
        self.dry_wet_mix = 0.5
        self.input_gain = 1.0
        self.output_gain = 1.0
        
        # PyAudio setup
        self.pa = pyaudio.PyAudio()
        self.stream = None
        
    def apply_preset(self, preset: Dict[str, Any]):
        """Apply preset parameters to audio engine"""
        self.parameters = preset.get('parameters', {})
        self.dry_wet_mix = self.parameters.get('dry_wet_mix', 0.5)
        self.input_gain = self.parameters.get('input_gain', 0.5) * 2
        self.output_gain = self.parameters.get('output_gain', 0.5) * 2
        
    def process_audio(self, input_data: np.ndarray) -> np.ndarray:
        """Process audio through the engine chain"""
        # Apply input gain
        processed = input_data * self.input_gain
        
        # Simple effect simulation based on active engines
        for slot in range(1, 7):
            engine_id = self.parameters.get(f'slot{slot}_engine', 0)
            if engine_id > 0 and self.parameters.get(f'slot{slot}_bypass', 0) < 0.5:
                mix = self.parameters.get(f'slot{slot}_mix', 0.5)
                drive = self.parameters.get(f'slot{slot}_param1', 0.5)
                tone = self.parameters.get(f'slot{slot}_param2', 0.5)
                
                # Simplified effect processing based on engine type
                engine_name = ENGINE_MAPPING.get(engine_id, "").lower()
                
                if "reverb" in engine_name:
                    # Simple reverb simulation (delay + feedback)
                    delay_samples = int(0.05 * self.sample_rate)
                    if len(processed) > delay_samples:
                        delayed = np.concatenate([np.zeros(delay_samples), processed[:-delay_samples]])
                        processed = processed + delayed * mix * 0.5
                        
                elif "delay" in engine_name:
                    # Simple delay
                    delay_time = 0.1 + tone * 0.4  # 100-500ms
                    delay_samples = int(delay_time * self.sample_rate)
                    if len(processed) > delay_samples:
                        delayed = np.concatenate([np.zeros(delay_samples), processed[:-delay_samples]])
                        processed = processed + delayed * mix
                        
                elif "distortion" in engine_name or "overdrive" in engine_name:
                    # Simple distortion
                    processed = np.tanh(processed * (1 + drive * 5)) * mix + processed * (1 - mix)
                    
                elif "filter" in engine_name:
                    # Simple low-pass filter simulation
                    cutoff = 0.1 + tone * 0.8
                    processed = processed * cutoff + processed * (1 - cutoff)
                    
                elif "chorus" in engine_name:
                    # Simple chorus (pitch modulation)
                    mod_depth = mix * 0.002
                    mod_rate = 0.5 + drive * 2
                    t = np.arange(len(processed)) / self.sample_rate
                    modulation = np.sin(2 * np.pi * mod_rate * t) * mod_depth
                    processed = processed + processed * modulation
                    
        # Apply dry/wet mix
        wet = processed
        dry = input_data
        output = dry * (1 - self.dry_wet_mix) + wet * self.dry_wet_mix
        
        # Apply output gain and limiting
        output = output * self.output_gain
        output = np.clip(output, -1.0, 1.0)
        
        return output
        
    def start_stream(self, input_device=None, output_device=None):
        """Start audio stream"""
        if self.stream:
            self.stop_stream()
            
        def callback(in_data, frame_count, time_info, status):
            # Convert input bytes to numpy array
            audio_data = np.frombuffer(in_data, dtype=np.float32)
            
            # Process audio
            if self.is_processing:
                processed = self.process_audio(audio_data)
            else:
                processed = audio_data
                
            # Convert back to bytes
            out_data = processed.astype(np.float32).tobytes()
            
            # Store for visualization
            if not self.audio_queue.full():
                try:
                    self.audio_queue.put_nowait(np.max(np.abs(processed)))
                except:
                    pass
                    
            return (out_data, pyaudio.paContinue)
            
        self.stream = self.pa.open(
            format=pyaudio.paFloat32,
            channels=1,
            rate=self.sample_rate,
            input=True,
            output=True,
            input_device_index=input_device,
            output_device_index=output_device,
            frames_per_buffer=self.buffer_size,
            stream_callback=callback
        )
        
        self.stream.start_stream()
        self.is_processing = True
        
    def stop_stream(self):
        """Stop audio stream"""
        self.is_processing = False
        if self.stream:
            self.stream.stop_stream()
            self.stream.close()
            self.stream = None
            
    def cleanup(self):
        """Cleanup audio resources"""
        self.stop_stream()
        self.pa.terminate()


class ChimeraStandalonePlugin:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Chimera Phoenix - Standalone Audio Plugin")
        self.root.geometry("1400x900")
        
        # Core components
        self.audio_engine = AudioEngine()
        self.current_preset: Optional[Dict[str, Any]] = None
        self.api_base = "http://localhost:8000"
        
        # UI Setup
        self.setup_ui()
        
        # Start audio processing
        self.setup_audio()
        
        # Check server
        self.check_server_connection()
        
        # Start meter update
        self.update_meters()
        
    def setup_ui(self):
        """Create the plugin interface"""
        # Main container
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=1)
        main_frame.rowconfigure(0, weight=1)
        
        # === TOP BAR ===
        top_frame = ttk.Frame(main_frame)
        top_frame.grid(row=0, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=(0, 10))
        
        # Logo/Title
        title_label = ttk.Label(top_frame, text="CHIMERA PHOENIX", font=("Arial", 18, "bold"))
        title_label.pack(side=tk.LEFT, padx=(0, 20))
        
        # Preset name display
        self.preset_name_label = ttk.Label(top_frame, text="No Preset Loaded", font=("Arial", 14))
        self.preset_name_label.pack(side=tk.LEFT, padx=20)
        
        # Server status
        self.status_label = ttk.Label(top_frame, text="Server: Checking...", foreground="gray")
        self.status_label.pack(side=tk.RIGHT, padx=10)
        
        # === LEFT PANEL: Preset Generation ===
        left_frame = ttk.LabelFrame(main_frame, text="Trinity Pipeline", padding="10")
        left_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(0, 5))
        
        # Generation section
        ttk.Label(left_frame, text="Generate Preset:", font=("Arial", 11, "bold")).grid(row=0, column=0, sticky=tk.W)
        
        self.prompt_text = scrolledtext.ScrolledText(left_frame, height=3, width=35)
        self.prompt_text.grid(row=1, column=0, pady=5, sticky=(tk.W, tk.E))
        self.prompt_text.insert(tk.END, "Dark ambient pad with metallic textures")
        
        # Quick presets
        quick_frame = ttk.Frame(left_frame)
        quick_frame.grid(row=2, column=0, pady=5)
        
        ttk.Button(quick_frame, text="Ambient", 
                  command=lambda: self.quick_prompt("Ethereal ambient pad with long reverb")).pack(side=tk.LEFT, padx=2)
        ttk.Button(quick_frame, text="Bass", 
                  command=lambda: self.quick_prompt("Heavy dubstep bass with sub frequencies")).pack(side=tk.LEFT, padx=2)
        ttk.Button(quick_frame, text="Lead", 
                  command=lambda: self.quick_prompt("Screaming lead synth with filter sweeps")).pack(side=tk.LEFT, padx=2)
        
        self.generate_btn = ttk.Button(left_frame, text="Generate", command=self.generate_preset)
        self.generate_btn.grid(row=3, column=0, pady=10, sticky=(tk.W, tk.E))
        
        # Modification section
        ttk.Separator(left_frame, orient='horizontal').grid(row=4, column=0, sticky=(tk.W, tk.E), pady=10)
        
        ttk.Label(left_frame, text="Modify Preset:", font=("Arial", 11, "bold")).grid(row=5, column=0, sticky=tk.W)
        
        self.mod_text = scrolledtext.ScrolledText(left_frame, height=2, width=35)
        self.mod_text.grid(row=6, column=0, pady=5, sticky=(tk.W, tk.E))
        self.mod_text.insert(tk.END, "Make it more aggressive")
        
        # Modification buttons
        mod_frame1 = ttk.Frame(left_frame)
        mod_frame1.grid(row=7, column=0, pady=5)
        
        ttk.Button(mod_frame1, text="Darker", 
                  command=lambda: self.quick_mod("Make it darker")).pack(side=tk.LEFT, padx=2)
        ttk.Button(mod_frame1, text="Brighter", 
                  command=lambda: self.quick_mod("Make it brighter")).pack(side=tk.LEFT, padx=2)
        ttk.Button(mod_frame1, text="Warmer", 
                  command=lambda: self.quick_mod("Make it warmer")).pack(side=tk.LEFT, padx=2)
        
        mod_frame2 = ttk.Frame(left_frame)
        mod_frame2.grid(row=8, column=0, pady=5)
        
        ttk.Button(mod_frame2, text="+Reverb", 
                  command=lambda: self.quick_mod("Add reverb")).pack(side=tk.LEFT, padx=2)
        ttk.Button(mod_frame2, text="+Delay", 
                  command=lambda: self.quick_mod("Add delay")).pack(side=tk.LEFT, padx=2)
        ttk.Button(mod_frame2, text="+Distortion", 
                  command=lambda: self.quick_mod("Add distortion")).pack(side=tk.LEFT, padx=2)
        
        self.modify_btn = ttk.Button(left_frame, text="Modify", command=self.modify_preset)
        self.modify_btn.grid(row=9, column=0, pady=10, sticky=(tk.W, tk.E))
        
        # File operations
        ttk.Separator(left_frame, orient='horizontal').grid(row=10, column=0, sticky=(tk.W, tk.E), pady=10)
        
        file_frame = ttk.Frame(left_frame)
        file_frame.grid(row=11, column=0, pady=5)
        
        ttk.Button(file_frame, text="Load Audio", command=self.load_audio_file).pack(side=tk.LEFT, padx=2)
        ttk.Button(file_frame, text="Save Preset", command=self.save_preset).pack(side=tk.LEFT, padx=2)
        ttk.Button(file_frame, text="Load Preset", command=self.load_preset).pack(side=tk.LEFT, padx=2)
        
        # === CENTER PANEL: Engine Display ===
        center_frame = ttk.LabelFrame(main_frame, text="Active Engines", padding="10")
        center_frame.grid(row=1, column=1, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5)
        
        # Engine slots display
        self.engine_frames = []
        for slot in range(6):
            frame = ttk.LabelFrame(center_frame, text=f"Slot {slot+1}", padding="5")
            frame.grid(row=slot//2, column=slot%2, padx=5, pady=5, sticky=(tk.W, tk.E))
            
            # Engine name
            name_label = ttk.Label(frame, text="Empty", font=("Arial", 10))
            name_label.grid(row=0, column=0, columnspan=2, sticky=tk.W)
            
            # Mix slider
            ttk.Label(frame, text="Mix:").grid(row=1, column=0, sticky=tk.W)
            mix_var = tk.DoubleVar(value=0)
            mix_slider = ttk.Scale(frame, from_=0, to=100, orient=tk.HORIZONTAL, 
                                  variable=mix_var, length=150)
            mix_slider.grid(row=1, column=1)
            mix_label = ttk.Label(frame, text="0%")
            mix_label.grid(row=1, column=2)
            
            # Drive slider  
            ttk.Label(frame, text="Drive:").grid(row=2, column=0, sticky=tk.W)
            drive_var = tk.DoubleVar(value=0)
            drive_slider = ttk.Scale(frame, from_=0, to=100, orient=tk.HORIZONTAL,
                                    variable=drive_var, length=150)
            drive_slider.grid(row=2, column=1)
            drive_label = ttk.Label(frame, text="0%")
            drive_label.grid(row=2, column=2)
            
            # Store references
            self.engine_frames.append({
                'frame': frame,
                'name': name_label,
                'mix_var': mix_var,
                'mix_slider': mix_slider,
                'mix_label': mix_label,
                'drive_var': drive_var,
                'drive_slider': drive_slider,
                'drive_label': drive_label
            })
            
            # Bind slider updates
            mix_slider.configure(command=lambda v, s=slot: self.update_param(s, 'mix', float(v)))
            drive_slider.configure(command=lambda v, s=slot: self.update_param(s, 'drive', float(v)))
            
        # === RIGHT PANEL: Audio Controls ===
        right_frame = ttk.LabelFrame(main_frame, text="Audio Controls", padding="10")
        right_frame.grid(row=1, column=2, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(5, 0))
        
        # Master controls
        ttk.Label(right_frame, text="Master Controls", font=("Arial", 11, "bold")).grid(row=0, column=0, columnspan=2)
        
        # Input gain
        ttk.Label(right_frame, text="Input:").grid(row=1, column=0, sticky=tk.W)
        self.input_gain_var = tk.DoubleVar(value=50)
        input_slider = ttk.Scale(right_frame, from_=0, to=100, orient=tk.HORIZONTAL,
                                variable=self.input_gain_var, length=150)
        input_slider.grid(row=1, column=1)
        self.input_label = ttk.Label(right_frame, text="50%")
        self.input_label.grid(row=1, column=2)
        input_slider.configure(command=self.update_input_gain)
        
        # Output gain
        ttk.Label(right_frame, text="Output:").grid(row=2, column=0, sticky=tk.W)
        self.output_gain_var = tk.DoubleVar(value=50)
        output_slider = ttk.Scale(right_frame, from_=0, to=100, orient=tk.HORIZONTAL,
                                 variable=self.output_gain_var, length=150)
        output_slider.grid(row=2, column=1)
        self.output_label = ttk.Label(right_frame, text="50%")
        self.output_label.grid(row=2, column=2)
        output_slider.configure(command=self.update_output_gain)
        
        # Dry/Wet
        ttk.Label(right_frame, text="Dry/Wet:").grid(row=3, column=0, sticky=tk.W)
        self.dry_wet_var = tk.DoubleVar(value=50)
        dry_wet_slider = ttk.Scale(right_frame, from_=0, to=100, orient=tk.HORIZONTAL,
                                   variable=self.dry_wet_var, length=150)
        dry_wet_slider.grid(row=3, column=1)
        self.dry_wet_label = ttk.Label(right_frame, text="50%")
        self.dry_wet_label.grid(row=3, column=2)
        dry_wet_slider.configure(command=self.update_dry_wet)
        
        # Level meters
        ttk.Separator(right_frame, orient='horizontal').grid(row=4, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=10)
        
        ttk.Label(right_frame, text="Level Meters", font=("Arial", 11, "bold")).grid(row=5, column=0, columnspan=2)
        
        # Input meter
        ttk.Label(right_frame, text="Input:").grid(row=6, column=0, sticky=tk.W)
        self.input_meter = ttk.Progressbar(right_frame, length=150, mode='determinate')
        self.input_meter.grid(row=6, column=1, columnspan=2)
        
        # Output meter  
        ttk.Label(right_frame, text="Output:").grid(row=7, column=0, sticky=tk.W)
        self.output_meter = ttk.Progressbar(right_frame, length=150, mode='determinate')
        self.output_meter.grid(row=7, column=1, columnspan=2)
        
        # Audio controls
        ttk.Separator(right_frame, orient='horizontal').grid(row=8, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=10)
        
        audio_frame = ttk.Frame(right_frame)
        audio_frame.grid(row=9, column=0, columnspan=3, pady=10)
        
        self.bypass_btn = ttk.Button(audio_frame, text="Bypass", command=self.toggle_bypass)
        self.bypass_btn.pack(side=tk.LEFT, padx=5)
        
        self.panic_btn = ttk.Button(audio_frame, text="Panic", command=self.panic)
        self.panic_btn.pack(side=tk.LEFT, padx=5)
        
        # Status bar
        status_frame = ttk.Frame(main_frame)
        status_frame.grid(row=2, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=(10, 0))
        
        self.status_text = ttk.Label(status_frame, text="Ready", relief=tk.SUNKEN, anchor=tk.W)
        self.status_text.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        self.cpu_label = ttk.Label(status_frame, text="CPU: 0%", relief=tk.SUNKEN, width=10)
        self.cpu_label.pack(side=tk.RIGHT, padx=(5, 0))
        
    def setup_audio(self):
        """Initialize audio system"""
        try:
            self.audio_engine.start_stream()
            self.set_status("Audio engine started")
        except Exception as e:
            self.set_status(f"Audio error: {str(e)}", error=True)
            messagebox.showerror("Audio Error", f"Failed to start audio: {str(e)}")
            
    def check_server_connection(self):
        """Check Trinity Pipeline server"""
        def check():
            try:
                response = requests.get(f"{self.api_base}/", timeout=2)
                if response.status_code == 200:
                    self.status_label.config(text="Server: Connected ✓", foreground="green")
                    self.set_status("Connected to Trinity Pipeline")
                    return
            except:
                pass
            
            self.status_label.config(text="Server: Not Connected ✗", foreground="red")
            self.set_status("Warning: Trinity server not connected", error=True)
            
        threading.Thread(target=check, daemon=True).start()
        
    def generate_preset(self):
        """Generate new preset"""
        prompt = self.prompt_text.get(1.0, tk.END).strip()
        if not prompt:
            messagebox.showwarning("Warning", "Please enter a prompt")
            return
            
        self.set_status(f"Generating: {prompt[:50]}...")
        self.generate_btn.config(state="disabled")
        
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
                    
                    self.root.after(0, self.apply_preset)
                    self.root.after(0, lambda: self.set_status(f"Generated: {self.current_preset.get('name')}"))
                else:
                    self.root.after(0, lambda: self.set_status("Generation failed", error=True))
                    
            except Exception as e:
                self.root.after(0, lambda: self.set_status(f"Error: {str(e)}", error=True))
            finally:
                self.root.after(0, lambda: self.generate_btn.config(state="normal"))
                
        threading.Thread(target=generate, daemon=True).start()
        
    def modify_preset(self):
        """Modify current preset"""
        if not self.current_preset:
            messagebox.showwarning("Warning", "No preset loaded")
            return
            
        modification = self.mod_text.get(1.0, tk.END).strip()
        if not modification:
            messagebox.showwarning("Warning", "Please enter a modification")
            return
            
        self.set_status(f"Modifying: {modification[:50]}...")
        self.modify_btn.config(state="disabled")
        
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
                        self.root.after(0, self.apply_preset)
                        self.root.after(0, lambda: self.set_status(f"Modified: {result.get('message')}"))
                    else:
                        self.root.after(0, lambda: self.set_status("Modification failed", error=True))
                else:
                    self.root.after(0, lambda: self.set_status("Modification failed", error=True))
                    
            except Exception as e:
                self.root.after(0, lambda: self.set_status(f"Error: {str(e)}", error=True))
            finally:
                self.root.after(0, lambda: self.modify_btn.config(state="normal"))
                
        threading.Thread(target=modify, daemon=True).start()
        
    def apply_preset(self):
        """Apply preset to audio engine and UI"""
        if not self.current_preset:
            return
            
        # Update name
        self.preset_name_label.config(text=self.current_preset.get('name', 'Unknown'))
        
        # Apply to audio engine
        self.audio_engine.apply_preset(self.current_preset)
        
        # Update UI
        parameters = self.current_preset.get('parameters', {})
        
        # Update engine displays
        for slot in range(6):
            engine_id = parameters.get(f'slot{slot+1}_engine', 0)
            
            if engine_id > 0 and parameters.get(f'slot{slot+1}_bypass', 0) < 0.5:
                engine_name = ENGINE_MAPPING.get(engine_id, f"Unknown ({engine_id})")
                mix = parameters.get(f'slot{slot+1}_mix', 0.5) * 100
                drive = parameters.get(f'slot{slot+1}_param1', 0) * 100
                
                self.engine_frames[slot]['frame'].config(text=f"Slot {slot+1}: Active")
                self.engine_frames[slot]['name'].config(text=engine_name[:25])
                self.engine_frames[slot]['mix_var'].set(mix)
                self.engine_frames[slot]['mix_label'].config(text=f"{mix:.0f}%")
                self.engine_frames[slot]['drive_var'].set(drive)
                self.engine_frames[slot]['drive_label'].config(text=f"{drive:.0f}%")
            else:
                self.engine_frames[slot]['frame'].config(text=f"Slot {slot+1}: Empty")
                self.engine_frames[slot]['name'].config(text="Empty")
                self.engine_frames[slot]['mix_var'].set(0)
                self.engine_frames[slot]['mix_label'].config(text="0%")
                self.engine_frames[slot]['drive_var'].set(0)
                self.engine_frames[slot]['drive_label'].config(text="0%")
                
        # Update master controls
        self.input_gain_var.set(parameters.get('input_gain', 0.5) * 100)
        self.output_gain_var.set(parameters.get('output_gain', 0.5) * 100)
        self.dry_wet_var.set(parameters.get('dry_wet_mix', 0.5) * 100)
        
        self.update_input_gain(self.input_gain_var.get())
        self.update_output_gain(self.output_gain_var.get())
        self.update_dry_wet(self.dry_wet_var.get())
        
    def update_param(self, slot: int, param: str, value: float):
        """Update parameter value"""
        if self.current_preset and self.current_preset.get('parameters'):
            if param == 'mix':
                key = f'slot{slot+1}_mix'
                self.current_preset['parameters'][key] = value / 100
                self.engine_frames[slot]['mix_label'].config(text=f"{value:.0f}%")
            elif param == 'drive':
                key = f'slot{slot+1}_param1'
                self.current_preset['parameters'][key] = value / 100
                self.engine_frames[slot]['drive_label'].config(text=f"{value:.0f}%")
                
            # Apply to audio engine
            self.audio_engine.apply_preset(self.current_preset)
            
    def update_input_gain(self, value):
        """Update input gain"""
        value = float(value)
        self.input_label.config(text=f"{value:.0f}%")
        self.audio_engine.input_gain = value / 50  # 0-2 range
        if self.current_preset and self.current_preset.get('parameters'):
            self.current_preset['parameters']['input_gain'] = value / 100
            
    def update_output_gain(self, value):
        """Update output gain"""
        value = float(value)
        self.output_label.config(text=f"{value:.0f}%")
        self.audio_engine.output_gain = value / 50  # 0-2 range
        if self.current_preset and self.current_preset.get('parameters'):
            self.current_preset['parameters']['output_gain'] = value / 100
            
    def update_dry_wet(self, value):
        """Update dry/wet mix"""
        value = float(value)
        self.dry_wet_label.config(text=f"{value:.0f}%")
        self.audio_engine.dry_wet_mix = value / 100
        if self.current_preset and self.current_preset.get('parameters'):
            self.current_preset['parameters']['dry_wet_mix'] = value / 100
            
    def toggle_bypass(self):
        """Toggle bypass mode"""
        self.audio_engine.is_processing = not self.audio_engine.is_processing
        if self.audio_engine.is_processing:
            self.bypass_btn.config(text="Bypass")
            self.set_status("Processing enabled")
        else:
            self.bypass_btn.config(text="Active")
            self.set_status("Bypassed")
            
    def panic(self):
        """Panic button - stop all audio"""
        self.audio_engine.stop_stream()
        self.set_status("Audio stopped - Restart required")
        
    def update_meters(self):
        """Update level meters"""
        try:
            if not self.audio_engine.audio_queue.empty():
                level = self.audio_engine.audio_queue.get() * 100
                self.output_meter['value'] = min(100, level)
                self.input_meter['value'] = min(100, level * 0.8)  # Simulated input
                
                # Clear queue
                while not self.audio_engine.audio_queue.empty():
                    self.audio_engine.audio_queue.get()
                    
        except:
            pass
            
        # Schedule next update
        self.root.after(50, self.update_meters)
        
    def quick_prompt(self, text: str):
        """Quick prompt button"""
        self.prompt_text.delete(1.0, tk.END)
        self.prompt_text.insert(tk.END, text)
        
    def quick_mod(self, text: str):
        """Quick modification button"""
        self.mod_text.delete(1.0, tk.END)
        self.mod_text.insert(tk.END, text)
        
    def load_audio_file(self):
        """Load audio file for processing"""
        filename = filedialog.askopenfilename(
            title="Load Audio File",
            filetypes=[("Audio Files", "*.wav *.mp3 *.aiff"), ("All Files", "*.*")]
        )
        if filename:
            self.set_status(f"Loaded: {filename}")
            # Note: Full audio file processing would require additional implementation
            
    def save_preset(self):
        """Save current preset"""
        if not self.current_preset:
            messagebox.showwarning("Warning", "No preset to save")
            return
            
        filename = filedialog.asksaveasfilename(
            title="Save Preset",
            defaultextension=".json",
            filetypes=[("JSON Files", "*.json"), ("All Files", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'w') as f:
                    json.dump(self.current_preset, f, indent=2)
                self.set_status(f"Saved: {filename}")
            except Exception as e:
                self.set_status(f"Save failed: {str(e)}", error=True)
                
    def load_preset(self):
        """Load preset from file"""
        filename = filedialog.askopenfilename(
            title="Load Preset",
            filetypes=[("JSON Files", "*.json"), ("All Files", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'r') as f:
                    self.current_preset = json.load(f)
                self.apply_preset()
                self.set_status(f"Loaded: {filename}")
            except Exception as e:
                self.set_status(f"Load failed: {str(e)}", error=True)
                
    def set_status(self, message: str, error: bool = False):
        """Update status bar"""
        self.status_text.config(text=message)
        if error:
            self.status_text.config(foreground="red")
        else:
            self.status_text.config(foreground="black")
            
    def cleanup(self):
        """Cleanup on exit"""
        self.audio_engine.cleanup()
        
    def run(self):
        """Run the plugin"""
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.root.mainloop()
        
    def on_closing(self):
        """Handle window closing"""
        self.cleanup()
        self.root.destroy()

if __name__ == "__main__":
    plugin = ChimeraStandalonePlugin()
    plugin.run()