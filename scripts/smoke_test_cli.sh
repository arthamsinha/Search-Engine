#!/bin/bash
set -e

echo "Building DevFinder..."
cd backend
cmake -B build -S .
cmake --build build
cd ..

echo ""
echo "=== 1. Fetch ==="
./backend/build/devfinder_cli fetch "kubernetes operator" 5

echo ""
echo "=== 2. Index ==="
./backend/build/devfinder_cli index

echo ""
echo "=== 3. Search ==="
./backend/build/devfinder_cli search "kubernetes operator"

echo ""
echo "Smoke test passed successfully!"
