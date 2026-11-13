#include "Common.hpp"
#include "Graph.hpp"
#include "Dijkstra.hpp"
#include "CampusMap.hpp"
#include "httplib.h"
#include "json.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <chrono>

using json = nlohmann::json;

namespace {

// Helper: Sets standard Cross-Origin Resource Sharing (CORS) headers
void applyCORSHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Accept");
}

} // anonymous namespace

int main() {
    CampusPath::CampusMap campusMap;

    // Resolve dataset paths whether launched from workspace root or backend/
    std::string nodesPath = "backend/data/nodes.csv";
    std::string edgesPath = "backend/data/edges.csv";
    if (!std::ifstream(nodesPath).good()) {
        nodesPath = "data/nodes.csv";
        edgesPath = "data/edges.csv";
    }

    if (!campusMap.loadFromCSV(nodesPath, edgesPath)) {
        std::cerr << "Fatal Error: Failed to load campus dataset.\n";
        std::cerr << campusMap.getLastError() << "\n";
        return 1;
    }

    httplib::Server svr;

    // ----------------------------------------------------
    // Static Frontend File Hosting
    // ----------------------------------------------------
    std::string frontendDir = "frontend";
    if (!std::ifstream("frontend/index.html").good()) {
        frontendDir = "../frontend";
    }
    if (std::ifstream(frontendDir + "/index.html").good()) {
        svr.set_mount_point("/", frontendDir);
    }

    // ----------------------------------------------------
    // CORS PRE-FLIGHT HANDLER (HTTP OPTIONS)
    // ----------------------------------------------------
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        applyCORSHeaders(res);
        res.status = 204; // 204 No Content
    });

    // ----------------------------------------------------
    // GET /api/locations
    // Returns all campus locations sorted by ID
    // ----------------------------------------------------
    svr.Get("/api/locations", [&campusMap](const httplib::Request&, httplib::Response& res) {
        std::cout << "[API] GET /api/locations\n";

        std::vector<CampusPath::Node> sortedNodes;
        sortedNodes.reserve(campusMap.getNodeCount());
        for (const auto& [id, node] : campusMap.getNodes()) {
            sortedNodes.push_back(node);
        }
        std::sort(sortedNodes.begin(), sortedNodes.end(),
                  [](const CampusPath::Node& a, const CampusPath::Node& b) {
                      return a.id < b.id;
                  });

        json locArray = json::array();
        for (const auto& node : sortedNodes) {
            size_t degree = campusMap.getGraph().getNeighbors(node.id).size();
            locArray.push_back({
                {"id", node.id},
                {"name", node.name},
                {"category", node.category},
                {"degree", degree}
            });
        }

        applyCORSHeaders(res);
        res.status = 200;
        res.set_content(locArray.dump(2), "application/json");
    });

    // ----------------------------------------------------
    // GET /api/connections
    // Returns all unique undirected walking paths
    // ----------------------------------------------------
    svr.Get("/api/connections", [&campusMap](const httplib::Request&, httplib::Response& res) {
        std::cout << "[API] GET /api/connections\n";

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
                // Ensure each undirected connection is reported only once
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

        json connArray = json::array();
        for (const auto& conn : connections) {
            connArray.push_back({
                {"source", conn.src},
                {"source_name", conn.srcName},
                {"destination", conn.dest},
                {"destination_name", conn.destName},
                {"distance", conn.distance}
            });
        }

        applyCORSHeaders(res);
        res.status = 200;
        res.set_content(connArray.dump(2), "application/json");
    });

    // ----------------------------------------------------
    // Shared Shortest-Path Computation Core
    // ----------------------------------------------------
    auto solveShortestRoute = [&campusMap](int sourceId, int destId, httplib::Response& res) {
        const auto* sourceNode = campusMap.getNodeById(sourceId);
        const auto* destNode = campusMap.getNodeById(destId);

        // Validate node existence
        if (sourceNode == nullptr || destNode == nullptr) {
            int missingId = (sourceNode == nullptr) ? sourceId : destId;
            applyCORSHeaders(res);
            res.status = 404;
            res.set_content(json({
                {"success", false},
                {"error", "Location ID " + std::to_string(missingId) + " does not exist."}
            }).dump(2), "application/json");
            return;
        }

        // Special case: source equals destination
        if (sourceId == destId) {
            applyCORSHeaders(res);
            res.status = 200;
            json responseJson = {
                {"success", true},
                {"distance", 0},
                {"route", json::array({
                    {
                        {"id", sourceNode->id},
                        {"name", sourceNode->name},
                        {"category", sourceNode->category},
                        {"step_distance", 0},
                        {"cumulative_distance", 0}
                    }
                })},
                {"telemetry", {
                    {"algorithm", "Dijkstra (Min-Heap Priority Queue)"},
                    {"time_complexity", "O((V + E) log V)"},
                    {"space_complexity", "O(V + E)"},
                    {"execution_time_us", 0.5},
                    {"total_vertices", campusMap.getNodeCount()},
                    {"total_edges", campusMap.getEdgeCount()},
                    {"relaxations_count", 0},
                    {"visited_order", json::array({sourceId})}
                }}
            };
            res.set_content(responseJson.dump(2), "application/json");
            return;
        }

        // Compute shortest path with high-precision timing
        auto t0 = std::chrono::high_resolution_clock::now();
        CampusPath::PathResult result = CampusPath::Dijkstra::findShortestPath(
            campusMap.getGraph(), sourceId, destId
        );
        auto t1 = std::chrono::high_resolution_clock::now();
        double elapsedUs = std::chrono::duration<double, std::micro>(t1 - t0).count();

        if (!result.found) {
            applyCORSHeaders(res);
            res.status = 404;
            res.set_content(json({
                {"success", false},
                {"error", "No walking route exists between " + sourceNode->name + " and " + destNode->name + "."}
            }).dump(2), "application/json");
            return;
        }

        // Build route objects with step distance and cumulative distance
        json routeArray = json::array();
        double runningDist = 0.0;
        for (size_t i = 0; i < result.path.size(); ++i) {
            int u = result.path[i];
            const auto* node = campusMap.getNodeById(u);
            std::string name = (node != nullptr) ? node->name : "Unknown";
            std::string category = (node != nullptr) ? node->category : "";

            double stepDist = 0.0;
            if (i > 0) {
                int prev = result.path[i - 1];
                for (const auto& edge : campusMap.getGraph().getNeighbors(prev)) {
                    if (edge.destination == u) {
                        stepDist = edge.distance;
                        break;
                    }
                }
                runningDist += stepDist;
            }

            routeArray.push_back({
                {"id", u},
                {"name", name},
                {"category", category},
                {"step_distance", static_cast<int>(stepDist)},
                {"cumulative_distance", static_cast<int>(runningDist)}
            });
        }

        json responseJson = {
            {"success", true},
            {"distance", static_cast<int>(result.totalDistance)},
            {"route", routeArray},
            {"telemetry", {
                {"algorithm", "Dijkstra (Min-Heap Priority Queue)"},
                {"time_complexity", "O((V + E) log V)"},
                {"space_complexity", "O(V + E)"},
                {"execution_time_us", elapsedUs},
                {"total_vertices", campusMap.getNodeCount()},
                {"total_edges", campusMap.getEdgeCount()},
                {"relaxations_count", result.relaxationsCount},
                {"visited_order", result.visitedOrder}
            }}
        };

        applyCORSHeaders(res);
        res.status = 200;
        res.set_content(responseJson.dump(2), "application/json");
    };

    // ----------------------------------------------------
    // GET /api/shortest-path
    // Query params: ?source=X&destination=Y or ?from=X&to=Y (IDs or Names)
    // ----------------------------------------------------
    svr.Get("/api/shortest-path", [&campusMap, &solveShortestRoute](const httplib::Request& req, httplib::Response& res) {
        std::string srcStr = req.has_param("source") ? req.get_param_value("source") : (req.has_param("from") ? req.get_param_value("from") : "");
        std::string dstStr = req.has_param("destination") ? req.get_param_value("destination") : (req.has_param("to") ? req.get_param_value("to") : "");

        if (srcStr.empty() || dstStr.empty()) {
            applyCORSHeaders(res);
            res.status = 400;
            res.set_content(json({
                {"success", false},
                {"error", "Query parameters 'source'/'from' and 'destination'/'to' are required."}
            }).dump(2), "application/json");
            return;
        }

        int sourceId = -1;
        int destId = -1;

        try {
            sourceId = std::stoi(srcStr);
        } catch (...) {
            const auto* sNode = campusMap.getNodeByName(srcStr);
            if (sNode) sourceId = sNode->id;
        }

        try {
            destId = std::stoi(dstStr);
        } catch (...) {
            const auto* dNode = campusMap.getNodeByName(dstStr);
            if (dNode) destId = dNode->id;
        }

        if (sourceId < 0 || destId < 0) {
            applyCORSHeaders(res);
            res.status = 404;
            res.set_content(json({
                {"success", false},
                {"error", "One or both specified locations could not be resolved."}
            }).dump(2), "application/json");
            return;
        }

        solveShortestRoute(sourceId, destId, res);
    });

    // ----------------------------------------------------
    // POST /api/shortest-path
    // Calculates the shortest walking route using Dijkstra
    // ----------------------------------------------------
    svr.Post("/api/shortest-path", [&campusMap, &solveShortestRoute](const httplib::Request& req, httplib::Response& res) {
        std::cout << "[API] POST /api/shortest-path - Body: " << req.body << "\n";

        // Parse JSON payload
        json reqJson;
        try {
            reqJson = json::parse(req.body);
        } catch (const json::parse_error&) {
            applyCORSHeaders(res);
            res.status = 400;
            res.set_content(json({
                {"success", false},
                {"error", "Malformed JSON payload: invalid JSON syntax."}
            }).dump(2), "application/json");
            return;
        }

        int sourceId = -1;
        int destId = -1;

        if (reqJson.contains("source") && reqJson["source"].is_number_integer()) {
            sourceId = reqJson["source"].get<int>();
        } else if (reqJson.contains("source") && reqJson["source"].is_string()) {
            const auto* sNode = campusMap.getNodeByName(reqJson["source"].get<std::string>());
            if (sNode) sourceId = sNode->id;
        } else if (reqJson.contains("from") && reqJson["from"].is_string()) {
            const auto* sNode = campusMap.getNodeByName(reqJson["from"].get<std::string>());
            if (sNode) sourceId = sNode->id;
        }

        if (reqJson.contains("destination") && reqJson["destination"].is_number_integer()) {
            destId = reqJson["destination"].get<int>();
        } else if (reqJson.contains("destination") && reqJson["destination"].is_string()) {
            const auto* dNode = campusMap.getNodeByName(reqJson["destination"].get<std::string>());
            if (dNode) destId = dNode->id;
        } else if (reqJson.contains("to") && reqJson["to"].is_string()) {
            const auto* dNode = campusMap.getNodeByName(reqJson["to"].get<std::string>());
            if (dNode) destId = dNode->id;
        }

        if (sourceId < 0 || destId < 0) {
            applyCORSHeaders(res);
            res.status = 400;
            res.set_content(json({
                {"success", false},
                {"error", "Invalid request: 'source' and 'destination' fields are required."}
            }).dump(2), "application/json");
            return;
        }

        solveShortestRoute(sourceId, destId, res);
    });

    const std::string host = "127.0.0.1";
    const int port = 8080;

    std::cout << "========================================================\n";
    std::cout << "  CampusPath HTTP API Server & Spatial Web Host\n";
    std::cout << "========================================================\n";
    std::cout << "  Web UI:      http://" << host << ":" << port << "/\n";
    std::cout << "  API Endpoints:\n";
    std::cout << "    GET  /api/locations\n";
    std::cout << "    GET  /api/connections\n";
    std::cout << "    POST /api/shortest-path\n";
    std::cout << "  Press Ctrl+C to stop the server.\n\n";

    svr.listen(host, port);

    return 0;
}
