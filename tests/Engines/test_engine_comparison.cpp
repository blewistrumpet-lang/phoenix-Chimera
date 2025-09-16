#include <iostream>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/PluginProcessor.h"

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "Comparing Engine Creation\n";
    std::cout << "==========================\n\n";
    
    // Test BitCrusher (which works)
    std::cout << "Testing BitCrusher (ID 18):\n";
    auto bitcrusher = EngineFactory::createEngine(18);
    if (bitcrusher) {
        std::cout << "  Created: YES\n";
        std::cout << "  Name: " << bitcrusher->getName().toStdString() << "\n";
        std::cout << "  Parameters: " << bitcrusher->getNumParameters() << "\n";
        for (int i = 0; i < bitcrusher->getNumParameters(); ++i) {
            std::cout << "    " << i << ": " << bitcrusher->getParameterName(i).toStdString() << "\n";
        }
    } else {
        std::cout << "  Created: NO\n";
    }
    
    std::cout << "\nTesting VintageOptoCompressor (ID 1):\n";
    auto opto = EngineFactory::createEngine(1);
    if (opto) {
        std::cout << "  Created: YES\n";
        std::cout << "  Name: " << opto->getName().toStdString() << "\n";
        std::cout << "  Parameters: " << opto->getNumParameters() << "\n";
        for (int i = 0; i < opto->getNumParameters(); ++i) {
            std::cout << "    " << i << ": " << opto->getParameterName(i).toStdString() << "\n";
        }
    } else {
        std::cout << "  Created: NO\n";
    }
    
    std::cout << "\nTesting ClassicCompressor (ID 2):\n";
    auto comp = EngineFactory::createEngine(2);
    if (comp) {
        std::cout << "  Created: YES\n";
        std::cout << "  Name: " << comp->getName().toStdString() << "\n";
        std::cout << "  Parameters: " << comp->getNumParameters() << "\n";
        for (int i = 0; i < comp->getNumParameters(); ++i) {
            std::cout << "    " << i << ": " << comp->getParameterName(i).toStdString() << "\n";
        }
    } else {
        std::cout << "  Created: NO\n";
    }
    
    // Now test in ChimeraAudioProcessor context
    std::cout << "\n\nTesting in Processor Context:\n";
    std::cout << "==============================\n";
    ChimeraAudioProcessor processor;
    
    // Check what the dropdown shows
    auto* param = processor.getValueTreeState().getParameter("slot1_engine");
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param)) {
        std::cout << "Engine dropdown choices:\n";
        for (int i = 0; i < juce::jmin(25, choiceParam->choices.size()); ++i) {
            std::cout << "  " << i << ": " << choiceParam->choices[i].toStdString() << "\n";
        }
    }
    
    return 0;
}