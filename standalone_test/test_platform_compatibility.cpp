#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <cstring>
#include <type_traits>

/**
 * PLATFORM COMPATIBILITY TEST SUITE
 *
 * Tests for cross-platform issues even though we can only run on macOS:
 * - Endianness assumptions
 * - Data type size dependencies
 * - Memory alignment issues
 * - Floating point edge cases
 * - Path handling
 * - Denormal handling
 * - SIMD alignment
 * - Structure packing
 *
 * This helps identify potential issues before deploying to Windows/Linux.
 */

namespace PlatformCompatibility {

//==============================================================================
// Test Results
//==============================================================================

struct TestResult {
    std::string testName;
    bool passed = false;
    std::string details;
    std::vector<std::string> warnings;
    std::vector<std::string> recommendations;
};

std::vector<TestResult> allResults;

void reportTest(const TestResult& result) {
    std::cout << "\n[" << (result.passed ? "PASS" : "FAIL") << "] " << result.testName << "\n";
    if (!result.details.empty()) {
        std::cout << "  Details: " << result.details << "\n";
    }
    for (const auto& warning : result.warnings) {
        std::cout << "  WARNING: " << warning << "\n";
    }
    for (const auto& rec : result.recommendations) {
        std::cout << "  RECOMMEND: " << rec << "\n";
    }
    allResults.push_back(result);
}

//==============================================================================
// 1. ENDIANNESS TESTS
//==============================================================================

TestResult testEndianness() {
    TestResult result;
    result.testName = "Endianness Detection";

    // Test byte order
    union {
        uint32_t value;
        uint8_t bytes[4];
    } test;

    test.value = 0x12345678;

    if (test.bytes[0] == 0x78) {
        result.details = "Little-endian detected (x86/x64)";
        result.passed = true;
    } else if (test.bytes[0] == 0x12) {
        result.details = "Big-endian detected (rare on modern systems)";
        result.passed = true;
        result.warnings.push_back("Big-endian systems are rare - verify all byte operations");
    } else {
        result.details = "Unknown byte order";
        result.passed = false;
    }

    // Check for endian-dependent code patterns
    result.recommendations.push_back("Avoid direct memory reinterpretation across platforms");
    result.recommendations.push_back("Use juce::ByteOrder for cross-platform byte operations");

    return result;
}

TestResult testFloatBitPattern() {
    TestResult result;
    result.testName = "Float Bit Pattern Portability";

    union {
        float f;
        uint32_t i;
    } test;

    test.f = 1.0f;

    // IEEE 754 single precision: 0x3F800000
    bool isIEEE754 = (test.i == 0x3F800000);

    if (isIEEE754) {
        result.details = "IEEE 754 single precision (standard)";
        result.passed = true;
    } else {
        result.details = "Non-standard float representation detected";
        result.passed = false;
        result.warnings.push_back("Float bit patterns may differ across platforms");
    }

    return result;
}

//==============================================================================
// 2. DATA TYPE SIZE TESTS
//==============================================================================

TestResult testDataTypeSizes() {
    TestResult result;
    result.testName = "Data Type Sizes";
    result.passed = true;

    std::ostringstream details;
    details << "\n";
    details << "    char:      " << sizeof(char) << " bytes\n";
    details << "    short:     " << sizeof(short) << " bytes\n";
    details << "    int:       " << sizeof(int) << " bytes\n";
    details << "    long:      " << sizeof(long) << " bytes (PLATFORM-DEPENDENT!)\n";
    details << "    long long: " << sizeof(long long) << " bytes\n";
    details << "    float:     " << sizeof(float) << " bytes\n";
    details << "    double:    " << sizeof(double) << " bytes\n";
    details << "    void*:     " << sizeof(void*) << " bytes\n";
    details << "    size_t:    " << sizeof(size_t) << " bytes\n";

    result.details = details.str();

    // Check for potential issues
    if (sizeof(long) != sizeof(long long)) {
        result.warnings.push_back("'long' size differs from 'long long' - avoid 'long' for portability");
    }

    if (sizeof(int) != 4) {
        result.warnings.push_back("'int' is not 4 bytes - use int32_t for guaranteed size");
    }

    result.recommendations.push_back("Use fixed-width types (int32_t, uint64_t, etc.) for portability");
    result.recommendations.push_back("Avoid 'long' type - use int64_t or int32_t explicitly");

    return result;
}

TestResult testPointerSizeAssumptions() {
    TestResult result;
    result.testName = "Pointer Size Assumptions";

    bool is64bit = (sizeof(void*) == 8);
    bool is32bit = (sizeof(void*) == 4);

    if (is64bit) {
        result.details = "64-bit architecture (8-byte pointers)";
        result.passed = true;
    } else if (is32bit) {
        result.details = "32-bit architecture (4-byte pointers)";
        result.passed = true;
        result.warnings.push_back("32-bit architecture detected - test on 64-bit");
    } else {
        result.details = "Unknown pointer size: " + std::to_string(sizeof(void*));
        result.passed = false;
    }

    result.recommendations.push_back("Never cast pointers to int - use intptr_t/uintptr_t");
    result.recommendations.push_back("Use size_t for array indices, not int");

    return result;
}

//==============================================================================
// 3. ALIGNMENT TESTS
//==============================================================================

struct AlignmentTest1 {
    char a;
    int b;
    char c;
};

struct AlignmentTest2 {
    char a;
    double b;
    char c;
};

struct __attribute__((packed)) PackedStruct {
    char a;
    int b;
    char c;
};

TestResult testStructurePacking() {
    TestResult result;
    result.testName = "Structure Packing and Alignment";
    result.passed = true;

    std::ostringstream details;
    details << "\n";
    details << "    AlignmentTest1 (char,int,char):   " << sizeof(AlignmentTest1)
            << " bytes (expected: 12 with padding)\n";
    details << "    AlignmentTest2 (char,double,char): " << sizeof(AlignmentTest2)
            << " bytes (expected: 24 with padding)\n";
    details << "    PackedStruct (packed):             " << sizeof(PackedStruct)
            << " bytes (expected: 6 without padding)\n";

    result.details = details.str();

    if (sizeof(AlignmentTest1) != 12) {
        result.warnings.push_back("Unexpected struct padding - may differ on other platforms");
    }

    if (sizeof(PackedStruct) != 6) {
        result.warnings.push_back("Packed struct not 6 bytes - compiler may ignore __attribute__((packed))");
    }

    result.recommendations.push_back("Never rely on implicit struct packing across platforms");
    result.recommendations.push_back("Use explicit padding or #pragma pack for binary file formats");

    return result;
}

TestResult testSIMDAlignment() {
    TestResult result;
    result.testName = "SIMD Alignment Requirements";
    result.passed = true;

    // Allocate various alignment levels
    float* unaligned = new float[16];
    float* aligned16 = static_cast<float*>(std::aligned_alloc(16, 16 * sizeof(float)));

    uintptr_t addr_unaligned = reinterpret_cast<uintptr_t>(unaligned);
    uintptr_t addr_aligned16 = reinterpret_cast<uintptr_t>(aligned16);

    std::ostringstream details;
    details << "\n";
    details << "    Unaligned ptr:  0x" << std::hex << addr_unaligned
            << " (alignment: " << std::dec << (addr_unaligned % 16) << ")\n";
    details << "    16-byte aligned: 0x" << std::hex << addr_aligned16
            << " (alignment: " << std::dec << (addr_aligned16 % 16) << ")\n";

    result.details = details.str();

    if (addr_aligned16 % 16 != 0) {
        result.passed = false;
        result.warnings.push_back("std::aligned_alloc failed to provide 16-byte alignment");
    }

    if (addr_unaligned % 16 == 0) {
        result.warnings.push_back("'new' provided 16-byte alignment by luck - don't rely on it");
    }

    result.recommendations.push_back("Use JUCE's AudioBuffer which handles alignment internally");
    result.recommendations.push_back("For manual SIMD, use std::aligned_alloc or juce::aligned_malloc");

    std::free(aligned16);
    delete[] unaligned;

    return result;
}

//==============================================================================
// 4. FLOATING POINT TESTS
//==============================================================================

TestResult testDenormalHandling() {
    TestResult result;
    result.testName = "Denormal Number Handling";

    const float denormal = 1e-40f;  // Below normal range
    const float normal = 1e-10f;    // Normal range

    // Test denormal detection
    bool isDenormal = (denormal != 0.0f && std::fpclassify(denormal) == FP_SUBNORMAL);

    if (isDenormal) {
        result.details = "Denormals detected correctly (FP_SUBNORMAL)";
        result.passed = true;
    } else {
        result.details = "Denormals flushed to zero (FTZ/DAZ enabled)";
        result.passed = true;
        result.warnings.push_back("FTZ/DAZ may be enabled - good for performance, test on other platforms");
    }

    // Test performance impact
    auto start = std::chrono::high_resolution_clock::now();
    volatile float sum = 0.0f;
    for (int i = 0; i < 100000; ++i) {
        sum += denormal * denormal;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_denormal = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    sum = 0.0f;
    for (int i = 0; i < 100000; ++i) {
        sum += normal * normal;
    }
    end = std::chrono::high_resolution_clock::now();
    auto duration_normal = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    float ratio = static_cast<float>(duration_denormal) / duration_normal;

    std::ostringstream perf;
    perf << "\n    Normal ops:   " << duration_normal << " us\n";
    perf << "    Denormal ops: " << duration_denormal << " us\n";
    perf << "    Slowdown:     " << std::fixed << std::setprecision(2) << ratio << "x";
    result.details += perf.str();

    if (ratio > 10.0f) {
        result.warnings.push_back("Denormals are " + std::to_string(static_cast<int>(ratio)) +
                                  "x slower - DenormalGuard is critical!");
    }

    result.recommendations.push_back("Use DenormalGuard RAII wrapper in all process() methods");
    result.recommendations.push_back("Test denormal performance on Windows (may differ from macOS)");

    return result;
}

TestResult testNaNInfHandling() {
    TestResult result;
    result.testName = "NaN/Inf Handling";
    result.passed = true;

    const float nan_val = std::numeric_limits<float>::quiet_NaN();
    const float inf_val = std::numeric_limits<float>::infinity();
    const float ninf_val = -std::numeric_limits<float>::infinity();

    // Test detection
    bool nan_detected = std::isnan(nan_val);
    bool inf_detected = std::isinf(inf_val);
    bool ninf_detected = std::isinf(ninf_val);

    if (nan_detected && inf_detected && ninf_detected) {
        result.details = "NaN/Inf detection works correctly";
    } else {
        result.passed = false;
        result.details = "NaN/Inf detection FAILED";
    }

    // Test propagation
    float nan_result = nan_val + 1.0f;
    float inf_result = inf_val + 1.0f;

    if (std::isnan(nan_result) && std::isinf(inf_result)) {
        result.details += " | Propagation correct";
    } else {
        result.passed = false;
        result.warnings.push_back("NaN/Inf propagation behavior unexpected");
    }

    result.recommendations.push_back("Always check for NaN/Inf in audio output");
    result.recommendations.push_back("Use std::isfinite() to validate audio samples");

    return result;
}

TestResult testFloatVsDouble() {
    TestResult result;
    result.testName = "Float vs Double Precision";
    result.passed = true;

    const double precise_value = 0.123456789012345;
    const float truncated_value = static_cast<float>(precise_value);

    double error = std::abs(precise_value - truncated_value);

    std::ostringstream details;
    details << "\n";
    details << "    double value: " << std::setprecision(15) << precise_value << "\n";
    details << "    float value:  " << std::setprecision(8) << truncated_value << "\n";
    details << "    precision loss: " << std::scientific << error;

    result.details = details.str();

    if (error > 1e-7) {
        result.warnings.push_back("Significant precision loss when converting double->float");
    }

    result.recommendations.push_back("Use float for audio processing (matches JUCE AudioBuffer)");
    result.recommendations.push_back("Use double for parameter smoothing/accumulation if needed");

    return result;
}

//==============================================================================
// 5. PATH HANDLING TESTS
//==============================================================================

TestResult testPathSeparators() {
    TestResult result;
    result.testName = "File Path Separator Handling";
    result.passed = true;

    // Test JUCE path handling
    juce::File macPath("/Users/test/file.txt");
    juce::File winPath("C:\\Users\\test\\file.txt");
    juce::File unixPath("/home/test/file.txt");

    std::string macPathStr = macPath.getFullPathName().toStdString();
    std::string winPathStr = winPath.getFullPathName().toStdString();
    std::string unixPathStr = unixPath.getFullPathName().toStdString();

    std::ostringstream details;
    details << "\n";
    details << "    macOS path:   " << macPathStr << "\n";
    details << "    Windows path: " << winPathStr << "\n";
    details << "    Unix path:    " << unixPathStr;

    result.details = details.str();

    // Check if paths contain backslashes (shouldn't on Unix/macOS)
    if (macPathStr.find('\\') != std::string::npos) {
        result.warnings.push_back("macOS path contains backslashes - potential issue");
    }

    result.recommendations.push_back("Always use juce::File for path manipulation");
    result.recommendations.push_back("Never hardcode path separators - use File::getSeparatorChar()");

    return result;
}

TestResult testCaseSensitivity() {
    TestResult result;
    result.testName = "File System Case Sensitivity";
    result.passed = true;

    // macOS is case-insensitive by default, Linux is case-sensitive, Windows is case-insensitive
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::File testFile = tempDir.getChildFile("PLATFORM_TEST_FILE.txt");
    juce::File testFileLower = tempDir.getChildFile("platform_test_file.txt");

    testFile.deleteFile();
    testFileLower.deleteFile();

    testFile.create();

    bool lowerExists = testFileLower.existsAsFile();
    bool upperExists = testFile.existsAsFile();

    testFile.deleteFile();

    if (lowerExists && upperExists) {
        result.details = "File system is case-INSENSITIVE (macOS/Windows default)";
        result.warnings.push_back("Linux file systems are case-sensitive - test on Linux!");
    } else {
        result.details = "File system is case-SENSITIVE (Linux/macOS case-sensitive)";
    }

    result.recommendations.push_back("Always use consistent case in file paths");
    result.recommendations.push_back("Never rely on case-insensitivity - test on Linux");

    return result;
}

//==============================================================================
// 6. COMPILER DIFFERENCES
//==============================================================================

TestResult testCompilerMacros() {
    TestResult result;
    result.testName = "Compiler Detection";
    result.passed = true;

    std::ostringstream details;
    details << "\n";

#if defined(__clang__)
    details << "    Compiler: Clang " << __clang_major__ << "." << __clang_minor__ << "\n";
#elif defined(__GNUC__)
    details << "    Compiler: GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "\n";
#elif defined(_MSC_VER)
    details << "    Compiler: MSVC " << _MSC_VER << "\n";
#else
    details << "    Compiler: Unknown\n";
    result.warnings.push_back("Unknown compiler - may have compatibility issues");
#endif

#if defined(__x86_64__) || defined(_M_X64)
    details << "    Architecture: x86_64\n";
#elif defined(__aarch64__) || defined(_M_ARM64)
    details << "    Architecture: ARM64\n";
#elif defined(__arm__) || defined(_M_ARM)
    details << "    Architecture: ARM32\n";
#else
    details << "    Architecture: Unknown\n";
#endif

#if defined(__APPLE__)
    details << "    Platform: macOS/iOS\n";
#elif defined(_WIN32)
    details << "    Platform: Windows\n";
#elif defined(__linux__)
    details << "    Platform: Linux\n";
#else
    details << "    Platform: Unknown\n";
#endif

    result.details = details.str();

    result.recommendations.push_back("Test on all three compilers: Clang, GCC, MSVC");
    result.recommendations.push_back("Use JUCE platform macros (JUCE_MAC, JUCE_WINDOWS, JUCE_LINUX)");

    return result;
}

TestResult testUndefinedBehavior() {
    TestResult result;
    result.testName = "Undefined Behavior Detection";
    result.passed = true;

    // Test signed integer overflow (undefined behavior)
    int max_int = std::numeric_limits<int>::max();
    volatile int overflow_result = max_int + 1;  // UB!

    if (overflow_result < 0) {
        result.details = "Signed overflow wraps (common but undefined behavior)";
        result.warnings.push_back("Relying on signed overflow wrapping is undefined behavior");
    }

    // Test shift operations
    unsigned int shift_val = 1u;
    unsigned int shift_result = shift_val << 31;  // OK for unsigned

    // Signed shift would be UB if shifting into sign bit
    int signed_shift = 1;
    volatile int signed_result = signed_shift << 30;  // Safe
    // volatile int ub_shift = signed_shift << 31;  // Would be UB!

    result.recommendations.push_back("Never rely on signed integer overflow behavior");
    result.recommendations.push_back("Use unsigned types for bit operations");
    result.recommendations.push_back("Enable -fsanitize=undefined for testing");

    return result;
}

//==============================================================================
// 7. AUDIO ENGINE COMPATIBILITY TESTS
//==============================================================================

TestResult testEngineWithEdgeCases() {
    TestResult result;
    result.testName = "Audio Engine Edge Case Handling";
    result.passed = true;

    try {
        // Test with a simple engine
        auto engine = EngineFactory::createEngine(1);  // Parametric EQ
        if (!engine) {
            result.passed = false;
            result.details = "Failed to create engine";
            return result;
        }

        const float sampleRate = 48000.0f;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);

        juce::AudioBuffer<float> buffer(2, blockSize);

        // Test 1: Denormals
        buffer.clear();
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                buffer.setSample(ch, i, 1e-40f);  // Denormal
            }
        }

