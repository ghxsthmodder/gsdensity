#include "../include/gdm_decoder.h"
#include "../include/image_utils.h"
#include <iostream>
#include <filesystem>

using namespace gsdensity;
namespace fs = std::filesystem;

int main() {
    std::cout << "=== GDM Decoder Test ===" << std::endl;
    
    // Find GDM files in samples directory
    std::string samples_dir = "../sources/samples";
    
    if (!fs::exists(samples_dir)) {
        samples_dir = "sources/samples";
    }
    
    if (!fs::exists(samples_dir)) {
        std::cerr << "Error: Cannot find samples directory" << std::endl;
        return 1;
    }
    
    int success_count = 0;
    int total_count = 0;
    
    for (const auto& entry : fs::directory_iterator(samples_dir)) {
        if (entry.path().extension() == ".gdm") {
            total_count++;
            std::string input_file = entry.path().string();
            std::string output_file = entry.path().stem().string() + "_decoded.png";
            
            std::cout << "\nTesting: " << entry.path().filename() << std::endl;
            
            try {
                // Decode GDM
                Image image = GDMDecoder::decode_file(input_file);
                
                std::cout << "  ✓ Decoded successfully" << std::endl;
                std::cout << "    Dimensions: " << image.width << "x" << image.height << std::endl;
                std::cout << "    Channels: " << static_cast<int>(image.channels) << std::endl;
                
                // Write PNG
                ImageUtils::write_png(output_file, image);
                std::cout << "    Saved to: " << output_file << std::endl;
                
                success_count++;
                
            } catch (const std::exception& e) {
                std::cerr << "  ✗ Failed: " << e.what() << std::endl;
            }
        }
    }
    
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Tested: " << total_count << " files" << std::endl;
    std::cout << "Success: " << success_count << " files" << std::endl;
    std::cout << "Failed: " << (total_count - success_count) << " files" << std::endl;
    
    return (success_count == total_count) ? 0 : 1;
}
