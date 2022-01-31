#pragma once

#include "Headers.hpp"
#include "Ordering.hpp"

/**
 * @brief enumerator for branching rule order for tc-lastc, tc-rneigh, and tc-wreach
 *
 */
enum BranchingRule
{
    RandomOrder,
    ByDistance
};

/**
 * @brief Comparator by distance from a vertex
 *
 */
struct CompByDistance
{
    const int problematic_vertex;
    const vector<vector<int>> &distance_matrix;
    CompByDistance(const int problematic_vertex, const vector<vector<int>> &distance_matrix);

    bool operator()(const int a, const int b) const;
};

/**
 * @brief A keeps track of vertices that can be placed in a search tree node during branching
 *
 */
class BranchingSet
{
public:
    virtual void Add(int v) = 0;
    virtual void Rem(int v) = 0;
    virtual vector<int> Get() = 0;
};

/**
 * @brief Branching set that is ordered by a comparator
 *
 */
class BranchingSetOrdered : public BranchingSet
{
private:
    using MySet = set<int, std::function<bool(const int a, const int b)>>;
    MySet s;

public:
    BranchingSetOrdered(vector<int> v, std::function<bool(const int a, const int b)> f);
    void Add(int v);
    void Rem(int v);
    vector<int> Get();
};

/**
 * @brief Unordered branchings set
 *
 */
class BranchingSetUnordered : public BranchingSet
{
private:
    unordered_set<int> s;

public:
    BranchingSetUnordered(vector<int> v);
    void Add(int v);
    void Rem(int v);
    vector<int> Get();
};

unordered_set<int> KFromN(int N, int k, std::mt19937 &gen);

class TurbochargerLastC
{
private:
    bool TryContinuous(int i);

public:
    BranchingSet *free;
    vector<int> positions;
    Ordering &ordering;
    OrderedGraph &graph;
    vector<vector<int>> &distance_matrix;
    int target_k;
    int at;
    int r;
    TurbochargerLastC(Ordering &ordering, OrderedGraph &graph, vector<vector<int>> &distance_matrix);
    bool Turbocharge(int c, bool only_reorder, bool draw_problem, BranchingRule rule);
};

class TurbochargerRNeigh
{
private:
    bool TryNotContinuous(int i);
    bool TryNotContinuousOptimizedOld(int i);
    bool TryNotContinuousOptimized(int i);
    unordered_map<int, int> original_ordering;

public:
    using BranchSet = set<int, std::function<bool(const int a, const int b)>>;
    BranchingSet *free;
    vector<int> positions;
    Ordering &ordering;
    OrderedGraph &graph;
    vector<vector<int>> &distance_matrix;
    int target_k;
    int at;
    int r;
    TurbochargerRNeigh(Ordering &ordering, OrderedGraph &graph, vector<vector<int>> &distance_matrix);
    bool TurbochargeOld(int c, bool only_reorder, bool draw_problem, BranchingRule rule);
    bool Turbocharge(int c, bool only_reorder, bool draw_problem, BranchingRule rule);
};

class TurbochargerWreach
{
private:
    bool TryNotContinuous(int i);
    bool TryNotContinuousOptimizedOld(int i);
    bool TryNotContinuousOptimized(int i);
    unordered_map<int, int> original_ordering;

public:
    using BranchSet = set<int, std::function<bool(const int a, const int b)>>;
    BranchingSet *free;
    vector<int> positions;
    Ordering &ordering;
    OrderedGraph &graph;
    vector<vector<int>> &distance_matrix;
    int target_k;
    int at;
    int r;
    TurbochargerWreach(Ordering &ordering, OrderedGraph &graph, vector<vector<int>> &distance_matrix);
    bool TurbochargeOld(int c, bool only_reorder, bool draw_problem, BranchingRule rule);
    bool Turbocharge(int c, bool only_reorder, bool draw_problem, BranchingRule rule);
};

class TurbochargerSwapNeighbours
{
private:
    int depth_limit;
    bool Try(int depth);
    unordered_map<int, unordered_set<int>> force_right;

public:
    Ordering &ordering;
    OrderedGraph &graph;
    int target_k;
    int at;
    int r;
    TurbochargerSwapNeighbours(Ordering &ordering, OrderedGraph &graph);
    bool Turbocharge(bool draw_problem);
};

class TurbochargerSwapLocalSearch
{
private:
    void SwapLocalSearch(set<pair<int, int>> &q);

public:
    Ordering &ordering;
    OrderedGraph &graph;
    int target_k;
    int at;
    int r;
    TurbochargerSwapLocalSearch(Ordering &ordering, OrderedGraph &graph);
    bool Turbocharge(bool draw_problem);
};

class TurbochargerMerge
{
private:
    bool Try(int i);
    bool Try2(int vright);
    vector<int> hackordering;
    vector<int> tryvertices;
    unordered_set<int> tryverticess;
    unordered_set<int> tryverticessbranch;
    bool Run(unordered_set<int> &vertices);

public:
    int target_k;
    int at;
    int r;
    Ordering &ordering;
    OrderedGraph &graph;
    TurbochargerMerge(Ordering &ordering, OrderedGraph &graph);
    bool Turbocharge(bool draw_problem, int c);
};