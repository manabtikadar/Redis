RediForge/
│
├── CMakeLists.txt
├── README.md
├── .gitignore
│
├── include/
│   │
│   ├── server/
│   │   ├── server.h
│   │   ├── client_connection.h
│   │   └── connection_manager.h
│   │
│   ├── protocol/
│   │   ├── resp_parser.h
│   │   ├── resp_encoder.h
│   │   └── resp_value.h
│   │
│   ├── commands/
│   │   ├── command_handler.h
│   │   ├── command.h
│   │   ├── ping_command.h
│   │   ├── get_command.h
│   │   ├── set_command.h
│   │   └── del_command.h
│   │
│   ├── database/
│   │   ├── database.h
│   │   ├── value.h
│   │   └── key_value_store.h
│   │
│   ├── concurrency/
│   │   ├── thread_pool.h
│   │   └── mutex_utils.h
│   │
│   ├── persistence/
│   │   ├── snapshot.h
│   │   └── aof.h
│   │
│   └── utils/
│       ├── logger.h
│       └── config.h
│
├── src/
│   │
│   ├── main.cpp
│   │
│   ├── server/
│   │   ├── server.cpp
│   │   ├── client_connection.cpp
│   │   └── connection_manager.cpp
│   │
│   ├── protocol/
│   │   ├── resp_parser.cpp
│   │   ├── resp_encoder.cpp
│   │   └── resp_value.cpp
│   │
│   ├── commands/
│   │   ├── command_handler.cpp
│   │   ├── ping_command.cpp
│   │   ├── get_command.cpp
│   │   ├── set_command.cpp
│   │   └── del_command.cpp
│   │
│   ├── database/
│   │   ├── database.cpp
│   │   ├── value.cpp
│   │   └── key_value_store.cpp
│   │
│   ├── concurrency/
│   │   └── thread_pool.cpp
│   │
│   ├── persistence/
│   │   ├── snapshot.cpp
│   │   └── aof.cpp
│   │
│   └── utils/
│       ├── logger.cpp
│       └── config.cpp
│
├── tests/
│   ├── server/
│   ├── protocol/
│   ├── commands/
│   └── database/
│
├── config/
│   └── rediforge.conf
│
├── data/
│   ├── dump.rdb
│   └── appendonly.aof
│
└── docs/
    ├── architecture.md
    ├── protocol.md
    └── commands.md


✅ TCP server
✅ Multiple clients
✅ Thread-per-client
✅ Shared database
✅ Mutex protection
✅ RESP parser
✅ Command registry
✅ Strings
   SET GET DEL
✅ Lists
   LPUSH RPUSH LPOP RPOP LRANGE
✅ Hashes
   HSET HGET HDEL HEXISTS HGETALL

                    ↓ NEXT

⏭️ Expiration / TTL
   EXPIRE
   TTL
   SET ... EX

⏭️ Sets
   SADD
   SREM
   SISMEMBER
   SMEMBERS

⏭️ Sorted Sets
   ZADD
   ZRANGE
   ZREM

⏭️ Persistence
   RDB
   AOF

⏭️ Transactions
   MULTI
   EXEC
   DISCARD

⏭️ Pub/Sub
   SUBSCRIBE
   PUBLISH

⏭️ Server improvements
   connection limits
   graceful shutdown
   thread pool
   logging

⏭️ Performance
   benchmarking
   lock optimization
   pipelining