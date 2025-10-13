// JUCE stub functions for standalone reverb test

#include <cstdio>

namespace juce {
    void logAssertion(const char* filename, int lineNum) {
        fprintf(stderr, "JUCE Assertion at %s:%d\n", filename, lineNum);
    }

    // Debug mode check struct
    namespace this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_debug_mode {
        class this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_debug_mode {
        public:
            this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_debug_mode() {}
        };

        // Create instance to satisfy linker
        this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_debug_mode checker;
    }
}
