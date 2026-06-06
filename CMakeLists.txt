cmake_minimum_required(VERSION 3.12)
project(MultiAgentes C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json" FORCE)

set(MA_PROBLEM "tsp" CACHE STRING "Active problem decoder: tsp or rcmpsp")
set_property(CACHE MA_PROBLEM PROPERTY STRINGS rcmpsp tsp)
set(MA_N_PROJECTS 4 CACHE STRING "Number of RCMPSP projects")
set(MA_PROJECT_REAL_ACTIVITIES 8 CACHE STRING "Number of real activities per RCMPSP project")
set(MA_N_RESOURCES 3 CACHE STRING "Number of RCMPSP renewable resources")
set(MA_RCMPSP_MAX_HORIZON 512 CACHE STRING "Scheduling horizon used by the RCMPSP decoder")
set(MA_N_CITIES 50 CACHE STRING "Number of TSP cities")
set(MA_POOL_SIZE 5 CACHE STRING "Number of solutions kept in the blackboard")
set(MA_MAX_GLOBAL_ITERATIONS 80 CACHE STRING "Global search iteration limit")

math(EXPR MA_PROJECT_ACTIVITY_COUNT "${MA_PROJECT_REAL_ACTIVITIES} + 2")
math(EXPR MA_N_ACTIVITIES "2 + (${MA_N_PROJECTS} * ${MA_PROJECT_ACTIVITY_COUNT})")

if (MA_PROBLEM STREQUAL "tsp")
  set(MA_ACTIVE_PROBLEM_DEFINE ACTIVE_PROBLEM_TSP=1)
  set(MA_PROBLEM_SIZE ${MA_N_CITIES})
elseif (MA_PROBLEM STREQUAL "rcmpsp")
  set(MA_ACTIVE_PROBLEM_DEFINE ACTIVE_PROBLEM_RCMPSP=1)
  math(EXPR MA_PROBLEM_SIZE "2 * ${MA_N_ACTIVITIES}")
else()
  message(FATAL_ERROR "MA_PROBLEM must be 'rcmpsp' or 'tsp'")
endif()

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
  src/core/rcmpsp.c
  src/core/rcmpsp_decoder.c
  src/core/tsp.c
  src/core/tsp_decoder.c
)

target_include_directories(multi_agentes_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(multi_agentes_core PUBLIC
  ${MA_ACTIVE_PROBLEM_DEFINE}
  N_PROJECTS=${MA_N_PROJECTS}
  PROJECT_REAL_ACTIVITIES=${MA_PROJECT_REAL_ACTIVITIES}
  PROJECT_ACTIVITY_COUNT=${MA_PROJECT_ACTIVITY_COUNT}
  N_ACTIVITIES=${MA_N_ACTIVITIES}
  N_RESOURCES=${MA_N_RESOURCES}
  RCMPSP_MAX_HORIZON=${MA_RCMPSP_MAX_HORIZON}
  N_CITIES=${MA_N_CITIES}
  PROBLEM_SIZE=${MA_PROBLEM_SIZE}
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
