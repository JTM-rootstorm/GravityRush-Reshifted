cmake_minimum_required(VERSION 3.25)
project(grr_recomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# SwARc's runtime and HLE modules are shared libraries: rebuilding one
# does not relink the game.
set(CMAKE_LINK_DEPENDS_NO_SHARED ON)

# SwARc: the lifter, the SSE/x87 models, the runtime and its PS4
# platform module (a sibling checkout), without the Win32 one.
set(SWARC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../SwARc"
    CACHE PATH "SwARc source tree")
set(RECOMP_PLATFORM_ORBIS ON CACHE BOOL "PS4 (Orbis OS) platform module" FORCE)
set(RECOMP_PLATFORM_WIN32 OFF CACHE BOOL "Win32 platform module (Windows PC games)" FORCE)

enable_testing()
add_subdirectory("${SWARC_DIR}" swarc)

# The game: its lifted code (generated/, produced by tools/lift.sh), a
# lifted module of its own, and an executable that holds main(), with the
# runtime and the PS4 HLE beside them in bin/ (recomp_bundle).
recomp_add_lifted(grr_lifted "${CMAKE_CURRENT_SOURCE_DIR}/generated" LINK recomp_orbis)
if(TARGET grr_lifted)
  # Replacements of lifted functions (`replace` in manifest/).
  target_sources(grr_lifted PRIVATE src/fios2_kick.cpp)
  add_executable(grr src/main.cpp)
  target_link_libraries(grr PRIVATE grr_lifted recomp_orbis recomp_orbis_kernel
                        recomp_orbis_libc_internal recomp_orbis_system recomp_orbis_fiber
                        recomp_orbis_audio recomp_orbis_ngs2 recomp_orbis_avplayer
                        recomp_orbis_font recomp_orbis_net recomp_orbis_np recomp_orbis_pad
                        recomp_orbis_savedata recomp_orbis_videoout recomp_orbis_gnm
                        recomp_orbis_savedata_dialog)
  recomp_bundle(grr)
endif()
