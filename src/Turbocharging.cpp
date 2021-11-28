#include "Turbocharging.hpp"

CompByDistance::CompByDistance(const int problematic_vertex, const vector<vector<int>> &distance_matrix) : problematic_vertex(problematic_vertex), distance_matrix(distance_matrix) {}

bool CompByDistance::operator()(const int a, const int b) const
{
    if (distance_matrix[problematic_vertex][a] == distance_matrix[problematic_vertex][b])
    {
        return a < b;
    }
    return distance_matrix[problematic_vertex][a] < distance_matrix[problematic_vertex][b];
}

BranchingSetOrdered::BranchingSetOrdered(vector<int> v, std::function<bool(const int a, const int b)> f)
{
    this->s = MySet(v.begin(), v.end(), f);
}

void BranchingSetOrdered::Add(int v)
{
    this->s.insert(v);
}

void BranchingSetOrdered::Rem(int v)
{
    this->s.erase(v);
}

vector<int> BranchingSetOrdered::Get()
{
    return vector<int>(this->s.begin(), this->s.end());
}

BranchingSetUnordered::BranchingSetUnordered(vector<int> v)
{
    this->s = unordered_set<int>(v.begin(), v.end());
}

void BranchingSetUnordered::Add(int v)
{
    this->s.insert(v);
}

void BranchingSetUnordered::Rem(int v)
{
    this->s.erase(v);
}

vector<int> BranchingSetUnordered::Get()
{
    vector<int> v(this->s.begin(), this->s.end());
    shuffle(v.begin(), v.end(), rng);
    return v;
}
/** Algorithm by Floyd
 * Generates k random values between 1 and N inclusive
 */
unordered_set<int> KFromN(int N, int k, std::mt19937 &gen)
{
    unordered_set<int> elems;
    for (int r = N - k + 1; r <= N; ++r)
    {
        int v = uniform_int_distribution<>(1, r)(gen);

        // there are two cases.
        // v is not in candidates ==> add it
        // v is in candidates ==> well, r is definitely not, because
        // this is the first iteration in the loop that we could've
        // picked something that big.

        if (!elems.insert(v).second)
        {
            elems.insert(r);
        }
    }
    return elems;
}

TurbochargerLastC::TurbochargerLastC(Ordering &ordering, OrderedGraph &graph, vector<vector<int>> &distance_matrix) : ordering(ordering), graph(graph), distance_matrix(distance_matrix)
{
    this->r = ordering.r;
    this->target_k = ordering.target_k;
    this->at = ordering.at;
}

// placing vertices recursively in ordering
// free vertices have to be a continuous region starting at the end of the subordering
// weakly reachable sets of graphs have to be maintained correctly corresponding to the ordering
bool TurbochargerLastC::TryContinuous(int i)
{
    if (i >= positions.size())
        return true;
    int pos = positions[i];

    vector<int> choices;
    choices = free->Get();

    if (ordering.opt_components)
    {
        int vcomponent = -1;
        if (ordering.at > 0)
        {
            int v = ordering.ordering[ordering.at - 1];
            for (auto w : boost::make_iterator_range(boost::adjacent_vertices(v, graph)))
            {
                if (graph[w].position == NOT_PLACED)
                {
                    vcomponent = w;
                    break;
                }
            }
        }
        if (vcomponent == -1)
        {
            vcomponent = *ordering.vertices_active.begin();
        }
        vector<int> component = ordering.ComponentNotPlaced(vcomponent);
        unordered_set<int> scomponent(component.begin(), component.end());
        vector<int> choices_actual;
        for (auto v : choices)
        {
            if (scomponent.find(v) != scomponent.end())
            {
                choices_actual.push_back(v);
            }
        }
        choices = choices_actual;
    }

    for (int j = 0; j < choices.size(); j++)
    {
        auto u = choices[j];

        ordering.Place(u);
        if (ordering.IsExtendable())
        {
            free->Rem(u);
            if (TryContinuous(i + 1))
            {
                return true;
            }
            free->Add(u);
        }
        ordering.UnPlace();
    }
    return false;
}

bool TurbochargerLastC::Turbocharge(int c, bool only_reorder, bool draw_problem, BranchingRule rule)
{
    assert(!ordering.IsExtendable());
    unordered_map<int, int> temp;
    positions.clear(); // positions we want to branch on
    int problematic_vertex = ordering.ordering[at - 1];

    vector<int> freevec;

    for (int i = at - 1; i >= max(0, at - c); i--)
    {
        auto u = ordering.ordering[i];
        freevec.push_back(u);
        positions.push_back(i);
        temp[i] = u;
        ordering.UnPlace();
    }
    reverse(positions.begin(), positions.end());

    if (!only_reorder)
    {
        // branch on all possible vertices
        // TODO: this can be optimized
        for (auto v : ordering.vertices_active)
        {
            if (graph[v].position == NOT_PLACED)
            {
                freevec.push_back(v);
            }
        }
    }

    if (draw_problem)
    {
        //GraphDrawer::getInstance().DrawGraph(graph);
    }

    if (rule == RandomOrder)
    {
        free = new BranchingSetUnordered(freevec);
    }
    else if (rule = ByDistance)
    {
        free = new BranchingSetOrdered(freevec, CompByDistance(problematic_vertex, distance_matrix));
    }

    bool succ = TryContinuous(0);
    if (!succ)
    {
        // revert changes
        for (int i = max(0, at - c); i < at; i++)
        {
            int u = temp[i];
            ordering.Place(u);
        }
    }
    return succ;
}

