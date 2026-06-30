#include "cli.h"
#include "grle_decoder.h"
#include "grle_encoder.h"
#include "gdm_decoder.h"
#include "gdm_encoder.h"
#include "image_utils.h"
#include <iostream>
#include <cstring>

namespace gsdensity {

void CLI::print_version() {
    std::cout << "gsdensity version 1.0.0" << std::endl;
    std::cout << "GDM and GRLE encoder/decoder for Farming Simulator" << std::endl;
}

void CLI::print_usage() {
    std::cout << "Usage: gsdensity <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  encoder    Encode PNG to GDM/GRLE\n";
    std::cout << "  decoder    Decode GDM/GRLE to PNG\n";
    std::cout << "  verify     Verify GDM/GRLE file integrity\n\n";
    
    std::cout << "Encoder options:\n";
    std::cout << "  -i, --input <file>       Input PNG file\n";
    std::cout << "  -o, --output <file>      Output GDM/GRLE file\n";
    std::cout << "  -f, --format <gdm|grle>  Output format (default: auto-detect from extension)\n";
    std::cout << "  -n, --num-channels <n>   Number of channels (GDM only, default: 10)\n";
    std::cout << "  -r, --ranges <n>         Number of compression ranges (GDM only, default: 1)\n";
    std::cout << "  -b, --boundaries <list>  Compression boundaries (GDM only, comma-separated)\n";
    std::cout << "  --extended               Use extended GDM format (\"MDF)\n\n";
    
    std::cout << "Decoder options:\n";
    std::cout << "  -f, --file <file>        Input GDM/GRLE file\n";
    std::cout << "  -o, --output-dir <dir>   Output directory (default: current directory)\n\n";
    
    std::cout << "Verify options:\n";
    std::cout << "  -f, --file <file>        Input GDM/GRLE file\n";
    std::cout << "  -v, --verbose            Verbose output\n\n";
    
    std::cout << "Global options:\n";
    std::cout << "  -h, --help               Show this help message\n";
    std::cout << "  --version                Show version information\n\n";
    
    std::cout << "Examples:\n";
    std::cout << "  gsdensity decoder -f map.grle -o output/\n";
    std::cout << "  gsdensity encoder -i input.png -o output.grle\n";
    std::cout << "  gsdensity encoder -i input.png -o output.gdm -n 10 -r 2 -b 5\n";
    std::cout << "  gsdensity verify -f map.gdm -v\n";
}

CLI::Options CLI::parse_args(int argc, char* argv[]) {
    Options opts;
    
    if (argc < 2) {
        print_usage();
        exit(1);
    }
    
    // Check for global options
    if (strcmp(argv[1], "--version") == 0) {
        print_version();
        exit(0);
    }
    
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage();
        exit(0);
    }
    
    // Parse command
    opts.command = argv[1];
    
    if (opts.command != "encoder" && opts.command != "decoder" && opts.command != "verify") {
        std::cerr << "Error: Unknown command '" << opts.command << "'" << std::endl;
        print_usage();
        exit(1);
    }
    
    // Parse options
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage();
            exit(0);
        } else if (arg == "--version") {
            print_version();
            exit(0);
        } else if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) {
                opts.input_file = argv[++i];
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                opts.output_file = argv[++i];
            }
        } else if (arg == "--output-dir") {
            if (i + 1 < argc) {
                opts.output_dir = argv[++i];
            }
        } else if (arg == "-f" || arg == "--file") {
            if (i + 1 < argc) {
                opts.input_file = argv[++i];
            }
        } else if (arg == "--format") {
            if (i + 1 < argc) {
                opts.format = argv[++i];
            }
        } else if (arg == "-n" || arg == "--num-channels") {
            if (i + 1 < argc) {
                opts.num_channels = std::stoi(argv[++i]);
            }
        } else if (arg == "-r" || arg == "--ranges") {
            if (i + 1 < argc) {
                opts.num_compression_ranges = std::stoi(argv[++i]);
            }
        } else if (arg == "-b" || arg == "--boundaries") {
            if (i + 1 < argc) {
                std::string boundaries_str = argv[++i];
                size_t pos = 0;
                while ((pos = boundaries_str.find(',')) != std::string::npos) {
                    opts.compression_boundaries.push_back(std::stoi(boundaries_str.substr(0, pos)));
                    boundaries_str.erase(0, pos + 1);
                }
                if (!boundaries_str.empty()) {
                    opts.compression_boundaries.push_back(std::stoi(boundaries_str));
                }
            }
        } else if (arg == "--extended") {
            opts.use_extended_format = true;
        } else if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
        }
    }
    
    return opts;
}

