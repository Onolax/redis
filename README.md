# Build Your Own Redis (C/C++)

A Redis-like in-memory key-value store built from scratch in C/C++, following the guide **"Build Your Own Redis with C/C++"** by James Smith.

## What This Project Does

This project implements a subset of Redis features from the ground up, including:

- **Event-driven server** using `poll()` for non-blocking I/O and connection multiplexing
- **Custom binary protocol** — length-prefixed messages with pipelined request support
- **Key-value store** with `get`, `set`, `del`, and `keys` commands
- **TTL / expiry** via `pexpire` and `pttl` (millisecond precision)
- **Sorted sets (ZSet)** with `zadd`, `zrem`, `zscore`, and `zquery` commands
- **Idle connection timeout** — connections inactive for 5 seconds are automatically closed
- **Progressive rehashing hashtable** — two-table design avoids blocking during resize
- **AVL tree** for sorted set ordering by (score, name)
- **Min-heap** for efficient TTL expiry tracking

### Data Structures Built From Scratch

| File | Description |
|------|-------------|
| `hashtable.h/cpp` | Chained hashtable with incremental rehashing |
| `avl.h/cpp` | Self-balancing AVL tree with rank/offset queries |
| `zset.h/cpp` | Sorted set backed by AVL tree + hashtable |
| `heap.h/cpp` | Binary min-heap for TTL timers |
| `list.h` | Intrusive doubly-linked list for idle timers |

### Supported Commands

| Command | Syntax | Description |
|---------|--------|-------------|
| `get` | `get <key>` | Get string value |
| `set` | `set <key> <value>` | Set string value |
| `del` | `del <key>` | Delete a key |
| `keys` | `keys` | List all keys |
| `pexpire` | `pexpire <key> <ttl_ms>` | Set TTL in milliseconds |
| `pttl` | `pttl <key>` | Get remaining TTL in milliseconds |
| `zadd` | `zadd <zset> <score> <name>` | Add/update member in sorted set |
| `zrem` | `zrem <zset> <name>` | Remove member from sorted set |
| `zscore` | `zscore <zset> <name>` | Get score of a member |
| `zquery` | `zquery <zset> <score> <name> <offset> <limit>` | Range query by score |

## Building

No build system is required — compile directly with `g++`:

```bash
# Build the server
g++ -Wall -Wextra -O2 -o server server.cpp hashtable.cpp avl.cpp zset.cpp heap.cpp -lm

# Build the client
g++ -Wall -Wextra -O2 -o client client.cpp

# Build AVL tree tests
g++ -Wall -Wextra -O2 -o test_avl test_avl.cpp avl.cpp

# Build heap tests
g++ -Wall -Wextra -O2 -o test_heap test_heap.cpp heap.cpp
```

## Running

Start the server (listens on port `1234`):

```bash
./server
```

In another terminal, use the client:

```bash
./client set foo bar
./client get foo
./client del foo
./client keys
```

## Testing Your System

### 1. Basic key-value operations

```bash
./client set name alice
# (nil)

./client get name
# (str) alice

./client del name
# (int) 1

./client get name
# (nil)
```

### 2. TTL expiry

```bash
./client set temp value
./client pexpire temp 5000    # expire in 5 seconds
./client pttl temp            # check remaining TTL (ms)
# (int) ~4999

# wait 5 seconds...
./client get temp
# (nil)
```

### 3. Sorted sets

```bash
./client zadd leaderboard 100 alice
./client zadd leaderboard 200 bob
./client zadd leaderboard 150 charlie

./client zscore leaderboard alice
# (dbl) 100

# query from score 100, name "", offset 0, limit 10
./client zquery leaderboard 100 "" 0 10
# (arr) len=6
# (str) alice
# (dbl) 100
# ...
# (arr) end

./client zrem leaderboard alice
# (int) 1
```

### 4. Run unit tests

```bash
./test_avl
./test_heap
```

### 5. Idle timeout test

Open a connection and leave it idle — the server will close it after **5 seconds** and log:

```
removing idle connection: <fd>
```

## Architecture Overview

```
client.cpp          — CLI client, sends commands, prints responses
server.cpp          — Event loop (poll), connection handling, command dispatch
├── hashtable.*     — Top-level KV store (progressive rehashing)
├── zset.*          — Sorted set implementation
│   ├── avl.*       — AVL tree (ordered by score+name)
│   └── hashtable.* — Secondary index (lookup by name)
├── heap.*          — Min-heap for TTL expiry
└── list.h          — Doubly-linked list for idle connection timers
```

## Protocol

Messages are length-prefixed:

```
+--------+----------+
| 4 bytes| N bytes  |
| length | payload  |
+--------+----------+
```

The payload encodes an array of strings (number of strings + each string length-prefixed). Responses are tagged with a type byte (`nil`, `err`, `str`, `int`, `dbl`, `arr`).
