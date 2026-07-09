#ifndef UET_TYPES_H
#define UET_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    UET_OK = 0,
    UET_ERR_INVAL = -1,
    UET_ERR_FULL = -2,
    UET_ERR_NOT_FOUND = -3,
    UET_ERR_STATE = -4,
    UET_ERR_ACCESS = -5
} uet_status;

typedef enum {
    UET_OPCODE_SEND = 0,
    UET_OPCODE_WRITE = 1,
    UET_OPCODE_READ = 2,
    UET_OPCODE_ATOMIC = 3,
    UET_OPCODE_ACK = 4,
    UET_OPCODE_INVALID = 255
} uet_opcode;

typedef enum {
    UET_QP_RESET = 0,
    UET_QP_INIT,
    UET_QP_RTR,
    UET_QP_RTS,
    UET_QP_ERR
} uet_qp_state;

enum {
    UET_ACCESS_LOCAL_WRITE = 1u << 0,
    UET_ACCESS_REMOTE_READ = 1u << 1,
    UET_ACCESS_REMOTE_WRITE = 1u << 2,
    UET_ACCESS_REMOTE_ATOMIC = 1u << 3
};

typedef struct {
    uint32_t wr_id;
    uet_opcode opcode;
    uint32_t length;
    uint32_t lkey;
    uint64_t local_addr;
    uint32_t remote_key;
    uint64_t remote_addr;
} uet_send_wqe;

typedef struct {
    uet_opcode opcode;
    uint32_t qp_num;
    uint32_t psn;
    uint32_t length;
    uint32_t rkey;
    uint64_t vaddr;
} uet_packet;

typedef struct {
    uint32_t wr_id;
    uint32_t qp_num;
    uet_opcode opcode;
    uint32_t byte_len;
    uet_status status;
} uet_cqe;

#endif
