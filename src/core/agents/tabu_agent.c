/**
 * @file tabu_agent.c
 * @brief Agente autonomo baseado em Tabu Search.
 */

#include "core/agents/agent.h"

void tabu_agent_run(agent_context *ctx) {
  hscopt_ts_ctx *ts =
      hscopt_ts_create(PROBLEM_SIZE, ctx->params->tabu_neighborhood_size,
                       ctx->params->tabu_tenure, ctx->max_global_iterations, 1,
                       ctx->decoder, ctx->dctx, &ctx->rng, NULL);
  if (ts == NULL) {
    return;
  }

  for (int it = 0;
       it < ctx->max_global_iterations && !agent_should_stop(ctx); it++) {
    double seed_keys[PROBLEM_SIZE];
    agent_consult_blackboard(ctx, seed_keys, 1);
    hscopt_ts_reset(ts, seed_keys);
    hscopt_ts_iterate(ts, ctx->params->tabu_inner_iterations);

    double fit = hscopt_ts_best_fitness(ts);
    const double *keys = hscopt_ts_best_keys(ts);
    agent_publish(ctx, keys, fit);
  }

  hscopt_ts_destroy(ts);
}
