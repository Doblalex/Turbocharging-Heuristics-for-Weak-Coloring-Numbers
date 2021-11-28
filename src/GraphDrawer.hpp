#pragma once

#include "Headers.hpp"

// graph drawer singleton because py_initialize cannot be called more that twice because of numpy..
class GraphDrawer
{
private:
    GraphDrawer();
    ~GraphDrawer();
    GraphDrawer(GraphDrawer const &);    // Don't Implement
    void operator=(GraphDrawer const &); // Don't implement
public:
    static GraphDrawer &getInstance();
    void DrawGraph(const OrderedGraph &graph);
};