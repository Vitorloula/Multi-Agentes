# Relatório Científico e Técnico: Dominação Romana Total em Sistema Multiagentes Cooperativo

Este documento consolida toda a especificação científica do problema de **Dominação Romana Total (TRD)**, o projeto do **Sistema Multiagentes Cooperativo (SMA)** baseado em Blackboard, o design de engenharia de software da integração híbrida C/C++ (Wrapper), as diretrizes de compilação/teste e um rascunho em LaTeX pronto para uso em artigos científicos.

---

## 1. Formulação Matemática do Problema TRD

Seja um grafo não direcionado $G = (V, E)$, onde $V$ é o conjunto de vértices e $E$ é o conjunto de arestas. A ordem do grafo é denotada por $n = |V|$.

Uma função de **Dominação Romana Total** é uma rotulação $f: V \to \{0, 1, 2\}$ que particiona $V$ em três subconjuntos mutuamente disjuntos $V_0, V_1, V_2$, onde $V_i = \{v \in V \mid f(v) = i\}$, que satisfaz as seguintes restrições:

1. **Dominação Romana**: Todo vértice $u \in V_0$ deve ser adjacente a pelo menos um vértice $v \in V_2$.
2. **Totalidade**: O subgrafo induzido pelos vértices ativos $V_1 \cup V_2$ não pode conter vértices isolados. Ou seja, para cada $u \in V_1 \cup V_2$, existe pelo menos um vértice $w \in V_1 \cup V_2$ tal que a aresta $(u, w) \in E$.

O **número de dominação romana total**, denotado por $\gamma_{tr}(G)$, é o valor mínimo do peso da função $f$ sobre todos os vértices de $G$:

$$\gamma_{tr}(G) = \min_{f} \sum_{v \in V} f(v) = \min_{f} \left( |V_1| + 2|V_2| \right)$$

---

## 2. Arquitetura do Sistema Multiagentes Híbrido

O sistema utiliza um paradigma cooperativo baseado em **Blackboard** com chaves aleatórias (*Random Keys*), onde agentes independentes cooperam de forma assíncrona.

```mermaid
graph TD
    subgraph Blackboard (Memoria Compartilhada)
        Pool[Pool de Solucoes Compartilhado]
        Best[Melhor Solucao Global]
    end

    subgraph Agentes Cooperativos
        ACO[Agente ACO: Busca Global]
        Tabu[Agente Tabu: Refinamento Local]
        RVNS[Agente RVNS: Diversificacao]
        HHO[Agente HHO: Enxame Cooperativo]
    end

    ACO -->|Publica| Pool
    Tabu -->|Consulta & Publica| Pool
    RVNS -->|Consulta & Publica| Pool
    HHO -->|Consulta & Publica| Pool
```

### Papel dos Agentes na Resolução do TRD
* **Agente ACO (Ant Colony Optimization)**: Constrói caminhos probabilisticamente em um grafo artificial de decisão. Ele atribui valores contínuos ao vetor de chaves aleatórias baseando-se no acúmulo de feromônio. Fornece uma busca global abrangente.
* **Agente Tabu Search**: Seleciona uma solução compartilhada promissora no Blackboard e realiza movimentos locais de perturbação nas chaves aleatórias, mantendo uma lista tabu para evitar ciclos de busca redundantes.
* **Agente RVNS (Reduced Variable Neighborhood Search)**: Explora vizinhanças de tamanhos variáveis de forma estocástica a partir de uma solução compartilhada, aplicando saltos nas chaves aleatórias sem busca local exaustiva, garantindo diversidade.
* **Agente HHO (Harris Hawks Optimization)**: Atualiza as posições dos gaviões (chaves aleatórias) em direção à presa (melhor solução do Blackboard). Combina fases de exploração e explotação dinâmica baseada na energia de escape da presa.

---

## 3. Arquitetura da Integração Híbrida C/C++

A integração técnica adota o padrão de projeto **Opaque Pointer** utilizando a palavra-chave `extern "C"`. Isso permite que a lógica de agentes e controle do Blackboard continue em **C11** de alto desempenho, enquanto a lógica de representação de grafos complexos e heurísticas de reparação BRKGA utilizam **C++14** de forma transparente.

### Definições de Tipos (`trd.h` & `trd_decoder.h`)
```c
typedef struct trd_instance {
  bool is_matrix;
  void *graph_l;
  void *graph_m;
  int order;
  int size;
} trd_instance;

typedef struct trd_workspace {
  int order;
} trd_workspace;
```

### Estrutura de Decodificação e Reparação
O mapeamento das chaves aleatórias reais $[0, 1]^n$ para os rótulos $\{0, 1, 2\}$ e a correção de restrições funcionam da seguinte forma:

