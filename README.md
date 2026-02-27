# CDB - Persistent Key-Value Store

CDB is a lightweight, disk-based Key-Value store implemented in C++. It uses a B+ Tree data structure to efficiently manage data on disk, providing persistence and efficient retrieval/insertion operations.

## Features

- **Disk-Based Persistence**: Data is stored in a seperate binary file on disk
- **B+ Tree Indexing**: Utilizes a B+ Tree structure for logarithmic time complexity in searches and insertions.
- **Page-Based Storage**: The file is divided into chunks of 4kb sections(Pages), each page holds one node of B+ tree
- **Copy-on-Write (CoW) semantics**: Updating either by inserting new KV pair or updating existing is done by allocating new pages, from leaf to root, existing tree is left untouched.


## Getting Started

### Prerequisites

- C++ Compiler (g++ recommended)
- Linux environment (uses POSIX file I/O: `open`, `read`, `write`, `lseek`)
- make

### Build and Run

1. Navigate to the `src` directory:
   ```bash
   cd src
   ```

2. Compile the project:
   ```bash
   make
   ```

3. Run the executable:
   ```bash
   ./CDB
   ```

## Usage

The current `main.cpp` contains test scenarios that demonstrate the functionality:

1.  **Insertion**: Inserts a few keys and prints the tree.
```bash
SET <key><SPACE><value>
```
2.  **Retrieval**: Retrieve existing kv pair by providing the key
```bash
GET <key>
```



## Roadmap

- [x] SET - insert(key,value)
- [x] Recursive GET (Retrieval)
- [ ] Delete operations
