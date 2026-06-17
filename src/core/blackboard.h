#ifndef MULTI_AGENTES_BLACKBOARD_H
#define MULTI_AGENTES_BLACKBOARD_H

/**
 * @file blackboard.h
 * @brief Blackboard compartilhado para cooperacao entre agentes.
 */

#include "core/config.h"
#include "hscopt/rng.h"
#include <omp.h>

/**
 * @brief Solucao candidata armazenada no blackboard.
 */
typedef struct {
  /** Vetor de random keys que codifica a rota. */
  double keys[PROBLEM_SIZE];
  /** Custo da rota; valores menores indicam solucoes melhores. */
  double fitness;
  /** Nome do agente que publicou a solucao. */
  char publisher[AGENT_NAME_SIZE];
} solution;

/**
 * @brief Estrutura compartilhada de solucoes elite.
 *
 * O blackboard funciona como memoria comum entre os agentes. Ele guarda um
 * conjunto pequeno de solucoes diversas e protege o acesso concorrente com uma
 * trava OpenMP.
 */
typedef struct {
  /** Pool ordenada das melhores solucoes distintas encontradas. */
  solution pool[POOL_SIZE];
  /** Quantidade de solucoes validas atualmente na pool. */
  int count;
  /** Quantidade total de publicacoes aceitas pela pool. */
  int total_publications;
  /** Quantidade total de consultas feitas pelos agentes. */
  int total_consultations;
  /** Quantidade de consultas respondidas com solucoes ja publicadas. */
  int shared_solution_reads;
  /** Melhor custo global ja aceito no blackboard. */
  double best_fitness;
  /** Tempo, em segundos, ate a primeira ocorrencia do melhor custo global. */
  double best_time_seconds;
  /** Trava usada para proteger leituras e escritas concorrentes. */
  omp_lock_t lock;
} blackboard;

/**
 * @brief Inicializa o blackboard e sua trava de sincronizacao.
 *
 * @param bb Blackboard a ser inicializado.
 */
void blackboard_init(blackboard *bb);

/**
 * @brief Destroi a trava associada ao blackboard.
 *
 * @param bb Blackboard previamente inicializado.
 */
void blackboard_destroy(blackboard *bb);

/**
 * @brief Publica uma solucao candidata no blackboard.
 *
 * A solucao entra na pool quando e diversa o suficiente e melhora o conjunto
 * atual de solucoes armazenadas.
 *
 * @param bb Blackboard compartilhado.
 * @param keys Vetor de random keys da solucao candidata.
 * @param fitness Custo da solucao candidata.
 * @param agent_name Nome do agente que publicou a solucao.
 * @return 1 quando a pool foi atualizada, 0 caso contrario.
 */
int blackboard_publish(blackboard *bb, const double *keys, double fitness,
                       const char *agent_name, double elapsed_seconds,
                       int *new_global_best);

/**
 * @brief Obtem uma solucao do blackboard para alimentar um agente.
 *
 * Se a pool estiver vazia, a funcao gera uma solucao aleatoria usando o gerador
 * informado. Caso contrario, retorna a melhor solucao ou uma solucao aleatoria
 * da pool, dependendo de @p get_best.
 *
 * @param bb Blackboard compartilhado.
 * @param rng Gerador pseudoaleatorio do agente chamador.
 * @param out_keys Vetor de saida com tamanho @ref PROBLEM_SIZE.
 * @param get_best Valor diferente de zero seleciona a melhor solucao da pool.
 * @return 1 quando a solucao veio da pool, 0 quando foi gerada aleatoriamente.
 */
int blackboard_get_solution(blackboard *bb, hscopt_rng *rng, double *out_keys,
                            int get_best);

#endif
