#include "core/trd/trd.h"
#include "ListGraph.hpp"
#include "MatrixGraph.hpp"
#include "Chromosome.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <unordered_map>
#include <vector>
#include <algorithm>

extern "C" {

int trd_init_instance(trd_instance *inst, const char *filename) {
  inst->is_matrix = false;
  inst->graph_l = nullptr;
  inst->graph_m = nullptr;
  inst->order = 0;
  inst->size = 0;

  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Erro: Não foi possível abrir o arquivo: " << filename << std::endl;
    return 0;
  }

  std::set<int> vertices;
  std::vector<std::pair<int, int>> edges;
  std::string line;

  while (std::getline(file, line)) {
    std::istringstream iss(line);
    int u, v;
    if (!(iss >> u >> v)) {
      continue;
    }
    vertices.insert(u);
    vertices.insert(v);
    if (u != v) {  // Ignorando arestas reflexivas
      edges.emplace_back(u, v);
    }
  }

  if (vertices.empty()) {
    std::cerr << "Erro: Nenhum vértice encontrado na entrada" << std::endl;
    return 0;
  }

  int num_vertices = vertices.size();
  if (num_vertices > MA_TRD_MAX_VERTICES) {
    std::cerr << "Erro: O numero de vertices do grafo (" << num_vertices 
              << ") excede o limite maximo (" << MA_TRD_MAX_VERTICES 
              << ") configurado. Recompile alterando MA_TRD_MAX_VERTICES." << std::endl;
    return 0;
  }

  // Criar mapeamento de IDs originais para novos IDs sequenciais de 0 a V-1
  std::unordered_map<int, int> id_map;
  int new_id = 0;
  for (int original_id : vertices) {
    id_map[original_id] = new_id++;
  }

  double max_possible_edges = num_vertices * (num_vertices - 1) / 2.0;

  // Normalizar arestas (remover duplicatas e tornar não-direcionadas)
  std::set<std::pair<int, int>> unique_edges;
  for (const auto &edge : edges) {
    int norm_u = id_map[edge.first];
    int norm_v = id_map[edge.second];
    if (norm_u > norm_v) {
      std::swap(norm_u, norm_v);
    }
    unique_edges.insert({norm_u, norm_v});
  }

  double density = (max_possible_edges > 0) ? (unique_edges.size() / max_possible_edges) : 0.0;

  inst->order = num_vertices;
  inst->size = unique_edges.size();

  std::cout << "Carregando grafo de " << filename << " com " << num_vertices 
            << " vertices e " << unique_edges.size() << " arestas (densidade: " 
            << density << ")..." << std::endl;

  if (density > 0.5) {
    inst->is_matrix = true;
    MatrixGraph *graph_m = new MatrixGraph(num_vertices);
    for (const auto &edge : unique_edges) {
      graph_m->add_edge(edge.first, edge.second);
    }
    inst->graph_m = static_cast<void*>(graph_m);
    std::cout << "Usando representacao com matriz de adjacencia (grafo denso)." << std::endl;
  } else {
    inst->is_matrix = false;
    ListGraph *graph_l = new ListGraph(num_vertices);
    for (const auto &edge : unique_edges) {
      graph_l->add_edge(edge.first, edge.second);
    }
    inst->graph_l = static_cast<void*>(graph_l);
    std::cout << "Usando representacao com lista de adjacencia (grafo esparso)." << std::endl;
  }

  return 1;
}

void trd_destroy_instance(trd_instance *inst) {
  if (inst) {
    if (inst->graph_l) {
      delete static_cast<ListGraph*>(inst->graph_l);
      inst->graph_l = nullptr;
    }
    if (inst->graph_m) {
      delete static_cast<MatrixGraph*>(inst->graph_m);
      inst->graph_m = nullptr;
    }
  }
}

int trd_get_order(const trd_instance *inst) {
  return inst ? inst->order : 0;
}

