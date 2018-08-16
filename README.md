# grud - A Gentoo maintenance tool

## How to use

1. `git clone ...`
2. `cd gcrud`
3. `mkdir build`
4. `cd build`
5. `cmake .. -DCMAKE_BUILD_TYPE=Release -G 'Unix Makefiles'`
6. `make`
7. `sudo ./gcruft2 | sort -u > out.log`

View `out.log` and check out things you *might* be able to remove. The whitelist is a work in progress. See [`whitelist.c`'s `whitelist_check()`](whitelist.c#L6) function.


## Contributing

[Issue tracker](https://gitlab.com/Tatsh/gcrud/issues)

1. Fork.
2. Make a new branch.
3. Make changes.
4. Build in debug mode: `cmake .. -DCMAKE_BUILD_TYPE=Debug -G 'Unix Makefiles' && make`
5. Test!
6. Before submitting, run: `./check-leaks.sh` to check for memory leaks (requires Valgrind) and `clang-format -i *.h *.c` (requires Clang).
