#ifndef MULTI_AGENTES_TRD_H
#define MULTI_AGENTES_TRD_H

#include "core/config.h"
#include "core/graph.h"

typedef graph trd_instance;

typedef struct {
  int labels[PROBLEM_SIZE];
  unsigned char dominated[PROBLEM_SIZE];
} trd_workspace;

int trd_init_instance(trd_instance *inst, const char *filename);
void trd_clear_instance(trd_instance *inst);
int trd_workspace_init(trd_workspace *ws);
void trd_workspace_clear(trd_workspace *ws);
void trd_print_solution(const double *keys, const trd_instance *inst);

#endif
