#ifndef MULTI_AGENTES_AGENT_H
#define MULTI_AGENTES_AGENT_H

/**
 * @file agent.h
 * @brief Interface comum para agentes autonomos de busca.
 */

#include "core/blackboard.h"
#include "core/agents.h"
#include "hscopt/hscopt.h"

#include <stdint.h>

/**
 * @brief Contexto individual recebido por cada agente.
 *
 * O contexto separa o estado local do agente dos recursos compartilhados. Cada
 * agente possui gerador pseudoaleatorio, contadores e identidade proprios, mas
 * coordena suas decisoes com os demais por meio do blackboard.
 */
typedef struct {
  /** Nome legivel do agente. */
  const char *name;
  /** Papel do agente no sistema multiagentes. */
  const char *role;
  /** Blackboard usado para cooperacao indireta. */
  blackboard *bb;
  /** Contexto de decodificacao compartilhado para avaliar solucoes. */
  hscopt_decode_ctx *dctx;
  /** Decoder do problema ativo. */
  hscopt_decoder_fn decoder;
  /** Contexto local usado para reavaliar solucoes antes da publicacao. */
  hscopt_decode_ctx eval_dctx;
  /** Flag global de parada observada por todos os agentes. */
  int *stop_criterion_met;
  /** Limite de iteracoes especifico da execucao. */
  int max_global_iterations;
  /** Parametros configuraveis das metaheuristicas. */
  const algorithm_params *params;
  /** Instante inicial da execucao cooperativa, em segundos OpenMP. */
  double run_start_time;
  /** Limite opcional de tempo da execucao cooperativa. Zero desativa. */
  double max_seconds;
  /** Gerador pseudoaleatorio exclusivo do agente. */
  hscopt_rng rng;
  /** Quantidade de solucoes aceitas no blackboard por este agente. */
  int accepted_publications;
  /** Quantidade de consultas feitas por este agente ao blackboard. */
  int consultations;
  /** Quantidade de consultas que retornaram solucoes compartilhadas. */
  int shared_reads;
  /** Melhor valor avaliado por este agente durante a execucao. */
  double best_fitness;
  /** Tempo ate a primeira ocorrencia do melhor valor deste agente. */
  double best_time_seconds;
} agent_context;

/**
 * @brief Assinatura padrao da rotina principal de um agente.
 *
 * @param ctx Contexto individual do agente.
 */
typedef void (*agent_run_fn)(agent_context *ctx);

/**
 * @brief Descritor estatico de um agente disponivel no sistema.
 */
typedef struct {
  /** Nome do agente. */
  const char *name;
  /** Papel do agente no sistema multiagentes. */
  const char *role;
  /** Semente base do gerador pseudoaleatorio local. */
  uint64_t seed;
  /** Saltos aplicados ao RNG para separar fluxos aleatorios. */
  int rng_jumps;
  /** Funcao que implementa a estrategia autonoma do agente. */
  agent_run_fn run;
} agent;

/**
 * @brief Inicializa o contexto individual de um agente.
 *
 * @param ctx Contexto a ser preenchido.
 * @param spec Descritor do agente.
 * @param bb Blackboard compartilhado.
 * @param dctx Contexto do decodificador.
 * @param stop_criterion_met Flag global de parada.
 * @param max_global_iterations Limite de iteracoes da execucao.
 */
void agent_context_init(agent_context *ctx, const agent *spec, blackboard *bb,
                        hscopt_decode_ctx *dctx, int *stop_criterion_met,
                        int max_global_iterations,
                        const algorithm_params *params);

/**
 * @brief Consulta a flag global de parada de forma atomica.
 *
 * @param ctx Contexto do agente.
 * @return Valor atual da flag de parada.
 */
int agent_should_stop(const agent_context *ctx);

/**
 * @brief Solicita parada global para todos os agentes.
 *
 * @param ctx Contexto do agente solicitante.
 */
void agent_request_stop(agent_context *ctx);

/**
 * @brief Publica uma solucao candidata em nome do agente.
 *
 * @param ctx Contexto do agente.
 * @param keys Random keys da solucao candidata.
 * @param fitness Custo da solucao candidata.
 * @return 1 quando a publicacao foi aceita, 0 caso contrario.
 */
int agent_publish(agent_context *ctx, const double *keys, double fitness);

/**
 * @brief Libera recursos locais do contexto do agente.
 *
 * @param ctx Contexto a ser finalizado.
 */
void agent_context_clear(agent_context *ctx);

/**
 * @brief Consulta uma solucao do blackboard.
 *
 * @param ctx Contexto do agente.
 * @param out_keys Vetor preenchido com a solucao consultada.
 * @param get_best Valor diferente de zero seleciona a melhor solucao.
 * @return 1 quando a solucao veio da pool compartilhada, 0 quando foi gerada.
 */
int agent_consult_blackboard(agent_context *ctx, double *out_keys,
                             int get_best);

void aco_agent_run(agent_context *ctx);
void tabu_agent_run(agent_context *ctx);
void rvns_agent_run(agent_context *ctx);
void hho_agent_run(agent_context *ctx);

#endif
