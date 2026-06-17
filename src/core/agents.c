/**
 * @file agents.c
 * @brief Orquestrador dos agentes cooperativos executados com OpenMP.
 *
 * Este modulo nao implementa uma meta-heuristica especifica. Ele registra os
 * agentes disponiveis, cria um contexto independente para cada um e executa as
 * estrategias em paralelo. A coordenacao e descentralizada: cada agente decide
 * quando consultar ou publicar no blackboard.
 */

#include "core/agents.h"
#include "core/agents/agent.h"
#include "core/problem.h"

#include <stddef.h>
#include <stdio.h>

/**
 * @brief Avanca a sequencia de um gerador pseudoaleatorio.
 *
 * @param rng Gerador a ser avancado.
 * @param jumps Quantidade de saltos aplicados.
 */
static void jump_rng(hscopt_rng *rng, int jumps) {
  for (int i = 0; i < jumps; i++) {
    hscopt_rng_jump(rng);
  }
}

void agent_context_init(agent_context *ctx, const agent *spec, blackboard *bb,
                        hscopt_decode_ctx *dctx, int *stop_criterion_met,
                        int max_global_iterations) {
  ctx->name = spec->name;
  ctx->role = spec->role;
  ctx->bb = bb;
  ctx->dctx = dctx;
  ctx->decoder = problem_decoder();
  ctx->eval_dctx = *dctx;
  ctx->stop_criterion_met = stop_criterion_met;
  ctx->max_global_iterations = max_global_iterations;
  ctx->accepted_publications = 0;
  ctx->consultations = 0;
  ctx->shared_reads = 0;
  ctx->eval_dctx.ws = dctx->ws_clone(dctx->ws, dctx->user);

  hscopt_rng_seed(&ctx->rng, spec->seed);
  jump_rng(&ctx->rng, spec->rng_jumps);
}

void agent_context_clear(agent_context *ctx) {
  if (ctx->eval_dctx.ws != NULL) {
    ctx->eval_dctx.ws_destroy(ctx->eval_dctx.ws, ctx->eval_dctx.user);
    ctx->eval_dctx.ws = NULL;
  }
}

int agent_should_stop(const agent_context *ctx) {
  int stop = 0;
#pragma omp atomic read
  stop = *ctx->stop_criterion_met;
  return stop;
}

void agent_request_stop(agent_context *ctx) {
#pragma omp atomic write
  *ctx->stop_criterion_met = 1;
}

int agent_publish(agent_context *ctx, const double *keys, double fitness) {
  (void)fitness;

  double checked_fitness = ctx->decoder(keys, PROBLEM_SIZE, &ctx->eval_dctx);

  int accepted =
      blackboard_publish(ctx->bb, keys, checked_fitness, ctx->name);
  if (accepted) {
    ctx->accepted_publications++;
  }
  return accepted;
}

int agent_consult_blackboard(agent_context *ctx, double *out_keys,
                             int get_best) {
  int shared =
      blackboard_get_solution(ctx->bb, &ctx->rng, out_keys, get_best);
  ctx->consultations++;
  if (shared) {
    ctx->shared_reads++;
  }
  return shared;
}

int run_multi_agent_search(problem_instance *problem, blackboard *bb,
                           int max_global_iterations) {
static const agent agents[] = {
      {.name = "ACO",
       .role = "exploracao global e injecao de solucoes compartilhadas",
       .seed = 1111ULL,
       .rng_jumps = 1,
       .run = aco_agent_run},
      {.name = "Tabu Search",
       .role = "intensificacao local sobre solucoes promissoras",
       .seed = 2222ULL,
       .rng_jumps = 2,
       .run = tabu_agent_run},
      {.name = "RVNS",
       .role = "diversificacao por vizinhancas variaveis",
       .seed = 3333ULL,
       .rng_jumps = 3,
       .run = rvns_agent_run},
      {.name = "HHO",
       .role = "convergencia guiada pela melhor solucao compartilhada",
       .seed = 4444ULL,
       .rng_jumps = 4,
       .run = hho_agent_run},
  };
  enum { AGENT_COUNT = sizeof(agents) / sizeof(agents[0]) };

  problem_workspace ws_base;
  if (!PROBLEM_WORKSPACE_INIT(&ws_base, problem)) {
    return 0;
  }

  int user_data = PROBLEM_SIZE;
  hscopt_decode_ctx dctx = {.inst = (const hscopt_instance *)problem,
                            .user = &user_data,
                            .ws = (hscopt_workspace *)&ws_base,
                            .ws_clone = problem_ws_clone(),
                            .ws_destroy = problem_ws_destroy()};

  int stop_criterion_met = 0;
  agent_context contexts[AGENT_COUNT];
  for (size_t i = 0; i < AGENT_COUNT; i++) {
    agent_context_init(&contexts[i], &agents[i], bb, &dctx,
                       &stop_criterion_met, max_global_iterations);
  }

  puts("\nAgentes autonomos:");
  for (size_t i = 0; i < AGENT_COUNT; i++) {
    printf("  - %s: %s | seed=%llu | rng_jumps=%d\n", contexts[i].name,
           contexts[i].role, (unsigned long long)agents[i].seed,
           agents[i].rng_jumps);
  }

#pragma omp parallel sections shared(contexts)
  {
#pragma omp section
    agents[0].run(&contexts[0]);

#pragma omp section
    agents[1].run(&contexts[1]);

#pragma omp section
    agents[2].run(&contexts[2]);

#pragma omp section
    agents[3].run(&contexts[3]);
  }

  puts("\nResumo individual dos agentes:");
  for (size_t i = 0; i < AGENT_COUNT; i++) {
    printf("  %s: publicacoes=%d | consultas=%d | leituras_compartilhadas=%d\n",
           contexts[i].name, contexts[i].accepted_publications,
           contexts[i].consultations, contexts[i].shared_reads);
  }

  for (size_t i = 0; i < AGENT_COUNT; i++) {
    agent_context_clear(&contexts[i]);
  }

  PROBLEM_WORKSPACE_CLEAR(&ws_base);
  return 1;
}
