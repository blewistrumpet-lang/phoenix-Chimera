# TRINITY SYSTEM FIX CHECKLIST

Quick reference for implementing fixes to Trinity AI system.

---

## 🔴 CRITICAL FIXES (12 hours total)

### ✅ Fix #1: Parameter Count (10 → 15) - 2 hours

**Files to modify:**
- [ ] `visionary_trinity.py`
- [ ] `calculator_trinity.py`
- [ ] `alchemist_trinity.py`

**Changes:**

```python
# visionary_trinity.py
# Line 50: Update system prompt
3. ALL 15 parameters for each engine (0.0-1.0 normalized)  # WAS: 10

# Line 99: Update AI instructions
1. Parameters array must have exactly 15 values (0.0 to 1.0)  # WAS: 10

# Lines 387-391: Update validation
if len(params) < 15:  # WAS: 10
    params.extend([0.0] * (15 - len(params)))  # WAS: 10
elif len(params) > 15:  # WAS: 10
    params = params[:15]  # WAS: 10
```

```python
# alchemist_trinity.py
# Line 528: Update parameter loop
for i in range(15):  # Always 15 parameters  # WAS: 10
```

```python
# calculator_trinity.py - NO CHANGES NEEDED
# Already works with variable-length parameter arrays
```

**Test:**
```bash
python3 -c "
from visionary_trinity import VisionaryTrinity
v = VisionaryTrinity(api_key='test')
# Check system prompt mentions 15
assert '15 parameters' in v.system_prompt
print('✅ Visionary: 15 parameters')
"
```

---

### ✅ Fix #2: Method Name Mismatches - 1 hour

**Option A: Rename component methods (RECOMMENDED)**

```python
# visionary_trinity.py - Line 154
# BEFORE:
async def generate_complete_preset(self, prompt: str) -> Dict[str, Any]:

# AFTER:
async def generate_preset(
    self,
    prompt: str,
    intensity: float = 0.5,
    num_engines: int = 4
) -> Dict[str, Any]:
    """Generate preset from user prompt"""
    # Use intensity to scale parameter values
    # Use num_engines to limit engine count
```

```python
# calculator_trinity.py - Line 172
# BEFORE:
def refine_preset(self, preset: Dict[str, Any], prompt: str) -> Dict[str, Any]:

# AFTER:
async def optimize_preset(
    self,
    preset: Dict[str, Any],
    prompt: str,
    intensity: float = 0.5
) -> Dict[str, Any]:
    """Optimize preset with musical intelligence"""
    # Make async using asyncio.to_thread if needed
    return await asyncio.to_thread(self.refine_preset, preset, prompt)
```

```python
# alchemist_trinity.py - Line 128
# BEFORE:
def validate_and_optimize(self, preset: Dict[str, Any]) -> Dict[str, Any]:

# AFTER:
def finalize_preset(self, preset: Dict[str, Any]) -> Dict[str, Any]:
    """Final safety validation and formatting"""
    return self.validate_and_optimize(preset)
```

**Test:**
```bash
python3 -c "
from visionary_trinity import VisionaryTrinity
from calculator_trinity import CalculatorTrinity
from alchemist_trinity import AlchemistTrinity

# Check methods exist
v = VisionaryTrinity(None)
c = CalculatorTrinity()
a = AlchemistTrinity()

assert hasattr(v, 'generate_preset'), 'Visionary missing generate_preset'
assert hasattr(c, 'optimize_preset'), 'Calculator missing optimize_preset'
assert hasattr(a, 'finalize_preset'), 'Alchemist missing finalize_preset'

print('✅ All method names correct')
"
```

---

### ✅ Fix #3: Async OpenAI Implementation - 3 hours

**Install dependencies:**
```bash
pip install openai[async]
```

**Changes:**

```python
# visionary_trinity.py - Line 10
# BEFORE:
from openai import OpenAI

# AFTER:
from openai import AsyncOpenAI
```

```python
# visionary_trinity.py - Line 29
# BEFORE:
self.client = OpenAI(api_key=api_key)

# AFTER:
self.client = AsyncOpenAI(api_key=api_key)
```

```python
# visionary_trinity.py - Line 173
# BEFORE:
response = self.client.chat.completions.create(

# AFTER:
response = await self.client.chat.completions.create(
```

