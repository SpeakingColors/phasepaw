#!/bin/sh
# Builds and runs the core tests. No JUCE needed.
set -e
cd "$(dirname "$0")/.."
mkdir -p build
c++ -std=c++17 -O2 -Wall -Wextra -o build/prototype tests/prototype.cpp
./build/prototype
