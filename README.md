# Turbocharging Heuristics for Weak Coloring Numbers

All relevant source files are in the `src/`-folder
To compile the source files, the following steps are required:
- Install a version of the Boost C++ library 
- Change your boost root folder in `src/Makefile`
- Go to the `src/`-folder and call `make` 

This will create executable versions of all the heuristics (`WreachHeuristic`, `DegreeHeuristic`, `SreachHeuristic`, and `DegreeHeuristicRL`)

Call these executables with the option `--help` to be able to see optional parameters and how to enable turbocharging of these heuristics.

The script `optimizer.py` takes a turbocharged heuristic and iteratively tries to compute better orderings with smaller weak r-coloring number for a graph and a radius.

All graph inputs are given as files containing adjacency lists, where each line corresponds to an undirected edge between two vertices. Vertices should be given as integers. Example (Path of length 3):
```
1 2
2 3
3 4
```