int CLI::run_encoder(const Options& opts) {
    try {
        if (opts.input_file.empty()) {
            std::cerr << "Error: Input file required (-i/--input)" << std::endl;
            return 1;
        }
        
        if (opts.output_file.empty()) {
            std::cerr << "Error: Output file required (-o/--output)" << std::endl;
            return 1;
        }
        
        // Determine format from extension if not specified
        std::string format = opts.format;
        if (format.empty()) {
            if (opts.output_file.find(".grle") != std::string::npos) {
                format = "grle";
            } else if (opts.output_file.find(".gdm") != std::string::npos) {
                format = "gdm";
            } else {
                std::cerr << "Error: Cannot determine format from extension. Use --format" << std::endl;
                return 1;
            }
        }
        
        std::cout << "Encoding " << opts.input_file << " to " << opts.output_file << std::endl;
        
        if (format == "grle") {
            GRLEEncoder::encode_file(opts.input_file, opts.output_file);
            std::cout << "Successfully encoded to GRLE format" << std::endl;
        } else if (format == "gdm") {
            GDMEncoder::EncodeOptions gdm_opts;
            gdm_opts.num_channels = opts.num_channels;
            gdm_opts.num_compression_ranges = opts.num_compression_ranges;
            gdm_opts.use_extended_format = opts.use_extended_format;
            
            for (int boundary : opts.compression_boundaries) {
                gdm_opts.compression_boundaries.push_back(static_cast<uint8_t>(boundary));
            }
            
            GDMEncoder::encode_file(opts.input_file, opts.output_file, gdm_opts);
            std::cout << "Successfully encoded to GDM format" << std::endl;
            std::cout << "  Channels: " << opts.num_channels << std::endl;
            std::cout << "  Compression ranges: " << opts.num_compression_ranges << std::endl;
        } else {
            std::cerr << "Error: Unknown format '" << format << "'" << std::endl;
            return 1;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int CLI::run_decoder(const Options& opts) {
    try {
        if (opts.input_file.empty()) {
            std::cerr << "Error: Input file required (-f/--file)" << std::endl;
            return 1;
        }
        
        // Determine output filename
        std::string output_file;
        if (!opts.output_dir.empty()) {
            size_t last_slash = opts.input_file.find_last_of("/\\");
            std::string basename = (last_slash != std::string::npos) ? 
                                  opts.input_file.substr(last_slash + 1) : opts.input_file;
            
            // Replace extension with .png
            size_t last_dot = basename.find_last_of('.');
            if (last_dot != std::string::npos) {
                basename = basename.substr(0, last_dot);
            }
            
            output_file = opts.output_dir;
            if (output_file.back() != '/' && output_file.back() != '\\') {
                output_file += "/";
            }
            output_file += basename + ".png";
        } else {
            output_file = opts.input_file;
            size_t last_dot = output_file.find_last_of('.');
            if (last_dot != std::string::npos) {
                output_file = output_file.substr(0, last_dot);
            }
            output_file += ".png";
        }
        
        std::cout << "Decoding " << opts.input_file << " to " << output_file << std::endl;
        
        // Determine format from extension
        Image image;
        if (opts.input_file.find(".grle") != std::string::npos) {
            image = GRLEDecoder::decode_file(opts.input_file);
            std::cout << "Successfully decoded GRLE file" << std::endl;
        } else if (opts.input_file.find(".gdm") != std::string::npos) {
            image = GDMDecoder::decode_file(opts.input_file);
            std::cout << "Successfully decoded GDM file" << std::endl;
        } else {
            std::cerr << "Error: Unknown file format (expected .grle or .gdm)" << std::endl;
            return 1;
        }
        
        std::cout << "  Dimensions: " << image.width << "x" << image.height << std::endl;
        std::cout << "  Channels: " << static_cast<int>(image.channels) << std::endl;
        
        // Write PNG
        ImageUtils::write_png(output_file, image);
        std::cout << "Saved to " << output_file << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int CLI::run_verify(const Options& opts) {
    try {
        if (opts.input_file.empty()) {
            std::cerr << "Error: Input file required (-f/--file)" << std::endl;
            return 1;
        }
        
        std::cout << "Verifying " << opts.input_file << std::endl;
        
        auto data = ImageUtils::read_binary_file(opts.input_file);
        
        bool valid = false;
        std::string error;
        
        if (opts.input_file.find(".grle") != std::string::npos) {
            valid = GRLEDecoder::verify(data, &error);
            if (valid) {
                std::cout << "✓ Valid GRLE file" << std::endl;
                
                if (opts.verbose) {
                    auto header = GRLEDecoder::decode(data);
                    std::cout << "  Dimensions: " << header.width << "x" << header.height << std::endl;
                    std::cout << "  File size: " << data.size() << " bytes" << std::endl;
                }
            } else {
                std::cout << "✗ Invalid GRLE file: " << error << std::endl;
            }
        } else if (opts.input_file.find(".gdm") != std::string::npos) {
            valid = GDMDecoder::verify(data, &error);
            if (valid) {
                std::cout << "✓ Valid GDM file" << std::endl;
                
                if (opts.verbose) {
                    auto image = GDMDecoder::decode(data);
                    std::cout << "  Dimensions: " << image.width << "x" << image.height << std::endl;
                    std::cout << "  Channels: " << static_cast<int>(image.channels) << std::endl;
                    std::cout << "  File size: " << data.size() << " bytes" << std::endl;
                }
            } else {
                std::cout << "✗ Invalid GDM file: " << error << std::endl;
            }
        } else {
            std::cerr << "Error: Unknown file format (expected .grle or .gdm)" << std::endl;
            return 1;
        }
        
        return valid ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int CLI::run(int argc, char* argv[]) {
    Options opts = parse_args(argc, argv);
    
    if (opts.command == "encoder") {
        return run_encoder(opts);
    } else if (opts.command == "decoder") {
        return run_decoder(opts);
    } else if (opts.command == "verify") {
        return run_verify(opts);
    }
    
    return 1;
}

} // namespace gsdensity
