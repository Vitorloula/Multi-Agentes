#ifndef MULTI_AGENTES_AGENTS_H
#define MULTI_AGENTES_AGENTS_H

/**
 * @file agents.h
 * @brief Orquestracao dos agentes de busca cooperativa.
 */

#include "blackboard.h"
#include "problem.h"

typedef struct {
  int aco_ant_count;
  int aco_candidate_count;
  double aco_evaporation;
  double aco_pheromone_weight;

  int tabu_neighborhood_size;
  int tabu_tenure;
  int tabu_inner_iterations;

  int rvns_neighborhood_count;
  int rvns_inner_iterations;

  int hho_population_size;
  int hho_inner_iterations;
} algorithm_params;

/**
 * @brief Executa a busca multiagentes sobre uma instancia Set Covering.
 *
 * A busca inicia agentes ACO, Tabu Search, RVNS e HHO em secoes OpenMP. Cada
 * agente possui contexto, gerador pseudoaleatorio e ciclo de decisao proprios.
 * A coordenacao ocorre de forma indireta: os agentes publicam e consultam
 * solucoes no blackboard sem depender de um controlador central de decisoes.
 *
 * @param problem Instancia do problema a ser otimizada.
 * @param bb Blackboard compartilhado que armazenara as solucoes encontradas.
 * @param max_global_iterations Limite de iteracoes usado pelos agentes.
 * @return 1 em caso de execucao valida, 0 se algum recurso essencial falhar.
 */
int run_multi_agent_search(problem_instance *problem, blackboard *bb,
                           int max_global_iterations,
                           const algorithm_params *params);

#endif
