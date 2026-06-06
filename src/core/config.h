#ifndef MULTI_AGENTES_CONFIG_H
#define MULTI_AGENTES_CONFIG_H

/**
 * @file config.h
 * @brief Parametros globais do experimento multiagentes.
 *
 * Os valores podem ser sobrescritos pelo CMake usando as opcoes de RCMPSP,
 * `MA_POOL_SIZE` e `MA_MAX_GLOBAL_ITERATIONS`.
 */

/**
 * @brief Quantidade de projetos da instancia RCMPSP.
 */
#ifndef N_PROJECTS
#define N_PROJECTS 4
#endif

/**
 * @brief Quantidade de atividades reais por projeto.
 */
#ifndef PROJECT_REAL_ACTIVITIES
#define PROJECT_REAL_ACTIVITIES 8
#endif

/**
 * @brief Quantidade de atividades por projeto incluindo inicio/fim dummy.
 */
#ifndef PROJECT_ACTIVITY_COUNT
#define PROJECT_ACTIVITY_COUNT (PROJECT_REAL_ACTIVITIES + 2)
#endif

/**
 * @brief Quantidade total de atividades incluindo inicio/fim globais.
 */
#ifndef N_ACTIVITIES
#define N_ACTIVITIES (2 + (N_PROJECTS * PROJECT_ACTIVITY_COUNT))
#endif

/**
 * @brief Quantidade de recursos renovaveis compartilhados.
 */
#ifndef N_RESOURCES
#define N_RESOURCES 3
#endif

/**
 * @brief Quantidade maxima de predecessores por atividade.
 */
#ifndef MAX_PREDECESSORS
#define MAX_PREDECESSORS 8
#endif

/**
 * @brief Horizonte maximo usado pelo decoder de escalonamento.
 */
#ifndef RCMPSP_MAX_HORIZON
#define RCMPSP_MAX_HORIZON 512
#endif

/**
 * @brief Quantidade de cidades na instancia TSP.
 */
#ifndef N_CITIES
#define N_CITIES 50
#endif

/**
 * @brief Quantidade de chaves aleatorias usadas pelo problema principal.
 *
 * O decoder RCMPSP usa `2n` chaves: as primeiras `n` sao prioridades das
 * atividades e as ultimas `n` definem atrasos de escalonamento.
 */
#ifndef PROBLEM_SIZE
#if defined(ACTIVE_PROBLEM_TSP)
#define PROBLEM_SIZE N_CITIES
#else
#define PROBLEM_SIZE (2 * N_ACTIVITIES)
#endif
#endif

/**
 * @brief Numero maximo de solucoes mantidas no blackboard compartilhado.
 */
#ifndef POOL_SIZE
#define POOL_SIZE 5
#endif

/**
 * @brief Tamanho maximo do nome de um agente nos registros do blackboard.
 */
#ifndef AGENT_NAME_SIZE
#define AGENT_NAME_SIZE 32
#endif

/**
 * @brief Limite global de iteracoes usado pelos agentes de busca.
 */
#ifndef MAX_GLOBAL_ITERATIONS
#define MAX_GLOBAL_ITERATIONS 80
#endif

#endif
