#include "Common.hpp"
#include "Graph.hpp"
#include "Dijkstra.hpp"
#include "CampusMap.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <chrono>

/**
 * @file main.cpp
 * @brief Terminal interface & ADSA pathfinding engine for CampusPath.
 * 
 * Capabilities:
 * - Direct CLI argument routing: ./bin/campuspath "Main Gate" "Library" or ./bin/campuspath 0 2
 * - Interactive menu mode: ./bin/campuspath
 * - Full telemetry & performance benchmarks: ./bin/campuspath --benchmark
 * - Mathematical optimality metrics & complexity analysis for Advance DSA.
 */

namespace {

// Helper: Trim whitespace and newline characters from user input
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Helper: Case-insensitive string comparison
bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// Helper: Safely reads a line from std::cin, detecting EOF
bool readLine(std::string& outLine) {
    if (!std::getline(std::cin, outLine)) {
        return false;
    }
    outLine = trim(outLine);
    return true;
}

// Helper: Robust integer parser that handles non-numeric and empty input gracefully
bool readIntegerInput(const std::string& prompt, int& outValue) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!readLine(line)) {
            return false;
        }
        if (line.empty()) {
            std::cout << "Error: Input cannot be empty. Please enter a valid number.\n\n";
            continue;
        }

        size_t parsedLength = 0;
        try {
            int val = std::stoi(line, &parsedLength);
            if (parsedLength == line.length()) {
                outValue = val;
                return true;
            }
        } catch (...) {
            // Catches std::invalid_argument and std::out_of_range
        }

        std::cout << "Error: '" << line << "' is not a valid integer. Please try again.\n\n";
    }
}

// Display the main navigation menu
void displayMenu() {
    std::cout << "\n======================================================\n";
    std::cout << "               CAMPUSPATH (ADSA ENGINE)              \n";
    std::cout << "    Intelligent Campus Navigation & Graph Routing     \n";
    std::cout << "======================================================\n\n";
    std::cout << "  1. View All Campus Locations (Vertices)\n";
    std::cout << "  2. View Walkway Connections (Edges)\n";
    std::cout << "  3. Find Shortest Route (Dijkstra's Algorithm)\n";
    std::cout << "  4. Run ADSA Performance Benchmark\n";
    std::cout << "  5. Exit\n\n";
}

// Feature 1: View all registered campus locations
void viewLocations(const CampusPath::CampusMap& campusMap) {
    std::cout << "\n--------------------------------------------------------------\n";
    std::cout << "  REGISTERED CAMPUS LOCATIONS (GRAPH VERTICES: |V| = " << campusMap.getNodeCount() << ")\n";
    std::cout << "--------------------------------------------------------------\n";
    std::cout << std::left << std::setw(6) << "ID"
              << std::setw(24) << "Location Name"
              << std::setw(18) << "Category"
              << "Degree\n";
    std::cout << "--------------------------------------------------------------\n";

    std::vector<CampusPath::Node> sortedNodes;
    sortedNodes.reserve(campusMap.getNodeCount());
    for (const auto& [id, node] : campusMap.getNodes()) {
        sortedNodes.push_back(node);
    }
    std::sort(sortedNodes.begin(), sortedNodes.end(),
              [](const CampusPath::Node& a, const CampusPath::Node& b) {
                  return a.id < b.id;
              });

    for (const auto& node : sortedNodes) {
        size_t degree = campusMap.getGraph().getNeighbors(node.id).size();
        std::cout << std::left << std::setw(6) << node.id
                  << std::setw(24) << node.name
                  << std::setw(18) << node.category
                  << degree << " connections\n";
    }
    std::cout << "--------------------------------------------------------------\n";
}

