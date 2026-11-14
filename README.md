# 🗺️ CampusPath

A clean, high-performance university campus navigation engine built in C++. It models the campus as an undirected weighted graph and calculates the optimal walking route using Dijkstra's algorithm with a min-heap priority queue.

Includes both a **Terminal CLI** and a **clean, lightweight Web Visualizer**.

---

## ⚡ Quick Start

### 1. Build
```bash
make
```

### 2. Run in Terminal (CLI)
```bash
# Find shortest route between any two landmarks
./campuspath "Main Gate" "Library"

# Or use numeric landmark IDs (0 to 9)
./campuspath 0 2

# Run performance benchmark (10,000 queries)
./campuspath --benchmark

# List all campus landmarks
./campuspath --locations
```

### 3. Open Web Visualizer
```bash
make run-server
```
Open **[http://localhost:8080](http://localhost:8080)** in your browser.

### 4. Run Unit Tests
```bash
make test
```
Runs all 88 automated unit tests for Graph, Dijkstra, and CampusMap data loaders.

---

## 🧠 Algorithm & Complexity

- **Algorithm**: Dijkstra's Single-Source Shortest Path
- **Data Structure**: Graph Adjacency List (`std::unordered_map<int, std::vector<Edge>>`) + Min-Heap Priority Queue (`std::priority_queue`)
- **Time Complexity**: $\mathcal{O}((V + E) \log V)$
- **Space Complexity**: $\mathcal{O}(V + E)$
- **Dataset**: 10 official campus landmarks ($|V| = 10$) connected by 16 pedestrian walkways ($|E| = 16$).

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
│   ├── src/                # Core implementation and HTTP server
│   └── tests/              # 88 automated unit tests
│
└── frontend/
    ├── index.html          # Clean Apple-style navigation interface
    ├── style.css           # Minimalist light styling
    ├── map.js              # Hardware-accelerated SVG campus map
    └── script.js           # REST API client & interaction controller
```

---

## 🌐 API Endpoints

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/locations` | Returns all 10 campus landmarks and categories |
| `GET` | `/api/connections` | Returns all 16 walkways and distances |
| `POST` | `/api/shortest-path` | Calculates optimal route between `source` and `destination` IDs |
