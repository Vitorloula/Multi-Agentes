cmake_minimum_required(VERSION 3.12)
project(MultiAgentesTSP C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Habilita OpenMP para suporte a múltiplos agentes assíncronos
find_package(OpenMP REQUIRED)

# Adiciona o diretório da biblioteca hscopt (assumindo clone em external/hscopt)
add_subdirectory(external/hscopt)

# Define o executável do projeto
add_executable(multi_agentes_tsp src/main.c)

# Linka com a hscopt e com o OpenMP
target_link_libraries(multi_agentes_tsp PRIVATE hscopt::hscopt OpenMP::OpenMP_C)

# Configura diretórios de inclusão para facilitar o desenvolvimento
target_include_directories(multi_agentes_tsp PRIVATE src)
