/**
 * @file blackboard.c
 * @brief Implementacao da memoria compartilhada entre agentes.
 */

#include "core/blackboard.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Verifica se um custo ja esta representado na pool.
 *
 * @param bb Blackboard analisado.
 * @param fitness Custo candidato.
 * @return 1 se a solucao for considerada diversa, 0 caso contrario.
 */
static int is_diverse(const blackboard *bb, double fitness) {
  for (int i = 0; i < bb->count; i++) {
    if (fabs(bb->pool[i].fitness - fitness) < 1e-3) {
      return 0;
    }
  }
  return 1;
}

/**
 * @brief Mantem a pool ordenada por custo crescente apos insercao.
 *
 * @param bb Blackboard cuja pool sera ordenada localmente.
 */
static void sort_pool(blackboard *bb) {
  for (int i = bb->count - 1; i > 0; i--) {
    if (bb->pool[i].fitness < bb->pool[i - 1].fitness) {
      solution tmp = bb->pool[i];
      bb->pool[i] = bb->pool[i - 1];
      bb->pool[i - 1] = tmp;
    }
  }
}

void blackboard_init(blackboard *bb) {
  bb->count = 0;
  bb->total_publications = 0;
  bb->total_consultations = 0;
  bb->shared_solution_reads = 0;
  omp_init_lock(&bb->lock);
}

void blackboard_destroy(blackboard *bb) { omp_destroy_lock(&bb->lock); }

int blackboard_publish(blackboard *bb, const double *keys, double fitness,
                       const char *agent_name) {
  omp_set_lock(&bb->lock);

  if (!is_diverse(bb, fitness)) {
    omp_unset_lock(&bb->lock);
    return 0;
  }

  int updated = 0;
  if (bb->count < POOL_SIZE) {
    memcpy(bb->pool[bb->count].keys, keys, PROBLEM_SIZE * sizeof(double));
    bb->pool[bb->count].fitness = fitness;
    snprintf(bb->pool[bb->count].publisher, AGENT_NAME_SIZE, "%s",
             agent_name);
    bb->count++;
    sort_pool(bb);
    updated = 1;
  } else if (fitness < bb->pool[POOL_SIZE - 1].fitness) {
    memcpy(bb->pool[POOL_SIZE - 1].keys, keys, PROBLEM_SIZE * sizeof(double));
    bb->pool[POOL_SIZE - 1].fitness = fitness;
    snprintf(bb->pool[POOL_SIZE - 1].publisher, AGENT_NAME_SIZE, "%s",
             agent_name);
    sort_pool(bb);
    updated = 1;
  }

  if (updated) {
    bb->total_publications++;
    printf("[%s] publicou %.2f no blackboard (pool: %d)\n", agent_name,
           fitness, bb->count);
  }

  omp_unset_lock(&bb->lock);
  return updated;
}

int blackboard_get_solution(blackboard *bb, hscopt_rng *rng, double *out_keys,
                            int get_best) {
  omp_set_lock(&bb->lock);
  bb->total_consultations++;

  if (bb->count == 0) {
    omp_unset_lock(&bb->lock);
    for (int i = 0; i < PROBLEM_SIZE; i++) {
      out_keys[i] = hscopt_rng_next_u01(rng);
    }
    return 0;
  }

  int idx = 0;
  if (!get_best) {
    idx = (int)hscopt_rng_random_index(rng, (size_t)bb->count);
  }

  memcpy(out_keys, bb->pool[idx].keys, PROBLEM_SIZE * sizeof(double));
  bb->shared_solution_reads++;
  omp_unset_lock(&bb->lock);
  return 1;
}
