#include "OrderingRL.hpp"

OrderingRL::OrderingRL(OrderedGraph &graph, int r, int target_k, bool save_potential_sreach, bool lower_bound) : r(r), graph(graph), target_k(target_k), save_potential_sreach(save_potential_sreach), lower_bound(lower_bound)
{
    ordering = vector<int>(boost::num_vertices(graph), -1);
    this->at = ordering.size() - 1;
    this->sreach_wo_wreach = vector<unordered_set<int>>(boost::num_vertices(graph));
    this->wreach = vector<unordered_set<int>>(boost::num_vertices(graph));
    this->sreach_on_place = vector<vector<pair<int, int>>>(boost::num_vertices(graph));

    BGL_FORALL_VERTICES(v, graph, OrderedGraph)
    {
        free_v.insert(v);
        if (save_potential_sreach)
        {
            BGL_FORALL_ADJ(v, w, graph, OrderedGraph)
            {
                this->sreach_wo_wreach[v].insert(w);
            }
        }

        this->wreach[v].insert(v);
        this->graph[v].wreach_sz = 1;
        assert(graph[v].position == NOT_PLACED);
    }

    if (lower_bound)
        assert(save_potential_sreach);
}

vector<int> OrderingRL::ReachableInSuborder(const int src, const int maxdist)
{
    static vector<bool> vis(boost::num_vertices(this->graph), false);

    queue<pair<int, int>> bfsque;
    vector<int> reachable;
    bfsque.push({src, 0});
    vis[src] = true;
    while (bfsque.size() > 0)
    {
        pair<int, int> elem = bfsque.front();
        bfsque.pop();
        int u = elem.first;
        int dist = elem.second;
        reachable.push_back(u);

        if (dist >= maxdist)
            continue;

        BGL_FORALL_ADJ(u, v, graph, OrderedGraph)
        {
            if (!vis[v] && graph[v].position != NOT_PLACED)
            {
                vis[v] = true;
                bfsque.push({v, dist + 1});
            }
        }
    }
    for (auto v : reachable)
    {
        vis[v] = false;
    }

    return reachable;
}

pair<vector<vector<int>>, vector<vector<int>>> OrderingRL::LayeredReachableWSreach(const int src, const int maxdist)
{
    static vector<bool> vis(boost::num_vertices(this->graph), false);

    queue<pair<int, int>> bfsque;
    vector<int> reachable;
    pair<vector<vector<int>>, vector<vector<int>>> ans;
    ans.first = vector<vector<int>>(maxdist + 1);
    ans.second = vector<vector<int>>(maxdist + 1);
    bfsque.push({src, 0});
    vis[src] = true;
    while (bfsque.size() > 0)
    {
        pair<int, int> elem = bfsque.front();
        bfsque.pop();
        int u = elem.first;
        int dist = elem.second;
        reachable.push_back(u);

        if (graph[u].position == NOT_PLACED)
        {
            ans.first[dist].push_back(u);
        }
        else
        {
            ans.second[dist].push_back(u);
        }

        if (dist >= maxdist || graph[u].position == NOT_PLACED)
            continue;

        BGL_FORALL_ADJ(u, v, graph, OrderedGraph)
        {
            if (!vis[v])
            {
                vis[v] = true;
                bfsque.push({v, dist + 1});
            }
        }
    }
    for (auto v : reachable)
    {
        vis[v] = false;
    }

    return ans;
}

void OrderingRL::OnWSReachUpdate(const int v)
{
    this->graph[v].wreach_sz = this->wreach[v].size() + this->sreach_wo_wreach[v].size();
    this->changed.insert(v);
    if (this->graph[v].wreach_sz > target_k && this->graph[v].position != NOT_PLACED)
        too_full.insert(v);
    else
        too_full.erase(v);
}

