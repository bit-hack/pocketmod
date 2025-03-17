cmake_minimum_required(VERSION 3.0)
project(pocketmod)

set(SDL_LIBRARY     "SDL_LIBRARY"     CACHE FILEPATH "SDL_LIBRARY")
set(SDL_INCLUDE_DIR "SDL_INCLUDE_DIR" CACHE PATH     "SDL_INCLUDE_DIR")

include_directories(${SDL_INCLUDE_DIR})

add_executable(pocketmod
  examples/player.c
  pocketmod.h
  pocketmod.c
)

add_executable(fpgaplay
  examples/fpgaplay.c
  pocketmod.h
  pocketmod.c
)

target_link_libraries(pocketmod ${SDL_LIBRARY})
