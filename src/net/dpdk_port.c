#include <string.h>

#include "uet/net/dpdk_port.h"

uet_status uet_dpdk_port_init(
    uet_dpdk_port *port,
    uint16_t port_id,
    uint16_t lcore_id,
    uint16_t rx_burst,
    uint16_t tx_burst
)
{
    if (port == NULL || rx_burst == 0 || tx_burst == 0) {
        return UET_ERR_INVAL;
    }

    memset(port, 0, sizeof(*port));
    port->port_id = port_id;
    port->lcore_id = lcore_id;
    port->rx_burst = rx_burst;
    port->tx_burst = tx_burst;
    return UET_OK;
}

void uet_dpdk_port_record_rx(uet_dpdk_port *port, size_t count)
{
    if (port != NULL) {
        port->rx_packets += count;
    }
}

void uet_dpdk_port_record_tx(uet_dpdk_port *port, size_t count)
{
    if (port != NULL) {
        port->tx_packets += count;
    }
}
