#include "core/graph.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int u;
  int v;
} edge;

static int compare_ints(const void *a, const void *b) {
  int ia = *(const int *)a;
  int ib = *(const int *)b;
  return (ia > ib) - (ia < ib);
}

static int compare_edges(const void *a, const void *b) {
  const edge *ea = (const edge *)a;
  const edge *eb = (const edge *)b;
  if (ea->u != eb->u) {
    return (ea->u > eb->u) - (ea->u < eb->u);
  }
  return (ea->v > eb->v) - (ea->v < eb->v);
}

static int append_int(int **values, size_t *count, size_t *cap, int value) {
  if (*count == *cap) {
    size_t new_cap = *cap == 0 ? 64 : *cap * 2;
    int *new_values = realloc(*values, new_cap * sizeof(int));
    if (new_values == NULL) {
      return 0;
    }
    *values = new_values;
    *cap = new_cap;
  }
  (*values)[(*count)++] = value;
  return 1;
}

static int append_edge(edge **edges, size_t *count, size_t *cap, int u, int v) {
  if (*count == *cap) {
    size_t new_cap = *cap == 0 ? 64 : *cap * 2;
    edge *new_edges = realloc(*edges, new_cap * sizeof(edge));
    if (new_edges == NULL) {
      return 0;
    }
    *edges = new_edges;
    *cap = new_cap;
  }
  (*edges)[*count].u = u;
  (*edges)[*count].v = v;
  (*count)++;
  return 1;
}

static int find_vertex_id(const int *vertices, size_t count, int value) {
  int *found =
      bsearch(&value, vertices, count, sizeof(int), compare_ints);
  if (found == NULL) {
    return -1;
  }
  return (int)(found - vertices);
}

int graph_init(graph *g, int order) {
  if (order <= 0) {
    return 0;
  }

  g->order = order;
  g->size = 0;
  g->degree = calloc((size_t)order, sizeof(int));
  g->adj = calloc((size_t)order, sizeof(int *));
  if (g->degree == NULL || g->adj == NULL) {
    graph_clear(g);
    return 0;
  }
  return 1;
}

void graph_clear(graph *g) {
  if (g == NULL) {
    return;
  }
  if (g->adj != NULL) {
    for (int i = 0; i < g->order; i++) {
      free(g->adj[i]);
    }
  }
  free(g->adj);
  free(g->degree);
  g->order = 0;
  g->size = 0;
  g->degree = NULL;
  g->adj = NULL;
}

int graph_contains_vertex(const graph *g, int v) {
  return g != NULL && v >= 0 && v < g->order;
}

int graph_contains_edge(const graph *g, int u, int v) {
  if (!graph_contains_vertex(g, u) || !graph_contains_vertex(g, v)) {
    return 0;
  }
  for (int i = 0; i < g->degree[u]; i++) {
    if (g->adj[u][i] == v) {
      return 1;
    }
  }
  return 0;
}

int graph_add_edge(graph *g, int u, int v) {
  if (u == v) {
    return 1;
  }
  if (!graph_contains_vertex(g, u) || !graph_contains_vertex(g, v)) {
    return 0;
  }
  if (graph_contains_edge(g, u, v)) {
    return 1;
  }

  int *u_adj = realloc(g->adj[u], (size_t)(g->degree[u] + 1) * sizeof(int));
  if (u_adj == NULL) {
    return 0;
  }
  g->adj[u] = u_adj;
  g->adj[u][g->degree[u]++] = v;

  int *v_adj = realloc(g->adj[v], (size_t)(g->degree[v] + 1) * sizeof(int));
  if (v_adj == NULL) {
    g->degree[u]--;
    return 0;
  }
  g->adj[v] = v_adj;
  g->adj[v][g->degree[v]++] = u;
  g->size++;
  return 1;
}

const int *graph_neighbors(const graph *g, int v) {
  if (!graph_contains_vertex(g, v)) {
    return NULL;
  }
  return g->adj[v];
}

