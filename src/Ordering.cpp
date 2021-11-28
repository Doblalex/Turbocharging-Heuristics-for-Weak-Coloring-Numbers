#include "Ordering.hpp"

Ordering::Ordering(OrderedGraph &graph, int r, bool track_full_neighbours, bool do_lower_bound, bool do_graph_adj_ordered, bool ignore_right, bool opt_components) : graph(graph), track_full_neighbours(track_full_neighbours), do_lower_bound(do_lower_bound), do_graph_adj_ordered(do_graph_adj_ordered), ignore_right(ignore_right), opt_components(opt_components)
{
    BGL_FORALL_VERTICES(v, graph, OrderedGraph)
    {
        assert(graph[v].position == NOT_PLACED);
    }
    this->ordering = vector<int>(boost::num_vertices(graph), -1);
    this->at = 0;
    this->r = r;
    this->wreach = vector<unordered_set<int>>(boost::num_vertices(graph), unordered_set<int>());
    this->wreach_inv = vector<unordered_set<int>>(boost::num_vertices(graph), unordered_set<int>());
    BGL_FORALL_VERTICES(v, graph, OrderedGraph)
    {
        vertices_active.insert(v);
        this->wreach[v].insert(v);
        this->wreach_inv[v].insert(v);
    }
    if (track_full_neighbours)
    {
        this->too_full_neighbours = vector<unordered_set<int>>(boost::num_vertices(graph), unordered_set<int>());
    }
    graph_notplaced = vector<unordered_set<int>>(boost::num_vertices(graph));
    BGL_FORALL_EDGES(e, graph, OrderedGraph)
    {
        int u = boost::source(e, graph);
        int v = boost::target(e, graph);
        graph_notplaced[u].insert(v);
        graph_notplaced[v].insert(u);
    }

    if (do_graph_adj_ordered)
    {
        for (int u = 0; u < boost::num_vertices(graph); u++)
        {
            set<int, std::function<bool(const int a, const int b)>> s(CompByOrder(u, graph));
            graph_adj_ordered.push_back(s);
            BGL_FORALL_ADJ(u, v, graph, OrderedGraph)
            {
                graph_adj_ordered[u].insert(v);
            }
        }
    }
}

