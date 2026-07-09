#include <string.h>

#include "uet/hw/hal.h"

uet_status uet_hal_init(uet_hal *hal, const uet_device_caps *caps)
{
    if (hal == NULL || caps == NULL) {
        return UET_ERR_INVAL;
    }

    memset(hal, 0, sizeof(*hal));
    hal->caps = *caps;
    return UET_OK;
}

uet_status uet_hal_queue_create(uet_hal *hal, uint32_t depth, uint32_t *queue_id)
{
    if (hal == NULL || queue_id == NULL || depth == 0) {
        return UET_ERR_INVAL;
    }

    hal->next_queue_id += 1;
    *queue_id = hal->next_queue_id;
    return UET_OK;
}

uet_status uet_hal_doorbell_ring(uet_hal *hal, uint32_t queue_id)
{
    if (hal == NULL || queue_id == 0) {
        return UET_ERR_INVAL;
    }

    hal->doorbell_count += 1;
    return UET_OK;
}

uet_status uet_hal_dma_map(uet_hal *hal, uint64_t addr, uint32_t length, uint32_t *map_id)
{
    if (hal == NULL || map_id == NULL || addr == 0 || length == 0) {
        return UET_ERR_INVAL;
    }

    hal->dma_map_count += 1;
    *map_id = (uint32_t) hal->dma_map_count;
    return UET_OK;
}

uet_status uet_hal_event_poll(uet_hal *hal, uint32_t budget, uint32_t *events_found)
{
    if (hal == NULL || events_found == NULL || budget == 0) {
        return UET_ERR_INVAL;
    }

    hal->event_poll_count += 1;
    *events_found = 0;
    return UET_OK;
}

uet_status uet_hal_cqe_post(uet_hal *hal, uint32_t cq_num, const uet_cqe *cqe)
{
    if (hal == NULL || cqe == NULL || cq_num == 0) {
        return UET_ERR_INVAL;
    }

    hal->cqe_post_count += 1;
    return UET_OK;
}