TurbochargerRNeigh::TurbochargerRNeigh(Ordering &ordering, OrderedGraph &graph, vector<vector<int>> &distance_matrix) : ordering(ordering), graph(graph), distance_matrix(distance_matrix)
{
    this->r = ordering.r;
    this->target_k = ordering.target_k;
    this->at = ordering.at;
}

bool TurbochargerRNeigh::TryNotContinuous(int i)
{
    if (i >= positions.size())
        return true;
    int pos = positions[i];

    vector<int> choices = free->Get();

    for (auto u : choices)
    {
        // try placing u at current position and then also placing all vertices until the next position from the original ordering
        int to = pos;
        if (i < positions.size() - 1)
        {
            // there is still a position left
            to = positions[i + 1];
        }
        else
        {
            // go as far as vertices were placed in the original ordering
            do
            {
                to++;
            } while (to < ordering.ordering.size() && original_ordering.find(to) != original_ordering.end());
        }

        bool ok = true;
        int posi;
        for (posi = pos; posi < to; posi++)
        {
            int v = posi == pos ? u : original_ordering[posi];
            ordering.Place(v);
            if (!ordering.IsExtendable())
            {
                ok = false;
                ordering.UnPlace();
                break;
            }
        }
        posi--; // at this position no placement was done
        if (ok)
        {
            free->Rem(u);
            if (TryNotContinuous(i + 1))
            {
                return true;
            }
            free->Add(u);
        }
        while (posi >= pos)
        {
            ordering.UnPlace();
            posi--;
        }
    }

    return false;
}

bool TurbochargerRNeigh::TryNotContinuousOptimizedOld(int i)
{
    if (i >= positions.size())
        return this->ordering.IsExtendable();
    int pos = positions[i];

    vector<int> choices = free->Get();

    int to = pos;
    if (i < positions.size() - 1)
    {
        // there is still a position left
        to = positions[i + 1];
    }
    else
    {
        // go as far as vertices were placed in the original ordering
        do
        {
            to++;
        } while (to < ordering.ordering.size() && original_ordering.find(to) != original_ordering.end());
    }

    for (int j = 0; j < choices.size(); j++)
    {
        int u = choices[j];

        if (j == 0)
        {
            for (int posi = pos; posi < to; posi++)
            {
                int v = posi == pos ? u : original_ordering[posi];
                ordering.Place(v);
            }
        }
        else
        {
            ordering.Swap(choices[j - 1], u);
        }

        if (ordering.IsExtendable())
        {
            free->Rem(u);
            if (TryNotContinuousOptimizedOld(i + 1))
            {
                return true;
            }
            free->Add(u);
        }

        if (j == choices.size() - 1)
        {
            for (int posi = to - 1; posi >= pos; posi--)
            {
                ordering.UnPlace();
            }
        }
    }

    return false;
}

bool TurbochargerRNeigh::TryNotContinuousOptimized(int i)
{
    if (i >= positions.size())
        return this->ordering.IsExtendable();
    int pos = positions[i];

    vector<int> choices = free->Get();
    unordered_set<int> choices_s(choices.begin(), choices.end());
    // check if weakly reachable set of choices is ok (it can be temporarily too full)
    for (auto u : choices)
    {
        if (graph[u].wreach_sz > target_k)
        {
            int cnt_wreach_here = 1;
            for (auto v : ordering.wreach[u])
            {
                if (graph[v].position < pos)
                    cnt_wreach_here++;
            }
            if (cnt_wreach_here > target_k)
                return false;
        }
    }
    for (auto u : choices)
    {
        this->ordering.Place(u, pos);
        bool ok = true;
        for (auto v : ordering.is_too_full)
        {
            if (choices_s.find(v) == choices_s.end())
                ok = false;
        }
        if (ok)
        {
            free->Rem(u);
            if (this->TryNotContinuousOptimized(i + 1))
            {
                return true;
            }
            free->Add(u);
        }
        this->ordering.Unplace(pos);
    }

    return false;
}

