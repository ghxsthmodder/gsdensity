#include "grle_decoder.h"
#include "image_utils.h"
#include <cstring>

namespace gsdensity {

GRLEHeader GRLEDecoder::parse_header(const std::vector<uint8_t>& data) {
    if (data.size() < 20) {
        throw FileFormatException("File too small to contain GRLE header");
    }
    
    GRLEHeader header;
    
    // Check magic
    memcpy(header.magic, data.data(), 4);
    if (memcmp(header.magic, "GRLE", 4) != 0) {
        throw FileFormatException("Invalid GRLE magic signature");
    }
    
    // Parse header fields
    header.version = read_u16_le(data.data() + 4);
    header.width_div_256 = read_u16_le(data.data() + 6);
    header.reserved1 = read_u16_le(data.data() + 8);
    header.height_div_256 = read_u16_le(data.data() + 10);
    header.reserved2 = data[12];
    header.channels = data[13];
    header.reserved3 = read_u16_le(data.data() + 14);
    header.compressed_size = read_u32_le(data.data() + 16);
    
    // Validate
    if (header.version != 1) {
        throw FileFormatException("Unsupported GRLE version: " + 
                                 std::to_string(header.version));
    }
    
    if (header.channels != 1) {
        throw FileFormatException("GRLE must have exactly 1 channel, got: " + 
                                 std::to_string(header.channels));
    }
    
    if (header.get_width() == 0 || header.get_height() == 0) {
        throw FileFormatException("Invalid dimensions");
    }
    
    return header;
}

std::vector<uint8_t> GRLEDecoder::decode_rle(const uint8_t* compressed_data,
                                               size_t compressed_size,
                                               size_t expected_pixels) {
    std::vector<uint8_t> output;
    output.reserve(expected_pixels);
    
    size_t i = 1;  // Skip first byte (padding)
    
    while (i + 1 < compressed_size && output.size() < expected_pixels) {
        uint8_t prev = compressed_data[i];
        uint8_t new_val = compressed_data[i + 1];
        i += 2;
        
        if (prev == new_val) {
            // RUN: same value repeated
            size_t count = 0;
            
            // Read extended count (0xFF bytes chain)
            while (i < compressed_size && compressed_data[i] == 0xFF) {
                count += 255;
                i++;
            }
            
            // Read final count byte
            if (i < compressed_size) {
                count += compressed_data[i];
                i++;
            }
            
            // Counts are offset by 2
            count += 2;
            
            // Emit 'count' copies of the value
            size_t to_emit = std::min(count, expected_pixels - output.size());
            for (size_t j = 0; j < to_emit; j++) {
                output.push_back(new_val);
            }
        } else {
            // TRANSITION: output previous value and back up
            output.push_back(prev);
            i--;  // Back up so 'new_val' becomes next 'prev'
        }
    }
    
    // Pad with zeros if needed
    while (output.size() < expected_pixels) {
        output.push_back(0);
    }
    
    return output;
}

Image GRLEDecoder::decode(const std::vector<uint8_t>& data) {
    // Parse header
    GRLEHeader header = parse_header(data);
    
    uint32_t width = header.get_width();
    uint32_t height = header.get_height();
    size_t expected_pixels = static_cast<size_t>(width) * height;
    
    // Decode RLE data (starts at byte 20)
    const uint8_t* compressed_data = data.data() + 20;
    size_t compressed_size = data.size() - 20;
    
    std::vector<uint8_t> pixels = decode_rle(compressed_data, 
                                              compressed_size, 
                                              expected_pixels);
    
    // Create image
    Image image(width, height, 1);
    image.data = std::move(pixels);
    
    return image;
}

Image GRLEDecoder::decode_file(const std::string& filename) {
    auto data = ImageUtils::read_binary_file(filename);
    return decode(data);
}

bool GRLEDecoder::verify(const std::vector<uint8_t>& data, std::string* error) {
    try {
        // Try to parse header
        GRLEHeader header = parse_header(data);
        
        // Check if we have enough data
        if (data.size() < 20) {
            if (error) *error = "File too small";
            return false;
        }
        
        // Try to decode
        uint32_t width = header.get_width();
        uint32_t height = header.get_height();
        size_t expected_pixels = static_cast<size_t>(width) * height;
        
        const uint8_t* compressed_data = data.data() + 20;
        size_t compressed_size = data.size() - 20;
        
        std::vector<uint8_t> pixels = decode_rle(compressed_data,
                                                  compressed_size,
                                                  expected_pixels);
        
        if (pixels.size() != expected_pixels) {
            if (error) *error = "Decoded pixel count mismatch";
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

} // namespace gsdensity
