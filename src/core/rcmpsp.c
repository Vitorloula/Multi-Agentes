/**
 * @file rcmpsp.c
 * @brief Geracao e impressao de instancias RCMPSP.
 */

#include "core/rcmpsp.h"
#include "core/rcmpsp_decoder.h"

#include <stdio.h>
#include <stdlib.h>

static void add_predecessor(rcmpsp_instance *inst, int activity,
                            int predecessor) {
  int count = inst->pred_count[activity];
  if (count < MAX_PREDECESSORS) {
    inst->predecessors[activity][count] = predecessor;
    inst->pred_count[activity]++;
  }
}

static int longest_project_path(const rcmpsp_instance *inst, int project) {
  int start = inst->project_start[project];
  int finish = inst->project_finish[project];
  int longest[N_ACTIVITIES] = {0};

  for (int activity = start; activity <= finish; activity++) {
    int best_pred = 0;
    for (int p = 0; p < inst->pred_count[activity]; p++) {
      int pred = inst->predecessors[activity][p];
      if (pred >= start && pred <= finish && longest[pred] > best_pred) {
        best_pred = longest[pred];
      }
    }
    longest[activity] = best_pred + inst->duration[activity];
  }

  return longest[finish] > 0 ? longest[finish] : 1;
}

void rcmpsp_create_random_instance(rcmpsp_instance *inst) {
  inst->n_projects = N_PROJECTS;
  inst->n_activities = N_ACTIVITIES;
  inst->n_resources = N_RESOURCES;
  inst->tardiness_weight = 1.0;
  inst->earliness_weight = 0.25;
  inst->flow_weight = 0.10;

  srand(100);

  for (int r = 0; r < N_RESOURCES; r++) {
    inst->capacity[r] = 6 + r;
  }

  for (int a = 0; a < N_ACTIVITIES; a++) {
    inst->duration[a] = 0;
    inst->pred_count[a] = 0;
    inst->project_of_activity[a] = -1;
    for (int r = 0; r < N_RESOURCES; r++) {
      inst->demand[a][r] = 0;
    }
  }

  int global_start = 0;
  int global_finish = N_ACTIVITIES - 1;

  for (int project = 0; project < N_PROJECTS; project++) {
    int start = 1 + project * PROJECT_ACTIVITY_COUNT;
    int finish = start + PROJECT_ACTIVITY_COUNT - 1;
    inst->project_start[project] = start;
    inst->project_finish[project] = finish;
    inst->release_date[project] = project * 3;

    inst->project_of_activity[start] = project;
    inst->project_of_activity[finish] = project;
    add_predecessor(inst, start, global_start);

    for (int i = 0; i < PROJECT_REAL_ACTIVITIES; i++) {
      int activity = start + 1 + i;
      inst->project_of_activity[activity] = project;
      inst->duration[activity] = 1 + (rand() % 8);
      for (int r = 0; r < N_RESOURCES; r++) {
        inst->demand[activity][r] = rand() % 4;
      }

      if (i == 0) {
        add_predecessor(inst, activity, start);
      } else {
        add_predecessor(inst, activity, activity - 1);
      }
      if (i >= 2 && (i % 3) == 0) {
        add_predecessor(inst, activity, activity - 2);
      }
    }

    add_predecessor(inst, finish, finish - 1);
    add_predecessor(inst, finish, finish - 2);
    add_predecessor(inst, global_finish, finish);
  }

  for (int project = 0; project < N_PROJECTS; project++) {
    int cp = longest_project_path(inst, project);
    inst->critical_path_duration[project] = cp;
    inst->due_date[project] = inst->release_date[project] + cp + 8;
  }

  printf("Gerando instancia RCMPSP com %d projetos, %d atividades e %d "
         "recursos...\n",
         N_PROJECTS, N_ACTIVITIES, N_RESOURCES);
}

void rcmpsp_init_instance(rcmpsp_instance *inst) {
  rcmpsp_create_random_instance(inst);
}

void rcmpsp_print_schedule(const double *keys, const rcmpsp_instance *inst) {
  rcmpsp_workspace ws;
  if (!rcmpsp_workspace_init(&ws)) {
    puts("Nao foi possivel alocar workspace para imprimir o cronograma.");
    return;
  }

  double penalty = rcmpsp_decode_schedule(keys, PROBLEM_SIZE, inst, &ws);
  printf("Penalidade validada: %.2f\n", penalty);

  puts("Projetos:");
  for (int p = 0; p < N_PROJECTS; p++) {
    int start = inst->project_start[p];
    int finish = inst->project_finish[p];
    int begin = ws.start_time[start];
    int conclusion = ws.finish_time[finish];
    int tardiness =
        conclusion > inst->due_date[p] ? conclusion - inst->due_date[p] : 0;
    int earliness =
        conclusion < inst->due_date[p] ? inst->due_date[p] - conclusion : 0;

    printf("  P%d: release=%d due=%d inicio=%d fim=%d E=%d T=%d\n", p + 1,
           inst->release_date[p], inst->due_date[p], begin, conclusion,
           earliness, tardiness);
  }

  rcmpsp_workspace_clear(&ws);
}
