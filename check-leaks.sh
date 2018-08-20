#!/usr/bin/env bash
set -e

rm -fR build2
mkdir build2
cd build2
cmake .. -DCMAKE_BUILD_TYPE=Debug -G 'Unix Makefiles'
make
valgrind \
    -v \
    --leak-check=full \
    --show-leak-kinds=definite \
    --suppressions=../gcrud.supp \
    ./gcrud
cd ..
rm -fR build2
