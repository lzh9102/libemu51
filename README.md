# libemu51 - a 8051/8052 emulator library

## Project Goals

- cross platform
- fully reentrant code
- clear code
- well-tested
- documentation

## Dependencies

The following software is required to build libemu51:

- C99-compliant C compiler
- [CMake](http://www.cmake.org/)

Optional dependencies:

- [cmocka](https://cmocka.org/): unit tests
- gcc+lcov: generating test coverage report
- [doxygen](http://www.stack.nl/~dimitri/doxygen/): generating API documentation

## Building

Extract libemu51 and chdir to the directory. Type the following commands:

```
mkdir build && cd build
cmake ..
make
```

Build and view API documentation:

```
make doc
firefox doc/html/index.html
```
## License

This work is distributed under the zlib/libpng license. See COPYING for the
full license text.
