from os import listdir
from os.path import isfile, join
import subprocess
import csv
from collections import defaultdict
import networkx as nx
import signal
import argparse


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
    args = parser.parse_args()
    return args


def handler(signum, frame):
    raise TimeoutError("end of time")


def decode_out(out):
    out = out.decode("utf-8")
    if "No success" in out:
        return None, None, float(
            out.split("\n")[-2].strip().strip("ms"))
    ordering = out.split("\n")[1].strip().split(" ")
    colnumber = int(out.split("\n")[0])
    runtime = float(
        out.split("\n")[-2].strip().strip("ms"))
    return colnumber, ordering, runtime


def print_improvement(colnumber, ordering):
    print("new best ordering")
    print("weak r-coloring number: {}".format(colnumber))
    print("ordering: {}".format(' '.join(map(str, ordering))))
    print()


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
    try:
        signal.signal(signal.SIGALRM, handler)
        signal.setitimer(signal.ITIMER_REAL, float(timeout))
        out = subprocess.check_output(
            ["./" + args.heuristic, "--in", "../"+args.graphpath, "--rad", str(args.radius)], cwd="./src")
        colnumber, ordering, runtime = decode_out(out)
        if not verify(args.graphpath, ordering, args.radius, colnumber):
            assert False
        print_improvement(colnumber, ordering)
        while True:
            colnumber -= 1
            c = 1
            while True:
                print("conservation parameter: ", c)
                if args.turbocharging in ["SwapLS", "SwapN"]:
                    out = subprocess.check_output(
                        options_prog+["--k", str(colnumber)], cwd="./src")
                else:
                    out = subprocess.check_output(
                        options_prog+["--c", str(c), "--k", str(colnumber)], cwd="./src")
                colnumber_n, ordering_n, runtime = decode_out(out)
                if ordering_n != None:
                    colnumber = colnumber_n
                    ordering = ordering_n
                    break
                c += 1
            if not verify(args.graphpath, ordering, args.radius, colnumber):
                assert False
            print_improvement(colnumber, ordering)

    except TimeoutError:
        print("Timeout")
    except Exception as e:
        print(e)
