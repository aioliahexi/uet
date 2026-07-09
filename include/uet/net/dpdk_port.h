#ifndef UET_NET_DPDK_PORT_H
#define UET_NET_DPDK_PORT_H

#include "uet/types.h"

typedef struct {
    uint16_t port_id;
    uint16_t rx_burst;
    uint16_t tx_burst;
    uint16_t lcore_id;
    uint64_t rx_packets;
    uint64_t tx_packets;
} uet_dpdk_port;

uet_status uet_dpdk_port_init(
    uet_dpdk_port *port,
    uint16_t port_id,
    uint16_t lcore_id,
    uint16_t rx_burst,
    uint16_t tx_burst
);
void uet_dpdk_port_record_rx(uet_dpdk_port *port, size_t count);
void uet_dpdk_port_record_tx(uet_dpdk_port *port, size_t count);

#endif
