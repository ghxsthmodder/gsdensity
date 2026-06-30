#include "../include/gdm_decoder.h"
#include "../include/gdm_encoder.h"
#include "../include/grle_decoder.h"
#include "../include/grle_encoder.h"
#include "../include/image_utils.h"
#include <iostream>
#include <cstring>

using namespace gsdensity;

bool test_gdm_roundtrip() {
    std::cout << "Testing GDM roundtrip encoding..." << std::endl;
    
    try {
        // Decode original
        auto original_image = GDMDecoder::decode_file("../sources/samples/cultivator_density.gdm");
        
        // Encode with same parameters (5 channels, 1 range)
        GDMEncoder::EncodeOptions opts;
        opts.num_channels = 5;
        opts.num_compression_ranges = 1;
        GDMEncoder::encode_file_from_image(original_image, "test_roundtrip.gdm", opts);
        
        // Decode re-encoded
        auto reencoded_image = GDMDecoder::decode_file("test_roundtrip.gdm");
        
        // Compare dimensions
        if (original_image.width != reencoded_image.width ||
            original_image.height != reencoded_image.height ||
            original_image.channels != reencoded_image.channels) {
            std::cerr << "❌ Dimensions mismatch!" << std::endl;
            return false;
        }
        
        // Compare pixel data
        if (original_image.data.size() != reencoded_image.data.size()) {
            std::cerr << "❌ Data size mismatch!" << std::endl;
            return false;
        }
        
        if (memcmp(original_image.data.data(), reencoded_image.data.data(), 
                   original_image.data.size()) != 0) {
            std::cerr << "❌ Pixel data mismatch!" << std::endl;
            return false;
        }
        
        std::cout << "✅ GDM roundtrip: PASSED (pixel-perfect)" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ GDM roundtrip failed: " << e.what() << std::endl;
        return false;
    }
}

bool test_grle_roundtrip() {
    std::cout << "Testing GRLE roundtrip encoding..." << std::endl;
    
    try {
        // Decode original
        auto original_image = GRLEDecoder::decode_file("../sources/samples/mapUSM_farmland.grle");
        
        // Encode
        GRLEEncoder::encode_file_from_image(original_image, "test_roundtrip.grle");
        
        // Decode re-encoded
        auto reencoded_image = GRLEDecoder::decode_file("test_roundtrip.grle");
        
        // Compare dimensions
        if (original_image.width != reencoded_image.width ||
            original_image.height != reencoded_image.height ||
            original_image.channels != reencoded_image.channels) {
            std::cerr << "❌ Dimensions mismatch!" << std::endl;
            return false;
        }
        
        // Compare pixel data
        if (original_image.data.size() != reencoded_image.data.size()) {
            std::cerr << "❌ Data size mismatch!" << std::endl;
            return false;
        }
        
        if (memcmp(original_image.data.data(), reencoded_image.data.data(),
                   original_image.data.size()) != 0) {
            std::cerr << "❌ Pixel data mismatch!" << std::endl;
            return false;
        }
        
        std::cout << "✅ GRLE roundtrip: PASSED (pixel-perfect)" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ GRLE roundtrip failed: " << e.what() << std::endl;
        return false;
    }
}

int main() {
    std::cout << "=== Roundtrip Encoding Tests ===" << std::endl;
    std::cout << "Testing that decode->encode->decode produces identical images" << std::endl;
    std::cout << std::endl;
    
    bool gdm_ok = test_gdm_roundtrip();
    bool grle_ok = test_grle_roundtrip();
    
    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "GDM:  " << (gdm_ok ? "✅ PASS" : "❌ FAIL") << std::endl;
    std::cout << "GRLE: " << (grle_ok ? "✅ PASS" : "❌ FAIL") << std::endl;
    
    return (gdm_ok && grle_ok) ? 0 : 1;
}