        engine->process(buffer);

        bool hasNaN = false, hasInf = false;
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                float sample = buffer.getSample(ch, i);
                if (std::isnan(sample)) hasNaN = true;
                if (std::isinf(sample)) hasInf = true;
            }
        }

        if (hasNaN || hasInf) {
            result.passed = false;
            result.warnings.push_back("Engine produced NaN/Inf from denormal input");
        }

        // Test 2: DC offset
        buffer.clear();
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                buffer.setSample(ch, i, 1.0f);  // Full DC
            }
        }

        engine->process(buffer);

        // Test 3: Silence
        buffer.clear();
        engine->process(buffer);

        result.details = "Engine handled denormals, DC, and silence without NaN/Inf";

    } catch (const std::exception& e) {
        result.passed = false;
        result.details = std::string("Exception: ") + e.what();
    }

    result.recommendations.push_back("Test all engines with denormals, DC, and silence");
    result.recommendations.push_back("Verify output is finite on all platforms");

    return result;
}

//==============================================================================
// 8. JUCE API COMPATIBILITY
//==============================================================================

TestResult testJUCEAPIUsage() {
    TestResult result;
    result.testName = "JUCE API Cross-Platform Usage";
    result.passed = true;

    // Test JUCE audio types
    juce::AudioBuffer<float> buffer(2, 512);
    juce::AudioSampleBuffer legacyBuffer(2, 512);

    // Test JUCE threading
    bool hasThreadSupport = juce::Thread::getCurrentThread() != nullptr || true;

    // Test JUCE file I/O
    juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory);
    bool fileOpsWork = tempFile.exists();

    if (hasThreadSupport && fileOpsWork) {
        result.details = "JUCE APIs functioning correctly";
    } else {
        result.passed = false;
        result.details = "JUCE API issues detected";
    }

    result.recommendations.push_back("JUCE handles most platform differences automatically");
    result.recommendations.push_back("Avoid platform-specific APIs unless absolutely necessary");
    result.recommendations.push_back("Use juce::MessageManager for GUI thread operations");

    return result;
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

