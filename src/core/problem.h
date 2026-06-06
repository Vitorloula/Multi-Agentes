#ifndef MULTI_AGENTES_PROBLEM_H
#define MULTI_AGENTES_PROBLEM_H

/**
 * @file problem.h
 * @brief Adaptador do problema ativo usado pelos agentes.
 */

#include "hscopt/hscopt.h"

#if defined(ACTIVE_PROBLEM_TSP)
#include "core/tsp.h"
#include "core/tsp_decoder.h"
typedef tsp_instance problem_instance;
typedef tsp_workspace problem_workspace;
#define PROBLEM_NAME() "TSP"
#define PROBLEM_INIT(inst) tsp_create_random_instance((inst))
#define PROBLEM_PRINT_SOLUTION(keys, inst) tsp_print_tour((keys), (inst))
#define PROBLEM_WORKSPACE_INIT(ws) tsp_workspace_init((ws))
#define PROBLEM_WORKSPACE_CLEAR(ws) tsp_workspace_clear((ws))
static inline hscopt_decoder_fn problem_decoder(void) { return tsp_decoder; }
static inline hscopt_workspace_clone_fn problem_ws_clone(void) {
  return tsp_ws_clone;
}
static inline hscopt_workspace_destroy_fn problem_ws_destroy(void) {
  return tsp_ws_destroy;
}
#else
#include "core/rcmpsp.h"
#include "core/rcmpsp_decoder.h"
typedef rcmpsp_instance problem_instance;
typedef rcmpsp_workspace problem_workspace;
#define PROBLEM_NAME() "RCMPSP"
#define PROBLEM_INIT(inst) rcmpsp_create_random_instance((inst))
#define PROBLEM_PRINT_SOLUTION(keys, inst) rcmpsp_print_schedule((keys), (inst))
#define PROBLEM_WORKSPACE_INIT(ws) rcmpsp_workspace_init((ws))
#define PROBLEM_WORKSPACE_CLEAR(ws) rcmpsp_workspace_clear((ws))
static inline hscopt_decoder_fn problem_decoder(void) { return rcmpsp_decoder; }
static inline hscopt_workspace_clone_fn problem_ws_clone(void) {
  return rcmpsp_ws_clone;
}
static inline hscopt_workspace_destroy_fn problem_ws_destroy(void) {
  return rcmpsp_ws_destroy;
}
#endif

#endif
