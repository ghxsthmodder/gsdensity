#ifndef GSDENSITY_CLI_H
#define GSDENSITY_CLI_H

#include <string>
#include <vector>

namespace gsdensity {

class CLI {
public:
    struct Options {
        std::string command;  // encoder, decoder, verify
        std::string input_file;
        std::string output_file;
        std::string output_dir;
        std::string i3d_file;
        std::string format;  // gdm or grle
        
        // Encoder options
        int num_channels = 10;
        int num_compression_ranges = 1;
        std::vector<int> compression_boundaries;
        bool use_extended_format = false;
        
        // Verify options
        bool verbose = false;
    };
    
    static int run(int argc, char* argv[]);
    
private:
    static Options parse_args(int argc, char* argv[]);
    static void print_usage();
    static void print_version();
    
    static int run_encoder(const Options& opts);
    static int run_decoder(const Options& opts);
    static int run_verify(const Options& opts);
};

} // namespace gsdensity

#endif // GSDENSITY_CLI_H
