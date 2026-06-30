#include "../include/grle_encoder.h"
#include "../include/grle_decoder.h"
#include "../include/image_utils.h"
#include <iostream>
#include <filesystem>

using namespace gsdensity;
namespace fs = std::filesystem;

int main() {
    std::cout << "=== GRLE Encoder Test ===" << std::endl;
    
    // First decode existing GRLE files to PNG
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
        if (entry.path().extension() == ".grle") {
            total_count++;
            std::string input_file = entry.path().string();
            std::string png_file = entry.path().stem().string() + "_temp.png";
            std::string output_file = entry.path().stem().string() + "_reencoded.grle";
            
            std::cout << "\nTesting: " << entry.path().filename() << std::endl;
            
            try {
                // Decode original GRLE to PNG
                Image image = GRLEDecoder::decode_file(input_file);
                ImageUtils::write_png(png_file, image);
                
                std::cout << "  ✓ Decoded to PNG" << std::endl;
                
                // Re-encode PNG to GRLE
                GRLEEncoder::encode_file(png_file, output_file);
                
                std::cout << "  ✓ Re-encoded to GRLE" << std::endl;
                
                // Verify by decoding again
                Image reencoded = GRLEDecoder::decode_file(output_file);
                
                std::cout << "  ✓ Verified re-encoded file" << std::endl;
                std::cout << "    Original size: " << fs::file_size(input_file) << " bytes" << std::endl;
                std::cout << "    Re-encoded size: " << fs::file_size(output_file) << " bytes" << std::endl;
                
                // Clean up temp PNG
                fs::remove(png_file);
                
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
