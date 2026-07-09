#include <string.h>

#include "uet/host_if/admin.h"

uet_status uet_device_bootstrap(uet_device *device)
{
    uet_device_caps caps = {
        .max_qps = UET_MAX_QPS,
        .max_cqs = UET_MAX_CQS,
        .max_mrs = UET_MAX_MRS,
        .max_pds = UET_MAX_PDS,
        .max_sq_depth = UET_MAX_SQ_DEPTH,
        .max_cq_depth = UET_MAX_CQ_DEPTH,
        .lcore_count = 1
    };
    uet_status status;

    if (device == NULL) {
        return UET_ERR_INVAL;
    }

    memset(device, 0, sizeof(*device));

    status = uet_hal_init(&device->hal, &caps);
    if (status != UET_OK) {
        return status;
    }

    status = uet_dpdk_port_init(&device->port, 0, 0, 32, 32);
    if (status != UET_OK) {
        return status;
    }

    status = uet_rdma_context_init(&device->rdma, &caps);
    if (status != UET_OK) {
        return status;
    }

    uet_reliability_init(&device->reliability);
    return UET_OK;
}

uet_status uet_device_query_caps(const uet_device *device, uet_device_caps *caps)
{
    if (device == NULL || caps == NULL) {
        return UET_ERR_INVAL;
    }

    *caps = device->hal.caps;
    return UET_OK;
}

uet_status uet_admin_alloc_pd(uet_device *device, uint32_t *pd_num)
{
    return uet_rdma_alloc_pd(&device->rdma, pd_num);
}

uet_status uet_admin_create_cq(uet_device *device, uint32_t depth, uint32_t *cq_num)
{
    return uet_rdma_create_cq(&device->rdma, depth, cq_num);
}

uet_status uet_admin_register_mr(
    uet_device *device,
    uint32_t pd_num,
    uint64_t base_addr,
    uint32_t length,
    uint32_t access,
    uint32_t *mr_num,
    uint32_t *rkey
)
{
    uint32_t map_id;
    uet_status status = uet_hal_dma_map(&device->hal, base_addr, length, &map_id);
    (void) map_id;

    if (status != UET_OK) {
        return status;
    }

    return uet_rdma_register_mr(&device->rdma, pd_num, base_addr, length, access, mr_num, rkey);
}

uet_status uet_admin_create_qp(
    uet_device *device,
    uint32_t pd_num,
    uint32_t send_cq_num,
    uint32_t recv_cq_num,
    uint16_t shard,
    uint32_t *qp_num
)
{
    uint32_t queue_id;
    uet_status status = uet_hal_queue_create(&device->hal, device->hal.caps.max_sq_depth, &queue_id);
    (void) queue_id;

    if (status != UET_OK) {
        return status;
    }

    status = uet_rdma_create_qp(&device->rdma, pd_num, send_cq_num, recv_cq_num, shard, qp_num);
    if (status != UET_OK) {
        return status;
    }

    return uet_rdma_modify_qp_state(&device->rdma, *qp_num, UET_QP_RTS);
}

uet_status uet_admin_post_send(uet_device *device, uint32_t qp_num, const uet_send_wqe *wqe)
{
    return uet_rdma_post_send(&device->rdma, qp_num, wqe);
}

uet_status uet_admin_tx_step(
    uet_device *device,
    uint32_t qp_num,
    uet_packet *packets,
    size_t capacity,
    size_t *count
)
{
    uet_status status = uet_engine_tx_burst(
        &device->rdma,
        &device->reliability,
        &device->hal,
        qp_num,
        packets,
        capacity,
        count
    );

    if (status == UET_OK) {
        uet_dpdk_port_record_tx(&device->port, *count);
        device->engine_stats.tx_packets += *count;
    }

    return status;
}

uet_status uet_admin_rx_step(
    uet_device *device,
    const uet_packet *packet,
    uet_packet *response,
    bool *has_response
)
{
    uet_status status;

    if (device == NULL || packet == NULL) {
        return UET_ERR_INVAL;
    }

    uet_dpdk_port_record_rx(&device->port, 1);
    status = uet_engine_rx_packet(
        &device->rdma,
        &device->reliability,
        &device->hal,
        packet,
        response,
        has_response,
        &device->engine_stats
    );
    if (status == UET_OK && has_response != NULL && *has_response) {
        uet_dpdk_port_record_tx(&device->port, 1);
    }

    return status;
}

uet_status uet_admin_poll_cq(
    uet_device *device,
    uint32_t cq_num,
    uet_cqe *cqes,
    size_t capacity,
    size_t *count
)
{
    return uet_rdma_poll_cq(&device->rdma, cq_num, cqes, capacity, count);
}

void uet_admin_query_stats(const uet_device *device, uet_telemetry_snapshot *snapshot)
{
    uet_telemetry_collect(snapshot, &device->hal, &device->port, &device->engine_stats);
}
