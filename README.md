# 4×4 Mesh Network-on-Chip Simulator — COE838

A cycle-accurate SystemC simulation of a 4×4 mesh Network-on-Chip (NoC). The network consists of 16 wormhole routers connected in a 2D mesh, each with 5 ports (local + N/S/E/W), supported by per-port FIFO buffers, a round-robin arbiter, and a 5×5 crossbar switch. Traffic injection is with uniform-random injection.

## Architecture Overview

```
                  Source 0  Source 1  ...  Source 15
                     │          │               │
                  ┌──▼──┐  ┌──▼──┐         ┌──▼──┐
    ←── W   E ──► │  R0 ├──┤  R1 ├── ... ──┤ R15 │ ←── W   E ──►
                  └──┬──┘  └──┬──┘         └──┬──┘
                     │          │               │
                  Sink 0    Sink 1           Sink 15

Each router:
   in0..in4 ──► [buf_fifo ×5] ──► [crossbar 5×5] ──► out0..out4
                      │                  ▲
                      └──► req ──► [arbiter] ──► grant + aselect
                           ack ◄────────────────── inack
```

## Module Hierarchy

```
sc_main  (top-level instantiator)
├── 16× source   [SC_MODULE]
├── 16× sink     [SC_MODULE]
└── 16× router   [SC_MODULE]
        ├── buf_fifo  [×5]   — one per input port (ports 0–4)
        ├── arbiter   [×1]   — grants one input per output, drives crossbar config
        └── crossbar  [×1]   — 5×5 packet switch
```

## Modules

### `source`
Generates packets and injects them into the network. Each source has a unique 4-bit ID and a configurable destination. It waits for an acknowledgement before sending the next packet.

| Port | Dir | Type | Description |
|------|-----|------|-------------|
| `packet_out` | out | packet | Outgoing packet to router port 0 |
| `source_id` | in | sc_uint<4> | This node's ID (0–15) |
| `ach_in` | in | bool | Backpressure ack from router |
| `CLK` | in | sc_in_clk | Source injection clock (125 ns period) |
| `d_est` | in | int | Destination node ID |
| `ch_k` | in | sc_uint<4> | (Traffic pattern check signal) |

Member: `pkt_snt` — running count of packets sent.

---

### `sink`
Receives packets from the network and issues acknowledgements upstream.

| Port | Dir | Type | Description |
|------|-----|------|-------------|
| `packet_in` | in | packet | Incoming packet from router port 0 |
| `ack_out` | out | bool | Acknowledgement back to router |
| `sink_id` | in | sc_uint<4> | This node's ID (0–15) |
| `sclk` | in | bool | Sink sampling clock (5 ns period) |
| `packet_out_sink` | out | packet | Forwarded packet (for tracing) |

Member: `pkt_recv` — running count of packets received.

---

### `router`
The core switching element. Routes wormhole packets across the mesh using XY routing (arbiter computes output port from `dest` field).

| Port | Dir | Type | Description |
|------|-----|------|-------------|
| `in0..in4` | in | packet | 5 input ports (0=local/source, 1–4=mesh neighbours) |
| `out0..out4` | out | packet | 5 output ports (0=local/sink, 1–4=mesh neighbours) |
| `inack0..inack4` | in | bool | Downstream backpressure (from sink or next router) |
| `outack0..outack4` | out | bool | Upstream backpressure (to source or previous router) |
| `router_id` | in | sc_uint<4> | Node ID — used by arbiter for XY routing |
| `rclk` | in | bool | Router clock (5 ns period) |

Internal signals: `req_s_0..4` (7-bit request from FIFO), `gr_s_0..4` (1-bit grant from arbiter), `re_s_0..4` (packet read from FIFO into crossbar), `select_s` (15-bit crossbar config).

---

### `buf_fifo`
4-deep packet FIFO on each router input port. Buffers incoming packets, issues routing requests to the arbiter, and generates backpressure acks to the upstream sender.

| Port | Dir | Type | Description |
|------|-----|------|-------------|
| `wr` | in | packet | Packet arriving from upstream |
| `re` | out | packet | Packet read out to crossbar |
| `grant` | in | sc_uint<1> | Arbiter grants this port access to an output |
| `req` | out | sc_uint<7> | Request to arbiter — encodes desired output port (XY-routed from packet.dest) |
| `ack` | out | bool | Flow-control ack sent upstream |
| `bclk` | in | bool | Buffer clock |

Internal: `fifo` struct with a 4-element `packet` array and `full`/`empty`/`reg_num` status fields.

---

### `arbiter`
Resolves contention among the 5 input FIFOs for the 5 output ports. Reads request vectors, checks downstream availability via `free_out` signals, and outputs a 15-bit `aselect` word that configures the crossbar.

| Port | Dir | Type | Description |
|------|-----|------|-------------|
| `arbiter_id` | in | sc_uint<4> | Router node ID (for XY routing computation) |
| `req0..req4` | in | sc_uint<7> | Requests from each buf_fifo |
| `free_out0..free_out4` | in | bool | Downstream port availability (from inack signals) |
| `aselect` | out | sc_uint<15> | Crossbar switch word: 3 bits × 5 outputs = selected input per output |
| `grant0..grant4` | out | sc_uint<1> | Grant issued to each FIFO |
| `aclk` | in | bool | Clock (sensitive to negative edge) |

---

### `crossbar`
5×5 non-blocking packet switch. Reads 5 packet inputs and routes each to the appropriate output port based on the `config` word from the arbiter.

