#pragma once

#include "Headers.hpp"
class OrderingRL
{
public:
    int at;
    int r;
    int target_k;
    OrderedGraph &graph;
    vector<unordered_set<int>> wreach;
    vector<unordered_set<int>> sreach_wo_wreach;
    vector<vector<pair<int, int>>> sreach_on_place;
    unordered_set<int> free_v;
    unordered_set<int> too_full;
    unordered_set<int> changed;
    bool save_potential_sreach;
    bool lower_bound;
    vector<int> ordering;
    OrderingRL(OrderedGraph &graph, int r, int target_k, bool save_potential_sreach, bool lower_bound);
    vector<int> ReachableInSuborder(const int src, const int maxdist);
    pair<vector<vector<int>>, vector<vector<int>>> LayeredReachableWSreach(const int src, const int maxdist);
    void OnWSReachUpdate(const int v);
    void Place(const int v);
    void UnPlace();
    bool IsExtendable();
    bool IsValidFull();
    void PrintOrdering();
    int Wreach();
    bool IsLBExceed();
};