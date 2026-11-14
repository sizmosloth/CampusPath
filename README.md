# 🗺️ CampusPath — Campus Navigation System

CampusPath is an **Advanced Data Structures and Algorithms (ADSA)** project implemented in C++. It models a university campus as an undirected weighted graph and calculates mathematically optimal walking routes between campus landmarks using **Dijkstra's Algorithm** with a **min-heap priority queue**.

The project provides two easy ways to use it:
1. **Terminal CLI** — A fast, scriptable command-line interface.
2. **Web Visualizer** — An interactive map and web terminal running locally in your browser.

---

## ⚡ How to Run

### 1. Build the Project
Compile the C++ binaries using the provided `Makefile`:
```bash
make
```
This generates the `./campuspath` CLI executable and the background HTTP server.

---

### 2. Run via Terminal (CLI)

You can run navigation queries directly from your terminal:

```bash
# Direct shortest route between two landmarks
./campuspath "Main Gate" "Library"

# Multi-stop route with an intermediate stop (e.g. stop at Stationery first)
./campuspath "Main Gate" "Stationery" "Library"

# You can also use numeric landmark IDs (0 to 10)
./campuspath 0 10 2

# List all 11 campus landmarks with IDs and categories
./campuspath --locations

# Run a performance benchmark (10,000 random queries)
./campuspath --benchmark
```

**Example CLI Output:**
```
======================================================
  CAMPUSPATH — OPTIMAL SHORTEST ROUTE
======================================================
  ORIGIN:       Main Gate [ID: 0]
  VIA STOP:     Stationery [ID: 10]
  DESTINATION:  Library [ID: 2]
------------------------------------------------------
  Total Distance: 510 metres
  Total Waypoints: 4 landmarks
  Compute Time:   74.2 microseconds
------------------------------------------------------
  WAYFINDING ITINERARY:
    [Start]    Main Gate (Entrance)
        |      + 250m pathway leg
    [Step 1]   Admin Block (Administration)
        |      + 120m pathway leg
    [Via Stop] Stationery (Store)
        |      + 140m pathway leg
    [Arrive]   Library (Academic)  [Total: 510m]
------------------------------------------------------
  ALGORITHM: Dijkstra's Shortest Path (Min-Heap Priority Queue)
  COMPLEXITY: Time: O((V + E) log V) | Space: O(V + E)
======================================================
```

---

### 3. Run the Web Visualizer

Start the local server:
```bash
make run-server
```
Then open **[http://localhost:8080](http://localhost:8080)** in any web browser.

**Web Features:**
- **Campus Map**: Visual schematic of the campus graph. Select start and destination from dropdowns or click directly on campus buildings. Click **"+ Add Stop"** to route via intermediate waypoints (such as Stationery).
- **Terminal CLI Tab**: An interactive shell right in your browser where you can type commands (`route "Main Gate" "Library"`, `locations`, `benchmark`, `clear`, `help`).
- **Algorithm Playback**: Click "Play Steps" to watch Dijkstra's priority queue explore nodes in order of shortest tentative distance.

---

### 4. Run Automated Unit Tests

Run the test suite to verify graph operations, Dijkstra correctness, and CSV dataset loading:
```bash
make test
```
Runs all **89 automated tests** (32 Graph tests, 29 Dijkstra tests, 28 CampusMap tests).

---

## 🧠 Data Structures & Algorithms

| Component | Implementation | Complexity |
| :--- | :--- | :--- |
| **Graph Representation** | Adjacency List using `std::unordered_map<int, std::vector<Edge>>` | Space: $\mathcal{O}(V + E)$ |
| **Shortest Path Engine** | Dijkstra's Algorithm with `std::priority_queue` (Min-Heap) | Time: $\mathcal{O}((V + E) \log V)$ |
| **Multi-Stop Routing** | Optimal sub-path decomposition: $\min(S \to V_{\text{stop}}) + \min(V_{\text{stop}} \to D)$ | Time: $\mathcal{O}((V + E) \log V)$ |
| **Dataset Storage** | CSV flat files (`backend/data/nodes.csv`, `backend/data/edges.csv`) | 11 Landmarks, 19 Walkways |
| **Benchmark Throughput** | Multi-query stress test (10,000 queries) | $\approx 120,000\text{ queries/sec}$ ($< 10\,\mu\text{s/query}$) |

---

## 📂 Project Structure

```
CampusPath/
├── Makefile                # Build coordinator (make, make test, make run-server)
├── campuspath              # CLI launcher script / executable
├── README.md               # Project documentation
│
├── backend/
│   ├── data/               # Campus dataset (nodes.csv, edges.csv)
│   ├── include/            # C++ headers (Graph.hpp, Dijkstra.hpp, CampusMap.hpp)
│   ├── src/                # Core C++ source (main.cpp, server.cpp)
│   └── tests/              # 89 unit tests (test_graph, test_dijkstra, test_campus_map)
│
└── frontend/
    ├── index.html          # Clean web interface & terminal tab
    ├── style.css           # Modern minimalist stylesheet
    ├── map.js              # SVG campus schematic and route rendering
    └── script.js           # REST API client & interactive browser shell
```

---

## 🌐 HTTP REST API

The backend server exposes simple JSON endpoints:

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/locations` | Returns all 11 campus landmarks with IDs and categories |
| `GET` | `/api/connections` | Returns all 19 pedestrian walkways with weights |
| `POST` | `/api/shortest-path` | Accepts `{"source": 0, "destination": 2}` (optional `"via": 10`) and returns optimal path + telemetry |
| `GET` | `/api/shortest-path?from=0&to=2&via=10` | Query-string equivalent |
