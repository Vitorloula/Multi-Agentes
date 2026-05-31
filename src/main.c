#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <omp.h>
#include "hscopt/hscopt.h"

#define N_CITIES 50    
#define POOL_SIZE 5  

typedef struct {
    double x;
    double y;
} City;

typedef struct {
    int n_cities;
    City cities[N_CITIES];
    double dist_matrix[N_CITIES][N_CITIES];
} TSPInstance;

typedef struct {
    double key;
    int index;
} KeyPair;

typedef struct hscopt_workspace {
    KeyPair *pairs;
} TSPWorkspace;

int compare_keypairs(const void *a, const void *b) {
    double ka = ((KeyPair*)a)->key;
    double kb = ((KeyPair*)b)->key;
    return (ka > kb) - (ka < kb);
}

hscopt_workspace *tsp_ws_clone(const hscopt_workspace *ws, void *user) {
    TSPWorkspace *new_ws = malloc(sizeof(TSPWorkspace));
    new_ws->pairs = malloc(N_CITIES * sizeof(KeyPair));
    return (hscopt_workspace*)new_ws;
}

void tsp_ws_destroy(hscopt_workspace *ws, void *user) {
    TSPWorkspace *tws = (TSPWorkspace*)ws;
    free(tws->pairs);
    free(tws);
}

double tsp_decoder(const double *keys, size_t n, hscopt_decode_ctx *ctx) {
    const TSPInstance *tsp = (const TSPInstance *)ctx->inst;
    TSPWorkspace *tws = (TSPWorkspace *)ctx->ws;
    
    for (int i = 0; i < N_CITIES; i++) {
        tws->pairs[i].key = keys[i];
        tws->pairs[i].index = i;
    }
    
    qsort(tws->pairs, N_CITIES, sizeof(KeyPair), compare_keypairs);
    
    double distance = 0.0;
    for (int i = 0; i < N_CITIES - 1; i++) {
        int u = tws->pairs[i].index;
        int v = tws->pairs[i + 1].index;
        distance += tsp->dist_matrix[u][v];
    }
    int last = tws->pairs[N_CITIES - 1].index;
    int first = tws->pairs[0].index;
    distance += tsp->dist_matrix[last][first];
    
    return distance;
}

typedef struct {
    double keys[N_CITIES];
    double fitness;
} Solution;

typedef struct {
    Solution pool[POOL_SIZE];
    int count;
    omp_lock_t lock;
} Blackboard;

void blackboard_init(Blackboard *bb) {
    bb->count = 0;
    omp_init_lock(&bb->lock);
}

void blackboard_destroy(Blackboard *bb) {
    omp_destroy_lock(&bb->lock);
}

int is_diverse(const Blackboard *bb, const double *keys, double fitness) {
    for (int i = 0; i < bb->count; i++) {
        if (fabs(bb->pool[i].fitness - fitness) < 1e-3) {
            return 0; 
        }
    }
    return 1; 
}

int blackboard_publish(Blackboard *bb, const double *keys, double fitness, const char *agent_name) {
    omp_set_lock(&bb->lock);
    
    if (!is_diverse(bb, keys, fitness)) {
        omp_unset_lock(&bb->lock);
        return 0;
    }
    
    int updated = 0;
    if (bb->count < POOL_SIZE) {
        memcpy(bb->pool[bb->count].keys, keys, N_CITIES * sizeof(double));
        bb->pool[bb->count].fitness = fitness;
        bb->count++;
        
        for (int i = bb->count - 1; i > 0; i--) {
            if (bb->pool[i].fitness < bb->pool[i-1].fitness) {
                Solution tmp = bb->pool[i];
                bb->pool[i] = bb->pool[i-1];
                bb->pool[i-1] = tmp;
            }
        }
        updated = 1;
    }
    else if (fitness < bb->pool[POOL_SIZE - 1].fitness) {
        memcpy(bb->pool[POOL_SIZE - 1].keys, keys, N_CITIES * sizeof(double));
        bb->pool[POOL_SIZE - 1].fitness = fitness;
        
        for (int i = POOL_SIZE - 1; i > 0; i--) {
            if (bb->pool[i].fitness < bb->pool[i-1].fitness) {
                Solution tmp = bb->pool[i];
                bb->pool[i] = bb->pool[i-1];
                bb->pool[i-1] = tmp;
            }
        }
        updated = 1;
    }
    
    if (updated) {
        printf("\033[1;36m[%s]\033[0m Nova melhor solucao publicada na pool! Custo: \033[1;32m%.2f\033[0m (Total na Pool: %d)\n", 
               agent_name, fitness, bb->count);
    }
    
    omp_unset_lock(&bb->lock);
    return updated;
}

