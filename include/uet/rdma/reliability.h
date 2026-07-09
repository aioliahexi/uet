#ifndef UET_RDMA_RELIABILITY_H
#define UET_RDMA_RELIABILITY_H

#include "uet/rdma/core.h"
#include "uet/rdma/proto.h"

typedef struct {
    uint32_t last_acked_psn[UET_MAX_QPS];
} uet_reliability;

void uet_reliability_init(uet_reliability *reliability);
uet_status uet_reliability_validate_rx(
    const uet_reliability *reliability,
    const uet_qp *qp,
    const uet_rdma_message *message
);
void uet_reliability_on_rx(uet_qp *qp);
void uet_reliability_on_ack(uet_reliability *reliability, uint32_t qp_num, uint32_t psn);

#endif
