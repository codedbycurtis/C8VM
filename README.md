# C8VM

![C8VM Main Menu](/screenshots/menu.png)

C8VM is a virtual machine, written in C, that interprets [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) bytecode.

It supports a number of adjustable settings, enabling compatibility with programs developed for both older and newer interpreters.

A number of programs have been tested and confirmed to be functional (with some quirks enabled/disabled), but feel free to open an issue if you discover a program that does not work.

## Build

A `CMakeLists.txt` is included in the root directory that will automatically download any required dependencies, before building and linking them with the project.

To build using [CMake](https://cmake.org/), run the following commands in the root directory:

```shell
cmake -B ./build -S .
cmake --build ./build
```

## Dependencies

> Note: Clay is a header-only library included in the project's `src` directory and SDL is downloaded automatically as part of the CMake build script; you do not need to download these manually.

- [Clay](https://github.com/nicbarker/clay)
- [SDL](https://github.com/libsdl-org/SDL)
- [SDL_ttf](https://github.com/libsdl-org/SDL_ttf)
