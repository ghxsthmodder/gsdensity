#include "grle_encoder.h"
#include "image_utils.h"
#include <cstring>
#include <algorithm>

namespace gsdensity {

std::vector<uint8_t> GRLEEncoder::create_header(uint32_t width, uint32_t height,
                                                 uint32_t compressed_size) {
    std::vector<uint8_t> header(20);
    
    // Magic "GRLE"
    header[0] = 'G';
    header[1] = 'R';
    header[2] = 'L';
    header[3] = 'E';
    
    // Version (always 1)
    write_u16_le(header.data() + 4, 1);
    
    // Width / 256
    write_u16_le(header.data() + 6, width / 256);
    
    // Reserved
    write_u16_le(header.data() + 8, 0);
    
    // Height / 256
    write_u16_le(header.data() + 10, height / 256);
    
    // Reserved
    header[12] = 0;
    
    // Channels (always 1)
    header[13] = 1;
    
    // Reserved
    write_u16_le(header.data() + 14, 0);
    
    // Compressed size
    write_u32_le(header.data() + 16, compressed_size);
    
    return header;
}

std::vector<uint8_t> GRLEEncoder::encode_rle(const std::vector<uint8_t>& pixels) {
    std::vector<uint8_t> compressed;
    
    // Add padding byte at start
    compressed.push_back(0);
    
    if (pixels.empty()) {
        return compressed;
    }
    
    size_t i = 0;
    while (i < pixels.size()) {
        uint8_t value = pixels[i];
        size_t run_length = 1;
        
        // Count consecutive identical values
        while (i + run_length < pixels.size() && 
               pixels[i + run_length] == value &&
               run_length < 100000) {  // Reasonable limit
            run_length++;
        }
        
        if (run_length >= 2) {
            // Encode as RUN
            compressed.push_back(value);
            compressed.push_back(value);
            
            // Encode count (offset by 2)
            size_t count = run_length - 2;
            
            // Write extended count bytes (0xFF for each 255)
            while (count >= 255) {
                compressed.push_back(0xFF);
                count -= 255;
            }
            
            // Write final count byte
            compressed.push_back(static_cast<uint8_t>(count));
            
            i += run_length;
        } else {
            // Single value - encode as transition
            if (i + 1 < pixels.size()) {
                uint8_t next_value = pixels[i + 1];
                compressed.push_back(value);
                compressed.push_back(next_value);
                i++;
            } else {
                // Last pixel - encode as tiny run
                compressed.push_back(value);
                compressed.push_back(value);
                compressed.push_back(0);  // count = 0 + 2 = 2
                i++;
            }
        }
    }
    
    return compressed;
}

std::vector<uint8_t> GRLEEncoder::encode(const Image& image) {
    // Validate input
    if (image.channels != 1) {
        throw GSDensityException("GRLE encoding requires grayscale image (1 channel)");
    }
    
    if (image.width % 256 != 0 || image.height % 256 != 0) {
        throw GSDensityException("GRLE dimensions must be multiples of 256");
    }
    
    if (image.width > 65536 || image.height > 65536) {
        throw GSDensityException("GRLE dimensions too large");
    }
    
    // Encode RLE data
    std::vector<uint8_t> compressed = encode_rle(image.data);
    
    // Create header
    std::vector<uint8_t> header = create_header(image.width, image.height, 
                                                 compressed.size());
    
    // Combine header and compressed data
    std::vector<uint8_t> result;
    result.reserve(header.size() + compressed.size());
    result.insert(result.end(), header.begin(), header.end());
    result.insert(result.end(), compressed.begin(), compressed.end());
    
    return result;
}

void GRLEEncoder::encode_file(const std::string& input_png,
                              const std::string& output_grle) {
    // Read PNG
    Image image = ImageUtils::read_png(input_png);
    
    // Convert to grayscale if needed
    if (image.channels == 3) {
        image = ImageUtils::rgb_to_grayscale(image);
    }
    
    // Encode
    std::vector<uint8_t> grle_data = encode(image);
    
    // Write output
    ImageUtils::write_binary_file(output_grle, grle_data);
}

void GRLEEncoder::encode_file_from_image(const Image& image,
                                         const std::string& output_grle) {
    // Encode
    std::vector<uint8_t> grle_data = encode(image);
    
    // Write output
    ImageUtils::write_binary_file(output_grle, grle_data);
}

} // namespace gsdensity