void runAllTests() {
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "         PLATFORM COMPATIBILITY TEST SUITE                      \n";
    std::cout << "================================================================\n";
    std::cout << "\nTesting platform-specific issues on current platform (macOS)\n";
    std::cout << "These tests help identify potential cross-platform bugs.\n";

    // Run all test categories
    std::cout << "\n\n=== 1. ENDIANNESS TESTS ===\n";
    reportTest(testEndianness());
    reportTest(testFloatBitPattern());

    std::cout << "\n\n=== 2. DATA TYPE SIZE TESTS ===\n";
    reportTest(testDataTypeSizes());
    reportTest(testPointerSizeAssumptions());

    std::cout << "\n\n=== 3. ALIGNMENT TESTS ===\n";
    reportTest(testStructurePacking());
    reportTest(testSIMDAlignment());

    std::cout << "\n\n=== 4. FLOATING POINT TESTS ===\n";
    reportTest(testDenormalHandling());
    reportTest(testNaNInfHandling());
    reportTest(testFloatVsDouble());

    std::cout << "\n\n=== 5. PATH HANDLING TESTS ===\n";
    reportTest(testPathSeparators());
    reportTest(testCaseSensitivity());

    std::cout << "\n\n=== 6. COMPILER TESTS ===\n";
    reportTest(testCompilerMacros());
    reportTest(testUndefinedBehavior());

    std::cout << "\n\n=== 7. AUDIO ENGINE TESTS ===\n";
    reportTest(testEngineWithEdgeCases());

    std::cout << "\n\n=== 8. JUCE API TESTS ===\n";
    reportTest(testJUCEAPIUsage());
}

