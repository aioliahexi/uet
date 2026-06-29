#ifndef UET_TRANSPORT_H
#define UET_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UET_TRANSPORT_VERSION 1U
#define UET_TRANSPORT_HEADER_LENGTH 20U

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

#endif
