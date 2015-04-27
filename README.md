# libemu51 - a 8051/8052 emulator library

## Project Goals

- cross platform
- fully reentrant code
- clear code
- well-tested
- documentation

## Dependencies

libemu51 is written in standard C99, with the following optional dependencies:

- [cmocka](https://cmocka.org/): unit tests
- gcc+lcov: generating test coverage report

[CMake](http://www.cmake.org/) is required to build libemu51.

## Building

Extract libemu51 and chdir to the directory. Type the following commands:

```
mkdir build && cd build
cmake ..
make
```

Run the unit tests:

```
make test
```

Generate and view coverage report (requires lcov):

```
make coverage
firefox coverage/index.html
```

## License

This work is distributed under the zlib/libpng license. See COPYING for the
full license text.
