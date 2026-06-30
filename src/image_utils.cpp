#include "image_utils.h"
#include <fstream>
#include <cstring>
#include <algorithm>

// Simple PNG implementation without external dependencies
// This is a minimal implementation for reading/writing 8-bit grayscale and RGB PNGs

namespace gsdensity {

// PNG signature
static const uint8_t PNG_SIGNATURE[8] = {137, 80, 78, 71, 13, 10, 26, 10};

// PNG chunk types
static const uint32_t IHDR = 0x49484452;
static const uint32_t IDAT = 0x49444154;
static const uint32_t IEND = 0x49454E44;

// CRC32 table for PNG
static uint32_t crc_table[256];
static bool crc_table_computed = false;

static void make_crc_table() {
    if (crc_table_computed) return;
    
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
    crc_table_computed = true;
}

static uint32_t update_crc(uint32_t crc, const uint8_t* buf, size_t len) {
    uint32_t c = crc;
    make_crc_table();
    
    for (size_t n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }
    return c;
}

static uint32_t crc(const uint8_t* buf, size_t len) {
    return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}

// Simple deflate/inflate (uncompressed blocks only for simplicity)
static std::vector<uint8_t> simple_deflate(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> result;
    
    // Zlib header (no compression)
    result.push_back(0x78);  // CMF
    result.push_back(0x01);  // FLG
    
    size_t pos = 0;
    while (pos < data.size()) {
        size_t block_size = std::min(size_t(65535), data.size() - pos);
        bool is_final = (pos + block_size >= data.size());
        
        // Block header
        result.push_back(is_final ? 0x01 : 0x00);
        
        // Block size (little endian)
        result.push_back(block_size & 0xFF);
        result.push_back((block_size >> 8) & 0xFF);
        
        // One's complement of block size
        uint16_t nlen = ~block_size;
        result.push_back(nlen & 0xFF);
        result.push_back((nlen >> 8) & 0xFF);
        
        // Block data
        result.insert(result.end(), data.begin() + pos, data.begin() + pos + block_size);
        pos += block_size;
    }
    
    // Adler32 checksum
    uint32_t s1 = 1, s2 = 0;
    for (uint8_t byte : data) {
        s1 = (s1 + byte) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    uint32_t adler = (s2 << 16) | s1;
    
    result.push_back((adler >> 24) & 0xFF);
    result.push_back((adler >> 16) & 0xFF);
    result.push_back((adler >> 8) & 0xFF);
    result.push_back(adler & 0xFF);
    
    return result;
}

static std::vector<uint8_t> simple_inflate(const uint8_t* data, size_t size) {
    std::vector<uint8_t> result;
    
    // Skip zlib header
    if (size < 2) throw IOError("Invalid compressed data");
    size_t pos = 2;
    
    while (pos < size - 4) {  // -4 for Adler32
        if (pos >= size) break;
        
        uint8_t header = data[pos++];
        bool is_final = header & 0x01;
        uint8_t btype = (header >> 1) & 0x03;
        
        if (btype != 0) {
            throw IOError("Only uncompressed blocks supported");
        }
        
        if (pos + 4 > size) break;
        
        uint16_t len = data[pos] | (data[pos + 1] << 8);
        pos += 4;  // Skip len and nlen
        
        if (pos + len > size - 4) break;
        
        result.insert(result.end(), data + pos, data + pos + len);
        pos += len;
        
        if (is_final) break;
    }
    
    return result;
}

Image ImageUtils::read_png(const std::string& filename) {
    auto data = read_binary_file(filename);
    
    if (data.size() < 8 || memcmp(data.data(), PNG_SIGNATURE, 8) != 0) {
        throw FileFormatException("Not a valid PNG file");
    }
    
    Image image;
    std::vector<uint8_t> idat_data;
    size_t pos = 8;
    
    while (pos < data.size()) {
        if (pos + 12 > data.size()) break;
        
        uint32_t length = read_u32_le(data.data() + pos);
        // PNG uses big-endian
        length = ((length & 0xFF) << 24) | ((length & 0xFF00) << 8) |
                 ((length & 0xFF0000) >> 8) | ((length & 0xFF000000) >> 24);
        
        uint32_t type = read_u32_le(data.data() + pos + 4);
        type = ((type & 0xFF) << 24) | ((type & 0xFF00) << 8) |
               ((type & 0xFF0000) >> 8) | ((type & 0xFF000000) >> 24);
        
        pos += 8;
        
        if (pos + length + 4 > data.size()) break;
        
        if (type == IHDR) {
            if (length < 13) throw FileFormatException("Invalid IHDR chunk");
            
            uint32_t width = read_u32_le(data.data() + pos);
            width = ((width & 0xFF) << 24) | ((width & 0xFF00) << 8) |
                    ((width & 0xFF0000) >> 8) | ((width & 0xFF000000) >> 24);
            
            uint32_t height = read_u32_le(data.data() + pos + 4);
            height = ((height & 0xFF) << 24) | ((height & 0xFF00) << 8) |
                     ((height & 0xFF0000) >> 8) | ((height & 0xFF000000) >> 24);
            
            uint8_t bit_depth = data[pos + 8];
            uint8_t color_type = data[pos + 9];
            
            if (bit_depth != 8) {
                throw FileFormatException("Only 8-bit depth supported");
            }
            
            if (color_type == 0) {
                image.channels = 1;  // Grayscale
            } else if (color_type == 2) {
                image.channels = 3;  // RGB
            } else {
                throw FileFormatException("Unsupported color type");
            }
            
            image.width = width;
            image.height = height;
            
        } else if (type == IDAT) {
            idat_data.insert(idat_data.end(), data.data() + pos, 
                           data.data() + pos + length);
        } else if (type == IEND) {
            break;
        }
        
        pos += length + 4;  // Skip data and CRC
    }
    
    if (image.width == 0 || image.height == 0) {
        throw FileFormatException("Invalid image dimensions");
    }
    
    // Decompress IDAT
    auto decompressed = simple_inflate(idat_data.data(), idat_data.size());
    
    // Unfilter scanlines
    image.data.resize(image.size());
    size_t bytes_per_pixel = image.channels;
    size_t stride = image.width * bytes_per_pixel;
    
    size_t src_pos = 0;
    for (uint32_t y = 0; y < image.height; y++) {
        if (src_pos >= decompressed.size()) break;
        
        uint8_t filter_type = decompressed[src_pos++];
        
        for (uint32_t x = 0; x < stride; x++) {
            if (src_pos >= decompressed.size()) break;
            
            uint8_t byte = decompressed[src_pos++];
            
            if (filter_type == 0) {
                // No filter
                image.data[y * stride + x] = byte;
            } else if (filter_type == 1) {
                // Sub filter
                uint8_t left = (x >= bytes_per_pixel) ? 
                              image.data[y * stride + x - bytes_per_pixel] : 0;
                image.data[y * stride + x] = byte + left;
            } else {
                // For simplicity, treat other filters as no filter
                image.data[y * stride + x] = byte;
            }
        }
    }
    
    return image;
}

void ImageUtils::write_png(const std::string& filename, const Image& image) {
    std::vector<uint8_t> png_data;
    
    // PNG signature
    png_data.insert(png_data.end(), PNG_SIGNATURE, PNG_SIGNATURE + 8);
    
    // IHDR chunk
    std::vector<uint8_t> ihdr_data(13);
    uint32_t width = image.width;
    uint32_t height = image.height;
    
    // Big-endian width
    ihdr_data[0] = (width >> 24) & 0xFF;
    ihdr_data[1] = (width >> 16) & 0xFF;
    ihdr_data[2] = (width >> 8) & 0xFF;
    ihdr_data[3] = width & 0xFF;
    
    // Big-endian height
    ihdr_data[4] = (height >> 24) & 0xFF;
    ihdr_data[5] = (height >> 16) & 0xFF;
    ihdr_data[6] = (height >> 8) & 0xFF;
    ihdr_data[7] = height & 0xFF;
    
    ihdr_data[8] = 8;  // Bit depth
    ihdr_data[9] = (image.channels == 1) ? 0 : 2;  // Color type (0=gray, 2=RGB)
    ihdr_data[10] = 0;  // Compression
    ihdr_data[11] = 0;  // Filter
    ihdr_data[12] = 0;  // Interlace
    
    // Write IHDR chunk
    uint32_t ihdr_len = 13;
    png_data.push_back((ihdr_len >> 24) & 0xFF);
    png_data.push_back((ihdr_len >> 16) & 0xFF);
    png_data.push_back((ihdr_len >> 8) & 0xFF);
    png_data.push_back(ihdr_len & 0xFF);
    
    png_data.push_back('I');
    png_data.push_back('H');
    png_data.push_back('D');
    png_data.push_back('R');
    
    png_data.insert(png_data.end(), ihdr_data.begin(), ihdr_data.end());
    
    std::vector<uint8_t> ihdr_crc_data = {'I', 'H', 'D', 'R'};
    ihdr_crc_data.insert(ihdr_crc_data.end(), ihdr_data.begin(), ihdr_data.end());
    uint32_t ihdr_crc = crc(ihdr_crc_data.data(), ihdr_crc_data.size());
    
    png_data.push_back((ihdr_crc >> 24) & 0xFF);
    png_data.push_back((ihdr_crc >> 16) & 0xFF);
    png_data.push_back((ihdr_crc >> 8) & 0xFF);
    png_data.push_back(ihdr_crc & 0xFF);
    
    // Prepare image data with filter bytes
    std::vector<uint8_t> filtered_data;
    size_t stride = image.width * image.channels;
    
    for (uint32_t y = 0; y < image.height; y++) {
        filtered_data.push_back(0);  // No filter
        filtered_data.insert(filtered_data.end(),
                           image.data.begin() + y * stride,
                           image.data.begin() + (y + 1) * stride);
    }
    
    // Compress with simple deflate
    auto compressed = simple_deflate(filtered_data);
    
    // Write IDAT chunk
    uint32_t idat_len = compressed.size();
    png_data.push_back((idat_len >> 24) & 0xFF);
    png_data.push_back((idat_len >> 16) & 0xFF);
    png_data.push_back((idat_len >> 8) & 0xFF);
    png_data.push_back(idat_len & 0xFF);
    
    png_data.push_back('I');
    png_data.push_back('D');
    png_data.push_back('A');
    png_data.push_back('T');
    
    png_data.insert(png_data.end(), compressed.begin(), compressed.end());
    
    std::vector<uint8_t> idat_crc_data = {'I', 'D', 'A', 'T'};
    idat_crc_data.insert(idat_crc_data.end(), compressed.begin(), compressed.end());
    uint32_t idat_crc = crc(idat_crc_data.data(), idat_crc_data.size());
    
    png_data.push_back((idat_crc >> 24) & 0xFF);
    png_data.push_back((idat_crc >> 16) & 0xFF);
    png_data.push_back((idat_crc >> 8) & 0xFF);
    png_data.push_back(idat_crc & 0xFF);
    
    // Write IEND chunk
    png_data.push_back(0);
    png_data.push_back(0);
    png_data.push_back(0);
    png_data.push_back(0);
    
    png_data.push_back('I');
    png_data.push_back('E');
    png_data.push_back('N');
    png_data.push_back('D');
    
    std::vector<uint8_t> iend_crc_data = {'I', 'E', 'N', 'D'};
    uint32_t iend_crc = crc(iend_crc_data.data(), iend_crc_data.size());
    
    png_data.push_back((iend_crc >> 24) & 0xFF);
    png_data.push_back((iend_crc >> 16) & 0xFF);
    png_data.push_back((iend_crc >> 8) & 0xFF);
    png_data.push_back(iend_crc & 0xFF);
    
    write_binary_file(filename, png_data);
}

std::vector<uint8_t> ImageUtils::read_binary_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        throw IOError("Cannot open file: " + filename);
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw IOError("Cannot read file: " + filename);
    }
    