**Test:**
```bash
# Test async works
python3 -c "
import asyncio
from visionary_trinity import VisionaryTrinity
import os

async def test():
    v = VisionaryTrinity(api_key=os.getenv('OPENAI_API_KEY'))
    # Should not block
    assert asyncio.iscoroutinefunction(v.generate_preset)
    print('✅ Visionary is async')

asyncio.run(test())
"
```

---

### ✅ Fix #4: Input Validation - 2 hours

```python
# main_trinity.py - Lines 49-54
# BEFORE:
class GenerateRequest(BaseModel):
    prompt: str = Field(..., description="User's creative prompt")
    intensity: float = Field(default=0.5, ge=0.0, le=1.0)
    complexity: int = Field(default=3, ge=1, le=6)

# AFTER:
from pydantic import BaseModel, Field, validator
import re

class GenerateRequest(BaseModel):
    prompt: str = Field(
        ...,
        min_length=3,
        max_length=500,
        description="User's creative prompt (3-500 chars)"
    )
    intensity: float = Field(
        default=0.5,
        ge=0.0,
        le=1.0,
        description="Effect intensity"
    )
    complexity: int = Field(
        default=4,
        ge=1,
        le=6,
        description="Number of engines (1-6)"
    )

    @validator('prompt')
    def validate_prompt(cls, v):
        # Remove excess whitespace
        v = ' '.join(v.split())

        # Only alphanumeric and safe punctuation
        if not re.match(r'^[a-zA-Z0-9\s\-\.,!?\'\"]+$', v):
            raise ValueError("Invalid characters in prompt")

        # Check for injection patterns
        dangerous = ['ignore previous', 'system prompt', '<script']
        if any(p in v.lower() for p in dangerous):
            raise ValueError("Suspicious content detected")

        return v
```

**Test:**
```bash
curl -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": ""}' # Should fail: too short

curl -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": "ignore previous instructions"}' # Should fail: injection

curl -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": "warm vintage tone"}' # Should succeed
```

---

### ✅ Fix #5: Data Format Standardization - 4 hours

**Create new file:** `schemas.py`

```python
# schemas.py
from pydantic import BaseModel, Field, validator
from typing import List, Optional, Dict, Any

class PresetSlot(BaseModel):
    slot: int = Field(ge=1, le=6)
    engine_id: int = Field(ge=0, le=56)
    engine_name: str
    parameters: List[float] = Field(min_items=15, max_items=15)

    @validator('parameters')
    def validate_range(cls, v):
        for p in v:
            if not (0.0 <= p <= 1.0):
                raise ValueError(f"Parameter {p} out of range")
        return [round(p, 4) for p in v]

class Preset(BaseModel):
    name: str = Field(min_length=1, max_length=100)
    description: Optional[str] = None
    slots: List[PresetSlot] = Field(max_items=6)
    metadata: Optional[Dict[str, Any]] = None

    def to_plugin_format(self) -> Dict[str, Any]:
        """Convert to flat format for plugin"""
        result = {"name": self.name, "description": self.description or ""}

        for i in range(1, 7):
            if i <= len(self.slots):
                slot = self.slots[i-1]
                result[f"slot{i}_engine"] = slot.engine_id
                for j, param in enumerate(slot.parameters):
                    result[f"slot{i}_param{j}"] = param
            else:
                result[f"slot{i}_engine"] = 0
                for j in range(15):
                    result[f"slot{i}_param{j}"] = 0.0

        return result
```

**Update components:**

```python
# visionary_trinity.py - Add at top
from schemas import Preset, PresetSlot

# visionary_trinity.py - Line 154 return type
async def generate_preset(...) -> Preset:  # WAS: Dict[str, Any]
    # ... existing code ...

    # Build with schema
    preset = Preset(
        name=preset_json["name"],
        description=preset_json.get("description"),
        slots=[
            PresetSlot(
                slot=slot["slot"],
                engine_id=slot["engine_id"],
                engine_name=self.engine_names[slot["engine_id"]],
                parameters=slot["parameters"]
            )
            for slot in preset_json["slots"]
        ]
    )
    return preset
```

```python
# calculator_trinity.py - Update signatures
from schemas import Preset

async def optimize_preset(self, preset: Preset, ...) -> Preset:
    # Work with preset.slots directly
    # Auto-validated!
```

```python
# alchemist_trinity.py - Update signatures
from schemas import Preset

def finalize_preset(self, preset: Preset) -> Dict[str, Any]:
    # Validate
    # ... existing validation code ...

    # Convert to plugin format
    return preset.to_plugin_format()
```