bool TurbochargerRNeigh::Turbocharge(int c, bool only_reorder, bool draw_problem, BranchingRule rule)
{
    assert(!ordering.IsExtendable());
    int vertex = ordering.ordering[at - 1];
    unordered_set<int> rneighbourhood;
    if (ordering.is_too_full.size() == 0)
    {
        // local search had lower bound
        vector<int> rreachable = ordering.RReachable(ordering.ordering[ordering.at - 1]);
        rneighbourhood.insert(rreachable.begin(), rreachable.end());
    }
    for (auto v : ordering.is_too_full)
    {
        vector<int> rreachable = ordering.RReachable(v);
        rneighbourhood.insert(rreachable.begin(), rreachable.end());
    }
    vector<int> positions_rneighbourhood;
    for (auto v : rneighbourhood)
    {
        if (graph[v].position != NOT_PLACED)
        {
            positions_rneighbourhood.push_back(graph[v].position);
        }
    }
    for (int ii = 0; ii < 10; ii++)
    {
        positions.clear(); // positions we want to branch on
        original_ordering.clear();
        // take the last vertices of ordering in r-neighourhood
        // sort(positions_rneighbourhood.begin(), positions_rneighbourhood.end(), greater<int>());
        // take some random vertices of the rneighbourhood
        shuffle(positions_rneighbourhood.begin(), positions_rneighbourhood.end(), rng);
        vector<int> freevec;
        // unordered_set<int> free;

        c = min(c, (int)positions_rneighbourhood.size());
        for (int i = 0; i < c; i++)
        {
            positions.push_back(positions_rneighbourhood[i]);
            freevec.push_back(ordering.ordering[positions_rneighbourhood[i]]);
        }
        if (!only_reorder)
        {
            for (auto v : ordering.vertices_active)
            {
                if (graph[v].position == NOT_PLACED)
                {
                    freevec.push_back(v);
                }
            }
        }
        // now that we determined the positions to branch on, sort them
        sort(positions.begin(), positions.end());

        // now backtrack until first of these positions
        for (int i = (int)positions.size() - 1; i >= 0; i--)
        {
            original_ordering[positions[i]] = this->ordering.ordering[positions[i]];
            ordering.Unplace(positions[i]);
        }

        if (rule == RandomOrder)
        {
            free = new BranchingSetUnordered(freevec);
        }
        else if (rule == ByDistance)
        {
            free = new BranchingSetOrdered(freevec, CompByDistance(vertex, distance_matrix));
        }
        // bool succ = TryNotContinuous(0);
        bool succ = TryNotContinuousOptimized(0);

        if (!succ)
        {
            // revert changes
            for (auto pos : positions)
            {
                ordering.Place(original_ordering[pos], pos);
            }
        }
        else
        {
            return true;
        }
    }

    return false;
}

bool TurbochargerRNeigh::TurbochargeOld(int c, bool only_reorder, bool draw_problem, BranchingRule rule)
{
    assert(!ordering.IsExtendable());
    positions.clear(); // positions we want to branch on
    original_ordering.clear();
    int vertex = ordering.ordering[at - 1];
    unordered_set<int> rneighbourhood;
    if (ordering.is_too_full.size() == 0)
    {
        // local search had lower bound
        vector<int> rreachable = ordering.RReachable(ordering.ordering[ordering.at - 1]);
        rneighbourhood.insert(rreachable.begin(), rreachable.end());
    }
    for (auto v : ordering.is_too_full)
    {
        vector<int> rreachable = ordering.RReachable(v);
        rneighbourhood.insert(rreachable.begin(), rreachable.end());
    }
    vector<int> positions_rneighbourhood;
    for (auto v : rneighbourhood)
    {
        if (graph[v].position != NOT_PLACED)
        {
            positions_rneighbourhood.push_back(graph[v].position);
        }
    }
    // take the last vertices of ordering in r-neighourhood
    // sort(positions_rneighbourhood.begin(), positions_rneighbourhood.end(), greater<int>());
    // take some random vertices of the rneighbourhood
    shuffle(positions_rneighbourhood.begin(), positions_rneighbourhood.end(), rng);
    vector<int> freevec;
    // unordered_set<int> free;

    c = min(c, (int)positions_rneighbourhood.size());
    for (int i = 0; i < c; i++)
    {
        positions.push_back(positions_rneighbourhood[i]);
        freevec.push_back(ordering.ordering[positions_rneighbourhood[i]]);
    }
    if (!only_reorder)
    {
        for (auto v : ordering.vertices_active)
        {
            if (graph[v].position == NOT_PLACED)
            {
                freevec.push_back(v);
            }
        }
    }
    // now that we determined the positions to branch on, sort them
    sort(positions.begin(), positions.end());

    // now backtrack until first of these positions
    for (int i = at - 1; i >= positions[0]; i--)
    {
        int u = ordering.ordering[i];
        original_ordering[i] = u;

        ordering.UnPlace();
    }

    if (rule == RandomOrder)
    {
        free = new BranchingSetUnordered(freevec);
    }
    else if (rule = ByDistance)
    {
        free = new BranchingSetOrdered(freevec, CompByDistance(vertex, distance_matrix));
    }
    bool succ = TryNotContinuousOptimizedOld(0);

    if (!succ)
    {
        // revert changes
        for (int i = positions[0]; i < at; i++)
        {
            ordering.Place(original_ordering[i]);
        }
    }
    return succ;
}

TurbochargerWreach::TurbochargerWreach(Ordering &ordering, OrderedGraph &graph, vector<vector<int>> &distance_matrix) : ordering(ordering), graph(graph), distance_matrix(distance_matrix)
{
    this->r = ordering.r;
    this->target_k = ordering.target_k;
    this->at = ordering.at;
}

bool TurbochargerWreach::TryNotContinuous(int i)
{
    if (i >= positions.size())
        return true;
    int pos = positions[i];

    vector<int> choices = free->Get();

    for (auto u : choices)
    {
        // try placing u at current position and then also placing all vertices until the next position from the original ordering
        int to = pos;
        if (i < positions.size() - 1)
        {
            // there is still a position left
            to = positions[i + 1];
        }
        else
        {
            // go as far as vertices were placed in the original ordering
            do
            {
                to++;
            } while (to < ordering.ordering.size() && original_ordering.find(to) != original_ordering.end());
        }

        bool ok = true;
        int posi;
        for (posi = pos; posi < to; posi++)
        {
            int v = posi == pos ? u : original_ordering[posi];
            ordering.Place(v);
            if (!ordering.IsExtendable())
            {
                ok = false;
                ordering.UnPlace();
                break;
            }
        }
        posi--; // at this position no placement was done
        if (ok)
        {
            free->Rem(u);
            if (TryNotContinuous(i + 1))
            {
                return true;
            }
            free->Add(u);
        }
        while (posi >= pos)
        {
            ordering.UnPlace();
            posi--;
        }
    }

    return false;
}

