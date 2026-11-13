# 🗺️ CampusPath

> **Intelligent Campus Navigation Engine & Spatial Wayfinding System**  
> Powered by C++ Graph Theory, Dijkstra's Algorithm, and an Apple Vision Pro–inspired Spatial Interface.

[![C++17](https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=flat-square&logo=c%2B%2B)](https://en.cppreference.com/)
[![Dijkstra Algorithm](https://img.shields.io/badge/Algorithm-Dijkstra's%20Shortest%20Path-00f0ff?style=flat-square)](https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm)
[![Time Complexity](https://img.shields.io/badge/Complexity-O((V%20%2B%20E)%20log%20V)-00e676?style=flat-square)]()
[![Build & Tests](https://img.shields.io/badge/Tests-88%2F88%20Passing-brightgreen?style=flat-square)]()
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-blueviolet?style=flat-square)]()

---

## 📌 Project Overview

**CampusPath** is a complete, production-grade Advanced Data Structures & Algorithms (ADSA) navigation system. It models a physical university campus as a **weighted, undirected graph $G = (V, E)$** and computes the mathematically guaranteed shortest walking path between landmarks using **Dijkstra's Algorithm** with an optimized **Min-Priority Queue (Min-Heap)**.

The system is accessible through two interfaces:
1. **High-Performance Terminal CLI**: Query routes, view graphs, and benchmark pathfinding throughput directly from your shell.
2. **Apple Vision Spatial Web Interface**: A floating glassmorphic visualizer with 3D isometric landmarks, step-by-step algorithm playback, and real-time Min-Heap inspection.

---

## 🧠 Algorithmic Core (ADSA Breakdown)

### 1. Graph Model: Adjacency List
- **Space Efficiency**: Rather than an $O(V^2)$ adjacency matrix, campus walkways are stored using an **Adjacency List** via `std::unordered_map<int, std::vector<Edge>>`.
- **Space Complexity**: $O(V + E)$
- **Lookup Complexity**: Average $O(1)$ vertex lookup.

### 2. Dijkstra's Algorithm with Min-Heap
- **Min-Priority Queue**: Built on `std::priority_queue` with `std::greater`, ensuring vertex extraction in $O(\log V)$.
- **Lazy Deletion**: Outdated tentative distance pairs in the heap are skipped in $O(1)$ time whenever `currentDist > dist[u]`.
- **Early-Exit Optimization**: Search terminates the exact moment the destination vertex is extracted from the min-heap, guaranteeing optimal runtime without redundant campus traversal.
- **Parent Backtracking**: Predecessors are tracked in an `unordered_map<int, int>` to reconstruct the turn-by-turn route in $O(L)$ time.

### 3. Complexity Matrix

| Metric | Complexity | Proof & Justification |
| :--- | :--- | :--- |
| **Time Complexity** | $\mathcal{O}((V + E) \log V)$ | Each of the $V$ vertices is pushed/popped at most once from the heap ($\mathcal{O}(V \log V)$), and each of the $E$ walkways is relaxed at most once ($\mathcal{O}(E \log V)$). |
| **Space Complexity** | $\mathcal{O}(V + E)$ | Storage allocated for adjacency list, tentative distance map, and priority queue elements. |
| **Optimality** | **Guaranteed** | Dijkstra guarantees the shortest path on graphs with non-negative edge weights ($\forall e \in E, w(e) \ge 0$). |

---

## 🚀 Quick Start (Terminal & Web)

### Prerequisites
- `g++` supporting C++17
- `make`

### 1. Build Everything
```bash
# Clone the repository
git clone https://github.com/sizmosloth/CampusPath.git
cd CampusPath

# Compile both CLI and Web Server with a single command
make
```

### 2. Run in Terminal (CLI Mode)
You can query routes directly using building names or numeric IDs:

```bash
# Query route by building names
./campuspath "Main Gate" "Library"

# Or query by node IDs (0 to 9)
./campuspath 0 2

# View all campus landmarks and connectivity degrees
./campuspath --locations

# View all walkways and walking distances
./campuspath --connections

# Run the 10,000-query benchmark suite
./campuspath --benchmark

# Launch the interactive menu
./campuspath
```

#### Terminal Sample Output
```
======================================================
           OPTIMAL CAMPUS ROUTE (DIJKSTRA)            
======================================================
  Origin:      Main Gate [ID: 0]
  Destination: Library [ID: 2]
------------------------------------------------------
  Total Distance: 450 metres
  Total Stops:    3 landmarks
  Search Time:    43.15 microseconds
------------------------------------------------------
  TURN-BY-TURN WAYFINDING:

    [Start]   Main Gate (Entrance)
        |   + 250m (Walk along campus pathway)
    [Hop 1]   Admin Block (Administration)  [Cumulative: 250m]
        |   + 200m (Walk along campus pathway)
    [Arrive]  Library (Academic)  [Cumulative: 450m]

------------------------------------------------------
  ADSA COMPLEXITY & GUARANTEES:
    - Algorithm:         Dijkstra's Single-Source Shortest Path
    - Data Structure:    Adjacency List + Min-Heap Priority Queue
    - Time Complexity:   O((V + E) log V)
    - Space Complexity:  O(V + E)
    - Optimality:        Mathematically proven shortest path
======================================================
```

### 3. Run the Spatial Web Interface
Launch the C++ HTTP server to host both the REST API and the Apple Vision Spatial Web UI:

```bash
make run-server
```
Then open **[http://localhost:8080](http://localhost:8080)** in any modern browser.

---

## 🧪 Automated Unit Test Suite

The project includes unit tests for graph integrity, edge-case routing, priority queue correctness, and CSV dataset parsing:

```bash
make test
```

```
========================================
  CampusPath - Graph Unit Tests
  Test Summary: 32 Passed, 0 Failed
========================================
  CampusPath - Dijkstra Unit Tests
  Test Summary: 29 Passed, 0 Failed
========================================
  CampusPath - CampusMap Unit Tests
  Test Summary: 27 Passed, 0 Failed
========================================
  Overall Result: 88 Passed, 0 Failed (100% Green)
```

---

## 📁 Project Architecture

```
CampusPath/
├── Makefile                     # Root build coordinator (make, make test, make server)
├── campuspath                   # Root executable launcher for instant CLI queries
├── README.md                    # Project documentation
│
├── backend/
│   ├── Makefile                 # C++ compilation with platform-aware LDFLAGS
│   ├── data/
│   │   ├── nodes.csv            # 10 official campus landmarks (|V| = 10)
│   │   └── edges.csv            # 16 pedestrian walkway distances (|E| = 16)
│   ├── include/
│   │   ├── Common.hpp           # Node, Edge, and PathResult data definitions
│   │   ├── Graph.hpp            # Adjacency list graph declaration
│   │   ├── Dijkstra.hpp         # Min-Heap Dijkstra engine declaration
│   │   ├── CampusMap.hpp        # Transactional CSV loader & landmark index
│   │   ├── httplib.h            # Single-header C++ HTTP server
│   │   └── json.hpp             # Modern JSON for C++
│   ├── src/
│   │   ├── Graph.cpp            # Graph operations & neighbor query implementations
│   │   ├── Dijkstra.cpp         # Dijkstra algorithm with lazy deletion & early exit
│   │   ├── CampusMap.cpp        # CSV parsing, name resolution & path translation
│   │   ├── main.cpp             # Interactive CLI & benchmark engine
│   │   └── server.cpp           # REST API & static web host (/api/shortest-path)
│   └── tests/
│       ├── test_graph.cpp       # Unit tests: graph validation & bidirectional edges
│       ├── test_dijkstra.cpp    # Unit tests: shortest path, loops, disconnected nodes
│       └── test_campus_map.cpp  # Unit tests: CSV validation & end-to-end integration
│
└── frontend/
    ├── index.html               # VisionOS spatial dashboard & terminal panel
    ├── style.css                # Apple Vision Pro spatial glassmorphism & 3D styling
    ├── map.js                   # 3D SVG schematic graph renderer & Dijkstra wave player
    └── script.js                # Frontend controller & C++ backend REST client
```

---

## 🌐 REST API Specification

The C++ server exposes the following JSON endpoints:

| Endpoint | Method | Description |
| :--- | :--- | :--- |
| `/api/locations` | `GET` | Returns all 10 registered campus landmarks with degrees and categories. |
| `/api/connections` | `GET` | Returns all 16 unique undirected walkways with edge distances. |
| `/api/shortest-path` | `POST` | Computes optimal route between `source` and `destination` IDs. |

#### Request Payload
```json
{
  "source": 0,
  "destination": 2
}
```

#### Response Payload
```json
{
  "success": true,
  "distance": 450,
  "route": [
    { "id": 0, "name": "Main Gate", "category": "Entrance", "step_distance": 0, "cumulative_distance": 0 },
    { "id": 7, "name": "Admin Block", "category": "Administration", "step_distance": 250, "cumulative_distance": 250 },
    { "id": 2, "name": "Library", "category": "Academic", "step_distance": 200, "cumulative_distance": 450 }
  ],
  "telemetry": {
    "algorithm": "Dijkstra (Min-Heap Priority Queue)",
    "execution_time_us": 43.15,
    "time_complexity": "O((V + E) log V)",
    "space_complexity": "O(V + E)",
    "relaxations_count": 5,
    "visited_order": [0, 8, 7, 5, 2]
  }
}
```

---

## ⚡ Performance Benchmarks

Executed on an Intel x86_64 host running 10,000 random source-to-destination queries:

- **Queries Executed**: `10,000`
- **Total Wall Time**: `~341 ms`
- **Average Latency per Query**: `~34.1 microseconds (0.034 ms)`
- **Throughput**: `~29,250 queries/second`
- **Memory Footprint**: `< 4.2 MB RSS`

---

## 🎓 Academic Verification
- **Course**: Advanced Data Structures & Algorithms (ADSA)
- **Primary Data Structure**: Weighted Undirected Graph (Adjacency List)
- **Primary Algorithm**: Dijkstra's Algorithm with Min-Priority Queue
- **Verification**: 88 Automated Unit Tests + High-Precision Benchmarking
