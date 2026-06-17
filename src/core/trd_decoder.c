#include "core/trd_decoder.h"
#include "core/decoder_adapter.h"

#include <stdlib.h>
#include <string.h>

static int chromosome_fitness(const trd_instance *graph, const int *labels) {
  int fitness = 0;
  for (int i = 0; i < graph->order; i++) {
    fitness += labels[i];
  }
  return fitness;
}

static void fix_chromosome(const trd_instance *graph, trd_workspace *ws) {
  memset(ws->dominated, 0, (size_t)graph->order * sizeof(ws->dominated[0]));

  for (int u = 0; u < graph->order; u++) {
    if (ws->labels[u] == LABEL_ZERO && !ws->dominated[u]) {
      int num_active = 0;
      int has_neighbor_with_label_two = 0;
      int vertex_with_label_one = -1;
      int last_neighbor = -1;

      for (int i = 0; i < graph->degree[u]; i++) {
        int w = graph->adj[u][i];
        last_neighbor = w;
        if (ws->labels[w] == LABEL_ONE) {
          num_active++;
          vertex_with_label_one = w;
        } else if (ws->labels[w] == LABEL_TWO) {
          num_active++;
          has_neighbor_with_label_two = 1;
          break;
        }
      }

      if (num_active == 0 && last_neighbor != -1) {
        ws->labels[last_neighbor] = LABEL_TWO;
      } else if (!has_neighbor_with_label_two && vertex_with_label_one != -1) {
        ws->labels[vertex_with_label_one] = LABEL_TWO;
      }
      ws->dominated[u] = 1;
    } else if (ws->labels[u] == LABEL_TWO) {
      int num_active = 0;
      int last_neighbor = -1;

      for (int i = 0; i < graph->degree[u]; i++) {
        int w = graph->adj[u][i];
        last_neighbor = w;
        ws->dominated[w] = 1;
        if (ws->labels[w] >= LABEL_ONE) {
          num_active++;
        }
      }

      if (num_active == 0 && last_neighbor != -1) {
        ws->labels[last_neighbor] = LABEL_ONE;
      }
    } else if (ws->labels[u] == LABEL_ONE && !ws->dominated[u]) {
      int num_active = 0;
      int last_neighbor = -1;

      for (int i = 0; i < graph->degree[u]; i++) {
        int w = graph->adj[u][i];
        last_neighbor = w;
        if (ws->labels[w] >= LABEL_ONE) {
          num_active++;
          break;
        }
      }

      if (num_active == 0 && last_neighbor != -1) {
        ws->labels[last_neighbor] = LABEL_ONE;
        ws->dominated[last_neighbor] = 1;
      }
      ws->dominated[u] = 1;
    }
  }
}

hscopt_workspace *trd_ws_clone(const hscopt_workspace *ws, void *user) {
  (void)ws;
  (void)user;

  trd_workspace *new_ws = malloc(sizeof(trd_workspace));
  if (new_ws == NULL) {
    return NULL;
  }
  trd_workspace_init(new_ws);
  return (hscopt_workspace *)new_ws;
}

void trd_ws_destroy(hscopt_workspace *ws, void *user) {
  (void)user;
  free(ws);
}

double trd_brkga_decode(const double *chromosome, size_t n,
                        const trd_instance *graph, trd_workspace *ws) {
  if (ws == NULL || n < (size_t)graph->order) {
    return 1.0e30;
  }

  for (int i = 0; i < graph->order; i++) {
    double gene = chromosome[i];
    if (gene < 0.3333) {
      ws->labels[i] = LABEL_ZERO;
    } else if (gene < 0.6666) {
      ws->labels[i] = LABEL_ONE;
    } else {
      ws->labels[i] = LABEL_TWO;
    }
  }

  fix_chromosome(graph, ws);
  return (double)chromosome_fitness(graph, ws->labels);
}

DEFINE_HSCOPT_DECODER_ADAPTER(trd_decoder, trd_brkga_decode, trd_instance,
                              trd_workspace)
