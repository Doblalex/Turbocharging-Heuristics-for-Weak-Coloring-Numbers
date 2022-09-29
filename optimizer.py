#!/usr/bin/env python3.7
from os import listdir
from os.path import isfile, join
import subprocess
from collections import defaultdict
import networkx as nx
import signal
import argparse
import json
from datetime import datetime
import os
import sys

cwdhere = "."


def parse_args():
    parser = argparse.ArgumentParser(
        "Computing ordering of small weak r-coloring number for a graph")
    parser.add_argument("-heur", "--heuristic", required=True,
                        choices=["WreachHeuristic", "DegreeHeuristic", "DegreeHeuristicRL", "SreachHeuristic"])
    parser.add_argument("-tc", "--turbocharging", required=True,
                        choices=["Merge", "LASTC", "SwapN", "RNeigh", "Wreach", "SwapLS"])
    parser.add_argument("-g", "--graphpath", required=True, type=str)
    parser.add_argument("-t", "--timeout", default=60, type=int)
    parser.add_argument("-r", "--radius", type=int, required=True)
    parser.add_argument("-opt", "--options", nargs="+", required=False,
                        choices=["lower-bound", "only-reorder", "opt-components", "ordered-adj", "ignore-right"])
    parser.add_argument("-out", "--output", required=False, type=str)
    args = parser.parse_args()
    return args


def handler(signum, frame):
    raise TimeoutError("end of time")


outputobj = {}
outputobj["improvements"] = []
outputobj["nonimprovements"] = []


def decode_out(out):
    out = out.decode("utf-8")
    tc_tracker = []
    for l in out.split("\n")[5:]:
        if l.strip() == "": continue
        cnt_nodes, cnt_dephts, sum_depth, at, n, time = l.strip().split(" ")
        tc_tracker.append({"cnt_nodes": int(cnt_nodes), "cnt_dephts": int(cnt_dephts), "sum_depth": int(sum_depth), "at": int(at), "n": int(n), "time": float(time)})
    if "No success" in out:
        return None, None, float(
            out.split("\n")[2].strip().strip("ms")), int(out.split("\n")[3].strip("Times turbocharged: ").strip()), float(out.split("\n")[4].strip("Time turbocharging: ").strip("ms").strip()), tc_tracker
    ordering = out.split("\n")[1].strip().split(" ")
    colnumber = int(out.split("\n")[0])
    runtime = float(
        out.split("\n")[2].strip().strip("ms"))
    times_turbocharged = int(out.split("\n")[3].strip(
        "Times turbocharged: ").strip())
    time_turbocharging = float(out.split("\n")[4].strip(
        "Time turbocharging: ").strip("ms").strip())
    return colnumber, ordering, runtime, times_turbocharged, time_turbocharging, tc_tracker


def print_improvement(colnumber, ordering, starttime: datetime, c, times_turbocharged, time, time_turbocharging, tc_tracker):
    print("new best ordering")
    print("weak r-coloring number: {}".format(colnumber))
    print("ordering: {}".format(' '.join(map(str, ordering))))
    print()
    outputobj["improvements"].append({"totaltime": (datetime.now()-starttime).total_seconds(), "c": c, "ordering": list(map(
        int, ordering)), "k": colnumber, "times_turbocharged": times_turbocharged, "time": time, "time_turbocharging": time_turbocharging, "tc_tracker": tc_tracker})


def non_improvement(colnumber, starttime: datetime, c, times_turbocharged, time, time_turbocharging, tc_tracker):
    outputobj["nonimprovements"].append({"totaltime": (datetime.now()-starttime).total_seconds(), "c": c, "k": colnumber,
                                        "times_turbocharged": times_turbocharged, "time": time, "time_turbocharging": time_turbocharging, "tc_tracker": tc_tracker})


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

if __name__ == "__main__":
    args = parse_args()
    tc = "--turbocharge" + args.turbocharging
    timeout = args.timeout  # seconds
    options = []
    if args.options:
        options = args.options
    options_prog = ["./src/"+args.heuristic]+["--"+opt for opt in options]
    options_prog.extend([tc])
    options_prog.extend(["--rad", str(args.radius)])
    options_prog.extend(["--in", args.graphpath])
    options_prog.extend(["--verbose"])
    outputobj["file"] = os.path.basename(args.graphpath)

    graph = nx.read_adjlist(args.graphpath, create_using=nx.Graph)
    time = 0.0
    starttime = datetime.now()
    try:
        signal.signal(signal.SIGALRM, handler)
        signal.setitimer(signal.ITIMER_REAL, float(timeout))
        out = subprocess.check_output(
            ["./src/" + args.heuristic, "--in", args.graphpath, "--rad", str(args.radius)], cwd=cwdhere)
        colnumber, ordering, runtime, times_turbocharged, time_turbocharging, tc_tracker = decode_out(
            out)
        if not verify(args.graphpath, ordering, args.radius, colnumber):
            assert False
        print_improvement(colnumber, ordering, starttime,
                          0, times_turbocharged, runtime, time_turbocharging, tc_tracker)
        while True:
            colnumber -= 1
            c = 1
            while True:
                outputobj["c_timeout"] = c
                if c > graph.number_of_nodes():
                    raise TimeoutError("end of time")
                print("conservation parameter: ", c)
                if args.turbocharging in ["SwapLS", "SwapN"]:
                    out = subprocess.check_output(
                        options_prog+["--k", str(colnumber)], cwd=cwdhere)
                else:
                    out = subprocess.check_output(
                        options_prog+["--c", str(c), "--k", str(colnumber)], cwd=cwdhere)
                colnumber_n, ordering_n, runtime, times_turbocharged, time_turbocharging, tc_tracker = decode_out(
                    out)
                if ordering_n != None:
                    colnumber = colnumber_n
                    ordering = ordering_n
                    break
                non_improvement(colnumber, starttime, c,
                                times_turbocharged, runtime, time_turbocharging, tc_tracker)
                c += 1
            if not verify(args.graphpath, ordering, args.radius, colnumber):
                assert False
            print_improvement(colnumber, ordering, starttime,
                              c, times_turbocharged, runtime, time_turbocharging, tc_tracker)
    except TimeoutError:
        if args.output is not None:
<<<<<<< HEAD
            os.makedirs(os.path.dirname(args.output), exist_ok=True)
=======
>>>>>>> origin/main
            with open(args.output, "w") as f:
                f.write(json.dumps(outputobj))
        print("Timeout")
    except Exception as e:
        outputobj["error"] = str(e)
        if args.output is not None:
            os.makedirs(os.path.dirname(args.output), exist_ok=True)
            with open(args.output, "w") as f:
                f.write(json.dumps(outputobj))
        print(args.graphpath, e, file=sys.stderr)
