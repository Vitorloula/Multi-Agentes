#ifndef MULTI_AGENTES_PROBLEM_H
#define MULTI_AGENTES_PROBLEM_H

/**
 * @file problem.h
 * @brief Adaptador do problema ativo usado pelos agentes.
 */

#include "core/trd.h"
#include "core/trd_decoder.h"
#include "hscopt/hscopt.h"

typedef trd_instance problem_instance;
typedef trd_workspace problem_workspace;

#define PROBLEM_NAME() "TRD"
#define PROBLEM_INIT(inst, filename) trd_init_instance((inst), (filename))
#define PROBLEM_CLEAR(inst) trd_clear_instance((inst))
#define PROBLEM_PRINT_SOLUTION(keys, inst) trd_print_solution((keys), (inst))
#define PROBLEM_WORKSPACE_INIT(ws) trd_workspace_init((ws))
#define PROBLEM_WORKSPACE_CLEAR(ws) trd_workspace_clear((ws))

static inline hscopt_decoder_fn problem_decoder(void) { return trd_decoder; }
static inline hscopt_workspace_clone_fn problem_ws_clone(void) {
  return trd_ws_clone;
}
static inline hscopt_workspace_destroy_fn problem_ws_destroy(void) {
  return trd_ws_destroy;
}

#endif
