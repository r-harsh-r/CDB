# CDB - Persistent Key-Value Store

CDB is a lightweight, disk-based Key-Value store implemented in C++. It uses a B+ Tree data structure to efficiently manage data on disk, providing persistence and efficient retrieval/insertion operations.

## Features

- **Disk-Based Persistence**: Data is stored in a binary file, ensuring it survives program restarts.
- **B+ Tree Indexing**: Utilizes a B+ Tree structure for logarithmic time complexity in searches and insertions.
- **Page-Based Storage**: Manages data in fixed-size pages (4KB), optimizing for disk I/O.
- **Copy-on-Write (CoW) semantics**: Updates involve allocating new pages and updating parent pointers, ensuring data consistency (mostly).
- **Variable Key/Value Support**: Handles string keys and values.

## Project Structure

```
CDB/
├── ds/
│   └── ds.h        # Core B+ Tree and DiskFile implementation
├── src/
│   └── main.cpp    # Entry point and test cases
├── DB/
│   └── test_db     # (Generated) Database file
└── TODO            # Project roadmap
```

## Getting Started

### Prerequisites

- C++ Compiler (g++ recommended)
- Linux environment (uses POSIX file I/O: `open`, `read`, `write`, `lseek`)

### Build and Run

1. Navigate to the `src` directory:
   ```bash
   cd src
   ```

2. Compile the project:
   ```bash
   g++ main.cpp -o cdb
   ```

3. Run the executable:
   ```bash
   ./cdb
   ```

## Usage

The current `main.cpp` contains test scenarios that demonstrate the functionality:

1.  **Basic Insertion**: Inserts a few keys and prints the tree.
2.  **Leaf Split**: Inserts enough keys to trigger a leaf node split.
3.  **Stress Test**: Inserts a larger number of keys to trigger root splits and increase tree execution.

To use the library in your own code:

```cpp
#include "../ds/ds.h"

int main() {
    DiskFile db("my_database.db");
    
    // Insert
    string key = "user:101";
    string val = "{name: 'Alice'}";
    db.insert(key, val);
    
    // Print structure (debug)
    db.printTree();
    
    return 0;
}
```

## Implementation Details

- **Node Types**:
    - `BNODE_LEAF`: Stores actual Key-Value pairs.
    - `BNODE_NODE`: Internal nodes storing Keys and Child Pointers.
- **Page Layout**:
    - Pages are 4096 bytes.
    - Header page (Page 0) stores metadata (page size, free list head, root page).
- **Split Logic**:
    - Handles overflow by splitting nodes.
    - Recursively promotes keys to parent nodes.
    - Handles root splitting by creating a new root.

## Roadmap

- [x] SET - insert(key,value)
- [ ] Recursive GET (Retrieval)
- [ ] Delete operations