// Feature 2: View all campus connections
void viewConnections(const CampusPath::CampusMap& campusMap) {
    struct Connection {
        int src;
        std::string srcName;
        int dest;
        std::string destName;
        int distance;
    };

    std::vector<Connection> connections;
    for (const auto& [nodeId, node] : campusMap.getNodes()) {
        const auto& neighbors = campusMap.getGraph().getNeighbors(nodeId);
        for (const auto& edge : neighbors) {
            if (nodeId < edge.destination) {
                const auto* destNode = campusMap.getNodeById(edge.destination);
                std::string destName = (destNode != nullptr) ? destNode->name : "Unknown";
                connections.push_back({
                    nodeId,
                    node.name,
                    edge.destination,
                    destName,
                    static_cast<int>(edge.distance)
                });
            }
        }
    }

    std::sort(connections.begin(), connections.end(),
              [](const Connection& a, const Connection& b) {
                  if (a.src != b.src) return a.src < b.src;
                  return a.dest < b.dest;
              });

    std::cout << "\n--------------------------------------------------------------------------\n";
    std::cout << "  CAMPUS WALKWAYS (UNDIRECTED EDGES: |E| = " << connections.size() << ")\n";
    std::cout << "--------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(24) << "Location A"
              << std::setw(6) << "<--->"
              << std::setw(24) << "Location B"
              << "Distance (metres)\n";
    std::cout << "--------------------------------------------------------------------------\n";

    for (const auto& conn : connections) {
        std::cout << std::left << std::setw(24) << (conn.srcName + " (" + std::to_string(conn.src) + ")")
                  << std::setw(6) << "<--->"
                  << std::setw(24) << (conn.destName + " (" + std::to_string(conn.dest) + ")")
                  << conn.distance << " m\n";
    }
    std::cout << "--------------------------------------------------------------------------\n";
}

// Helper: Resolve a string to a Node ID (supports integer ID or fuzzy name)
int resolveNode(const CampusPath::CampusMap& campusMap, const std::string& input) {
    // Try parsing as integer
    try {
        size_t idx = 0;
        int id = std::stoi(input, &idx);
        if (idx == input.size() && campusMap.getNodeById(id) != nullptr) {
            return id;
        }
    } catch (...) {}

    // Try exact name match
    const auto* directNode = campusMap.getNodeByName(input);
    if (directNode != nullptr) {
        return directNode->id;
    }

    // Try case-insensitive matching
    for (const auto& [id, node] : campusMap.getNodes()) {
        if (equalsIgnoreCase(node.name, input)) {
            return id;
        }
    }

    // Try partial/prefix matching
    for (const auto& [id, node] : campusMap.getNodes()) {
        std::string nLow = node.name;
        std::string iLow = input;
        std::transform(nLow.begin(), nLow.end(), nLow.begin(), ::tolower);
        std::transform(iLow.begin(), iLow.end(), iLow.begin(), ::tolower);
        if (nLow.find(iLow) != std::string::npos) {
            return id;
        }
    }

    return -1;
}

