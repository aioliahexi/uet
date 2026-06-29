#ifndef UET_TRANSPORT_H
#define UET_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UET_TRANSPORT_VERSION 1U
#define UET_TRANSPORT_HEADER_LENGTH 20U

/* UET-T OpCode values (UE-Spec §8.x Transport Layer OpCode field). */
#define UET_OP_WRITE          0x04U  /* Unacknowledged write to remote buffer  */
#define UET_OP_READ_REQUEST   0x0CU  /* Request data from remote buffer        */
#define UET_OP_READ_RESPONSE  0x10U  /* Response carrying requested data       */

/* Payload carried by a READ_REQUEST packet (UE-Spec §8.x Read Request). */
#define UET_READ_REQUEST_PAYLOAD_LENGTH 12U

struct uet_read_request_payload {
    uint64_t remote_addr;   /* remote buffer address to read from */
    uint32_t read_length;   /* number of bytes requested          */
};

struct uet_transport_config {
    uint16_t port_id;
    uint16_t rx_queue_id;
    uint16_t tx_queue_id;
    uint16_t burst_size;
    uint32_t flow_id;
    uint16_t mtu;
};

struct uet_transport_header {
    uint8_t version;
    uint8_t flags;
    uint16_t header_length;
    uint32_t flow_id;
    uint32_t sequence_number;
    uint32_t acknowledgement_number;
    uint16_t payload_length;
    uint16_t checksum;
};

struct uet_transport_stats {
    uint64_t tx_packets;
    uint64_t tx_bytes;
    uint64_t rx_packets;
    uint64_t rx_bytes;
    uint64_t dropped_packets;
};

bool uet_transport_config_is_valid(const struct uet_transport_config *config);
int uet_transport_encode_header(const struct uet_transport_header *header, uint8_t *buffer, size_t buffer_length);
int uet_transport_decode_header(struct uet_transport_header *header, const uint8_t *buffer, size_t buffer_length);

/*
 * Encode/decode the 12-byte READ_REQUEST payload (UE-Spec §8.x Read Request):
 *   bytes 0-7  : remote_addr (big-endian uint64)
 *   bytes 8-11 : read_length (big-endian uint32)
 *
 * uet_transport_encode_read_request: returns 0 or -EINVAL/-EMSGSIZE.
 * uet_transport_decode_read_request: returns 0 or -EINVAL/-EMSGSIZE.
 */
int uet_transport_encode_read_request(
    const struct uet_read_request_payload *req,
    uint8_t *buffer,
    size_t buffer_length);
int uet_transport_decode_read_request(
    struct uet_read_request_payload *req,
    const uint8_t *buffer,
    size_t buffer_length);

#endif
