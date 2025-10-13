# Build System Quick Start Guide
## ChimeraPhoenix Optimized Build System

---

## TL;DR

The build system is now **5.6x faster** with automatic caching and parallel compilation.

```bash
# Just run make - everything is automatic
make

# Check build statistics
make stats
```

---

## What Changed?

### Before
- Single-core compilation: **61.74 seconds**
- No caching: every build from scratch
- Manual dependency management

### After
- Multi-core parallel compilation: **11.09 seconds** (5.6x faster)
- Compiler caching: **<1 second** for cached rebuilds
- Automatic dependency tracking
- Precompiled headers

---

## Installation (One-Time Setup)

ccache is already installed and configured. No action needed.

If you need to reinstall:
```bash
brew install ccache
```

---

## Daily Usage

### Building the Project

```bash
# Build everything (recommended)
make

# Or explicitly
make all
```

The build system automatically:
- Uses all CPU cores (14 cores detected)
- Caches compilation results
- Tracks dependencies
- Rebuilds only what changed

### Cleaning

```bash
# Remove build artifacts (keeps ccache)
make clean

# Full clean including cache (rarely needed)
make clean-all
```

### Checking Performance

```bash
# See ccache statistics
make stats
```

Expected output:
```
Build System Statistics
================================================
CPU Cores: 14

Cacheable calls:     245 /  245 (100.0%)
  Hits:              220 /  245 ( 89.8%)  ← Good!
  Misses:             25 /  245 ( 10.2%)
Cache size: 0.5 GB / 10.0 GB
```

High cache hit rate (>80%) = fast builds!

---

## Build Times You Should Expect

| Scenario | Time | Notes |
|----------|------|-------|
| First clean build | ~11s | Populates ccache |
| Clean build (cached) | <1s | 90%+ cache hits |
| Single file change | 2-5s | Recompile + relink |
| Header file change | 10-20s | Multiple files rebuild |
| No changes | <0.1s | Dependency check only |

---

## Advanced Usage

### Building Specific Tests

```bash
# Build just one test
make build/reverb_test

# Build multiple specific tests
make build/filter_test build/dynamics_test
```

### Controlling Parallel Builds

```bash
# Use fewer cores (for background builds)
make -j4

# Single-core build (for debugging)
make -j1
```

### Selective Cleaning

```bash
# Remove only test executables
make clean-tests

# Remove only engine objects
make clean-engines

# Remove precompiled header
make clean-pch
```

---

## Understanding the Build Process

### What Happens When You Run `make`

1. **Check ccache** (auto-detected)
   - If not found, warns and continues

2. **Build Precompiled Header** (~0.5s)
   - Only if changed or missing
   - Speeds up all subsequent compilations

3. **Compile Source Files** (~8-9s)
   - Runs in parallel across all cores
   - Uses ccache for unchanged files
   - Compiles: engines, tests, JUCE modules

4. **Link Executables** (~2-3s)
   - Creates test binaries
   - Uses cached object files

### Directory Structure

```
standalone_test/
├── build/
│   ├── obj/              # Object files (*.o)
│   ├── pch/              # Precompiled headers
│   ├── standalone_test   # Test executables
│   ├── reverb_test
│   └── ...
├── Makefile              # Optimized build rules
├── PrecompiledHeader.h   # Standard library headers
└── *.cpp                 # Test source files
```

Never delete `build/obj/` manually - use `make clean`!

---

## Troubleshooting

### "Build is slow"

**Check ccache is working:**
```bash
make stats
```

Look for "Hits" percentage. Should be >80% after first build.

**If ccache not found:**
```bash
brew install ccache
```

### "Linking errors"

**Try a clean rebuild:**
```bash
make clean && make
```

### "Out of disk space"

**Clear ccache (keeps build artifacts):**
```bash
ccache -C
```

**Or full clean:**
```bash
make clean-all
```

### "Build using too much CPU"

**Limit parallel jobs:**
```bash
make -j4  # Use only 4 cores
```

**Add to your shell profile for permanent change:**
```bash
echo 'alias make="make -j4"' >> ~/.zshrc
```

---

## Performance Tips

### DO

✓ Let ccache run normally
✓ Use `make` without clean unless necessary
✓ Check `make stats` periodically
✓ Commit build artifacts to .gitignore

### DON'T

✗ Run `make clean` frequently
✗ Delete build/ directory manually
✗ Disable parallel builds (`-j1`) unless debugging
✗ Clear ccache unnecessarily

---

## Monitoring Build Performance

### Watch Build in Real-Time

```bash
# See what's being compiled
make 2>&1 | grep "→ Compiling"
```

### Measure Build Time

```bash
time make
```

### Check CPU Usage

```bash
# In another terminal while building
top -pid $(pgrep make)
```

You should see ~900-1000% CPU usage (9-10 cores active).

---

## Integration with IDEs

### VS Code

Add to `.vscode/tasks.json`:
```json
{
    "label": "Build ChimeraPhoenix",
    "type": "shell",
    "command": "make",
    "group": {
        "kind": "build",
        "isDefault": true
    },
    "problemMatcher": ["$gcc"]
}
```

Keyboard shortcut: Cmd+Shift+B

### CLion

CLion will auto-detect the Makefile.

Settings → Build → Use Makefile: Yes

---

## CI/CD Integration

### GitHub Actions Example

```yaml
- name: Install ccache
  run: brew install ccache

- name: Cache ccache
  uses: actions/cache@v3
  with:
    path: ~/Library/Caches/ccache
    key: ccache-${{ runner.os }}-${{ github.sha }}
    restore-keys: ccache-${{ runner.os }}-

- name: Build
  run: make

- name: Show build stats
  run: make stats
```

### Expected CI Build Times

- First build: ~15-20s (cache miss)
- Subsequent builds: ~2-5s (cache hit)

---

## FAQ

**Q: Do I need to do anything special to use the optimizations?**
A: No! Just run `make`. Everything is automatic.

**Q: Will this work on other machines?**
A: Yes! The Makefile auto-detects:
- Number of CPU cores
- ccache availability
- Pre-built JUCE objects

**Q: What if I don't have ccache?**
A: The build system detects this and warns you, but still works (just slower).

**Q: Can I use this with CMake/other build systems?**
A: These optimizations are Makefile-specific, but the concepts (ccache, PCH, parallel builds) apply to any build system.

**Q: How much disk space does ccache use?**
A: Configured for 10GB max. Typically uses ~500MB for this project.

**Q: Is ccache safe?**
A: Yes! It detects compiler version, flags, and source changes. False cache hits are extremely rare.

**Q: What if the build breaks after pulling changes?**
A: Try `make clean && make`. This clears all build artifacts.

---

## Getting Help

### Build System Information

```bash
# Show all available targets
make help

# Show configured engines
make list-engines
```

### Full Documentation

See `BUILD_OPTIMIZATION_REPORT.md` for detailed technical information.

### Issues

If you encounter build issues:

1. Check ccache: `make stats`
2. Try clean build: `make clean && make`
3. Check disk space: `df -h`
4. Verify ccache version: `ccache --version`

---

## Summary

The optimized build system is:
- **Fast:** 5.6x faster clean builds, <1s cached rebuilds
- **Automatic:** No configuration needed
- **Robust:** Handles dependencies correctly
- **Scalable:** Uses all available CPU cores

**Just run `make` and enjoy faster builds!**

---

*Last Updated: October 11, 2025*
*Build System Version: 1.0 (Optimized)*