int graph_degree(const graph *g, int v) {
  if (!graph_contains_vertex(g, v)) {
    return -1;
  }
  return g->degree[v];
}

double graph_density(const graph *g) {
  if (g == NULL || g->order <= 1) {
    return 0.0;
  }
  return (2.0 * (double)g->size) / ((double)g->order * (double)(g->order - 1));
}

void graph_print(const graph *g) {
  printf("Graph: %d vertices, %d edges, density %.4f\n", g->order, g->size,
         graph_density(g));
  for (int u = 0; u < g->order; u++) {
    printf("%d:", u);
    for (int i = 0; i < g->degree[u]; i++) {
      printf(" %d", g->adj[u][i]);
    }
    putchar('\n');
  }
}

int graph_load_and_normalize(const char *filename, graph *g, int max_order) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "Nao foi possivel abrir o arquivo: %s\n", filename);
    return 0;
  }

  int *vertices = NULL;
  edge *input_edges = NULL;
  size_t vertex_count = 0;
  size_t vertex_cap = 0;
  size_t edge_count = 0;
  size_t edge_cap = 0;
  int ok = 1;
  char line[1024];
  int u;
  int v;

  while (fgets(line, sizeof(line), file) != NULL) {
    if (sscanf(line, "%d %d", &u, &v) != 2) {
      continue;
    }
    if (!append_int(&vertices, &vertex_count, &vertex_cap, u) ||
        !append_int(&vertices, &vertex_count, &vertex_cap, v)) {
      ok = 0;
      break;
    }
    if (u != v && !append_edge(&input_edges, &edge_count, &edge_cap, u, v)) {
      ok = 0;
      break;
    }
  }
  fclose(file);

  if (!ok || vertex_count == 0) {
    fprintf(stderr, "Nenhum vertice encontrado na entrada ou falta de memoria.\n");
    free(vertices);
    free(input_edges);
    return 0;
  }

  qsort(vertices, vertex_count, sizeof(int), compare_ints);
  size_t unique_vertices = 0;
  for (size_t i = 0; i < vertex_count; i++) {
    if (i == 0 || vertices[i] != vertices[i - 1]) {
      vertices[unique_vertices++] = vertices[i];
    }
  }

  if ((int)unique_vertices > max_order) {
    fprintf(stderr,
            "Grafo com %zu vertices excede PROBLEM_SIZE=%d. Reconfigure com "
            "-DMA_TRD_MAX_VERTICES maior.\n",
            unique_vertices, max_order);
    free(vertices);
    free(input_edges);
    return 0;
  }

  edge *normalized = malloc(edge_count * sizeof(edge));
  if (edge_count > 0 && normalized == NULL) {
    free(vertices);
    free(input_edges);
    return 0;
  }

  for (size_t i = 0; i < edge_count; i++) {
    int nu = find_vertex_id(vertices, unique_vertices, input_edges[i].u);
    int nv = find_vertex_id(vertices, unique_vertices, input_edges[i].v);
    if (nu > nv) {
      int tmp = nu;
      nu = nv;
      nv = tmp;
    }
    normalized[i].u = nu;
    normalized[i].v = nv;
  }

  qsort(normalized, edge_count, sizeof(edge), compare_edges);
  graph_clear(g);
  if (!graph_init(g, (int)unique_vertices)) {
    free(vertices);
    free(input_edges);
    free(normalized);
    return 0;
  }

  for (size_t i = 0; i < edge_count; i++) {
    if (i > 0 && normalized[i].u == normalized[i - 1].u &&
        normalized[i].v == normalized[i - 1].v) {
      continue;
    }
    if (!graph_add_edge(g, normalized[i].u, normalized[i].v)) {
      graph_clear(g);
      free(vertices);
      free(input_edges);
      free(normalized);
      return 0;
    }
  }

  free(vertices);
  free(input_edges);
  free(normalized);
  return 1;
}
