#ifndef MULTI_AGENTES_DECODER_ADAPTER_H
#define MULTI_AGENTES_DECODER_ADAPTER_H

#include "hscopt/hscopt.h"

/*
 * Adapta uma funcao no formato de decoder BRKGA do projeto:
 *
 *   double fn(const double *chromosome, size_t n,
 *             const instance_type *instance, workspace_type *workspace);
 *
 * para a assinatura esperada pela biblioteca hscopt:
 *
 *   double fn(const double *keys, size_t n, hscopt_decode_ctx *ctx);
 */
#define DEFINE_HSCOPT_DECODER_ADAPTER(adapter_name, brkga_fn, instance_type,  \
                                      workspace_type)                         \
  double adapter_name(const double *keys, size_t n, hscopt_decode_ctx *ctx) { \
    return brkga_fn(keys, n, (const instance_type *)ctx->inst,               \
                    (workspace_type *)ctx->ws);                              \
  }

#endif
