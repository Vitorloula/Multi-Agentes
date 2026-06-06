# multi-agentes

Sistema multiagentes cooperativo para otimizacao combinatoria com decoders alternaveis. O problema padrao e o **TSP**, e tambem ha suporte ao **RCMPSP**.

Os agentes executam em paralelo com OpenMP e cooperam indiretamente por um blackboard compartilhado.

## documentacao

- [arquitetura do sistema](arquitetura_sistema.md)
- [funcionamento do processo](explicacao_processo.md)

## requisitos

- CMake
- compilador C com suporte a C11
- OpenMP
- Git com submodulos
- opcional: `mise`
- opcional: Doxygen para gerar documentacao da API

Depois de clonar o repositorio, inicialize o submodulo:

```sh
git submodule update --init --recursive
```

## compilando com mise

O TSP e o decoder padrao:

```sh
mise run build
mise run run
```

Para selecionar explicitamente o problema:

```sh
mise run run:tsp
mise run run:rcmpsp
```

Tarefas disponiveis:

```sh
mise run configure
mise run configure:tsp
mise run configure:rcmpsp
mise run build
mise run build:tsp
mise run build:rcmpsp
mise run docs
mise run clean
```

## compilando sem mise

Configurar e compilar com o TSP:

```sh
cmake -S . -B build -DMA_PROBLEM=tsp
cmake --build build
./build/multi_agentes
```

Configurar e compilar com o RCMPSP:

```sh
cmake -S . -B build -DMA_PROBLEM=rcmpsp
cmake --build build
./build/multi_agentes
```

## opcoes principais do cmake

| opcao | padrao | descricao |
| :--- | :--- | :--- |
| `MA_PROBLEM` | `tsp` | decoder ativo: `tsp` ou `rcmpsp` |
| `MA_N_CITIES` | `50` | numero de cidades do TSP |
| `MA_N_PROJECTS` | `4` | numero de projetos do RCMPSP |
| `MA_PROJECT_REAL_ACTIVITIES` | `8` | atividades reais por projeto no RCMPSP |
| `MA_N_RESOURCES` | `3` | recursos renovaveis do RCMPSP |
| `MA_POOL_SIZE` | `5` | tamanho da pool do blackboard |
| `MA_MAX_GLOBAL_ITERATIONS` | `80` | limite de iteracoes dos agentes |

Exemplo:

```sh
cmake -S . -B build -DMA_PROBLEM=tsp -DMA_N_CITIES=100
cmake --build build
```

## documentacao doxygen

Com mise:

```sh
mise run docs
```

Sem mise:

```sh
cmake --build build --target docs
```

A saida fica em:

```text
docs/doxygen/html/index.html
```

## observacao sobre compile_commands

O build cria um link `compile_commands.json` na raiz apontando para `build/compile_commands.json`, facilitando o uso de clangd e outras ferramentas.
