#include "ReadTxt.hpp"
#include "FilesOps.hpp"

OrderedGraph GraphReader::ReadGraph(string filename)
{
  ifstream in;
  InitIfstream(in, filename);
  string line;
  vector<pair<string, string>> edges;
  while (getline(in, line))
  {
    if (line[0] == '#')
    {
      continue;
    }
    stringstream stream(line);
    string a, b;
    stream >> a >> b;
    if (a == b)
      continue;
    shrink_indices[a] = shrink_indices[b] = 1;
    edges.push_back({a, b});
  }
  in.close();
  int n = 0;
  for (auto &p : shrink_indices)
  {
    p.nd = n;
    inv_shrinking[p.nd] = p.st;
    n++;
  }
  OrderedGraph graph(n);
  for (int i = 0; i < n; i++)
  {
    graph[i].id = inv_shrinking[i];
    graph[i].position = NOT_PLACED;
    graph[i].wreach_sz = 1;
  }

  vector<Edge> edges_shrunk;
  for (auto e : edges)
  {
    if (e.nd == e.st)
      continue;
    boost::add_edge(shrink_indices[e.nd], shrink_indices[e.st], 1, graph);
  }
  return graph;
}

string GraphReader::GetOriginalFromMapped(int ind)
{
  if (inv_shrinking.count(ind) == 0)
  {
    assert(false);
  }
  return inv_shrinking[ind];
}

int GraphReader::GetMappedFromOriginal(string ind)
{
  if (shrink_indices.count(ind) == 0)
  {
    debug(ind);
    assert(false);
  }
  return shrink_indices[ind];
}

pair<vector<int>, vector<int>> GetOrderAndWhInOrder(string filename, GraphReader &reader)
{
  int n = reader.shrink_indices.size();
  vector<int> order;
  ifstream oin;
  InitIfstream(oin, filename);
  vector<int> where_in_order(n + 1);
  string v;
  int i = 0;
  while (oin >> v)
  {
    int mapped = reader.GetMappedFromOriginal(v);
    assert(mapped != -1 && where_in_order[mapped] == 0);
    order.PB(mapped);
    where_in_order[mapped] = i;
    i++;
  }
  oin.close();
  return {order, where_in_order};
}