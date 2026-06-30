#include "gdm_decoder.h"
#include "image_utils.h"
#include <cstring>
#include <algorithm>

namespace gsdensity {

GDMHeader GDMDecoder::parse_header(const std::vector<uint8_t>& data, size_t& header_size) {
    if (data.size() < 9) {
        throw FileFormatException("File too small to contain GDM header");
    }
    
    GDMHeader header;
    memcpy(header.magic, data.data(), 4);
    
    if (memcmp(header.magic, "!MDF", 4) == 0) {
        // Simple format (9 bytes)
        header_size = 9;
        header.dim_log2 = data[4];
        header.chunk_log2 = data[5];
        header.max_bpp = data[6];
        header.num_channels = data[7];
        header.num_compression_ranges = data[8];
        header.version = 0;
        header.type_index_channels = 0;
        
    } else if (memcmp(header.magic, "\"MDF", 4) == 0) {
        // Extended format (16 bytes)
        if (data.size() < 16) {
            throw FileFormatException("File too small for extended GDM header");
        }
        
        header_size = 16;
        header.version = read_u32_le(data.data() + 4);
        header.dim_log2 = data[8];
        header.chunk_log2 = data[9];
        header.max_bpp = data[10];
        header.num_channels = data[11];
        header.num_compression_ranges = data[12];
        header.type_index_channels = data[13];
        
        if (header.version != 0) {
            throw FileFormatException("Unsupported GDM version: " + 
                                     std::to_string(header.version));
        }
        
    } else {
        throw FileFormatException("Invalid GDM magic signature");
    }
    
    // Validate
    if (header.num_channels == 0 || header.num_channels > 24) {
        throw FileFormatException("Invalid number of channels: " + 
                                 std::to_string(header.num_channels));
    }
    
    if (header.num_compression_ranges == 0 || header.num_compression_ranges > 6) {
        throw FileFormatException("Invalid number of compression ranges: " + 
                                 std::to_string(header.num_compression_ranges));
    }
    
    return header;
}

std::vector<uint8_t> GDMDecoder::read_boundaries(const std::vector<uint8_t>& data,
                                                  size_t offset,
                                                  uint8_t num_ranges,
                                                  uint8_t num_channels) {
    std::vector<uint8_t> boundaries;
    boundaries.push_back(0);  // Implicit start
    
    // Read boundary bytes
    for (uint8_t i = 0; i < num_ranges - 1; i++) {
        if (offset + i >= data.size()) {
            throw FileFormatException("Unexpected end of file reading boundaries");
        }
        boundaries.push_back(data[offset + i]);
    }
    
    boundaries.push_back(num_channels);  // Implicit end
    
    // Validate boundaries
    for (size_t i = 1; i < boundaries.size(); i++) {
        if (boundaries[i] <= boundaries[i-1]) {
            throw FileFormatException("Invalid compression boundaries");
        }
    }
    
    return boundaries;
}

uint16_t GDMDecoder::extract_bitmap_value(const uint8_t* bitmap,
                                           size_t pixel_idx,
                                           uint8_t bit_depth) {
    // Calculate bit position
    size_t bit_pos = pixel_idx * bit_depth;
    size_t byte_idx = bit_pos / 8;
    size_t bit_offset = bit_pos % 8;
    
    // Read 2 bytes (handles values spanning byte boundaries)
    uint16_t raw_value = bitmap[byte_idx];
    if (byte_idx + 1 < 1024) {  // Reasonable upper bound
        raw_value |= static_cast<uint16_t>(bitmap[byte_idx + 1]) << 8;
    }
    
    // Create mask and extract
    uint16_t mask = (1u << bit_depth) - 1;
    return (raw_value >> bit_offset) & mask;
}

std::vector<uint16_t> GDMDecoder::decode_block(const uint8_t* data,
                                                size_t& pos,
                                                uint32_t chunk_size) {
    uint8_t bit_depth = data[pos];
    uint8_t palette_count = data[pos + 1];
    
    // Read palette
    std::vector<uint16_t> palette;
    for (uint8_t i = 0; i < palette_count; i++) {
        uint16_t value = read_u16_le(data + pos + 2 + i * 2);
        palette.push_back(value);
    }
    
    size_t palette_size = 2 * palette_count;
    size_t bitmap_size = (bit_depth > 0) ? (bit_depth * 128) : 0;
    size_t block_size = 2 + palette_size + bitmap_size;
    
    uint32_t total_pixels = chunk_size * chunk_size;
    std::vector<uint16_t> pixels;
    pixels.reserve(total_pixels);
    
    if (bit_depth == 0) {
        // Uniform block - all pixels have same value
        uint16_t value = palette.empty() ? 0 : palette[0];
        pixels.resize(total_pixels, value);
    } else {
        // Decode bitmap
        const uint8_t* bitmap = data + pos + 2 + palette_size;
        
        for (uint32_t pixel_idx = 0; pixel_idx < total_pixels; pixel_idx++) {
            uint16_t index = extract_bitmap_value(bitmap, pixel_idx, bit_depth);
            
            uint16_t pixel_value;
            if (bit_depth <= 2 && !palette.empty()) {
                // Use palette lookup
                pixel_value = (index < palette.size()) ? palette[index] : 0;
            } else {
                // Use raw value
                pixel_value = index;
            }
            
            pixels.push_back(pixel_value);
        }
    }
    
    pos += block_size;
    return pixels;
}

uint32_t GDMDecoder::combine_ranges(const std::vector<std::vector<uint16_t>>& range_values,
                                     size_t pixel_idx,
                                     const std::vector<uint8_t>& bits_per_range) {
    uint32_t combined = 0;
    uint32_t shift = 0;
    
    for (size_t range_idx = 0; range_idx < range_values.size(); range_idx++) {
        uint32_t value = range_values[range_idx][pixel_idx];
        combined |= (value << shift);
        shift += bits_per_range[range_idx];
    }
    
    return combined;
}

Image GDMDecoder::decode(const std::vector<uint8_t>& data) {
    // Parse header
    size_t header_size;
    GDMHeader header = parse_header(data, header_size);
    
    uint32_t dimension = header.get_dimension();
    uint32_t chunk_size = header.get_chunk_size();
    uint32_t chunks_per_dim = dimension / chunk_size;
    uint32_t total_chunks = chunks_per_dim * chunks_per_dim;
    
    // Read compression boundaries
    size_t boundaries_offset = header_size;
    size_t boundaries_size = (header.num_compression_ranges > 1) ? 
                             (header.num_compression_ranges - 1) : 0;
    
    std::vector<uint8_t> boundaries = read_boundaries(data, boundaries_offset,
                                                       header.num_compression_ranges,
                                                       header.num_channels);
    
    // Calculate bits per range
    std::vector<uint8_t> bits_per_range;
    for (size_t i = 0; i < header.num_compression_ranges; i++) {
        bits_per_range.push_back(boundaries[i + 1] - boundaries[i]);
    }
    
    // Determine output format
    bool use_rgb = (header.num_channels > 8);
    uint8_t output_channels = use_rgb ? 3 : 1;
    
    // Create output image
    Image image(dimension, dimension, output_channels);
    
    // Decode all blocks
    size_t pos = header_size + boundaries_size;
    
    for (uint32_t chunk_idx = 0; chunk_idx < total_chunks; chunk_idx++) {
        // Decode all ranges for this chunk
        std::vector<std::vector<uint16_t>> range_values;
        
        for (uint8_t range_idx = 0; range_idx < header.num_compression_ranges; range_idx++) {
            if (pos >= data.size()) {
                throw FileFormatException("Unexpected end of file reading blocks");
            }
            
            std::vector<uint16_t> pixels = decode_block(data.data(), pos, chunk_size);
            range_values.push_back(std::move(pixels));
        }
        
        // Calculate chunk position in image
        uint32_t chunk_row = chunk_idx / chunks_per_dim;
        uint32_t chunk_col = chunk_idx % chunks_per_dim;
        uint32_t base_x = chunk_col * chunk_size;
        uint32_t base_y = chunk_row * chunk_size;
        
        // Combine ranges and write pixels
        for (uint32_t pixel_idx = 0; pixel_idx < chunk_size * chunk_size; pixel_idx++) {
            uint32_t combined = combine_ranges(range_values, pixel_idx, bits_per_range);
            
            uint32_t px = pixel_idx % chunk_size;
            uint32_t py = pixel_idx / chunk_size;
            uint32_t x = base_x + px;
            uint32_t y = base_y + py;
            
            if (x >= dimension || y >= dimension) continue;
            
            uint8_t* pixel = image.pixel_at(x, y);
            
            if (use_rgb) {
                // RGB output
                pixel[0] = combined & 0xFF;           // R
                pixel[1] = (combined >> 8) & 0xFF;    // G
                pixel[2] = (combined >> 16) & 0xFF;   // B
            } else {
                // Grayscale output
                pixel[0] = combined & 0xFF;
            }
        }
    }
    
    return image;
}

Image GDMDecoder::decode_file(const std::string& filename) {
    auto data = ImageUtils::read_binary_file(filename);
    return decode(data);
}

bool GDMDecoder::verify(const std::vector<uint8_t>& data, std::string* error) {
    try {
        // Try to parse header
        size_t header_size;
        GDMHeader header = parse_header(data, header_size);
        
        // Check basic validity
        uint32_t dimension = header.get_dimension();
        uint32_t chunk_size = header.get_chunk_size();
        
        if (dimension == 0 || chunk_size == 0) {
            if (error) *error = "Invalid dimensions";
            return false;
        }
        
        if (dimension % chunk_size != 0) {
            if (error) *error = "Dimension not divisible by chunk size";
            return false;
        }
        
        // Try to decode (this will catch most issues)
        Image image = decode(data);
        
        if (image.width != dimension || image.height != dimension) {
            if (error) *error = "Decoded image size mismatch";
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

} // namespace gsdensity