bool TurbochargerWreach::TryNotContinuousOptimizedOld(int i)
{
    if (i >= positions.size())
        return this->ordering.IsExtendable();
    int pos = positions[i];

    vector<int> choices = free->Get();

    int to = pos;
    if (i < positions.size() - 1)
    {
        // there is still a position left
        to = positions[i + 1];
    }
    else
    {
        // go as far as vertices were placed in the original ordering
        do
        {
            to++;
        } while (to < ordering.ordering.size() && original_ordering.find(to) != original_ordering.end());
    }

    for (int j = 0; j < choices.size(); j++)
    {
        int u = choices[j];

        if (j == 0)
        {
            for (int posi = pos; posi < to; posi++)
            {
                int v = posi == pos ? u : original_ordering[posi];
                ordering.Place(v);
            }
        }
        else
        {
            ordering.Swap(choices[j - 1], u);
        }

        if (ordering.IsExtendable())
        {
            free->Rem(u);
            if (TryNotContinuousOptimizedOld(i + 1))
            {
                return true;
            }
            free->Add(u);
        }

        if (j == choices.size() - 1)
        {
            for (int posi = to - 1; posi >= pos; posi--)
            {
                ordering.UnPlace();
            }
        }
    }

    return false;
}

bool TurbochargerWreach::TryNotContinuousOptimized(int i)
{
    if (i >= positions.size())
        return this->ordering.IsExtendable();
    int pos = positions[i];

    vector<int> choices = free->Get();
    unordered_set<int> choices_s(choices.begin(), choices.end());
    // check if weakly reachable set of choices is ok (it can be temporarily too full)
    for (auto u : choices)
    {
        if (graph[u].wreach_sz > target_k)
        {
            int cnt_wreach_here = 1;
            for (auto v : ordering.wreach[u])
            {
                if (graph[v].position < pos)
                    cnt_wreach_here++;
            }
            if (cnt_wreach_here > target_k)
                return false;
        }
    }
    for (auto u : choices)
    {
        this->ordering.Place(u, pos);
        bool ok = true;
        for (auto v : ordering.is_too_full)
        {
            if (choices_s.find(v) == choices_s.end())
                ok = false;
        }
        if (ok)
        {
            free->Rem(u);
            if (this->TryNotContinuousOptimized(i + 1))
            {
                return true;
            }
            free->Add(u);
        }
        this->ordering.Unplace(pos);
    }

    return false;
}

bool TurbochargerWreach::TurbochargeOld(int c, bool only_reorder, bool draw_problem, BranchingRule rule)
{
    assert(!ordering.IsExtendable());
    positions.clear(); // positions we want to branch on
    original_ordering.clear();
    int vertex = ordering.ordering[at - 1];
    unordered_set<int> wreach_union;
    if (ordering.is_too_full.size() == 0)
    {
        // local search had lower bound
        int v = ordering.ordering[ordering.at - 1];
        wreach_union.insert(ordering.wreach[v].begin(), ordering.wreach[v].end());
    }
    for (auto v : ordering.is_too_full)
    {
        wreach_union.insert(ordering.wreach[v].begin(), ordering.wreach[v].end());
    }
    vector<int> positions_wreach;
    for (auto v : wreach_union)
    {
        if (graph[v].position != NOT_PLACED)
        {
            positions_wreach.push_back(graph[v].position);
        }
    }
    // take the last vertices of ordering in r-neighourhood
    // sort(positions_wreach.begin(), positions_wreach.end(), greater<int>());
    // take some random vertices of the wreach
    shuffle(positions_wreach.begin(), positions_wreach.end(), rng);
    vector<int> freevec;
    // unordered_set<int> free;

    c = min(c, (int)positions_wreach.size());
    for (int i = 0; i < c; i++)
    {
        positions.push_back(positions_wreach[i]);
        freevec.push_back(ordering.ordering[positions_wreach[i]]);
    }
    if (!only_reorder)
    {
        for (auto v : ordering.vertices_active)
        {
            if (graph[v].position == NOT_PLACED)
            {
                freevec.push_back(v);
            }
        }
    }
    // now that we determined the positions to branch on, sort them
    sort(positions.begin(), positions.end());

    // now backtrack until first of these positions
    for (int i = at - 1; i >= positions[0]; i--)
    {
        int u = ordering.ordering[i];
        original_ordering[i] = u;

        ordering.UnPlace();
    }

    if (rule == RandomOrder)
    {
        free = new BranchingSetUnordered(freevec);
    }
    else if (rule = ByDistance)
    {
        free = new BranchingSetOrdered(freevec, CompByDistance(vertex, distance_matrix));
    }
    // bool succ = TryNotContinuous(0);
    bool succ = TryNotContinuousOptimizedOld(0);

    if (!succ)
    {
        // revert changes
        for (int i = positions[0]; i < at; i++)
        {
            ordering.Place(original_ordering[i]);
        }
    }
    return succ;
}

