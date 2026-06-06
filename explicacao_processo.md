# Funcionamento do Processo Multi-Agentes

Este documento detalha o fluxo operacional do sistema multi-agentes assíncrono usando a biblioteca **hscopt** com paralelismo **OpenMP**.

---

## 1. Fase de Inicialização e Preparação

Antes do disparo dos agentes paralelos, o sistema monta o ambiente de computação.

```mermaid
flowchart LR
    A[Selecionar MA_PROBLEM] --> B{Problema ativo}
    B -->|tsp| C[Criar instancia aleatoria TSP]
    B -->|rcmpsp| D[Criar instancia aleatoria RCMPSP]
    C --> E[Alocar workspace do decoder]
    D --> E
    E --> F[Inicializar Blackboard]
    F --> G[Criar contextos dos agentes]
```

### Detalhes das Etapas

1. **Seleção do problema**: `MA_PROBLEM=tsp` é o padrão; `MA_PROBLEM=rcmpsp` ativa o decoder de escalonamento multi-projeto.
2. **Instância aleatória**: cada problema possui uma função própria para criar uma instância aleatória.
3. **Workspace do decoder**: cada decoder fornece funções de criação, clonagem e destruição de workspace.
4. **Blackboard**: a pool compartilhada é inicializada antes dos agentes.

---

## 2. Fase de Disparo e Paralelismo Assíncrono

Os agentes são executados em seções OpenMP independentes.

```mermaid
flowchart TD
    O[Orquestrador] --> A1[Contexto ACO<br/>RNG proprio]
    O --> A2[Contexto Tabu Search<br/>RNG proprio]
    O --> A3[Contexto RVNS<br/>RNG proprio]
    O --> A4[Contexto HHO<br/>RNG proprio]

    A1 --> P[OpenMP parallel sections]
    A2 --> P
    A3 --> P
    A4 --> P

    P --> B[Blackboard compartilhado]
```

Cada agente possui:

- nome e papel;
- gerador pseudoaleatório próprio;
- workspace local para reavaliar soluções;
- contadores de publicações, consultas e leituras compartilhadas.

---

## 3. Protocolo de Comunicação

O Blackboard gerencia a aceitação e o compartilhamento de soluções.

```mermaid
flowchart TD
    Start[Agente gera solucao] --> Eval[Reavalia com decoder ativo]
    Eval --> Lock[Adquire lock do Blackboard]
    Lock --> Div{Solucao diversa?}
    Div -- Nao --> Reject[Rejeita]
    Div -- Sim --> Space{Pool tem espaco?}
    Space -- Sim --> Insert[Insere e ordena]
    Space -- Nao --> Better{Melhor que a pior?}
    Better -- Sim --> Replace[Substitui pior e ordena]
    Better -- Nao --> Reject
    Insert --> Unlock[Libera lock]
    Replace --> Unlock
    Reject --> Unlock
```

---

## 4. Critério de Parada e Métricas

Ao final da execução, o sistema imprime:

- melhor solução e custo validado;
- origem de cada solução mantida na pool;
- publicações aceitas;
- consultas ao Blackboard;
- leituras de soluções compartilhadas;
- resumo individual de cada agente.
