# HiveDB

A from-scratch relational database engine written in C++23. It implements the full classic DB engine stack — from raw disk I/O up to SQL parsing and execution — under the `hivedb` namespace.

## Architecture Overview

```
User Input (SQL text)
       ↓
   [Lexer]          → tokenizes raw SQL into tokens
       ↓
   [Parser]         → builds an AST from the tokens
       ↓
[Storage Engine]    → executes the AST (CREATE TABLE, INSERT, SELECT)
       ↓
[Buffer Pool]       → caches disk pages in memory, manages eviction
       ↓
[Disk Scheduler]    → queues async read/write requests on a background thread
       ↓
 [Disk Manager]     → raw page-level I/O to a file on disk
```

## Components

### Lexer (`src/parser/lexer.cpp`)

Turns a raw SQL string into a flat list of **tokens**. Each token has a `token_type` (e.g. `select`, `from`, `integer`, `identifier`) and a `literal` string view into the original input — no copies.

It walks the input character by character, matching reserved keywords (`SELECT`, `FROM`, `CREATE TABLE`, `INSERT INTO`, `VALUES`, `NOT NULL`), operators (`+`, `-`, `*`, `/`, `!`), identifiers, string literals, integers, and floats. Queries are terminated by `;`.

---

### Parser (`src/parser/parser.cpp`)

Takes the token list and builds an **Abstract Syntax Tree (AST)** using recursive descent. Each grammar rule is one function:

```
expr()
  └─ commaExpr()        — comma-separated lists
       └─ binaryExpr()  — +, -, *, /
            └─ unaryExpr()  — !, -
                 └─ primaryExpr()  — literals, identifiers, parentheses
```

SQL statements produce specific AST node types:

| Node | SQL |
|---|---|
| `create_tbl_expr` | `CREATE TABLE foo (col INT NOT NULL, ...)` |
| `insert_expr` | `INSERT INTO foo (col) VALUES (42)` |
| `select_expr` | `SELECT expr1, expr2 FROM table WHERE ...` |
| `binary_expr` | `a + b * 2` |
| `literal_expr<T>` | A literal value or column name reference |

Every node has an `execute()` method that evaluates it, returning a `std::variant<float, int, string_view, vector<values>>`.

---

### Data Types (`src/data_types/`)

Three column types are supported: `integer`, `real`, and `varchar`. Each knows how to **serialize** and **deserialize** its value to/from raw bytes, so values can be written into page buffers and read back out.

---

### Storage Engine (`src/storage_engine/storage_engine.cpp`)

The **executor** — it takes a parsed AST node and performs the operation:

- `createTable()` → registers a new `table` in an in-memory map
- `insertIntoTable()` → appends serialized row bytes to the table's data store
- `queryDataFromTable()` → fetches requested columns and evaluates select expressions row by row

Each `table` stores column definitions and all row data in a flat `std::vector<std::byte>`. Rows are fixed-width, determined by the column types at creation time.

---

### Disk Manager (`src/disk/disk_manager.cpp`)

The lowest layer — raw file I/O. Pages are **4096 bytes** each. The disk manager maps `page_id → offset` in a file (`page_id * PAGE_SIZE`), then does `read_page()` / `write_page()` at that offset.

A `disk_manager_mock` variant operates on an in-memory buffer instead of a real file, used by tests to avoid actual disk I/O.

---

### Disk Scheduler (`include/disk/disk_scheduler.hpp`)

Sits above the disk manager and adds **asynchronous I/O** via the producer-consumer pattern:

1. Caller calls `scheduler.schedule(disk_request{...})` — pushes the request into a thread-safe `channel<disk_request>` queue.
2. A background `std::thread` pulls requests and calls `manager.read_page()` or `manager.write_page()`.
3. Each request carries a `std::promise<bool>`. The worker signals completion via `is_done.set_value(true)`. The caller holds the corresponding `std::future<bool>` and blocks on `.wait()` when it needs the result.

---

### Buffer Pool (`include/buffer_pool/buffer_pool.hpp`)

A **page cache** that keeps a fixed number of disk pages in memory so you don't thrash the disk on every access.

Core data structures:
- `m_frames` — a vector of `frame_header`, each holding a 4096-byte page buffer
- `m_page_table` — `unordered_map<page_id, frame_id>` mapping pages to their in-memory frames
- `m_empty_frames` — list of currently unused frame slots
- `m_frame_replacer` (LRU-K) — decides which frame to evict when the pool is full

**`request_page(id)` logic:**
1. Page already in memory → record the access, return it ✅
2. Page not found (page fault) → read from disk, then:
   - Empty frame available → use it
   - No empty frame, but a dirty unpinned frame exists → flush it to disk, reuse the slot
   - Otherwise → ask LRU-K to pick a **victim**, evict it, use that slot

**Pin counts** prevent a frame from being evicted while it's in use. **Dirty flags** mark frames that have been modified and need to be written back before eviction.

---

### LRU-K Replacer (`src/buffer_pool/lru_k.cpp`)

The eviction policy for the buffer pool. Plain LRU evicts the least recently used page. LRU-K tracks the last **K accesses** per frame and evicts the frame whose K-th most recent access is furthest in the past (K=10 is configured).

This is better suited to database workloads where some pages get short-burst access without being truly "hot" long-term.

---

### B+ Tree (`include/b_plus_tree/b_plus_tree.hpp`)

A disk-backed **index** structure where keys map to values, stored across pages managed by the buffer pool. Two node types exist:

- **Inner nodes** — hold separator keys and child page IDs for navigation
- **Leaf nodes** — hold the actual key-value records

**Insert flow:**
1. Walk inner nodes to find the correct leaf
2. Insert into the leaf; if no overflow → done
3. Overflow → split the leaf into two, propagate the new separator key upward
4. If the parent inner node overflows too → split it recursively
5. If the root splits → create a new root inner node above both halves

`find(key)` traverses top-down to the correct leaf, then binary-searches within it.

> **Note:** `remove()` is not yet implemented.

---

## Build

Requires CMake 3.11+, a C++23 compiler, and `spdlog` installed system-wide. Other dependencies (`fmt`, `libassert`, `Catch2`) are fetched automatically.

```bash
cmake -B build
cmake --build build

# Run the database
./build/hive

# Run tests
./build/tests
```

## Status

The disk layer through the buffer pool is fully functional and tested. The parser and storage engine work independently. The B+ tree is implemented but not yet wired into the storage engine. The `main()` REPL currently echoes input — the lexer/parser/executor pipeline is not yet connected end-to-end.
