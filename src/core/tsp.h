#ifndef MULTI_AGENTES_TSP_H
#define MULTI_AGENTES_TSP_H

/**
 * @file tsp.h
 * @brief Estruturas e operacoes basicas do TSP.
 */

#include "core/config.h"

typedef struct {
  double x;
  double y;
} city;

typedef struct {
  int n_cities;
  city cities[N_CITIES];
  double dist_matrix[N_CITIES][N_CITIES];
} tsp_instance;

typedef struct {
  double key;
  int index;
} key_pair;

int compare_keypairs(const void *a, const void *b);
void tsp_create_random_instance(tsp_instance *tsp);
void tsp_init_instance(tsp_instance *tsp);
void tsp_print_tour(const double *keys, const tsp_instance *tsp);

#endif
