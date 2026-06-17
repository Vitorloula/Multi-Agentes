#ifndef MULTI_AGENTES_TRD_DECODER_H
#define MULTI_AGENTES_TRD_DECODER_H

#include "core/trd.h"
#include "hscopt/hscopt.h"

#define LABEL_ZERO 0
#define LABEL_ONE 1
#define LABEL_TWO 2

hscopt_workspace *trd_ws_clone(const hscopt_workspace *ws, void *user);
void trd_ws_destroy(hscopt_workspace *ws, void *user);
double trd_brkga_decode(const double *chromosome, size_t n,
                        const trd_instance *graph, trd_workspace *ws);
double trd_decoder(const double *keys, size_t n, hscopt_decode_ctx *ctx);

#endif
