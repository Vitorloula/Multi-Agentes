cmake_minimum_required(VERSION 3.12)
project(MultiAgentes C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json" FORCE)

set(MA_TRD_MAX_VERTICES 128 CACHE STRING "Maximum number of graph vertices for the TRD decoder")
set(MA_POOL_SIZE 5 CACHE STRING "Number of solutions kept in the blackboard")
set(MA_MAX_GLOBAL_ITERATIONS 80 CACHE STRING "Global search iteration limit")

find_package(OpenMP REQUIRED)

add_subdirectory(external/hscopt)

if (UNIX)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
  )
endif()

add_library(multi_agentes_core STATIC
  src/core/agents.c
  src/core/agents/aco_agent.c
  src/core/agents/hho_agent.c
  src/core/agents/rvns_agent.c
  src/core/agents/tabu_agent.c
  src/core/blackboard.c
  src/core/graph.c
  src/core/trd.c
  src/core/trd_decoder.c
)

target_include_directories(multi_agentes_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(multi_agentes_core PUBLIC
  PROBLEM_SIZE=${MA_TRD_MAX_VERTICES}
  POOL_SIZE=${MA_POOL_SIZE}
  MAX_GLOBAL_ITERATIONS=${MA_MAX_GLOBAL_ITERATIONS}
)

target_link_libraries(multi_agentes_core PUBLIC
  hscopt::hscopt
  OpenMP::OpenMP_C
)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(multi_agentes_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
endif()

add_executable(multi_agentes
  src/main.c
)

target_link_libraries(multi_agentes PRIVATE
  multi_agentes_core
)

find_package(Doxygen QUIET)
if (DOXYGEN_FOUND)
  add_custom_target(docs
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating Doxygen documentation"
    VERBATIM
  )
endif()
