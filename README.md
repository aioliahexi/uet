# uet

Minimal software-defined UET transport bootstrap with a DPDK 22.11.x data plane.

## What is included

- A compact UET wire header encoder/decoder
- A DPDK-backed send/receive transport skeleton
- A demo application that sends a single UET payload
- Focused protocol tests that run without DPDK installed

## Requirements

- `libdpdk` **22.11.x** available through `pkg-config`
- A C11 compiler
- GNU `make`

The default build enforces the DPDK 22.11.x requirement and fails fast if another
version is installed.

## Build

```sh
make
```

## Test

```sh
make test
```

## Run

Pass DPDK EAL arguments first, then app-specific arguments after `--`:

```sh
./build/uet_dpdk_demo -l 0-1 -n 4 -- --port 0 --flow 1 --message hello
```