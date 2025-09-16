#include <iostream>
#include <iomanip>
#include <cmath>

int main() {
    std::cout << "REVERB FEEDBACK ANALYSIS\n";
    std::cout << "========================\n\n";
    
    // Calculate required feedback for different RT60 values
    std::cout << "Required feedback coefficients for various RT60:\n";
    std::cout << "(Assuming average delay of 50ms)\n\n";
    
    float delay_ms = 50.0f;
    float delay_sec = delay_ms / 1000.0f;
    
    std::cout << "RT60 | Required Feedback | Current Max\n";
    std::cout << "-----|-------------------|------------\n";
    
    float rt60_values[] = {0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f};
    
    for (float rt60 : rt60_values) {
        // Formula: feedback = 10^(-3 * delay_time / RT60)
        float required_feedback = std::pow(10.0f, -3.0f * delay_sec / rt60);
        
        std::cout << std::fixed << std::setprecision(1) << std::setw(4) << rt60 << "s | "
                  << std::fixed << std::setprecision(4) << required_feedback << "          | ";
        
        // Current limits in our code
        if (rt60 <= 1.0f) {
            std::cout << "0.8200 (PlateReverb)";
        } else {
            std::cout << "0.8200 (TOO LOW!)";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\nCurrent feedback limits in code:\n";
    std::cout << "PlateReverb MAX_FEEDBACK: 0.82\n";
    std::cout << "SpringReverb self-feedback: 0.65\n";
    std::cout << "ShimmerReverb max: 0.82\n";
    
    std::cout << "\n\nGain staging analysis:\n";
    std::cout << "PlateReverb FDN:\n";
    std::cout << "  inputGain: 0.25\n";
    std::cout << "  feedbackGain: 0.55\n";
    std::cout << "  feedback multiplier: 0.85\n";
    std::cout << "  Total loop gain: 0.55 * 0.85 = " << (0.55f * 0.85f) << "\n";
    std::cout << "  With MAX_FEEDBACK: 0.55 * 0.85 * 0.82 = " << (0.55f * 0.85f * 0.82f) << "\n";
    
    std::cout << "\nProblem: Total loop gain of " << (0.55f * 0.85f * 0.82f) 
              << " is too low for proper reverb tails!\n";
    std::cout << "Need at least 0.85-0.90 for 2-3 second tails.\n";
    
    return 0;
}
