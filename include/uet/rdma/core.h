#ifndef UET_RDMA_CORE_H
#define UET_RDMA_CORE_H

#include "uet/hw/hal.h"
#include "uet/types.h"

enum {
    UET_MAX_PDS = 16,
    UET_MAX_CQS = 16,
    UET_MAX_MRS = 16,
    UET_MAX_QPS = 16,
    UET_MAX_CQ_DEPTH = 64,
    UET_MAX_SQ_DEPTH = 32
};

typedef struct {
    bool in_use;
    uint32_t pd_num;
    uint32_t refcnt;
} uet_pd;

typedef struct {
    bool in_use;
    uint32_t cq_num;
    uint32_t depth;
    size_t head;
    size_t tail;
    uet_cqe entries[UET_MAX_CQ_DEPTH];
} uet_cq;

typedef struct {
    bool in_use;
    uint32_t mr_num;
    uint32_t pd_num;
    uint32_t lkey;
    uint32_t rkey;
    uint64_t base_addr;
    uint32_t length;
    uint32_t access;
} uet_mr;

typedef struct {
    bool in_use;
    uint32_t qp_num;
    uint32_t pd_num;
    uint32_t send_cq_num;
    uint32_t recv_cq_num;
    uet_qp_state state;
    uint16_t shard;
    uint32_t next_tx_psn;
    uint32_t expected_rx_psn;
    size_t sq_head;
    size_t sq_tail;
    uet_send_wqe sq[UET_MAX_SQ_DEPTH];
} uet_qp;

typedef struct {
    uet_device_caps caps;
    uet_pd pds[UET_MAX_PDS];
    uet_cq cqs[UET_MAX_CQS];
    uet_mr mrs[UET_MAX_MRS];
    uet_qp qps[UET_MAX_QPS];
} uet_rdma_context;

uet_status uet_rdma_context_init(uet_rdma_context *ctx, const uet_device_caps *caps);
uet_status uet_rdma_alloc_pd(uet_rdma_context *ctx, uint32_t *pd_num);
uet_status uet_rdma_create_cq(uet_rdma_context *ctx, uint32_t depth, uint32_t *cq_num);
uet_status uet_rdma_register_mr(
    uet_rdma_context *ctx,
    uint32_t pd_num,
    uint64_t base_addr,
    uint32_t length,
    uint32_t access,
    uint32_t *mr_num,
    uint32_t *rkey
);
uet_status uet_rdma_create_qp(
    uet_rdma_context *ctx,
    uint32_t pd_num,
    uint32_t send_cq_num,
    uint32_t recv_cq_num,
    uint16_t shard,
    uint32_t *qp_num
);
uet_status uet_rdma_modify_qp_state(uet_rdma_context *ctx, uint32_t qp_num, uet_qp_state state);
uet_status uet_rdma_post_send(uet_rdma_context *ctx, uint32_t qp_num, const uet_send_wqe *wqe);
uet_status uet_rdma_complete(uet_rdma_context *ctx, uint32_t cq_num, const uet_cqe *cqe);
uet_status uet_rdma_poll_cq(
    uet_rdma_context *ctx,
    uint32_t cq_num,
    uet_cqe *cqes,
    size_t capacity,
    size_t *count
);
uet_qp *uet_rdma_find_qp(uet_rdma_context *ctx, uint32_t qp_num);
uet_mr *uet_rdma_find_mr_by_rkey(uet_rdma_context *ctx, uint32_t rkey);

#endif
