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

/*
 * uet_dpdk_transport_write - Send a UET WRITE packet.
 *
 * Encodes the transport header with opcode UET_OP_WRITE and transmits
 * @payload_length bytes from @payload in a single packet.
 *
 * Returns 0 on success, negative errno on failure.
 */
int uet_dpdk_transport_write(
    struct uet_dpdk_transport *transport,
    const void *payload,
    uint16_t payload_length,
    uint32_t sequence_number);

/*
 * uet_dpdk_transport_read - Issue a UET READ_REQUEST and collect the response.
 *
 * Sends a READ_REQUEST packet carrying @remote_addr and @read_length, then
 * polls the RX queue until a matching READ_RESPONSE arrives (or until the
 * packet queue is exhausted and -EAGAIN is returned).
 *
 * On success, up to @buffer_capacity bytes of response payload are copied
 * into @buffer and the actual byte count is written to *@received_length.
 *
 * Returns 0 on success, negative errno on failure.
 */
int uet_dpdk_transport_read(
    struct uet_dpdk_transport *transport,
    uint64_t remote_addr,
    uint32_t read_length,
    uint32_t sequence_number,
    void *buffer,
    size_t buffer_capacity,
    uint16_t *received_length);

const struct uet_transport_stats *uet_dpdk_transport_stats(
    const struct uet_dpdk_transport *transport);

#endif