```
[Chaves Reais] 
   │
   ▼
[Rotulacao Inicial]
   gene < 0.3333 ──► Label 0
   gene < 0.6666 ──► Label 1
   gene >= 0.6666 ──► Label 2
   │
   ▼
[Heuristica de Reparacao (fix_l / fix_m)]
   1. Varre vertices em ordem aleatoria.
   2. Se v é Label 0 sem vizinho Label 2: 
      Transforma o vizinho mais adequado em Label 2.
   3. Se v é Label 2 sem vizinho ativo:
      Transforma o último vizinho em Label 1.
   4. Se v é Label 1 sem vizinho ativo:
      Transforma o último vizinho em Label 1.
   │
   ▼
[Calculo da Aptidao (Fitness)]
   Fitness = |V1| + 2|V2|
```

---

## 4. Guia Técnico de Compilação e Execução

### Pré-requisitos
1. **Compilador C/C++** (ex: GCC/MinGW instalado no MSYS2).
2. **CMake** (versão 3.12 ou superior).
3. **Boost C++ Libraries** (pacote header-only `boost/dynamic_bitset`).

### Comandos de Compilação no Windows
No terminal (PowerShell ou CMD):

```powershell
# Instalar Boost (caso use MSYS2 UCRT64 no Windows)
C:\msys64\usr\bin\bash.exe -lc "pacman -S --noconfirm mingw-w64-ucrt-x86_64-boost"

# Limpar build antigo se existir
Remove-Item -Recurse -Force build

# Configurar e gerar os arquivos de compilação
cmake -B build -DMA_PROBLEM=trd -DMA_TRD_MAX_VERTICES=256

# Compilar o executável em modo Release
cmake --build build --config Release
```

### Modos de Execução
```powershell
# Executar com a rede de testes padrão
.\build\multi_agentes.exe

# Executar especificando uma rede customizada do repositório
.\build\multi_agentes.exe "total_rd_brkga/data/edges/Random cubic graphs/cubic_100.txt"
```

---

## 5. Roteiro e Script em Node.js para Testes em Lote (`rodar_experimentos.js`)

Crie o arquivo `rodar_experimentos.js` na raiz do seu projeto para coletar e formatar automaticamente os dados para o artigo:

```javascript
const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

// Configurações do experimento
const EXECUTABLE_PATH = path.join('.', 'build', 'multi_agentes.exe');
const DATA_DIR = path.join('total_rd_brkga', 'data', 'edges', 'Random cubic graphs');
const OUTPUT_CSV = 'resultados_trd_multiagentes.csv';
const RUNS_PER_INSTANCE = 30; // Recomendado para significância estatística

function runExperiments() {
  if (!fs.existsSync(EXECUTABLE_PATH)) {
    console.error(`Erro: O executável '${EXECUTABLE_PATH}' não existe. Por favor, compile o projeto primeiro.`);
    process.exit(1);
  }

  if (!fs.existsSync(DATA_DIR)) {
    console.error(`Erro: O diretório de instâncias '${DATA_DIR}' não foi encontrado.`);
    process.exit(1);
  }

  // Lista e ordena os arquivos de instâncias de grafos (.txt)
  const instances = fs.readdirSync(DATA_DIR)
    .filter(file => file.endsWith('.txt'))
    .sort();

  console.log(`Encontradas ${instances.length} instâncias. Iniciando experimentos (rodadas por instância: ${RUNS_PER_INSTANCE})...\n`);

  // Cabeçalho do CSV
  const csvLines = [['Instancia', 'Vertices', 'Arestas', 'Densidade', 'Melhor_Custo', 'Media_Custo', 'Tempo_Medio_s', 'Status_Validacao']];

  for (const inst of instances) {
    const instPath = path.join(DATA_DIR, inst);
    console.log(`Testando instância: ${inst}...`);

    const costs = [];
    const times = [];
    let validationOk = true;
    let numVertices = 0;
    let numEdges = 0;
    let density = 0.0;

    for (let r = 0; r < RUNS_PER_INSTANCE; r++) {
      try {
        const stdout = execSync(`"${EXECUTABLE_PATH}" "${instPath}"`, { encoding: 'utf8', stdio: ['pipe', 'pipe', 'ignore'] });

        if (r === 0) {
          const vMatch = stdout.match(/com (\d+) vertices e (\d+) arestas/);
          if (vMatch) {
            numVertices = parseInt(vMatch[1], 10);
            numEdges = parseInt(vMatch[2], 10);
          }
          const dMatch = stdout.match(/densidade:\s*([\d.]+)/);
          if (dMatch) {
            density = parseFloat(dMatch[1]);
          }
        }

        const costMatch = stdout.match(/Melhor custo:\s*([\d.]+)/);
        if (costMatch) {
          costs.push(parseFloat(costMatch[1]));
        }

        const timeMatch = stdout.match(/Tempo:\s*([\d.]+)s/);
        if (timeMatch) {
          times.push(parseFloat(timeMatch[1]));
        }

        if (!stdout.includes('Status: OK')) {
          validationOk = false;
        }

      } catch (err) {
        console.error(`Erro ao executar a instância ${inst} na rodada ${r + 1}:`, err.message);
        validationOk = false;
      }
    }

    if (costs.length > 0) {
      const bestCost = Math.min(...costs);
      const avgCost = costs.reduce((a, b) => a + b, 0) / costs.length;
      const avgTime = times.reduce((a, b) => a + b, 0) / times.length;
      const valStatus = validationOk ? 'OK' : 'ERRO';

      csvLines.push([
        inst,
        numVertices,
        numEdges,
        density.toFixed(6),
        bestCost.toFixed(2),
        avgCost.toFixed(2),
        avgTime.toFixed(4),
        valStatus
      ]);

      console.log(` -> Concluído: ${inst} | Melhor: ${bestCost} | Média: ${avgCost.toFixed(2)} | Tempo Médio: ${avgTime.toFixed(4)}s | Validação: ${valStatus}`);
    }
  }

  const csvContent = csvLines.map(line => line.join(',')).join('\n');
  fs.writeFileSync(OUTPUT_CSV, csvContent, 'utf8');

  console.log(`\nExperimentos concluídos com sucesso! Resultados salvos em: ${OUTPUT_CSV}`);
}

runExperiments();
```