| Port | Dir | Type | Description |
|------|-----|------|-------------|
| `i0..i4` | in | packet | Packet inputs from buf_fifo read ports |
| `o0..o4` | out | packet | Packet outputs to router output ports |
| `config` | in | sc_uint<15> | Switch configuration (3 bits per output selects one of 5 inputs) |

---

## Packet Format

```cpp
struct packet {
    sc_uint<11> data;      // 11-bit payload
    sc_uint<4>  id;        // source node ID
    sc_uint<4>  dest;      // destination node ID
    sc_uint<1>  pkt_clk;  // token bit (distinguishes new packets)
    sc_uint<1>  h_t;       // 0=header flit, 1=tail flit (wormhole routing)
};
```

Wormhole routing: a packet is split into flits. The header flit establishes the path through the network; subsequent flits follow without re-arbitration until the tail flit clears the route.

## Network Topology

- **16 routers** arranged in a 4×4 grid (nodes 0–15).
- **5 ports per router**: port 0 = local (to/from source and sink), ports 1–4 = inter-router mesh links.
- **48 inter-router channels** (`si_output[0..47]`) — each is a `sc_signal<packet>`.
- **48 backpressure channels** (`si_ack_in[0..47]`) — `sc_signal<bool>` flowing in the opposite direction.
- Port connectivity is defined by the `routerIn[16][5]` and `routerOut[16][5]` tables in `main_noc.cpp`.

## Clocks

| Clock | Period | Used by |
|-------|--------|---------|
| `s_clock` | 125 ns | Source injection (one packet injection opportunity per tick) |
| `r_clock` | 5 ns | Router processing — arbiter and buf_fifo |
| `d_clock` | 5 ns | Sink sampling |

## Traffic Patterns

Configured interactively at simulation startup:

| Input | Pattern |
|-------|---------|
| `1` | **Uniform random** — each source is assigned a random unique destination (random permutation of 0–15) |
| `0` | **Neighbour** — not fully implemented in the current source |

## Simulation Output

At the end of simulation, a per-node summary is printed:

```
Node 0: Sent = X, Received = Y
Node 1: Sent = X, Received = Y
...
```

A VCD waveform file (`graph.vcd`) is generated tracing `s_clock`, `d_clock`, `r_clock`, and all `si_source` / `si_sink` signals, viewable in GTKWave or ModelSim.

## Files

| File | Description |
|------|-------------|
| `main_noc.cpp` | Top-level `sc_main` — instantiates all modules, wires the mesh, runs simulation |
| `router.h` | `SC_MODULE(router)` — port declarations and constructor wiring |
| `router.cpp` | Router `func()` thread (monitoring/logging) |
| `buf_fifo.h` | `SC_MODULE(buf_fifo)` and `fifo` struct |
| `buf_fifo.cpp` | FIFO `func()` — enqueue, dequeue, req/ack logic |
| `arbiter.h` | `SC_MODULE(arbiter)` — port declarations |
| `arbiter.cpp` | Arbiter `func()` — XY routing, grant resolution, crossbar config |
| `crossbar.h` | `SC_MODULE(crossbar)` — port declarations |
| `crossbar.cpp` | Crossbar `func()` — applies `config` to route packets |
| `source.h` | `SC_MODULE(source)` — port declarations |
| `source.cpp` | Source `func()` — packet generation and injection |
| `sink.h` | `SC_MODULE(sink)` — port declarations |
| `sink.cpp` | Sink `receive_data()` — packet reception and ack |
| `packet.h` | `packet` struct definition + SystemC trace support |

## How to Run

**Prerequisites:** SystemC 2.3.x, g++ with C++11

```
1. Install SystemC: https://www.accellera.org/downloads/standards/systemc
2. Set SYSTEMC_HOME in your environment:
   export SYSTEMC_HOME=/path/to/systemc
3. Build:
   make
4. Run (select traffic pattern at prompt):
   ./noc.x
5. View waveforms:
   gtkwave graph.vcd
```

## Design Decisions & Tradeoffs

- **SystemC TLM (transaction-level) rather than RTL** — enables fast simulation of full network behaviour without gate-level detail; the model is not synthesizable but is ideal for architecture exploration.
- **2D mesh topology with XY (dimension-order) routing** — deadlock-free by construction and simple to implement; not adaptive, so traffic imbalances cannot be rerouted around congested links.
- **Wormhole switching with 4-deep FIFOs** — low latency for short packets since header routing is established immediately; head-of-line blocking is possible when a packet stalls waiting for a downstream port.
- **Round-robin arbitration** (inferred from arbiter logic) — provides fair access across all input ports but is not QoS-aware; high-priority traffic cannot be expedited.

## Future Improvements

- Implement the neighbour traffic pattern alongside the existing uniform-random injection — the input switch exists in main_noc.cpp but the pattern logic was not completed.
- Add adaptive routing (turn model or odd-even routing) to improve throughput under non-uniform congestion.
- Move from TLM to cycle-accurate RTL for FPGA synthesis and hardware validation.
- Add QoS support: priority queues or virtual channels to prevent head-of-line blocking for latency-sensitive traffic.
- Benchmark with standard synthetic traffic patterns (uniform random, hotspot, bit-complement, bit-reversal) and produce throughput vs. latency curves.

## Skills Demonstrated

`SystemC · TLM simulation · mesh topology · wormhole routing · XY routing · arbitration · FIFO design · C++ · GTKWave · Makefile`

## License

MIT License — see [LICENSE](../LICENSE) for details.
