#pragma once

#include <any>
#include <functional>
#include <limits>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "../heuristic/base.h"
#include "../task.h"
#include "breadth_first_search.h"
#include "searchspace.h"

const int INF = std::numeric_limits<int>::max();

inline std::vector<int> astar(BaseTask& planning_task, Heuristic& heuristic) {
    int iteration = 0;
    int expansions = 0;
    std::priority_queue<tuple<float, float, int>> queue;
    std::vector<SearchNode> nodes;
    nodes.push_back(make_root_node(planning_task.initial_state));
    float h = heuristic.calculate_h(0, nodes);
    std::cout << "Initial h value: " << h << "\n";
    queue.push({-1.0 * (h + (float)nodes[0].g), -h, 0});

    flat_hash_map<size_t, int> state_cost = {{nodes[0].hash_value, 0}};
    std::vector<std::pair<int, pair<size_t, flat_hash_set<int>>>> successors;
    tuple<float, float, int> front_status;
    int node_idx, succ_g, old_succ_g;

    while (!queue.empty()) {
        ++iteration;

        front_status = queue.top();
        node_idx = get<2>(front_status);
        queue.pop();

        if (state_cost[nodes[node_idx].hash_value] == nodes[node_idx].g) {
            expansions++;
            if (planning_task.goal_reached(nodes[node_idx].state)) {
                std::cout << iteration << " Nodes expanded\n";
                return extract_solution(node_idx, nodes);
            }

            successors.clear();
            planning_task.get_successor_states(
                nodes[node_idx].state, successors, nodes[node_idx].hash_value);
            for (auto& opss : successors) {
                // SearchNode succ_node =
                nodes.emplace_back(
                    make_child_node(node_idx, nodes[node_idx].g, opss.first,
                                    opss.second.second, opss.second.first));
                // nodes.emplace_back(succ_node);
                h = heuristic.calculate_h(nodes.size() - 1, nodes);
                old_succ_g = INF;
                succ_g = nodes[nodes.size() - 1].g;
                if (state_cost.find(opss.second.first) != state_cost.end()) {
                    old_succ_g = state_cost[opss.second.first];
                    if (succ_g < old_succ_g) {
                        queue.push(
                            {-1 * (h + (float)succ_g), -h, nodes.size() - 1});
                        state_cost[opss.second.first] = succ_g;
                    }
                } else {
                    queue.push(
                        {-1 * (h + (float)succ_g), -h, nodes.size() - 1});
                    state_cost.emplace(opss.second.first, succ_g);
                }
            }
        }
    }

    std::cout << iteration << " Nodes expanded" << std::endl;
    std::cerr << "No solution found" << std::endl;
    return {};  // No solution found
}
