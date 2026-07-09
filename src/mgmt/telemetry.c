#include <string.h>

#include "uet/mgmt/telemetry.h"

void uet_telemetry_collect(
    uet_telemetry_snapshot *snapshot,
    const uet_hal *hal,
    const uet_dpdk_port *port,
    const uet_engine_stats *stats
)
{
    if (snapshot == NULL || hal == NULL || port == NULL || stats == NULL) {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->rx_packets = port->rx_packets + stats->rx_packets;
    snapshot->tx_packets = port->tx_packets + stats->tx_packets;
    snapshot->completions = stats->completions;
    snapshot->acks = stats->acks;
    snapshot->drops = stats->drops;
    snapshot->doorbells = hal->doorbell_count;
    snapshot->dma_maps = hal->dma_map_count;
    snapshot->event_polls = hal->event_poll_count;
    snapshot->cqe_posts = hal->cqe_post_count;
}
