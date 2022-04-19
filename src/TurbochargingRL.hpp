#pragma once

#include "Headers.hpp"
#include "OrderingRL.hpp"

class TurbochargerLastC
{
private:
    bool TryContinuous(int i);
    unordered_set<int> choices;

public:
    OrderingRL &ordering;
    OrderedGraph &graph;
    int target_k;
    int at;
    int r;
    int c;
    long long cnt_nodes;
    long long sum_depth;
    long long cnt_depths;
    TurbochargerLastC(OrderingRL &ordering, OrderedGraph &graph);
    bool Turbocharge(int c);
};