void blackboard_get_solution(Blackboard *bb, hscopt_rng *rng, double *out_keys, int get_best) {
    omp_set_lock(&bb->lock);
    
    if (bb->count == 0) {
        omp_unset_lock(&bb->lock);
        for (int i = 0; i < N_CITIES; i++) {
            out_keys[i] = hscopt_rng_next_u01(rng);
        }
        return;
    }
    
    int idx = 0;
    if (!get_best) {
        idx = hscopt_rng_random_index(rng, bb->count);
    }
    
    memcpy(out_keys, bb->pool[idx].keys, N_CITIES * sizeof(double));
    omp_unset_lock(&bb->lock);
}

void init_tsp_instance(TSPInstance *tsp) {
    tsp->n_cities = N_CITIES;
    srand(100);
    
    printf("Inicializando Instancia TSP com %d cidades em mapa 2D...\n", N_CITIES);
    for (int i = 0; i < N_CITIES; i++) {
        tsp->cities[i].x = (double)(rand() % 1000);
        tsp->cities[i].y = (double)(rand() % 1000);
    }
    
    for (int i = 0; i < N_CITIES; i++) {
        for (int j = 0; j < N_CITIES; j++) {
            if (i == j) {
                tsp->dist_matrix[i][j] = 0.0;
            } else {
                double dx = tsp->cities[i].x - tsp->cities[j].x;
                double dy = tsp->cities[i].y - tsp->cities[j].y;
                tsp->dist_matrix[i][j] = sqrt(dx*dx + dy*dy);
            }
        }
    }
}

void print_tour(const double *keys, TSPInstance *tsp) {
    KeyPair pairs[N_CITIES];
    for (int i = 0; i < N_CITIES; i++) {
        pairs[i].key = keys[i];
        pairs[i].index = i;
    }
    qsort(pairs, N_CITIES, sizeof(KeyPair), compare_keypairs);
    
    printf("\033[1;35mSequencia da Rota:\033[0m ");
    for (int i = 0; i < N_CITIES; i++) {
        printf("%d -> ", pairs[i].index);
    }
    printf("%d\n", pairs[0].index);
}

