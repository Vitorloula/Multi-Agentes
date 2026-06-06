#ifndef MULTI_AGENTES_TSP_DECODER_H
#define MULTI_AGENTES_TSP_DECODER_H

/**
 * @file tsp_decoder.h
 * @brief Decoder de random keys para o TSP.
 */

#include "core/tsp.h"
#include "hscopt/hscopt.h"

#include <stddef.h>

typedef struct hscopt_workspace {
  key_pair *pairs;
} tsp_workspace;

int tsp_workspace_init(tsp_workspace *ws);
void tsp_workspace_clear(tsp_workspace *ws);
hscopt_workspace *tsp_ws_clone(const hscopt_workspace *ws, void *user);
void tsp_ws_destroy(hscopt_workspace *ws, void *user);
double tsp_decoder(const double *keys, size_t n, hscopt_decode_ctx *ctx);

#endif
