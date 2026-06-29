#include "uet_dpdk_transport.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_version.h>

#if RTE_VERSION < RTE_VERSION_NUM(22, 11, 0, 0) || RTE_VERSION >= RTE_VERSION_NUM(22, 12, 0, 0)
#error "UET transport requires DPDK 22.11.x"
#endif

#define UET_RX_DESC 1024U
#define UET_TX_DESC 1024U
#define UET_NB_MBUFS 8192U
#define UET_MEMPOOL_CACHE 256U
#define UET_READ_MAX_POLLS 1024U

struct uet_dpdk_transport {
    struct uet_transport_config config;
    struct rte_mempool *mempool;
    struct uet_transport_stats stats;
    bool started;
};

static void uet_dpdk_transport_release_port(struct uet_dpdk_transport *transport)
{
    if (transport != NULL && transport->started) {
        rte_eth_dev_stop(transport->config.port_id);
        rte_eth_dev_close(transport->config.port_id);
        transport->started = false;
    }
}

struct uet_dpdk_transport *uet_dpdk_transport_create(
    const struct uet_transport_config *config,
    int argc,
    char **argv)
{
    struct uet_dpdk_transport *transport;
    struct rte_eth_conf port_conf;
    char mempool_name[64];
    int eal_result;
    int result;

    if (!uet_transport_config_is_valid(config)) {
        errno = EINVAL;
        return NULL;
    }

    transport = calloc(1, sizeof(*transport));
    if (transport == NULL) {
        return NULL;
    }

    transport->config = *config;

    eal_result = rte_eal_init(argc, argv);
    if (eal_result < 0) {
        free(transport);
        errno = EIO;
        return NULL;
    }

    if (transport->config.port_id >= rte_eth_dev_count_avail()) {
        rte_eal_cleanup();
        free(transport);
        errno = ENODEV;
        return NULL;
    }

    snprintf(mempool_name, sizeof(mempool_name), "uet_pool_%u", transport->config.port_id);
    transport->mempool = rte_pktmbuf_pool_create(
        mempool_name,
        UET_NB_MBUFS,
        UET_MEMPOOL_CACHE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id());
    if (transport->mempool == NULL) {
        errno = rte_errno != 0 ? rte_errno : ENOMEM;
        rte_eal_cleanup();
        free(transport);
        return NULL;
    }

    memset(&port_conf, 0, sizeof(port_conf));

    result = rte_eth_dev_configure(transport->config.port_id, 1, 1, &port_conf);
    if (result < 0) {
        errno = -result;
        goto fail;
    }

    result = rte_eth_rx_queue_setup(
        transport->config.port_id,
        transport->config.rx_queue_id,
        UET_RX_DESC,
        rte_eth_dev_socket_id(transport->config.port_id),
        NULL,
        transport->mempool);
    if (result < 0) {
        errno = -result;
        goto fail;
    }

    result = rte_eth_tx_queue_setup(
        transport->config.port_id,
        transport->config.tx_queue_id,
        UET_TX_DESC,
        rte_eth_dev_socket_id(transport->config.port_id),
        NULL);
    if (result < 0) {
        errno = -result;
        goto fail;
    }

    result = rte_eth_dev_start(transport->config.port_id);
    if (result < 0) {
        errno = -result;
        goto fail;
    }

    transport->started = true;

    return transport;

fail:
    rte_mempool_free(transport->mempool);
    rte_eal_cleanup();
    free(transport);
    return NULL;
}

void uet_dpdk_transport_destroy(struct uet_dpdk_transport *transport)
{
    if (transport == NULL) {
        return;
    }

    uet_dpdk_transport_release_port(transport);
    if (transport->mempool != NULL) {
        rte_mempool_free(transport->mempool);
    }

    rte_eal_cleanup();
    free(transport);
}

int uet_dpdk_transport_send(
    struct uet_dpdk_transport *transport,
    const void *payload,
    uint16_t payload_length,
    uint32_t sequence_number,
    uint8_t flags)
{
    struct rte_mbuf *mbuf;
    struct uet_transport_header header;
    uint8_t *packet_data;

    if (transport == NULL || payload == NULL) {
        return -EINVAL;
    }

    if ((uint32_t)payload_length + UET_TRANSPORT_HEADER_LENGTH > transport->config.mtu) {
        return -EMSGSIZE;
    }

    mbuf = rte_pktmbuf_alloc(transport->mempool);
    if (mbuf == NULL) {
        transport->stats.dropped_packets++;
        return -ENOMEM;
    }

    packet_data = rte_pktmbuf_append(mbuf, (uint16_t)(UET_TRANSPORT_HEADER_LENGTH + payload_length));
    if (packet_data == NULL) {
        rte_pktmbuf_free(mbuf);
        transport->stats.dropped_packets++;
        return -ENOSPC;
    }

    header.version = UET_TRANSPORT_VERSION;
    header.flags = flags;
    header.header_length = UET_TRANSPORT_HEADER_LENGTH;
    header.flow_id = transport->config.flow_id;
    header.sequence_number = sequence_number;
    header.acknowledgement_number = 0;
    header.payload_length = payload_length;
    header.checksum = 0;

    if (uet_transport_encode_header(&header, packet_data, UET_TRANSPORT_HEADER_LENGTH) != 0) {
        rte_pktmbuf_free(mbuf);
        transport->stats.dropped_packets++;
        return -EINVAL;
    }

    memcpy(packet_data + UET_TRANSPORT_HEADER_LENGTH, payload, payload_length);

    if (rte_eth_tx_burst(transport->config.port_id, transport->config.tx_queue_id, &mbuf, 1) != 1U) {
        rte_pktmbuf_free(mbuf);
        transport->stats.dropped_packets++;
        return -EIO;
    }

    transport->stats.tx_packets++;
    transport->stats.tx_bytes += payload_length;
    return 0;
}

