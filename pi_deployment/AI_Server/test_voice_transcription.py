#!/usr/bin/env python3
"""
Test voice transcription endpoint for Trinity pipeline
"""

import requests
import os
import tempfile
import wave
import numpy as np

def create_test_audio():
    """Create a simple test WAV file with a tone"""
    sample_rate = 16000
    duration = 2  # seconds
    frequency = 440  # A4 note
    
    # Generate a simple sine wave
    t = np.linspace(0, duration, sample_rate * duration)
    audio = 0.5 * np.sin(2 * np.pi * frequency * t)
    
    # Convert to 16-bit PCM
    audio_int16 = (audio * 32767).astype(np.int16)
    
    # Create temporary WAV file
    with tempfile.NamedTemporaryFile(delete=False, suffix='.wav') as tmp_file:
        with wave.open(tmp_file.name, 'wb') as wav_file:
            wav_file.setnchannels(1)  # Mono
            wav_file.setsampwidth(2)   # 16-bit
            wav_file.setframerate(sample_rate)
            wav_file.writeframes(audio_int16.tobytes())
        
        return tmp_file.name

def test_transcription_endpoint():
    """Test the /transcribe endpoint"""
    
    print("Testing Trinity Voice Transcription")
    print("="*50)
    
    # Check server is running
    try:
        health = requests.get("http://localhost:8000/health", timeout=2)
        if health.status_code != 200:
            print("❌ Server not healthy")
            return
        print("✅ Server is running")
    except:
        print("❌ Server not running at http://localhost:8000")
        print("   Start it with: ./start_trinity.sh")
        return
    
    # Check if OpenAI key is set
    if not os.getenv("OPENAI_API_KEY"):
        print("⚠️  WARNING: OPENAI_API_KEY not set")
        print("   Transcription will fail without it")
    
    # Test with a real audio file if available
    test_files = [
        "test_audio.wav",
        "test_audio.webm",
        "test_audio.mp3"
    ]
    
    audio_file = None
    for file in test_files:
        if os.path.exists(file):
            audio_file = file
            break
    
    if not audio_file:
        print("\n📝 No test audio found, creating synthetic test file...")
        audio_file = create_test_audio()
        print(f"   Created: {audio_file}")
    
    print(f"\n🎤 Testing transcription with: {audio_file}")
    
    # Send to transcription endpoint
    try:
        with open(audio_file, 'rb') as f:
            files = {'audio': ('test.wav', f, 'audio/wav')}
            response = requests.post(
                "http://localhost:8000/transcribe",
                files=files,
                timeout=30
            )
        
        if response.status_code == 200:
            result = response.json()
            if result.get('success'):
                print(f"✅ Transcription successful!")
                print(f"   Text: '{result.get('text', '')}'")
            else:
                print(f"❌ Transcription failed: {result.get('message', 'Unknown error')}")
        else:
            print(f"❌ Server returned status: {response.status_code}")
            print(f"   Response: {response.text}")
    except Exception as e:
        print(f"❌ Error: {e}")
    
    # Clean up synthetic file
    if audio_file.startswith('/tmp'):
        os.unlink(audio_file)
        print(f"\n🧹 Cleaned up temporary file")

def test_voice_to_preset():
    """Test full voice to preset flow"""
    
    print("\n" + "="*50)
    print("Testing Voice → Text → Preset Flow")
    print("="*50)
    
    # Simulate the flow
    test_prompts = [
        "warm vintage guitar tone",
        "aggressive metal distortion with gate",
        "ethereal ambient pad"
    ]
    
    for prompt in test_prompts:
        print(f"\n📝 Simulating voice prompt: '{prompt}'")
        
        # In real usage, this would come from transcription
        # Now test preset generation
        try:
            response = requests.post(
                "http://localhost:8000/generate",
                json={
                    "prompt": prompt,
                    "intensity": 0.5,
                    "complexity": 3
                },
                timeout=10
            )
            
            if response.status_code == 200:
                result = response.json()
                if result.get('success'):
                    preset = result.get('preset', {})
                    print(f"✅ Generated preset: {preset.get('name', 'Unnamed')}")
                else:
                    print(f"❌ Generation failed: {result.get('message', 'Unknown')}")
            else:
                print(f"❌ Server error: {response.status_code}")
        except Exception as e:
            print(f"❌ Error: {e}")

def main():
    print("🎙️ PHOENIX VOICE INPUT TEST")
    print("="*50)
    
    # Test transcription
    test_transcription_endpoint()
    
    # Test full flow
    test_voice_to_preset()
    
    print("\n" + "="*50)
    print("Voice Capabilities Summary:")
    print("  • Server endpoint: POST /transcribe")
    print("  • Accepts: WAV, WEBM, MP3 audio files")
    print("  • Returns: Transcribed text via Whisper API")
    print("  • Plugin: Microphone button next to Trinity text box")
    print("  • Flow: Record → Transcribe → Fill text box → Generate preset")
    print("="*50)

if __name__ == "__main__":
    main()