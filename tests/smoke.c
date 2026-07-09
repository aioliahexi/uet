#include <assert.h>
#include <stddef.h>

#include "uet/host_if/admin.h"

int main(void)
{
    uet_device device;
    uet_device_caps caps;
    uet_send_wqe wqe = {
        .wr_id = 1,
        .opcode = UET_OPCODE_WRITE,
        .length = 128,
        .remote_addr = 0x200000,
    };
    uet_packet tx_packets[4];
    uet_packet ack;
    uet_packet inbound_write;
    uet_cqe cqes[4];
    uet_telemetry_snapshot stats;
    uint32_t pd_num;
    uint32_t cq_num;
    uint32_t qp_num;
    uint32_t mr_num;
    uint32_t rkey;
    size_t count;
    bool has_response;

    assert(uet_device_bootstrap(&device) == UET_OK);
    assert(uet_device_query_caps(&device, &caps) == UET_OK);
    assert(caps.max_qps == UET_MAX_QPS);

    assert(uet_admin_alloc_pd(&device, &pd_num) == UET_OK);
    assert(uet_admin_create_cq(&device, 16, &cq_num) == UET_OK);
    assert(
        uet_admin_register_mr(
            &device,
            pd_num,
            0x100000,
            4096,
            UET_ACCESS_LOCAL_WRITE | UET_ACCESS_REMOTE_WRITE,
            &mr_num,
            &rkey
        ) == UET_OK
    );
    assert(mr_num == 1);

    assert(uet_admin_create_qp(&device, pd_num, cq_num, cq_num, 0, &qp_num) == UET_OK);
    wqe.remote_key = rkey;

    assert(uet_admin_post_send(&device, qp_num, &wqe) == UET_OK);
    assert(uet_admin_tx_step(&device, qp_num, tx_packets, 4, &count) == UET_OK);
    assert(count == 1);
    assert(tx_packets[0].opcode == UET_OPCODE_WRITE);
    assert(tx_packets[0].qp_num == qp_num);

    inbound_write = tx_packets[0];
    assert(uet_admin_rx_step(&device, &inbound_write, &ack, &has_response) == UET_OK);
    assert(has_response);
    assert(ack.opcode == UET_OPCODE_ACK);
    assert(ack.psn == inbound_write.psn);

    assert(uet_admin_poll_cq(&device, cq_num, cqes, 4, &count) == UET_OK);
    assert(count == 1);
    assert(cqes[0].qp_num == qp_num);
    assert(cqes[0].opcode == UET_OPCODE_WRITE);
    assert(cqes[0].status == UET_OK);

    uet_admin_query_stats(&device, &stats);
    assert(stats.dma_maps == 1);
    assert(stats.doorbells == 1);
    assert(stats.completions == 1);
    assert(stats.acks == 1);
    assert(stats.cqe_posts == 1);

    return 0;
}
