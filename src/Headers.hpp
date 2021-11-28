#pragma once

#include <bits/stdc++.h>
#include <boost/program_options.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/format.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <random>
#include "./dynamic-graph/dynamic_connectivity.hpp"
#define MP make_pair
#define PB push_back
#define st first
#define nd second
#define NOT_PLACED INT_MAX

using namespace std;

static std::random_device rd; // random device engine, usually based on /dev/random on UNIX-like systems
// initialize Mersennes' twister using rd to generate the seed
static std::mt19937 rng{rd()};

template <typename TH>
void _dbg(const char *sdbg, TH h) { cerr << sdbg << "=" << h << "\n"; }
template <typename TH, typename... TA>
void _dbg(const char *sdbg, TH h, TA... t)
{
  while (*sdbg != ',')
    cerr << *sdbg++;
  cerr << "=" << h << ",";
  _dbg(sdbg + 1, t...);
}
#ifndef DEBUG
#define debug(...)
#else
#ifdef LOCAL
#define debug(...) _dbg(#__VA_ARGS__, __VA_ARGS__)
#define debugv(x)           \
  {                         \
    {                       \
      cerr << #x << " = ";  \
      FORE(itt, (x))        \
      cerr << *itt << ", "; \
      cerr << "\n";         \
    }                       \
  }
#else
#define debug(...) (__VA_ARGS__)
#define debugv(x)
#define cerr \
  if (0)     \
  cout
#endif
#endif

template <class T>
ostream &operator<<(ostream &out, vector<T> vec)
{
  out << "(";
  for (auto &v : vec)
    out << v << ", ";
  return out << ")";
}
template <class T>
ostream &operator<<(ostream &out, set<T> vec)
{
  out << "(";
  for (auto &v : vec)
    out << v << ", ";
  return out << ")";
}

struct vertex_info
{
  string id;    // id from text file
  int position; // position in ordering
  int wreach_sz;
};
using EP = boost::property<boost::edge_weight_t, int>;

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, vertex_info, EP> OrderedGraph;
typedef vector<unordered_set<int>> EdgeMutableGraph;
typedef std::pair<int, int> Edge;
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, EP> SimpleGraph;