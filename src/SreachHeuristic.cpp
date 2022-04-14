#include "FlagParser.hpp"
#include "Headers.hpp"
#include "ReadTxt.hpp"
#include "TurbochargingRL.hpp"
#include "OrderingRL.hpp"

namespace po = boost::program_options;
unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

void Usage()
{
    cerr << "Usage: ./SreachHeuristic --in graph.txtg --rad radius [TURBOCHARGING]" << endl;
    cerr << "where TURBOCHARGING is one of the following options" << endl;
    cerr << "--turbochargeLASTC --c c --k k [--lower-bound]" << endl;
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
        desc.add_options()("lower-bound", po::bool_switch()->default_value(false), "use lower-bounding");
        desc.add_options()("turbochargeLASTC", po::bool_switch()->default_value(false), "turbocharge last c vertices placed");
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

        bool turbocharging_active = vm["turbochargeLASTC"].as<bool>();
        bool turbocharging_needs_c = turbocharging_active;
        if (vm.count("k") ^ turbocharging_active)
        {
            Err();
        }
        if (vm.count("c") ^ turbocharging_needs_c)
        {
            Err();
        }
        if (vm["lower-bound"].as<bool>() && !turbocharging_active)
        {
            Err();
        }
        int tcccounter = 0;
        tcccounter += vm["turbochargeLASTC"].as<bool>() ? 1 : 0;
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
    int times_turbocharged = 0;
    chrono::duration<double, std::milli> time_turbocharging = std::chrono::duration<double, std::milli>(0);
    po::variables_map vm = ParseArgs(argc, argv);
    int radius = vm["rad"].as<int>();
    bool turbocharging_active = vm["turbochargeLASTC"].as<bool>();
    int target_k = INT_MAX;
    int c;
    if (turbocharging_active)
    {
        target_k = vm["k"].as<int>();
        c = vm["c"].as<int>();
    }

    GraphReader reader;
    OrderedGraph graph = reader.ReadGraph(vm["in"].as<string>());
    OrderedGraph graph_copy(boost::num_vertices(graph));
    BGL_FORALL_VERTICES(v, graph_copy, OrderedGraph)
    {
        graph_copy[v].position = NOT_PLACED;
        graph_copy[v].wreach_sz = 1;
    }
    BGL_FORALL_EDGES(e, graph, OrderedGraph)
    {
        int u = boost::source(e, graph);
        int v = boost::target(e, graph);
        boost::add_edge(u, v, 1, graph_copy);
    }
    int n = boost::num_vertices(graph);
    vector<vector<int>> distance_matrix;
    OrderingRL ordering(graph, radius, target_k, vm["lower-bound"].as<bool>(), vm["lower-bound"].as<bool>());
    OrderingRL ordering_for_pq(graph_copy, radius, target_k, true, true);

    auto cmp = [&graph](pair<int, int> v_srsize1, pair<int, int> v_srsize2)
    {
        int v1 = v_srsize1.first;
        int v2 = v_srsize2.first;
        int sreach_sz1 = v_srsize1.second;
        int sreach_sz2 = v_srsize2.second;
        if (sreach_sz1 != sreach_sz2)
        {
            return sreach_sz1 < sreach_sz2;
        }
        if (boost::degree(v1, graph) != boost::degree(v2, graph))
        {
            return boost::degree(v1, graph) < boost::degree(v2, graph);
        }
        return v1 < v2;
    };

    vector<int> sreach_in_pq(n);
    set<pair<int, int>, decltype(cmp)> pq(cmp);
    BGL_FORALL_VERTICES(v, graph, OrderedGraph)
    {
        pq.insert({v, ordering_for_pq.sreach_wo_wreach[v].size()});
        sreach_in_pq[v] = ordering_for_pq.sreach_wo_wreach[v].size();
    }

    auto start = std::chrono::high_resolution_clock::now();
    while (!ordering.IsValidFull())
    {
        pair<int, int> entry = *pq.begin();
        int vertex = entry.first;
        ordering.Place(vertex);
        ordering_for_pq.Place(vertex);

        for (auto v : ordering_for_pq.changed)
        {
            int srsize_pq = sreach_in_pq[v];
            if (srsize_pq != -1)
            {
                // remove current entry in priority queue
                pq.erase({v, srsize_pq});
                sreach_in_pq[v] = -1;
            }
            if (graph[v].position == NOT_PLACED)
            {
                // reinsert if not placed
                pq.insert({v, ordering_for_pq.sreach_wo_wreach[v].size()});
                sreach_in_pq[v] = ordering_for_pq.sreach_wo_wreach[v].size();
            }
        }
        ordering_for_pq.changed.clear();

        if (turbocharging_active && !ordering.IsExtendable())
        {
            times_turbocharged += 1;
            bool succ = false;
            auto start_turbocharging = std::chrono::high_resolution_clock::now();
            if (vm["turbochargeLASTC"].as<bool>())
            {
                TurbochargerLastC TC(ordering, graph);
                succ = TC.Turbocharge(c);
            }
            if (succ)
            {
                int cactual = min(c, (int)ordering.ordering.size() - ordering.at - 1);
                for (int i = 0; i < cactual; i++)
                {
                    ordering_for_pq.UnPlace();
                }
                for (int i = 0; i < cactual; i++)
                {
                    ordering_for_pq.Place(ordering.ordering[ordering_for_pq.at]);
                }
                for (auto v : ordering_for_pq.changed)
                {
                    int srsize_pq = sreach_in_pq[v];
                    if (srsize_pq != -1)
                    {
                        // remove current entry in priority queue
                        pq.erase({v, srsize_pq});
                        sreach_in_pq[v] = -1;
                    }
                    if (graph[v].position == NOT_PLACED)
                    {
                        // reinsert if not placed
                        pq.insert({v, ordering_for_pq.sreach_wo_wreach[v].size()});
                        sreach_in_pq[v] = ordering_for_pq.sreach_wo_wreach[v].size();
                    }
                }
                ordering_for_pq.changed.clear();
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