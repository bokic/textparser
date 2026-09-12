#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

cmake -S "$project_dir" -B "$project_dir/build" -DCMAKE_BUILD_TYPE=Debug "$@"
cmake --build "$project_dir/build" --parallel