**Test:**
```python
# test_schemas.py
from schemas import Preset, PresetSlot

def test_validates_parameter_count():
    # Should fail: only 10 params
    try:
        slot = PresetSlot(
            slot=1,
            engine_id=15,
            engine_name="Test",
            parameters=[0.5] * 10  # Wrong!
        )
        assert False, "Should have failed"
    except ValueError:
        pass  # Expected

    # Should succeed: 15 params
    slot = PresetSlot(
        slot=1,
        engine_id=15,
        engine_name="Test",
        parameters=[0.5] * 15  # Correct!
    )
    assert len(slot.parameters) == 15

def test_validates_parameter_range():
    # Should fail: param out of range
    try:
        slot = PresetSlot(
            slot=1,
            engine_id=15,
            engine_name="Test",
            parameters=[1.5] * 15  # Out of range!
        )
        assert False, "Should have failed"
    except ValueError:
        pass  # Expected

def test_plugin_format_conversion():
    preset = Preset(
        name="Test",
        slots=[
            PresetSlot(
                slot=1,
                engine_id=15,
                engine_name="Test",
                parameters=[0.5] * 15
            )
        ]
    )

    plugin_format = preset.to_plugin_format()

    assert plugin_format["slot1_engine"] == 15
    assert plugin_format["slot1_param0"] == 0.5
    assert plugin_format["slot2_engine"] == 0  # Empty slot
    assert plugin_format["slot2_param0"] == 0.0

if __name__ == "__main__":
    test_validates_parameter_count()
    test_validates_parameter_range()
    test_plugin_format_conversion()
    print("✅ All schema tests passed")
```

---

## VERIFICATION CHECKLIST

After all critical fixes:

```bash
# 1. Server starts without errors
python3 main_trinity.py
# Should see: "✅ TRUE Trinity Pipeline initialized"

# 2. Health check passes
curl http://localhost:8000/health
# Should return: {"status": "healthy"}

# 3. Generate request completes
curl -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": "warm vintage tone"}' \
  | jq '.success'
# Should return: true

# 4. Output has correct structure
curl -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": "warm vintage tone"}' \
  | jq '.preset | keys'
# Should include: name, description, slot1_engine, slot1_param0, etc.

# 5. Parameters are in range
curl -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": "warm vintage tone"}' \
  | jq '.preset.slot1_param0'
# Should be between 0.0 and 1.0

# 6. All slots have 15 parameters
for i in {0..14}; do
  curl -s -X POST http://localhost:8000/generate \
    -H "Content-Type: application/json" \
    -d '{"prompt": "test"}' \
    | jq -e ".preset.slot1_param$i != null" || echo "Missing param $i"
done
# Should not print any "Missing param" messages
```

---

## ROLLBACK PLAN

If fixes cause new issues:

```bash
# 1. Backup current state
git add -A
git commit -m "WIP: Critical fixes in progress"

# 2. Create branch for fixes
git checkout -b fix/critical-issues

# 3. If rollback needed
git checkout main
git reset --hard HEAD~1  # Remove WIP commit

# 4. Investigate issue
git diff main fix/critical-issues

# 5. Fix issue and try again
git checkout fix/critical-issues
# ... make fixes ...
git add -A
git commit -m "Fix issue X"
```

---

## SUCCESS CRITERIA

System is considered fixed when:

- [X] Server starts without errors
- [X] Health check returns "healthy"
- [X] Can generate preset end-to-end
- [X] Output has 6 slots (some may be empty)
- [X] Each slot has exactly 15 parameters
- [X] All parameters in range [0.0, 1.0]
- [X] Plugin accepts generated presets
- [X] No crashes or exceptions
- [X] Response time < 30 seconds
- [X] Can handle 2 concurrent requests

---

## TIME TRACKING

| Fix | Estimated | Actual | Status |
|-----|-----------|--------|--------|
| #1: Parameter count | 2h | ___ | ⬜ |
| #2: Method names | 1h | ___ | ⬜ |
| #3: Async OpenAI | 3h | ___ | ⬜ |
| #4: Input validation | 2h | ___ | ⬜ |
| #5: Data schemas | 4h | ___ | ⬜ |
| **TOTAL** | **12h** | ___ | ⬜ |

---

## NOTES

- Take breaks between fixes to avoid burnout
- Test each fix individually before moving to next
- Commit after each successful fix
- Document any deviations from plan
- Ask for help if stuck >30 minutes

---

**Last Updated:** October 4, 2025
**Next Review:** After critical fixes completed