void OrderingRL::Place(const int v)
{
    assert(graph[v].position == NOT_PLACED);
    assert(at >= 0);
    free_v.erase(v);

    changed.insert(v);
    graph[v].position = at;
    ordering[at--] = v;

    pair<vector<vector<int>>, vector<vector<int>>> ab = LayeredReachableWSreach(v, r);
    vector<vector<int>> layeredsreach = ab.first;
    vector<vector<int>> layeredinorder = ab.second;

    if (save_potential_sreach)
    {
        for (auto w : sreach_wo_wreach[v])
        {
            sreach_wo_wreach[w].erase(v);
        }
    }

    for (int r1 = 0; r1 <= r; r1++)
    {
        for (auto vinorder : layeredinorder[r1])
        {
            for (int r2 = 1; r1 + r2 <= r; r2++)
            {
                for (auto vinsreach : layeredsreach[r2])
                {
                    if (sreach_wo_wreach[vinorder].find(vinsreach) == sreach_wo_wreach[vinorder].end())
                    {
                        sreach_wo_wreach[vinorder].insert(vinsreach);
                        this->sreach_on_place[v].push_back({vinorder, vinsreach});
                    }
                }
            }
            if (graph[vinorder].position != NOT_PLACED && vinorder != v)
            {
                this->wreach[vinorder].insert(v);
                if (this->sreach_wo_wreach[vinorder].find(v) != this->sreach_wo_wreach[vinorder].end())
                    this->sreach_wo_wreach[vinorder].erase(v);
            }
            OnWSReachUpdate(vinorder);
        }

        if (save_potential_sreach)
        {
            for (auto vinsreach1 : layeredsreach[r1])
            {
                for (int r2 = 1; r1 + r2 <= r; r2++)
                {
                    for (auto vinsreach2 : layeredsreach[r2])
                    {
                        if (vinsreach1 != vinsreach2 && sreach_wo_wreach[vinsreach1].find(vinsreach2) == sreach_wo_wreach[vinsreach1].end())
                        {
                            sreach_wo_wreach[vinsreach1].insert(vinsreach2);
                            this->sreach_on_place[v].push_back({vinsreach1, vinsreach2});
                        }
                    }
                }
                OnWSReachUpdate(vinsreach1);
            }
        }
    }
}

void OrderingRL::UnPlace()
{
    assert(at < ((int)ordering.size()) - 1);
    at++;
    int v = ordering[at];
    free_v.insert(v);
    graph[v].position = NOT_PLACED;
    ordering[at] = -1;
    changed.insert(v);

    for (auto w : ReachableInSuborder(v, r))
    {
        if (v == w)
            continue;
        this->sreach_wo_wreach[w].insert(v);
        this->wreach[w].erase(v);
        OnWSReachUpdate(w);
    }

    for (auto p : sreach_on_place[v])
    {
        this->sreach_wo_wreach[p.first].erase(p.second);
        OnWSReachUpdate(p.first);
    }
    if (save_potential_sreach)
    {
        for (auto w : sreach_wo_wreach[v])
        {
            sreach_wo_wreach[w].insert(v);
            OnWSReachUpdate(w);
        }
    }

    sreach_on_place[v].clear();
}

bool OrderingRL::IsExtendable()
{
    if (lower_bound)
        return this->too_full.size() == 0 && !IsLBExceed();
    else
        return this->too_full.size() == 0;
}

bool OrderingRL::IsValidFull()
{
    return at == -1 && IsExtendable();
}

void OrderingRL::PrintOrdering()
{
    for (auto v : this->ordering)
    {
        cout << this->graph[v].id << " ";
    }
    cout << endl;
}

int OrderingRL::Wreach()
{
    int mx = 0;
    BGL_FORALL_VERTICES(v, this->graph, OrderedGraph)
    {
        mx = max(mx, graph[v].wreach_sz);
    }
    return mx;
}

bool OrderingRL::IsLBExceed()
{
    vector<pair<int, int>> removed;
    set<pair<int, int>> pq;
    for (auto v : free_v)
    {
        pq.insert({sreach_wo_wreach[v].size() + 1, v});
    }
    bool lb_exceeded = false;
    while (!pq.empty())
    {
        pair<int, int> p = *pq.begin();
        pq.erase(p);

        if (p.first > target_k)
        {
            lb_exceeded = true;
            break;
        }
        int v = p.second;
        for (auto w : this->sreach_wo_wreach[v])
        {
            assert(pq.find({(this->sreach_wo_wreach[w].size() + 1), w}) != pq.end());
            pq.erase({(this->sreach_wo_wreach[w].size() + 1), w});
            sreach_wo_wreach[w].erase(v);
            removed.push_back({w, v});
            pq.insert({(this->sreach_wo_wreach[w].size() + 1), w});
        }
    }
    for (auto p : removed)
    {
        sreach_wo_wreach[p.first].insert(p.second);
    }
    return lb_exceeded;
}