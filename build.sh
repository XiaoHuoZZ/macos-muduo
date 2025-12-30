#!/bin/bash
dir="./build"
[ -d "$dir" ] && rm -rf "$dir"
mkdir "$dir"
cd "$dir"
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j