CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude

SRC := \
	src/host_if/admin.c \
	src/hw/hal.c \
	src/mgmt/telemetry.c \
	src/net/dpdk_port.c \
	src/rdma/core.c \
	src/rdma/engine.c \
	src/rdma/proto.c \
	src/rdma/reliability.c

OBJ := $(SRC:.c=.o)

.PHONY: all clean test

all: tests/smoke

tests/smoke: tests/smoke.c $(OBJ)
	$(CC) $(CFLAGS) -o $@ tests/smoke.c $(OBJ)

test: tests/smoke
	./tests/smoke

clean:
	rm -f $(OBJ) tests/smoke
