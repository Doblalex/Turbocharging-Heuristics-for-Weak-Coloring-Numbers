#pragma once

#include "Headers.hpp"
#include "DSU.hpp"

/**
 * @brief Keeps track of a subordering of vertices
 *
 */
class Ordering
{
private:
    /**
     * @brief Comparator by order for ordered adjacency list
     *
     */
    struct CompByOrder
    {
        const int v;
        const OrderedGraph &graph;
        CompByOrder(const int v, const OrderedGraph &graph);

        bool operator()(const int a, const int b) const;
    };
    void SwapInOut(int v_in, int v_out);
    void SwapInIn(int vleft, int vright);
    vector<int> RReachableRightOptimized(const int src);

public:
    int at;                                         // current position
    int r;                                          // radius
    vector<int> ordering;                           // array for subordering
    OrderedGraph &graph;                            // the graph
    vector<unordered_set<int>> wreach;              // weakly reachable sets for all vertices
    vector<unordered_set<int>> wreach_inv;          // inverse weakly reachable sets for all vertices
    unordered_set<int> is_too_full;                 // vertices with weakly reachable set too big
    vector<unordered_set<int>> too_full_neighbours; // saves for each vertex with wreach size k rreachable neighbours with wreach size k, where both are not placed in the ordering.
    unordered_set<int> is_full_neighbour;           // vertices with neighbour who is too big
    bool track_full_neighbours;
    EdgeMutableGraph graph_notplaced;                                                  // the graph G[T]\cup S where T are the free vertices. We only augment edges for performance reasons
    unordered_set<int> vertices_active;                                                // the set of free vertices T
    bool do_lower_bound;                                                               // apply WCOL-MMD lower bound
    vector<set<int, std::function<bool(const int a, const int b)>>> graph_adj_ordered; // ordered adjacency list
    bool do_graph_adj_ordered;                                                         // apply ordered adjacnecy list
    bool ignore_right;                                                                 // ignore weakly reachable sets of free vertices for lower-bounding
    bool opt_components;                                                               // use connected components for lastc branching order and heuristics

    // For Heuristics for updating binary search tree for next decision
    unordered_set<int> changed; // either removed, added, or wreach_size changed

    int target_k = INT_MAX; // the desired weak coloring number

    Ordering(OrderedGraph &graph, int r, bool track_full_neighbours, bool do_lower_bound, bool do_graph_adj_ordered, bool ignore_right, bool opt_components);

    vector<int> RReachableRight(int src);
    vector<int> RReachable(int src);
    vector<int> RReachableNotPlaced(int src);
    vector<int> ComponentNotPlaced(int src);
    void WreachAdd(int vertex, int u);
    void WreachRem(int vertex, int u);
    void Place(int vertex);
    void UnPlace();
    bool IsExtendable();
    bool IsExtendableNoLB();
    int Wreach();
    void PrintOrdering();
    bool IsValidFull();
    void Swap(int v1, int v2);
    void VerifyImpl();
    void VerifyImplMerge(vector<int> &hackordering, vector<int> ignored);
    void Place(int vertex, int position);
    void Unplace(int position);
    void UpdateWreachHelper(int u, unordered_set<pair<int, int>, boost::hash<std::pair<int, int>>> &alladded);
    void ResetTo(vector<int> &oldordering);
    void UpdateWreach(int v);
    void RemoveVertex(int v);
    void AddVertex(int v);
    bool IsLBDegeneracyExceed();
    int LBDegeneracy();
    bool IsLBDegeneracyContractExceed();
    int LBDegeneracyContract();
    int LBDegeneracyContractSubGraph();
    void ChangePosition(const int u, const int pos);
};