# Essential Documents for Next GPIO Session

**Last Updated**: October 26, 2025
**Current Phase**: Phase 2 Deployed, Awaiting Verification

---

## 📚 Read These First (In Order)

### 1. **GPIO_MASTER_REFERENCE.md** ⭐⭐⭐
**Why**: Complete consolidated knowledge as of Oct 26, 2025
**Contains**:
- Current deployment status (Pi .65, PID 815978)
- All Phase 2 fixes explained
- Architecture overview
- Testing checklist
- Build/deploy procedures
- SSH access info

**Start here** - this is the single source of truth.

---

### 2. **PHASE2_DEPLOYED.md** ⭐⭐⭐
**Why**: Runtime status of currently deployed code
**Contains**:
- Exact binary location & PID
- GPIO initialization status
- Log file paths
- What's running right now

**Use this** to understand what's already deployed before making changes.

---

### 3. **PHASE2_OUTCOME.md** ⭐⭐
**Why**: Detailed implementation of Phase 2 fixes
**Contains**:
- What was fixed (4 bugs)
- Code changes with file:line references
- Verification checklist
- Expected log patterns

**Use this** when debugging or extending Phase 2 code.

---

### 4. **TRINITY_GPIO_4WEEK_PLAN.md** ⭐⭐
**Why**: Overall roadmap and milestones
**Contains**:
- 4-week plan (Weeks 1-4)
- Current progress (Week 2 Phase 2)
- Acceptance criteria
- Deferred features (morph, LIVE mode, etc.)

**Use this** to understand where we are in the overall plan.

---

## 📖 Important Context (Read If Needed)

### 5. **GPIO_SESSION_HANDOFF.md**
**Date**: Oct 25
**Why**: Documents the mix encoder fix & identifies bugs that Phase 2 addresses
**Use when**: Understanding why Phase 2 was needed

### 6. **GPIO_WEEK2_PHASE2_SESSION.md**
**Date**: Oct 24
**Why**: Documents GPIOPresetManager creation & Week 2 Phase 2 infrastructure
**Use when**: Understanding preset system architecture

### 7. **PI_MIGRATION_PLAN.md**
**Date**: Oct 18
**Why**: Explains repository structure & device roles
**Key Info**:
- Pi .65 = GPIO dev board (no HiFiBerry)
- Pi .68 = Production (HiFiBerry, hostname: `hifiberrypi`)
- Both use `~/phoenix-Chimera/pi_deployment/` structure
**Use when**: Confused about which Pi does what

### 8. **REPOSITORY_ANALYSIS.md**
**Date**: Oct 18
**Why**: Explains codebase architecture, engine system, parameter flow
**Use when**: Need to understand plugin architecture beyond GPIO

### 9. **pi_deployment/CHIMERA_PHOENIX_PI_COMPLETE_ANALYSIS.md**
**Date**: Oct 18
**Why**: Pi-specific architecture (Trinity AI, voice input, HiFiBerry config)
**Use when**: Working on Trinity integration or audio routing

---

## 🚫 DO NOT READ (Obsolete or Conflicting)

These have been superseded or contain outdated information:

- ❌ **GPIO_COMPREHENSIVE_PLAN.md** - Superseded by TRINITY_GPIO_4WEEK_PLAN.md
- ❌ **GPIO_NEXT_SESSION_PROMPT.md** - Superseded by GPIO_MASTER_REFERENCE.md
- ❌ **FINAL_FIX_SUMMARY.md** - Content merged into PHASE2_OUTCOME.md
- ❌ **FIXES_APPLIED_SUMMARY.md** - Build history, no longer relevant
- ❌ **IMPLEMENTATION_PLAN.md** - Work notes, superseded by outcome docs
- ❌ **SYSTEM_ANALYSIS_AND_4WEEK_PLAN.md** - Draft, superseded by official 4-week plan
- ❌ **GPIO_OCT24_SESSION_SUMMARY.md** - Superseded by GPIO_WEEK2_PHASE2_SESSION.md
- ❌ **PRESET_BUG_FIX_VERIFICATION.md** - Old verification, now part of PHASE2_OUTCOME.md

---

## 🎯 Quick Start for Next Session

### If Phase 2 Verification Passed
```
Read:
1. GPIO_MASTER_REFERENCE.md (sections: "Week 3 Planning")
2. TRINITY_GPIO_4WEEK_PLAN.md (section: "Week 3: Engines + MODE")

Start with:
> "Phase 2 verified! All tests passed. Ready to start Week 3 Phase 1:
> Engine parameter registration and MODE switch macro controls."
```

