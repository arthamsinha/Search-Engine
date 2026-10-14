#!/bin/bash
set -e

echo "=== Building Backend ==="
cd /app/backend
cmake -B build -S .
cmake --build build -j4

echo "=== Fetching Data ==="
cd /app
# Fetch without token should work for just 5 repos
./backend/build/devfinder_cli fetch "kubernetes operator" 5

echo "=== Indexing Data ==="
./backend/build/devfinder_cli index

echo "=== Starting Backend Server ==="
./backend/build/devfinder_cli serve 45678 &

echo "=== Starting Frontend Server ==="
cd /app/frontend
python3 -m http.server 8000
