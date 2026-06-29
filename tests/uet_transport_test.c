#include <assert.h>
#include <stdint.h>

#include "uet_transport.h"

static void test_config_validation(void)
{
    struct uet_transport_config config = {
        .port_id = 0,
        .rx_queue_id = 0,
        .tx_queue_id = 0,
        .burst_size = 1,
        .flow_id = 7,
        .mtu = 1500,
    };

    assert(uet_transport_config_is_valid(&config));

    config.burst_size = 0;
    assert(!uet_transport_config_is_valid(&config));

    config.burst_size = 1;
    config.mtu = UET_TRANSPORT_HEADER_LENGTH;
    assert(!uet_transport_config_is_valid(&config));
}

static void test_header_round_trip(void)
{
    struct uet_transport_header input = {
        .version = UET_TRANSPORT_VERSION,
        .flags = 0x5aU,
        .header_length = UET_TRANSPORT_HEADER_LENGTH,
        .flow_id = 0x01020304U,
        .sequence_number = 0x11121314U,
        .acknowledgement_number = 0x21222324U,
        .payload_length = 512U,
        .checksum = 0U,
    };
    struct uet_transport_header output = {0};
    uint8_t encoded[UET_TRANSPORT_HEADER_LENGTH];

    assert(uet_transport_encode_header(&input, encoded, sizeof(encoded)) == 0);
    assert(uet_transport_decode_header(&output, encoded, sizeof(encoded)) == 0);
    assert(output.version == input.version);
    assert(output.flags == input.flags);
    assert(output.header_length == input.header_length);
    assert(output.flow_id == input.flow_id);
    assert(output.sequence_number == input.sequence_number);
    assert(output.acknowledgement_number == input.acknowledgement_number);
    assert(output.payload_length == input.payload_length);
    assert(output.checksum == input.checksum);
}

static void test_reject_short_buffer(void)
{
    struct uet_transport_header header = {
        .version = UET_TRANSPORT_VERSION,
        .flags = 0,
        .header_length = UET_TRANSPORT_HEADER_LENGTH,
        .flow_id = 1,
        .sequence_number = 1,
        .acknowledgement_number = 0,
        .payload_length = 32,
        .checksum = 0,
    };
    uint8_t encoded[UET_TRANSPORT_HEADER_LENGTH];

    assert(uet_transport_encode_header(&header, encoded, sizeof(encoded) - 1) < 0);
    assert(uet_transport_decode_header(&header, encoded, sizeof(encoded) - 1) < 0);
}

int main(void)
{
    test_config_validation();
    test_header_round_trip();
    test_reject_short_buffer();
    return 0;
}
