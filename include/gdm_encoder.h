#ifndef GSDENSITY_GDM_ENCODER_H
#define GSDENSITY_GDM_ENCODER_H

#include "common.h"
#include <vector>

namespace gsdensity {

class GDMEncoder {
public:
    struct EncodeOptions {
        uint8_t num_channels = 10;
        uint8_t num_compression_ranges = 1;
        std::vector<uint8_t> compression_boundaries;
        bool use_extended_format = false;  // Use '"MDF' format
        
        EncodeOptions() = default;
    };
    
    // Encode Image to GDM format
    static std::vector<uint8_t> encode(const Image& image, const EncodeOptions& options);
    
    // Encode Image from file to GDM file
    static void encode_file(const std::string& input_png,
                           const std::string& output_gdm,
                           const EncodeOptions& options);
    
private:
    // Create GDM header
    static std::vector<uint8_t> create_header(uint32_t dimension,
                                               const EncodeOptions& options);
    
    // Encode a single chunk
    static std::vector<uint8_t> encode_chunk(const std::vector<uint16_t>& chunk_data,
                                              uint8_t bits_per_pixel);
    
    // Find optimal bit depth for chunk
    static uint8_t find_optimal_bit_depth(const std::vector<uint16_t>& chunk_data);
    
    // Create palette for chunk
    static std::vector<uint16_t> create_palette(const std::vector<uint16_t>& chunk_data,
                                                 uint8_t bit_depth);
    
    // Encode bitmap with palette indices or raw values
    static std::vector<uint8_t> encode_bitmap(const std::vector<uint16_t>& chunk_data,
                                               const std::vector<uint16_t>& palette,
                                               uint8_t bit_depth);
};

} // namespace gsdensity

#endif // GSDENSITY_GDM_ENCODER_H
