#!/bin/bash
set -e

BUILD_DIR="build"
TARGET="${1:-all}"   # pass "bench", "print", or "dump" to build just one

cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    > /dev/null

if [ "$TARGET" = "all" ]; then
    cmake --build "$BUILD_DIR"
else
    cmake --build "$BUILD_DIR" --target "$TARGET"
fi

echo ""
echo "Built targets in $BUILD_DIR/"
echo ""
echo "  Benchmark (no I/O — pure parse speed):"
echo "    $BUILD_DIR/bench <file.itch>"
echo ""
echo "  Print (human-readable, I/O bound):"
echo "    $BUILD_DIR/print <file.itch>"
echo "    $BUILD_DIR/print <file.itch> -o out.txt"
echo ""
echo "  Tip: decompress first if needed:"
echo "    gunzip -k file.NASDAQ_ITCH50.gz"