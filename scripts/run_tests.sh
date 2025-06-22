#!/bin/bash

# Test Runner Script

echo "=== Building Project ==="
cmake --build build --config Debug

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "=== Running All Tests ==="
cd build
ctest --output-on-failure --verbose

echo ""
echo "=== Test Results ==="
ctest --output-on-failure