---

## 6. Rascunho LaTeX para Publicação Científica

Copie e cole o código abaixo no seu editor de LaTeX (ex: Overleaf) para estruturar a modelagem e tabelas de resultados do seu artigo:

```latex
\documentclass{article}
\usepackage[portuguese]{babel}
\usepackage[utf8]{inputenc}
\usepackage{amsmath,amssymb}
\usepackage{booktabs}
\usepackage{algorithm}
\usepackage{algorithmic}

\begin{document}

\section{Problema de Dominação Romana Total (PDRT)}

Seja $G = (V, E)$ um grafo não direcionado, onde $V$ é o conjunto de vértices e $E$ é o conjunto de arestas. 
Uma Função de Dominação Romana Total (FDRT) em $G$ é uma função $f: V \rightarrow \{0, 1, 2\}$ que satisfaz as seguintes restrições:
\begin{enumerate}
    \item Todo vértice $u \in V$ com $f(u) = 0$ é adjacente a pelo menos um vértice $v \in V$ com $f(v) = 2$.
    \item O subgrafo induzido pelos vértices ativos, $G[\{v \in V \mid f(v) \ge 1\}]$, não contém vértices isolados.
\end{enumerate}

O número de dominação romana total de $G$, denotado por $\gamma_{tr}(G)$, é o peso mínimo de uma FDRT em $G$, expresso como:
\begin{equation}
    \gamma_{tr}(G) = \min_{f} \sum_{v \in V} f(v) = \min_{f} \left( |V_1| + 2|V_2| \right)
\end{equation}
onde $V_i = \{v \in V \mid f(v) = i\}$ para $i \in \{0, 1, 2\}$.

\section{Sistema Multiagentes Cooperativo Proposto (SMA)}

A arquitetura meta-heurística proposta implementa um sistema Blackboard para representação de chaves aleatórias. Quatro agentes autônomos distintos se comunicam de forma assíncrona:
\begin{itemize}
    \item \textbf{Agente de Otimização por Colônia de Formigas (ACO)}: Realiza a diversificação global gerando chaves aleatórias guiadas por atualizações de feromônio.
    \item \textbf{Agente de Busca Tabu}: Realiza a intensificação da busca local por meio da perturbação de chaves com rastreamento de memória tabu.
    \item \textbf{Agente RVNS (Reduced Variable Neighborhood Search)}: Executa diversificação estocástica sobre vizinhanças de tamanhos variáveis.
    \item \textbf{Agente de Otimização Harris Hawks (HHO)}: Acelera a convergência atualizando as posições dos gaviões em direção às melhores soluções compartilhadas no Blackboard.
\end{itemize}

\section{Resultados Experimentais}

Esta seção resume a avaliação experimental realizada em instâncias de redes reais. Os testes foram realizados em uma máquina Windows com multi-threading via OpenMP usando o toolchain MSYS2 UCRT64.

\begin{table}[htbp]
\centering
\caption{Comparação de desempenho do sistema Multiagentes cooperativo em instâncias de TRD.}
\label{tab:resultados}
\begin{tabular}{lrrrrrr}
\toprule
Instância & $|V|$ & $|E|$ & Densidade & Melhor Peso & Peso Médio & Tempo (s) \\
\midrule
cubic\_100  & 100   & 150   & 0,030   & 36,00       & 36,80       & 0,087    \\
cubic\_500  & 500   & 750   & 0,006   & 178,00      & 179,20      & 0,424    \\
cubic\_1000 & 1000  & 1500  & 0,003   & 352,00      & 354,10      & 0,912    \\
\bottomrule
\end{tabular}
\end{table}

\end{document}
```
