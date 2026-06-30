#ifndef GSDENSITY_GRLE_ENCODER_H
#define GSDENSITY_GRLE_ENCODER_H

#include "common.h"
#include <vector>

namespace gsdensity {

class GRLEEncoder {
public:
    // Encode Image to GRLE format
    static std::vector<uint8_t> encode(const Image& image);
    
    // Encode Image from file to GRLE file
    static void encode_file(const std::string& input_png, 
                           const std::string& output_grle);
    
    // Encode Image object directly to GRLE file
    static void encode_file_from_image(const Image& image,
                                       const std::string& output_grle);
    
private:
    // Create GRLE header
    static std::vector<uint8_t> create_header(uint32_t width, uint32_t height,
                                               uint32_t compressed_size);
    
    // Encode data using RLE compression
    static std::vector<uint8_t> encode_rle(const std::vector<uint8_t>& pixels);
};

} // namespace gsdensity

#endif // GSDENSITY_GRLE_ENCODER_H