int main() {
    TSPInstance tsp;
    init_tsp_instance(&tsp);
    
    TSPWorkspace ws_base;
    ws_base.pairs = malloc(N_CITIES * sizeof(KeyPair));
    
    int user_data = N_CITIES;
    hscopt_decode_ctx dctx = {
        .inst = (const hscopt_instance *)&tsp,
        .user = &user_data,
        .ws = (hscopt_workspace *)&ws_base,
        .ws_clone = tsp_ws_clone,
        .ws_destroy = tsp_ws_destroy
    };
    
    Blackboard bb;
    blackboard_init(&bb);
    
    int stop_criterion_met = 0;
    int max_global_iterations = 150;
    
    double start_time = omp_get_wtime();
    
    printf("\n\033[1;33m======================================================================\033[0m\n");
    printf("\033[1;36mIniciando execucao multi-agentes assincrona (OpenMP Sections)...\033[0m\n");
    printf("\033[1;33m======================================================================\033[0m\n\n");
    
    #pragma omp parallel sections shared(bb, stop_criterion_met, dctx)
    {
        #pragma omp section
        {
            hscopt_rng my_rng;
            hscopt_rng_seed(&my_rng, 1111ULL);
            hscopt_rng_jump(&my_rng);

            hscopt_aco_ctx *aco = hscopt_aco_create(
                N_CITIES, 20, 10, max_global_iterations, 1, 0.1, 1.0, 
                tsp_decoder, &dctx, &my_rng
            );
            
            for (int it = 0; it < max_global_iterations && !stop_criterion_met; it++) {
                hscopt_aco_iterate(aco, 1);
                
                double fit = hscopt_aco_best_fitness(aco);
                const double *keys = hscopt_aco_best_keys(aco);
                
                blackboard_publish(&bb, keys, fit, "Agente ACO");
                
                double guest_keys[N_CITIES];
                blackboard_get_solution(&bb, &my_rng, guest_keys, 0);
                hscopt_aco_try_update_best(aco, guest_keys);
            }
            hscopt_aco_destroy(aco);
        }

        #pragma omp section
        {
            hscopt_rng my_rng;
            hscopt_rng_seed(&my_rng, 2222ULL);
            hscopt_rng_jump(&my_rng);
            hscopt_rng_jump(&my_rng); 
            
            hscopt_ts_ctx *ts = hscopt_ts_create(
                N_CITIES, 30, 7, max_global_iterations, 1, 
                tsp_decoder, &dctx, &my_rng, NULL
            );
            
            for (int it = 0; it < max_global_iterations && !stop_criterion_met; it++) {
                double seed_keys[N_CITIES];
                blackboard_get_solution(&bb, &my_rng, seed_keys, 1);
                hscopt_ts_reset(ts, seed_keys);
                
                hscopt_ts_iterate(ts, 10); 
                
                double fit = hscopt_ts_best_fitness(ts);
                const double *keys = hscopt_ts_best_keys(ts);
                
                blackboard_publish(&bb, keys, fit, "Agente Tabu Search");
            }
            hscopt_ts_destroy(ts);
        }
        
        #pragma omp section
        {
            hscopt_rng my_rng;
            hscopt_rng_seed(&my_rng, 3333ULL);
            hscopt_rng_jump(&my_rng);
            hscopt_rng_jump(&my_rng);
            hscopt_rng_jump(&my_rng); 

            hscopt_rvns_ctx *rvns = hscopt_rvns_create(
                N_CITIES, 3, max_global_iterations, 1,
                tsp_decoder, &dctx, &my_rng, NULL
            );
            
            for (int it = 0; it < max_global_iterations && !stop_criterion_met; it++) {
                double seed_keys[N_CITIES];
                blackboard_get_solution(&bb, &my_rng, seed_keys, 0);
                hscopt_rvns_reset(rvns, seed_keys);
                
                hscopt_rvns_iterate(rvns, 8);
                
                double fit = hscopt_rvns_best_fitness(rvns);
                const double *keys = hscopt_rvns_best_keys(rvns);
                
                blackboard_publish(&bb, keys, fit, "Agente RVNS");
            }
            hscopt_rvns_destroy(rvns);
        }
        
        #pragma omp section
        {
            hscopt_rng my_rng;
            hscopt_rng_seed(&my_rng, 4444ULL);
            hscopt_rng_jump(&my_rng);
            hscopt_rng_jump(&my_rng);
            hscopt_rng_jump(&my_rng);
            hscopt_rng_jump(&my_rng); 
            
            hscopt_hho_ctx *hho = hscopt_hho_create(
                N_CITIES, 15, max_global_iterations, 1,
                tsp_decoder, &dctx, &my_rng
            );
            
            int iterations = 0;
            while (!stop_criterion_met) {
                double rabbit_keys[N_CITIES];
                blackboard_get_solution(&bb, &my_rng, rabbit_keys, 1);
                hscopt_hho_try_update_rabbit(hho, rabbit_keys);
                
                hscopt_hho_iterate(hho, 3);
                
                double fit = hscopt_hho_best_fitness(hho);
                const double *keys = hscopt_hho_best_keys(hho);
                
                blackboard_publish(&bb, keys, fit, "Agente HHO");
                
                iterations += 3;
                if (iterations >= max_global_iterations) {
                    stop_criterion_met = 1; 
                }
            }
            hscopt_hho_destroy(hho);
        }
    }
    
    double end_time = omp_get_wtime();
    
    printf("\n\033[1;33m======================================================================\033[0m\n");
    printf("\033[1;32m                    RESULTADOS DA EXECUCAO MULTI-AGENTES              \033[0m\n");
    printf("\033[1;33m======================================================================\033[0m\n");
    printf("Tempo total de execucao: \033[1;36m%.4f\033[0m segundos\n", end_time - start_time);
    
    if (bb.count > 0) {
        printf("Melhor distancia encontrada para o TSP: \033[1;32m%.2f\033[0m\n", bb.pool[0].fitness);
        print_tour(bb.pool[0].keys, &tsp);
        
        printf("\n\033[1;36mConteudo Final do Solution Pool (Top %d Melhores Diferentes):\033[0m\n", bb.count);
        printf("----------------------------------------------------------------------\n");
        for (int i = 0; i < bb.count; i++) {
            printf("  Rank \033[1;33m%d\033[0m - Custo: \033[1;32m%.2f\033[0m\n", i + 1, bb.pool[i].fitness);
        }
        printf("----------------------------------------------------------------------\n");
    } else {
        printf("\033[1;31mNenhuma solucao valida foi publicada no Blackboard.\033[0m\n");
    }
    
    blackboard_destroy(&bb);
    free(ws_base.pairs);
    
    return 0;
}
