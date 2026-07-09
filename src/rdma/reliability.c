#include <string.h>

#include "uet/rdma/reliability.h"

void uet_reliability_init(uet_reliability *reliability)
{
    if (reliability != NULL) {
        memset(reliability, 0, sizeof(*reliability));
    }
}

uet_status uet_reliability_validate_rx(
    const uet_reliability *reliability,
    const uet_qp *qp,
    const uet_rdma_message *message
)
{
    (void) reliability;

    if (qp == NULL || message == NULL) {
        return UET_ERR_INVAL;
    }

    if (message->psn != qp->expected_rx_psn) {
        return UET_ERR_STATE;
    }

    return UET_OK;
}

void uet_reliability_on_rx(uet_qp *qp)
{
    if (qp != NULL) {
        qp->expected_rx_psn += 1;
    }
}

void uet_reliability_on_ack(uet_reliability *reliability, uint32_t qp_num, uint32_t psn)
{
    if (reliability != NULL && qp_num > 0 && qp_num <= UET_MAX_QPS) {
        reliability->last_acked_psn[qp_num - 1] = psn;
    }
}
