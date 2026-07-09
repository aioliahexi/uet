#ifndef UET_RDMA_PROTO_H
#define UET_RDMA_PROTO_H

#include "uet/types.h"

typedef struct {
    uet_opcode opcode;
    uint32_t qp_num;
    uint32_t psn;
    uint32_t length;
    uint32_t rkey;
    uint64_t vaddr;
} uet_rdma_message;

uet_status uet_proto_decode(const uet_packet *packet, uet_rdma_message *message);
uet_status uet_proto_encode_ack(const uet_rdma_message *message, uet_packet *packet);
uet_status uet_proto_encode_wqe(
    uint32_t qp_num,
    uint32_t psn,
    const uet_send_wqe *wqe,
    uet_packet *packet
);

#endif
