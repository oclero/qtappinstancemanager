#!/bin/bash

source $(dirname "$0")/configure-ubuntu.sh
cmake -B $build_dir
