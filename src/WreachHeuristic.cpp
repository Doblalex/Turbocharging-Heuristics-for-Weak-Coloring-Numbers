#include "FlagParser.hpp"
#include "Headers.hpp"
#include "ReadTxt.hpp"
#include "Turbocharging.hpp"
#include "Ordering.hpp"

namespace po = boost::program_options;
unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

void Usage()
{
    cerr << "Usage: ./WreachHeuristic --in graph.txtg --rad radius [TURBOCHARGING] [--draw]" << endl;
    cerr << "where TURBOCHARGING is one of the following options" << endl;
    cerr << "--turbochargeLASTC --c c --k k [--only-reorder] [--opt-components] [--lower-bound]" << endl;
    cerr << "--turbochargeRNeigh --c c --k k [--only-reorder] [--opt-components] [--lower-bound] [--ordered-adj] [--ignore-right]" << endl;
    cerr << "--turbochargeWreach --c c --k k [--only-reorder] [--opt-components] [--lower-bound] [--ordered-adj] [--ignore-right]" << endl;
    cerr << "--turbochargeSwapN --k k [--opt-components] [--lower-bound] [--ordered-adj] [--ignore-right]" << endl;
    cerr << "--turbochargeSwapLS --k k [--opt-components] [--lower-bound] [--ordered-adj] [--ignore-right]" << endl;
    cerr << "--turbochargeMerge --k k --c c [--opt-components] [--lower-bound] [--ordered-adj] [--ignore-right]" << endl;
    cerr << "--h for help" << endl;
}

void Err()
{
    Usage();
    exit(1);
}

po::variables_map ParseArgs(int argc, char **argv)
{
    po::variables_map vm;
    try
    {
        po::options_description desc("Allowed options");
        desc.add_options()("help", "produce help message");
        desc.add_options()("in", po::value<string>()->required(), "set graph input file");
        desc.add_options()("rad", po::value<int>()->required(), "set radius");
        desc.add_options()("k", po::value<int>(), "set target value k for turbocharging");
        desc.add_options()("c", po::value<int>(), "set backtracking steps c for turbocharging");
        desc.add_options()("draw", po::bool_switch()->default_value(false), "draw turbocharging subproblems");
        desc.add_options()("only-reorder", po::bool_switch()->default_value(false), "only reorder turbocharged vertices (do not use all)");
        desc.add_options()("opt-components", po::bool_switch()->default_value(false), "next vertex has to in ordering is always from connected component of last vertex");
        desc.add_options()("lower-bound", po::bool_switch()->default_value(false), "apply some lower bounds");
        desc.add_options()("ordered-adj", po::bool_switch()->default_value(false), "Keep track of ordered adjacency list for breath-first-search to the right");
        desc.add_options()("turbochargeLASTC", po::bool_switch()->default_value(false), "turbocharge last c vertices placed");
        desc.add_options()("turbochargeRNeigh", po::bool_switch()->default_value(false), "turbocharge c random vertices of r-neighbourhood");
        desc.add_options()("turbochargeWreach", po::bool_switch()->default_value(false), "turbocharge c random vertices of wreach");
        desc.add_options()("turbochargeSwapN", po::bool_switch()->default_value(false), "turbocharge by swapping neighbouring vertices prioritizing too full");
        desc.add_options()("turbochargeSwapLS", po::bool_switch()->default_value(false), "turbocharge by swapping vertices that are too full back randomly");
        desc.add_options()("turbochargeMerge", po::bool_switch()->default_value(false), "turbocharge by trying to merge parts of the weakly reachable sets with rest of ordering");
        desc.add_options()("ignore-right", po::bool_switch()->default_value(false), "ignore full weakly reachable sets of vertices that are not placed during turbocharging");

        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            std::cout << desc << "\n";
            exit(0);
        }

        po::notify(vm);
        if (vm["rad"].as<int>() <= 0)
        {
            cerr << "Radius has to be greater than 0!" << endl;
            Err();
        }

        if (vm.count("k") && vm["k"].as<int>() < 1)
        {
            cerr << "Target value k has to be greater than 0!" << endl;
            Err();
        }
        if (vm.count("c") && vm["c"].as<int>() < 1)
        {
            cerr << "Target value c has to be greater than 0!" << endl;
            Err();
        }
        bool turbocharging_active = vm["turbochargeLASTC"].as<bool>() || vm["turbochargeRNeigh"].as<bool>() || vm["turbochargeSwapN"].as<bool>() || vm["turbochargeSwapLS"].as<bool>() || vm["turbochargeWreach"].as<bool>() || vm["turbochargeMerge"].as<bool>();
        bool turbocharging_needs_c = vm["turbochargeLASTC"].as<bool>() || vm["turbochargeRNeigh"].as<bool>() || vm["turbochargeWreach"].as<bool>() || vm["turbochargeMerge"].as<bool>();
        bool turbocharging_ordered_adjacency_possible = turbocharging_active && !vm["turbochargeLASTC"].as<bool>();
        if (vm["ordered-adj"].as<bool>() && !turbocharging_ordered_adjacency_possible)
        {
            Err();
        }
        if (vm["ignore-right"].as<bool>() && !turbocharging_ordered_adjacency_possible)
        {
            Err();
        }
        if (vm.count("k") ^ turbocharging_active)
        {
            Err();
        }
        if (vm["lower-bound"].as<bool>() && !turbocharging_active)
        {
            Err();
        }
        if (vm.count("c") ^ turbocharging_needs_c)
        {
            Err();
        }
        int tcccounter = 0;
        tcccounter += vm["turbochargeLASTC"].as<bool>() ? 1 : 0;
        tcccounter += vm["turbochargeRNeigh"].as<bool>() ? 1 : 0;
        tcccounter += vm["turbochargeWreach"].as<bool>() ? 1 : 0;
        tcccounter += vm["turbochargeSwapN"].as<bool>() ? 1 : 0;
        tcccounter += vm["turbochargeSwapLS"].as<bool>() ? 1 : 0;
        tcccounter += vm["turbochargeMerge"].as<bool>() ? 1 : 0;
        if (tcccounter > 1)
        {
            Err();
        }
    }
    catch (exception &e)
    {
        cerr << "error: " << e.what() << "\n";
        Err();
    }
    catch (...)
    {
        cerr << "Exception of unknown type!\n";
        Err();
    }
    return vm;
}

