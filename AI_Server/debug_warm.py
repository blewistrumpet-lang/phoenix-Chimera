#!/usr/bin/env python3
"""
Debug the 'warm' keyword issue
"""

import logging
import traceback

# Set up detailed logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(name)s - %(levelname)s - %(message)s'
)

# Test just the calculator with warm
from calculator_enhanced import CalculatorEnhanced

calc = CalculatorEnhanced()

# Create a test preset with string values like from corpus
test_preset = {
    "slot1_engine": 4,
    "slot1_param0": "0.5",  # String instead of float
    "slot1_param6": "0.3",  # String instead of float
    "slot5_engine": 1,      # Opto Compressor
    "slot5_param6": "0.5",  # String that will be adjusted for warm
}

prompt = "warm vintage"
blueprint = {"name": "test"}

print("Testing calculator with 'warm' keyword and string params...")
print("=" * 60)

try:
    result = calc.apply_nudges(test_preset, prompt, blueprint)
    print("✅ SUCCESS")
except Exception as e:
    print(f"❌ FAILED: {e}")
    traceback.print_exc()