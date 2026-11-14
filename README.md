# 🗺️ CampusPath

A clean, high-performance university campus navigation engine built in C++. It models the campus as an undirected weighted graph and calculates mathematically optimal walking routes using Dijkstra's algorithm with a min-heap priority queue.

Includes both a **Terminal CLI** (supporting direct and multi-stop waypoints) and a **clean, Apple-style Web Visualizer with an interactive in-browser CLI**.

---

## ⚡ Quick Start

### 1. Build
```bash
make
```

### 2. Run in Terminal (CLI)
```bash
# Direct shortest route between two landmarks
./campuspath "Main Gate" "Library"

# Multi-waypoint route with an intermediate stop (e.g. stop at Stationery first)
./campuspath "Main Gate" "Stationery" "Library"

# Or use numeric landmark IDs (0 to 10)
./campuspath 0 10 2

# Run 10,000-query Dijkstra benchmark
./campuspath --benchmark

# List all 11 campus landmarks
./campuspath --locations
```

### 3. Open Web Visualizer
```bash
make run-server
```
Open **[http://localhost:8080](http://localhost:8080)** in your browser:
- **Campus Map**: Visual Dijkstra routing with interactive pan/zoom, step-by-step playback, and multi-stop waypoints.
- **Terminal CLI**: Fully interactive shell in the browser — run `route "Main Gate" "Stationery" "Library"`, `locations`, `benchmark`, etc.

### 4. Run Unit Tests
```bash
make test
```
Executes all **89 automated unit tests** across Graph, Dijkstra, and CampusMap data loaders.

---

## 🧠 Algorithm & Complexity

- **Algorithm**: Dijkstra's Single-Source Shortest Path (with multi-waypoint sub-path decomposition)
- **Data Structure**: Graph Adjacency List (`std::unordered_map<int, std::vector<Edge>>`) + Min-Heap Priority Queue (`std::priority_queue`)
- **Time Complexity**: $\mathcal{O}((V + E) \log V)$
- **Space Complexity**: $\mathcal{O}(V + E)$
- **Optimality**: Guaranteed global minimum walking distance with zero redundant backtracking
- **Dataset**: 11 official campus landmarks ($|V| = 11$) connected by 19 pedestrian walkways ($|E| = 19$).

---

## 📂 Project Structure

```
CampusPath/
├── Makefile                # Build coordinator (make, make test, make run-server)
├── campuspath              # CLI launcher executable
├── README.md               # Documentation
│
├── backend/
│   ├── data/               # Campus dataset (nodes.csv, edges.csv)
│   ├── include/            # C++ headers (Graph.hpp, Dijkstra.hpp, CampusMap.hpp)
│   ├── src/                # Core C++ engine (main.cpp, server.cpp)
│   └── tests/              # 89 automated unit tests
│
└── frontend/
    ├── index.html          # Clean Apple-style navigation interface & terminal
    ├── style.css           # Lightweight SF Pro / Inter design system
    ├── map.js              # Hardware-accelerated SVG campus schematic
    └── script.js           # REST API client & interactive CLI interpreter
```

---

## 🌐 API Endpoints

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/locations` | Returns all 11 campus landmarks and categories |
| `GET` | `/api/connections` | Returns all 19 walkways and distances |
| `POST` | `/api/shortest-path` | Computes optimal route between `source` and `destination` (with optional `via` stop) |
| `GET` | `/api/shortest-path?from=0&to=2&via=10` | Query-string route lookup |
