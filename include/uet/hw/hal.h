#ifndef UET_HW_HAL_H
#define UET_HW_HAL_H

#include "uet/types.h"

typedef struct {
    uint32_t max_qps;
    uint32_t max_cqs;
    uint32_t max_mrs;
    uint32_t max_pds;
    uint32_t max_sq_depth;
    uint32_t max_cq_depth;
    uint16_t lcore_count;
} uet_device_caps;

typedef struct {
    uet_device_caps caps;
    uint32_t next_queue_id;
    uint64_t doorbell_count;
    uint64_t dma_map_count;
    uint64_t event_poll_count;
    uint64_t cqe_post_count;
} uet_hal;

uet_status uet_hal_init(uet_hal *hal, const uet_device_caps *caps);
uet_status uet_hal_queue_create(uet_hal *hal, uint32_t depth, uint32_t *queue_id);
uet_status uet_hal_doorbell_ring(uet_hal *hal, uint32_t queue_id);
uet_status uet_hal_dma_map(uet_hal *hal, uint64_t addr, uint32_t length, uint32_t *map_id);
uet_status uet_hal_event_poll(uet_hal *hal, uint32_t budget, uint32_t *events_found);
uet_status uet_hal_cqe_post(uet_hal *hal, uint32_t cq_num, const uet_cqe *cqe);

#endif