int main(int argc, char **argv)
{
    po::variables_map vm = ParseArgs(argc, argv);
    BranchingRule branching_rule;
    int times_turbocharged = 0;
    chrono::duration<double, std::milli> time_turbocharging = std::chrono::duration<double, std::milli>(0);
    if (vm["turbochargeLASTC"].as<bool>())
    {
        branching_rule = ByDistance;
    }
    else if (vm["turbochargeRNeigh"].as<bool>() || vm["turbochargeWreach"].as<bool>())
    {
        branching_rule = RandomOrder;
    }
    bool opt_components = vm["opt-components"].as<bool>();
    bool lower_bound = vm["lower-bound"].as<bool>();
    int radius = vm["rad"].as<int>();
    bool turbocharging_active = vm["turbochargeLASTC"].as<bool>() || vm["turbochargeRNeigh"].as<bool>() || vm["turbochargeSwapN"].as<bool>() || vm["turbochargeSwapLS"].as<bool>() || vm["turbochargeWreach"].as<bool>() || vm["turbochargeMerge"].as<bool>();
    int target_k = INT_MAX;
    int c;
    if (turbocharging_active)
    {
        target_k = vm["k"].as<int>();
    }
    if (vm["turbochargeLASTC"].as<bool>() || vm["turbochargeRNeigh"].as<bool>() || vm["turbochargeWreach"].as<bool>() || vm["turbochargeMerge"].as<bool>())
    {
        c = vm["c"].as<int>();
    }

    GraphReader reader;
    OrderedGraph graph = reader.ReadGraph(vm["in"].as<string>());
    int n = boost::num_vertices(graph);
    vector<vector<int>> distance_matrix;
    Ordering ordering(graph, radius, vm["turbochargeLASTC"].as<bool>() && !vm["only-reorder"].as<bool>(), lower_bound, vm["ordered-adj"].as<bool>(), vm["ignore-right"].as<bool>(), opt_components);

    ordering.target_k = target_k;

    if (branching_rule == ByDistance)
    {
        distance_matrix = vector<vector<int>>(n, vector<int>(n, 0));
        boost::johnson_all_pairs_shortest_paths(graph, distance_matrix);
    }

    auto cmp = [&graph](pair<int, int> v_wrsize1, pair<int, int> v_wrsize2)
    {
        int v1 = v_wrsize1.first;
        int v2 = v_wrsize2.first;
        int wreach_sz1 = v_wrsize1.second;
        int wreach_sz2 = v_wrsize2.second;
        if (wreach_sz1 != wreach_sz2)
        {
            return wreach_sz1 > wreach_sz2;
        }
        if (boost::degree(v1, graph) != boost::degree(v2, graph))
        {
            return boost::degree(v1, graph) > boost::degree(v2, graph);
        }
        return v1 < v2;
    };
    // save pairs of vertices and sizes of their weakly reachable sets because
    // weakly reachable sets can change during turbocharging or calls in ordering class
    set<pair<int, int>, decltype(cmp)> pq(cmp);
    vector<int> wreach_in_pq(n, 1);

    for (auto v : boost::make_iterator_range(boost::vertices(graph)))
    {
        pq.insert({v, 1}); // n log n insert into priority queue
    }
    auto start = std::chrono::high_resolution_clock::now();

    // std::cout << ordering.LBDegeneracy() << endl;
    // std::cout << ordering.LBDegeneracyContract() << endl;
    // std::cout << ordering.LBDegeneracyContractSubGraph() << endl;
    while (!ordering.IsValidFull())
    {
        int vertex;
        if (opt_components)
        {
            int vcomponent = -1;
            if (ordering.at > 0)
            {
                int v = ordering.ordering[ordering.at - 1];
                for (auto w : boost::make_iterator_range(boost::adjacent_vertices(v, graph)))
                {
                    if (graph[w].position == NOT_PLACED)
                    {
                        vcomponent = w;
                        break;
                    }
                }
            }
            if (vcomponent == -1)
            {
                vcomponent = *ordering.vertices_active.begin();
            }
            if (vcomponent != -1)
            {
                vector<int> component = ordering.ComponentNotPlaced(vcomponent);
                unordered_set<int> scomponent(component.begin(), component.end());
                auto it = pq.begin();
                while (scomponent.find(it->first) == scomponent.end())
                {
                    it++;
                }
                vertex = it->first;
            }
            else
                vertex = pq.begin()->first;
        }
        else
        {
            pair<int, int> v_wrsize = *pq.begin();
            vertex = v_wrsize.first; // vertex with maximal weakly reachable set
        }
        ordering.Place(vertex);
        for (auto v : ordering.changed)
        {
            int wrsize_pq = wreach_in_pq[v];
            if (wrsize_pq != -1)
            {
                // remove current entry in priority queue
                pq.erase({v, wrsize_pq});
                wreach_in_pq[v] = -1;
            }
            if (graph[v].position == NOT_PLACED)
            {
                // reinsert if not placed
                pq.insert({v, graph[v].wreach_sz});
                wreach_in_pq[v] = graph[v].wreach_sz;
            }
        }
        ordering.changed.clear();

        if (turbocharging_active && !ordering.IsExtendable())
        {
            times_turbocharged += 1;
            bool succ = false;
            auto start_turbocharging = std::chrono::high_resolution_clock::now();
            if (vm["turbochargeLASTC"].as<bool>())
            {
                TurbochargerLastC TC(ordering, graph, distance_matrix);
                succ = TC.Turbocharge(c, vm["only-reorder"].as<bool>(), vm["draw"].as<bool>(), branching_rule);
            }
            else if (vm["turbochargeRNeigh"].as<bool>())
            {
                TurbochargerRNeigh TC(ordering, graph, distance_matrix);
                succ = TC.Turbocharge(c, vm["only-reorder"].as<bool>(), vm["draw"].as<bool>(), branching_rule);
            }
            else if (vm["turbochargeWreach"].as<bool>())
            {
                TurbochargerWreach TC(ordering, graph, distance_matrix);
                succ = TC.Turbocharge(c, vm["only-reorder"].as<bool>(), vm["draw"].as<bool>(), branching_rule);
            }
            else if (vm["turbochargeSwapN"].as<bool>())
            {
                TurbochargerSwapNeighbours TC(ordering, graph);
                succ = TC.Turbocharge(vm["draw"].as<bool>());
            }
            else if (vm["turbochargeSwapLS"].as<bool>())
            {
                TurbochargerSwapLocalSearch TC(ordering, graph);
                succ = TC.Turbocharge(vm["draw"].as<bool>());
            }
            else if (vm["turbochargeMerge"].as<bool>())
            {
                TurbochargerMerge TC(ordering, graph);
                succ = TC.Turbocharge(vm["draw"].as<bool>(), c);
            }
            if (succ)
            {
                for (auto v : ordering.changed)
                {
                    int wrsize_pq = wreach_in_pq[v];
                    if (wrsize_pq != -1)
                    {
                        // remove current entry in priority queue
                        pq.erase({v, wrsize_pq});
                        wreach_in_pq[v] = -1;
                    }
                    if (graph[v].position == NOT_PLACED)
                    {
                        // reinsert if not placed
                        pq.insert({v, graph[v].wreach_sz});
                        wreach_in_pq[v] = graph[v].wreach_sz;
                    }
                }
                ordering.changed.clear();
            }
            else
            {
                auto end_turbocharging = std::chrono::high_resolution_clock::now();
                chrono::duration<double, std::milli> ms_double = end_turbocharging - start_turbocharging;
                time_turbocharging += ms_double;
                cout << "No success" << endl;
                cout << "Failed at position " << ordering.at << " from " << n << endl;
                /* Getting number of milliseconds as an integer. */
                auto end = std::chrono::high_resolution_clock::now();
                auto ms_int = std::chrono::duration_cast<chrono::milliseconds>(end - start);

                /* Getting number of milliseconds as a double. */
                ms_double = end - start;
                cout << ms_double.count() << "ms" << endl;
                cout << "Times turbocharged: " << times_turbocharged << endl;
                cout << "Time turbocharging: " << time_turbocharging.count()<<"ms" <<endl;
                exit(0);
            }
            auto end_turbocharging = std::chrono::high_resolution_clock::now();
            chrono::duration<double, std::milli> ms_double = end_turbocharging - start_turbocharging;
            time_turbocharging += ms_double;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    cout << ordering.Wreach() << endl;
    ordering.PrintOrdering();
    /* Getting number of milliseconds as an integer. */
    auto ms_int = std::chrono::duration_cast<chrono::milliseconds>(end - start);

    /* Getting number of milliseconds as a double. */
    chrono::duration<double, std::milli> ms_double = end - start;
    cout << ms_double.count() << "ms" << endl;
    cout << "Times turbocharged: " << times_turbocharged << endl;
    cout << "Time turbocharging: " << time_turbocharging.count()<<"ms" <<endl;
}