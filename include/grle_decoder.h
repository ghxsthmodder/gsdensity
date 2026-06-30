#ifndef GSDENSITY_GRLE_DECODER_H
#define GSDENSITY_GRLE_DECODER_H

#include "common.h"
#include <vector>

namespace gsdensity {

class GRLEDecoder {
public:
    // Decode GRLE file to Image
    static Image decode(const std::vector<uint8_t>& data);
    
    // Decode GRLE file from path
    static Image decode_file(const std::string& filename);
    
    // Verify GRLE file integrity
    static bool verify(const std::vector<uint8_t>& data, std::string* error = nullptr);
    
private:
    // Parse GRLE header
    static GRLEHeader parse_header(const std::vector<uint8_t>& data);
    
    // Decode RLE compressed data
    static std::vector<uint8_t> decode_rle(const uint8_t* compressed_data,
                                            size_t compressed_size,
                                            size_t expected_pixels);
};

} // namespace gsdensity

#endif // GSDENSITY_GRLE_DECODER_H
