from os import listdir
from os.path import isfile, join
import subprocess
from collections import defaultdict
import networkx as nx
import signal
import argparse
import json
from datetime import datetime


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
    if "No success" in out:
        return None, None, float(
            out.split("\n")[2].strip().strip("ms")), int(out.split("\n")[3].strip("Times turbocharged: ").strip()), float(out.split("\n")[4].strip("Time turbocharging: ").strip("ms").strip())
    ordering = out.split("\n")[1].strip().split(" ")
    colnumber = int(out.split("\n")[0])
    runtime = float(
        out.split("\n")[2].strip().strip("ms"))
    times_turbocharged = int(out.split("\n")[3].strip(
        "Times turbocharged: ").strip())
    time_turbocharging = float(out.split("\n")[4].strip(
        "Time turbocharging: ").strip("ms").strip())
    return colnumber, ordering, runtime, times_turbocharged, time_turbocharging


def print_improvement(colnumber, ordering, starttime: datetime, c, times_turbocharged, time, time_turbocharging):
    print("new best ordering")
    print("weak r-coloring number: {}".format(colnumber))
    print("ordering: {}".format(' '.join(map(str, ordering))))
    print()
    outputobj["improvements"].append({"totaltime": (datetime.now()-starttime).total_seconds(), "c": c, "ordering": list(map(
        int, ordering)), "k": colnumber, "times_turbocharged": times_turbocharged, "time": time, "time_turbocharging": time_turbocharging})


def non_improvement(colnumber, starttime: datetime, c, times_turbocharged, time, time_turbocharging):
    outputobj["nonimprovements"].append({"totaltime": (datetime.now()-starttime).total_seconds(), "c": c, "k": colnumber,
                                        "times_turbocharged": times_turbocharged, "time": time, "time_turbocharging": time_turbocharging})


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
    options_prog = ["./"+args.heuristic]+["--"+opt for opt in options]
    options_prog.extend([tc])
    options_prog.extend(["--rad", str(args.radius)])
    options_prog.extend(["--in", "../"+args.graphpath])

    time = 0.0
    starttime = datetime.now()
    try:
        signal.signal(signal.SIGALRM, handler)
        signal.setitimer(signal.ITIMER_REAL, float(timeout))
        out = subprocess.check_output(
            ["./" + args.heuristic, "--in", "../"+args.graphpath, "--rad", str(args.radius)], cwd="./src")
        colnumber, ordering, runtime, times_turbocharged, time_turbocharging = decode_out(
            out)
        if not verify(args.graphpath, ordering, args.radius, colnumber):
            assert False
        print_improvement(colnumber, ordering, starttime,
                          0, times_turbocharged, runtime, time_turbocharging)
        while True:
            colnumber -= 1
            c = 1
            while True:
                outputobj["c_timeout"] = c
                print("conservation parameter: ", c)
                if args.turbocharging in ["SwapLS", "SwapN"]:
                    out = subprocess.check_output(
                        options_prog+["--k", str(colnumber)], cwd="./src")
                else:
                    out = subprocess.check_output(
                        options_prog+["--c", str(c), "--k", str(colnumber)], cwd="./src")
                colnumber_n, ordering_n, runtime, times_turbocharged, time_turbocharging = decode_out(
                    out)
                if ordering_n != None:
                    colnumber = colnumber_n
                    ordering = ordering_n
                    break
                non_improvement(colnumber, starttime, c,
                                times_turbocharged, runtime, time_turbocharging)
                c += 1
            if not verify(args.graphpath, ordering, args.radius, colnumber):
                assert False
            print_improvement(colnumber, ordering, starttime,
                              c, times_turbocharged, runtime, time_turbocharging)
    except TimeoutError:
        if "output" in args:
            with open(args.output, "w") as f:
                f.write(json.dumps(outputobj))
        print("Timeout")
    except Exception as e:
        print(e)
