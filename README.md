# uet

`uet` 现在提供一个面向 **DPU ARM + DPDK + RDMA/RoCEv2** 的最小代码骨架，用来表达以下设计原则：

- 硬件抽象层、DPU 协议处理层、Host 设备接口三层解耦
- 控制面和数据面分离
- 关键对象通过统一对象表管理
- 快路径保持定长结构和低锁路径

## 目录结构

```text
/home/runner/work/uet/uet
├── Makefile
├── README.md
├── include/uet
│   ├── host_if/admin.h
│   ├── hw/hal.h
│   ├── mgmt/telemetry.h
│   ├── net/dpdk_port.h
│   ├── rdma/core.h
│   ├── rdma/engine.h
│   ├── rdma/proto.h
│   ├── rdma/reliability.h
│   └── types.h
├── src
│   ├── host_if/admin.c
│   ├── hw/hal.c
│   ├── mgmt/telemetry.c
│   ├── net/dpdk_port.c
│   ├── rdma/core.c
│   ├── rdma/engine.c
│   ├── rdma/proto.c
│   └── rdma/reliability.c
└── tests
    └── smoke.c
```

## 模块职责

### `hw/`

硬件抽象层只暴露稳定能力：

- queue create
- doorbell ring
- dma map
- event poll
- cqe post

上层 RDMA 代码不直接碰寄存器，从而允许后续替换 FPGA/ASIC 实现。

### `net/`

`net/dpdk_port.*` 只抽象 DPDK 风格的数据路径边界：

- RX/TX burst 配置
- lcore/port 参数
- 收发计数

当前实现不依赖真实 DPDK 库，后续可以在保持接口不变的前提下接入 `rte_mbuf` 和 `rte_ethdev`。

### `rdma/core/`

控制面对象表：

- PD
- CQ
- MR
- QP

这里实现对象生命周期、状态机、发送队列和完成队列，是 Host 资源操作的主入口。

### `rdma/proto/`

协议层只负责：

- 包到 RDMA 消息的解码
- WQE 到线包描述的编码
- ACK 响应构造

### `rdma/engine/`

执行引擎负责：

- 从发送队列取 WQE，生成数据面发包
- 收包后按 QP 和 opcode 分发
- MR 权限校验
- 生成 CQE
- 必要时返回 ACK

### `rdma/reliability/`

可靠性模块负责每个 QP 的：

- 期望接收 PSN
- 最近确认 PSN
- 基本 ACK 推进

当前只做最小闭环，便于后续增加重传、乱序缓存和超时恢复。

### `host_if/`

Host 接口层暴露的是资源和队列抽象，而不是协议细节：

- 申请 PD/CQ/MR/QP
- post send
- poll CQ
- query stats
- 驱动 RX/TX step

### `mgmt/`

管理面负责聚合统计信息，便于健康检查和调试。

## 当前最小闭环

当前骨架已经打通以下链路：

1. 初始化设备能力和 HAL
2. 创建 PD/CQ/MR/QP
3. Host post send
4. 执行引擎从发送队列生成线包
5. 收到写请求后做 QP 状态与 MR 权限校验
6. 生成 CQE 和 ACK
7. Host poll CQ 和 query stats

## 构建与测试

```bash
cd /home/runner/work/uet/uet
make
make test
```

## 下一步建议

下一阶段可以在保持当前接口稳定的前提下继续扩展：

1. 接入真实 DPDK RX/TX 和 mempool
2. 为每个 shard/lcore 建立独立 QP 分片
3. 增加 read/atomic 路径
4. 增加超时、重传、乱序重组
5. 用真实 doorbell、DMA 和 completion ring 替换当前 HAL stub
