#include <string.h>

#include "uet/rdma/engine.h"

static uet_status uet_engine_complete(uet_rdma_context *ctx, uet_hal *hal, uint32_t cq_num, const uet_cqe *cqe)
{
    uet_status status = uet_rdma_complete(ctx, cq_num, cqe);
    if (status != UET_OK) {
        return status;
    }

    return uet_hal_cqe_post(hal, cq_num, cqe);
}

uet_status uet_engine_tx_burst(
    uet_rdma_context *ctx,
    uet_reliability *reliability,
    uet_hal *hal,
    uint32_t qp_num,
    uet_packet *packets,
    size_t capacity,
    size_t *count
)
{
    uet_qp *qp;
    size_t produced = 0;

    (void) reliability;

    if (ctx == NULL || hal == NULL || packets == NULL || count == NULL || capacity == 0) {
        return UET_ERR_INVAL;
    }

    qp = uet_rdma_find_qp(ctx, qp_num);
    if (qp == NULL) {
        return UET_ERR_NOT_FOUND;
    }

    while (produced < capacity && qp->sq_head != qp->sq_tail) {
        uet_send_wqe *wqe = &qp->sq[qp->sq_head];
        uet_status status = uet_proto_encode_wqe(qp->qp_num, qp->next_tx_psn, wqe, &packets[produced]);
        if (status != UET_OK) {
            return status;
        }

        qp->sq_head = (qp->sq_head + 1) % UET_MAX_SQ_DEPTH;
        qp->next_tx_psn += 1;
        produced += 1;
    }

    if (produced > 0) {
        uet_status status = uet_hal_doorbell_ring(hal, qp->qp_num);
        if (status != UET_OK) {
            return status;
        }
    }

    *count = produced;
    return UET_OK;
}

uet_status uet_engine_rx_packet(
    uet_rdma_context *ctx,
    uet_reliability *reliability,
    uet_hal *hal,
    const uet_packet *packet,
    uet_packet *response,
    bool *has_response,
    uet_engine_stats *stats
)
{
    uet_rdma_message message;
    uet_qp *qp;
    uet_status status;

    if (ctx == NULL || reliability == NULL || hal == NULL || packet == NULL || response == NULL || has_response == NULL || stats == NULL) {
        return UET_ERR_INVAL;
    }

    *has_response = false;
    stats->rx_packets += 1;

    status = uet_proto_decode(packet, &message);
    if (status != UET_OK) {
        stats->drops += 1;
        return status;
    }

    qp = uet_rdma_find_qp(ctx, message.qp_num);
    if (qp == NULL || qp->state != UET_QP_RTS) {
        stats->drops += 1;
        return UET_ERR_STATE;
    }

    if (message.opcode == UET_OPCODE_ACK) {
        uet_reliability_on_ack(reliability, qp->qp_num, message.psn);
        return UET_OK;
    }

    status = uet_reliability_validate_rx(reliability, qp, &message);
    if (status != UET_OK) {
        stats->drops += 1;
        return status;
    }

    if (message.opcode == UET_OPCODE_WRITE) {
        uet_mr *mr = uet_rdma_find_mr_by_rkey(ctx, message.rkey);
        if (mr == NULL || (mr->access & UET_ACCESS_REMOTE_WRITE) == 0u) {
            stats->drops += 1;
            return UET_ERR_ACCESS;
        }
    }

    if (message.opcode == UET_OPCODE_READ || message.opcode == UET_OPCODE_ATOMIC) {
        stats->drops += 1;
        return UET_ERR_STATE;
    }

    {
        uet_cqe cqe;
        memset(&cqe, 0, sizeof(cqe));
        cqe.qp_num = qp->qp_num;
        cqe.opcode = message.opcode;
        cqe.byte_len = message.length;
        cqe.status = UET_OK;
        status = uet_engine_complete(ctx, hal, qp->recv_cq_num, &cqe);
        if (status != UET_OK) {
            stats->drops += 1;
            return status;
        }
    }

    uet_reliability_on_rx(qp);
    stats->completions += 1;

    status = uet_proto_encode_ack(&message, response);
    if (status != UET_OK) {
        stats->drops += 1;
        return status;
    }

    *has_response = true;
    stats->acks += 1;
    stats->tx_packets += 1;
    return UET_OK;
}
