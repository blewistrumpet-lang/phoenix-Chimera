#!/usr/bin/env python3
"""
DEFINITIVE PROOF TEST FOR ENGINE MAPPING
"""
import requests
import json

# Make a request to the server
response = requests.post(
    "http://localhost:8000/generate",
    json={"prompt": "test preset for verification"}
)

data = response.json()
preset = data['preset']

print("="*80)
print("DEFINITIVE ENGINE MAPPING PROOF")
print("="*80)

# Get the engine values from the response
print("\n🔴 ENGINE VALUES IN FINAL SERVER RESPONSE:")
for i in range(1, 7):
    key = f'slot{i}_engine'
    value = preset['parameters'].get(key, 0)
    if value != 0:
        print(f"  {key} = {value}")

print("\n📊 WHAT THESE VALUES MEAN:")
print("  If mapping is WORKING: Values should be 44, 45, 50 (choice indices)")
print("  If mapping is BROKEN: Values would be 41, 40, 52 (engine IDs)")

# Check what we got
slot1 = preset['parameters'].get('slot1_engine', 0)
slot2 = preset['parameters'].get('slot2_engine', 0) 
slot3 = preset['parameters'].get('slot3_engine', 0)

print("\n✅ VERIFICATION:")
if slot1 == 44 and slot2 == 45 and slot3 == 50:
    print("  ✅✅✅ ENGINE MAPPING IS WORKING CORRECTLY!")
    print("  The server is returning CHOICE INDICES (44, 45, 50)")
    print("  These will select the correct engines in the plugin dropdown")
elif slot1 == 41 and slot2 == 40 and slot3 == 52:
    print("  ❌❌❌ ENGINE MAPPING IS BROKEN!")
    print("  The server is returning ENGINE IDs (41, 40, 52)")
    print("  The plugin will interpret these as wrong choice indices!")
else:
    print(f"  ⚠️ UNEXPECTED VALUES: {slot1}, {slot2}, {slot3}")
    
print("\n🎯 ENGINE MAPPING TABLE:")
print("  Engine ID 41 (Chaos Generator)  -> Choice Index 44")
print("  Engine ID 40 (Buffer Repeat)    -> Choice Index 45")
print("  Engine ID 52 (Resonant Chorus)  -> Choice Index 50")

print("\n" + "="*80)