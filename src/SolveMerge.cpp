#include "FlagParser.hpp"
#include "Headers.hpp"
#include "ReadTxt.hpp"
#include "Turbocharging.hpp"
#include "Ordering.hpp"

namespace po = boost::program_options;
unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

void Usage()
{
    cerr << "Usage: ./SolveMerge --in graph.txtg --rad radius --k k" << endl;
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
        desc.add_options()("k", po::value<int>()->required(), "set target value k for turbocharging");

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
    int radius = vm["rad"].as<int>();
    int target_k = INT_MAX;
    target_k = vm["k"].as<int>();

    GraphReader reader;
    OrderedGraph graph = reader.ReadGraph(vm["in"].as<string>());
    int n = boost::num_vertices(graph);
    vector<vector<int>> distance_matrix;
    Ordering ordering(graph, radius, false, false, false, false, false);
    
    ordering.target_k = target_k;   
        
    TurbochargerMerge TC(ordering, graph);
    bool succ = TC.Turbocharge(false, boost::num_vertices(graph));
    cout<<succ<<endl;
    cout << ordering.Wreach() << endl;
    ordering.PrintOrdering();
}