CC ?= cc
PKG_CONFIG ?= pkg-config
BUILD_DIR := build
APP_BIN := $(BUILD_DIR)/uet_dpdk_demo
TEST_BIN := $(BUILD_DIR)/uet_transport_test
COMMON_CFLAGS := -O2 -Wall -Wextra -Werror -std=c11 -Iinclude
DPDK_CFLAGS := $(shell $(PKG_CONFIG) --cflags libdpdk 2>/dev/null)
DPDK_LIBS := $(shell $(PKG_CONFIG) --libs libdpdk 2>/dev/null)

.PHONY: all build clean check-dpdk test

all: check-dpdk $(APP_BIN)

build:
	mkdir -p $(BUILD_DIR)

check-dpdk:
	@$(PKG_CONFIG) --exists libdpdk || (echo "libdpdk was not found via pkg-config." && exit 1)
	@version="$$($(PKG_CONFIG) --modversion libdpdk)"; \
	case "$$version" in \
		22.11*) ;; \
		*) echo "DPDK 22.11.x is required, found $$version."; exit 1 ;; \
	esac

$(APP_BIN): src/main.c src/uet_dpdk_transport.c src/uet_transport.c include/uet_dpdk_transport.h include/uet_transport.h | build
	$(CC) $(COMMON_CFLAGS) $(DPDK_CFLAGS) -o $@ src/main.c src/uet_dpdk_transport.c src/uet_transport.c $(DPDK_LIBS)

$(TEST_BIN): tests/uet_transport_test.c src/uet_transport.c include/uet_transport.h | build
	$(CC) $(COMMON_CFLAGS) -o $@ tests/uet_transport_test.c src/uet_transport.c

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)
