# myDB — Relational Database Management System

A high-performance relational database built entirely from scratch in **C++17**, designed to demonstrate a deep understanding of database internals: storage engines, indexing, query planning, and execution.

The project includes a full SQL parser, cost-based query planner, B+ tree indexes, and a modular execution engine, all implemented without external database libraries.

---

## 🎯 Project Overview

**myDB** is a complete educational RDBMS implementation focused on correctness, performance awareness, and clean system design.

It supports SQL-like queries through an interactive CLI and features a multi-stage query pipeline:

```
SQL → Lexer → Parser → AST → Validator → Planner → Optimizer → Executor
```

The system is disk-backed, page-oriented, and optimized for indexed workloads.

---

## ✨ Key Features

### Core Database Engine

* **Custom storage engine**

  * Page-based layout (4KB pages)
  * Explicit block allocation and free-space tracking
* **B+ tree indexing**

  * Template-based implementation
  * Supports 4, 8, 16, and 32-byte keys
  * Leaf linking for efficient range scans
* **Buffer pool**

  * LRU eviction policy
  * Dirty page tracking
  * Configurable size (default: 300K pages ≈ 1.2GB)
* **Record management**

  * Variable-length records
  * Efficient serialization and column extraction

---

### Query Processing

#### SQL Parser

* Hand-written lexer and recursive descent parser
* Supported statements:

  * `CREATE TABLE`
  * `CREATE INDEX`
  * `INSERT`
  * `SELECT`
  * `UPDATE`
  * `DELETE`
* Custom commands:

  * `LOAD`
  * `SHOW`
  * `RUN`
* Features:

  * Complex `WHERE` expressions
  * `AND` / `OR` operator precedence
  * Multi-table joins

---

#### Query Optimizer

* Multi-stage optimization pipeline:

  * AST → logical query plan
  * Logical → physical operator tree
* **Cost-based decisions**

  * Index scan vs. full table scan
* **Predicate pushdown**

  * Filters applied as close to data as possible
* **Join optimization**

  * Index nested loop joins when indexes exist
* Predicate classification:

  * Scan predicates
  * Filter predicates
  * Join predicates

---

#### Execution Engine

* Volcano-style iterator model
* Operators:

  * **Scan** — full table or index scan
  * **Filter** — predicate evaluation
  * **Join** — index nested loop join
* Unified **cursor abstraction** for:

  * Sequential scans
  * Index traversal
  * Range queries

---

### Data Types & Indexing

* Supported column types:

  * `INTEGER`
  * `CHAR(4 | 8 | 16 | 32)`
  * `TEXT`
  * `BOOL`
* Index support:

  * Primary key indexes
  * Secondary indexes on fixed-size columns
* Index-aware query planning

---

### CLI & Utilities

* Interactive REPL shell
* CSV bulk loading
* SQL script execution
* Schema inspection
* Python test-data generator

  * Users
  * Products
  * Orders

---

## 🏗️ Architecture

### Storage Layer (`src/storage/`)

* `file.*` — low-level file I/O and block allocation
* `record.*` — record layout and serialization
* `cache.*` — page cache and eviction logic

---

### Core Engine (`src/core/`)

* `database.*` — top-level database interface
* `table.*` — schema metadata and index mapping
* `btree.*` — B+ tree implementation
* `cursor.*` — unified iteration interface
* `cache.*` — buffer pool and WAL thread

---

### Query Engine (`src/query/`)

* `lexer.*` — SQL tokenization
* `parser.*` — syntax analysis
* `ast.hpp` — AST definitions
* `validator.*` — semantic validation
* `executor.*` — query dispatch
* `plan/planner.*` — logical planning
* `plan/builder.*` — physical plan construction

---

### Relational Operators (`src/relational/`)

* `operations.hpp` — operator base interfaces
* `scan.cpp` — table and index scans
* `filter.cpp` — predicate evaluation
* `join.cpp` — index nested loop joins
* `key.hpp` — index key representation

---

### CLI (`src/cli/`)

* `input.*` — REPL loop and command handling
* `output.hpp` — formatted query output

---

### Configuration (`src/config.h`)

* Page size constants
* Cache limits
* B+ tree fanout values
* Common type aliases

---

## 🔧 Build System

Uses a Makefile with multiple build configurations:

```bash
# Development build (default)
make

# Debug build with sanitizers
make DEBUG=1

# Production build
make PROD=1

# Run the database
make run

# Clean artifacts
make clean
```