### If Phase 2 Verification Failed
```
Read:
1. GPIO_MASTER_REFERENCE.md (sections: "Testing Phase 2", "Debugging Quick Reference")
2. PHASE2_OUTCOME.md (section: "Verification Checklist")

Start with:
> "Phase 2 verification results:
> - A1 (Preset stepping): [PASS/FAIL]
> - A2 (Fast spin): [PASS/FAIL]
> - A3 (Continuous smooth): [PASS/FAIL]
> - A4 (A/B independence): [PASS/FAIL]
> - A5 (Mix per-bank): [PASS/FAIL]
> - A6 (Debounce): [PASS/FAIL]
>
> Logs: [paste relevant grep output]"
```

### If Starting Fresh Topic (Non-GPIO)
```
Read:
1. REPOSITORY_ANALYSIS.md (overall architecture)
2. CHIMERA_PHOENIX_BETA_ROADMAP.md (product status)
3. README.md (project overview)
```

---

## 🧭 Document Hierarchy

```
GPIO_MASTER_REFERENCE.md              ← START HERE (consolidates everything)
├─ PHASE2_DEPLOYED.md                 ← Current runtime status
├─ PHASE2_OUTCOME.md                  ← Phase 2 implementation details
├─ TRINITY_GPIO_4WEEK_PLAN.md         ← Overall roadmap
└─ Context docs (read as needed):
    ├─ GPIO_SESSION_HANDOFF.md        ← Oct 25 session
    ├─ GPIO_WEEK2_PHASE2_SESSION.md   ← Oct 24 session
    ├─ PI_MIGRATION_PLAN.md           ← Device roles
    ├─ REPOSITORY_ANALYSIS.md         ← Codebase architecture
    └─ CHIMERA_PHOENIX_PI_COMPLETE_ANALYSIS.md ← Pi architecture
```

---

## 📋 Recency-Weighted Truth

When documents conflict, **newest information wins**:

| Date | Document | Authority |
|------|----------|-----------|
| **Oct 26** | GPIO_MASTER_REFERENCE.md | ✅ CANONICAL |
| **Oct 26** | PHASE2_DEPLOYED.md | ✅ CURRENT STATE |
| **Oct 26** | PHASE2_OUTCOME.md | ✅ PHASE 2 TRUTH |
| Oct 25 | GPIO_SESSION_HANDOFF.md | ⚠️ Historical (Phase 2 WIP) |
| Oct 24 | GPIO_WEEK2_PHASE2_SESSION.md | ⚠️ Historical (pre-Phase 2 fixes) |
| Oct 23 | TRINITY_GPIO_4WEEK_PLAN.md | ✅ ROADMAP (still valid) |
| Oct 18 | PI_MIGRATION_PLAN.md | ⚠️ Partial (. 68 not migrated yet) |

**Rule**: If Oct 25 doc says "mix restore disabled" but Oct 26 doc says "mix restore enabled", trust Oct 26.

---

## 🚀 Session Starter Templates

### Template 1: Verification Complete
```
I've tested Phase 2 on Pi .65. Results:
- A1 (Preset stepping): PASS - stepped cleanly 1→2→3...→10
- A2 (Fast spin): PASS - no jumps even with rapid turns
- A3 (Continuous): PASS - E2/E3 smooth
- A4 (A/B independence): PASS - Bank A restored exactly
- A5 (Mix per-bank): PASS - Mix differs between banks
- A6 (Debounce): PASS - no double-triggers

All Phase 2 objectives met! Ready for Week 3 Phase 1.

Provide these docs:
- GPIO_MASTER_REFERENCE.md
- TRINITY_GPIO_4WEEK_PLAN.md (Week 3 section)
```

### Template 2: Bugs Found
```
Phase 2 verification found issues:
- A1: FAIL - Still jumps from 3 to 8 on slow turn
- A4: FAIL - Bank A resets to 0 on switch

Logs: [paste grep output]

Provide these docs:
- GPIO_MASTER_REFERENCE.md
- PHASE2_OUTCOME.md
- PHASE2_DEPLOYED.md
```

### Template 3: New Feature Request
```
Phase 2 working! Now I want to add [feature description].

Provide these docs:
- GPIO_MASTER_REFERENCE.md (current state)
- TRINITY_GPIO_4WEEK_PLAN.md (check if feature is planned)
- REPOSITORY_ANALYSIS.md (if touching non-GPIO code)
```

---

## 💡 Tips for Effective Sessions

### DO:
✅ Always provide GPIO_MASTER_REFERENCE.md
✅ Mention which test results you have (if testing)
✅ Include log excerpts when reporting bugs
✅ Specify which Pi (.65 or .68)

### DON'T:
❌ Provide 10+ docs without context
❌ Mix old conflicting docs with new ones
❌ Forget to mention current deployment status
❌ Assume I know what changed since last session

---

**End of Essential Docs List**
*Keep this with GPIO_MASTER_REFERENCE.md for every GPIO session*
