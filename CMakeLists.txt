cmake_minimum_required(VERSION 3.15)
project(gsdensity VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build test executables" ON)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Source files
set(GSDENSITY_SOURCES
    src/grle_decoder.cpp
    src/grle_encoder.cpp
    src/gdm_decoder.cpp
    src/gdm_encoder.cpp
    src/image_utils.cpp
    src/i3d_parser.cpp
    src/cli.cpp
)

# Header files
set(GSDENSITY_HEADERS
    include/grle_decoder.h
    include/grle_encoder.h
    include/gdm_decoder.h
    include/gdm_encoder.h
    include/image_utils.h
    include/i3d_parser.h
    include/common.h
)

# Main executable
add_executable(gsdensity
    src/main.cpp
    ${GSDENSITY_SOURCES}
)

# Compiler warnings
if(MSVC)
    target_compile_options(gsdensity PRIVATE /W4)
else()
    target_compile_options(gsdensity PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Link libraries
if(UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(gsdensity PRIVATE pthread)
endif()

# Install target
install(TARGETS gsdensity DESTINATION bin)

# Tests
if(BUILD_TESTS)
    enable_testing()
    
    add_executable(test_grle_decode
        tests/test_grle_decode.cpp
        src/grle_decoder.cpp
        src/image_utils.cpp
    )
    
    add_executable(test_grle_encode
        tests/test_grle_encode.cpp
        src/grle_encoder.cpp
        src/grle_decoder.cpp
        src/image_utils.cpp
    )
    
    add_executable(test_gdm_decode
        tests/test_gdm_decode.cpp
        src/gdm_decoder.cpp
        src/image_utils.cpp
    )
    
    add_executable(test_gdm_encode
        tests/test_gdm_encode.cpp
        src/gdm_encoder.cpp
        src/gdm_decoder.cpp
        src/image_utils.cpp
    )
    
    add_executable(test_roundtrip
        tests/test_roundtrip.cpp
        src/gdm_encoder.cpp
        src/gdm_decoder.cpp
        src/grle_encoder.cpp
        src/grle_decoder.cpp
        src/image_utils.cpp
    )
    
    add_test(NAME test_grle_decode COMMAND test_grle_decode)
    add_test(NAME test_grle_encode COMMAND test_grle_encode)
    add_test(NAME test_gdm_decode COMMAND test_gdm_decode)
    add_test(NAME test_gdm_encode COMMAND test_gdm_encode)
    add_test(NAME test_roundtrip COMMAND test_roundtrip)
endif()