# Smart Insurance Claim Processing & Risk Assessment System

A terminal-based C++ application that demonstrates the practical use of **Data Structures and Algorithms (DSA)** in an insurance management environment. The system provides policy management, priority-based claim processing, fraud network analysis, and efficient search operations through an interactive Text User Interface (TUI) powered by **FTXUI**.

---

## 📌 Overview

Insurance companies need to efficiently manage large policy databases, process claims based on urgency, verify policyholder credentials instantly, and identify fraudulent claim networks.

This project solves these challenges by combining multiple data structures and algorithms into a single real-time dashboard.

### Core Features

* ⚡ **Instant Policy Verification** using Hash Tables
* 🌳 **Sorted Policy Management** using AVL Trees
* 📊 **Priority-Based Claim Processing** using Sorting Algorithms + Queue
* ↩️ **Undo & Rollback System** using Stack
* 🕸️ **Fraud Ring Detection** using Graph Traversal (BFS & DFS)
* 🔍 **Linear & Binary Search Operations**
* 🖥️ **Interactive Multi-Tab TUI Dashboard** using FTXUI

---

## 🎯 Objectives

### 1. Rapid Policy Verification

* Hash Table implementation
* Average lookup complexity: **O(1)**

### 2. Sorted Policy Database

* AVL Tree maintains balanced policy records
* Search and insertion complexity: **O(log n)**

### 3. Priority Claim Ranking

Implemented:

* Bubble Sort
* Selection Sort
* Insertion Sort
* Merge Sort
* Quick Sort

Merge Sort is used in production claim processing due to its stability and guaranteed performance.

### 4. Pipeline Management

* FIFO Queue for claim processing
* LIFO Stack for undo functionality

### 5. Fraud Network Analysis

* Adjacency List Graph
* BFS Traversal
* DFS Traversal

### 6. Search Operations

* Linear Search (partial name matching)
* Binary Search (exact claim ID lookup)

### 7. Interactive Dashboard

Visualizes all DSA components in real time through six dedicated tabs.

---

# 🏗️ System Architecture

```text
┌─────────────────────────┐
│ Insurance Policies      │
└─────────────┬───────────┘
              │
              ▼
      Hash Table (O(1))
              │
              ▼
        AVL Tree (O(log n))
              │
              ▼
      Claims Management
              │
    ┌─────────┴─────────┐
    ▼                   ▼
Merge Sort          Queue (FIFO)
    │                   │
    ▼                   ▼
Priority Order    Claim Processing
                        │
                        ▼
                  Stack (Undo)
                        │
                        ▼
               Fraud Analysis Graph
                        │
            ┌───────────┴───────────┐
            ▼                       ▼
          BFS                     DFS
```

---

# 📚 Data Structures Used

## Hash Table

Used for instant policy verification.

### Benefits

* Average lookup time: **O(1)**
* Eliminates linear scanning
* Direct access by Policy ID

---

## AVL Tree

Maintains policies in sorted order.

### Features

* Self-balancing Binary Search Tree
* Guarantees **O(log n)** operations
* Supports ordered traversal

### Rotations

* LL Rotation
* RR Rotation
* LR Rotation
* RL Rotation

---

## Queue (FIFO)

Used for claim processing.

### Workflow

1. Collect pending claims
2. Sort by priority using Merge Sort
3. Enqueue claims
4. Process from front of queue

---

## Stack (LIFO)

Used for rollback functionality.

### Workflow

* Store complete claim snapshots before modification
* Undo restores previous claim state
* Queue automatically rebuilds after rollback

---

## Graph

Used for fraud network analysis.

### Representation

```text
1001 ↔ 1004 ↔ 1007
```

### Traversal Methods

#### BFS

Explores neighbors level-by-level.

Example:

```text
1001 → 1004 → 1007
```

#### DFS

Explores paths deeply before backtracking.

Example:

```text
1001 → 1004 → 1007
```

---

# 🔄 Sorting Algorithms

All algorithms rank claims by descending priority.

