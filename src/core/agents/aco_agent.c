/**
 * @file aco_agent.c
 * @brief Agente autonomo baseado em Ant Colony Optimization.
 */

#include "core/agents/agent.h"

void aco_agent_run(agent_context *ctx) {
  hscopt_aco_ctx *aco =
      hscopt_aco_create(PROBLEM_SIZE, ctx->params->aco_archive_size,
                        ctx->params->aco_ant_count,
                        ctx->max_global_iterations, ctx->params->max_threads,
                        ctx->params->aco_q, ctx->params->aco_xi,
                        ctx->decoder, ctx->dctx, &ctx->rng);
  if (aco == NULL) {
    agent_request_stop(ctx);
    return;
  }

  for (int it = 0;
       it < ctx->max_global_iterations && !agent_should_stop(ctx); it++) {
    hscopt_aco_iterate(aco, 1);

    double fit = hscopt_aco_best_fitness(aco);
    const double *keys = hscopt_aco_best_keys(aco);
    agent_publish(ctx, keys, fit);

    double guest_keys[PROBLEM_SIZE];
    agent_consult_blackboard(ctx, guest_keys, 0);
    hscopt_aco_try_update_best(aco, guest_keys);
  }

  hscopt_aco_destroy(aco);
}
