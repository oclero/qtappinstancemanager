#!/bin/bash

source $(dirname "$0")/configure-msvc.sh
cmake --build "$build_dir" --config Release --target "QtAppInstanceManagerTests"
cd "$build_dir"
ctest -C Release
