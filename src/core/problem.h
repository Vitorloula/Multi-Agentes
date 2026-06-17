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
#define PROBLEM_INIT(inst, path) tsp_create_random_instance((inst))
#define PROBLEM_CLEAR(inst) ((void)(inst))
#define PROBLEM_PRINT_SOLUTION(keys, inst) tsp_print_tour((keys), (inst))
#define PROBLEM_WORKSPACE_INIT(ws, inst) tsp_workspace_init((ws))
#define PROBLEM_WORKSPACE_CLEAR(ws) tsp_workspace_clear((ws))
static inline hscopt_decoder_fn problem_decoder(void) { return tsp_decoder; }
static inline hscopt_workspace_clone_fn problem_ws_clone(void) {
  return tsp_ws_clone;
}
static inline hscopt_workspace_destroy_fn problem_ws_destroy(void) {
  return tsp_ws_destroy;
}
#elif defined(ACTIVE_PROBLEM_TRD)
#include "core/trd/trd.h"
#include "core/trd/trd_decoder.h"
typedef trd_instance problem_instance;
typedef trd_workspace problem_workspace;
#define PROBLEM_NAME() "Total Roman Domination (TRD)"
#define PROBLEM_INIT(inst, path) trd_init_instance((inst), (path))
#define PROBLEM_CLEAR(inst) trd_destroy_instance((inst))
#define PROBLEM_PRINT_SOLUTION(keys, inst) trd_print_solution((keys), (inst))
#define PROBLEM_WORKSPACE_INIT(ws, inst) trd_workspace_init((ws), trd_get_order((inst)))
#define PROBLEM_WORKSPACE_CLEAR(ws) trd_workspace_clear((ws))
static inline hscopt_decoder_fn problem_decoder(void) { return trd_decoder; }
static inline hscopt_workspace_clone_fn problem_ws_clone(void) {
  return trd_ws_clone;
}
static inline hscopt_workspace_destroy_fn problem_ws_destroy(void) {
  return trd_ws_destroy;
}
#else
#include "core/rcmpsp.h"
#include "core/rcmpsp_decoder.h"
typedef rcmpsp_instance problem_instance;
typedef rcmpsp_workspace problem_workspace;
#define PROBLEM_NAME() "RCMPSP"
#define PROBLEM_INIT(inst, path) rcmpsp_create_random_instance((inst))
#define PROBLEM_CLEAR(inst) ((void)(inst))
#define PROBLEM_PRINT_SOLUTION(keys, inst) rcmpsp_print_schedule((keys), (inst))
#define PROBLEM_WORKSPACE_INIT(ws, inst) rcmpsp_workspace_init((ws))
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

