import json

# Test if the response matches TrinityProtocol expectations
response = {
    "success": True,
    "type": "preset",
    "message": "Generated: Test Preset",
    "data": {
        "preset": {
            "name": "Test Preset",
            "slots": []
        }
    }
}

# The plugin checks:
# 1. response.data exists and is an object
# 2. response.data.preset exists

print("Response structure:")
print(json.dumps(response, indent=2))

print("\nChecking TrinityProtocol conditions:")
print(f"Has 'data' property: {'data' in response}")
print(f"'data' is dict: {isinstance(response.get('data'), dict)}")
print(f"Has 'data.preset': {'preset' in response.get('data', {})}")

# What the plugin actually receives
data = response.get("data", {})
preset = data.get("preset", {})
print(f"\nExtracted preset: {preset}")
