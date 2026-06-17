#ifndef MULTI_AGENTES_TRD_H
#define MULTI_AGENTES_TRD_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct trd_instance {
  bool is_matrix;
  void *graph_l;
  void *graph_m;
  int order;
  int size;
} trd_instance;

int trd_init_instance(trd_instance *inst, const char *filename);
void trd_destroy_instance(trd_instance *inst);
void trd_print_solution(const double *keys, const trd_instance *inst);
int trd_validate_solution(const double *keys, const trd_instance *inst);
int trd_get_order(const trd_instance *inst);

#ifdef __cplusplus
}
#endif

#endif // MULTI_AGENTES_TRD_H
