#!/usr/bin/env python3.7
import argparse
from platform import java_ver
import networkx as nx
import gurobipy as gp
from gurobipy import GRB as GRBpy, max_
import itertools as it
from collections import defaultdict, deque
import subprocess
import os
import time

def parse_args():
    parser = argparse.ArgumentParser(
        "Computing ordering of smallest weak r-coloring number for a graph with ilp")
    parser.add_argument("-g", "--graphpath", required=True, type=str)
    parser.add_argument("-t", "--timeout", default=None, type=int)
    parser.add_argument("-r", "--radius", type=int, required=True)
    args = parser.parse_args()
    return args

def verify(graphpath, ordering, rad, colnumber):
    graph = nx.read_adjlist(graphpath, create_using=nx.Graph)

    ok = True
    ok = ok and (len(set(ordering)) == len(ordering))
    ok = ok and (set(graph.nodes) == set(ordering))

    wreach_sz = defaultdict(int)
    for v in ordering:
        reachable_edges = list(nx.bfs_edges(graph, v, depth_limit=rad))
        reachable = set(
            list(node for edge in reachable_edges for node in edge))
        reachable.add(v)
        for u in reachable:
            wreach_sz[u] += 1
        graph.remove_node(v)
    ok = ok and (colnumber == max(wreach_sz.values()))
    return ok

def get_initial_solution(args):
    try:
        ord = subprocess.check_output(["./src/SreachHeuristic", "--in", args.graphpath, "--rad", str(args.radius)]).decode("utf-8").split("\n")[1]
        ord = list(ord.split(" "))
        return ord
    except Exception as e:
        print(e)
        return None

def get_lowerbound(args):
    try:
        ord = subprocess.check_output(["./src/LowerBound", "--in", args.graphpath, "--rad", str(args.radius)]).decode("utf-8").split("\n")
        ret = 0
        for x in ord:
            if x.strip() == "": continue
            ret = max(ret, int(x.split(": ")[1]))
        return ret
    except Exception as e:
        print(e)
        return None    
    
def reduction_rule(graph: nx.Graph, lowerbound, radius):
    ordend = deque()
    print(f"Starting nodes: {(graph.number_of_nodes())}")
    print(f"Lower bound: {lowerbound}")
    removednode = True
    num_nodes = 0
    
    while removednode:
        removednode = False
        for node in list(graph.nodes):
            if graph.degree(node) == 1 and nx.bfs_tree(graph, node, depth_limit=radius).number_of_nodes() <= lowerbound:
                graph.remove_node(node)
                ordend.appendleft(node)
                num_nodes+=1
                removednode = True
                
    for node in graph.nodes:
        graphnew = graph.copy()
        graphnew.remove_node(node)
        canremove = True
        for neigh in graph.neighbors(node):
            if neigh == node: continue
            num_reachable = nx.bfs_tree(graphnew, neigh, depth_limit=radius).number_of_nodes()+1
            if num_reachable > lowerbound:
                canremove = False
        if canremove:
            print(f"Can remove {node}")
        
    print(f"Number of removed: {num_nodes}")
    return list(ordend)

def main():
    args = parse_args()
    
    ordinitial = get_initial_solution(args)
    graph: nx.Graph = nx.read_adjlist(args.graphpath)
    ordend = reduction_rule(graph, get_lowerbound(args), args.radius)
    nodeset = set(graph.nodes)
    nodelist = list(graph.nodes)
    model = gp.Model()
    startt = time.time()
    keys = []
    for i, u in enumerate(nodelist):
        for v in nodelist[i+1:]:
            keys.append((u,v))
    ordvar = model.addVars(keys, vtype=GRBpy.BINARY)
    if ordinitial is not None:
        index = {}
        for i,u in enumerate(ordinitial):
            index[u]=i
        for u,v in keys:
            if index[u]<index[v]:
                ordvar[u,v].Start = 1
            else:
                ordvar[u,v].Start = 0
    wreachvars = model.addVars(it.product(nodeset, nodeset, range(args.radius+1)),vtype=GRBpy.BINARY)
    ans = model.addVar(vtype = GRBpy.INTEGER)
    ordvarset = model.addVars(nodeset, vtype=GRBpy.INTEGER)
    for v in nodeset:
        model.addConstr(ordvarset[v]>=1)
        model.addConstr(ordvarset[v]<=len(nodeset))
    for u,v in keys:
        model.addConstr((ordvar[u,v] == 1) >> (ordvarset[u]+1<=ordvarset[v]))
        model.addConstr((ordvar[u,v] == 0) >> (ordvarset[u]>=ordvarset[v]+1))
    
    for u in nodelist:
        model.addConstr(wreachvars[u,u,0]==1)
        for v in nodelist:
            for r in range(1, args.radius+1):
                model.addConstr(wreachvars[u,v,r]>=wreachvars[u,v,r-1])
    for v,w in graph.edges:
        for u in nodelist:                      
            for r in range(args.radius):                
                # wreachvars(u,v,r) and ordvar[u,w] -> u in wreach_r+1(w)
                if u != w:                   
                    if (u,w) in ordvar:                                            
                        model.addConstr(wreachvars[u,v,r]+ordvar[u,w]<=1+wreachvars[u,w,r+1])
                    else:                        
                        model.addConstr(wreachvars[u,v,r]+(1-ordvar[w,u])<=1+wreachvars[u,w,r+1])
        v,w = w,v
        for u in nodelist:                      
            for r in range(args.radius):                
                # wreachvars(u,v,r) and ordvar[u,w] -> u in wreach_r+1(w)
                if u != w:                   
                    if (u,w) in ordvar:                                            
                        model.addConstr(wreachvars[u,v,r]+ordvar[u,w]<=1+wreachvars[u,w,r+1])
                    else:                        
                        model.addConstr(wreachvars[u,v,r]+(1-ordvar[w,u])<=1+wreachvars[u,w,r+1])
                    
                
    for u in nodelist:
        model.addConstr(ans>=gp.quicksum(wreachvars[v,u,args.radius] for v in nodelist))
        
    def mycallback(model, where):
        if where == GRBpy.Callback.MIPSOL:
            print("Integer solution: ", model.cbGetSolution(model._ans))
    
    
    model.setObjective(ans, sense = GRBpy.MINIMIZE)
    model._ans = ans
    model.optimize(mycallback)
    if model.status == GRBpy.OPTIMAL:
        vals = model.getAttr("X", ordvar)
        vals2 = model.getAttr("X", ordvarset)
        print(vals2)
        ordering = [0 for _ in range(len(nodelist))]
        for u in nodelist:
            pos = -1
            for v in nodelist:
                if ((v,u) in vals and vals[v,u] >= 0.5) or ((u,v) in vals and vals[u,v]<0.5) or u==v:
                    pos+=1
            ordering[pos] = u
        ordering = ordering+ordend
        filename = os.path.basename(args.graphpath)
        with open(f"outilp/res_{args.radius}_{filename}.txt", "w") as f:
            f.write("Ordering: " + " ".join(map(str, ordering)) + "\n")
            f.write("Time: " + str(time.time()-startt) + "\n")
            f.write("Weak coloring number: " + str(ans.X) + "\n")
            
        print("Ordering: ", *ordering)
        print("Weak coloring number:", ans.X)
        assert(verify(args.graphpath, ordering, args.radius, int(ans.X)))  

if __name__ == "__main__":
    main()