#include "TurbochargingRL.hpp"

TurbochargerLastC::TurbochargerLastC(OrderingRL &ordering, OrderedGraph &graph) : ordering(ordering), graph(graph)
{
    this->r = ordering.r;
    this->target_k = ordering.target_k;
    this->at = ordering.at;
    this->cnt_nodes = 0;
    this->cnt_depths = 0;
    this->sum_depth = 0;
}

bool TurbochargerLastC::TryContinuous(int i)
{
    this->cnt_nodes++;
    if (i == c) {
        this->cnt_depths++;
        this->sum_depth+=i;
        return ordering.IsExtendable();
    }
        

    vector<int> choices_vec(choices.begin(), choices.end());
    shuffle(choices_vec.begin(), choices_vec.end(), rng);
    bool is_some_extendable = false;
    for (auto v : choices_vec)
    {
        ordering.Place(v);
        if (ordering.IsExtendable())
        {
            is_some_extendable = true;
            choices.erase(v);
            if (TryContinuous(i + 1))
            {
                return true;
            }
            choices.insert(v);
        }
        ordering.UnPlace();
    }
    if (!is_some_extendable) {
        this->cnt_depths++;
        this->sum_depth+=i;
    }
    return false;
}

bool TurbochargerLastC::Turbocharge(int c)
{
    assert(!ordering.IsExtendable());
    unordered_map<int, int> temp;
    choices.clear();
    this->c = c;

    for (int i = at + 1; i < min((int)ordering.ordering.size(), at + c + 1); i++)
    {
        auto u = ordering.ordering[i];
        temp[i] = u;
        ordering.UnPlace();
    }

    BGL_FORALL_VERTICES(v, graph, OrderedGraph)
    {
        if (graph[v].position == NOT_PLACED)
        {
            choices.insert(v);
        }
    }

    bool succ = TryContinuous(0);
    if (!succ)
    {
        // revert changes
        for (int i = min((int)ordering.ordering.size() - 1, at + c + 1) - 1; i > at; i--)
        {
            int u = temp[i];
            ordering.Place(u);
        }
    }
    return succ;
}