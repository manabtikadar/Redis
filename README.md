# RediForge

### A Redis-Inspired In-Memory Database Server in C++

RediForge is a **Redis-inspired in-memory key-value database server written from scratch in modern C++**.

The project was built to understand and implement the core systems behind an in-memory database and distributed key-value store, including:

* TCP networking
* Concurrent client handling
* RESP protocol parsing
* Command dispatching
* In-memory data structures
* LRU/LFU eviction
* Persistence
* Primary-replica replication
* Initial database synchronization
* Live replication
* Replica read-only enforcement
* Failure detection
* Leader election
* Automatic failover
* Majority-based voting

The project focuses heavily on **systems programming, networking, concurrency, distributed systems, and C++ architecture**.

> **Note:** RediForge is an educational Redis-inspired implementation and is not intended to be a drop-in replacement for production Redis.

---

# Table of Contents

* [Architecture](#architecture)
* [Features](#features)
* [Technology Stack](#technology-stack)
* [Project Structure](#project-structure)
* [Building the Project](#building-the-project)
* [Starting a Server](#starting-a-server)
* [Running Multiple Nodes](#running-multiple-nodes)
* [Connecting with redis-cli](#connecting-with-redis-cli)
* [Supported Operations](#supported-operations)
* [How the Server Works](#how-the-server-works)
* [RESP Protocol](#resp-protocol)
* [Concurrent Client Handling](#concurrent-client-handling)
* [Database Architecture](#database-architecture)
* [Redis Data Types](#redis-data-types)
* [LRU Eviction](#lru-eviction)
* [LFU Eviction](#lfu-eviction)
* [Persistence](#persistence)
* [Primary-Replica Replication](#primary-replica-replication)
* [Initial Synchronization](#initial-synchronization)
* [Live Replication](#live-replication)
* [Replica Read-Only Mode](#replica-read-only-mode)
* [Failure Detection](#failure-detection)
* [Leader Election](#leader-election)
* [Election Protocol](#election-protocol)
* [Automatic Failover](#automatic-failover)
* [Network Architecture](#network-architecture)
* [Configuration](#configuration)
* [Example Distributed Setup](#example-distributed-setup)
* [Design Decisions](#design-decisions)
* [Current Limitations](#current-limitations)
* [Future Improvements](#future-improvements)
* [Learning Outcomes](#learning-outcomes)

---

# Architecture

At a high level, RediForge follows this architecture:

```text
                         ┌───────────────────────┐
                         │       Client          │
                         │    redis-cli / TCP    │
                         └───────────┬───────────┘
                                     │
                                     │ TCP
                                     ▼
                         ┌───────────────────────┐
                         │      TCP Server       │
                         │   accept() clients    │
                         └───────────┬───────────┘
                                     │
                              client socket
                                     │
                                     ▼
                         ┌───────────────────────┐
                         │   Client Handler      │
                         │                       │
                         │ recv()                │
                         │ input buffering       │
                         │ RESP parsing          │
                         └───────────┬───────────┘
                                     │
                                     ▼
                         ┌───────────────────────┐
                         │   Command Handler     │
                         │                       │
                         │ SET / GET / DEL /     │
                         │ EXISTS / INCR / ...   │
                         └───────────┬───────────┘
                                     │
                                     ▼
                         ┌───────────────────────┐
                         │       Database        │
                         │                       │
                         │ unordered_map         │
                         │ key → RedisValue      │
                         └───────────┬───────────┘
                                     │
                   ┌─────────────────┼─────────────────┐
                   │                 │                 │
                   ▼                 ▼                 ▼
              Persistence       LRU / LFU        Replication
                                                    │
                                                    ▼
                                               Other Nodes
```

The distributed architecture is:

```text
                         ┌─────────────────┐
                         │   Primary       │
                         │   Port 6379     │
                         └────────┬────────┘
                                  │
                         replication stream
                       ┌──────────┴──────────┐
                       │                     │
                       ▼                     ▼
                ┌──────────────┐      ┌──────────────┐
                │   Replica    │      │   Replica    │
                │   Port 6380  │      │   Port 6381  │
                └──────────────┘      └──────────────┘
```

---

# Features

## Core Database

* In-memory key-value database
* String values
* List values
* Hash values
* Set values
* Sorted Set representation
* Command-based interface
* Thread-safe shared database architecture

## Networking

* TCP server
* Multiple simultaneous clients
* One listening socket supporting multiple connections
* Per-client handling using `std::thread`
* Socket-based communication using Linux/POSIX networking APIs

## Protocol

* RESP-style request parsing
* Incremental input buffering
* Handles incomplete TCP messages
* Parses commands into internal `Command` objects
* RESP-compatible responses for supported commands

## Eviction

* LRU cache policy
* LFU cache policy
* Separate LRU/LFU implementations

## Persistence

* Database dump file
* Startup loading
* Database serialization/deserialization infrastructure

## Replication

* Primary-replica architecture
* `REPLICAOF`
* Replica handshake
* Initial synchronization
* Live command replication
* Multiple replicas
* Replica read-only mode

## High Availability

* Primary failure detection
* Candidate state
* Election manager
* Election communication channel
* Vote requests
* Vote responses
* Majority voting
* Randomized election delay
* Automatic promotion of a replica to primary

---

# Technology Stack

| Component       | Technology             |
| --------------- | ---------------------- |
| Language        | C++                    |
| Standard        | C++20                  |
| Build System    | CMake                  |
| Networking      | POSIX TCP sockets      |
| Concurrency     | `std::thread`, mutexes |
| Protocol        | RESP-style protocol    |
| Data Structure  | `std::unordered_map`   |
| Persistence     | File-based storage     |
| OS              | Linux / Ubuntu         |
| Testing         | `redis-cli`, netcat    |
| Version Control | Git / GitHub           |

---

# Project Structure

```text
Redis/
│
├── CMakeLists.txt
│
├── data/
│   └── dump.rdb
│
├── include/
│   │
│   ├── commands/
│   │   ├── command.h
│   │   └── command_handler.h
│   │
│   ├── database/
│   │   ├── database.h
│   │   ├── redis_value.h
│   │   ├── lru.h
│   │   └── lfu.h
│   │
│   ├── persistence/
│   │   └── persistence.h
│   │
│   ├── protocol/
│   │   ├── resp_parser.h
│   │   └── resp_value.h
│   │
│   ├── Replica/
│   │   └── replication_manager.h
│   │
│   ├── election/
│   │   └── election_manager.h
│   │
│   └── server/
│       ├── server.h
│       ├── client.h
│       └── client_handler.h
│
├── src/
│   │
│   ├── commands/
│   │   └── command_handler.cpp
│   │
│   ├── database/
│   │   ├── database.cpp
│   │   ├── redis_value.cpp
│   │   ├── lru.cpp
│   │   └── lfu.cpp
│   │
│   ├── persistence/
│   │   └── persistence.cpp
│   │
│   ├── protocol/
│   │   ├── resp_parser.cpp
│   │   └── resp_value.cpp
│   │
│   ├── Replica/
│   │   └── replication_manager.cpp
│   │
│   ├── election/
│   │   └── election_manager.cpp
│   │
│   ├── server/
│   │   ├── server.cpp
│   │   ├── client.cpp
│   │   └── client_handler.cpp
│   │
│   └── main.cpp
│
└── README.md
```

---

# Building the Project

## Requirements

Linux environment with:

* GCC / G++
* C++20 support
* CMake
* POSIX socket support
* `redis-cli` for testing

For Ubuntu:

```bash
sudo apt update

sudo apt install build-essential cmake redis-tools
```

---

## Clone

```bash
git clone https://github.com/manabtikadar/Redis.git
cd Redis
```

---

## Build

Create a build directory:

```bash
mkdir -p build
cd build
```

Configure the project:

```bash
cmake ..
```

Compile:

```bash
make -j$(nproc)
```

The executable will be generated in the build directory.

---

# Starting a Server

A RediForge node is started using:

```bash
./rediforge <port> [peer_port ...]
```

For example:

```bash
./rediforge 6379
```

This starts a standalone RediForge server on:

```text
127.0.0.1:6379
```

---

# Running Multiple Nodes

RediForge supports multiple independent server processes.

For a three-node distributed setup:

### Node 1

```bash
./rediforge 6379 6380 6381
```

### Node 2

```bash
./rediforge 6380 6379 6381
```

### Node 3

```bash
./rediforge 6381 6379 6380
```

The first argument specifies the node's own port.

The remaining arguments specify the ports of its election peers.

```text
./rediforge <my_port> <peer_1> <peer_2> ...
```

For example:

```text
Node 6379
    │
    ├── Peer 6380
    └── Peer 6381

Node 6380
    │
    ├── Peer 6379
    └── Peer 6381

Node 6381
    │
    ├── Peer 6379
    └── Peer 6380
```

---

# Connecting with redis-cli

After starting the server:

```bash
redis-cli -p 6379
```

Example:

```text
127.0.0.1:6379> SET name Manab
OK

127.0.0.1:6379> GET name
"Manab"

127.0.0.1:6379> SET age 22
OK

127.0.0.1:6379> GET age
"22"
```

You can open multiple clients simultaneously:

```bash
redis-cli -p 6379
```

in multiple terminals.

---

# Supported Operations

The command system is implemented around a command-dispatch architecture.

```text
Client
   │
   ▼
RESP Parser
   │
   ▼
Command
   │
   ▼
CommandHandler
   │
   ▼
Database
```

Examples include:

```text
SET
GET
DEL
EXISTS
INCR
REPLICAOF
```

Additional data-structure commands are implemented/under development as the corresponding data types are expanded.

---

# How the Server Works

When a client connects:

```text
Client
  │
  │ TCP connection
  ▼
accept()
  │
  ▼
new client socket
  │
  ▼
std::thread
  │
  ▼
handle_client()
  │
  ▼
recv()
  │
  ▼
Input Buffer
  │
  ▼
RESP Parser
  │
  ▼
CommandHandler
  │
  ▼
Database
  │
  ▼
Response
  │
  ▼
send()
```

The important point is that the listening socket is **not** used to communicate with every client.

The listening socket is only responsible for accepting connections.

Each call to:

```cpp
accept()
```

creates a new connected socket:

```text
Listening Socket
      │
      ├── Client Socket 1
      ├── Client Socket 2
      ├── Client Socket 3
      └── Client Socket N
```

Each client can therefore be handled independently.

---

# Concurrent Client Handling

RediForge uses a thread-per-client model.

Conceptually:

```cpp
while (server_running) {

    int client_fd = accept(server_fd, ...);

    std::thread(
        handle_client,
        client_fd,
        ...
    ).detach();
}
```

This allows multiple clients to interact with the same database concurrently.

Because the database is shared between client threads, synchronization is required when accessing shared state.

---

# RESP Protocol

RediForge implements a RESP-style protocol parser inspired by Redis' wire protocol.

For example, a command:

```text
SET name Manab
```

can be represented using an array of bulk strings:

```text
*3\r\n
$3\r\n
SET\r\n
$4\r\n
name\r\n
$5\r\n
Manab\r\n
```

The parser converts the network byte stream into an internal representation:

```text
RespValue
    │
    ▼
Command
    │
    ├── name = SET
    └── arguments = [name, Manab]
```

The parser also supports incremental input.

This is important because TCP does not guarantee that one `recv()` call corresponds to exactly one application-level command.

For example:

```text
recv()
   ↓
"*3\r\n$3\r\nSET\r\n$4\r\nna"
```

may contain only part of a command.

The server keeps the incomplete data in an input buffer until the remaining bytes arrive.

---

# Database Architecture

The database stores keys and associated `RedisValue` objects.

Conceptually:

```text
unordered_map<string, RedisValue>
```

Example:

```text
"name" → RedisValue(STRING)
"age"  → RedisValue(STRING)
"list" → RedisValue(LIST)
"tags" → RedisValue(SET)
```

---

# RedisValue

Instead of storing every value as a plain string, RediForge uses a `RedisValue` abstraction.

Supported value categories include:

```cpp
enum class Type {
    STRING,
    LIST,
    HASH,
    SET,
    SORTED_SET
};
```

The object maintains the appropriate internal representation for each type.

Conceptually:

```text
RedisValue
│
├── STRING
│      └── string
│
├── LIST
│      └── list
│
├── HASH
│      └── key → value
│
├── SET
│      └── unique values
│
└── SORTED_SET
       └── value + score
```

This design allows the database to support multiple Redis-like data structures instead of being limited to simple key-value strings.

---

# LRU Eviction

LRU stands for:

**Least Recently Used**

The idea is to remove the item that has not been accessed for the longest period of time.

Example:

```text
Cache:

A
B
C
D

Access A

A becomes recently used.

LRU order:

B → C → D → A
```

If the cache becomes full and another item needs to be inserted:

```text
B → C → D → A
^
|
Least recently used
```

`B` can be evicted.

A typical LRU implementation uses:

```text
Hash Map + Doubly Linked List
```

The hash map provides fast lookup while the linked list maintains usage order.

Target complexity:

```text
GET      O(1)
UPDATE   O(1)
INSERT   O(1)
EVICT    O(1)
```

---

# LFU Eviction

LFU stands for:

**Least Frequently Used**

Instead of tracking only recency, LFU tracks how frequently a key is accessed.

Example:

```text
A → frequency 10
B → frequency 3
C → frequency 1
```

If an item needs to be evicted:

```text
C
```

is the candidate because it has the lowest access frequency.

LFU is useful when frequently accessed data should remain cached even if it has not been accessed very recently.

---

# LRU vs LFU

| Property    | LRU                 | LFU                      |
| ----------- | ------------------- | ------------------------ |
| Full form   | Least Recently Used | Least Frequently Used    |
| Tracks      | Recency             | Frequency                |
| Evicts      | Oldest unused item  | Least accessed item      |
| Useful for  | Temporal locality   | Repeated access patterns |
| Main metric | Last access         | Access count             |

RediForge contains separate implementations for these eviction strategies so that cache replacement policies can be explored independently.

---

# Persistence

RediForge contains a persistence layer for storing database state on disk.

The database can load state during server startup:

```text
Server starts
      │
      ▼
Check persistence file
      │
      ▼
Load stored database
      │
      ▼
Start accepting clients
```

The project uses:

```text
data/dump.rdb
```

as the database dump location.

If no previous database file exists, the server starts with an empty database.

> Persistence is currently a simplified educational implementation and is not intended to be fully compatible with Redis' production RDB format.

---

# Primary-Replica Replication

RediForge implements a primary-replica architecture.

```text
                 PRIMARY
                  6379
                   │
          ┌────────┴────────┐
          │                 │
          ▼                 ▼
       REPLICA            REPLICA
        6380                6381
```

The primary is responsible for accepting writes.

Replicas receive replicated operations from the primary.

---

# REPLICAOF

A node can be configured as a replica using:

```text
REPLICAOF 127.0.0.1 6379
```

The replica then connects to the primary.

The connection flow is:

```text
Replica
   │
   │ TCP connect
   ▼
Primary
   │
   │ handshake
   ▼
Replication connection established
```

The replica maintains a dedicated connection to the primary for receiving replicated data.

---

# Initial Synchronization

When a replica connects to a primary, the primary performs an initial synchronization.

Conceptually:

```text
PRIMARY
   │
   │ SYNC_START
   │
   │ SET age 22
   │ SET name Manab
   │ ...
   │
   │ SYNC_END
   ▼
REPLICA
```

Example replication stream:

```text
SYNC_START
SET age 22
SET name Manab
SYNC_END
```

The replica processes these commands and reconstructs its local database.

After synchronization completes, the replica starts receiving live updates.

---

# Live Replication

Suppose the primary receives:

```text
SET name Alice
```

The flow is:

```text
Client
  │
  ▼
PRIMARY
  │
  ├── Update local database
  │
  └── ReplicationManager
          │
          ├── Replica 6380
          └── Replica 6381
```

The replicas receive the corresponding write operation and apply it to their local databases.

Therefore:

```text
SET name Alice
```

on the primary eventually results in:

```text
GET name
```

returning:

```text
Alice
```

on the replicas.

---

# Replication Manager

The `ReplicationManager` is responsible for maintaining the set of replica connections.

Conceptually:

```text
ReplicationManager
│
├── Replica 6380
├── Replica 6381
└── ...
```

When a write occurs on the primary:

```text
Command
   │
   ▼
Database
   │
   ▼
ReplicationManager
   │
   ├── send → Replica 6380
   └── send → Replica 6381
```

This separates replication logic from the core database implementation.

---

# Replica Read-Only Mode

A replica should not independently accept writes while it is following a primary.

Therefore write commands received by a replica are rejected.

Conceptually:

```text
Replica
  │
  ├── GET  → allowed
  ├── EXISTS → allowed
  │
  ├── SET → rejected
  ├── DEL → rejected
  └── INCR → rejected
```

Example:

```text
127.0.0.1:6380> SET name Test
-READONLY You can't write to a replica
```

This prevents a replica from diverging from the primary.

---

# Primary Failure Detection

A major part of RediForge is automatic primary failure detection.

The replica continuously receives data from the primary through its replication connection.

If the primary disconnects:

```cpp
recv(primary_fd, ...)
```

can return:

```text
0
```

which indicates that the peer closed the connection.

The replica then detects:

```text
Primary disconnected
```

and starts the failover procedure.

---

# Failure Detection Flow

```text
PRIMARY
  6379
    │
    │ replication connection
    ▼
REPLICA
  6381
    │
    │ recv()
    │
    X
Primary connection lost
    │
    ▼
handle_primary_failure()
    │
    ▼
Start election
```

---

# Leader Election

RediForge implements a simplified majority-based leader election mechanism inspired by distributed consensus algorithms such as Raft.

The server roles are:

```text
PRIMARY
REPLICA
CANDIDATE
```

When the primary fails, a replica can become a candidate.

```text
REPLICA
   │
   │ primary failure
   ▼
CANDIDATE
   │
   │ request votes
   ▼
Other nodes
```

---

# Election Network

Client traffic and election traffic are kept separate.

For a node running on:

```text
6379
```

the election port is:

```text
7379
```

Similarly:

```text
6379 → 7379
6380 → 7380
6381 → 7381
```

This gives the system separate communication channels:

```text
Client / Redis traffic
6379 / 6380 / 6381

Election traffic
7379 / 7380 / 7381
```

This separation prevents election messages from being mixed with normal database commands.

---

# Election Protocol

When a replica detects primary failure:

```text
1. Start election
2. Become CANDIDATE
3. Increment term
4. Vote for itself
5. Request votes from peers
6. Receive vote responses
7. Count votes
8. Check majority
9. Become PRIMARY if majority is achieved
```

---

# Election Message Format

A candidate sends:

```text
VOTE_REQUEST <term> <candidate_id>
```

Example:

```text
VOTE_REQUEST 1 6381
```

A peer responds:

```text
VOTE_RESPONSE <term> <server_id> <vote_granted>
```

Example:

```text
VOTE_RESPONSE 1 6380 1
```

where:

```text
1 = vote granted
0 = vote rejected
```

---

# Election Example

Assume:

```text
6379 = PRIMARY
6380 = REPLICA
6381 = REPLICA
```

The primary fails:

```text
6379
  X
```

Node `6381` detects the failure.

It becomes:

```text
CANDIDATE
```

and increments its term:

```text
Term = 1
```

It votes for itself:

```text
6381 → vote for 6381
```

Then it requests votes:

```text
6381
 │
 ├── VOTE_REQUEST → 6380
 │
 └── VOTE_REQUEST → 6379
```

Since `6379` is down:

```text
6379 → no response
```

But `6380` responds:

```text
6380 → VOTE_RESPONSE 1 6380 1
```

Votes become:

```text
6381 → self vote
6380 → granted vote
```

Total:

```text
2 votes
```

For a three-node cluster:

```text
majority = 2
```

Therefore:

```text
6381 becomes PRIMARY
```

---

# Majority Voting

For `N` nodes, a majority requires:

```text
floor(N / 2) + 1
```

Examples:

```text
3 nodes → 2 votes
5 nodes → 3 votes
7 nodes → 4 votes
```

The majority requirement prevents two different candidates from normally being elected by overlapping majorities in the same term.

---

# Randomized Election Delay

To reduce the possibility of multiple replicas starting an election at exactly the same time, RediForge uses a randomized election delay.

Conceptually:

```text
Replica A → waits 200 ms
Replica B → waits 430 ms
Replica C → waits 310 ms
```

The node whose election timeout expires first starts the election.

This reduces the probability of simultaneous candidates repeatedly competing for votes.

---

# Automatic Failover

The complete failover flow is:

```text
                    PRIMARY
                      6379
                        X
                        │
                 Primary failure
                        │
                        ▼
                 Replica 6381
                        │
                        ▼
                 Detect failure
                        │
                        ▼
                    CANDIDATE
                        │
                        ▼
                 Increment term
                        │
                        ▼
                  Self vote
                        │
                        ▼
               Request peer votes
                        │
             ┌──────────┴──────────┐
             ▼                     ▼
         Peer 6380             Peer 6379
             │                     X
             │
             ▼
        Vote granted
             │
             ▼
      Majority achieved
             │
             ▼
        6381 → PRIMARY
```

This allows the cluster to continue operating even after the original primary becomes unavailable.

---

# Server State Machine

A node can move through different roles:

```text
             Primary failure
                    │
                    ▼
                REPLICA
                    │
                    ▼
               CANDIDATE
                    │
              majority vote
                    │
                    ▼
                PRIMARY
```

The role is maintained by the server and is used to determine how the node handles commands and replication.

---

# Network Architecture

RediForge uses two logical network channels.

## Client / Database Channel

```text
Client
  │
  │ TCP
  ▼
Server Port
```

Example:

```text
6379
```

Used for:

* Client commands
* GET
* SET
* DEL
* EXISTS
* INCR
* Replica connections

## Election Channel

```text
Node
 │
 │ TCP
 ▼
Election Port
```

Example:

```text
6379 → 7379
6380 → 7380
6381 → 7381
```

Used for:

* Vote requests
* Vote responses
* Election communication

---

# Design Decisions

## Why TCP?

TCP provides:

* Reliable delivery
* Ordered byte stream
* Connection-oriented communication

This is appropriate for database command processing and replication streams.

---

## Why separate election ports?

Database traffic and election traffic have different responsibilities.

Separating them makes the architecture easier to reason about:

```text
Database traffic
        ≠
Election traffic
```

It also prevents election messages from interfering with normal client commands.

---

## Why threads?

RediForge currently uses a thread-per-client architecture because it provides a simple model for concurrent client handling.

```text
Client 1 → Thread 1
Client 2 → Thread 2
Client 3 → Thread 3
...
```

The architecture can later be evolved toward:

* thread pools
* event loops
* `epoll`
* asynchronous I/O

---

## Why a Command Handler?

The command handler separates:

```text
Network layer
      ↓
Protocol layer
      ↓
Command layer
      ↓
Database layer
```

This prevents networking code from becoming tightly coupled with database operations.

---

# Current Limitations

RediForge is a learning-oriented distributed database implementation.

Some production-grade features are still under development.

Current limitations include:

* Simplified persistence format
* No full Redis RDB compatibility
* No AOF implementation
* Simplified replication protocol
* Replication offsets are not yet fully implemented
* Reconnection after primary failure is under development
* Replica reconfiguration after failover is under development
* Heartbeat mechanism is under development
* Election state persistence is under development
* Candidate log/data freshness checks are not fully implemented
* Split-brain protection requires further work
* The election algorithm is not a complete Raft implementation

Therefore, the current election mechanism should be considered a **simplified educational consensus/failover mechanism**, rather than a production-ready Raft implementation.

---

# Future Improvements

The next planned improvements include:

## 1. Automatic Replica Reconfiguration

After a failover:

```text
6379 → DEAD

6381 → NEW PRIMARY
6380 → OLD REPLICA
```

The system should automatically reconfigure:

```text
6381 PRIMARY
     │
     ▼
6380 REPLICA
```

---

## 2. Heartbeats

The primary should periodically send heartbeats:

```text
PRIMARY
   │
   ├── HEARTBEAT → Replica 6380
   └── HEARTBEAT → Replica 6381
```

This allows replicas to detect failures more explicitly.

---

## 3. Reconnection

If a temporary network failure occurs:

```text
Replica
   │
   X
Primary connection lost
   │
   ▼
Reconnect
   │
   ▼
Resume synchronization
```

---

## 4. Replication Offsets

Instead of resending the entire database after every reconnect, replication offsets can be maintained:

```text
Primary log:

1 SET A 10
2 SET B 20
3 SET C 30
4 SET D 40
```

If a replica has received up to:

```text
offset = 2
```

the primary can send only:

```text
3 SET C 30
4 SET D 40
```

This significantly reduces synchronization overhead.

---

## 5. Persistent Election State

Election terms and voting state should eventually be persisted so that a server restart does not lose important consensus information.

---

## 6. Stronger Consensus Guarantees

The current election mechanism can be extended with concepts such as:

* election timers
* leader heartbeats
* term validation
* log indices
* commit indices
* candidate log freshness
* replicated state machines
* leader fencing
* split-brain prevention

---

# Example Three-Node Deployment

Start three terminals.

### Terminal 1

```bash
./rediforge 6379 6380 6381
```

### Terminal 2

```bash
./rediforge 6380 6379 6381
```

### Terminal 3

```bash
./rediforge 6381 6379 6380
```

Make `6379` the primary and configure replicas:

```text
6380> REPLICAOF 127.0.0.1 6379
6381> REPLICAOF 127.0.0.1 6379
```

Now:

```text
                  6379
                 PRIMARY
                /       \
               /         \
            6380         6381
           REPLICA      REPLICA
```

Write data:

```text
6379> SET name Manab
OK
```

The command is replicated to the replicas.

---

# Failure Test

Terminate the primary:

```text
6379 → STOP
```

The replicas detect the failure.

One replica starts an election:

```text
6381 → CANDIDATE
```

It requests votes from peers.

If it receives a majority:

```text
6381 → PRIMARY
```

The resulting cluster becomes:

```text
6379 → DEAD

6381 → PRIMARY

6380 → REPLICA
```

This demonstrates the core automatic failover mechanism implemented in RediForge.

---

# Important Distributed-System Concepts Demonstrated

RediForge was designed to provide practical experience with several systems concepts:

### Networking

```text
TCP
Sockets
bind()
listen()
accept()
connect()
send()
recv()
```

### Concurrency

```text
std::thread
Mutexes
Shared database state
Concurrent clients
```

### Database Internals

```text
Hash maps
Data structures
Command dispatch
Cache eviction
Persistence
```

### Distributed Systems

```text
Primary-replica replication
Failure detection
Leader election
Majority voting
Terms
Failover
```

### Protocol Design

```text
RESP parsing
Command framing
Incremental network buffers
Replication messages
Election messages
```

---

# Learning Outcomes

Building RediForge provided practical experience in:

* Designing a TCP server from scratch
* Handling multiple concurrent clients
* Implementing a network protocol parser
* Designing a command dispatch architecture
* Building an in-memory database
* Implementing cache eviction policies
* Designing primary-replica replication
* Performing initial database synchronization
* Streaming live updates to replicas
* Detecting node failures
* Implementing majority-based leader election
* Designing automatic failover
* Working with C++ concurrency primitives
* Debugging distributed systems
* Understanding the challenges involved in maintaining consistency across nodes

---

# Project Status

| Component                         | Status                   |
| --------------------------------- | ------------------------ |
| TCP Server                        | ✅ Implemented            |
| Multiple Clients                  | ✅ Implemented            |
| Client Threads                    | ✅ Implemented            |
| RESP Parser                       | ✅ Implemented            |
| Command Handler                   | ✅ Implemented            |
| In-Memory Database                | ✅ Implemented            |
| RedisValue                        | ✅ Implemented            |
| String Data                       | ✅ Implemented            |
| List Data Structure               | ✅ Implemented / Expanded |
| Hash Data Structure               | ✅ Implemented / Expanded |
| Set Data Structure                | ✅ Implemented / Expanded |
| Sorted Set Representation         | ✅ Implemented / Expanded |
| LRU                               | ✅ Implemented            |
| LFU                               | ✅ Implemented            |
| Persistence                       | 🟡 Partial               |
| Primary-Replica Architecture      | ✅ Implemented            |
| `REPLICAOF`                       | ✅ Implemented            |
| Initial Synchronization           | ✅ Implemented            |
| Live Replication                  | ✅ Implemented            |
| Replica Read-Only Mode            | ✅ Implemented            |
| Failure Detection                 | ✅ Implemented            |
| Election Manager                  | ✅ Implemented            |
| Vote Requests                     | ✅ Implemented            |
| Vote Responses                    | ✅ Implemented            |
| Majority Voting                   | ✅ Implemented            |
| Randomized Election Delay         | ✅ Implemented            |
| Automatic Promotion               | ✅ Implemented            |
| Automatic Replica Reconfiguration | 🚧 Planned               |
| Heartbeats                        | 🚧 Planned               |
| Replication Offsets               | 🚧 Planned               |
| Automatic Reconnection            | 🚧 Planned               |
| Persistent Election State         | 🚧 Planned               |
| Full Raft-style Consensus         | 🚧 Planned               |

---

# Why This Project?

RediForge was built to go beyond implementing a simple CRUD application.

The project explores what happens when a database must:

1. Accept many clients concurrently.
2. Parse a network protocol.
3. Maintain shared in-memory state.
4. Replicate state to other machines/processes.
5. Detect node failures.
6. Elect a new leader.
7. Continue serving after a primary failure.

The project therefore combines **C++, operating-system networking, concurrency, database internals, and distributed systems** into a single implementation.

---

# Repository

GitHub:

https://github.com/manabtikadar/Redis

---

# Author

**Manab Tikadar**

Built using **C++ / CMake / TCP Sockets / Multithreading / Distributed Systems**.