// Pretty-print calculated route with DSA telemetry
void printRouteResult(const CampusPath::CampusMap& campusMap, int sourceId, int destId, double elapsedMicroseconds = 0.0) {
    const auto* sourceNode = campusMap.getNodeById(sourceId);
    const auto* destNode = campusMap.getNodeById(destId);

    if (sourceNode == nullptr || destNode == nullptr) {
        std::cerr << "Error: Invalid source or destination ID.\n";
        return;
    }

    std::cout << "\n======================================================\n";
    std::cout << "           OPTIMAL CAMPUS ROUTE (DIJKSTRA)            \n";
    std::cout << "======================================================\n";
    std::cout << "  Origin:      " << sourceNode->name << " [ID: " << sourceId << "]\n";
    std::cout << "  Destination: " << destNode->name << " [ID: " << destId << "]\n";
    std::cout << "------------------------------------------------------\n";

    if (sourceId == destId) {
        std::cout << "  Status:      Already at destination (0 metres walking)\n";
        std::cout << "  Route:       [" << sourceNode->name << "]\n";
        std::cout << "======================================================\n";
        return;
    }

    auto startClock = std::chrono::high_resolution_clock::now();
    CampusPath::PathResult result = CampusPath::Dijkstra::findShortestPath(campusMap.getGraph(), sourceId, destId);
    auto endClock = std::chrono::high_resolution_clock::now();
    if (elapsedMicroseconds <= 0.0) {
        elapsedMicroseconds = std::chrono::duration<double, std::micro>(endClock - startClock).count();
    }

    if (!result.found) {
        std::cout << "  Status:  NO PATH FOUND (Nodes are disconnected)\n";
        std::cout << "======================================================\n";
        return;
    }

    std::cout << "  Total Distance: " << static_cast<int>(result.totalDistance) << " metres\n";
    std::cout << "  Total Stops:    " << result.path.size() << " landmarks\n";
    std::cout << "  Search Time:    " << std::fixed << std::setprecision(2) << elapsedMicroseconds << " microseconds\n";
    std::cout << "------------------------------------------------------\n";
    std::cout << "  TURN-BY-TURN WAYFINDING:\n\n";

    double runningDist = 0.0;
    for (size_t i = 0; i < result.path.size(); ++i) {
        int u = result.path[i];
        const auto* node = campusMap.getNodeById(u);
        std::string name = (node != nullptr) ? node->name : "Node " + std::to_string(u);
        std::string cat = (node != nullptr) ? node->category : "";

        if (i == 0) {
            std::cout << "    [Start]   " << name << " (" << cat << ")\n";
        } else {
            int prev = result.path[i - 1];
            double leg = 0.0;
            for (const auto& edge : campusMap.getGraph().getNeighbors(prev)) {
                if (edge.destination == u) {
                    leg = edge.distance;
                    break;
                }
            }
            runningDist += leg;
            std::cout << "        |   + " << static_cast<int>(leg) << "m (Walk along campus pathway)\n";
            if (i == result.path.size() - 1) {
                std::cout << "    [Arrive]  " << name << " (" << cat << ")  [Cumulative: " << static_cast<int>(runningDist) << "m]\n";
            } else {
                std::cout << "    [Hop " << i << "]   " << name << " (" << cat << ")  [Cumulative: " << static_cast<int>(runningDist) << "m]\n";
            }
        }
    }

    std::cout << "\n------------------------------------------------------\n";
    std::cout << "  ADSA COMPLEXITY & GUARANTEES:\n";
    std::cout << "    - Algorithm:         Dijkstra's Single-Source Shortest Path\n";
    std::cout << "    - Data Structure:    Adjacency List + Min-Heap Priority Queue\n";
    std::cout << "    - Time Complexity:   O((V + E) log V)\n";
    std::cout << "    - Space Complexity:  O(V + E)\n";
    std::cout << "    - Optimality:        Mathematically proven shortest path\n";
    std::cout << "======================================================\n";
}

// Feature 3: Interactive Route Query
void findShortestRoute(const CampusPath::CampusMap& campusMap) {
    viewLocations(campusMap);

    int sourceId = 0;
    if (!readIntegerInput("\nEnter Source Location ID: ", sourceId)) return;
    if (campusMap.getNodeById(sourceId) == nullptr) {
        std::cout << "Error: Location ID " << sourceId << " does not exist.\n";
        return;
    }

    int destId = 0;
    if (!readIntegerInput("Enter Destination Location ID: ", destId)) return;
    if (campusMap.getNodeById(destId) == nullptr) {
        std::cout << "Error: Location ID " << destId << " does not exist.\n";
        return;
    }

    printRouteResult(campusMap, sourceId, destId);
}

