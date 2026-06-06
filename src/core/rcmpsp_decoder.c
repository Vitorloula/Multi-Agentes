/**
 * @file rcmpsp_decoder.c
 * @brief Decoder BRKGA para Resource Constrained Multi-Project Scheduling.
 */

#include "core/rcmpsp_decoder.h"

#include <math.h>
#include <stdlib.h>

int rcmpsp_workspace_init(rcmpsp_workspace *ws) {
  ws->scheduled = calloc(N_ACTIVITIES, sizeof(unsigned char));
  ws->start_time = calloc(N_ACTIVITIES, sizeof(int));
  ws->finish_time = calloc(N_ACTIVITIES, sizeof(int));
  ws->resource_usage =
      calloc(RCMPSP_MAX_HORIZON * N_RESOURCES, sizeof(int));

  if (ws->scheduled == NULL || ws->start_time == NULL ||
      ws->finish_time == NULL || ws->resource_usage == NULL) {
    rcmpsp_workspace_clear(ws);
    return 0;
  }

  return 1;
}

void rcmpsp_workspace_clear(rcmpsp_workspace *ws) {
  free(ws->scheduled);
  free(ws->start_time);
  free(ws->finish_time);
  free(ws->resource_usage);
  ws->scheduled = NULL;
  ws->start_time = NULL;
  ws->finish_time = NULL;
  ws->resource_usage = NULL;
}

hscopt_workspace *rcmpsp_ws_clone(const hscopt_workspace *ws, void *user) {
  (void)ws;
  (void)user;

  rcmpsp_workspace *new_ws = malloc(sizeof(rcmpsp_workspace));
  if (new_ws == NULL) {
    return NULL;
  }
  new_ws->scheduled = NULL;
  new_ws->start_time = NULL;
  new_ws->finish_time = NULL;
  new_ws->resource_usage = NULL;

  if (!rcmpsp_workspace_init(new_ws)) {
    free(new_ws);
    return NULL;
  }

  return (hscopt_workspace *)new_ws;
}

void rcmpsp_ws_destroy(hscopt_workspace *ws, void *user) {
  (void)user;

  rcmpsp_workspace *rc_ws = (rcmpsp_workspace *)ws;
  if (rc_ws == NULL) {
    return;
  }

  rcmpsp_workspace_clear(rc_ws);
  free(rc_ws);
}

static int usage_index(int time, int resource) {
  return time * N_RESOURCES + resource;
}

static int max_duration(const rcmpsp_instance *inst) {
  int max_value = 1;
  for (int a = 0; a < N_ACTIVITIES; a++) {
    if (inst->duration[a] > max_value) {
      max_value = inst->duration[a];
    }
  }
  return max_value;
}

static int predecessor_finish_bound(const rcmpsp_instance *inst,
                                    const rcmpsp_workspace *ws,
                                    int activity) {
  int bound = 0;
  for (int i = 0; i < inst->pred_count[activity]; i++) {
    int pred = inst->predecessors[activity][i];
    if (!ws->scheduled[pred]) {
      return -1;
    }
    if (ws->finish_time[pred] > bound) {
      bound = ws->finish_time[pred];
    }
  }
  return bound;
}

static int release_bound(const rcmpsp_instance *inst, int activity) {
  int project = inst->project_of_activity[activity];
  if (project < 0) {
    return 0;
  }
  return inst->release_date[project];
}

static int resources_available(const rcmpsp_instance *inst,
                               const rcmpsp_workspace *ws, int activity,
                               int start) {
  int duration = inst->duration[activity];
  if (start + duration >= RCMPSP_MAX_HORIZON) {
    return 0;
  }

  for (int t = start; t < start + duration; t++) {
    for (int r = 0; r < N_RESOURCES; r++) {
      int used = ws->resource_usage[usage_index(t, r)];
      if (used + inst->demand[activity][r] > inst->capacity[r]) {
        return 0;
      }
    }
  }

  return 1;
}

static void reserve_resources(const rcmpsp_instance *inst, rcmpsp_workspace *ws,
                              int activity, int start) {
  for (int t = start; t < start + inst->duration[activity]; t++) {
    for (int r = 0; r < N_RESOURCES; r++) {
      ws->resource_usage[usage_index(t, r)] += inst->demand[activity][r];
    }
  }
}

static int earliest_feasible_start(const rcmpsp_instance *inst,
                                   const rcmpsp_workspace *ws, int activity,
                                   int min_start) {
  for (int start = min_start; start < RCMPSP_MAX_HORIZON; start++) {
    if (resources_available(inst, ws, activity, start)) {
      return start;
    }
  }
  return RCMPSP_MAX_HORIZON - 1;
}

