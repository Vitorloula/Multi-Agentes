# multi-agentes

Sistema multiagentes cooperativo para otimizacao combinatoria usando o decoder
TRD (Total Roman Domination) sobre grafos em lista de adjacencia.

Os agentes executam em paralelo com OpenMP e cooperam indiretamente por um
blackboard compartilhado. Todas as metaheuristicas usam o mesmo decoder ativo
por meio da assinatura `hscopt_decoder_fn` da biblioteca `hscopt`.

## documentacao

- [arquitetura do sistema](arquitetura_sistema.md)
- [funcionamento do processo](explicacao_processo.md)

## formato do grafo

Coloque as instancias de teste na pasta `data/`. O programa recebe um
arquivo de arestas, com dois inteiros por linha:

```text
10 20
20 30
30 10
```

A leitura normaliza os ids originais para vertices sequenciais `0..n-1`,
remove self-loops e remove arestas duplicadas tratando o grafo como nao
direcionado.

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

```sh
mise run build
mise run run
```

A tarefa `run` usa o grafo de exemplo em `data/trd_graph.txt`.

## compilando sem mise

```sh
cmake -S . -B build
cmake --build build
./build/multi_agentes data/trd_graph.txt
```

Para usar outro grafo:

```sh
./build/multi_agentes caminho/para/grafo.txt
```

## experimentos em lote

Para rodar todas as instancias da pasta `data/`, com 30 execucoes independentes
por instancia e limite cooperativo de 15 minutos por execucao:

```sh
python3 scripts/run_experiments.py
```

O script detecta automaticamente o maior numero de vertices nas instancias e
ajusta o build antes de compilar.

Saidas principais:

- `raw_runs.csv`: dados de cada execucao independente.
- `agent_metrics.csv`: publicacoes, consultas e melhor valor por agente.
- `convergence_events.csv`: eventos aceitos no blackboard para curvas de convergencia.
- `convergence_summary.csv`: curva media amostrada por instancia.
- `summary.csv`: resumo agregado por instancia.
- `comparison_table.csv/.tex`: tabela PLI/AG/Blackboard.
- `stability_table.csv/.tex`: tabela de estabilidade.
- `cooperation_table.csv/.tex`: indicadores de cooperacao.

Para informar valores de PLI, AG ou melhor conhecido, passe um CSV com coluna
`instance` e, opcionalmente, `z_pli`, `t_pli`, `z_ag`, `t_ag`, `z_best`:

```sh
python3 scripts/run_experiments.py --reference-csv referencias.csv
```

## parametros padrao dos algoritmos

Os parametros fechados ficam em `src/main.c`, no bloco `algorithm_params`.
Eles foram definidos para ficar proximos aos valores calibrados por IRACE nas
tabelas ACOR+TS e HHO+RVNS.

| algoritmo | parametro | valor |
| :--- | :--- | ---: |
| Todos | threads por metaheuristica | `8` |
| ACO | tamanho do arquivo (`archive_size`) | `200` |
| ACO | numero de formigas (`n_ants`) | `200` |
| ACO | pressao de selecao (`q`) | `0.8` |
| ACO | escala da amostragem (`xi`) | `0.8` |
| TS | tamanho da vizinhanca | `40` |
| TS | tenure tabu | `30` |
| TS | iteracoes internas por chamada | `1000` |
| RVNS | maior vizinhanca (`k_max`) | `50` |
| RVNS | iteracoes internas por chamada | `1000` |
| HHO | numero de gavioes | `200` |
| HHO | iteracoes internas por publicacao | `10` |

Exemplo:

```sh
python3 scripts/run_experiments.py
```

## adaptador de decoder

O decoder TRD fica dividido em duas camadas:

- `trd_brkga_decode`: funcao no estilo BRKGA/random keys, recebendo cromossomo,
  instancia e workspace.
- `trd_decoder`: wrapper gerado por `DEFINE_HSCOPT_DECODER_ADAPTER`, adaptando
  para a assinatura exigida pela biblioteca `hscopt`.

Assim, ACO, HHO, RVNS e Tabu Search chamam a mesma funcao de biblioteca sem
conhecer detalhes do problema.

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

O build cria um link `compile_commands.json` na raiz apontando para
`build/compile_commands.json`, facilitando o uso de clangd e outras ferramentas.
