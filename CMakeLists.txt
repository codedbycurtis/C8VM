cmake_minimum_required(VERSION 3.21)
project(C8VM)
include(FetchContent)

set(EXECUTABLE_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${EXECUTABLE_DIR})

set(CMAKE_C_STANDARD 23)

set(SDLTTF_VENDORED ON)

# Ideally we would use a release tag here but builds fail using the latest release (3.2.24)
FetchContent_Declare(
		SDL
		GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
		GIT_TAG        main
)
# Ideally we would use a release tag here too but builds fail using the latest release (3.2.2)
FetchContent_Declare(
		SDL_ttf
		GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
		GIT_TAG        main
)
FetchContent_MakeAvailable(SDL SDL_ttf)

add_executable(
	${PROJECT_NAME}
	src/vm.h
	src/vm.c
	src/arena.h
	src/arena.c
	src/core.h
	src/clay.h
	src/clay_renderer_SDL3.c
	src/components.h
	src/components.c
	src/layouts.h
	src/layouts.c
	src/main.c
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
	SDL3::SDL3
	SDL3_ttf::SDL3_ttf
)

add_custom_target(
		CopyDirs
		COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${EXECUTABLE_DIR}/assets"
)

add_dependencies(${PROJECT_NAME} CopyDirs)