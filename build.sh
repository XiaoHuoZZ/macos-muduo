#!/bin/bash
dir="./build"
[ -d "$dir" ] && rm -rf "$dir"
mkdir "$dir"
cd "$dir"
cmake ..
make -j