void trd_print_solution(const double *keys, const trd_instance *inst) {
  if (!inst || inst->order <= 0) {
    return;
  }
  Chromosome chr(inst->order);
  for (int i = 0; i < inst->order; ++i) {
    double gene = keys[i];
    int label;
    if (gene < 0.3333) {
      label = LABEL_ZERO;
    } else if (gene < 0.6666) {
      label = LABEL_ONE;
    } else {
      label = LABEL_TWO;
    }
    chr.set_value(i, label);
  }

  if (inst->is_matrix) {
    chr.fix_m(*(static_cast<MatrixGraph*>(inst->graph_m)));
  } else {
    chr.fix_l(*(static_cast<ListGraph*>(inst->graph_l)));
  }

  std::cout << chr << std::endl;

  // Realizar validação independente das restrições de TRD
  int valid = trd_validate_solution(keys, inst);
  if (valid) {
    std::cout << "[VALIDACAO TRD] Status: OK (Solucao respeita todas as restricoes!)" << std::endl;
  } else {
    std::cout << "[VALIDACAO TRD] Status: FALHA (Solucao nao respeita as restricoes!)" << std::endl;
  }
}

int trd_validate_solution(const double *keys, const trd_instance *inst) {
  if (!inst || inst->order <= 0) {
    return 0;
  }
  Chromosome chr(inst->order);
  for (int i = 0; i < inst->order; ++i) {
    double gene = keys[i];
    int label;
    if (gene < 0.3333) {
      label = LABEL_ZERO;
    } else if (gene < 0.6666) {
      label = LABEL_ONE;
    } else {
      label = LABEL_TWO;
    }
    chr.set_value(i, label);
  }

  if (inst->is_matrix) {
    chr.fix_m(*(static_cast<MatrixGraph*>(inst->graph_m)));
  } else {
    chr.fix_l(*(static_cast<ListGraph*>(inst->graph_l)));
  }

  // Verificar as duas condições do Total Roman Domination:
  // 1. Todo vértice com rótulo 0 precisa ter pelo menos um vizinho com rótulo 2.
  // 2. Todo vértice ativo (rótulo 1 ou 2) precisa ter pelo menos um vizinho ativo (rótulo >= 1).
  for (int u = 0; u < inst->order; ++u) {
    int val_u = chr.get_value(u);
    if (val_u == 0) {
      bool has_neighbor_two = false;
      if (inst->is_matrix) {
        const auto &neighbors = static_cast<MatrixGraph*>(inst->graph_m)->get_neighbors(u);
        for (int v = 0; v < inst->order; ++v) {
          if (neighbors[v] && chr.get_value(v) == 2) {
            has_neighbor_two = true;
            break;
          }
        }
      } else {
        const auto &neighbors = static_cast<ListGraph*>(inst->graph_l)->get_neighbors(u);
        for (int v : neighbors) {
          if (chr.get_value(v) == 2) {
            has_neighbor_two = true;
            break;
          }
        }
      }
      if (!has_neighbor_two) {
        std::cerr << "[VALIDACAO TRD] Erro: Vertice " << u << " tem rotulo 0 mas nenhum vizinho possui rotulo 2!" << std::endl;
        return 0;
      }
    } else {
      // Vértice ativo (1 ou 2)
      bool has_active_neighbor = false;
      if (inst->is_matrix) {
        const auto &neighbors = static_cast<MatrixGraph*>(inst->graph_m)->get_neighbors(u);
        for (int v = 0; v < inst->order; ++v) {
          if (neighbors[v] && chr.get_value(v) >= 1) {
            has_active_neighbor = true;
            break;
          }
        }
      } else {
        const auto &neighbors = static_cast<ListGraph*>(inst->graph_l)->get_neighbors(u);
        for (int v : neighbors) {
          if (chr.get_value(v) >= 1) {
            has_active_neighbor = true;
            break;
          }
        }
      }
      if (!has_active_neighbor) {
        std::cerr << "[VALIDACAO TRD] Erro: Vertice ativo " << u << " (rotulo " << val_u << ") nao possui nenhum vizinho ativo (isolado)!" << std::endl;
        return 0;
      }
    }
  }

  return 1;
}

}
