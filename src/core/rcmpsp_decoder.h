#ifndef MULTI_AGENTES_RCMPSP_DECODER_H
#define MULTI_AGENTES_RCMPSP_DECODER_H

/**
 * @file rcmpsp_decoder.h
 * @brief Decoder BRKGA para Resource Constrained Multi-Project Scheduling.
 */

#include "core/rcmpsp.h"
#include "hscopt/hscopt.h"

#include <stddef.h>

/**
 * @brief Workspace reutilizavel pelo decoder RCMPSP.
 */
typedef struct hscopt_workspace {
  unsigned char *scheduled;
  int *start_time;
  int *finish_time;
  int *resource_usage;
} rcmpsp_workspace;

int rcmpsp_workspace_init(rcmpsp_workspace *ws);
void rcmpsp_workspace_clear(rcmpsp_workspace *ws);
hscopt_workspace *rcmpsp_ws_clone(const hscopt_workspace *ws, void *user);
void rcmpsp_ws_destroy(hscopt_workspace *ws, void *user);

/**
 * @brief Decodifica um cromossomo `2n` em cronograma e calcula a penalidade.
 *
 * As primeiras `n` chaves definem prioridade das atividades. As ultimas `n`
 * chaves definem o atraso maximo considerado na formacao do conjunto candidato.
 *
 * @param keys Cromossomo de random keys.
 * @param n Quantidade de chaves.
 * @param inst Instancia RCMPSP.
 * @param ws Workspace auxiliar.
 * @return Penalidade total do cronograma.
 */
double rcmpsp_decode_schedule(const double *keys, size_t n,
                              const rcmpsp_instance *inst,
                              rcmpsp_workspace *ws);

/**
 * @brief Decoder compativel com a hscopt.
 */
double rcmpsp_decoder(const double *keys, size_t n, hscopt_decode_ctx *ctx);

#endif