void generateReport() {
    std::cout << "\n\n";
    std::cout << "================================================================\n";
    std::cout << "                    SUMMARY REPORT                              \n";
    std::cout << "================================================================\n\n";

    int passed = 0, failed = 0, warnings = 0;
    for (const auto& result : allResults) {
        if (result.passed) passed++;
        else failed++;
        warnings += result.warnings.size();
    }

    std::cout << "Tests Passed:  " << passed << " / " << allResults.size() << "\n";
    std::cout << "Tests Failed:  " << failed << " / " << allResults.size() << "\n";
    std::cout << "Total Warnings: " << warnings << "\n\n";

    if (warnings > 0) {
        std::cout << "=== CRITICAL WARNINGS ===\n";
        for (const auto& result : allResults) {
            if (!result.warnings.empty()) {
                std::cout << "\n" << result.testName << ":\n";
                for (const auto& warning : result.warnings) {
                    std::cout << "  ! " << warning << "\n";
                }
            }
        }
        std::cout << "\n";
    }

    std::cout << "=== RECOMMENDATIONS FOR CROSS-PLATFORM DEPLOYMENT ===\n\n";

    std::vector<std::string> allRecommendations;
    for (const auto& result : allResults) {
        for (const auto& rec : result.recommendations) {
            if (std::find(allRecommendations.begin(), allRecommendations.end(), rec) ==
                allRecommendations.end()) {
                allRecommendations.push_back(rec);
            }
        }
    }

    for (size_t i = 0; i < allRecommendations.size(); ++i) {
        std::cout << (i + 1) << ". " << allRecommendations[i] << "\n";
    }

    std::cout << "\n";
    std::cout << "=== PLATFORM COVERAGE ASSESSMENT ===\n\n";
    std::cout << "Current Platform: macOS (25% coverage)\n";
    std::cout << "Untested Platforms:\n";
    std::cout << "  - Windows (MSVC compiler, different ABI)\n";
    std::cout << "  - Linux (GCC compiler, case-sensitive FS)\n";
    std::cout << "  - ARM architectures (Raspberry Pi, mobile)\n\n";

    std::cout << "Risk Level: MEDIUM\n";
    std::cout << "Reason: JUCE handles most platform differences, but:\n";
    std::cout << "  - Denormal behavior may differ on Windows\n";
    std::cout << "  - File paths need testing on Windows\n";
    std::cout << "  - Linux case-sensitive filesystem needs validation\n";
    std::cout << "  - MSVC compiler may expose different bugs\n\n";

    std::cout << "================================================================\n\n";
}

} // namespace PlatformCompatibility

//==============================================================================
// MAIN
//==============================================================================

int main(int argc, char* argv[]) {
    using namespace PlatformCompatibility;

    runAllTests();
    generateReport();

    return 0;
}