bool TurbochargerWreach::Turbocharge(int c, bool only_reorder, bool draw_problem, BranchingRule rule)
{
    assert(!ordering.IsExtendable());
    int vertex = ordering.ordering[at - 1];
    unordered_set<int> wreach_union;
    if (ordering.is_too_full.size() == 0)
    {
        // local search had lower bound
        int v = ordering.ordering[ordering.at - 1];
        wreach_union.insert(ordering.wreach[v].begin(), ordering.wreach[v].end());
    }
    for (auto v : ordering.is_too_full)
    {
        wreach_union.insert(ordering.wreach[v].begin(), ordering.wreach[v].end());
    }
    vector<int> positions_wreach;
    for (auto v : wreach_union)
    {
        if (graph[v].position != NOT_PLACED)
        {
            positions_wreach.push_back(graph[v].position);
        }
    }
    for (int ii = 0; ii < 10; ii++)
    {
        positions.clear(); // positions we want to branch on
        original_ordering.clear();
        // take the last vertices of ordering in r-neighourhood
        // sort(positions_rneighbourhood.begin(), positions_rneighbourhood.end(), greater<int>());
        // take some random vertices of the rneighbourhood
        shuffle(positions_wreach.begin(), positions_wreach.end(), rng);
        vector<int> freevec;
        // unordered_set<int> free;

        c = min(c, (int)positions_wreach.size());
        for (int i = 0; i < c; i++)
        {
            positions.push_back(positions_wreach[i]);
            freevec.push_back(ordering.ordering[positions_wreach[i]]);
        }
        if (!only_reorder)
        {
            for (auto v : ordering.vertices_active)
            {
                if (graph[v].position == NOT_PLACED)
                {
                    freevec.push_back(v);
                }
            }
        }
        // now that we determined the positions to branch on, sort them
        sort(positions.begin(), positions.end());

        // now backtrack until first of these positions
        for (int i = (int)positions.size() - 1; i >= 0; i--)
        {
            original_ordering[positions[i]] = this->ordering.ordering[positions[i]];
            ordering.Unplace(positions[i]);
        }

        if (rule == RandomOrder)
        {
            free = new BranchingSetUnordered(freevec);
        }
        else if (rule == ByDistance)
        {
            free = new BranchingSetOrdered(freevec, CompByDistance(vertex, distance_matrix));
        }
        // bool succ = TryNotContinuous(0);
        bool succ = TryNotContinuousOptimized(0);

        if (!succ)
        {
            // revert changes
            for (auto pos : positions)
            {
                ordering.Place(original_ordering[pos], pos);
            }
        }
        else
        {
            return true;
        }
    }

    return false;
}

TurbochargerSwapNeighbours::TurbochargerSwapNeighbours(Ordering &ordering, OrderedGraph &graph) : ordering(ordering), graph(graph)
{
    this->at = ordering.at;
    assert(this->at > 0);
}

bool TurbochargerSwapNeighbours::Try(int depth)
{
    if (ordering.IsExtendable())
        return true;

    if (depth >= depth_limit)
        return false;

    vector<int> toofull(ordering.is_too_full.begin(), ordering.is_too_full.end());
    int v = toofull[rand() % toofull.size()];
    unordered_set<int> choices_first;
    unordered_set<int> choices_second;
    assert(graph[v].position > 0);
    int vleft = graph[v].position == NOT_PLACED ? ordering.ordering[at - 1] : ordering.ordering[graph[v].position - 1];

    if (force_right.find(vleft) == force_right.end() || force_right[vleft].find(v) == force_right[vleft].end())
    {
        choices_first.insert(v);
    }

    for (int i = graph[vleft].position; i > 0; i--)
    {
        if (force_right.find(ordering.ordering[i - 1]) == force_right.end() || force_right[ordering.ordering[i - 1]].find(ordering.ordering[i]) == force_right[ordering.ordering[i - 1]].end())
        {
            choices_second.insert(ordering.ordering[i]);
            break;
        }
    }

    // cout << ordering.is_too_full.size() << " " << choices_first.size() << " " << choices_second.size() << endl;

    for (auto v : choices_first)
    {
        int vleft = graph[v].position == NOT_PLACED ? ordering.ordering[at - 1] : ordering.ordering[graph[v].position - 1];
        force_right[v].insert(vleft);
        ordering.Swap(vleft, v);
        if (Try(depth + 1))
        {
            return true;
        }
        ordering.Swap(v, vleft);
        force_right[v].erase(vleft);
    }
    for (auto v : choices_second)
    {
        int vleft = graph[v].position == NOT_PLACED ? ordering.ordering[at - 1] : ordering.ordering[graph[v].position - 1];
        force_right[v].insert(vleft);
        ordering.Swap(vleft, v);
        if (Try(depth + 1))
        {
            return true;
        }
        ordering.Swap(v, vleft);
        force_right[v].erase(vleft);
    }
    return false;
}

bool TurbochargerSwapNeighbours::Turbocharge(bool draw_problem)
{
    if (draw_problem)
    {
        //GraphDrawer::getInstance().DrawGraph(graph);
    }

    for (int d = 1; d < 100; d++)
    {
        debug(d);
        this->depth_limit = d;
        if (Try(0))
        {
            return true;
        }
    }
    return false;
}