vector<int> Ordering::RReachableRight(int src)
{
    if (do_graph_adj_ordered)
    {
        return RReachableRightOptimized(src);
    }
    static vector<bool> vis(boost::num_vertices(this->graph), false);
    int position_src = graph[src].position;

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

        if (dist >= this->r)
            continue;

        BGL_FORALL_ADJ(u, v, graph, OrderedGraph)
        {
            if (!vis[v] && graph[v].position >= position_src)
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

vector<int> Ordering::RReachableRightOptimized(const int src)
{
    static vector<bool> vis(boost::num_vertices(this->graph), false);
    int position_src = graph[src].position;

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

        if (dist >= this->r)
            continue;

        auto it = graph_adj_ordered[u].begin();
        for (; it != graph_adj_ordered[u].end(); it++)
        {
            int v = *it;
            if (graph[v].position < graph[src].position)
                break;
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

    return reachable;
}

vector<int> Ordering::RReachable(int src)
{
    static vector<bool> vis(boost::num_vertices(this->graph), false);
    int position_src = graph[src].position;

    assert(vis.size() == boost::num_vertices(graph));

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

        if (dist >= this->r)
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

    return reachable;
}

void Ordering::WreachAdd(int vertex, int u)
{
    assert(vertex != u);
    this->wreach[vertex].insert(u);
    this->wreach_inv[u].insert(vertex);
    this->graph[vertex].wreach_sz++;
    this->changed.insert(vertex);

    if (this->graph[vertex].wreach_sz > target_k)
    {
        if (!this->ignore_right || this->graph[vertex].position != NOT_PLACED)
            this->is_too_full.insert(vertex);
    }
    else if (this->track_full_neighbours &&
             this->graph[vertex].wreach_sz == target_k &&
             this->graph[vertex].position == NOT_PLACED)
    {
        for (auto v : RReachableRight(vertex))
        {
            if (v != vertex && graph[v].wreach_sz >= target_k)
            {
                this->too_full_neighbours[vertex].insert(v);
                this->too_full_neighbours[v].insert(vertex);
                this->is_full_neighbour.insert(v);
                this->is_full_neighbour.insert(vertex);
            }
        }
    }
}

void Ordering::WreachRem(int vertex, int u)
{
    assert(vertex != u);
    this->wreach[vertex].erase(u);
    this->wreach_inv[u].erase(vertex);
    this->graph[vertex].wreach_sz--;
    this->changed.insert(vertex);

    if (this->graph[vertex].wreach_sz == target_k)
    {
        this->is_too_full.erase(vertex);
    }
    else if (this->track_full_neighbours && this->graph[vertex].wreach_sz == target_k - 1 && this->graph[vertex].position == NOT_PLACED)
    {
        if (this->too_full_neighbours[vertex].size() != 0)
        {
            for (auto v : this->too_full_neighbours[vertex])
            {
                this->too_full_neighbours[v].erase(vertex);
                if (this->too_full_neighbours[v].size() == 0)
                {
                    this->is_full_neighbour.erase(v);
                }
            }
            this->too_full_neighbours[vertex].clear();
            this->is_full_neighbour.erase(vertex);
        }
    }
}

void Ordering::Place(int vertex)
{
    assert(this->at < ordering.size());
    // uncommented this because new rneighbourhood branching does not care
    // assert(this->is_full_neighbour.size() == 0 && this->is_too_full.size() == 0);
    this->changed.insert(vertex);
    vector<int> reachable = this->RReachableNotPlaced(vertex);
    this->ordering[at] = vertex;
    ChangePosition(vertex, at);

    for (auto v : reachable)
    {
        if (v == vertex)
            continue;
        this->WreachAdd(v, vertex);
    }
    this->at++;
    this->RemoveVertex(vertex);
}

void Ordering::UnPlace()
{
    assert(at > 0);
    int vertex = this->ordering[at - 1];
    this->changed.insert(vertex);
    vector<int> reachable(this->wreach_inv[vertex].begin(), this->wreach_inv[vertex].end());
    for (auto v : reachable)
    {
        if (v == vertex)
            continue;
        this->WreachRem(v, vertex);
    }
    this->at--;
    this->ordering[at] = -1;
    ChangePosition(vertex, NOT_PLACED);
    this->AddVertex(vertex);
}

bool Ordering::IsExtendable()
{
    if (do_lower_bound)
        return this->is_too_full.size() == 0 && this->is_full_neighbour.size() == 0 && !this->IsLBDegeneracyContractExceed();
    else
        return this->is_too_full.size() == 0 && this->is_full_neighbour.size() == 0;
}

bool Ordering::IsExtendableNoLB()
{
    return this->is_too_full.size() == 0 && this->is_full_neighbour.size() == 0;
}

int Ordering::Wreach()
{
    int mx = 0;
    BGL_FORALL_VERTICES(v, this->graph, OrderedGraph)
    {
        mx = max(mx, graph[v].wreach_sz);
    }
    return mx;
}

void Ordering::PrintOrdering()
{
    for (auto v : this->ordering)
    {
        cout << this->graph[v].id << " ";
    }
    cout << endl;
}

bool Ordering::IsValidFull()
{
    return at == ordering.size() && is_too_full.size() == 0;
}

void Ordering::SwapInOut(int v_in, int v_out)
{
    this->changed.insert(v_in);
    this->changed.insert(v_out);
    unordered_set<int> vertices_between_changable;

    // just for runtime optimization
    if (this->at - this->graph[v_in].position - 1 <= this->wreach_inv[v_in].size() + this->wreach[v_out].size())
    {
        for (int pos = this->graph[v_in].position + 1; pos < at; pos++)
        {
            int v = this->ordering[pos];
            bool is_in_v_in = this->wreach_inv[v_in].find(v) != this->wreach_inv[v_in].end();
            bool is_in_v_out = this->wreach[v_out].find(v) != this->wreach[v_out].end();
            if (is_in_v_in || is_in_v_out)
            {
                vertices_between_changable.insert(v);
            }
        }
    }
    else
    {
        for (auto v : wreach[v_out])
        {
            if (graph[v].position < at && graph[v].position > graph[v_in].position)
            {
                vertices_between_changable.insert(v);
            }
        }
        for (auto v : wreach_inv[v_in])
        {
            if (graph[v].position < at && graph[v].position > graph[v_in].position)
            {
                vertices_between_changable.insert(v);
            }
        }
    }
    // cout << "Optimized: " << vertices_between_changable.size() << " " << at - this->graph[v_in].position + 1 << endl;

    // swap positions of the two vertices
    ChangePosition(v_out, this->graph[v_in].position);
    this->ordering[this->graph[v_out].position] = v_out;
    ChangePosition(v_in, NOT_PLACED);

    // v_in is now not placed, remove him from wreach
    for (auto v : vector<int>(this->wreach_inv[v_in].begin(), this->wreach_inv[v_in].end()))
    {
        if (v != v_in)
            this->WreachRem(v, v_in);
    }

    // v_out is now in weakly reachable sets
    for (auto v : RReachableRight(v_out))
    {
        if (v != v_out)
            this->WreachAdd(v, v_out);
    }

    for (auto u : vertices_between_changable)
    {
        // weakly reachable sets w.r.t. u could change
        UpdateWreach(u);
    }
    this->AddVertex(v_in);
    this->RemoveVertex(v_out);
}

void Ordering::SwapInIn(int vleft, int vright)
{
    this->changed.insert(vleft);
    this->changed.insert(vright);

    unordered_set<int> vertices_changable;
    bool do_recomputeleft = false;
    bool do_recomputeright = false;

    // just for runtime optimization
    if (this->graph[vright].position - this->graph[vleft].position - 1 <= this->wreach_inv[vleft].size() + this->wreach[vright].size())
    {
        for (int pos = this->graph[vleft].position + 1; pos < this->graph[vright].position; pos++)
        {
            int v = this->ordering[pos];
            bool is_in_vleft = this->wreach_inv[vleft].find(v) != this->wreach_inv[vleft].end();
            bool is_in_vright = this->wreach[vright].find(v) != this->wreach[vright].end();
            do_recomputeleft = do_recomputeleft || is_in_vleft;
            do_recomputeright = do_recomputeright || is_in_vright;

            if (is_in_vleft || is_in_vright)
            {
                vertices_changable.insert(v);
            }
        }
    }
    else
    {
        for (auto v : wreach[vright])
        {
            if (graph[v].position < this->graph[vright].position && graph[v].position > this->graph[vleft].position)
            {
                do_recomputeright = true;
                vertices_changable.insert(v);
            }
        }
        for (auto v : wreach_inv[vleft])
        {
            if (graph[v].position < this->graph[vright].position && graph[v].position > this->graph[vleft].position)
            {
                do_recomputeleft = true;
                vertices_changable.insert(v);
            }
        }
    }
    if (this->wreach[vright].find(vleft) != this->wreach[vright].end())
    {
        do_recomputeright = true;
        do_recomputeleft = true;
    }
    if (do_recomputeleft)
    {
        vertices_changable.insert(vleft);
    }
    if (do_recomputeright)
    {
        vertices_changable.insert(vright);
    }

    // swap positions of the two vertices
    swap(this->ordering[this->graph[vleft].position], this->ordering[this->graph[vright].position]);
    int temp = this->graph[vleft].position;
    ChangePosition(vleft, this->graph[vright].position);
    ChangePosition(vright, temp);

    for (auto u : vertices_changable)
    {
        // weakly reachable sets w.r.t. u could change
        UpdateWreach(u);
    }
}

/**
 * Assumes position of v1 is smaller tan the one of v1
 */
void Ordering::Swap(int v1, int v2)
{
    assert(!this->track_full_neighbours);
    if (this->graph[v1].position != NOT_PLACED && this->graph[v2].position == NOT_PLACED)
    {
        SwapInOut(v1, v2);
    }
    else if (this->graph[v1].position != NOT_PLACED && this->graph[v2].position != NOT_PLACED)
    {
        assert(this->graph[v1].position < this->graph[v2].position);
        SwapInIn(v1, v2);
    }
    else
    {
        assert(false);
    }
}

/**
 * This function assumes that the "Place" function is correctly implemented.
 **/
void Ordering::VerifyImpl()
{
    int n = boost::num_vertices(graph);
    OrderedGraph graph_ver(boost::num_vertices(graph));
    BGL_FORALL_VERTICES(v, graph_ver, OrderedGraph)
    {
        graph_ver[v].position = NOT_PLACED;
        graph_ver[v].wreach_sz = 1;
    }
    BGL_FORALL_EDGES(e, graph, OrderedGraph)
    {
        int u = boost::source(e, graph);
        int v = boost::target(e, graph);
        boost::add_edge(u, v, 1, graph_ver);
    }
    Ordering ordering_ver(graph_ver, r, this->track_full_neighbours, do_lower_bound, do_graph_adj_ordered, ignore_right, opt_components);
    ordering_ver.target_k = this->target_k;
    for (auto v : this->ordering)
    {
        if (v != -1)
        {
            assert(graph_ver[v].position == NOT_PLACED);
            ordering_ver.Place(v);
        }
    }

    if (ordering_ver.is_too_full != this->is_too_full)
    {
        assert(false);
    }
    assert(ordering_ver.is_too_full == this->is_too_full);
    BGL_FORALL_VERTICES(v, graph_ver, OrderedGraph)
    {
        if (graph_ver[v].wreach_sz > graph[v].wreach_sz)
        {
            assert(false);
        }
        if (graph_ver[v].wreach_sz < graph[v].wreach_sz)
        {
            assert(false);
        }
        assert(graph_ver[v].wreach_sz == graph[v].wreach_sz);
        assert(ordering_ver.wreach[v] == this->wreach[v]);
        assert(ordering_ver.wreach_inv[v] == this->wreach_inv[v]);
    }
}

void Ordering::VerifyImplMerge(vector<int> &hackordering, vector<int> ignored)
{
    int n = boost::num_vertices(graph);
    OrderedGraph graph_ver(boost::num_vertices(graph));
    BGL_FORALL_VERTICES(v, graph_ver, OrderedGraph)
    {
        graph_ver[v].position = NOT_PLACED;
        graph_ver[v].wreach_sz = 1;
    }
    BGL_FORALL_EDGES(e, graph, OrderedGraph)
    {
        int u = boost::source(e, graph);
        int v = boost::target(e, graph);
        boost::add_edge(u, v, 1, graph_ver);
    }
    Ordering ordering_ver(graph_ver, r, this->track_full_neighbours, do_lower_bound, do_graph_adj_ordered, ignore_right, opt_components);
    for (auto v : ignored)
    {
        graph_ver[v].position = -1;
    }
    ordering_ver.target_k = this->target_k;
    for (auto v : hackordering)
    {
        if (v != -1)
        {
            assert(graph_ver[v].position == NOT_PLACED);
            ordering_ver.Place(v);
        }
    }

    assert(ordering_ver.is_too_full == this->is_too_full);
    BGL_FORALL_VERTICES(v, graph_ver, OrderedGraph)
    {
        assert(graph_ver[v].wreach_sz == graph[v].wreach_sz);
        assert(ordering_ver.wreach[v] == this->wreach[v]);
        assert(ordering_ver.wreach_inv[v] == this->wreach_inv[v]);
    }
}

void Ordering::Place(int vertex, int position)
{
    assert(!this->track_full_neighbours);
    assert(graph[vertex].position == NOT_PLACED);
    assert(ordering[position] == -1);
    assert(position < at);

    this->changed.insert(vertex);
    vector<int> vs_recompute;

    // just for runtime optimization
    if (at - position <= this->wreach[vertex].size())
    {
        for (int pos = position + 1; pos < this->at; pos++)
        {
            int v = this->ordering[pos];
            if (v != -1 && this->wreach[vertex].find(v) != this->wreach[vertex].end())
            {
                vs_recompute.push_back(v);
            }
        }
    }
    else
    {
        for (auto v : this->wreach[vertex])
        {
            if (graph[v].position > position)
            {
                vs_recompute.push_back(v);
            }
        }
    }
    this->ordering[position] = vertex;
    ChangePosition(vertex, position);

    for (auto v : this->RReachableRight(vertex))
    {
        if (v != vertex)
        {
            this->WreachAdd(v, vertex);
        }
    }

    for (auto u : vs_recompute)
    {
        UpdateWreach(u);
    }
    this->RemoveVertex(vertex);
}

void Ordering::Unplace(int position)
{
    assert(!this->track_full_neighbours);
    assert(this->ordering[position] != -1);
    int vertex = this->ordering[position];
    vector<int> vs_recompute;
    this->changed.insert(vertex);

    // just for runtime optimization
    if (at - position <= this->wreach_inv[vertex].size())
    {
        for (int pos = position + 1; pos < this->at; pos++)
        {
            int v = this->ordering[pos];
            if (v != -1 && this->wreach_inv[vertex].find(v) != this->wreach_inv[vertex].end())
            {
                vs_recompute.push_back(v);
            }
        }
    }
    else
    {
        for (auto v : this->wreach_inv[vertex])
        {
            if (graph[v].position > position && graph[v].position != NOT_PLACED)
            {
                vs_recompute.push_back(v);
            }
        }
    }
    this->ordering[position] = -1;
    ChangePosition(vertex, NOT_PLACED);
    for (auto v : vector<int>(this->wreach_inv[vertex].begin(), this->wreach_inv[vertex].end()))
    {
        if (v != vertex)
            this->WreachRem(v, vertex);
    }

    for (auto u : vs_recompute)
    {
        UpdateWreach(u);
    }
    this->AddVertex(vertex);
}

void Ordering::UpdateWreach(int u)
{
    this->changed.insert(u);
    static vector<bool> rreachable_lookup(boost::num_vertices(graph), false);
    assert(this->graph[u].position != NOT_PLACED);
    // weakly reachable sets w.r.t. u could change
    vector<int> rreachable = RReachableRight(u);
    vector<int> to_add;
    vector<int> to_remove;

    for (auto v : rreachable)
    {
        rreachable_lookup[v] = true;
        if (this->wreach_inv[u].find(v) == this->wreach_inv[u].end())
        {
            to_add.push_back(v);
        }
    }
    for (auto v : this->wreach_inv[u])
    {
        if (!rreachable_lookup[v])
        {
            to_remove.push_back(v);
        }
    }
    for (auto v : to_remove)
    {
        this->WreachRem(v, u);
    }
    for (auto v : to_add)
    {
        this->WreachAdd(v, u);
    }
    for (auto v : rreachable)
    {
        rreachable_lookup[v] = false;
    }
}

void Ordering::UpdateWreachHelper(int u, unordered_set<pair<int, int>, boost::hash<std::pair<int, int>>> &alladded)
{
    assert(this->graph[u].position != NOT_PLACED);
    // weakly reachable sets w.r.t. u could change
    this->changed.insert(u);
    vector<int> rreachable = RReachableRight(u);
    unordered_set<int> srreachable(rreachable.begin(), rreachable.end());
    vector<int> to_add;
    vector<int> to_remove;

    for (auto v : rreachable)
    {
        if (this->wreach_inv[u].find(v) == this->wreach_inv[u].end())
        {
            to_add.push_back(v);
        }
    }
    for (auto v : this->wreach_inv[u])
    {
        if (srreachable.find(v) == srreachable.end())
        {
            to_remove.push_back(v);
        }
    }
    for (auto v : to_remove)
    {
        if (alladded.find({v, u}) != alladded.end())
        {
            alladded.erase({v, u});
        }
        this->WreachRem(v, u);
    }
    for (auto v : to_add)
    {
        alladded.insert({v, u});
        this->WreachAdd(v, u);
    }
}

void Ordering::ResetTo(vector<int> &oldordering)
{
    BGL_FORALL_VERTICES(v, graph, OrderedGraph)
    {
        graph[v].wreach_sz = 1;
        ChangePosition(v, NOT_PLACED);
    }
    this->is_full_neighbour.clear();
    this->is_too_full.clear();
    this->ordering = vector<int>(boost::num_vertices(graph), -1);
    this->at = 0;
    this->wreach = vector<unordered_set<int>>(boost::num_vertices(graph), unordered_set<int>());
    this->wreach_inv = vector<unordered_set<int>>(boost::num_vertices(graph), unordered_set<int>());
    BGL_FORALL_VERTICES(v, graph, OrderedGraph)
    {
        this->wreach[v].insert(v);
        this->wreach_inv[v].insert(v);
        if (this->vertices_active.find(v) == this->vertices_active.end())
        {
            this->AddVertex(v);
        }
    }
    if (track_full_neighbours)
    {
        this->too_full_neighbours = vector<unordered_set<int>>(boost::num_vertices(graph), unordered_set<int>());
    }

    for (auto v : oldordering)
    {
        if (v != -1)
            Place(v);
    }
}

void Ordering::AddVertex(int v)
{
    for (auto w : boost::make_iterator_range(boost::adjacent_vertices(v, graph)))
    {
        if (vertices_active.find(w) != vertices_active.end())
        {
            graph_notplaced[v].insert(w);
            graph_notplaced[w].insert(v);
        }
    }
    vertices_active.insert(v);
}

void Ordering::RemoveVertex(int v)
{
    for (auto w : vector<int>(graph_notplaced[v].begin(), graph_notplaced[v].end()))
    {
        graph_notplaced[v].erase(w);
        graph_notplaced[w].erase(v);
    }
    vertices_active.erase(v);
}

vector<int> Ordering::RReachableNotPlaced(int src)
{
    static vector<bool> vis(boost::num_vertices(this->graph), false);
    if (graph[src].position != NOT_PLACED)
    {
        cout << src << endl;
        cout << graph[src].position << endl;
        cout << graph[src].position << " " << ordering[graph[src].position] << endl;
    }
    assert(graph[src].position == NOT_PLACED);

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

        if (dist >= this->r)
            continue;

        for (auto v : graph_notplaced[u])
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

    return reachable;
}

vector<int> Ordering::ComponentNotPlaced(int src)
{
    static vector<bool> vis(boost::num_vertices(this->graph), false);

    assert(graph[src].position == NOT_PLACED);

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

        for (auto v : graph_notplaced[u])
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

    return reachable;
}

bool Ordering::IsLBDegeneracyExceed()
{
    set<pair<int, int>> pq;
    vector<pair<int, int>> edges_removed;
    for (auto v : vertices_active)
    {
        pq.insert({(this->graph_notplaced[v].size() + this->graph[v].wreach_sz), v});
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
        for (auto w : this->graph_notplaced[v])
        {
            pq.erase({(this->graph_notplaced[w].size() + this->graph[w].wreach_sz), w});
            graph_notplaced[w].erase(v);
            edges_removed.push_back({w, v});
            pq.insert({(this->graph_notplaced[w].size() + this->graph[w].wreach_sz), w});
        }
    }
    for (auto p : edges_removed)
    {
        graph_notplaced[p.first].insert(p.second);
    }
    return lb_exceeded;
}

int Ordering::LBDegeneracy()
{
    set<pair<int, int>> pq;
    vector<pair<int, int>> edges_removed;
    for (auto v : vertices_active)
    {
        pq.insert({(this->graph_notplaced[v].size() + this->graph[v].wreach_sz), v});
    }

    int mx = 0;
    while (!pq.empty())
    {
        pair<int, int> p = *pq.begin();
        pq.erase(p);

        mx = max(p.first, mx);

        int v = p.second;
        for (auto w : this->graph_notplaced[v])
        {
            pq.erase({(this->graph_notplaced[w].size() + this->graph[w].wreach_sz), w});
            graph_notplaced[w].erase(v);
            edges_removed.push_back({w, v});
            pq.insert({(this->graph_notplaced[w].size() + this->graph[w].wreach_sz), w});
        }
    }
    for (auto p : edges_removed)
    {
        graph_notplaced[p.first].insert(p.second);
    }
    return mx;
}

bool Ordering::IsLBDegeneracyContractExceed()
{
    set<pair<int, int>> pq;
    static vector<int> contract_cnt(boost::num_vertices(graph));
    static vector<int> cur_fvalue(boost::num_vertices(graph));
    static vector<unordered_set<int>> graph_copy(boost::num_vertices(graph));
    for (auto v : vertices_active)
    {
        graph_copy[v].clear();
        contract_cnt[v] = 1;
        cur_fvalue[v] = this->graph[v].wreach_sz;
        pq.insert({(this->graph_notplaced[v].size() + cur_fvalue[v]), v});
    }
    for (auto v : vertices_active)
    {
        for (auto w : graph_notplaced[v])
        {
            graph_copy[v].insert(w);
        }
    }

    bool lb_exceeded = false;
    while (!pq.empty())
    {
        pair<int, int> p = *pq.begin();
        pq.erase(p);

        if (p.first > this->target_k)
        {
            lb_exceeded = true;
            break;
        }
        int v = p.second;
        int vcontract = -1;
        int mdeg = INT_MAX;
        for (auto w : graph_copy[v])
        {
            if (contract_cnt[v] + contract_cnt[w] - 1 <= (r - 1) / 2)
            {
                // we can still contract this edge
                if (graph_copy[w].size() + cur_fvalue[w] < mdeg)
                {
                    mdeg = graph_copy[w].size() + cur_fvalue[w];
                    vcontract = w;
                }
            }
        }

        if (vcontract != -1)
        {
            // contract v and vcontract
            pq.erase({graph_copy[vcontract].size() + cur_fvalue[vcontract], vcontract});
            for (auto w : graph_copy[vcontract])
            {
                if (graph_copy[v].find(w) == graph_copy[v].end() && v != w)
                {
                    pq.erase({(graph_copy[w].size() + cur_fvalue[w]), w});
                    graph_copy[v].insert(w);
                    graph_copy[w].insert(v);
                    pq.insert({(graph_copy[w].size() + cur_fvalue[w]), w});
                }
                if (v != w)
                {
                    pq.erase({(graph_copy[w].size() + cur_fvalue[w]), w});
                    graph_copy[w].erase(vcontract);
                    pq.insert({(graph_copy[w].size() + cur_fvalue[w]), w});
                }
            }

            graph_copy[v].erase(vcontract);
            cur_fvalue[v] = max(cur_fvalue[v], cur_fvalue[vcontract]);
            contract_cnt[v] = contract_cnt[v] + contract_cnt[vcontract];
            pq.insert({graph_copy[v].size() + cur_fvalue[v], v});
        }
        else
        {
            // delete v
            for (auto w : graph_copy[v])
            {
                pq.erase({(graph_copy[w].size() + cur_fvalue[w]), w});
                graph_copy[w].erase(v);
                pq.insert({(graph_copy[w].size() + cur_fvalue[w]), w});
            }
        }
    }

    return lb_exceeded;
}

int Ordering::LBDegeneracyContract()
{
    set<pair<int, int>> pq;
    static vector<int> contract_cnt(boost::num_vertices(graph));
    static vector<int> cur_fvalue(boost::num_vertices(graph));
    static vector<unordered_set<int>> graph_copy(boost::num_vertices(graph));
    for (auto v : vertices_active)
    {
        graph_copy[v].clear();
        contract_cnt[v] = 1;
        cur_fvalue[v] = this->graph[v].wreach_sz;
        pq.insert({(this->graph_notplaced[v].size() + cur_fvalue[v]), v});
    }
    for (auto v : vertices_active)
    {
        for (auto w : graph_notplaced[v])
        {
            graph_copy[v].insert(w);
        }
    }

    int LB = 0;
    while (!pq.empty())
    {
        pair<int, int> p = *pq.begin();
        pq.erase(p);

        LB = max(LB, p.first);
        int v = p.second;
        int vcontract = -1;
        int mdeg = INT_MAX;
        for (auto w : graph_copy[v])
        {
            if (contract_cnt[v] + contract_cnt[w] - 1 <= (this->r - 1) / 2)
            {
                // we can still contract this edge
                if (graph_copy[w].size() + cur_fvalue[w] < mdeg)
                {
                    mdeg = graph_copy[w].size() + cur_fvalue[w];
                    vcontract = w;
                }
            }
        }

        if (vcontract != -1)
        {
            // contract v and vcontract
            pq.erase({graph_copy[vcontract].size() + cur_fvalue[vcontract], vcontract});
            for (auto w : graph_copy[vcontract])
            {
                if (graph_copy[v].find(w) == graph_copy[v].end() && v != w)
                {
                    pq.erase({(graph_copy[w].size() + cur_fvalue[w]), w});
                    graph_copy[v].insert(w);
                    graph_copy[w].insert(v);
                    pq.insert({(graph_copy[w].size() + cur_fvalue[w]), w});
                }
                if (v != w)
                {
                    pq.erase({(graph_copy[w].size() + cur_fvalue[w]), w});
                    graph_copy[w].erase(vcontract);
                    pq.insert({(graph_copy[w].size() + cur_fvalue[w]), w});
                }
            }

            graph_copy[v].erase(vcontract);
            cur_fvalue[v] = max(cur_fvalue[v], cur_fvalue[vcontract]);
            contract_cnt[v] = contract_cnt[v] + contract_cnt[vcontract];
            pq.insert({graph_copy[v].size() + cur_fvalue[v], v});
        }
        else
        {
            // delete v
            for (auto w : graph_copy[v])
            {
                pq.erase({(graph_copy[w].size() + cur_fvalue[w]), w});
                graph_copy[w].erase(v);
                pq.insert({(graph_copy[w].size() + cur_fvalue[w]), w});
            }
        }
    }

    return LB;
}

int Ordering::LBDegeneracyContractSubGraph()
{
    set<pair<int, int>> pq;
    static vector<int> cur_fvalue(boost::num_vertices(graph));
    static vector<unordered_set<int>> graph_copy(boost::num_vertices(graph));
    int ind = 0;
    static vector<int> backref(boost::num_vertices(graph));
    static vector<int> frontref(boost::num_vertices(graph));
    static vector<int> backref_sub(boost::num_vertices(graph));
    static vector<vector<int>> subgraph_vertices(boost::num_vertices(graph));
    for (auto v : vertices_active)
    {
        backref[v] = ind;
        frontref[ind] = v;
        graph_copy[ind].clear();
        cur_fvalue[ind] = this->graph[v].wreach_sz;
        subgraph_vertices[ind].clear();
        subgraph_vertices[ind].push_back(ind);
        pq.insert({(this->graph_notplaced[v].size() + cur_fvalue[ind]), ind});
        ind++;
    }
    for (auto v : vertices_active)
    {
        for (auto w : graph_notplaced[v])
        {
            graph_copy[backref[v]].insert(backref[w]);
        }
    }

    UF dsu(vertices_active.size());
    int LB = 0;
    while (!pq.empty())
    {
        pair<int, int> p = *pq.begin();
        pq.erase(p);

        LB = max(LB, p.first);
        int v = p.second;
        int vcontract = -1;
        int mdeg = INT_MAX;

        for (auto w : graph_copy[v])
        {
            int vroot = dsu.find(v);
            int wroot = dsu.find(w);
            // check diameter if we merge vroot and wroot
            vector<int> vertices_joined;
            vertices_joined.insert(vertices_joined.end(), subgraph_vertices[vroot].begin(), subgraph_vertices[vroot].end());
            vertices_joined.insert(vertices_joined.end(), subgraph_vertices[wroot].begin(), subgraph_vertices[wroot].end());

            int ind_sub = 0;
            for (auto x : vertices_joined)
            {
                backref_sub[x] = ind_sub++;
            }

            SimpleGraph subgraph(vertices_joined.size());
            for (int i = 0; i < vertices_joined.size(); i++)
            {
                for (int j = i + 1; j < vertices_joined.size(); j++)
                {
                    if (graph_notplaced[frontref[vertices_joined[i]]].find(frontref[vertices_joined[j]]) != graph_notplaced[frontref[vertices_joined[i]]].end())
                    {
                        boost::add_edge(backref_sub[vertices_joined[i]], backref_sub[vertices_joined[j]], 1, subgraph);
                    }
                }
            }
            vector<vector<int>> distance_matrix = vector<vector<int>>(vertices_joined.size(), vector<int>(vertices_joined.size(), 0));
            boost::johnson_all_pairs_shortest_paths(subgraph, distance_matrix);

            int mxdist = 0;
            for (auto row : distance_matrix)
            {
                for (auto x : row)
                {
                    mxdist = max(mxdist, x);
                }
            }

            if (mxdist <= (this->r - 1) / 2)
            {
                // we can still contract this edge
                if (graph_copy[w].size() + cur_fvalue[w] < mdeg)
                {
                    mdeg = graph_copy[w].size() + cur_fvalue[w];
                    vcontract = w;
                }
            }
        }

        if (vcontract != -1)
        {
            int vroot = dsu.find(v);
            int wroot = dsu.find(vcontract);
            assert(vroot != wroot);
            dsu.merge(v, vcontract);
            int newroot = dsu.find(v);
            subgraph_vertices[vroot].insert(subgraph_vertices[vroot].end(), subgraph_vertices[wroot].begin(), subgraph_vertices[wroot].end());
            subgraph_vertices[newroot] = subgraph_vertices[vroot];

            // contract v and vcontract
            pq.erase({graph_copy[vcontract].size() + cur_fvalue[vcontract], vcontract});
            for (auto w : graph_copy[vcontract])
            {
                if (graph_copy[v].find(w) == graph_copy[v].end() && v != w)
                {
                    pq.erase({(graph_copy[w].size() + cur_fvalue[w]), w});
                    graph_copy[v].insert(w);
                    graph_copy[w].insert(v);
                    pq.insert({(graph_copy[w].size() + cur_fvalue[w]), w});
                }
                if (v != w)
                {
                    pq.erase({(graph_copy[w].size() + cur_fvalue[w]), w});
                    graph_copy[w].erase(vcontract);
                    pq.insert({(graph_copy[w].size() + cur_fvalue[w]), w});
                }
            }

            graph_copy[v].erase(vcontract);
            cur_fvalue[v] = max(cur_fvalue[v], cur_fvalue[vcontract]);
            pq.insert({graph_copy[v].size() + cur_fvalue[v], v});
        }
        else
        {
            // delete v
            for (auto w : graph_copy[v])
            {
                pq.erase({(graph_copy[w].size() + cur_fvalue[w]), w});
                graph_copy[w].erase(v);
                pq.insert({(graph_copy[w].size() + cur_fvalue[w]), w});
            }
        }
    }

    return LB;
}

Ordering::CompByOrder::CompByOrder(const int v, const OrderedGraph &graph) : v(v), graph(graph)
{
}

bool Ordering::CompByOrder::operator()(const int a, const int b) const
{
    if (this->graph[a].position == this->graph[b].position)
    {
        return a < b;
    }
    return this->graph[a].position > this->graph[b].position;
}

void Ordering::ChangePosition(const int u, const int pos)
{
    if (do_graph_adj_ordered)
    {
        BGL_FORALL_ADJ(u, v, graph, OrderedGraph)
        {
            graph_adj_ordered[v].erase(u);
        }
    }

    graph[u].position = pos;
    if (ignore_right)
    {
        if (pos == NOT_PLACED)
        {
            this->is_too_full.erase(u);
        }
        else if (graph[u].wreach_sz > target_k)
        {
            this->is_too_full.insert(u);
        }
    }

    if (do_graph_adj_ordered)
    {
        BGL_FORALL_ADJ(u, v, graph, OrderedGraph)
        {
            graph_adj_ordered[v].insert(u);
        }
    }
}