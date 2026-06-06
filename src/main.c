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

/**
 * @brief Imprime parametros principais da execucao.
 */
static void print_header(void) {
  printf("Sistema multiagentes para %s\n", PROBLEM_NAME());
  printf("Pool: %d | Iteracoes: %d\n\n", POOL_SIZE, MAX_GLOBAL_ITERATIONS);
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
  } else {
    puts("Nenhuma solucao valida foi publicada no blackboard.");
  }
}

int main(void) {
  problem_instance problem;
  blackboard bb;
  const int max_global_iterations = MAX_GLOBAL_ITERATIONS;

  PROBLEM_INIT(&problem);
  blackboard_init(&bb);

  print_header();
  double start_time = omp_get_wtime();
  int search_ok = run_multi_agent_search(&problem, &bb, max_global_iterations);
  double end_time = omp_get_wtime();

  if (!search_ok) {
    fprintf(stderr, "Falha ao inicializar recursos da busca multi-agentes.\n");
    blackboard_destroy(&bb);
    return 1;
  }

  print_results(&bb, &problem, end_time - start_time);
  blackboard_destroy(&bb);

  return 0;
}
