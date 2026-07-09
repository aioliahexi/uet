#include "uet/rdma/proto.h"

uet_status uet_proto_decode(const uet_packet *packet, uet_rdma_message *message)
{
    if (packet == NULL || message == NULL) {
        return UET_ERR_INVAL;
    }

    message->opcode = packet->opcode;
    message->qp_num = packet->qp_num;
    message->psn = packet->psn;
    message->length = packet->length;
    message->rkey = packet->rkey;
    message->vaddr = packet->vaddr;
    return UET_OK;
}

uet_status uet_proto_encode_ack(const uet_rdma_message *message, uet_packet *packet)
{
    if (message == NULL || packet == NULL) {
        return UET_ERR_INVAL;
    }

    packet->opcode = UET_OPCODE_ACK;
    packet->qp_num = message->qp_num;
    packet->psn = message->psn;
    packet->length = 0;
    packet->rkey = 0;
    packet->vaddr = 0;
    return UET_OK;
}

uet_status uet_proto_encode_wqe(
    uint32_t qp_num,
    uint32_t psn,
    const uet_send_wqe *wqe,
    uet_packet *packet
)
{
    if (wqe == NULL || packet == NULL || qp_num == 0 || psn == 0) {
        return UET_ERR_INVAL;
    }

    packet->opcode = wqe->opcode;
    packet->qp_num = qp_num;
    packet->psn = psn;
    packet->length = wqe->length;
    packet->rkey = wqe->remote_key;
    packet->vaddr = wqe->remote_addr;
    return UET_OK;
}
