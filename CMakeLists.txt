cmake_minimum_required(VERSION 3.25)
project(grr_recomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# x86recomp's runtime and HLE modules are shared libraries: rebuilding one
# does not relink the game.
set(CMAKE_LINK_DEPENDS_NO_SHARED ON)

# x86recomp: the lifter, the SSE/x87 models, the runtime and its PS4
# platform module (a sibling checkout).
set(X86RECOMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../x86recomp"
    CACHE PATH "x86recomp source tree")
set(RECOMP_PLATFORM_ORBIS ON CACHE BOOL "PS4 (Orbis OS) platform module" FORCE)

enable_testing()
add_subdirectory("${X86RECOMP_DIR}" x86recomp)

# The game: runtime + PS4 HLE + lifted code (generated/, produced by
# tools/lift.sh).
recomp_add_lifted(grr_lifted "${CMAKE_CURRENT_SOURCE_DIR}/generated")
if(TARGET grr_lifted)
  add_executable(grr src/main.cpp $<TARGET_OBJECTS:grr_lifted>)
  target_link_libraries(grr PRIVATE recomp_orbis recomp_orbis_kernel recomp_orbis_libc_internal
                        recomp_orbis_system recomp_orbis_fiber recomp_orbis_audio recomp_orbis_net recomp_orbis_np recomp_orbis_savedata recomp_orbis_videoout
                        recomp_orbis_gnm)
  set_target_properties(grr PROPERTIES ENABLE_EXPORTS ON)  # symbol names in fault backtraces
endif()
