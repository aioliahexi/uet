#include <string.h>

#include "uet/rdma/core.h"

static bool uet_valid_pd(uint32_t pd_num)
{
    return pd_num > 0 && pd_num <= UET_MAX_PDS;
}

static bool uet_valid_cq(uint32_t cq_num)
{
    return cq_num > 0 && cq_num <= UET_MAX_CQS;
}

uet_status uet_rdma_context_init(uet_rdma_context *ctx, const uet_device_caps *caps)
{
    if (ctx == NULL || caps == NULL) {
        return UET_ERR_INVAL;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->caps = *caps;
    return UET_OK;
}

uet_status uet_rdma_alloc_pd(uet_rdma_context *ctx, uint32_t *pd_num)
{
    size_t i;

    if (ctx == NULL || pd_num == NULL) {
        return UET_ERR_INVAL;
    }

    for (i = 0; i < UET_MAX_PDS; ++i) {
        if (!ctx->pds[i].in_use) {
            ctx->pds[i].in_use = true;
            ctx->pds[i].pd_num = (uint32_t) i + 1;
            ctx->pds[i].refcnt = 1;
            *pd_num = ctx->pds[i].pd_num;
            return UET_OK;
        }
    }

    return UET_ERR_FULL;
}

uet_status uet_rdma_create_cq(uet_rdma_context *ctx, uint32_t depth, uint32_t *cq_num)
{
    size_t i;

    if (ctx == NULL || cq_num == NULL || depth == 0 || depth > UET_MAX_CQ_DEPTH) {
        return UET_ERR_INVAL;
    }

    for (i = 0; i < UET_MAX_CQS; ++i) {
        if (!ctx->cqs[i].in_use) {
            ctx->cqs[i].in_use = true;
            ctx->cqs[i].cq_num = (uint32_t) i + 1;
            ctx->cqs[i].depth = depth;
            *cq_num = ctx->cqs[i].cq_num;
            return UET_OK;
        }
    }

    return UET_ERR_FULL;
}

uet_status uet_rdma_register_mr(
    uet_rdma_context *ctx,
    uint32_t pd_num,
    uint64_t base_addr,
    uint32_t length,
    uint32_t access,
    uint32_t *mr_num,
    uint32_t *rkey
)
{
    size_t i;

    if (ctx == NULL || mr_num == NULL || rkey == NULL || !uet_valid_pd(pd_num) || base_addr == 0 || length == 0) {
        return UET_ERR_INVAL;
    }

    if (!ctx->pds[pd_num - 1].in_use) {
        return UET_ERR_NOT_FOUND;
    }

    for (i = 0; i < UET_MAX_MRS; ++i) {
        if (!ctx->mrs[i].in_use) {
            ctx->mrs[i].in_use = true;
            ctx->mrs[i].mr_num = (uint32_t) i + 1;
            ctx->mrs[i].pd_num = pd_num;
            ctx->mrs[i].lkey = 0x1000u + (uint32_t) i;
            ctx->mrs[i].rkey = 0x2000u + (uint32_t) i;
            ctx->mrs[i].base_addr = base_addr;
            ctx->mrs[i].length = length;
            ctx->mrs[i].access = access;
            *mr_num = ctx->mrs[i].mr_num;
            *rkey = ctx->mrs[i].rkey;
            return UET_OK;
        }
    }

    return UET_ERR_FULL;
}

uet_status uet_rdma_create_qp(
    uet_rdma_context *ctx,
    uint32_t pd_num,
    uint32_t send_cq_num,
    uint32_t recv_cq_num,
    uint16_t shard,
    uint32_t *qp_num
)
{
    size_t i;

    if (ctx == NULL || qp_num == NULL || !uet_valid_pd(pd_num) || !uet_valid_cq(send_cq_num) || !uet_valid_cq(recv_cq_num)) {
        return UET_ERR_INVAL;
    }

    if (!ctx->pds[pd_num - 1].in_use || !ctx->cqs[send_cq_num - 1].in_use || !ctx->cqs[recv_cq_num - 1].in_use) {
        return UET_ERR_NOT_FOUND;
    }

    for (i = 0; i < UET_MAX_QPS; ++i) {
        if (!ctx->qps[i].in_use) {
            ctx->qps[i].in_use = true;
            ctx->qps[i].qp_num = (uint32_t) i + 1;
            ctx->qps[i].pd_num = pd_num;
            ctx->qps[i].send_cq_num = send_cq_num;
            ctx->qps[i].recv_cq_num = recv_cq_num;
            ctx->qps[i].state = UET_QP_RESET;
            ctx->qps[i].shard = shard;
            ctx->qps[i].next_tx_psn = 1;
            ctx->qps[i].expected_rx_psn = 1;
            *qp_num = ctx->qps[i].qp_num;
            return UET_OK;
        }
    }

    return UET_ERR_FULL;
}

uet_status uet_rdma_modify_qp_state(uet_rdma_context *ctx, uint32_t qp_num, uet_qp_state state)
{
    uet_qp *qp = uet_rdma_find_qp(ctx, qp_num);
    if (qp == NULL) {
        return UET_ERR_NOT_FOUND;
    }

    qp->state = state;
    return UET_OK;
}

uet_status uet_rdma_post_send(uet_rdma_context *ctx, uint32_t qp_num, const uet_send_wqe *wqe)
{
    uet_qp *qp = uet_rdma_find_qp(ctx, qp_num);
    size_t next_tail;

    if (qp == NULL || wqe == NULL) {
        return UET_ERR_INVAL;
    }

    if (qp->state != UET_QP_RTS) {
        return UET_ERR_STATE;
    }

    next_tail = (qp->sq_tail + 1) % UET_MAX_SQ_DEPTH;
    if (next_tail == qp->sq_head) {
        return UET_ERR_FULL;
    }

    qp->sq[qp->sq_tail] = *wqe;
    qp->sq_tail = next_tail;
    return UET_OK;
}

uet_status uet_rdma_complete(uet_rdma_context *ctx, uint32_t cq_num, const uet_cqe *cqe)
{
    uet_cq *cq;
    size_t next_tail;

    if (ctx == NULL || cqe == NULL || !uet_valid_cq(cq_num)) {
        return UET_ERR_INVAL;
    }

    cq = &ctx->cqs[cq_num - 1];
    if (!cq->in_use) {
        return UET_ERR_NOT_FOUND;
    }

    next_tail = (cq->tail + 1) % cq->depth;
    if (next_tail == cq->head) {
        return UET_ERR_FULL;
    }

    cq->entries[cq->tail] = *cqe;
    cq->tail = next_tail;
    return UET_OK;
}

uet_status uet_rdma_poll_cq(
    uet_rdma_context *ctx,
    uint32_t cq_num,
    uet_cqe *cqes,
    size_t capacity,
    size_t *count
)
{
    uet_cq *cq;
    size_t polled = 0;

    if (ctx == NULL || cqes == NULL || count == NULL || capacity == 0 || !uet_valid_cq(cq_num)) {
        return UET_ERR_INVAL;
    }

    cq = &ctx->cqs[cq_num - 1];
    if (!cq->in_use) {
        return UET_ERR_NOT_FOUND;
    }

    while (polled < capacity && cq->head != cq->tail) {
        cqes[polled] = cq->entries[cq->head];
        cq->head = (cq->head + 1) % cq->depth;
        polled += 1;
    }

    *count = polled;
    return UET_OK;
}

uet_qp *uet_rdma_find_qp(uet_rdma_context *ctx, uint32_t qp_num)
{
    if (ctx == NULL || qp_num == 0 || qp_num > UET_MAX_QPS) {
        return NULL;
    }

    return ctx->qps[qp_num - 1].in_use ? &ctx->qps[qp_num - 1] : NULL;
}

uet_mr *uet_rdma_find_mr_by_rkey(uet_rdma_context *ctx, uint32_t rkey)
{
    size_t i;

    if (ctx == NULL) {
        return NULL;
    }

    for (i = 0; i < UET_MAX_MRS; ++i) {
        if (ctx->mrs[i].in_use && ctx->mrs[i].rkey == rkey) {
            return &ctx->mrs[i];
        }
    }

    return NULL;
}
