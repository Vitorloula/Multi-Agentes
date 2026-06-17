#include "core/trd/trd_decoder.h"
#include "core/trd/trd.h"
#include "Decoder.hpp"
#include "ListGraph.hpp"
#include "MatrixGraph.hpp"

#include <vector>

extern "C" {

int trd_workspace_init(trd_workspace *ws, int order) {
  ws->order = order;
  return 1;
}

void trd_workspace_clear(trd_workspace *ws) {
  (void)ws;
}

hscopt_workspace *trd_ws_clone(const hscopt_workspace *ws, void *user) {
  (void)user;
  const trd_workspace *tws = (const trd_workspace *)ws;
  trd_workspace *new_ws = new trd_workspace();
  new_ws->order = tws->order;
  return (hscopt_workspace *)new_ws;
}

void trd_ws_destroy(hscopt_workspace *ws, void *user) {
  (void)user;
  trd_workspace *tws = (trd_workspace *)ws;
  delete tws;
}

double trd_decoder(const double *keys, size_t n, hscopt_decode_ctx *ctx) {
  (void)n;
  const trd_instance *inst = (const trd_instance *)ctx->inst;
  std::vector<double> chromosome(keys, keys + inst->order);

  if (inst->is_matrix) {
    TRDDecoder<MatrixGraph> decoder(*(static_cast<MatrixGraph*>(inst->graph_m)));
    return decoder.decode(chromosome);
  } else {
    TRDDecoder<ListGraph> decoder(*(static_cast<ListGraph*>(inst->graph_l)));
    return decoder.decode(chromosome);
  }
}

}
