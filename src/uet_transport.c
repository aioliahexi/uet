#include "uet_transport.h"

#include <errno.h>

static void write_u16(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value >> 8);
    buffer[1] = (uint8_t)value;
}

static void write_u32(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value >> 24);
    buffer[1] = (uint8_t)(value >> 16);
    buffer[2] = (uint8_t)(value >> 8);
    buffer[3] = (uint8_t)value;
}

static uint16_t read_u16(const uint8_t *buffer)
{
    return (uint16_t)(((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1]);
}

static uint32_t read_u32(const uint8_t *buffer)
{
    return ((uint32_t)buffer[0] << 24) |
           ((uint32_t)buffer[1] << 16) |
           ((uint32_t)buffer[2] << 8) |
           (uint32_t)buffer[3];
}

bool uet_transport_config_is_valid(const struct uet_transport_config *config)
{
    if (config == NULL) {
        return false;
    }

    if (config->burst_size == 0U || config->mtu <= UET_TRANSPORT_HEADER_LENGTH) {
        return false;
    }

    return true;
}

int uet_transport_encode_header(
    const struct uet_transport_header *header,
    uint8_t *buffer,
    size_t buffer_length)
{
    if (header == NULL || buffer == NULL) {
        return -EINVAL;
    }

    if (buffer_length < UET_TRANSPORT_HEADER_LENGTH) {
        return -EMSGSIZE;
    }

    if (header->version != UET_TRANSPORT_VERSION ||
        header->header_length != UET_TRANSPORT_HEADER_LENGTH) {
        return -EINVAL;
    }

    buffer[0] = header->version;
    buffer[1] = header->flags;
    write_u16(buffer + 2, header->header_length);
    write_u32(buffer + 4, header->flow_id);
    write_u32(buffer + 8, header->sequence_number);
    write_u32(buffer + 12, header->acknowledgement_number);
    write_u16(buffer + 16, header->payload_length);
    write_u16(buffer + 18, header->checksum);

    return 0;
}

int uet_transport_decode_header(
    struct uet_transport_header *header,
    const uint8_t *buffer,
    size_t buffer_length)
{
    if (header == NULL || buffer == NULL) {
        return -EINVAL;
    }

    if (buffer_length < UET_TRANSPORT_HEADER_LENGTH) {
        return -EMSGSIZE;
    }

    header->version = buffer[0];
    header->flags = buffer[1];
    header->header_length = read_u16(buffer + 2);
    header->flow_id = read_u32(buffer + 4);
    header->sequence_number = read_u32(buffer + 8);
    header->acknowledgement_number = read_u32(buffer + 12);
    header->payload_length = read_u16(buffer + 16);
    header->checksum = read_u16(buffer + 18);

    if (header->version != UET_TRANSPORT_VERSION ||
        header->header_length != UET_TRANSPORT_HEADER_LENGTH) {
        return -EPROTO;
    }

    return 0;
}
