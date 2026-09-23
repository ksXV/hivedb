# HiveDB

A disk-backed relational database engine written in C++23.

## Features

- **Interactive REPL**:
  - `hive> ` prompt with continuation `   -> ` for multiline input.
  - Semicolon-delimited multi-statement queries.
  - Dynamic ASCII table result formatting.
  - Dot-commands: `.help`, `.exit`, `.quit`.
  - Persistent mode (`./hive <db_path>`) and ephemeral in-memory mode (`./hive`).

- **SQL & Query Engine**:
  - `CREATE TABLE`: column types, optional `NOT NULL`.
  - `INSERT INTO`: column-specified row insertion.
  - `SELECT`: projections, wildcard `*`, scalar calculations (`SELECT 1 + 2 * 3;`), and `WHERE` filtering.
  - Expressions: arithmetic (`+`, `-`, `*`, `/`), unary (`-`, `!`), and comparisons.
  - String literals: single quotes (`'text'`) or double quotes (`"text"`).

- **Data Types**:
  - `INTEGER` / `INT`: 4-byte signed integer.
  - `REAL` / `FLOAT`: 4-byte floating point.
  - `VARCHAR` / `STRING` / `TEXT`: variable-length strings.

- **Storage Engine & Catalog**:
  - **Catalog (Page 0)**: persists multi-table schemas, column offsets, row sizes, root page IDs, and row counters.
  - **B+ Tree Indexing**: tables store rows in a disk-backed B+ tree indexed by 64-bit row IDs. Supports root splits, node splits, and leaf scans.

- **Buffer Pool**:
  - Fixed-size in-memory frame cache over 4096-byte disk pages.
  - **LRU-K** page eviction policy (default $K=10$).
  - Pinning mechanism and dirty page tracking for safe eviction.

- **Disk Layer**:
  - **Disk Scheduler**: background worker thread processing an asynchronous request queue.
  - **Disk Manager**: 4KB page read, write, and dynamic file allocation with page recycling.

## Build

Prerequisites: C++23 compiler, CMake 3.11+, Ninja, and `spdlog`.

```bash
cmake -B build-ninja -G Ninja
ninja -C build-ninja hive tests
```

## Usage

```bash
# Run with a database file
./build-ninja/hive mydb.db

# Run ephemeral in-memory database
./build-ninja/hive
```

### Example Session

```sql
hive> CREATE TABLE users (id INT NOT NULL, name TEXT, balance REAL);
Table created successfully.

hive> INSERT INTO users (id, name, balance) VALUES (1, 'Alice', 150.50);
1 row inserted.

hive> INSERT INTO users (id, name, balance) VALUES (2, 'Bob', 89.25);
1 row inserted.

hive> SELECT id, name, balance * 1.1 FROM users WHERE id == 1;
+----+-------+--------------------+
| id | name  | (balance * 1.1000) |
+----+-------+--------------------+
| 1  | Alice | 165.55             |
+----+-------+--------------------+

hive> .exit
Bye!
```

## Tests

```bash
# Run all tests
./build-ninja/tests

# Run by tag
./build-ninja/tests "[storage_engine]"
./build-ninja/tests "[parser]"
./build-ninja/tests "[b_plus_tree]"
./build-ninja/tests "[buffer_pool]"
./build-ninja/tests "[disk_manager]"
```
