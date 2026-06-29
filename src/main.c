#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uet_dpdk_transport.h"

static int find_app_arg_offset(int argc, char **argv)
{
    int index;

    for (index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--") == 0) {
            return index + 1;
        }
    }

    return argc;
}

static int parse_uint16(const char *text, uint16_t *value)
{
    unsigned long parsed;
    char *end;

    parsed = strtoul(text, &end, 10);
    if (*text == '\0' || *end != '\0' || parsed > 0xffffUL) {
        return -EINVAL;
    }

    *value = (uint16_t)parsed;
    return 0;
}

static int parse_uint32(const char *text, uint32_t *value)
{
    unsigned long parsed;
    char *end;

    parsed = strtoul(text, &end, 10);
    if (*text == '\0' || *end != '\0' || parsed > 0xffffffffUL) {
        return -EINVAL;
    }

    *value = (uint32_t)parsed;
    return 0;
}

static int parse_app_args(
    int argc,
    char **argv,
    struct uet_transport_config *config,
    const char **message)
{
    int index;

    for (index = 0; index < argc; index++) {
        if (strcmp(argv[index], "--port") == 0 && index + 1 < argc) {
            if (parse_uint16(argv[++index], &config->port_id) != 0) {
                return -EINVAL;
            }
        } else if (strcmp(argv[index], "--flow") == 0 && index + 1 < argc) {
            if (parse_uint32(argv[++index], &config->flow_id) != 0) {
                return -EINVAL;
            }
        } else if (strcmp(argv[index], "--mtu") == 0 && index + 1 < argc) {
            if (parse_uint16(argv[++index], &config->mtu) != 0) {
                return -EINVAL;
            }
        } else if (strcmp(argv[index], "--message") == 0 && index + 1 < argc) {
            *message = argv[++index];
        } else {
            return -EINVAL;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    struct uet_transport_config config = {
        .port_id = 0,
        .rx_queue_id = 0,
        .tx_queue_id = 0,
        .burst_size = 1,
        .flow_id = 1,
        .mtu = 1500,
    };
    const struct uet_transport_stats *stats;
    struct uet_dpdk_transport *transport;
    const char *message = "uet";
    int app_arg_offset;
    int eal_argc;

    app_arg_offset = find_app_arg_offset(argc, argv);
    eal_argc = app_arg_offset == argc ? argc : app_arg_offset - 1;

    if (app_arg_offset < argc &&
        parse_app_args(argc - app_arg_offset, argv + app_arg_offset, &config, &message) != 0) {
        fprintf(
            stderr,
            "Usage: %s [dpdk eal args] -- [--port N] [--flow N] [--mtu N] [--message TEXT]\n",
            argv[0]);
        return EXIT_FAILURE;
    }

    transport = uet_dpdk_transport_create(&config, eal_argc, argv);
    if (transport == NULL) {
        perror("uet_dpdk_transport_create");
        return EXIT_FAILURE;
    }

    if (uet_dpdk_transport_send(
            transport,
            message,
            (uint16_t)strlen(message),
            1,
            0) != 0) {
        perror("uet_dpdk_transport_send");
        uet_dpdk_transport_destroy(transport);
        return EXIT_FAILURE;
    }

    stats = uet_dpdk_transport_stats(transport);
    printf(
        "sent message on port %u flow %u (%llu packets, %llu bytes)\n",
        config.port_id,
        config.flow_id,
        (unsigned long long)stats->tx_packets,
        (unsigned long long)stats->tx_bytes);

    uet_dpdk_transport_destroy(transport);
    return EXIT_SUCCESS;
}
