#ifndef GSDENSITY_COMMON_H
#define GSDENSITY_COMMON_H

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

namespace gsdensity {

// Exception types
class GSDensityException : public std::runtime_error {
public:
    explicit GSDensityException(const std::string& message)
        : std::runtime_error(message) {}
};

class FileFormatException : public GSDensityException {
public:
    explicit FileFormatException(const std::string& message)
        : GSDensityException("File format error: " + message) {}
};

class IOError : public GSDensityException {
public:
    explicit IOError(const std::string& message)
        : GSDensityException("I/O error: " + message) {}
};

// Image data structure
struct Image {
    uint32_t width;
    uint32_t height;
    uint8_t channels;  // 1 for grayscale, 3 for RGB
    std::vector<uint8_t> data;
    
    Image() : width(0), height(0), channels(0) {}
    
    Image(uint32_t w, uint32_t h, uint8_t ch)
        : width(w), height(h), channels(ch) {
        data.resize(static_cast<size_t>(w) * h * ch);
    }
    
    size_t size() const {
        return static_cast<size_t>(width) * height * channels;
    }
    
    uint8_t* pixel_at(uint32_t x, uint32_t y) {
        size_t offset = (static_cast<size_t>(y) * width + x) * channels;
        return &data[offset];
    }
    
    const uint8_t* pixel_at(uint32_t x, uint32_t y) const {
        size_t offset = (static_cast<size_t>(y) * width + x) * channels;
        return &data[offset];
    }
};

// File header structures
struct GRLEHeader {
    char magic[4];           // "GRLE"
    uint16_t version;        // Always 1
    uint16_t width_div_256;  // Width / 256
    uint16_t reserved1;      // 0x0000
    uint16_t height_div_256; // Height / 256
    uint8_t reserved2;       // 0x00
    uint8_t channels;        // Always 1
    uint16_t reserved3;      // 0x0000
    uint32_t compressed_size;
    
    uint32_t get_width() const { return width_div_256 * 256; }
    uint32_t get_height() const { return height_div_256 * 256; }
};

struct GDMHeader {
    char magic[4];           // "!MDF" or '"MDF'
    uint8_t dim_log2;
    uint8_t chunk_log2;
    uint8_t max_bpp;
    uint8_t num_channels;
    uint8_t num_compression_ranges;
    
    // For '"MDF' format only
    uint32_t version;        // Must be 0
    uint8_t type_index_channels;
    
    bool is_extended_format() const {
        return magic[0] == '"';
    }
    
    uint32_t get_dimension() const {
        return 1u << (dim_log2 + 5);
    }
    
    uint32_t get_chunk_size() const {
        return 1u << chunk_log2;
    }
};

// Utility functions
inline uint16_t read_u16_le(const uint8_t* data) {
    return static_cast<uint16_t>(data[0]) | 
           (static_cast<uint16_t>(data[1]) << 8);
}

inline uint32_t read_u32_le(const uint8_t* data) {
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

inline void write_u16_le(uint8_t* data, uint16_t value) {
    data[0] = static_cast<uint8_t>(value & 0xFF);
    data[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

inline void write_u32_le(uint8_t* data, uint32_t value) {
    data[0] = static_cast<uint8_t>(value & 0xFF);
    data[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    data[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

} // namespace gsdensity

#endif // GSDENSITY_COMMON_H