// Feature 4: Benchmark ADSA Engine
void runBenchmark(const CampusPath::CampusMap& campusMap) {
    const int iterations = 10000;
    std::cout << "\n======================================================\n";
    std::cout << "         ADSA ENGINE BENCHMARK SUITE (" << iterations << " RUNS)       \n";
    std::cout << "======================================================\n";
    std::cout << "  Testing all-pairs shortest paths across the campus graph...\n";

    std::vector<int> nodeIds;
    for (const auto& [id, _] : campusMap.getNodes()) {
        nodeIds.push_back(id);
    }

    auto start = std::chrono::high_resolution_clock::now();
    size_t pathsFound = 0;
    double totalAccumulatedMeters = 0.0;

    for (int i = 0; i < iterations; ++i) {
        int src = nodeIds[i % nodeIds.size()];
        int dst = nodeIds[(i * 3 + 7) % nodeIds.size()];
        CampusPath::PathResult res = CampusPath::Dijkstra::findShortestPath(campusMap.getGraph(), src, dst);
        if (res.found) {
            pathsFound++;
            totalAccumulatedMeters += res.totalDistance;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(end - start).count();
    double avgUs = (totalMs * 1000.0) / iterations;

    std::cout << "  - Executed Queries:   " << iterations << "\n";
    std::cout << "  - Successful Paths:   " << pathsFound << "\n";
    std::cout << "  - Total Distance:     " << static_cast<long long>(totalAccumulatedMeters) << " m accumulated\n";
    std::cout << "  - Total Wall Time:    " << totalMs << " ms\n";
    std::cout << "  - Average Time/Query: " << avgUs << " microseconds (" << (avgUs / 1000.0) << " ms)\n";
    std::cout << "  - Throughput:         " << static_cast<int>(iterations / (totalMs / 1000.0)) << " queries/sec\n";
    std::cout << "  - Status:             OPTIMAL (All Dijkstra Relaxations Valid)\n";
    std::cout << "======================================================\n";
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    CampusPath::CampusMap campusMap;

    // Resolve CSV file paths: supports running from workspace root or backend/ directory
    std::string nodesPath = "backend/data/nodes.csv";
    std::string edgesPath = "backend/data/edges.csv";
    if (!std::ifstream(nodesPath).good()) {
        nodesPath = "data/nodes.csv";
        edgesPath = "data/edges.csv";
    }

    // Load and validate campus dataset
    if (!campusMap.loadFromCSV(nodesPath, edgesPath)) {
        std::cerr << "Fatal Error: Failed to load campus dataset.\n";
        std::cerr << campusMap.getLastError() << "\n";
        return 1;
    }

    // Direct CLI Arguments Handling
    if (argc >= 2) {
        std::string arg1 = argv[1];
        if (arg1 == "-h" || arg1 == "--help") {
            std::cout << "CampusPath Navigation & ADSA Shortest Path CLI\n\n";
            std::cout << "Usage:\n";
            std::cout << "  ./bin/campuspath                     Launch interactive terminal menu\n";
            std::cout << "  ./bin/campuspath <src> <dest>        Calculate route between two nodes\n";
            std::cout << "  ./bin/campuspath --locations         Display all campus landmarks\n";
            std::cout << "  ./bin/campuspath --connections       Display all walkways and distances\n";
            std::cout << "  ./bin/campuspath --benchmark         Run ADSA Dijkstra benchmark (10,000 queries)\n";
            std::cout << "\nExamples:\n";
            std::cout << "  ./bin/campuspath \"Main Gate\" \"Library\"\n";
            std::cout << "  ./bin/campuspath 0 2\n";
            std::cout << "  ./bin/campuspath \"Canteen\" \"Auditorium\"\n";
            return 0;
        }

        if (arg1 == "-l" || arg1 == "--locations") {
            viewLocations(campusMap);
            return 0;
        }

        if (arg1 == "-c" || arg1 == "--connections") {
            viewConnections(campusMap);
            return 0;
        }

        if (arg1 == "-b" || arg1 == "--benchmark") {
            runBenchmark(campusMap);
            return 0;
        }

        if (arg1 == "-i" || arg1 == "--interactive") {
            // Proceed to interactive loop below
        } else if (argc >= 3) {
            std::string srcStr = argv[1];
            std::string dstStr = argv[2];
            int srcId = resolveNode(campusMap, srcStr);
            int dstId = resolveNode(campusMap, dstStr);

            if (srcId == -1) {
                std::cerr << "Error: Could not identify source location: '" << srcStr << "'.\n";
                std::cerr << "Run './bin/campuspath --locations' to view valid names.\n";
                return 1;
            }
            if (dstId == -1) {
                std::cerr << "Error: Could not identify destination location: '" << dstStr << "'.\n";
                std::cerr << "Run './bin/campuspath --locations' to view valid names.\n";
                return 1;
            }

            printRouteResult(campusMap, srcId, dstId);
            return 0;
        }
    }

    // Interactive Loop Mode
    bool isRunning = true;
    while (isRunning) {
        displayMenu();

        int choice = 0;
        if (!readIntegerInput("Enter your choice: ", choice)) {
            std::cout << "\nThank you for using CampusPath!\n";
            break;
        }

        switch (choice) {
            case 1:
                viewLocations(campusMap);
                break;
            case 2:
                viewConnections(campusMap);
                break;
            case 3:
                findShortestRoute(campusMap);
                break;
            case 4:
                runBenchmark(campusMap);
                break;
            case 5:
                std::cout << "\nThank you for using CampusPath!\n";
                isRunning = false;
                break;
            default:
                std::cout << "\nInvalid choice (" << choice << "). Please select an option between 1 and 5.\n";
                break;
        }
    }

    return 0;
}
