#include "FlagParser.hpp"
#include "Headers.hpp"
#include "ReadTxt.hpp"
#include "Turbocharging.hpp"
#include "Ordering.hpp"

namespace po = boost::program_options;
unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

void Usage()
{
    cerr << "Usage: ./LowerBound --in graph.txtg --rad radius" << endl;
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
    GraphReader reader;
    OrderedGraph graph = reader.ReadGraph(vm["in"].as<string>());
    int n = boost::num_vertices(graph);
    int radius = vm["rad"].as<int>();

    Ordering ordering = Ordering(graph, radius, false, false, false, false, false);

    cout << "Degeneracy: " << ordering.LBDegeneracy() << endl;
    cout << "WCOL-UB-MMD+: " << ordering.LBDegeneracyContract() << endl;
    cout << "WCOL-MMD+: " << ordering.LBDegeneracyContractSubGraph() << endl;
}