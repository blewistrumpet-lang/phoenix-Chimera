// Minimal test to find crash in SlotComponent
#include <iostream>
#include <vector>
#include <map>

// Mock the parts we need
namespace juce {
    class String {
    public:
        String() {}
        String(const char* s) : str(s) {}
        int getIntValue() const { 
            try {
                return std::stoi(str);
            } catch (...) {
                return 0;
            }
        }
        std::string toString() const { return str; }
    private:
        std::string str;
    };
}

// Simulate the JSON parsing part that might be crashing
void testJSONParsing() {
    std::cout << "Testing JSON parsing logic..." << std::endl;
    
    // This simulates what happens in loadParameterMapping()
    struct ParameterInfo {
        juce::String name;
        juce::String controlType;
    };
    
    std::map<int, std::vector<ParameterInfo>> parameterMap;
    
    // Simulate parsing engine IDs from JSON keys
    std::vector<std::string> engineKeys = {"0", "1", "2", "3", "4", "5", "10", "20", "30", "40", "50", "56"};
    
    for (const auto& key : engineKeys) {
        juce::String engineIdStr(key.c_str());
        int engineId = engineIdStr.getIntValue();
        
        std::cout << "Parsing engine key '" << key << "' -> ID: " << engineId << std::endl;
        
        if (engineId < 0 || engineId > 100) {
            std::cerr << "ERROR: Invalid engine ID!" << std::endl;
        }
        
        // Add dummy parameter
        std::vector<ParameterInfo> params;
        ParameterInfo info;
        info.name = "TestParam";
        info.controlType = "rotary";
        params.push_back(info);
        
        parameterMap[engineId] = params;
    }
    
    std::cout << "Successfully parsed " << parameterMap.size() << " engines" << std::endl;
    
    // Test accessing the map
    for (int i = 0; i < 57; i++) {
        if (parameterMap.count(i) > 0) {
            std::cout << "Engine " << i << " has " << parameterMap[i].size() << " parameters" << std::endl;
        }
    }
}

int main() {
    try {
        std::cout << "Starting SlotComponent crash test..." << std::endl;
        testJSONParsing();
        std::cout << "Test completed successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "CRASH: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "CRASH: Unknown error" << std::endl;
        return 1;
    }
    
    return 0;
}