#pragma once

#include "Headers.hpp"
#include "DSU.hpp"

class Ordering
{
private:
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
    int at;
    int r;
    vector<int> ordering;
    OrderedGraph &graph;
    vector<unordered_set<int>> wreach;
    vector<unordered_set<int>> wreach_inv;
    unordered_set<int> is_too_full;                 // vertices with weakly reachable set too big
    vector<unordered_set<int>> too_full_neighbours; // saves for each vertex with wreach size k rreachable neighbours with wreach size k, where both are not placed in the ordering.
    unordered_set<int> is_full_neighbour;           // vertices with neighbour who is too big
    bool track_full_neighbours;
    EdgeMutableGraph graph_notplaced;
    unordered_set<int> vertices_active;
    bool do_lower_bound;
    vector<set<int, std::function<bool(const int a, const int b)>>> graph_adj_ordered;
    bool do_graph_adj_ordered;
    bool ignore_right;
    bool opt_components;

    // For Heuristics for updating binary search tree for next decision
    unordered_set<int> changed; // either removed, added, or wreach_size changed

    int target_k = INT_MAX;

    Ordering(OrderedGraph &graph, int r, bool track_full_neighbours, bool do_lower_bound, bool do_graph_adj_ordered, bool ignore_right, bool opt_components);

    /**
     * Puts the vertex at the rightmost position
     **/
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