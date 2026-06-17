#ifndef MULTI_AGENTES_TRD_DECODER_H
#define MULTI_AGENTES_TRD_DECODER_H

#include "hscopt/hscopt.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct trd_workspace {
  int order;
} trd_workspace;

int trd_workspace_init(trd_workspace *ws, int order);
void trd_workspace_clear(trd_workspace *ws);
hscopt_workspace *trd_ws_clone(const hscopt_workspace *ws, void *user);
void trd_ws_destroy(hscopt_workspace *ws, void *user);

double trd_decoder(const double *keys, size_t n, hscopt_decode_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif // MULTI_AGENTES_TRD_DECODER_H
