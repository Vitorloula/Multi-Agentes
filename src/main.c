/**
 * @file main.c
 * @brief Ponto de entrada do experimento multiagentes.
 */

#include "core/agents.h"
#include "core/blackboard.h"
#include "core/config.h"
#include "core/problem.h"

#include <omp.h>
#include <stdio.h>

static void print_header(void) {
  printf("Sistema multiagentes para %s\n\n", PROBLEM_NAME());
}

/**
 * @brief Imprime o resumo final da busca multiagentes.
 *
 * @param bb Blackboard contendo as solucoes finais.
 * @param problem Instancia usada para decodificar e imprimir a melhor solucao.
 * @param elapsed_seconds Tempo total da busca em segundos.
 */
static void print_results(const blackboard *bb, const problem_instance *problem,
                          double elapsed_seconds) {
  puts("\nResultado");
  printf("Tempo: %.4fs\n", elapsed_seconds);

  if (bb->count > 0) {
    printf("Melhor custo: %.2f\n", bb->pool[0].fitness);
    PROBLEM_PRINT_SOLUTION(bb->pool[0].keys, problem);

    printf("\nPool final (%d solucoes):\n", bb->count);
    for (int i = 0; i < bb->count; i++) {
      printf("%2d. %.2f | origem: %s\n", i + 1, bb->pool[i].fitness,
             bb->pool[i].publisher);
    }

    printf("\nCoordenacao autonoma:\n");
    printf("Publicacoes aceitas: %d\n", bb->total_publications);
    printf("Consultas ao blackboard: %d\n", bb->total_consultations);
    printf("Consultas com solucao compartilhada: %d\n",
           bb->shared_solution_reads);
    printf("Tempo ate melhor solucao: %.6fs\n", bb->best_time_seconds);
  } else {
    puts("Nenhuma solucao valida foi publicada no blackboard.");
  }
}

int main(int argc, char *argv[]) {
  problem_instance problem;
  blackboard bb;
  const int max_global_iterations = MAX_GLOBAL_ITERATIONS;
  const algorithm_params params = {
      .max_threads = 8,

      .aco_archive_size = 200,
      .aco_ant_count = 200,
      .aco_q = 0.8,
      .aco_xi = 0.8,

      .tabu_neighborhood_size = 40,
      .tabu_tenure = 30,
      .tabu_inner_iterations = 1000,

      .rvns_neighborhood_count = 50,
      .rvns_inner_iterations = 1000,

      .hho_population_size = 200,
      .hho_inner_iterations = 10,
  };

  const char *instance_path = NULL;
#if defined(ACTIVE_PROBLEM_TRD)
  if (argc > 1) {
    instance_path = argv[1];
  } else {
    instance_path = "total_rd_brkga/data/edges/Random cubic graphs/cubic_100.txt";
  }
#else
  (void)argc;
  (void)argv;
#endif

  if (!PROBLEM_INIT(&problem, instance_path)) {
    return 1;
  }
  
#if defined(ACTIVE_PROBLEM_TRD)
  if (problem.order == 0) {
    fprintf(stderr, "Erro ao carregar a instancia de grafo: %s\n", instance_path);
    return 1;
  }
#endif

  blackboard_init(&bb);

  print_header();
  double start_time = omp_get_wtime();
  int search_ok =
      run_multi_agent_search(&problem, &bb, max_global_iterations, &params);
  double end_time = omp_get_wtime();

  if (!search_ok) {
    fprintf(stderr, "Falha ao inicializar recursos da busca multi-agentes.\n");
    blackboard_destroy(&bb);
    PROBLEM_CLEAR(&problem);
    return 1;
  }

  print_results(&bb, &problem, end_time - start_time);
  blackboard_destroy(&bb);
  PROBLEM_CLEAR(&problem);

  return 0;
}
