/**
 * @file tsp.c
 * @brief Implementacao da instancia sintetica do TSP.
 */

#include "core/tsp.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int compare_keypairs(const void *a, const void *b) {
  double ka = ((const key_pair *)a)->key;
  double kb = ((const key_pair *)b)->key;
  return (ka > kb) - (ka < kb);
}

void tsp_create_random_instance(tsp_instance *tsp) {
  tsp->n_cities = N_CITIES;
  srand(100);

  printf("Gerando instancia TSP com %d cidades...\n", N_CITIES);
  for (int i = 0; i < N_CITIES; i++) {
    tsp->cities[i].x = (double)(rand() % 1000);
    tsp->cities[i].y = (double)(rand() % 1000);
  }

  for (int i = 0; i < N_CITIES; i++) {
    for (int j = 0; j < N_CITIES; j++) {
      if (i == j) {
        tsp->dist_matrix[i][j] = 0.0;
      } else {
        double dx = tsp->cities[i].x - tsp->cities[j].x;
        double dy = tsp->cities[i].y - tsp->cities[j].y;
        tsp->dist_matrix[i][j] = sqrt(dx * dx + dy * dy);
      }
    }
  }
}

void tsp_init_instance(tsp_instance *tsp) { tsp_create_random_instance(tsp); }

void tsp_print_tour(const double *keys, const tsp_instance *tsp) {
  (void)tsp;

  key_pair pairs[N_CITIES];
  for (int i = 0; i < N_CITIES; i++) {
    pairs[i].key = keys[i];
    pairs[i].index = i;
  }

  qsort(pairs, N_CITIES, sizeof(key_pair), compare_keypairs);

  printf("Rota: ");
  for (int i = 0; i < N_CITIES; i++) {
    printf("%d -> ", pairs[i].index);
  }
  printf("%d\n", pairs[0].index);
}
