# How to use

1. `git clone ...`
2. `cd gcrud`
3. `mkdir build`
4. `cd build`
5. `cmake .. -DCMAKE_BUILD_TYPE=Release`
6. `make`
7. `sudo ./gcruft2 | sort -u > out.log`

View `out.log` and check out things you *might* be able to remove. The whitelist is a work in progress. See `util.c`'s `whitelist_check()` function.