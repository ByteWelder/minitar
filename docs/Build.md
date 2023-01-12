# minitar build instructions

minitar uses the cross-platform [CMake](https://cmake.org/) build generator.

## Configuring

Standard CMake out-of-source build:

```sh
$ mkdir -p build
$ cmake -S . -B build
```

## Building

Simply run `$ cmake --build build`.

## Installing

`# cmake --install build`

This will (on UNIX-like platforms) install `minitar.h` to /usr/local/include, and `libmtar.a` to /usr/local/lib.

## Using

After installation, you can compile regular programs that include `minitar.h` and use the minitar API by adding the `-lmtar` flag to your compiler.

Example:

`cc -o my-own-tar my-own-tar.c -O2 -Wall -lmtar`

## Using (with CMake)

Add the `minitar` directory as a subdirectory of your project (perhaps using git submodules?) and add the following lines to your `CMakeLists.txt`:

```
add_subdirectory(minitar)

target_link_libraries(<YOUR_PROJECT_NAME> PRIVATE minitar)
```

If you're using this method, this is the only step necessary, since minitar will be built and linked along with the rest of your project when you invoke your own build system.