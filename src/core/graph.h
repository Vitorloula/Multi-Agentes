#ifndef MULTI_AGENTES_GRAPH_H
#define MULTI_AGENTES_GRAPH_H

#include <stddef.h>

typedef struct {
  int order;
  int size;
  int *degree;
  int **adj;
} graph;

int graph_init(graph *g, int order);
void graph_clear(graph *g);
int graph_contains_vertex(const graph *g, int v);
int graph_contains_edge(const graph *g, int u, int v);
int graph_add_edge(graph *g, int u, int v);
const int *graph_neighbors(const graph *g, int v);
int graph_degree(const graph *g, int v);
double graph_density(const graph *g);
void graph_print(const graph *g);
int graph_load_and_normalize(const char *filename, graph *g, int max_order);

#endif
