/**
 * @file rvns_agent.c
 * @brief Agente autonomo baseado em RVNS.
 */

#include "core/agents/agent.h"

void rvns_agent_run(agent_context *ctx) {
  hscopt_rvns_ctx *rvns =
      hscopt_rvns_create(PROBLEM_SIZE, 3, ctx->max_global_iterations, 1,
                         ctx->decoder, ctx->dctx, &ctx->rng, NULL);
  if (rvns == NULL) {
    return;
  }

  for (int it = 0;
       it < ctx->max_global_iterations && !agent_should_stop(ctx); it++) {
    double seed_keys[PROBLEM_SIZE];
    agent_consult_blackboard(ctx, seed_keys, 0);
    hscopt_rvns_reset(rvns, seed_keys);
    hscopt_rvns_iterate(rvns, 8);

    double fit = hscopt_rvns_best_fitness(rvns);
    const double *keys = hscopt_rvns_best_keys(rvns);
    agent_publish(ctx, keys, fit);
  }

  hscopt_rvns_destroy(rvns);
}