    return buffer;
}

void ImageUtils::write_binary_file(const std::string& filename,
                                    const std::vector<uint8_t>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw IOError("Cannot create file: " + filename);
    }
    
    if (!file.write(reinterpret_cast<const char*>(data.data()), data.size())) {
        throw IOError("Cannot write file: " + filename);
    }
}

Image ImageUtils::grayscale_to_rgb(const Image& grayscale) {
    if (grayscale.channels != 1) {
        throw GSDensityException("Input must be grayscale");
    }
    
    Image rgb(grayscale.width, grayscale.height, 3);
    
    for (size_t i = 0; i < grayscale.width * grayscale.height; i++) {
        uint8_t gray = grayscale.data[i];
        rgb.data[i * 3] = gray;
        rgb.data[i * 3 + 1] = gray;
        rgb.data[i * 3 + 2] = gray;
    }
    
    return rgb;
}

Image ImageUtils::rgb_to_grayscale(const Image& rgb) {
    if (rgb.channels != 3) {
        throw GSDensityException("Input must be RGB");
    }
    
    Image grayscale(rgb.width, rgb.height, 1);
    
    for (size_t i = 0; i < rgb.width * rgb.height; i++) {
        // Simple average
        uint8_t r = rgb.data[i * 3];
        uint8_t g = rgb.data[i * 3 + 1];
        uint8_t b = rgb.data[i * 3 + 2];
        grayscale.data[i] = (r + g + b) / 3;
    }
    
    return grayscale;
}

Image ImageUtils::create_image(uint32_t width, uint32_t height, uint8_t channels) {
    return Image(width, height, channels);
}

} // namespace gsdensity
