cmake_minimum_required(VERSION 3.12)
project(MultiAgentes C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json" FORCE)

set(MA_PROBLEM "trd" CACHE STRING "Active problem decoder: tsp, rcmpsp or trd")
set_property(CACHE MA_PROBLEM PROPERTY STRINGS rcmpsp tsp trd)
set(MA_TRD_MAX_VERTICES 3000 CACHE STRING "Max vertex count for TRD")
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
  set(PROBLEM_SOURCES
    src/core/tsp.c
    src/core/tsp_decoder.c
  )
elseif (MA_PROBLEM STREQUAL "rcmpsp")
  set(MA_ACTIVE_PROBLEM_DEFINE ACTIVE_PROBLEM_RCMPSP=1)
  math(EXPR MA_PROBLEM_SIZE "2 * ${MA_N_ACTIVITIES}")
  set(PROBLEM_SOURCES
    src/core/rcmpsp.c
    src/core/rcmpsp_decoder.c
  )
elseif (MA_PROBLEM STREQUAL "trd")
  set(MA_ACTIVE_PROBLEM_DEFINE ACTIVE_PROBLEM_TRD=1)
  set(MA_PROBLEM_SIZE ${MA_TRD_MAX_VERTICES})
  set(PROBLEM_SOURCES
    src/core/trd/ListGraph.cpp
    src/core/trd/MatrixGraph.cpp
    src/core/trd/Chromosome.cpp
    src/core/trd/trd.cpp
    src/core/trd/trd_decoder.cpp
  )
else()
  message(FATAL_ERROR "MA_PROBLEM must be 'rcmpsp', 'tsp' or 'trd'")
endif()

find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)

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
  ${PROBLEM_SOURCES}
)

target_include_directories(multi_agentes_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${Boost_INCLUDE_DIRS}
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
  MA_TRD_MAX_VERTICES=${MA_TRD_MAX_VERTICES}
  PROBLEM_SIZE=${MA_PROBLEM_SIZE}
  POOL_SIZE=${MA_POOL_SIZE}
  MAX_GLOBAL_ITERATIONS=${MA_MAX_GLOBAL_ITERATIONS}
)

target_link_libraries(multi_agentes_core PUBLIC
  hscopt::hscopt
  OpenMP::OpenMP_C
  OpenMP::OpenMP_CXX
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

# Alvo para compilar o BRKGA puro original para fins de comparacao cientifica
add_executable(total_rd_brkga_executable
  total_rd_brkga/src/main.cpp
  total_rd_brkga/src/Chromosome.cpp
  total_rd_brkga/src/ListGraph.cpp
  total_rd_brkga/src/MatrixGraph.cpp
  total_rd_brkga/lib/brkgaAPI/Population.cpp
)

target_include_directories(total_rd_brkga_executable PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/total_rd_brkga/src
  ${CMAKE_CURRENT_SOURCE_DIR}/total_rd_brkga/lib/brkgaAPI
  ${Boost_INCLUDE_DIRS}
)

target_link_libraries(total_rd_brkga_executable PRIVATE
  OpenMP::OpenMP_CXX
)

