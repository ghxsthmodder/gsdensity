#ifndef GSDENSITY_GDM_DECODER_H
#define GSDENSITY_GDM_DECODER_H

#include "common.h"
#include <vector>

namespace gsdensity {

class GDMDecoder {
public:
    // Decode GDM file to Image
    static Image decode(const std::vector<uint8_t>& data);
    
    // Decode GDM file from path
    static Image decode_file(const std::string& filename);
    
    // Verify GDM file integrity
    static bool verify(const std::vector<uint8_t>& data, std::string* error = nullptr);
    
private:
    // Parse GDM header
    static GDMHeader parse_header(const std::vector<uint8_t>& data, size_t& header_size);
    
    // Read compression boundaries
    static std::vector<uint8_t> read_boundaries(const std::vector<uint8_t>& data,
                                                 size_t offset,
                                                 uint8_t num_ranges,
                                                 uint8_t num_channels);
    
    // Decode a single block
    static std::vector<uint16_t> decode_block(const uint8_t* data,
                                               size_t& pos,
                                               uint32_t chunk_size);
    
    // Extract value from bitmap
    static uint16_t extract_bitmap_value(const uint8_t* bitmap,
                                          size_t pixel_idx,
                                          uint8_t bit_depth);
    
    // Combine range values into final pixel value
    static uint32_t combine_ranges(const std::vector<std::vector<uint16_t>>& range_values,
                                    size_t pixel_idx,
                                    const std::vector<uint8_t>& bits_per_range);
};

} // namespace gsdensity

#endif // GSDENSITY_GDM_DECODER_H