### Build Modes

| Mode  | Flags                                 | Purpose        |
| ----- | ------------------------------------- | -------------- |
| DEV   | `-g -O0`                              | Fast iteration |
| DEBUG | `-g -O1 -fsanitize=address,undefined` | Memory safety  |
| PROD  | `-O3 -DNDEBUG`                        | Benchmarking   |

---

## 📊 Technical Highlights

### B+ Tree Design

* Disk-oriented layout
* Nodes fit exactly into 4KB pages
* Key size–dependent fanout:

| Key Size | Keys per Node |
| -------: | ------------- |
|  4 bytes | 339           |
|  8 bytes | 254           |
| 16 bytes | 169           |
| 32 bytes | 101           |

* Leaf sibling pointers enable fast range scans
* Split and merge logic fully implemented

---

### Query Optimization Pipeline

1. Parse SQL into AST
2. Validate schema and types
3. Generate logical plan
4. Classify predicates
5. Select access paths (index vs scan)
6. Build physical operator tree
7. Execute using iterators

---

### Cache Management

* O(1) LRU promotion and eviction
* Dirty page tracking
* Background WAL thread
* Supports very large memory footprints (1GB+)

---

## 🚀 Usage Examples

### Creating Tables and Indexes

```sql
create table users (uid=int, name=char32, state=char8, age=int);
create table orders (oid=int, uid=int, pid=int, total=int);

create index on orders (uid);
create index on orders (pid);
```

---

### Loading Data

```sql
load "users.csv" into users;
load "products.csv" into products;
```

---

### Queries

```sql
select users where age > 25;

select users, orders
where users.uid == orders.uid
  and users.state == "CA";

select users, orders, products
where users.uid == orders.uid
  and orders.pid == products.pid;
```

---

### Updates and Deletes

```sql
update users set age = 30 where uid == 42;
delete from orders where total < 100;
```

---

### Scripts

```sql
run "store.sql";
```

---

## 🎯 Key Challenges & Solutions

### B+ Tree Deletion

* Careful handling of underflow
* Borrowing and merging logic
* Extensive stress testing with large datasets

---

### Query Optimizer Without Statistics

* Predicate bucketing system
* Index-aware planning
* Greedy heuristics effective for common workloads

---

### Memory Management

* RAII throughout the system
* `std::unique_ptr` for operator trees
* Strict page ownership rules
* Validated with AddressSanitizer

---

### Join Performance

* Index nested loop joins
* Outer tuple values dynamically pushed into inner index scans
* Cursor abstraction enables seamless switching

---

### Parser Robustness

* Recursive descent + Pratt parsing
* Explicit operator precedence handling
* Clean AST ownership model

---

## 📈 Performance Notes

While WAL transaction handling is not fully implemented, read-query performance has been tested.

### Observations

* **Point lookups (hot cache)**: ~2–3× faster than SQLite
* **Joins**: ~1–1.5× faster depending on selectivity
* **Full table scans**: ~5× slower due to virtual operator abstraction and reduced CPU cache locality

---

### Strengths

* Indexed lookups
* Simple read queries
* Lightweight execution paths

### Weaknesses

* Full scans
* Limited optimizer statistics
* Single-threaded execution
* No compression

---

## 🔮 Future Features

### High Priority

* Transaction support (ACID)
* Commit / rollback integration with WAL
* Improved predicate evaluation
* Additional join algorithms

### Medium Priority

* Aggregations and GROUP BY
* Subqueries
* Views
* NULL semantics
* Statistics collection
* Result caching

### Low Priority

* MVCC
* Network protocol
* Expanded SQL compliance
* Stored procedures
* Replication

---

## 📝 Code Statistics

* ~5000+ lines of C++
* 10+ major subsystems
* 8 supported query types
* 3 relational operators
* 4 B+ tree key variants

---

## 🛠️ Development Notes

### Design Principles

1. Clear separation between layers
2. Minimal abstraction overhead in hot paths
3. Modern C++17 usage
4. Disk-oriented design
5. Emphasis on correctness first, performance second

### Code Style

* Consistent naming conventions
* Header / implementation separation
* Forward declarations to reduce coupling
* Detailed comments in complex algorithms

---


**Built with**: C++17, GNU Make, Python
**Platform**: Linux (POSIX APIs, mmap, pread/pwrite)
**Author**: Vlad
