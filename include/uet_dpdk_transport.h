#ifndef UET_DPDK_TRANSPORT_H
#define UET_DPDK_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

#include "uet_transport.h"

struct uet_dpdk_transport;

struct uet_dpdk_transport *uet_dpdk_transport_create(
    const struct uet_transport_config *config,
    int argc,
    char **argv);
void uet_dpdk_transport_destroy(struct uet_dpdk_transport *transport);
int uet_dpdk_transport_send(
    struct uet_dpdk_transport *transport,
    const void *payload,
    uint16_t payload_length,
    uint32_t sequence_number,
    uint8_t flags);
int uet_dpdk_transport_receive(
    struct uet_dpdk_transport *transport,
    struct uet_transport_header *header,
    void *payload,
    size_t payload_capacity,
    uint16_t *payload_length);
const struct uet_transport_stats *uet_dpdk_transport_stats(
    const struct uet_dpdk_transport *transport);

#endif
