#ifndef UET_MGMT_TELEMETRY_H
#define UET_MGMT_TELEMETRY_H

#include "uet/hw/hal.h"
#include "uet/net/dpdk_port.h"
#include "uet/rdma/engine.h"

typedef struct {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t completions;
    uint64_t acks;
    uint64_t drops;
    uint64_t doorbells;
    uint64_t dma_maps;
    uint64_t event_polls;
    uint64_t cqe_posts;
} uet_telemetry_snapshot;

void uet_telemetry_collect(
    uet_telemetry_snapshot *snapshot,
    const uet_hal *hal,
    const uet_dpdk_port *port,
    const uet_engine_stats *stats
);

#endif
