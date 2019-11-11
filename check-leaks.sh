#!/usr/bin/env bash
rm -fR build2
mkdir build2
cd build2 || exit 1
cmake .. -DCMAKE_BUILD_TYPE=Debug -G 'Unix Makefiles'
make
valgrind \
    -v \
    --leak-check=full \
    --show-leak-kinds=definite \
    --suppressions=../gcrud.supp \
    ./gcrud
cd .. || exit 1
rm -fR build2
