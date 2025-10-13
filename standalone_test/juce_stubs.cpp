// Stubs for missing JUCE symbols needed by header-only code

namespace juce {

const char* juce_compilationDate = __DATE__;
const char* juce_compilationTime = __TIME__;

// Stubbed Colour class for code that includes JuceHeader.h but doesn't use it
namespace {
    class DummyColour {
    public:
        DummyColour(unsigned int) {}
    };
}

#ifndef JUCE_COLOUR_H_INCLUDED
// Only define if Colour isn't already defined
class Colour {
public:
    Colour(unsigned int) {}
};
#endif

}
