#!/bin/bash

# Build script for rasterizer project

set -e  # Exit on error

BUILD_DIR="build"
BUILD_TYPE="${1:-Release}"

echo "Building rasterizer..."
echo "Build type: $BUILD_TYPE"

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Configure and build
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "Build complete!"
echo "Executable: $BUILD_DIR/bin/app"
echo ""
echo "To run: ./$BUILD_DIR/bin/app"