| Algorithm      | Strategy          | Time Complexity | Space Complexity | Purpose     |
| -------------- | ----------------- | --------------- | ---------------- | ----------- |
| Bubble Sort    | Adjacent Swaps    | O(n²)           | O(1)             | Educational |
| Selection Sort | Min/Max Selection | O(n²)           | O(1)             | Educational |
| Insertion Sort | Shift & Insert    | O(n²)           | O(1)             | Educational |
| Merge Sort     | Divide & Conquer  | O(n log n)      | O(n)             | Production  |
| Quick Sort     | Pivot Partition   | O(n²) Worst     | O(log n)         | Alternative |

---

# 🔍 Searching Algorithms

## Linear Search

Used for partial policyholder name matching.

```cpp
O(n)
```

### Advantages

* No preprocessing required
* Supports substring matching

---

## Binary Search

Used for exact Claim ID lookup.

```cpp
O(log n)
```

### Advantages

* Extremely fast lookups
* Operates on pre-sorted claim data

---

# 📈 Complexity Analysis

| Operation        | Data Structure / Algorithm | Time Complexity | Space Complexity |
| ---------------- | -------------------------- | --------------- | ---------------- |
| Policy Lookup    | Hash Table                 | O(1) Average    | O(n)             |
| Policy Lookup    | AVL Tree                   | O(log n)        | O(n)             |
| Policy Insertion | AVL Tree                   | O(log n)        | O(n)             |
| AVL Traversal    | AVL Tree                   | O(n)            | O(n)             |
| Priority Sorting | Merge Sort                 | O(n log n)      | O(n)             |
| Priority Sorting | Quick Sort                 | O(n log n) Avg  | O(log n)         |
| Priority Sorting | Bubble/Selection/Insertion | O(n²)           | O(1)             |
| Queue Operations | FIFO Queue                 | O(1)            | O(n)             |
| Stack Operations | LIFO Stack                 | O(1)            | O(n)             |
| Name Search      | Linear Search              | O(n)            | O(k)             |
| ID Search        | Binary Search              | O(log n)        | O(1)             |
| Fraud Traversal  | BFS / DFS                  | O(V + E)        | O(V)             |

---

# 🖥️ Dashboard Tabs

## Tab 0 – Dashboard Overview

Displays:

* KPI Cards
* Top Pending Claims
* Processing Queue
* Undo Stack Depth

---

## Tab 1 – Policy Database (AVL Tree)

Displays:

* All policies
* Sorted AVL traversal
* Risk-based color coding

---

## Tab 2 – Claims Sorting

Displays:

* Claim priority rankings
* Sorting algorithm comparison
* Interactive algorithm switching

---

## Tab 3 – Queue & Stack Operations

Displays:

* FIFO processing queue
* Undo stack state

---

## Tab 4 – Fraud Network Analysis

Displays:

* Fraud adjacency graph
* BFS results
* DFS results

---

## Tab 5 – Search Operations

Displays:

* Linear Search panel
* Binary Search panel

---

# 📊 Results

### Hash Table Efficiency

Policy verification consistently operates in **O(1)** average time.

### AVL Tree Balance

Sequential insertion of 20 policies maintains tree height at approximately **5**, avoiding BST degeneration.

### Priority Queue Accuracy

Merge Sort correctly ranks claims while preserving stability among equal-priority entries.

### Undo Reliability

Rollback restores both claim state and queue position successfully.

### Fraud Detection

BFS correctly identifies connected fraud rings.

### Search Performance

Binary Search significantly reduces comparisons compared to Linear Search.

---

# 🚀 Future Improvements

Potential enhancements include:

* Min Heap for dynamic priority insertion
* Trie for prefix-based customer searches
* B+ Tree for persistent storage
* Database integration
* Real-time claim analytics
* Multi-user support

---

# 🛠️ Tech Stack

* **Language:** C++
* **UI Framework:** FTXUI
* **Data Structures:** Hash Table, AVL Tree, Queue, Stack, Graph
* **Algorithms:** Merge Sort, Quick Sort, Bubble Sort, Selection Sort, Insertion Sort, Binary Search, BFS, DFS

---

# 📂 Project Structure

```text
.
├── main.cpp
├── dsa_logic.hpp
└── README.md
```

---
