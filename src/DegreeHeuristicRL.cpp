#include "FlagParser.hpp"
#include "Headers.hpp"
#include "ReadTxt.hpp"
#include "TurbochargingRL.hpp"
#include "OrderingRL.hpp"

namespace po = boost::program_options;
unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

void Usage()
{
    cerr << "Usage: ./DegreeHeuristic --in graph.txtg --rad radius [TURBOCHARGING]" << endl;
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
    int n = boost::num_vertices(graph);
    vector<vector<int>> distance_matrix;
    OrderingRL ordering(graph, radius, target_k, vm["lower-bound"].as<bool>(), vm["lower-bound"].as<bool>());

    auto cmp = [&graph](int v1, int v2)
    {
        if (boost::degree(v1, graph) != boost::degree(v2, graph))
        {
            return boost::degree(v1, graph) < boost::degree(v2, graph);
        }
        return v1 > v2;
    };

    set<int, decltype(cmp)> pq(cmp);
    for (auto v : boost::make_iterator_range(boost::vertices(graph)))
    {
        pq.insert(v); // n log n insert into priority queue
    }
    auto start = std::chrono::high_resolution_clock::now();
    while (!ordering.IsValidFull())
    {
        int vertex = *pq.begin();
        ordering.Place(vertex);

        for (auto v : ordering.changed)
        {
            if (graph[v].position != NOT_PLACED && pq.find(v) != pq.end())
            {
                pq.erase(v);
            }
            if (graph[v].position == NOT_PLACED && pq.find(v) == pq.end())
            {
                pq.insert(v);
            }
        }
        ordering.changed.clear();

        if (turbocharging_active && !ordering.IsExtendable())
        {
            times_turbocharged += 1;
            auto start_turbocharging = std::chrono::high_resolution_clock::now();
            bool succ = false;
            if (vm["turbochargeLASTC"].as<bool>())
            {
                TurbochargerLastC TC(ordering, graph);
                succ = TC.Turbocharge(c);
            }
            if (succ)
            {
                for (auto v : ordering.changed)
                {
                    if (pq.find(v) != pq.end() && graph[v].position != NOT_PLACED)
                    {
                        pq.erase(v);
                    }
                    if (graph[v].position == NOT_PLACED && pq.find(v) == pq.end())
                    {
                        pq.insert(v);
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