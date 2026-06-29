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

static void test_opcode_values(void)
{
    /* Verify the opcode constants fit in a uint8_t and are distinct. */
    assert(UET_OP_WRITE         <= 0xffU);
    assert(UET_OP_READ_REQUEST  <= 0xffU);
    assert(UET_OP_READ_RESPONSE <= 0xffU);
    assert(UET_OP_WRITE         != UET_OP_READ_REQUEST);
    assert(UET_OP_WRITE         != UET_OP_READ_RESPONSE);
    assert(UET_OP_READ_REQUEST  != UET_OP_READ_RESPONSE);

    /* Verify a WRITE header encodes the right opcode. */
    struct uet_transport_header h = {
        .version = UET_TRANSPORT_VERSION,
        .flags = UET_OP_WRITE,
        .header_length = UET_TRANSPORT_HEADER_LENGTH,
        .flow_id = 42U,
        .sequence_number = 1U,
        .acknowledgement_number = 0U,
        .payload_length = 64U,
        .checksum = 0U,
    };
    struct uet_transport_header decoded = {0};
    uint8_t buf[UET_TRANSPORT_HEADER_LENGTH];

    assert(uet_transport_encode_header(&h, buf, sizeof(buf)) == 0);
    assert(uet_transport_decode_header(&decoded, buf, sizeof(buf)) == 0);
    assert(decoded.flags == UET_OP_WRITE);
}

static void test_read_request_round_trip(void)
{
    struct uet_read_request_payload in = {
        .remote_addr = 0xDEADBEEFCAFEBABEULL,
        .read_length = 0x00001000U,
    };
    struct uet_read_request_payload out = {0};
    uint8_t buf[UET_READ_REQUEST_PAYLOAD_LENGTH];

    assert(uet_transport_encode_read_request(&in, buf, sizeof(buf)) == 0);
    assert(uet_transport_decode_read_request(&out, buf, sizeof(buf)) == 0);
    assert(out.remote_addr  == in.remote_addr);
    assert(out.read_length  == in.read_length);
}

static void test_read_request_reject_short_buffer(void)
{
    struct uet_read_request_payload req = {
        .remote_addr = 0xABCDULL,
        .read_length = 128U,
    };
    uint8_t buf[UET_READ_REQUEST_PAYLOAD_LENGTH];

    assert(uet_transport_encode_read_request(&req, buf, sizeof(buf) - 1) < 0);
    assert(uet_transport_decode_read_request(&req, buf, sizeof(buf) - 1) < 0);
}

int main(void)
{
    test_config_validation();
    test_header_round_trip();
    test_reject_short_buffer();
    test_opcode_values();
    test_read_request_round_trip();
    test_read_request_reject_short_buffer();
    return 0;
}
