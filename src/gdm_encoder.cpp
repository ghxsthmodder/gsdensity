#include "gdm_encoder.h"
#include "image_utils.h"
#include <cstring>
#include <algorithm>
#include <set>
#include <map>

namespace gsdensity {

std::vector<uint8_t> GDMEncoder::create_header(uint32_t dimension,
                                                const EncodeOptions& options) {
    std::vector<uint8_t> header;
    
    // Calculate dim_log2
    uint8_t dim_log2 = 0;
    uint32_t test_dim = 32;
    while (test_dim < dimension && dim_log2 < 8) {
        test_dim *= 2;
        dim_log2++;
    }
    
    if (test_dim != dimension) {
        throw GSDensityException("Dimension must be power of 2 times 32");
    }
    
    if (options.use_extended_format) {
        // '"MDF' format (16 bytes)
        header.resize(16);
        header[0] = '"';
        header[1] = 'M';
        header[2] = 'D';
        header[3] = 'F';
        
        // Version (always 0)
        write_u32_le(header.data() + 4, 0);
        
        header[8] = dim_log2;
        header[9] = 5;  // chunk_log2 (always 32)
        header[10] = 2; // max_bpp
        header[11] = options.num_channels;
        header[12] = options.num_compression_ranges;
        header[13] = 0; // type_index_channels
        header[14] = 0; // reserved
        header[15] = 0; // reserved
        
    } else {
        // '!MDF' format (9 bytes)
        header.resize(9);
        header[0] = '!';
        header[1] = 'M';
        header[2] = 'D';
        header[3] = 'F';
        
        header[4] = dim_log2;
        header[5] = 5;  // chunk_log2 (always 32)
        header[6] = 2;  // max_bpp
        header[7] = options.num_channels;
        header[8] = options.num_compression_ranges;
    }
    
    return header;
}

uint8_t GDMEncoder::find_optimal_bit_depth(const std::vector<uint16_t>& chunk_data) {
    // Find unique values
    std::set<uint16_t> unique_values(chunk_data.begin(), chunk_data.end());
    
    // If all same, use bit_depth 0
    if (unique_values.size() == 1) {
        return 0;
    }
    
    // Find minimum bit depth needed
    size_t num_unique = unique_values.size();
    
    if (num_unique <= 2) return 1;
    if (num_unique <= 4) return 2;
    if (num_unique <= 8) return 3;
    if (num_unique <= 16) return 4;
    if (num_unique <= 32) return 5;
    
    return 6;  // Maximum for most cases
}

std::vector<uint16_t> GDMEncoder::create_palette(const std::vector<uint16_t>& chunk_data,
                                                  uint8_t bit_depth) {
    if (bit_depth == 0) {
        // Uniform block - single palette entry
        return {chunk_data[0]};
    }
    
    if (bit_depth > 2) {
        // No palette for higher bit depths
        return {};
    }
    
    // Create palette from unique values
    std::set<uint16_t> unique_values(chunk_data.begin(), chunk_data.end());
    std::vector<uint16_t> palette(unique_values.begin(), unique_values.end());
    
    // Sort for consistency
    std::sort(palette.begin(), palette.end());
    
    // Limit palette size
    size_t max_palette = (1u << bit_depth);
    if (palette.size() > max_palette) {
        palette.resize(max_palette);
    }
    
    return palette;
}

std::vector<uint8_t> GDMEncoder::encode_bitmap(const std::vector<uint16_t>& chunk_data,
                                                const std::vector<uint16_t>& palette,
                                                uint8_t bit_depth) {
    if (bit_depth == 0) {
        return {};  // No bitmap for uniform blocks
    }
    
    size_t bitmap_size = bit_depth * 128;
    std::vector<uint8_t> bitmap(bitmap_size, 0);
    
    // Create reverse palette lookup
    std::map<uint16_t, uint16_t> palette_map;
    if (bit_depth <= 2 && !palette.empty()) {
        for (size_t i = 0; i < palette.size(); i++) {
            palette_map[palette[i]] = i;
        }
    }
    
    // Encode each pixel
    for (size_t pixel_idx = 0; pixel_idx < chunk_data.size(); pixel_idx++) {
        uint16_t value = chunk_data[pixel_idx];
        
        // Get index or raw value
        uint16_t index;
        if (bit_depth <= 2 && !palette.empty()) {
            auto it = palette_map.find(value);
            index = (it != palette_map.end()) ? it->second : 0;
        } else {
            index = value;
        }
        
        // Pack into bitmap
        size_t bit_pos = pixel_idx * bit_depth;
        size_t byte_idx = bit_pos / 8;
        size_t bit_offset = bit_pos % 8;
        
        if (byte_idx < bitmap.size()) {
            bitmap[byte_idx] |= (index & 0xFF) << bit_offset;
            
            // Handle overflow to next byte
            if (bit_offset + bit_depth > 8 && byte_idx + 1 < bitmap.size()) {
                bitmap[byte_idx + 1] |= (index >> (8 - bit_offset)) & 0xFF;
            }
        }
    }
    
    return bitmap;
}

std::vector<uint8_t> GDMEncoder::encode_chunk(const std::vector<uint16_t>& chunk_data,
                                               uint8_t bits_per_pixel) {
    std::vector<uint8_t> block;
    
    // Find optimal bit depth
    uint8_t bit_depth = find_optimal_bit_depth(chunk_data);
    
    // Create palette
    std::vector<uint16_t> palette = create_palette(chunk_data, bit_depth);
    
    // Write block header
    block.push_back(bit_depth);
    block.push_back(static_cast<uint8_t>(palette.size()));
    
    // Write palette
    for (uint16_t value : palette) {
        uint8_t bytes[2];
        write_u16_le(bytes, value);
        block.push_back(bytes[0]);
        block.push_back(bytes[1]);
    }
    
    // Encode and write bitmap
    if (bit_depth > 0) {
        std::vector<uint8_t> bitmap = encode_bitmap(chunk_data, palette, bit_depth);
        block.insert(block.end(), bitmap.begin(), bitmap.end());
    }
    
    return block;
}

std::vector<uint8_t> GDMEncoder::encode(const Image& image, const EncodeOptions& options) {
    // Validate input
    uint32_t dimension = image.width;
    if (image.width != image.height) {
        throw GSDensityException("GDM requires square images");
    }
    
    if (dimension % 32 != 0) {
        throw GSDensityException("GDM dimensions must be multiples of 32");
    }
    
    // Create header
    std::vector<uint8_t> result = create_header(dimension, options);
    
    // Write compression boundaries if needed
    if (options.num_compression_ranges > 1) {
        if (options.compression_boundaries.size() != options.num_compression_ranges - 1) {
            throw GSDensityException("Invalid compression boundaries");
        }
        result.insert(result.end(), 
                     options.compression_boundaries.begin(),
                     options.compression_boundaries.end());
    }
    
    // Calculate bits per range
    std::vector<uint8_t> boundaries = {0};
    boundaries.insert(boundaries.end(),
                     options.compression_boundaries.begin(),
                     options.compression_boundaries.end());
    boundaries.push_back(options.num_channels);
    
    std::vector<uint8_t> bits_per_range;
    for (size_t i = 0; i < options.num_compression_ranges; i++) {
        bits_per_range.push_back(boundaries[i + 1] - boundaries[i]);
    }
    
    // Process chunks
    uint32_t chunk_size = 32;
    uint32_t chunks_per_dim = dimension / chunk_size;
    
    for (uint32_t chunk_row = 0; chunk_row < chunks_per_dim; chunk_row++) {
        for (uint32_t chunk_col = 0; chunk_col < chunks_per_dim; chunk_col++) {
            uint32_t base_x = chunk_col * chunk_size;
            uint32_t base_y = chunk_row * chunk_size;
            
            // Extract chunk data for each range
            for (uint8_t range_idx = 0; range_idx < options.num_compression_ranges; range_idx++) {
                std::vector<uint16_t> chunk_data;
                chunk_data.reserve(chunk_size * chunk_size);
                
                uint8_t shift = 0;
                for (uint8_t r = 0; r < range_idx; r++) {
                    shift += bits_per_range[r];
                }
                
                uint16_t mask = (1u << bits_per_range[range_idx]) - 1;
                
                // Extract pixels for this chunk and range
                for (uint32_t py = 0; py < chunk_size; py++) {
                    for (uint32_t px = 0; px < chunk_size; px++) {
                        uint32_t x = base_x + px;
                        uint32_t y = base_y + py;
                        
                        if (x >= dimension || y >= dimension) {
                            chunk_data.push_back(0);
                            continue;
                        }
                        
                        const uint8_t* pixel = image.pixel_at(x, y);
                        
                        // Reconstruct full value from pixel
                        uint32_t full_value = 0;
                        if (image.channels == 1) {
                            full_value = pixel[0];
                        } else if (image.channels == 3) {
                            full_value = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
                        }
                        
                        // Extract range bits
                        uint16_t range_value = (full_value >> shift) & mask;
                        chunk_data.push_back(range_value);
                    }
                }
                
                // Encode chunk
                std::vector<uint8_t> block = encode_chunk(chunk_data, bits_per_range[range_idx]);
                result.insert(result.end(), block.begin(), block.end());
            }
        }
    }
    
    return result;
}

void GDMEncoder::encode_file(const std::string& input_png,
                             const std::string& output_gdm,
                             const EncodeOptions& options) {
    // Read PNG
    Image image = ImageUtils::read_png(input_png);
    
    // Encode
    std::vector<uint8_t> gdm_data = encode(image, options);
    
    // Write output
    ImageUtils::write_binary_file(output_gdm, gdm_data);
}

} // namespace gsdensity
