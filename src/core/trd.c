#include "core/trd.h"
#include "core/trd_decoder.h"

#include <stdio.h>

int trd_init_instance(trd_instance *inst, const char *filename) {
  inst->order = 0;
  inst->size = 0;
  inst->degree = NULL;
  inst->adj = NULL;

  if (!graph_load_and_normalize(filename, inst, PROBLEM_SIZE)) {
    return 0;
  }

  printf("Grafo TRD carregado de %s\n", filename);
  graph_print(inst);
  return 1;
}

void trd_clear_instance(trd_instance *inst) { graph_clear(inst); }

int trd_workspace_init(trd_workspace *ws) {
  for (int i = 0; i < PROBLEM_SIZE; i++) {
    ws->labels[i] = LABEL_ZERO;
    ws->dominated[i] = 0;
  }
  return 1;
}

void trd_workspace_clear(trd_workspace *ws) { (void)ws; }

void trd_print_solution(const double *keys, const trd_instance *inst) {
  trd_workspace ws;
  hscopt_decode_ctx ctx = {.inst = (const hscopt_instance *)inst,
                           .user = NULL,
                           .ws = (hscopt_workspace *)&ws,
                           .ws_clone = trd_ws_clone,
                           .ws_destroy = trd_ws_destroy};

  trd_workspace_init(&ws);
  double fitness = trd_decoder(keys, (size_t)PROBLEM_SIZE, &ctx);

  printf("Rotulos TRD: ");
  for (int i = 0; i < inst->order; i++) {
    printf("%d:%d ", i, ws.labels[i]);
  }
  printf("\nFitness TRD: %.0f\n", fitness);
}
