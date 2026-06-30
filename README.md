# GSDENSITY - GDM and GRLE Encoder/Decoder

A high-performance C++ tool for encoding and decoding GDM (Giants Density Map) and GRLE (Giants Run-Length Encoded) files used in Farming Simulator games, with focus on FS20 compatibility.

## Features

- ✅ **GRLE Format Support**: Full encode/decode support for GRLE files
- ✅ **GDM Format Support**: Complete implementation of GDM chunk-based compression
- ✅ **Multiple Formats**: Support for both `!MDF` and `"MDF` GDM variants
- ✅ **Compression Ranges**: Handle multi-range compression for complex density maps
- ✅ **File Verification**: Built-in integrity checking for GDM/GRLE files
- ✅ **Cross-Platform**: Builds on Linux, macOS, Windows, and Android (aarch64)
- ✅ **No External Dependencies**: Self-contained PNG implementation

## Building

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)

### Linux/macOS

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Android (aarch64)

```bash
mkdir build-android
cd build-android
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-21 \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=OFF
make -j$(nproc)
```

## Usage

### Decoding (GDM/GRLE to PNG)

```bash
# Decode GRLE file
gsdensity decoder -f input.grle -o output_dir/

# Decode GDM file
gsdensity decoder -f input.gdm -o output_dir/
```

### Encoding (PNG to GDM/GRLE)

```bash
# Encode to GRLE (format auto-detected from extension)
gsdensity encoder -i input.png -o output.grle

# Encode to GDM with 10 channels
gsdensity encoder -i input.png -o output.gdm -n 10

# Encode to GDM with compression ranges
gsdensity encoder -i input.png -o output.gdm -n 10 -r 2 -b 5

# Use extended GDM format ("MDF)
gsdensity encoder -i input.png -o output.gdm -n 10 --extended
```

### Verification

```bash
# Verify file integrity
gsdensity verify -f input.grle

# Verbose verification
gsdensity verify -f input.gdm -v
```

## Command Reference

### Global Options

- `-h, --help` - Show help message
- `--version` - Show version information

### Encoder Options

- `-i, --input <file>` - Input PNG file (required)
- `-o, --output <file>` - Output GDM/GRLE file (required)
- `-f, --format <gdm|grle>` - Output format (default: auto-detect from extension)
- `-n, --num-channels <n>` - Number of channels for GDM (default: 10)
- `-r, --ranges <n>` - Number of compression ranges for GDM (default: 1)
- `-b, --boundaries <list>` - Compression boundaries (comma-separated, e.g., "5,10")
- `--extended` - Use extended GDM format ("MDF)

### Decoder Options

- `-f, --file <file>` - Input GDM/GRLE file (required)
- `-o, --output-dir <dir>` - Output directory (default: current directory)

### Verify Options

- `-f, --file <file>` - Input GDM/GRLE file (required)
- `-v, --verbose` - Verbose output with file details

## File Format Details

### GRLE Format

GRLE (Giants Run-Length Encoded) files store single-channel grayscale data using RLE compression:

- **Magic**: `GRLE` (4 bytes)
- **Version**: Always 1
- **Dimensions**: Must be multiples of 256
- **Channels**: Always 1 (grayscale)
- **Compression**: Simple run-length encoding

**Use cases**: Environment zones, farmland boundaries, collision masks, level layers

### GDM Format

GDM (Giants Density Map) files store multi-channel data using chunk-based palette compression:

- **Magic**: `!MDF` (simple) or `"MDF` (extended)
- **Chunk Size**: 32×32 pixels
- **Channels**: 1-24 channels (typically 3-12)
- **Compression**: Block-based with palette optimization
- **Ranges**: Support for splitting channels into compression groups

**Use cases**: Crop density, terrain height, stone placement, weed coverage

## Test Results

All tests passed successfully:

- ✅ GRLE Decoding: 4/4 files
- ✅ GRLE Encoding: 4/4 files  
- ✅ GDM Decoding: 4/4 files
- ✅ GDM Encoding: 4/4 files

## Examples

### Example 1: Convert GRLE to PNG

```bash
gsdensity decoder -f mapUSM_farmland.grle -o output/
# Output: output/mapUSM_farmland.png (1024x1024 grayscale)
```

### Example 2: Create GRLE from PNG

```bash
gsdensity encoder -i farmland_edited.png -o mapUSM_farmland_new.grle
# Creates GRLE file with RLE compression
```

### Example 3: Decode GDM with Multiple Channels

```bash
gsdensity decoder -f fruit_density.gdm -o output/
# Output: output/fruit_density.png (RGB image for 10+ channels)
```

### Example 4: Create GDM with Compression Ranges

```bash
# Create 10-channel GDM with 2 compression ranges
# Range 0: channels 0-4 (5 channels)
# Range 1: channels 5-9 (5 channels)
gsdensity encoder -i input.png -o output.gdm -n 10 -r 2 -b 5
```

## Technical Details

### Compression Efficiency

**GRLE Files:**
- Uniform regions: ~500:1 compression ratio
- Sparse data: ~50:1 compression ratio
- Complex patterns: ~15:1 compression ratio

**GDM Files:**
- Uniform chunks: 512:1 (4 bytes per 32×32 chunk)
- Simple patterns: 15:1 to 7:1
- Complex data: 3:1 to 4:1

### Output Format Selection

- **Grayscale PNG**: Used when channels ≤ 8
- **RGB PNG**: Used when channels > 8
  - R channel: bits 0-7
  - G channel: bits 8-15
  - B channel: bits 16-23

## Project Structure

```
gsdensity/
├── include/           # Header files
│   ├── common.h
│   ├── grle_decoder.h
│   ├── grle_encoder.h
│   ├── gdm_decoder.h
│   ├── gdm_encoder.h
│   ├── image_utils.h
│   ├── i3d_parser.h
│   └── cli.h
├── src/              # Implementation files
│   ├── main.cpp
│   ├── grle_decoder.cpp
│   ├── grle_encoder.cpp
│   ├── gdm_decoder.cpp
│   ├── gdm_encoder.cpp
│   ├── image_utils.cpp
│   ├── i3d_parser.cpp
│   └── cli.cpp
├── tests/            # Test programs
│   ├── test_grle_decode.cpp
│   ├── test_grle_encode.cpp
│   ├── test_gdm_decode.cpp
│   └── test_gdm_encode.cpp
├── sources/          # Documentation and samples
│   ├── gsdensity.md
│   ├── Relatório_de_Sugestões_para_o_CLI_GSDENSITY.md
│   ├── grleconvert-master/
│   └── samples/
└── CMakeLists.txt
```

## Contributing

Contributions are welcome! Please ensure:

1. Code follows C++17 standards
2. All tests pass
3. New features include tests
4. Documentation is updated

## License

See LICENSE file for details.

## References

- [GRLE Format Specification](sources/grleconvert-master/docs/GRLE_FORMAT.md)
- [GDM Format Specification](sources/grleconvert-master/docs/GDM_FORMAT.md)
- [Project Suggestions Report](sources/Relatório_de_Sugestões_para_o_CLI_GSDENSITY.md)

## Acknowledgments

- Format specifications based on reverse-engineering work by Paint-a-Farm/grleconvert
- Designed for Farming Simulator 20 (FS20) compatibility