int uet_dpdk_transport_receive(
    struct uet_dpdk_transport *transport,
    struct uet_transport_header *header,
    void *payload,
    size_t payload_capacity,
    uint16_t *payload_length)
{
    struct rte_mbuf *mbuf;
    const uint8_t *packet_data;
    uint32_t packet_length;
    int decode_result;

    if (transport == NULL || header == NULL || payload == NULL || payload_length == NULL) {
        return -EINVAL;
    }

    if (rte_eth_rx_burst(transport->config.port_id, transport->config.rx_queue_id, &mbuf, 1) == 0U) {
        return -EAGAIN;
    }

    if (rte_pktmbuf_is_contiguous(mbuf) == 0 && rte_pktmbuf_linearize(mbuf) < 0) {
        rte_pktmbuf_free(mbuf);
        transport->stats.dropped_packets++;
        return -ENOMEM;
    }

    packet_length = rte_pktmbuf_pkt_len(mbuf);
    if (packet_length < UET_TRANSPORT_HEADER_LENGTH) {
        rte_pktmbuf_free(mbuf);
        transport->stats.dropped_packets++;
        return -EPROTO;
    }

    packet_data = rte_pktmbuf_mtod(mbuf, const uint8_t *);
    decode_result = uet_transport_decode_header(header, packet_data, packet_length);
    if (decode_result != 0) {
        rte_pktmbuf_free(mbuf);
        transport->stats.dropped_packets++;
        return decode_result;
    }

    if ((size_t)header->payload_length > payload_capacity ||
        (uint32_t)header->header_length + (uint32_t)header->payload_length > packet_length) {
        rte_pktmbuf_free(mbuf);
        transport->stats.dropped_packets++;
        return -EMSGSIZE;
    }

    memcpy(payload, packet_data + header->header_length, header->payload_length);
    *payload_length = header->payload_length;

    transport->stats.rx_packets++;
    transport->stats.rx_bytes += header->payload_length;

    rte_pktmbuf_free(mbuf);
    return 0;
}

int uet_dpdk_transport_write(
    struct uet_dpdk_transport *transport,
    const void *payload,
    uint16_t payload_length,
    uint32_t sequence_number)
{
    return uet_dpdk_transport_send(
        transport,
        payload,
        payload_length,
        sequence_number,
        UET_OP_WRITE);
}

int uet_dpdk_transport_read(
    struct uet_dpdk_transport *transport,
    uint64_t remote_addr,
    uint32_t read_length,
    uint32_t sequence_number,
    void *buffer,
    size_t buffer_capacity,
    uint16_t *received_length)
{
    uint8_t req_payload[UET_READ_REQUEST_PAYLOAD_LENGTH];
    struct uet_transport_header resp_header;
    uint16_t resp_payload_len;
    unsigned int attempts;
    int result;

    if (transport == NULL || buffer == NULL || received_length == NULL) {
        return -EINVAL;
    }

    /* Serialize READ_REQUEST payload (UE-Spec §8.x Read Request). */
    {
        struct uet_read_request_payload rr = {
            .remote_addr = remote_addr,
            .read_length = read_length,
        };
        if (uet_transport_encode_read_request(&rr, req_payload, sizeof(req_payload)) != 0) {
            return -EINVAL;
        }
    }

    result = uet_dpdk_transport_send(
        transport,
        req_payload,
        UET_READ_REQUEST_PAYLOAD_LENGTH,
        sequence_number,
        UET_OP_READ_REQUEST);
    if (result != 0) {
        return result;
    }

    /* Poll for a READ_RESPONSE whose sequence number matches the request.
     * Non-matching packets are dropped; give up after UET_READ_MAX_POLLS
     * attempts to avoid spinning forever if the response never arrives. */
    for (attempts = 0; attempts < UET_READ_MAX_POLLS; attempts++) {
        result = uet_dpdk_transport_receive(
            transport,
            &resp_header,
            buffer,
            buffer_capacity,
            &resp_payload_len);

        if (result == -EAGAIN) {
            return -EAGAIN;
        }

        if (result != 0) {
            return result;
        }

        if (resp_header.flags == UET_OP_READ_RESPONSE &&
            resp_header.sequence_number == sequence_number) {
            *received_length = resp_payload_len;
            return 0;
        }
    }

    return -ETIMEDOUT;
}

const struct uet_transport_stats *uet_dpdk_transport_stats(
