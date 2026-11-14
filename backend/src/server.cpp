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
    // Shared Shortest-Path Computation Core (Supports Multi-Waypoint Stop)
    // ----------------------------------------------------
    auto solveShortestRoute = [&campusMap](int sourceId, int destId, int viaId, httplib::Response& res) {
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

        // Special case: intermediate waypoint stop requested
        if (viaId >= 0 && viaId != sourceId && viaId != destId) {
            const auto* viaNode = campusMap.getNodeById(viaId);
            if (viaNode == nullptr) {
                applyCORSHeaders(res);
                res.status = 404;
                res.set_content(json({
                    {"success", false},
                    {"error", "Via Stop ID " + std::to_string(viaId) + " does not exist."}
                }).dump(2), "application/json");
                return;
            }

            auto t0 = std::chrono::high_resolution_clock::now();
            CampusPath::PathResult leg1 = CampusPath::Dijkstra::findShortestPath(campusMap.getGraph(), sourceId, viaId);
            CampusPath::PathResult leg2 = CampusPath::Dijkstra::findShortestPath(campusMap.getGraph(), viaId, destId);
            auto t1 = std::chrono::high_resolution_clock::now();
            double elapsedUs = std::chrono::duration<double, std::micro>(t1 - t0).count();

            if (!leg1.found || !leg2.found) {
                applyCORSHeaders(res);
                res.status = 404;
                res.set_content(json({
                    {"success", false},
                    {"error", "No route exists via " + viaNode->name + "."}
                }).dump(2), "application/json");
                return;
            }

            std::vector<int> mergedPath = leg1.path;
            if (!mergedPath.empty() && !leg2.path.empty()) {
                mergedPath.pop_back(); // Remove duplicate junction node
            }
            mergedPath.insert(mergedPath.end(), leg2.path.begin(), leg2.path.end());

            std::vector<int> mergedVisited = leg1.visitedOrder;
            for (int v : leg2.visitedOrder) {
                mergedVisited.push_back(v);
            }

            json routeArray = json::array();
            double runningDist = 0.0;
            for (size_t i = 0; i < mergedPath.size(); ++i) {
                int u = mergedPath[i];
                const auto* node = campusMap.getNodeById(u);
                std::string name = (node != nullptr) ? node->name : "Unknown";
                std::string category = (node != nullptr) ? node->category : "";

                double stepDist = 0.0;
                if (i > 0) {
                    int prev = mergedPath[i - 1];
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
                    {"cumulative_distance", static_cast<int>(runningDist)},
                    {"is_stop", (u == viaId)}
                });
            }

            json responseJson = {
                {"success", true},
                {"distance", static_cast<int>(leg1.totalDistance + leg2.totalDistance)},
                {"route", routeArray},
                {"has_stop", true},
                {"stop_id", viaId},
                {"stop_name", viaNode->name},
                {"telemetry", {
                    {"algorithm", "Multi-Waypoint Dijkstra (Min-Heap Priority Queue)"},
                    {"time_complexity", "O((V + E) log V)"},
                    {"space_complexity", "O(V + E)"},
                    {"execution_time_us", elapsedUs},
                    {"total_vertices", campusMap.getNodeCount()},
                    {"total_edges", campusMap.getEdgeCount()},
                    {"relaxations_count", leg1.relaxationsCount + leg2.relaxationsCount},
                    {"visited_order", mergedVisited}
                }}
            };

            applyCORSHeaders(res);
            res.status = 200;
            res.set_content(responseJson.dump(2), "application/json");
            return;
        }

        // Direct Route (No Stop or source == dest)
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
                        {"cumulative_distance", 0},
                        {"is_stop", false}
                    }
                })},
                {"has_stop", false},
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

        // Direct shortest path computation
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
                {"cumulative_distance", static_cast<int>(runningDist)},
                {"is_stop", false}
            });
        }

        json responseJson = {
            {"success", true},
            {"distance", static_cast<int>(result.totalDistance)},
            {"route", routeArray},
            {"has_stop", false},
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
    // Query params: ?source=X&destination=Y[&via=Z] or ?from=X&to=Y[&stop=Z]
    // ----------------------------------------------------
    svr.Get("/api/shortest-path", [&campusMap, &solveShortestRoute](const httplib::Request& req, httplib::Response& res) {
        std::string srcStr = req.has_param("source") ? req.get_param_value("source") : (req.has_param("from") ? req.get_param_value("from") : "");
        std::string dstStr = req.has_param("destination") ? req.get_param_value("destination") : (req.has_param("to") ? req.get_param_value("to") : "");
        std::string viaStr = req.has_param("via") ? req.get_param_value("via") : (req.has_param("stop") ? req.get_param_value("stop") : "");

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
        int viaId = -1;

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

        if (!viaStr.empty()) {
            try {
                viaId = std::stoi(viaStr);
            } catch (...) {
                const auto* vNode = campusMap.getNodeByName(viaStr);
                if (vNode) viaId = vNode->id;
            }
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

        solveShortestRoute(sourceId, destId, viaId, res);
    });

    // ----------------------------------------------------
    // POST /api/shortest-path
    // Calculates the shortest walking route using Dijkstra (supports optional "via" or "stop")
    // ----------------------------------------------------
    svr.Post("/api/shortest-path", [&campusMap, &solveShortestRoute](const httplib::Request& req, httplib::Response& res) {
        std::cout << "[API] POST /api/shortest-path - Body: " << req.body << "\n";

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
        int viaId = -1;

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

        if (reqJson.contains("via") && reqJson["via"].is_number_integer()) {
            viaId = reqJson["via"].get<int>();
        } else if (reqJson.contains("via") && reqJson["via"].is_string()) {
            const auto* vNode = campusMap.getNodeByName(reqJson["via"].get<std::string>());
            if (vNode) viaId = vNode->id;
        } else if (reqJson.contains("stop") && reqJson["stop"].is_number_integer()) {
            viaId = reqJson["stop"].get<int>();
        } else if (reqJson.contains("stop") && reqJson["stop"].is_string()) {
            const auto* vNode = campusMap.getNodeByName(reqJson["stop"].get<std::string>());
            if (vNode) viaId = vNode->id;
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

        solveShortestRoute(sourceId, destId, viaId, res);
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
