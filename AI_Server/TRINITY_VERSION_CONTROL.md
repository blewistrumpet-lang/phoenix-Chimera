# Trinity Server - Version Control & Production Setup

**Date:** October 13, 2025
**Current Production Version:** `trinity_server_pi_CURRENT_USE.py`
**Backup:** `trinity_server_PRODUCTION.py`

---

## ⚠️ CRITICAL: Which Trinity Server to Use

### **Production Server (USE THIS ONE)**

```
~/phoenix-Chimera/AI_Server/trinity_server_pi_CURRENT_USE.py
```

**Symlinked as:** `trinity_server_pi.py` (for backwards compatibility)

### **Architecture:**

```
Voice → Whisper → Visionary (HYBRID) → Calculator (AI+Rules) → Alchemist → Preset
         OpenAI   GPT-4o-mini + Rules    GPT-4o + Gain      Python
```

### **Components:**

1. **Visionary:** `visionary_complete.py`
   - Uses `engine_selector.py` for rule-based engine selection
   - Character-based prompts (80%) → Rules
   - Knowledge-based prompts (20%) → GPT with constraints
   - Generates musical intent metadata

2. **Calculator:** `calculator_max_intelligence.py`
   - GPT-4o for parameter optimization
   - GainStagingAnalyzer for professional audio engineering
   - Musical time subdivisions (deterministic)
   - Parameter parsing (regex-based)

3. **Alchemist:** `alchemist_complete.py`
   - Pure Python validation
   - Parameter clamping
   - Safety checks

### **Why This Version:**

✅ **Hybrid Intelligence** - Rules prevent coherence failures, AI adds creativity
✅ **Performance Optimized** - 1 GPT call in Calculator (was 3)
✅ **Professional Audio** - Gain staging prevents clipping
✅ **95%+ Coherence** - Rule-based engine selection for character prompts
✅ **Musical Intent Flow** - Visionary tells Calculator WHY engines were chosen

---

## 🚫 DO NOT USE These Files

These are **deprecated** or **experimental** versions:

### Old Trinity Implementations:
- `main.py` - Old implementation from Oct 4
- `main_trinity.py` - Experimental variant
- `main_string_ids.py` - Old string-based version
- `trinity_server.py` - Generic old version
- `trinity_server_intelligent.py` - Superseded
- `trinity_server_complete.py` - Superseded
- `trinity_server_max.py` - Experimental
- `trinity_server_pi_backup.py` - Old backup
- `trinity_server_pi_enhanced.py` - Failed experiment

### Why Not to Use Them:
- Missing hybrid intelligence
- No gain staging
- Different component imports
- May use deprecated Oracle/Corpus
- Not tested with current plugin

---

## 🔄 Launch Scripts

Both launch scripts correctly use the production server:

### `launch_chimera.sh` (Full-featured)
```bash
python3 trinity_server_pi.py  # → trinity_server_pi_CURRENT_USE.py
```

### `launch_chimera_fixed.sh` (Simplified)
```bash
python3 trinity_server_pi.py  # → trinity_server_pi_CURRENT_USE.py
```

**Note:** The symlink ensures both work correctly.

---

## 📋 Version History

### **v3.0 - Production (Current) - Oct 13, 2025**
- **Hybrid Intelligence:** Rules + AI collaboration
- **Gain Staging:** Professional audio engineering
- **Musical Intent:** Visionary → Calculator communication
- **Performance:** Single GPT call optimization
- **Components:**
  - `visionary_complete.py` (GPT-4o-mini + rules)
  - `calculator_max_intelligence.py` (GPT-4o + gain analysis)
  - `alchemist_complete.py` (Python validation)
  - `engine_selector.py` (Rule-based engine selection)

### **v2.x - Deprecated - Oct 4-9, 2025**
- Pure AI approach (no rules)
- Multiple GPT calls (slow)
- Coherence issues with certain prompts
- Files: `main.py`, `trinity_server_intelligent.py`

### **v1.x - Deprecated - Pre-Oct 2025**
- Oracle + Golden Corpus (removed)
- FAISS semantic search (removed)
- Files: Oracle-related, corpus-related

---

## 🛠️ Making Modifications

### If you need to modify Trinity:

1. **ALWAYS work on** `trinity_server_pi_CURRENT_USE.py`
2. **Test thoroughly** before deploying
3. **Update this document** with changes
4. **Keep backup:** `trinity_server_PRODUCTION.py` is your safety net

### Testing Process:

```bash
# 1. Stop current server
pkill -f trinity_server_pi.py

# 2. Make changes to trinity_server_pi_CURRENT_USE.py

# 3. Test manually
cd ~/phoenix-Chimera/AI_Server
python3 trinity_server_pi_CURRENT_USE.py

# 4. Test health endpoint
curl http://localhost:8000/health

# 5. Test generation
curl -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": "warm vintage tape echo"}'

# 6. If successful, update PRODUCTION backup
cp trinity_server_pi_CURRENT_USE.py trinity_server_PRODUCTION.py
```

### Rollback If Needed:

```bash
# Restore from production backup
cp trinity_server_PRODUCTION.py trinity_server_pi_CURRENT_USE.py

# Restart server
pkill -f trinity_server_pi.py
cd ~/phoenix-Chimera/AI_Server
nohup python3 trinity_server_pi.py > /tmp/trinity_server.log 2>&1 &
```

---

## 📊 Performance Metrics

### Current Production Performance:
- **Typical Generation:** 2-3 seconds
- **Whisper Transcription:** 1-2 seconds
- **Visionary (Hybrid):** 800-1200ms (or instant for rule-based)
- **Calculator:** 1000-1500ms (1 GPT call)
- **Alchemist:** 50-100ms (local)

### Success Rates:
- **Coherence:** 95%+ (rule-based prevents mistakes)
- **API Uptime:** Dependent on OpenAI
- **Parameter Validity:** 100% (Alchemist validation)

---

## 🔍 Troubleshooting

### "Which Trinity is running?"

```bash
# Check process
ps aux | grep trinity_server

# Should show: trinity_server_pi.py (symlink to CURRENT_USE)

# Verify it's the right one
readlink ~/phoenix-Chimera/AI_Server/trinity_server_pi.py
# Should output: trinity_server_pi_CURRENT_USE.py
```

### "Server not responding"

```bash
# Check logs
tail -f /tmp/trinity_server.log

# Common issues:
# - Missing OpenAI API key in .env
# - Port 8000 already in use
# - Missing Python dependencies
```

### "Getting wrong responses"

```bash
# Verify components are correct
cd ~/phoenix-Chimera/AI_Server
grep "from visionary_complete" trinity_server_pi_CURRENT_USE.py
grep "from calculator_max_intelligence" trinity_server_pi_CURRENT_USE.py
grep "from alchemist_complete" trinity_server_pi_CURRENT_USE.py

# All three should be present
```

---

## 📝 Quick Reference Card

### Start Server:
```bash
cd ~/phoenix-Chimera/AI_Server
python3 trinity_server_pi.py
```

### Check Health:
```bash
curl http://localhost:8000/health
```

### View Logs:
```bash
tail -f /tmp/trinity_server.log
```

### Stop Server:
```bash
pkill -f trinity_server_pi.py
```

### Production File:
```
~/phoenix-Chimera/AI_Server/trinity_server_pi_CURRENT_USE.py
```

### Backup:
```
~/phoenix-Chimera/AI_Server/trinity_server_PRODUCTION.py
```

---

**Remember:** Don't modify random Trinity files. Use `trinity_server_pi_CURRENT_USE.py` only.
