/**
 * PrecompiledHeader.h - Precompiled header for ChimeraPhoenix
 *
 * This header includes commonly used headers to speed up compilation.
 * Only include headers that are:
 * - Used frequently across the codebase
 * - Stable (rarely change)
 * - Large (expensive to parse)
 */

#pragma once

// Standard C++ headers - These are stable and widely used
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <complex>

// Common macros
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif
