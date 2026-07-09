#ifndef UET_HOST_IF_ADMIN_H
#define UET_HOST_IF_ADMIN_H

#include "uet/hw/hal.h"
#include "uet/mgmt/telemetry.h"
#include "uet/net/dpdk_port.h"
#include "uet/rdma/core.h"
#include "uet/rdma/engine.h"
#include "uet/rdma/reliability.h"

typedef struct {
    uet_hal hal;
    uet_dpdk_port port;
    uet_rdma_context rdma;
    uet_reliability reliability;
    uet_engine_stats engine_stats;
} uet_device;

uet_status uet_device_bootstrap(uet_device *device);
uet_status uet_device_query_caps(const uet_device *device, uet_device_caps *caps);
uet_status uet_admin_alloc_pd(uet_device *device, uint32_t *pd_num);
uet_status uet_admin_create_cq(uet_device *device, uint32_t depth, uint32_t *cq_num);
uet_status uet_admin_register_mr(
    uet_device *device,
    uint32_t pd_num,
    uint64_t base_addr,
    uint32_t length,
    uint32_t access,
    uint32_t *mr_num,
    uint32_t *rkey
);
uet_status uet_admin_create_qp(
    uet_device *device,
    uint32_t pd_num,
    uint32_t send_cq_num,
    uint32_t recv_cq_num,
    uint16_t shard,
    uint32_t *qp_num
);
uet_status uet_admin_post_send(uet_device *device, uint32_t qp_num, const uet_send_wqe *wqe);
uet_status uet_admin_tx_step(
    uet_device *device,
    uint32_t qp_num,
    uet_packet *packets,
    size_t capacity,
    size_t *count
);
uet_status uet_admin_rx_step(
    uet_device *device,
    const uet_packet *packet,
    uet_packet *response,
    bool *has_response
);
uet_status uet_admin_poll_cq(
    uet_device *device,
    uint32_t cq_num,
    uet_cqe *cqes,
    size_t capacity,
    size_t *count
);
void uet_admin_query_stats(const uet_device *device, uet_telemetry_snapshot *snapshot);

#endif
