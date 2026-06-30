#ifndef GSDENSITY_IMAGE_UTILS_H
#define GSDENSITY_IMAGE_UTILS_H

#include "common.h"
#include <string>

namespace gsdensity {

class ImageUtils {
public:
    // Read PNG file into Image structure
    static Image read_png(const std::string& filename);
    
    // Write Image structure to PNG file
    static void write_png(const std::string& filename, const Image& image);
    
    // Read raw binary file
    static std::vector<uint8_t> read_binary_file(const std::string& filename);
    
    // Write raw binary file
    static void write_binary_file(const std::string& filename, 
                                   const std::vector<uint8_t>& data);
    
    // Convert grayscale to RGB
    static Image grayscale_to_rgb(const Image& grayscale);
    
    // Convert RGB to grayscale
    static Image rgb_to_grayscale(const Image& rgb);
    
    // Create empty image
    static Image create_image(uint32_t width, uint32_t height, uint8_t channels);
};

} // namespace gsdensity

#endif // GSDENSITY_IMAGE_UTILS_H
