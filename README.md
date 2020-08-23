# Indigo

Indigo is a compiler for a C-like toy language (named "SysY") into ARMv7a assembly, written in C++ 17.

## Building

Environment:

- CMake and Make
- GCC >= 9.2.0 or Clang > 8.0

Steps:

```sh
mkdir build && cd build
cmake .. && make -j8
./compiler -h    # for help information
```

## Architecture



## Testing

Environment:

- Building environment
- GCC
- Clang (if you need to compare compiler speeds)
- Python >= 3.7
  - Packages: `colorlog`, `tqdm`

Steps:

```sh
mkdir -p output

# Test against official functional tests
python3 scripts/test.py build/compiler \
    test_codes/sysyruntimelibrary/libsysy.a test_codes/functional_test -r
    
# Test against custom tests
python3 scripts/test.py build/compiler \
    test_codes/sysyruntimelibrary/libsysy.a test_codes/upload -r -t 90
    
# Test against performance tests
python3 scripts/test.py build/compiler \
    test_codes/sysyruntimelibrary/libsysy.a test_codes/performance_test -r -t 120 -z
    
# Compare performance with GCC and Clang, assuming all tests are passed
python3 scripts/speed_compare.py build/compiler \
    test_codes/sysyruntimelibrary/libsysy.a \
    test_codes/sysyruntimelibrary/stdlib.c \
    test_codes/performance_test -r -t 120
```

## License

Copyright (c) 2020 SEGVIOL Team (Bo Zhao, Qi Teng, Ruichen He, Ziye Li).

This program is licensed under the [GPLv3 license][gplv3].

[gplv3]: https://www.gnu.org/licenses/gpl-3.0.en.html

All files in the `test_codes` folder, except those in `test_codes/upload`, are official test cases written by the NSCSCC 2020 Committee. These files follow the license specified in the [official test code repository][test_code].

[test_code]: https://gitlab.eduxiji.net/windcome/sysyruntimelibrary

