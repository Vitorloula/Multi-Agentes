#ifndef MULTI_AGENTES_CONFIG_H
#define MULTI_AGENTES_CONFIG_H

/**
 * @file config.h
 * @brief Parametros globais do experimento multiagentes.
 *
 * Os valores podem ser sobrescritos pelo CMake usando `MA_TRD_MAX_VERTICES`,
 * `MA_POOL_SIZE` e `MA_MAX_GLOBAL_ITERATIONS`.
 */

/**
 * @brief Quantidade de chaves aleatorias usadas pelo problema principal.
 *
 * Para TRD, este valor e o numero maximo de vertices aceito pelo programa.
 * O decoder usa apenas os primeiros `graph.order` genes.
 */
#ifndef PROBLEM_SIZE
#define PROBLEM_SIZE 128
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
#define MAX_GLOBAL_ITERATIONS 4000
#endif

#endif
