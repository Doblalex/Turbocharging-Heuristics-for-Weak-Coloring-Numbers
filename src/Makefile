EXECS=WreachHeuristic DegreeHeuristic DegreeHeuristicRL SreachHeuristic LowerBound

OPTFLAGS=-O3 -std=c++17
OPTLOCALFLAGS= -DLOCAL -std=c++17 -O3
DBGFLAGS= -DLOCAL -lubsan -std=c++17 -fsanitize=address -g -fsanitize=undefined -pedantic -D_GLIBCXX_DEBUG -Wall -Wextra -Wfloat-equal -Wlogical-op -Wshadow -Wunused
#CXXFLAGS= -DLOCAL -lubsan -std=c++11 -fsanitize=address -g -fsanitize=undefined -pedantic -D_GLIBCXX_DEBUG -Wall -Wextra -Wfloat-equal -Wlogical-op -Wshadow -Wunused
CXXFLAGS=-DLOCAL -std=c++17 -O3
#LDFLAGS += -fsanitize=address -fsanitize=undefined

# Boost C++ library
ROOT_BOOST = /lib/x86_64-linux-gnu

LDFLAGS += -L$(ROOT_BOOST) -lboost_program_options -lboost_system -lboost_graph
SOURCES=$(wildcard *.cpp)
DEPENDS=$(patsubst %.cpp,.%.depends,$(SOURCES))
OBJECTS=$(patsubst %.cpp,%.o,$(SOURCES))
CXX=g++-11

debug: CXXFLAGS = -DDEBUG -g -DLOCAL -std=c++17
debug: OPTFLAGS = std=c++17
debug: OPTLOCALFLAGS = -DLOCAL -std=c++17
debug: all

all: $(EXECS)

WreachHeuristic: WreachHeuristic.o ReadTxt.o FilesOps.o FlagParser.o Turbocharging.o Ordering.o DSU.o
	$(CXX) $^ -o $@ $(LDFLAGS) 

DegreeHeuristic: DegreeHeuristic.o ReadTxt.o FilesOps.o FlagParser.o Turbocharging.o Ordering.o DSU.o
	$(CXX) $^ -o $@ $(LDFLAGS) 

DegreeHeuristicRL: DegreeHeuristicRL.o ReadTxt.o FilesOps.o FlagParser.o TurbochargingRL.o OrderingRL.o DSU.o
	$(CXX) $^ -o $@ $(LDFLAGS) 

SreachHeuristic: SreachHeuristic.o ReadTxt.o FilesOps.o FlagParser.o TurbochargingRL.o OrderingRL.o DSU.o
	$(CXX) $^ -o $@ $(LDFLAGS) 

LowerBound: LowerBound.o ReadTxt.o FilesOps.o FlagParser.o Turbocharging.o Ordering.o DSU.o
	$(CXX) $^ -o $@ $(LDFLAGS) 

.%.depends: %.cpp
	$(CXX) $(OPTFLAGS) -MM $< -o $@ $(CXXFLAGS)

.PHONY: clean
clean:
	rm -f *.o $(EXECS) $(DEPENDS)

-include $(DEPENDS)
