/**
 * @file hho_agent.c
 * @brief Agente autonomo baseado em Harris Hawks Optimization.
 */

#include "core/agents/agent.h"

void hho_agent_run(agent_context *ctx) {
  hscopt_hho_ctx *hho =
      hscopt_hho_create(PROBLEM_SIZE, 15, ctx->max_global_iterations, 1,
                        ctx->decoder, ctx->dctx, &ctx->rng);
  if (hho == NULL) {
    agent_request_stop(ctx);
    return;
  }

  int iterations = 0;
  while (!agent_should_stop(ctx)) {
    double rabbit_keys[PROBLEM_SIZE];
    agent_consult_blackboard(ctx, rabbit_keys, 1);
    hscopt_hho_try_update_rabbit(hho, rabbit_keys);
    hscopt_hho_iterate(hho, 3);

    double fit = hscopt_hho_best_fitness(hho);
    const double *keys = hscopt_hho_best_keys(hho);
    agent_publish(ctx, keys, fit);

    iterations += 3;
    if (iterations >= ctx->max_global_iterations) {
      agent_request_stop(ctx);
    }
  }

  hscopt_hho_destroy(hho);
}
