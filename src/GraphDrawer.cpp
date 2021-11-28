#include "Headers.hpp"
#include "GraphDrawer.hpp"

#define PR PyRun_SimpleString

using namespace boost::python;

GraphDrawer::GraphDrawer()
{
    Py_Initialize();
}

GraphDrawer::~GraphDrawer()
{
    Py_Finalize();
}

GraphDrawer &GraphDrawer::getInstance()
{
    static GraphDrawer instance;
    return instance;
}

void GraphDrawer::DrawGraph(const OrderedGraph &graph)
{
    // PyRun_SimpleString("print('If we are here, we did not crash')");
    static int cnt = 0;
    PR("import networkx as nx");
    PR("import numpy as np");
    PR("import matplotlib.pyplot as plt");
    PR("G=nx.Graph()");
    PR("labeldict = {}");
    for (auto u : boost::make_iterator_range(boost::vertices(graph)))
    {
        if (graph[u].position == NOT_PLACED)
        {
            PR(("G.add_node(" + to_string(u) + ")").c_str());
            PR(("labeldict[" + to_string(u) + "] = " + to_string(graph[u].wreach_sz)).c_str());
            for (auto v : boost::make_iterator_range(boost::adjacent_vertices(u, graph)))
            {
                if (graph[v].position == NOT_PLACED)
                {
                    PR(("G.add_edge(" + to_string(u) + ", " + to_string(v) + ")").c_str());
                }
            }
        }
    }
    PR("nx.draw(G, labels=labeldict, with_labels = True)");
    PR("plt.show()");
    // PR(("plt.savefig('img/foo" + to_string(cnt++) + ".png', bbox_inches='tight')").c_str());
    // PR("plt.clf()");
}