TurbochargerSwapLocalSearch::TurbochargerSwapLocalSearch(Ordering &ordering, OrderedGraph &graph) : ordering(ordering), graph(graph)
{
    this->at = ordering.at;
    this->target_k = ordering.target_k;
    this->r = ordering.r;
    assert(this->at > 0);
}

bool TurbochargerSwapLocalSearch::Try(int depth)
{
    if (ordering.IsExtendable())
        return true;

    if (depth >= depth_limit)
        return false;

    int v = *ordering.is_too_full.begin();

    int right_index = graph[v].position == NOT_PLACED ? at - 1 : graph[v].position - 1;
    int left_index = 0;

    /*for (auto w : force_left[v])
    {
        if (graph[w].position == NOT_PLACED)
        {
            left_index = right_index + 1;
        }
        else
        {
            left_index = max(left_index, graph[w].position + 1);
        }
    }
    if (right_index >= left_index)
    {
        int k_samples = min(right_index - left_index + 1, this->branching_factor);
        unordered_set<int> samples_s = KFromN(right_index - left_index + 1, k_samples, rng);
        vector<int> samples;
        for (auto sample : samples_s)
            samples.push_back(sample - 1 + left_index);
        shuffle(samples.begin(), samples.end(), rng);

        for (auto sample : samples)
        {
            int w = ordering.ordering[sample];
            ordering.Swap(w, v);
            if (graph[v].wreach_sz <= target_k)
            {
                force_left[w].insert(v);
                if (Try(depth + 1))
                {
                    return true;
                }
                force_left[w].erase(v);
            }
            ordering.Swap(v, w);
        }
    }*/

    int ind = (rand() % (right_index - left_index + 1)) + left_index;
    if (depth > 50)
    {
        ind = right_index;
    }
    int w = ordering.ordering[ind];
    ordering.Swap(w, v);
    if (Try(depth + 1))
    {
        return true;
    }
    ordering.Swap(v, w);
    return false;
}

void TurbochargerSwapLocalSearch::SwapLocalSearch(set<pair<int, int>> &q)
{
    int SWAPS = 20;

    int i = 0;
    while (!ordering.IsExtendable() && i < SWAPS)
    {
        int v;
        if (ordering.is_too_full.size() > 0)
        {
            auto it = ordering.is_too_full.begin();
            int rnd = rand() % ordering.is_too_full.size();
            while (rnd--)
            {
                it = next(it);
            }
            v = *it;
        }
        else
        {
            auto it = prev(q.end());
            int rnd = rand() % min(10, (int)q.size());
            while (rnd)
            {
                rnd--;
                it = prev(it);
            }
            v = it->second;
        }
        if (graph[v].position == 0)
            continue;
        int leftind = graph[v].position == NOT_PLACED ? at - 1 : graph[v].position - 1;
        int pos = rand() % (leftind + 1);
        ordering.Swap(ordering.ordering[pos], v);
        i++;
    }
}

bool TurbochargerSwapLocalSearch::Turbocharge(bool draw_problem)
{
    this->force_left.clear();
    if (draw_problem)
    {
        //GraphDrawer::getInstance().DrawGraph(graph);
    }

    unordered_set<int> really_changed;
    set<pair<int, int>> q;
    vector<int> wreach_in_q(boost::num_vertices(graph));
    BGL_FORALL_VERTICES(v, graph, OrderedGraph)
    {
        q.insert({graph[v].wreach_sz, v});
        wreach_in_q[v] = graph[v].wreach_sz;
    }

    int best = ordering.is_too_full.size();
    int steps_from_imp = 0;
    int STEPS_UNTIL_RANDOM = 50;
    while (!this->ordering.IsExtendable())
    {
        if (ordering.is_too_full.size() < best)
        {
            best = ordering.is_too_full.size();
            steps_from_imp = 0;
        }
        if (steps_from_imp > STEPS_UNTIL_RANDOM)
        {
            this->SwapLocalSearch(q);
            for (auto w : ordering.changed)
            {
                q.erase({wreach_in_q[w], w});
                q.insert({graph[w].wreach_sz, w});
                wreach_in_q[w] = graph[w].wreach_sz;
            }
            really_changed.insert(ordering.changed.begin(), ordering.changed.end());
            ordering.changed.clear();
            if (ordering.IsExtendable())
                break;
            best = ordering.is_too_full.size();
            steps_from_imp = 0;
        }
        //cout << this->ordering.is_too_full.size() << endl;
        auto it = prev(q.end());
        int rs = rand() % (min(10, (int)boost::num_vertices(graph)));
        for (int i = 0; i < rs; i++)
        {
            it = prev(it);
        }

        int v = it->second;
        int target = max(1, graph[v].wreach_sz - 1 - rand() % 3);
        //cout << v << " " << graph[v].wreach_sz << endl;
        while (graph[v].wreach_sz > target)
        {
            int leftind = graph[v].position == NOT_PLACED ? at - 1 : graph[v].position - 1;
            ordering.Swap(ordering.ordering[leftind], v);
        }
        for (auto w : ordering.changed)
        {
            q.erase({wreach_in_q[w], w});
            q.insert({graph[w].wreach_sz, w});
            wreach_in_q[w] = graph[w].wreach_sz;
        }
        really_changed.insert(ordering.changed.begin(), ordering.changed.end());
        ordering.changed.clear();
        //cout << v << " " << graph[v].wreach_sz << endl;
        steps_from_imp++;
    }
    ordering.changed.insert(really_changed.begin(), really_changed.end());
    return true;
}