static void schedule_activity(const rcmpsp_instance *inst, rcmpsp_workspace *ws,
                              int activity, int start) {
  ws->scheduled[activity] = 1;
  ws->start_time[activity] = start;
  ws->finish_time[activity] = start + inst->duration[activity];
  reserve_resources(inst, ws, activity, start);
}

static int choose_candidate(const double *keys, const rcmpsp_instance *inst,
                            const rcmpsp_workspace *ws, int time,
                            int max_delay) {
  int best_activity = -1;
  double best_priority = -1.0;

  for (int activity = 0; activity < N_ACTIVITIES; activity++) {
    if (ws->scheduled[activity]) {
      continue;
    }

    int pred_finish = predecessor_finish_bound(inst, ws, activity);
    if (pred_finish < 0) {
      continue;
    }

    int delay = (int)(keys[N_ACTIVITIES + activity] * 1.5 * max_delay);
    if (pred_finish > time + delay) {
      continue;
    }

    if (keys[activity] > best_priority) {
      best_priority = keys[activity];
      best_activity = activity;
    }
  }

  return best_activity;
}

static int next_decision_time(const rcmpsp_workspace *ws, int scheduled_count,
                              int current_time) {
  int next_time = RCMPSP_MAX_HORIZON - 1;
  for (int a = 0; a < N_ACTIVITIES; a++) {
    if (ws->scheduled[a] && ws->finish_time[a] > current_time &&
        ws->finish_time[a] < next_time) {
      next_time = ws->finish_time[a];
    }
  }

  if (scheduled_count == 0 || next_time == RCMPSP_MAX_HORIZON - 1) {
    return current_time + 1;
  }
  return next_time;
}

static double schedule_penalty(const rcmpsp_instance *inst,
                               const rcmpsp_workspace *ws) {
  double penalty = 0.0;

  for (int p = 0; p < N_PROJECTS; p++) {
    int start = inst->project_start[p];
    int finish = inst->project_finish[p];
    double bd = (double)ws->start_time[start];
    double cd = (double)ws->finish_time[finish];
    double due = (double)inst->due_date[p];
    double tardiness = cd > due ? cd - due : 0.0;
    double earliness = cd < due ? due - cd : 0.0;
    double critical_path = (double)inst->critical_path_duration[p];
    double flow = cd - bd;

    penalty += inst->tardiness_weight * tardiness * tardiness * tardiness;
    penalty += inst->earliness_weight * earliness * earliness;
    penalty += inst->flow_weight * flow * flow / critical_path;
  }

  return penalty;
}

double rcmpsp_decode_schedule(const double *keys, size_t n,
                              const rcmpsp_instance *inst,
                              rcmpsp_workspace *ws) {
  (void)n;

  for (int a = 0; a < N_ACTIVITIES; a++) {
    ws->scheduled[a] = 0;
    ws->start_time[a] = 0;
    ws->finish_time[a] = 0;
  }
  for (int i = 0; i < RCMPSP_MAX_HORIZON * N_RESOURCES; i++) {
    ws->resource_usage[i] = 0;
  }

  int scheduled_count = 0;
  int time = 0;
  int max_delay = max_duration(inst);

  while (scheduled_count < N_ACTIVITIES && time < RCMPSP_MAX_HORIZON - 1) {
    int scheduled_this_time = 0;

    while (1) {
      int activity = choose_candidate(keys, inst, ws, time, max_delay);
      if (activity < 0) {
        break;
      }

      int pred_finish = predecessor_finish_bound(inst, ws, activity);
      int min_start = pred_finish > release_bound(inst, activity)
                          ? pred_finish
                          : release_bound(inst, activity);
      int start = earliest_feasible_start(inst, ws, activity, min_start);
      schedule_activity(inst, ws, activity, start);
      scheduled_count++;
      scheduled_this_time = 1;
    }

    if (!scheduled_this_time) {
      time = next_decision_time(ws, scheduled_count, time);
    }
  }

  if (scheduled_count < N_ACTIVITIES) {
    return 1e9 + (double)(N_ACTIVITIES - scheduled_count) * 1e6;
  }

  return schedule_penalty(inst, ws);
}

double rcmpsp_decoder(const double *keys, size_t n, hscopt_decode_ctx *ctx) {
  const rcmpsp_instance *inst = (const rcmpsp_instance *)ctx->inst;
  rcmpsp_workspace *ws = (rcmpsp_workspace *)ctx->ws;
  return rcmpsp_decode_schedule(keys, n, inst, ws);
}
