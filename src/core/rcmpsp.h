#ifndef MULTI_AGENTES_RCMPSP_H
#define MULTI_AGENTES_RCMPSP_H

/**
 * @file rcmpsp.h
 * @brief Instancia sintetica do Resource Constrained Multi-Project Scheduling.
 */

#include "core/config.h"

/**
 * @brief Instancia do problema RCMPSP.
 *
 * Cada projeto possui uma atividade inicial dummy, atividades reais e uma
 * atividade final dummy. As atividades globais 0 e N_ACTIVITIES-1 conectam os
 * projetos em uma unica rede de precedencias.
 */
typedef struct {
  int n_projects;
  int n_activities;
  int n_resources;
  int duration[N_ACTIVITIES];
  int demand[N_ACTIVITIES][N_RESOURCES];
  int capacity[N_RESOURCES];
  int pred_count[N_ACTIVITIES];
  int predecessors[N_ACTIVITIES][MAX_PREDECESSORS];
  int project_of_activity[N_ACTIVITIES];
  int project_start[N_PROJECTS];
  int project_finish[N_PROJECTS];
  int release_date[N_PROJECTS];
  int due_date[N_PROJECTS];
  int critical_path_duration[N_PROJECTS];
  double tardiness_weight;
  double earliness_weight;
  double flow_weight;
} rcmpsp_instance;

/**
 * @brief Inicializa uma instancia sintetica e reprodutivel de RCMPSP.
 *
 * @param inst Instancia a ser preenchida.
 */
void rcmpsp_create_random_instance(rcmpsp_instance *inst);
void rcmpsp_init_instance(rcmpsp_instance *inst);

/**
 * @brief Imprime o cronograma decodificado a partir de random keys.
 *
 * @param keys Cromossomo com tamanho @ref PROBLEM_SIZE.
 * @param inst Instancia do problema.
 */
void rcmpsp_print_schedule(const double *keys, const rcmpsp_instance *inst);

#endif