TurbochargerMerge::TurbochargerMerge(Ordering &ordering, OrderedGraph &graph) : ordering(ordering), graph(graph)
{
    this->at = ordering.at;
    this->r = ordering.r;
    this->target_k = ordering.target_k;
}

bool TurbochargerMerge::Try(int i)
{
    if (i >= tryvertices.size() && ordering.IsExtendable())
        return true;
    int vertex = tryvertices[i];
    unordered_set<pair<int, int>, boost::hash<std::pair<int, int>>> wreach_addedd;

    while (graph[vertex].wreach_sz < target_k)
    {
        int nextpos = -1;
        if (graph[vertex].position == -1)
            nextpos = 0;
        else
        {
            int minpos = INT_MAX;
            bool found = false;
            for (auto w : ordering.wreach_inv[vertex])
            {
                if (w != vertex && graph[w].position >= graph[vertex].position && graph[w].position < minpos)
                {
                    minpos = graph[w].position;
                    found = true;
                }
            }
            if (found)
                nextpos = minpos + 1;
        }
        if (nextpos == -1)
            break;

        int posi = nextpos;
        int temp = vertex;
        while (hackordering[posi] != -1)
        {
            int tempp = hackordering[posi];
            hackordering[posi] = temp;
            ordering.ChangePosition(temp, posi);
            temp = tempp;
            posi++;
        }
        hackordering[posi] = temp;
        ordering.ChangePosition(temp, posi);

        ordering.UpdateWreachHelper(vertex, wreach_addedd);
        if (nextpos != 0)
        {
            ordering.UpdateWreachHelper(hackordering[nextpos - 1], wreach_addedd);
        }
        if (ordering.IsExtendable())
        {
            if (Try(i + 1))
            {
                return true;
            }
        }
        posi = nextpos;
        hackordering[posi] = -1;
        while (posi < hackordering.size() - 1 &&
               hackordering[posi + 1] != -1 &&
               tryverticess.find(hackordering[posi + 1]) != tryverticess.end())
        {
            hackordering[posi] = hackordering[posi + 1];
            hackordering[posi + 1] = -1;
            ordering.ChangePosition(hackordering[posi], posi);
            posi++;
        }
    }
    ordering.ChangePosition(vertex, -1);
    for (auto p : wreach_addedd)
    {
        ordering.WreachRem(p.first, p.second);
    }
    return false;
}

bool TurbochargerMerge::Try2(int vright)
{
    if (tryverticessbranch.size() == 0 && ordering.IsExtendable())
        return true;
    for (auto vertex : vector<int>(tryverticessbranch.begin(), tryverticessbranch.end()))
    {
        unordered_set<pair<int, int>, boost::hash<std::pair<int, int>>> wreach_addedd;
        tryverticessbranch.erase(vertex);
        while (graph[vertex].wreach_sz < target_k)
        {
            int nextpos = -1;
            if (graph[vertex].position == -1)
                nextpos = 0;
            else
            {
                int minpos = INT_MAX;
                bool found = false;
                for (auto w : ordering.wreach_inv[vertex])
                {
                    if (w != vertex && graph[w].position >= graph[vertex].position && graph[w].position < minpos)
                    {
                        minpos = graph[w].position;
                        found = true;
                    }
                }
                if (found)
                    nextpos = minpos + 1;
            }
            if (nextpos == -1 || (vright != -1 && graph[vright].position < nextpos))
                break;

            ordering.ChangePosition(vertex, nextpos);
            ordering.UpdateWreachHelper(vertex, wreach_addedd);
            if (nextpos != 0)
            {
                ordering.UpdateWreachHelper(hackordering[nextpos - 1], wreach_addedd);
            }
            // compute actual next position that is left of the leftmost vertex in wreachinv
            int minpos = INT_MAX;
            for (auto w : ordering.wreach_inv[vertex])
            {
                if (w != vertex && graph[w].position >= graph[vertex].position && graph[w].position < minpos)
                {
                    minpos = graph[w].position;
                }
            }
            nextpos = minpos != INT_MAX ? minpos - 1 : hackordering.size() - 1;
            if (vright != -1 && nextpos >= graph[vright].position)
            {
                break;
            }

            int posi = nextpos;
            int temp = vertex;
            while (hackordering[posi] != -1)
            {
                int tempp = hackordering[posi];
                hackordering[posi] = temp;
                ordering.ChangePosition(temp, posi);
                temp = tempp;
                posi--;
            }
            hackordering[posi] = temp;
            ordering.ChangePosition(temp, posi);

            if (ordering.IsExtendable())
            {
                if (Try2(vertex))
                {
                    return true;
                }
            }
            posi = nextpos;
            hackordering[posi] = -1;
            while (posi > 0 &&
                   hackordering[posi - 1] != -1 &&
                   tryverticess.find(hackordering[posi - 1]) != tryverticess.end())
            {
                hackordering[posi] = hackordering[posi - 1];
                hackordering[posi - 1] = -1;
                ordering.ChangePosition(hackordering[posi], posi);
                posi--;
            }
        }
        ordering.ChangePosition(vertex, -1);
        for (auto p : wreach_addedd)
        {
            ordering.WreachRem(p.first, p.second);
        }
        tryverticessbranch.insert(vertex);
    }
    return false;
}

