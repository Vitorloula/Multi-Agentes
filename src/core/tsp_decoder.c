/**
 * @file tsp_decoder.c
 * @brief Implementacao do decoder TSP por ordenacao de random keys.
 */

#include "core/tsp_decoder.h"

#include <stdlib.h>

int tsp_workspace_init(tsp_workspace *ws) {
  ws->pairs = malloc(N_CITIES * sizeof(key_pair));
  return ws->pairs != NULL;
}

void tsp_workspace_clear(tsp_workspace *ws) {
  free(ws->pairs);
  ws->pairs = NULL;
}

hscopt_workspace *tsp_ws_clone(const hscopt_workspace *ws, void *user) {
  (void)ws;
  (void)user;

  tsp_workspace *new_ws = malloc(sizeof(tsp_workspace));
  if (new_ws == NULL) {
    return NULL;
  }
  if (!tsp_workspace_init(new_ws)) {
    free(new_ws);
    return NULL;
  }
  return (hscopt_workspace *)new_ws;
}

void tsp_ws_destroy(hscopt_workspace *ws, void *user) {
  (void)user;

  tsp_workspace *tws = (tsp_workspace *)ws;
  if (tws == NULL) {
    return;
  }
  tsp_workspace_clear(tws);
  free(tws);
}

double tsp_decoder(const double *keys, size_t n, hscopt_decode_ctx *ctx) {
  (void)n;

  const tsp_instance *tsp = (const tsp_instance *)ctx->inst;
  tsp_workspace *tws = (tsp_workspace *)ctx->ws;

  for (int i = 0; i < N_CITIES; i++) {
    tws->pairs[i].key = keys[i];
    tws->pairs[i].index = i;
  }

  qsort(tws->pairs, N_CITIES, sizeof(key_pair), compare_keypairs);

  double distance = 0.0;
  for (int i = 0; i < N_CITIES - 1; i++) {
    int u = tws->pairs[i].index;
    int v = tws->pairs[i + 1].index;
    distance += tsp->dist_matrix[u][v];
  }

  int last = tws->pairs[N_CITIES - 1].index;
  int first = tws->pairs[0].index;
  distance += tsp->dist_matrix[last][first];

  return distance;
}
