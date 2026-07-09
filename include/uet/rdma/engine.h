#ifndef UET_RDMA_ENGINE_H
#define UET_RDMA_ENGINE_H

#include "uet/hw/hal.h"
#include "uet/rdma/core.h"
#include "uet/rdma/proto.h"
#include "uet/rdma/reliability.h"

typedef struct {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t completions;
    uint64_t acks;
    uint64_t drops;
} uet_engine_stats;

uet_status uet_engine_tx_burst(
    uet_rdma_context *ctx,
    uet_reliability *reliability,
    uet_hal *hal,
    uint32_t qp_num,
    uet_packet *packets,
    size_t capacity,
    size_t *count
);
uet_status uet_engine_rx_packet(
    uet_rdma_context *ctx,
    uet_reliability *reliability,
    uet_hal *hal,
    const uet_packet *packet,
    uet_packet *response,
    bool *has_response,
    uet_engine_stats *stats
);

#endif