bool TurbochargerMerge::Run(unordered_set<int> &vertices)
{
    vector<int> oldordering(ordering.ordering.begin(), ordering.ordering.end());
    vector<int> tempordering;

    /*int cnt_not_in_ordering = 0;
    for (auto v : vertices)
    {
        cnt_not_in_ordering++;
    }*/

    for (auto v : ordering.ordering)
    {
        if (v != -1 && vertices.find(v) == vertices.end())
        {
            tempordering.push_back(v);
        }
    }

    /*for (int i = 0; i < cnt_not_in_ordering && tempordering.size() > 0; i++)
    {
        tempordering.pop_back();
    }*/

    ordering.is_too_full.clear();
    for (auto v : boost::make_iterator_range(boost::vertices(graph)))
    {
        ordering.wreach[v].clear();
        ordering.wreach[v].insert(v);
        ordering.wreach_inv[v].clear();
        ordering.wreach_inv[v].insert(v);
        if (graph[v].position != NOT_PLACED)
        {
            ordering.AddVertex(v);
        }
        graph[v].wreach_sz = 1;
        ordering.ChangePosition(v, NOT_PLACED);
    }

    for (auto v : vertices)
    {
        ordering.ChangePosition(v, -1);
        ordering.RemoveVertex(v);
    }
    hackordering = vector<int>(vertices.size() * (tempordering.size() + 1) + tempordering.size(), -1);
    for (int i = 0; i < tempordering.size(); i++)
    {
        ordering.ChangePosition(tempordering[i], (i + 1) * vertices.size() + i);
        hackordering[(i + 1) * vertices.size() + i] = tempordering[i];
        ordering.UpdateWreach(tempordering[i]);
        ordering.RemoveVertex(tempordering[i]);
    }
    for (auto v : vertices)
    {
        // we know part of wreachinv of vertices that will not be placed
        ordering.ChangePosition(v, hackordering.size());
        ordering.UpdateWreach(v);
        ordering.ChangePosition(v, -1);
    }

    tryvertices = vector<int>(vertices.begin(), vertices.end());
    shuffle(tryvertices.begin(), tryvertices.end(), rng);
    tryverticess = unordered_set<int>(vertices.begin(), vertices.end());
    tryverticessbranch = unordered_set<int>(vertices.begin(), vertices.end());

    bool succ = Try2(-1);
    if (!succ)
        ordering.ResetTo(oldordering);
    else
    {
        int i = 0;
        for (auto v : hackordering)
        {
            if (v != -1)
            {
                ordering.ordering[i] = v;
                ordering.ChangePosition(v, i);
                i++;
            }
        }
        ordering.at = vertices.size() + tempordering.size();
    }
    return succ;
}

bool TurbochargerMerge::Turbocharge(bool draw_problem, int c)
{
    bool always_take_too_full = ordering.ignore_right;

    if (draw_problem)
    {
        //GraphDrawer::getInstance().DrawGraph(graph);
    }

    assert(!ordering.IsExtendable());
    unordered_set<int> wreach_union;

    if (ordering.is_too_full.size() == 0)
    {
        // local search had lower bound
        int v = ordering.ordering[ordering.at - 1];
        wreach_union.insert(ordering.wreach[v].begin(), ordering.wreach[v].end());
    }
    for (auto v : ordering.is_too_full)
    {
        //branchset.insert(v);
        /* vector<int> rreachable = ordering.RReachable(v);
        for (auto w : rreachable)
        {
            // if (graph[w].position != NOT_PLACED)
            wreach_union.insert(w);
        }
        wreach_union.insert(v);*/
        wreach_union.insert(ordering.wreach[v].begin(), ordering.wreach_inv[v].end());
        if (always_take_too_full)
            wreach_union.erase(v);
    }
    unordered_set<int> rest;
    BGL_FORALL_VERTICES(v, graph, OrderedGraph)
    {
        rest.insert(v);
    }

    for (auto v : wreach_union)
    {
        rest.erase(v);
    }
    if (always_take_too_full)
    {
        for (auto v : ordering.is_too_full)
        {
            rest.erase(v);
        }
    }

    vector<int> rest_vec(rest.begin(), rest.end());
    vector<int> wreach_union_vec(wreach_union.begin(), wreach_union.end());

    for (int i = 0; i < 10; i++)
    {
        shuffle(wreach_union_vec.begin(), wreach_union_vec.end(), rng);
        unordered_set<int> branchset;
        if (always_take_too_full)
            branchset.insert(ordering.is_too_full.begin(), ordering.is_too_full.end());
        int have = branchset.size();
        for (int i = 0; i < min(c - have, (int)wreach_union_vec.size()); i++)
        {
            branchset.insert(wreach_union_vec[i]);
        }

        if (branchset.size() < c)
        {
            unordered_set<int> nums = KFromN(rest_vec.size(), min(rest_vec.size(), c - branchset.size()), rng);
            for (auto num : nums)
            {
                branchset.insert(rest_vec[num - 1]);
            }
        }

        if (Run(branchset))
        {
            return true;
        }
    }

    return false;
}